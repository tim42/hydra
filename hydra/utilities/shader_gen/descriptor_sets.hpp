//
// created by : Timothée Feuillet
// date: 2024-2-13
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

#pragma once

#include <vector>

#include <hydra/hydra_glm.hpp>
#include <hydra/vulkan/descriptor_set_layout.hpp>
#include <hydra/vulkan/descriptor_set.hpp>
#include <hydra/vulkan/descriptor_pool.hpp>
#include <hydra/vulkan/queue.hpp>
#include <ntools/struct_metadata/struct_metadata.hpp>
#include <ntools/enum.hpp>
#include <ntools/id/string_id.hpp>
#include <ntools/type_id.hpp>
#include <ntools/raw_data.hpp>

#include "types.hpp"
#include "block.hpp"
#include "descriptor_sets_types.hpp"
#include "vulkan/command_buffer_recorder.hpp"

namespace neam::hydra
{
  struct hydra_context;
}
namespace neam::hydra::shaders
{
  namespace internal
  {
#if N_HYDRA_SHADERS_ALLOW_GENERATION
    using generate_ds_function_t = std::string (*)(uint32_t);
    void register_descriptor_set(string_id cpp_name, generate_ds_function_t generate, std::vector<id_t>&& deps);
    void unregister_descriptor_set(string_id cpp_name);

    std::string generate_descriptor_set(id_t cpp_name, uint32_t set_index);
    bool is_descriptor_set_registered(id_t cpp_name);
    void get_descriptor_set_dependencies(id_t cpp_name, std::vector<id_t>& deps);
#endif

    using generate_ds_layout_function_t = vk::descriptor_set_layout (*)(vk::device&, VkDescriptorSetLayoutCreateFlags);
    void register_runtime_descriptor_set(string_id cpp_name, generate_ds_layout_function_t generate);
    void unregister_runtime_descriptor_set(string_id cpp_name);

    vk::descriptor_set_layout generate_descriptor_set_layout(id_t cpp_name, vk::device& dev, VkDescriptorSetLayoutCreateFlags flags = 0);

    /// \brief Gen (glsl / ds layouts) + validation of the descriptor set
    template<metadata::concepts::StructWithMetadata Struct>
    class descriptor_set_gen
    {
      public:
#if N_HYDRA_SHADERS_ALLOW_GENERATION
        static std::string generate_code(uint32_t set_index)
        {
          std::string ret;
          uint32_t binding = 0;
          [&]<size_t... Indices>(std::index_sequence<Indices...>)
          {
            ([&]<size_t Index>()
            {
              using member = ct::list::get_type<n_metadata_member_list<Struct>, Index>;

              if constexpr (Index > 0 && !ct::list::has_type<typename member::metadata_types, alias_of_previous_entry>)
                ++binding;

              ret = fmt::format("{}{};", ret, descriptor_generator_wrapper<typename member::type>::generate_glsl_code(member::name.str, set_index, binding));

              // aliases don't increment the binding

            } .template operator()<Indices>(), ...);
          } (std::make_index_sequence<ct::list::size<n_metadata_member_list<Struct>>> {});
          return ret;
        }
        static std::vector<id_t> compute_dependencies()
        {
          std::vector<id_t> ret;
          [&]<size_t... Indices>(std::index_sequence<Indices...>)
          {
            ([&]<size_t Index>()
            {
              // aliases still generate code, so we treat them as normal
              using member = ct::list::get_type<n_metadata_member_list<Struct>, Index>;
              descriptor_generator_wrapper<typename member::type>::update_dependencies(ret);
            } .template operator()<Indices>(), ...);
          } (std::make_index_sequence<ct::list::size<n_metadata_member_list<Struct>>> {});
          return ret;
        }
#endif
        /// \see has_unbound_array
        static constexpr uint32_t compute_descriptor_count()
        {
          uint32_t highest_binding = 0;
          [&]<size_t... Indices>(std::index_sequence<Indices...>)
          {
            ([&]<size_t Index>()
            {
              using member = ct::list::get_type<n_metadata_member_list<Struct>, Index>;
              if constexpr (ct::list::has_type<typename member::metadata_types, alias_of_previous_entry>)
                return;
              else if constexpr (Index > 0)
                highest_binding += 1;
            } .template operator()<Indices>(), ...);
          } (std::make_index_sequence<ct::list::size<n_metadata_member_list<Struct>>> {});
          return highest_binding + 1;
        }
        static constexpr uint32_t k_descriptor_count = compute_descriptor_count();

