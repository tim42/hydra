
// #include <neam/reflective/reflective.hpp>

#define N_ALLOW_DEBUG true // we want full debug information

#include <hydra/tools/logger/logger.hpp>

#include <hydra/hydra.hpp>
#include <hydra/hydra_glm_udl.hpp> // we do want the easy glm udl

#include <vulkan/vulkan.h>


int main(int, char **)
{
  neam::cr::out.log_level = neam::cr::stream_logger::verbosity_level::debug;
  neam::hydra::hydra_instance_creator hcreator("test-dev-app");

  neam::hydra::hydra_vulkan_instance instance = hcreator.create_instance();

  neam::cr::out.log() << "INSTANCE CREATED !" << std::endl;

  return 0;
}

