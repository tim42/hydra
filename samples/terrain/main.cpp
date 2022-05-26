
// #include <neam/reflective/reflective.hpp>

#define N_ALLOW_DEBUG true // we want full debug information
//#define N_DISABLE_CHECKS // we don't want anything

#include "sample.hpp"

int main(int, char **)
{
  // I want all these debug information to be printed:
  // neam::cr::out.min_severity = neam::cr::logger::severity::debug;
  neam::cr::out.min_severity = neam::cr::logger::severity::message;

  neam::sample_app app(900_uvec2_xy, "HYDRA");
  app.init_and_run();


  return 0;
}

