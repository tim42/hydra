
// #include <neam/reflective/reflective.hpp>

//#define N_ALLOW_DEBUG true // we want full debug information
#define N_DISABLE_CHECKS // we don't want anything

#include <hydra/tools/logger/logger.hpp>

#include <hydra/hydra.hpp>          // the main header of hydra
#include <hydra/hydra_glm_udl.hpp>  // we do want the easy glm udl (optional, not used by hydra)
#include <hydra/hydra_glfw_ext.hpp> // for the window creation / handling
#include <hydra/hydra_lodepng_ext.hpp> // for the window creation / handling


#include <hydra/tools/chrono.hpp>

/// \brief A dummy vertex, just for fun
struct dummy_vertex
{
  glm::vec2 pos;
  glm::vec3 color;
  glm::vec2 uv;

  static neam::hydra::vk::pipeline_vertex_input_state get_vertex_input_state()
  {
    neam::hydra::vk::pipeline_vertex_input_state pvis;
    pvis.add_binding_description(0, sizeof(dummy_vertex), VK_VERTEX_INPUT_RATE_VERTEX);
    pvis.add_attribute_description(0, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(dummy_vertex, pos));
    pvis.add_attribute_description(0, 1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(dummy_vertex, color));
    pvis.add_attribute_description(0, 2, VK_FORMAT_R32G32_SFLOAT, offsetof(dummy_vertex, uv));
    return pvis;
  }
};
const std::vector<dummy_vertex> vertices =
{
    {{-0.5f, -0.5f}, {0x09 / 255.f, 1.0f, 0.0f}, {0.f, 0.f}},
    {{0.5f,  -0.5f}, {0.0f, 0x89 / 255.f, 1.0f}, {1.f, 0.f}},
    {{0.5f,   0.5f}, {0xF6 / 255.f, 0.0f, 1.0f}, {1.f, 1.f}},
    {{-0.5f,  0.5f}, {1.0f, 0x76 / 255.f, 0.0f}, {0.f, 1.f}}
};
const std::vector<uint16_t> indices =
{
    0, 1, 2, 2, 3, 0
};


