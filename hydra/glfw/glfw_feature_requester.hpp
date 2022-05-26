//
// file : glfw_feature_requester.hpp
// in : file:///home/tim/projects/hydra/hydra/glfw/glfw_feature_requester.hpp
//
// created by : Timothée Feuillet
// date: Wed Apr 27 2016 17:27:53 GMT+0200 (CEST)
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

#ifndef __N_18919125232751427991_2329432231_GLFW_FEATURE_REQUESTER_HPP__
#define __N_18919125232751427991_2329432231_GLFW_FEATURE_REQUESTER_HPP__

#include <list>
#include <GLFW/glfw3.h>
#include "../init/feature_requester_interface.hpp"
#include "../init/hydra_instance_creator.hpp"
#include "../init/hydra_device_creator.hpp"
#include "../hydra_debug.hpp"

#include "glfw_window.hpp"

namespace neam
{
  namespace hydra
  {
    namespace glfw
    {
      /// \brief A feature requester for GLFW
      /// Its job is to ask hydra the features GLFW needs to work
      class feature_requester : public neam::hydra::feature_requester_interface
      {
        public:
          virtual ~feature_requester() {}

          void request_instace_layers_extensions(neam::hydra::hydra_instance_creator &hic) override
          {
            uint32_t required_extension_count;
            const char **required_extensions;
            required_extensions = glfwGetRequiredInstanceExtensions(&required_extension_count);

            // this is fatal (at least fatal enough to throw an exception) here 'cause if we're in this function
            // the user explicitly asked for GLFW support
            check::on_vulkan_error::n_assert(required_extensions != nullptr, "GLFW failed to find the platform surface extensions");

            // insert the needed instance extensions:
            for (size_t i = 0; i < required_extension_count; ++i)
              hic.require_extension(required_extensions[i]);
          }
      };
    } // namespace glfw
  } // namespace hydra
} // namespace neam

#endif // __N_18919125232751427991_2329432231_GLFW_FEATURE_REQUESTER_HPP__

