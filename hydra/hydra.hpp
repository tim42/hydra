//
// file : hydra.hpp
// in : file:///home/tim/projects/hydra/hydra/hydra.hpp
//
// created by : Timothée Feuillet
// date: Sat Apr 23 2016 15:19:07 GMT+0200 (CEST)
//
//
// Copyright (c) 2016 Timothée Feuillet
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

#ifndef __N_396519513304418946_3159317424_HYDRA_HPP__
#define __N_396519513304418946_3159317424_HYDRA_HPP__

// NOTE: There's *_ext.hpp file in the current folder. Those files aren't included
// by this header as they are the main header of some extensions. If you plan using
// that extension with hydra, you may want to include 

// a logger for hydra (define HYDRA_NO_MESSAGES to disable logs)
#include "hydra_logger.hpp"

// an optional reflective support for hydra (define HYDRA_NO_REFLECTIVE_SUPPORT to disable this)
// you don't need to have reflective to build and use hydra, but if the reflective header is included
// hydra will generate some meta/data for it
#include "hydra_reflective.hpp"

// some specific exception for hydra. Also, some checks.
// The macro HYDRA_DISABLE_OPTIONAL_CHECKS will disable asserts and error checking
// The macro HYDRA_ENABLE_DEBUG_CHECKS will enable some asserts and error checking that may slow down a bit the code
// on non-critical sections of the code,
// N_DISABLE_CHECKS will disable all the asserts and error checking
// N_ALLOW_DEBUG will log a lot of information about checks and asserts
// NOTE: hydra assert throws exceptions (neam::hydra::exception that inherits from std::exception)
#include "hydra_exception.hpp"

// include the bootstrap / init files
#include "init/bootstrap.hpp"
#include "init/feature_requesters/gen_feature_requester.hpp"

// Make GLM compatible with vulkan
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include <glm/glm.hpp> // glm is a core dependency of hydra

// include the vulkan wrapper
#include "vulkan/vulkan.hpp"

// include main components
#include "utilities/utilities.hpp"
#include "geometry/geometry.hpp"
#include "threading/threading.hpp"
#include "geometry/geometry.hpp"

// #ifndef HYDRA_NO_RENDERER
// // include the renderer (can be disabled by defining HYDRA_NO_RENDERER)
// #include "renderer/renderer.hpp"
// #endif

namespace neam
{
  namespace hydra
  {
  } // namespace hydra
} // namespace neam

#endif // __N_396519513304418946_3159317424_HYDRA_HPP__
