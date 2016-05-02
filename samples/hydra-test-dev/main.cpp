
// #include <neam/reflective/reflective.hpp>

#define N_ALLOW_DEBUG true // we want full debug information

#include <hydra/tools/logger/logger.hpp>

#include <hydra/hydra.hpp>
#include <hydra/glfw/glfw.hpp> // for the window creation / handling
#include <hydra/hydra_glm_udl.hpp> // we do want the easy glm udl

#include <vulkan/vulkan.h>

#include <unistd.h> // TODO: remove

int main(int, char **)
{
  // I want all these debug information to be printed:
  neam::cr::out.log_level = neam::cr::stream_logger::verbosity_level::debug;

  neam::hydra::glfw::init_extension glfw_ext;
  neam::hydra::gen_feature_requester gfr;

  // some requirements of the app
  glfw_ext.request_graphic_queue(true); // the queue
  gfr.require_device_extension(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

  // initialize vulkan/hydra
  neam::hydra::bootstrap hydra_init;
  hydra_init.register_init_extension(glfw_ext);
  hydra_init.register_feature_requester(gfr);
  neam::hydra::vk::instance instance = hydra_init.create_instance("hydra-test-dev");

  neam::hydra::glfw::window win = glfw_ext.create_window(instance, glm::uvec2(500, 500), "hydra-test-dev");

  neam::hydra::vk::device device = hydra_init.create_device(instance);
  neam::hydra::vk::queue gqueue(device, win._get_win_queue());

  // run something
  neam::cr::out.log() << "device created !" << std::endl;

  neam::hydra::vk::command_pool cmd_pool = gqueue.create_command_pool();
  neam::hydra::vk::command_buffer cmd = cmd_pool.create_command_buffer();

  neam::hydra::vk::submit_info si;

  si << neam::hydra::vk::cmd_buffer_pair{cmd, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT};

  sleep(25);

  return 0;
}

