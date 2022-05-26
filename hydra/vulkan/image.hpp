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

#include <tuple>

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>

// #include "../tools/execute_pack.hpp"
// #include "../tools/genseq.hpp"

#include "../hydra_debug.hpp"

#include "device.hpp"
#include "device_memory.hpp"

// default creators
#include "image_creators/image_2d.hpp"

namespace neam
{
  namespace hydra
  {
    namespace vk
    {
      /// \brief Wrap a vulkan image.
      /// \todo That is not finished, and I really HATE the fact the image class holds the memory for the image
      class image
      {
        public: // advanced
          /// \brief Create from a vulkan image
          image(device &_dev, VkImage _vk_image, const VkImageCreateInfo &_ici, bool _do_not_destroy = false)
            : dev(_dev), vk_image(_vk_image), image_create_info(_ici),
              do_not_destroy(_do_not_destroy)
            {
              dev._vkGetImageMemoryRequirements(vk_image, &mem_requirements);
            }

        public:
          /// \brief Move constructor
          image(image &&o)
            : dev(o.dev), vk_image(o.vk_image), image_create_info(o.image_create_info),
              do_not_destroy(o.do_not_destroy),
              mem_requirements(o.mem_requirements)
          {
            o.vk_image = nullptr;
          }

          image &operator = (image &&o)
          {
            if (&o == this)
              return *this;

            check::on_vulkan_error::n_assert(&o.dev == &dev, "can't assign images with different vulkan devices");

            if (vk_image && do_not_destroy == false)
              dev._vkDestroyImage(vk_image, nullptr);

            vk_image = o.vk_image;
            image_create_info = o.image_create_info;
            do_not_destroy = o.do_not_destroy;
            mem_requirements = o.mem_requirements;

            o.vk_image = nullptr;

            return *this;
          }

          /// \brief Destructor
          ~image()
          {
            if (vk_image && do_not_destroy == false)
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

            (ImageCreators::update_image_create_info(ici), ...);

            VkImage vk_image;

            check::on_vulkan_error::n_assert_success(dev._vkCreateImage(&ici, nullptr, &vk_image));
            return image(dev, vk_image, ici);
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

            (ImageCreators::update_image_create_info(ici), ...);

            for (const auto &it : container)
              it.update_image_create_info(ici);

            VkImage vk_image;

            check::on_vulkan_error::n_assert_success(dev._vkCreateImage(&ici, nullptr, &vk_image));
            return image(dev, vk_image, ici);
          }

          /// \brief Create the image from a bunch of image creators
          /// An image creators allows you fine grained configuration on how
          /// the image will be used, stored, ... and is probably easier to use
          /// than a bare structure or functions with a lot of parameters
          ///
          /// In order to work, an image creator must have a method with
          /// the following signature (for compile-time):
          /// static void update_image_create_info(VkImageCreateInfo &);
          /// and (for those in parameter):
          /// void update_image_create_info(VkImageCreateInfo &) const;
          /// There's also an interface that define this:
          /// image_creator_interface, but that's optional.
          ///
          /// Compile-time (those in template parameters) image creators are
          /// executed *before* runtime ones.
          ///
          /// There's a bunch of image creators in the image_creators folder
          template<typename... ImageCreators, typename... ImageCreatorsArgs>
          static image create_image_arg(device &dev, const ImageCreatorsArgs &... args)
          {
            check::on_vulkan_error::n_assert((sizeof...(ImageCreators) + sizeof...(ImageCreatorsArgs)) > 0, "you should set at least one image creator when calling image::create_image()");

            VkImageCreateInfo ici;

            memset(&ici, 0, sizeof(ici));

            ici.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            ici.pNext = nullptr;

            ici.queueFamilyIndexCount = 0;
            ici.pQueueFamilyIndices = nullptr;

            (ImageCreators::update_image_create_info(ici), ...);

            (args.update_image_create_info(ici), ...);

            VkImage vk_image;

            check::on_vulkan_error::n_assert_success(dev._vkCreateImage(&ici, nullptr, &vk_image));
            return image(dev, vk_image, ici);
          }

          /// \brief Bind some memory to the image
          void bind_memory(const device_memory &mem, size_t offset)
          {
            check::on_vulkan_error::n_assert_success(dev._vkBindImageMemory(vk_image, mem._get_vk_device_memory(), offset));
          }

          /// \brief Return a structure that describe the image resource
          /// (this is a vulkan structure)
          VkSubresourceLayout get_image_subresource_layout(VkImageAspectFlags mask, size_t mip_level = 0, size_t array_layer = 0) const
          {
            VkImageSubresource subres;
            subres.aspectMask = mask;
            subres.mipLevel = mip_level;
            subres.arrayLayer = array_layer;

            VkSubresourceLayout layout;
            dev._vkGetImageSubresourceLayout(vk_image, &subres, &layout);
            return layout;
          }

          /// \brief Return an image memory barrier that will create a transition to old_layout from new_layout
          /// \note as this class is pretty dumb (and generic), you will have to handle yourself specific bits of
          /// the image
//           image_memory_barrier layout_transition(VkImageLayout new_layout)
//           {
//             const VkImageLayout old_layout = image_create_info.initialLayout;
//             image_create_info.initialLayout = new_layout;
//             return image_memory_barrier(vk_image, old_layout, new_layout);
//           }

          /// \brief Return the type of image stored (1D, 2D, 3D)
          VkImageType get_image_type() const
          {
            return image_create_info.imageType;
          }

          /// \brief Return the image internal format (like R8G8B8A8, ...)
          VkFormat get_image_format() const
          {
            return image_create_info.format;
          }

          /// \brief Return the image size
          glm::uvec3 get_size() const
          {
            return glm::uvec3(image_create_info.extent.width, image_create_info.extent.height, image_create_info.extent.depth);
          }

          /// \brief Return the memory requirements of the image
          const VkMemoryRequirements &get_memory_requirements() const
          {
            return mem_requirements;
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

          VkImageCreateInfo image_create_info;

          bool do_not_destroy;
          VkMemoryRequirements mem_requirements;
      };
    } // namespace vk
  } // namespace hydra
} // namespace neam

#endif // __N_24664119311256314033_1924421680_IMAGE_HPP__