        /// \see has_unbound_array
        static constexpr bool compute_has_unbound_array()
        {
          bool has = false;
          [&]<size_t... Indices>(std::index_sequence<Indices...>)
          {
              ([&]<size_t Index>()
              {
                using member = ct::list::get_type<n_metadata_member_list<Struct>, Index>;
                if constexpr (ct::list::has_type<typename member::metadata_types, alias_of_previous_entry>)
                  return;
                else
                  has = has || descriptor_generator_wrapper<typename member::type>::is_unbound_array;
              } .template operator()<Indices>(), ...);
            } (std::make_index_sequence<ct::list::size<n_metadata_member_list<Struct>>> {});
            return has;
        }
        static constexpr bool has_unbound_array = compute_has_unbound_array();

        static uint32_t unbound_array_size(Struct& s)
        {
          uint32_t ret = ~0u;
          [&]<size_t... Indices>(std::index_sequence<Indices...>)
          {
            ([&]<size_t Index>()
            {
              using member = ct::list::get_type<n_metadata_member_list<Struct>, Index>;
              if constexpr(ct::list::has_type<typename member::metadata_types, alias_of_previous_entry>)
                return;
              else if constexpr (descriptor_generator_wrapper<typename member::type>::is_unbound_array)
                ret = member::extract_from(s).size();
            } .template operator()<Indices>(), ...);
          } (std::make_index_sequence<ct::list::size<n_metadata_member_list<Struct>>> {});
          return ret;
        }

        static vk::descriptor_set_layout create_layout(vk::device& dev, VkDescriptorSetLayoutCreateFlags flags = 0)
        {
          static std::vector<vk::descriptor_set_layout_binding> bindings;
          static bool is_initialized = false;
          if (!is_initialized)
          {
            is_initialized = true;
            uint32_t binding = 0;
            [&]<size_t... Indices>(std::index_sequence<Indices...>)
            {
              ([&]<size_t Index>()
              {
                using member = ct::list::get_type<n_metadata_member_list<Struct>, Index>;
                if constexpr(ct::list::has_type<typename member::metadata_types, alias_of_previous_entry>)
                {
                  return;
                }
                else
                {
                  if (Index > 0)
                    ++binding;
                  descriptor_generator_wrapper<typename member::type>::fill_descriptor_layout_bindings(bindings, binding);
                }
              } .template operator()<Indices>(), ...);
            } (std::make_index_sequence<ct::list::size<n_metadata_member_list<Struct>>> {});
          }

          vk::descriptor_set_layout ret {dev, bindings, flags};
          ret._set_debug_name((std::string)ct::type_name<Struct>.view());
          return ret;
        }

        static void update_descriptor_set(Struct& s, vk::device& dev, VkDescriptorSet vk_ds)
        {
          static_assert(k_descriptor_count > 0);
          descriptor_write_struct<k_descriptor_count> update_data;
          get_descriptor_set_update_struct(s, vk_ds, update_data);
          dev._vkUpdateDescriptorSets(update_data.descriptors.size(), update_data.descriptors.data(), 0, nullptr);
        }
        static void push_descriptor_set(hydra_context& hctx, vk::command_buffer_recorder& cbr, Struct& s, VkDescriptorSet vk_ds)
        {
          static_assert(k_descriptor_count > 0);
          descriptor_write_struct<k_descriptor_count> update_data;
          get_descriptor_set_update_struct(s, vk_ds, update_data);
          cbr.push_descriptor_set<hydra_context, Struct>(hctx, update_data.descriptors.size(), update_data.descriptors.data());
        }
      private:
        static void get_descriptor_set_update_struct(Struct& s, VkDescriptorSet vk_ds, descriptor_write_struct<k_descriptor_count>& dws)
        {
          uint32_t binding = 0;
          [&]<size_t... Indices>(std::index_sequence<Indices...>)
          {
            ([&]<size_t Index>()
            {
              using member = ct::list::get_type<n_metadata_member_list<Struct>, Index>;
                if constexpr(ct::list::has_type<typename member::metadata_types, alias_of_previous_entry>)
                {
                  return;
                }
                else
                {
                  if (Index > 0)
                    ++binding;
                  descriptor_generator_wrapper<typename member::type>::setup_descriptor(member::extract_from(s), binding, vk_ds, dws);
                }
            } .template operator()<Indices>(), ...);
          } (std::make_index_sequence<ct::list::size<n_metadata_member_list<Struct>>> {});
        }
    };

    template<typename BlockStruct>
    class raii_ds_register
    {
      public:
        raii_ds_register()
        {
          register_runtime_descriptor_set(ct::type_name<BlockStruct>, descriptor_set_gen<BlockStruct>::create_layout);
#if N_HYDRA_SHADERS_ALLOW_GENERATION
          register_descriptor_set(ct::type_name<BlockStruct>, descriptor_set_gen<BlockStruct>::generate_code, descriptor_set_gen<BlockStruct>::compute_dependencies());
#endif
        }
        ~raii_ds_register()
        {
          unregister_runtime_descriptor_set(ct::type_name<BlockStruct>);
#if N_HYDRA_SHADERS_ALLOW_GENERATION
          unregister_descriptor_set(ct::type_name<BlockStruct>);
#endif
        }
    };

