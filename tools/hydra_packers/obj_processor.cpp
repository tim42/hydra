//
// created by : Timothée Feuillet
// date: 2024-2-2
//
//
// Copyright (c) 2024 Timothée Feuillet
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//

#include <hydra/resources/processor.hpp>
#include <hydra/assets/static_mesh.hpp>

#include "static_mesh_packer.hpp"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>           // Output data structure
#include <assimp/postprocess.h>     // Post processing flags
#include <assimp/defs.h>     // Post processing flags

namespace neam::hydra::processor
{
  struct obj_processor : resources::processor::processor<obj_processor, "file-ext:.obj">
  {
    static constexpr id_t processor_hash = "neam/obj-processor:0.1.0"_rid;

    static resources::processor::chain process_resource(hydra::core_context& /*ctx*/, resources::processor::input_data&& input)
    {
      TRACY_SCOPED_ZONE;
      const string_id res_id = get_resource_id(input.file);
      input.db.resource_name(res_id, input.file);

      Assimp::Importer importer;

      constexpr uint32_t flags = aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_PreTransformVertices
                               | aiProcess_GenUVCoords | aiProcess_OptimizeMeshes | aiProcess_OptimizeGraph
                               | aiProcess_SortByPType | aiProcess_CalcTangentSpace /* costly for big meshes */ | aiProcess_JoinIdenticalVertices
                               | aiProcess_FindInvalidData | aiProcess_RemoveComponent | aiProcess_GenBoundingBoxes
      ;

      importer.SetPropertyInteger(AI_CONFIG_PP_RVC_FLAGS, aiComponent_BONEWEIGHTS | aiComponent_ANIMATIONS
                                                        | aiComponent_TEXTURES | aiComponent_LIGHTS
                                                        | aiComponent_CAMERAS);
      importer.SetPropertyInteger(AI_CONFIG_PP_SBP_REMOVE, aiPrimitiveType_LINE | aiPrimitiveType_POINT);
      importer.SetPropertyBool(AI_CONFIG_PP_FD_REMOVE, true);


      const aiScene* scene = importer.ReadFileFromMemory(input.file_data.get(), input.file_data.size, flags, input.file.extension().c_str());
      if (scene == nullptr)
      {
        input.db.error<obj_processor>(res_id, "assimp error: {}", importer.GetErrorString());

        // Tag the object properly but return a failure:
        std::vector<resources::processor::data> ret;
        ret.emplace_back(resources::processor::data
        {
          .resource_id = res_id,
          .resource_type = assets::static_mesh::type_name,
          .data = {},
          .metadata = std::move(input.metadata),
          .db = input.db,
        });

        return resources::processor::chain::create_and_complete({.to_pack = std::move(ret)}, resources::status::failure);
      }

      // compute some information:
      uint32_t color_channels = 0;
      uint32_t uv_channels = 0;
      uint32_t total_vertice_count = 0;
      uint32_t total_indices_count = 0;

      packer::static_mesh_packer_input mesh_data;

      glm::vec3 aabb_max;
      glm::vec3 aabb_min;
      // Compute channel count / material count / bounding sphere / ...
      for (uint32_t mesh_index = 0; mesh_index < scene->mNumMeshes; ++mesh_index)
      {
        // channels:
        const aiMesh& mesh = *scene->mMeshes[mesh_index];
        uint32_t mesh_color_channels = 0;
        for (uint32_t i = 0; i < AI_MAX_NUMBER_OF_COLOR_SETS; ++i)
        {
          if (mesh.HasVertexColors(i))
            mesh_color_channels += 1;
        }
        uint32_t mesh_uv_channels = 0;
        for (uint32_t i = 0; i < AI_MAX_NUMBER_OF_TEXTURECOORDS; ++i)
        {
          if (mesh.HasTextureCoords(i))
            mesh_uv_channels += 1;
        }
        color_channels = std::max(color_channels, mesh_color_channels);
        uv_channels = std::max(uv_channels, mesh_uv_channels);

        total_vertice_count += mesh.mNumVertices;
        total_indices_count += mesh.mNumFaces * 3;

        // materials:
        const id_t mat_id = string_id::_runtime_build_from_string(mesh.mName.data, mesh.mName.length);
        mesh_data.material_indices.emplace(mat_id, mesh_data.material_count);
        mesh_data.material_names.emplace(mat_id, std::string_view{mesh.mName.data, mesh.mName.length});
        mesh_data.material_count += 1;

        // AABB:
        if (mesh_index == 0)
        {
          aabb_max = { mesh.mAABB.mMax.x, mesh.mAABB.mMax.y, mesh.mAABB.mMax.z };
          aabb_min = { mesh.mAABB.mMin.x, mesh.mAABB.mMin.y, mesh.mAABB.mMin.z };
        }
        else
        {
          aabb_max = glm::max(aabb_max, glm::vec3{ mesh.mAABB.mMax.x, mesh.mAABB.mMax.y, mesh.mAABB.mMax.z });
          aabb_min = glm::min(aabb_min, glm::vec3{ mesh.mAABB.mMin.x, mesh.mAABB.mMin.y, mesh.mAABB.mMin.z });
        }
      }
      const uint32_t total_channel_count = color_channels + uv_channels;
      const uint32_t total_memory = total_channel_count * sizeof(glm::vec4) + total_vertice_count * sizeof(packer::vertex_data) + total_indices_count * sizeof(uint32_t);
      input.db.message<obj_processor>(res_id, "colors: {}, uvs: {} | vertices: {} | memory: {:.3f}Mib", color_channels, uv_channels, total_vertice_count, total_memory / 1024.0f / 1024.0f);

      // bounding sphere:
      {
        glm::vec3 aabb_center = (aabb_max + aabb_min) * 0.5f;
        glm::vec3 aabb_extent = (aabb_max - aabb_min) * 0.5f;
        mesh_data.bounding_sphere = {aabb_center, glm::length(aabb_extent)};
      }

      // register color / uv channels:
      // NOTE: We will want .can_be_interpolated and .must_be_strictly_different to be overridable in the metadata
      for (uint32_t color_index = 0; color_index < color_channels; ++color_index)
      {
        mesh_data.data.push_back(
        {
          .data = {},
          .is_vec2 = false,
          .must_be_strictly_different = false,
        });
        mesh_data.data.back().data.resize(total_vertice_count);
      }
      for (uint32_t uv_index = 0; uv_index < uv_channels; ++uv_index)
      {
        mesh_data.data.push_back(
        {
          .data = {},
          .is_vec2 = true, // assume UV are two channels only
          .must_be_strictly_different = true,
        });
        mesh_data.data.back().data.resize(total_vertice_count);
      }

      // Build the mesh:
      uint32_t base_vertex_index = 0; // indices offset
      resources::status status = resources::status::success;
      for (uint32_t mesh_index = 0; mesh_index < scene->mNumMeshes; ++mesh_index)
      {
        const aiMesh& mesh = *scene->mMeshes[mesh_index];

        if (!mesh.HasPositions())
        {
          input.db.warning<obj_processor>(res_id, "submesh {} doesn't have any positions", mesh.mName.C_Str());
          status = resources::worst(status, resources::status::partial_success);
          continue;
        }
        if (!mesh.HasNormals())
        {
          input.db.warning<obj_processor>(res_id, "submesh {} doesn't have normals", mesh.mName.C_Str());
          status = resources::worst(status, resources::status::partial_success);
          continue;
        }
        if (!mesh.HasTangentsAndBitangents())
        {
          input.db.warning<obj_processor>(res_id, "submesh {} doesn't have tangents / bitangents", mesh.mName.C_Str());
          status = resources::worst(status, resources::status::partial_success);
          continue;
        }

        input.db.debug<obj_processor>(res_id, "submesh {}: {} vertices", mesh.mName.C_Str(), mesh.mNumVertices);

        // push the vertex data:
        for (uint32_t vert_index = 0; vert_index < mesh.mNumVertices; ++vert_index)
        {
          mesh_data.vertices.push_back(
          {
            .position = { mesh.mVertices[vert_index].x, mesh.mVertices[vert_index].y, mesh.mVertices[vert_index].z },
            .normal = { mesh.mNormals[vert_index].x, mesh.mNormals[vert_index].y, mesh.mNormals[vert_index].z },
            .tangent = { mesh.mTangents[vert_index].x, mesh.mTangents[vert_index].y, mesh.mTangents[vert_index].z },
            .bitangent = { mesh.mBitangents[vert_index].x, mesh.mBitangents[vert_index].y, mesh.mBitangents[vert_index].z },
            .material_index = mesh_index,
          });
        }

        // push the index data:
        for (uint32_t face_index = 0; face_index < mesh.mNumFaces; ++face_index)
        {
          mesh_data.indices.push_back(mesh.mFaces[face_index].mIndices[0] + base_vertex_index);
          mesh_data.indices.push_back(mesh.mFaces[face_index].mIndices[1] + base_vertex_index);
          mesh_data.indices.push_back(mesh.mFaces[face_index].mIndices[2] + base_vertex_index);
        }

        // push the extra data (color then UV):
        uint32_t channel_index = 0;
        for (uint32_t i = 0; i < AI_MAX_NUMBER_OF_COLOR_SETS; ++i)
        {
          if (mesh.HasVertexColors(i))
          {
            for (uint32_t vert_index = 0; vert_index < mesh.mNumVertices; ++vert_index)
            {
              mesh_data.data[channel_index].data[vert_index + base_vertex_index] =
              {
                mesh.mColors[i][vert_index].r,
                mesh.mColors[i][vert_index].g,
                mesh.mColors[i][vert_index].b,
                mesh.mColors[i][vert_index].a,
              };
            }
            ++channel_index;
          }
        }
        for (uint32_t i = 0; i < AI_MAX_NUMBER_OF_TEXTURECOORDS; ++i)
        {
          if (mesh.HasTextureCoords(i))
          {
            for (uint32_t vert_index = 0; vert_index < mesh.mNumVertices; ++vert_index)
            {
              mesh_data.data[channel_index].data[vert_index + base_vertex_index] =
              {
                mesh.mTextureCoords[i][vert_index].x,
                mesh.mTextureCoords[i][vert_index].y,
                mesh.mTextureCoords[i][vert_index].z,
                0, // assimp doesn't support 4 component UV sets
              };
            }
            ++channel_index;
          }
        }

        // update the offset for the next submesh:
        base_vertex_index += mesh.mNumVertices;
      }

      // send the data to the packer:
      std::vector<resources::processor::data> ret;
      ret.emplace_back(resources::processor::data
      {
        .resource_id = res_id,
        .resource_type = assets::static_mesh::type_name,
        .data = rle::serialize(mesh_data),
        .metadata = std::move(input.metadata),
        .db = input.db,
      });
      return resources::processor::chain::create_and_complete({.to_pack = std::move(ret)}, status);
    }
  };
}

