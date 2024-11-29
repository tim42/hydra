//
// created by : Timothée Feuillet
// date: 2024-3-8
//
//
// Copyright (c) 2024 Timothée Feuillet
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


// To qualify for being a global descriptor set, one would be a unique, constant for the whole frame, set of data
// The data has to *independent* of the render-context.
//  example:
//    - textures and meshes are global descriptor sets, as they come from the packed data/resources.
//    - scene information is not a global descriptor set. You can have a render context associated to one scene, and another one associated to another.
//  The goal of a global DS is to bind it once when needed and forget about it
#define HYDRA_GLOBAL_DESCRIPTOR_SET_BASE_INDEX        10


// NOTE: these macro are directly expanded in hydra text replace thingy, which mean no constant folding or anything of the sort...
#define HYDRA_GLOBAL_DESCRIPTOR_SET_TEXTURE_MANAGER   10
#define HYDRA_GLOBAL_DESCRIPTOR_SET_MESH_MANAGER      11