    class descriptor_set_struct_internal
    {
      protected:
        static vk::descriptor_set allocate_descriptor_set(hydra_context& hctx, vk::descriptor_set_layout& ds_layout, uint32_t variable_descriptor_count = ~0u);
        static void deallocate_descriptor_set(hydra_context& hctx, vk::descriptor_set&& set);
        static vk::queue& get_graphic_queue(hydra_context& hctx);
        static vk::device& get_device(hydra_context& hctx);
    };
  }

  /// \brief descriptor-set cpp -> glsl helper. Generate glsl code for shaders and hold descriptor set data.
  /// \todo state tracking to avoid updates that does nothing
  template<typename Struct>
  class descriptor_set_struct : private internal::descriptor_set_struct_internal, cr::mt_checked<descriptor_set_struct<Struct>>
  {
    public:
      const vk::descriptor_set* get_descriptor_set() const
      {
        N_MTC_READER_SCOPE;
        if (ds && ds->_get_vk_descritpor_set() != nullptr)
          return &*ds;
        return nullptr;
      }
      vk::descriptor_set* get_descriptor_set()
      {
        N_MTC_READER_SCOPE;
        if (ds && ds->_get_vk_descritpor_set() != nullptr)
          return &*ds;
        return nullptr;
      }

      vk::descriptor_set& get_or_create_descriptor_set(hydra_context& hctx)
      {
        if constexpr(internal::descriptor_set_gen<Struct>::has_unbound_array)
        {
          uint32_t current_alloc_size;
          {
            N_MTC_READER_SCOPE;
            current_alloc_size = internal::descriptor_set_gen<Struct>::unbound_array_size(*static_cast<Struct*>(this));
          }
          if (unbound_array_alloc_size < current_alloc_size)
          {
            N_MTC_WRITER_SCOPE;
            deallocate_descriptor_set(hctx, std::move(*ds));
            ds.reset();
          }
        }
        {
          {
            N_MTC_READER_SCOPE;
            if (ds && ds->_get_vk_descritpor_set() != nullptr)
              return *ds;
          }

          N_MTC_WRITER_SCOPE;
          // layout will not change, as the C++ type that generated it is static
          if (!ds_layout)
            ds_layout = internal::descriptor_set_gen<Struct>::create_layout(get_device(hctx));

          if constexpr(internal::descriptor_set_gen<Struct>::has_unbound_array)
          {
            unbound_array_alloc_size = internal::descriptor_set_gen<Struct>::unbound_array_size(*static_cast<Struct*>(this));
            ds = allocate_descriptor_set(hctx, *ds_layout, unbound_array_alloc_size);
          }
          else
          {
            ds = allocate_descriptor_set(hctx, *ds_layout);
          }
          return *ds;
        }
      }
      vk::descriptor_set& get_or_create_descriptor_set_for_update(hydra_context& hctx)
      {
        bool reset = false;
        {
          N_MTC_READER_SCOPE;
          if (ds && ds->_get_vk_descritpor_set() != nullptr)
            reset = true;
        }
        if (reset)
        {
          N_MTC_WRITER_SCOPE;
          deallocate_descriptor_set(hctx, std::move(*ds));
          ds.reset();
        }
        return get_or_create_descriptor_set(hctx);

      }

      void update_descriptor_set(hydra_context& hctx)
      {
        N_MTC_WRITER_SCOPE;
        internal::descriptor_set_gen<Struct>::update_descriptor_set
        (
          *static_cast<Struct*>(this), get_device(hctx),
          get_or_create_descriptor_set_for_update(hctx)._get_vk_descritpor_set()
        );
      }

      void push_descriptor_set(hydra_context& hctx, vk::command_buffer_recorder& cbr)
      {
        N_MTC_WRITER_SCOPE;
        internal::descriptor_set_gen<Struct>::push_descriptor_set
        (
          hctx, cbr,
          *static_cast<Struct*>(this),
          get_or_create_descriptor_set(hctx)._get_vk_descritpor_set()
        );
      }

      /// \brief Return the descriptor-set if one is present, or an invalid descriptor-set.
      /// \note The struct will not have a descriptor-set after this
      [[nodiscard]] std::optional<vk::descriptor_set> reset()
      {
        N_MTC_WRITER_SCOPE;
        return std::move(ds);
      }

      /// \brief Write the descriptor to a memory area (for use in a buffer. Returns the end offset)
      /// FIXME: probably a better return value (like an object or something)
      uint32_t write_to_buffer(raw_data& data, uint32_t offset);
    private:
      std::optional<vk::descriptor_set_layout> ds_layout;
      std::optional<vk::descriptor_set> ds;

      uint32_t unbound_array_alloc_size = ~0u;


      inline static internal::raii_ds_register<Struct> _registration;

      // force instantiation of the static member: (and avoid a warning)
      static_assert(&_registration == &_registration);
  };
}
