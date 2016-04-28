
// #include <neam/reflective/reflective.hpp>

#define N_ALLOW_DEBUG true // we want full debug information

#include <hydra/tools/logger/logger.hpp>

#include <hydra/hydra.hpp>
#include <hydra/glfw/glfw.hpp> // for the window creation / handling
#include <hydra/hydra_glm_udl.hpp> // we do want the easy glm udl

#include <vulkan/vulkan.h>

#include <unistd.h>
int main(int, char **)
{
  // I want all these debug information to be printed:
  neam::cr::out.log_level = neam::cr::stream_logger::verbosity_level::debug;

  neam::hydra::glfw::init_extension glfw_ext;
  neam::hydra::gen_feature_requester gfr;

  // some requirements of the program
  gfr.require_queue_capacity(VK_QUEUE_GRAPHICS_BIT, 1, true);

  neam::hydra::bootstrap hydra_init;
  hydra_init.register_init_extension(glfw_ext);
  hydra_init.register_feature_requester(gfr);
  neam::hydra::hydra_vulkan_instance instance = hydra_init.create_instance("hydra-test-dev");

  neam::hydra::glfw::window win = glfw_ext.create_window(instance, glm::uvec2(500, 500), "hydra-test-dev");

  neam::hydra::hydra_vulkan_device device = hydra_init.create_device(instance);

  neam::cr::out.log() << "device created !" << std::endl;


  sleep(25);

  return 0;
}

