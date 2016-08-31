//
// file : bootstrap.hpp
// in : file:///home/tim/projects/hydra/hydra/init/bootstrap.hpp
//
// created by : Timothée Feuillet
// date: Wed Apr 27 2016 21:39:22 GMT+0200 (CEST)
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

#ifndef __N_8425856979821857_753717183_BOOTSTRAP_HPP__
#define __N_8425856979821857_753717183_BOOTSTRAP_HPP__

#include <deque>

#include "hydra_init_interface.hpp"
#include "hydra_instance_creator.hpp"
#include "hydra_device_creator.hpp"

namespace neam
{
  namespace hydra
  {
    /// \brief The class that manages the initialization process of hydra & vulkan
    /// It provides a higher level of abstraction than using initialization routines
    /// one after the other. It also handle init extensions automatically.
    ///
    /// It should not be used as a standalone initialization process, but instead
    /// the main application class should inherit of it.
    ///
    /// the bootstrap class allow you to bypass some of its functionalities, but
    /// you should be carefull if you do this as some extension may not be
    /// activated, prepared or aware for/of the operations you've done
    class bootstrap
    {
      public:
        bootstrap() = default;
        ~bootstrap() = default;

        bootstrap(const bootstrap &) = delete;
        bootstrap &operator = (const bootstrap &) = delete;

        /// \brief Register an initialization extension (like GLFW)
        /// An initialization extension is a scheme to allow some non-core hydra components
        /// to perform some operation at different stages of the initialization process
        ///
        /// The bootstrap class is not responsible in freeing the memory allocated for the
        /// initialization extension object if that instance was allocated on the heap.
        void register_init_extension(init_interface &initializer)
        {
          initializers.push_back(&initializer);
          initializer.on_register();
        }

        /// \brief Register a new feature requester
        /// If your application have some special requirements and you still can use the
        /// default instance and device creation process, then creating a feature_requester_interface
        /// implementation could be a way to request some special features
        ///
        /// The bootstrap class is not responsible in freeing the memory allocated for the
        /// feature requester object if that instance was allocated on the heap.
        void register_feature_requester(feature_requester_interface &fri)
        {
          feature_requesters.push_back(&fri);
        }

        /// \brief Create a vulkan instance with the default parameters
        vk::instance create_instance(std::string application_name, size_t application_version = 1)
        {
          vk::instance instance = request_instance_creator(application_name, application_version).create_instance();
          new_instance_created(instance);
          return instance;
        }

        /// \brief Create a vulkan logical device with the default parameters
        vk::device create_device(vk::instance &instance)
        {
          hydra_device_creator hdc = request_device_creator(instance);
          auto compatible_gpu_list = hdc.filter_devices();
          check::on_vulkan_error::n_assert(compatible_gpu_list.size() > 0, "could not find a GPU compatible with the requirements of the application");

          vk::device device = hdc.create_device(compatible_gpu_list[0]);
          new_device_created(device);

          cr::out.log() << LOGGER_INFO << "vulkan device created on " << compatible_gpu_list[0].get_name() << std::endl;

          return device;
        }

      public: // bypass
        /// \brief Request an instance creator.
        /// This method will run the initialization extensions on the instance creator
        /// before returning it.
        hydra_instance_creator request_instance_creator(std::string application_name, size_t application_version = 1)
        {
          for (init_interface *ii : initializers)
            ii->pre_instance_creation();

          hydra_instance_creator hic(application_name, application_version);

          for (init_interface *ii : initializers)
            hic.require(ii->get_feature_requester());
          for (feature_requester_interface *fri : feature_requesters)
            hic.require(fri);

          return hic;
        }

        /// \brief Notify the initializer extensions that a new instance has been
        /// created. This step may be mandatory, depending on the initialization extensions
        /// enabled.
        void new_instance_created(vk::instance &instance)
        {
          for (init_interface *ii : initializers)
            ii->post_instance_creation(instance);
        }

        /// \brief Request a device creator.
        /// This method will run the initialization extensions on the device creator
        /// before returning it.
        hydra_device_creator request_device_creator(vk::instance &instance)
        {
          for (init_interface *ii : initializers)
            ii->pre_device_creation(instance);

          hydra_device_creator hdc = instance.get_device_creator();

          for (init_interface *ii : initializers)
            hdc.require(ii->get_feature_requester());
          for (feature_requester_interface *fri : feature_requesters)
            hdc.require(fri);

          return hdc;
        }

        /// \brief Notify the initializer extensions that a new logical device has been
        /// created. This step may be mandatory, depending on the initialization extensions
        /// enabled.
        void new_device_created(vk::device &device)
        {
          for (init_interface *ii : initializers)
            ii->post_device_creation(device);
        }

      private:
        std::deque<init_interface *> initializers;
        std::deque<feature_requester_interface *> feature_requesters;
    };
  } // namespace hydra
} // namespace neam

#endif // __N_8425856979821857_753717183_BOOTSTRAP_HPP__

