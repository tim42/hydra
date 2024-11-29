//
// file : image_loader.hpp
// in : file:///home/tim/projects/hydra/hydra/utilities/image_loader.hpp
//
// created by : Timothée Feuillet
// date: Sun Sep 04 2016 18:19:11 GMT+0200 (CEST)
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

#pragma once


#include <cstdint>
#include <string>

#include <vulkan/vulkan.h> // for image type
#include <hydra_glm.hpp>
 // for uvec2

namespace neam
{
  namespace hydra
  {
    /// \brief Define the interface for an image loader
    class image_loader
    {
      public:
        /// \brief Load an image from a file. Return an array of pixel
        /// \param format the requested return format. Please note that an implementation
        ///               may not support the requested format, and only VK_FORMAT_R8G8B8A8_UNORM
        ///               is guaranteed to be present.
        virtual uint8_t *load_image_from_file(const std::string &file, VkFormat format, glm::uvec2 &image_size) = 0;
    };
  } // namespace hydra
} // namespace neam



