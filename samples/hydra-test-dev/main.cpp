
// #include <neam/reflective/reflective.hpp>

#define N_ALLOW_DEBUG true // we want full debug information
//#define N_DISABLE_CHECKS // we don't want anything

#include <hydra/tools/logger/logger.hpp>

#include <hydra/hydra.hpp>          // the main header of hydra
#include <hydra/hydra_glfw_ext.hpp> // for the window creation / handling
#include <hydra/hydra_glm_udl.hpp>  // we do want the easy glm udl (optional, not used by hydra)


#include <hydra/tools/chrono.hpp>

/// \brief A dummy vertex, just for fun
struct dummy_vertex
{
  glm::vec2 pos;
  glm::vec3 color;

  static neam::hydra::vk::pipeline_vertex_input_state get_vertex_description()
  {
    neam::hydra::vk::pipeline_vertex_input_state pvis;
    pvis.add_binding_description(0, sizeof(dummy_vertex), VK_VERTEX_INPUT_RATE_VERTEX);
    pvis.add_attribute_description(0, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(dummy_vertex, pos));
    pvis.add_attribute_description(0, 1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(dummy_vertex, color));
    return pvis;
  }
};
const std::vector<dummy_vertex> vertices =
{
    {{-0.5f, -0.5f}, {0x09 / 255.f, 1.0f, 0.0f}},
    {{0.5f, -0.5f}, {0.0f, 0x89 / 255.f, 1.0f}},
    {{0.5f, 0.5f}, {0xF6 / 255.f, 0.0f, 1.0f}},
    {{-0.5f, 0.5f}, {1.0f, 0x76 / 255.f, 0.0f}}
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
  gfr.require_instance_layer("VK_LAYER_LUNARG_standard_validation");
  auto temp_transfer_queue = gfr.require_queue_capacity(VK_QUEUE_TRANSFER_BIT, false);

  // initialize vulkan/hydra
  neam::hydra::bootstrap hydra_init;
  hydra_init.register_init_extension(glfw_ext);
  hydra_init.register_feature_requester(gfr);

  neam::hydra::vk::instance instance = hydra_init.create_instance("hydra-test-dev");
  instance.install_default_debug_callback(VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT | VK_DEBUG_REPORT_ERROR_BIT_EXT);
//   instance.install_default_debug_callback(VK_DEBUG_REPORT_FLAG_BITS_MAX_ENUM_EXT); // We want all the reports (maximum debug)

  neam::hydra::glfw::window win = glfw_ext.create_window(instance, glm::uvec2(500, 500), "hydra-test-dev");

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
  neam::hydra::batch_transfers btransfers(device, tqueue, transfer_cmd_pool);
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
  mesh.get_vertex_input_state() = dummy_vertex::get_vertex_description();

  mesh.allocate_memory(mem_alloc);

  mesh.transfer_data(btransfers, 0, sizeof(indices[0]) * indices.size(), indices.data());
  mesh.transfer_data(btransfers, 1, sizeof(vertices[0]) * vertices.size(), vertices.data(), nullptr, &end_transfer);

  btransfers.start(); // We can transfer buffers while the other things initialize...

  //////////////////////////////////////////////////////////////////////////////
  // load the shaders
  neam::hydra::vk::shader_module vert_mod = neam::hydra::vk::spirv_shader::load_from_file(device, "vert.spv");
  neam::hydra::vk::shader_module frag_mod = neam::hydra::vk::spirv_shader::load_from_file(device, "frag.spv");
  // create the pipeline
  neam::hydra::vk::pipeline_creator pcr;
  pcr.get_pipeline_shader_stage()
              .add_shader(vert_mod, VK_SHADER_STAGE_VERTEX_BIT)
              .add_shader(frag_mod, VK_SHADER_STAGE_FRAGMENT_BIT);

  // yep.
  mesh.setup_vertex_description(pcr);

  pcr.get_viewport_state().add_viewport(&sc.get_full_viewport())
                          .add_scissor(&sc.get_full_rect2D());

  neam::hydra::vk::attachment_color_blending acb;
  pcr.get_pipeline_color_blending_state().add_attachment_color_blending(&acb);

  neam::hydra::vk::pipeline_layout pl(device);
  pcr.set_pipeline_layout(pl);
  pcr.set_render_pass(render_pass);

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

      cbr.begin_render_pass(render_pass, fb_vct.back(), sc.get_full_rect2D(), VK_SUBPASS_CONTENTS_INLINE, { glm::vec4(0, 0, 0, 1) });
      cbr.bind_pipeline(pipeline);
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

  neam::cr::out.log() << LOGGER_INFO << "btransfer: remaining " << btransfers.get_total_size_to_transfer() << " bytes..." << std::endl;
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

