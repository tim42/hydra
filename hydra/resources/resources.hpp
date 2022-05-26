//
// created by : Timothée Feuillet
// date: 2021-11-24
//
//
// Copyright (c) 2021 Timothée Feuillet
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

//
// NOTE: You can set N_HYDRA_RESOURCES_STRIP_DEBUG to completly remove any string data from the resources.
//       Usefull when dealing with packed data as string data is not really usefull.
//       string_id in the case will still have its functions returning strings, but instead will return nullptr.
//
// WARNING: Never serialize a string_id as it will change size when debug information is enabled or disabled

#ifndef N_HYDRA_RESOURCES_STRIP_DEBUG
  // Strip all debug information from the binary
  // This includes any strings passed as argument to string_id
  #define N_HYDRA_RESOURCES_STRIP_DEBUG 0
#endif

#ifndef N_HYDRA_RESOURCES_OBFUSCATE
  #define N_HYDRA_RESOURCES_OBFUSCATE 1
#endif

#if !N_HYDRA_RESOURCES_OBFUSCATE
  #warning Compilling without resource obfuscation.
#endif

#include "index.hpp"
#include "file_map.hpp"
#include "context.hpp"

namespace neam::resources
{
  
}


