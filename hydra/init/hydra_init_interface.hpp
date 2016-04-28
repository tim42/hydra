//
// file : hydra_init_interface.hpp
// in : file:///home/tim/projects/hydra/hydra/init/hydra_init_interface.hpp
//
// created by : Timothée Feuillet
// date: Wed Apr 27 2016 20:43:04 GMT+0200 (CEST)
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

#ifndef __N_36986591700713807_658417094_HYDRA_INIT_INTERFACE_HPP__
#define __N_36986591700713807_658417094_HYDRA_INIT_INTERFACE_HPP__

namespace neam
{
  namespace hydra
  {
    namespace vk
    {
      class instance;
      class device;
    } // namespace vk
    class feature_requester_interface;

    /// \brief The interface that any extension class that want to participate to the
    /// initialization process should implement.
    /// Like said before, the application developper doesn't have to implement this
    /// interface, depending on the initialization process chosen for the application.
    ///
    /// A notable implementation of this class is the GLFW extension.
    ///
    /// \todo I don't really like interfaces like those as they does not handle (by definition)
    ///       some kind of use cases. This may change in the future, if I found something better.
    class init_interface
    {
      public:
        virtual ~init_interface() {}

        /// \brief called when the interface is registered
        virtual void on_register() = 0;

        /// \brief Called just before the creation of a new instance
        virtual void pre_instance_creation() = 0;

        /// \brief Called just after the creation of a new instance
        virtual void post_instance_creation(vk::instance &new_instance) = 0;

        /// \brief Called just before the creation of a new device
        virtual void pre_device_creation(vk::instance &instance) = 0;
        /// \brief Called just after the creation of a new device
        virtual void post_device_creation(vk::device &new_device) = 0;

        /// \brief Return the "feature requester" of the extension
        virtual feature_requester_interface &get_feature_requester() = 0;
    };
  } // namespace hydra
} // namespace neam

#endif // __N_36986591700713807_658417094_HYDRA_INIT_INTERFACE_HPP__