int main(int, char **)
{
  // I want all these debug information to be printed:
//   neam::cr::out.log_level = neam::cr::stream_logger::verbosity_level::debug;
  neam::cr::out.log_level = neam::cr::stream_logger::verbosity_level::log;

  neam::hydra::glfw::init_extension glfw_ext;
  neam::hydra::gen_feature_requester gfr;

  // some requirements of the app
  glfw_ext.request_graphic_queue(true); // the queue
  gfr.require_device_extension(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

  // debug extensions / layers:
  gfr.require_instance_extension(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
  gfr.require_instance_layer("VK_LAYER_KHRONOS_validation");
  auto temp_transfer_queue = gfr.require_queue_capacity(VK_QUEUE_TRANSFER_BIT, false);

  // initialize vulkan/hydra
  neam::hydra::bootstrap hydra_init;
  hydra_init.register_init_extension(glfw_ext);
  hydra_init.register_feature_requester(gfr);

  neam::hydra::vk::instance instance = hydra_init.create_instance("hydra-test-dev");
  instance.install_default_debug_callback(VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT | VK_DEBUG_REPORT_ERROR_BIT_EXT);
//   instance.install_default_debug_callback(VK_DEBUG_REPORT_FLAG_BITS_MAX_ENUM_EXT); // We want all the reports (maximum debug)

  neam::hydra::glfw::window win = glfw_ext.create_window(instance, glm::uvec2(900, 900), "hydra-test-dev");

  neam::hydra::vk::device device = hydra_init.create_device(instance);
  neam::hydra::vk::queue gqueue(device, win._get_win_queue());
  neam::hydra::vk::queue tqueue(device, *temp_transfer_queue);

  // create the memory allocator
  neam::hydra::memory_allocator mem_alloc(device);

  // create the swapchain
  neam::hydra::vk::swapchain sc = win._create_swapchain(device);

  // create the command pool
  neam::hydra::vk::command_pool cmd_pool = gqueue.create_command_pool();
  neam::hydra::vk::command_pool transfer_cmd_pool = tqueue.create_command_pool(VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

  // create the batch transfer
  neam::hydra::vk_resource_destructor vrd;
  neam::hydra::batch_transfers btransfers(device, tqueue, transfer_cmd_pool, vrd);
  btransfers.allocate_memory(mem_alloc);

  //////////////////////////////////////////////////////////////////////////////
  // create the render pass
  neam::hydra::vk::render_pass render_pass(device);
  render_pass.create_subpass().add_attachment(neam::hydra::vk::subpass::attachment_type::color, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 0);
  render_pass.create_subpass_dependency(VK_SUBPASS_EXTERNAL, 0)
              .dest_subpass_masks(VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_ACCESS_MEMORY_READ_BIT)
              .source_subpass_masks(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);

  render_pass.create_attachment().set_swapchain(&sc).set_samples(VK_SAMPLE_COUNT_1_BIT)
              .set_load_op(VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_LOAD_OP_DONT_CARE)
              .set_store_op(VK_ATTACHMENT_STORE_OP_STORE, VK_ATTACHMENT_STORE_OP_DONT_CARE)
              .set_layouts(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

  render_pass.refresh();

  //////////////////////////////////////////////////////////////////////////////
  // setup the mesh
  neam::hydra::vk::fence end_transfer(device);
  neam::hydra::mesh mesh(device);

  mesh.add_buffer(sizeof(indices[0]) * indices.size(), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
  mesh.add_buffer(sizeof(vertices[0]) * vertices.size(), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
  mesh.vertex_input_state() = dummy_vertex::get_vertex_input_state();
  mesh.allocate_memory(mem_alloc);

  mesh.transfer_data(btransfers, 0, sizeof(indices[0]) * indices.size(), indices.data());
  mesh.transfer_data(btransfers, 1, sizeof(vertices[0]) * vertices.size(), vertices.data());

  //////////////////////////////////////////////////////////////////////////////
  // setup the descriptors/...
  neam::hydra::vk::descriptor_set_layout_binding sampler_dslb(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT);
  neam::hydra::vk::descriptor_set_layout_binding ubuffer_dslb(1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT);
  neam::hydra::vk::descriptor_set_layout sampler_ds_layout(device, {sampler_dslb, ubuffer_dslb});

  neam::hydra::vk::descriptor_pool ds_pool(device, 1, {{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1}});
  neam::hydra::vk::descriptor_set descriptor_set = ds_pool.allocate_descriptor_set(sampler_ds_layout);

  // image, view, sampler:
  unsigned int logo_size = 1024;
  neam::hydra::vk::image hydra_logo_img = neam::hydra::vk::image::create_image_arg(device,
     neam::hydra::vk::image_2d({logo_size, logo_size}, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT)
  );

  neam::hydra::vk::sampler sampler(device, VK_FILTER_NEAREST, VK_FILTER_LINEAR, VK_SAMPLER_MIPMAP_MODE_LINEAR, 0.f, 0.f, 0.f);

  // allocate memory for the image (+ transfer data to it)
  {
    neam::hydra::memory_allocation ma = mem_alloc.allocate_memory(hydra_logo_img.get_memory_requirements(), VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, neam::hydra::allocation_type::optimal_image);
    hydra_logo_img.bind_memory(*ma.mem(), ma.offset());

    uint8_t *pixels = new uint8_t[logo_size * logo_size * 4];
    neam::hydra::generate_rgba_logo(pixels, logo_size, 5);
    btransfers.add_transfer(hydra_logo_img, VK_IMAGE_LAYOUT_PREINITIALIZED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, logo_size * logo_size * 4, pixels, nullptr, &end_transfer);
    delete[] pixels;

    btransfers.start(); // We can transfer buffers while the other things initialize...
  }

  neam::hydra::vk::image_view hydra_logo_img_view(device, hydra_logo_img, VK_IMAGE_VIEW_TYPE_2D);

  // update ds
  descriptor_set.write_descriptor_set(0, 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, {{ sampler, hydra_logo_img_view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL }});

  //////////////////////////////////////////////////////////////////////////////
  // load the shaders
  neam::hydra::vk::shader_module vert_mod = neam::hydra::vk::spirv_shader::load_from_file(device, "data/shaders/2d_plane.vert.spv");
  neam::hydra::vk::shader_module frag_mod = neam::hydra::vk::spirv_shader::load_from_file(device, "data/shaders/2d_plane.frag.spv");

  // create the pipeline
  neam::hydra::vk::pipeline_creator pcr;
  pcr.get_pipeline_shader_stage()
              .add_shader(vert_mod, VK_SHADER_STAGE_VERTEX_BIT)
              .add_shader(frag_mod, VK_SHADER_STAGE_FRAGMENT_BIT);

  // yep.
  mesh.setup_vertex_description(pcr);

  pcr.get_viewport_state().add_viewport(&sc.get_full_viewport())
                          .add_scissor(&sc.get_full_rect2D());

  neam::hydra::vk::attachment_color_blending acb = neam::hydra::vk::attachment_color_blending::create_alpha_blending();
  pcr.get_pipeline_color_blending_state().add_attachment_color_blending(&acb);

  neam::hydra::vk::pipeline_layout pl(device, { &sampler_ds_layout });
  pcr.set_pipeline_layout(pl);
  pcr.set_render_pass(render_pass);
  pcr.set_subpass_index(0);

  neam::hydra::vk::pipeline pipeline = pcr.create_pipeline(device);

  //////////////////////////////////////////////////////////////////////////////
  // record the command buffers
  std::vector<neam::hydra::vk::command_buffer> cmd_vct;
  std::vector<neam::hydra::vk::framebuffer> fb_vct;

  neam::hydra::vk::semaphore image_ready(device);
  neam::hydra::vk::semaphore render_finished(device);
  std::vector<neam::hydra::vk::submit_info> si_vct;

  for (size_t i = 0; i < sc.get_image_count(); ++i)
  {
    fb_vct.emplace_back(neam::hydra::vk::framebuffer(device, render_pass, { &sc.get_image_view_vector()[i] }, sc));
    cmd_vct.emplace_back(cmd_pool.create_command_buffer());

    {
      neam::hydra::vk::command_buffer_recorder cbr = cmd_vct.back().begin_recording(VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT);

      cbr.begin_render_pass(render_pass, fb_vct.back(), sc.get_full_rect2D(), VK_SUBPASS_CONTENTS_INLINE, { glm::vec4(0.0f, 0x89 / 255.f, 1.0f, 1.f) });
      cbr.bind_pipeline(pipeline);
      cbr.bind_descriptor_set(VK_PIPELINE_BIND_POINT_GRAPHICS, pcr.get_pipeline_layout(), 0, { &descriptor_set });
      mesh.bind(cbr);
      cbr.draw_indexed(indices.size(), 1, 0, 0, 0); // TODO: a draw state, like in YägGLer
      cbr.end_render_pass();

      cmd_vct.back().end_recording();
    }

    si_vct.emplace_back();
    si_vct.back() << neam::hydra::vk::cmd_sema_pair{image_ready, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT} << cmd_vct.back() >> render_finished;
  }

  //////////////////////////////////////////////////////////////////////////////
  // The loop
  bool recreate = false;

  neam::cr::chrono cr;
  float frame_cnt = 0;

  neam::cr::out.log() << "btransfer: remaining " << btransfers.get_total_size_to_transfer() << " bytes..." << std::endl;
  end_transfer.wait(); // can't really do much more here

  cr.reset();
  while (!win.should_close())
  {
    glfwPollEvents();

    {
      size_t index = sc.get_next_image_index(image_ready, std::numeric_limits<uint64_t>::max(), &recreate);
      gqueue.submit(si_vct[index]);
      gqueue.present(sc, index, { &render_finished });
    }

    ++frame_cnt;

    if (cr.get_accumulated_time() > 2.)
    {
      neam::cr::out.log() << (cr.get_accumulated_time() / frame_cnt) * 1000.f << "ms/frame\t(" << (int)(frame_cnt / cr.get_accumulated_time()) << "fps)"<< std::endl;
      cr.reset();
      frame_cnt = 0;
    }
  }

  device.wait_idle();

  return 0;
}

