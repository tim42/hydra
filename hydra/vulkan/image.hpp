//
// file : image.hpp
// in : file:///home/tim/projects/hydra/hydra/vulkan/image.hpp
//
// created by : Timothée Feuillet
// date: Sat Apr 30 2016 13:17:05 GMT+0200 (CEST)
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

#ifndef __N_24664119311256314033_1924421680_IMAGE_HPP__
#define __N_24664119311256314033_1924421680_IMAGE_HPP__

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>

#include "../tools/execute_pack.hpp"

#include "../hydra_exception.hpp"

#include "device.hpp"
#include "device_memory.hpp"


namespace neam
{
  namespace hydra
  {
    namespace vk
    {
      /// \brief Wrap a vulkan image.
      class image
      {
        public: // advanced
          /// \brief Create from a vulkan image
          image(device &_dev, VkImage _vk_image)
            : dev(_dev), vk_image(_vk_image), mem(_dev)
            {
              dev._vkGetImageMemoryRequirements(vk_image, &mem_requirements);
            }

        public:
          /// \brief Move constructor
          image(image &&o)
           : dev(o.dev), vk_image(o.vk_image), mem_requirements(o.mem_requirements), mem(std::move(o.mem))
          {
            o.vk_image = nullptr;
          }

          ~image()
          {
            if (vk_image)
              dev._vkDestroyImage(vk_image, nullptr);
          }

          /// \brief Create the image from a bunch of image creators
          /// An image creators allows you fine grained configuration on how
          /// the image will be used, stored, ... and is probably easier to use
          /// than a bare structure or functions with a lot of parameters
          ///
          /// In order to work, an image creator must have a static method with
          /// the following signature:
          /// static void update_image_create_info(VkImageCreateInfo &);
          ///
          /// If you're interrested in having non compile-time creators, there's
          /// a variant of create_image that can take a container of creators
          ///
          /// There's a bunch of image creators in the image_creators folder
          template<typename... ImageCreators>
          static image create_image(device &dev)
          {
            static_assert(sizeof...(ImageCreators) > 0, "you should set at least one image creator");

            VkImageCreateInfo ici;

            memset(&ici, 0, sizeof(ici));

            ici.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            ici.pNext = nullptr;

            ici.queueFamilyIndexCount = 0;
            ici.pQueueFamilyIndices = nullptr;

            NEAM_EXECUTE_PACK(ImageCreators::update_image_create_info(ici));

            VkImage vk_image;

            check::on_vulkan_error::n_throw_exception(dev._vkCreateImage(&ici, nullptr, &vk_image));
            return image(dev, vk_image);
          }

          /// \brief Create the image from a bunch of image creators
          /// An image creators allows you fine grained configuration on how
          /// the image will be used, stored, ... and is probably easier to use
          /// than a bare structure or functions with a lot of parameters
          ///
          /// In order to work, an image creator must have a method with
          /// the following signature (for compile-time):
          /// static void update_image_create_info(VkImageCreateInfo &);
          /// and (for those in the container):
          /// void update_image_create_info(VkImageCreateInfo &) const;
          /// There's also an interface that define this:
          /// image_creator_interface, but that's optional.
          ///
          /// The container parameter must be iterable with that loop:
          ///   for (const auto &it : container)
          /// This mean C arrays, most of std containers, and most of boost
          /// containers and probably most of every containers with
          /// begin() const and end() const
          ///
          /// Compile-time (those in template parameters) image creators are
          /// executed *before* runtime ones.
          ///
          /// There's a bunch of image creators in the image_creators folder
          template<typename... ImageCreators, typename ImageCreatorsContainerT>
          static image create_image(device &dev, const ImageCreatorsContainerT &container)
          {
            check::on_vulkan_error::n_assert((sizeof...(ImageCreators) + container.size()) > 0, "you should set at least one image creator when calling image::create_image()");

            VkImageCreateInfo ici;

            memset(&ici, 0, sizeof(ici));

            ici.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            ici.pNext = nullptr;

            ici.queueFamilyIndexCount = 0;
            ici.pQueueFamilyIndices = nullptr;

            NEAM_EXECUTE_PACK(ImageCreators::update_image_create_info(ici));

            for (const auto &it : container)
              it.update_image_create_info(ici);

            VkImage vk_image;

            check::on_vulkan_error::n_throw_exception(dev._vkCreateImage(&ici, nullptr, &vk_image));
            return image(dev, vk_image);
          }

          /// \brief allocate the memory for the image.
          /// \param requirements is the requirements the memory must meet
          ///                  and should be a value of VkMemoryPropertyFlagBits
          /// \note The memory is managed by the image instance and freed when
          ///       its life ends. You can still detach the memory and claim it
          ///       for some other use, but then its management is yours.
          /// \note If a memory area was previously allocated for the texture,
          ///       it is freed.
          void allocate_memory(VkFlags requirements = 0)
          {
            mem.allocate(mem_requirements.size, requirements, mem_requirements.memoryTypeBits);

            dev._vkBindImageMemory(vk_image, mem._get_vk_device_memory(), 0);
          }

          /// \brief Use an already allocated memory for the image
          /// The operation will fail if there isn't enough space in the
          /// allocated area.
          /// \note The memory is then managed by the image object. If you
          /// want to get it back, there's the detach_memory() method.
          void use_allocated_memory(device_memory &&_mem)
          {
            check::on_vulkan_error::n_assert(_mem.get_size() >= mem_requirements.size, "supplied device_memory does not have enough space to store the image");

            mem = std::move(_mem);
            dev._vkBindImageMemory(vk_image, mem._get_vk_device_memory(), 0);
          }

          /// \brief Return the memory required by the image
          size_t get_required_size() const
          {
            return mem_requirements.size;
          }

          /// \brief Return a const reference to the device_memory object that
          /// handle the memory are of the image
          /// You can use it to map the memory into the app address space, flush
          /// the changes, ...
          /// If you must have a non-const instance, you must call
          /// the detach_memory() method instead
          const device_memory &get_memory() const
          {
            return mem;
          }

          /// \brief Unbind the memory from the texture and return an instance
          /// that manage that memory
          device_memory detach_memory()
          {
            if (!mem.is_allocated())
              return device_memory(dev);

            device_memory ret = std::move(mem);
            return ret;
          }

        public: // advanced
          /// \brief Return the vulkan resource
          VkImage get_vk_image() const
          {
            return vk_image;
          }

        private:
          device &dev;
          VkImage vk_image;
          VkMemoryRequirements mem_requirements;

          device_memory mem;
      };
    } // namespace vk
  } // namespace hydra
} // namespace neam

#endif // __N_24664119311256314033_1924421680_IMAGE_HPP__

