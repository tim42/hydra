//
// file : feature_requester_interface.hpp
// in : file:///home/tim/projects/hydra/hydra/init/feature_requester_interface.hpp
//
// created by : Timothée Feuillet
// date: Tue Apr 26 2016 11:31:36 GMT+0200 (CEST)
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

#ifndef __N_19988267012501112031_521026960_FEATURE_REQUESTER_INTERFACE_HPP__
#define __N_19988267012501112031_521026960_FEATURE_REQUESTER_INTERFACE_HPP__

namespace neam
{
  namespace hydra
  {
    class hydra_instance_creator;
    class hydra_device_creator;
    class hydra_vulkan_instance;

    /// \brief This is the base interface for device and instance requesters.
    /// You don't have to use it, except if you want to store and use requesters
    class feature_requester_interface
    {
      public:
        virtual ~feature_requester_interface() {}

        virtual void request_instace_layers_extensions(hydra_instance_creator &) = 0;
        virtual void request_device_layers_extensions(const hydra_vulkan_instance &, hydra_device_creator &) = 0;
    };
  } // namespace hydra
} // namespace neam

#endif // __N_19988267012501112031_521026960_FEATURE_REQUESTER_INTERFACE_HPP__
