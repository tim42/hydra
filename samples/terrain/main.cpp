
// #include <neam/reflective/reflective.hpp>

//#define N_ALLOW_DEBUG true // we want full debug information
#define N_DISABLE_CHECKS // we don't want anything

#include "sample.hpp"

int main(int, char **)
{
  // I want all these debug information to be printed:
//   neam::cr::out.log_level = neam::cr::stream_logger::verbosity_level::debug;
  neam::cr::out.log_level = neam::cr::stream_logger::verbosity_level::log;

  neam::sample_app app(900_uvec2_xy, "HYDRA");
  app.init_and_run();


  return 0;
}

