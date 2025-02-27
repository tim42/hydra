//
// created by : Timothée Feuillet
// date: 2024-9-8
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

#include <vector>

#include <imgui.h>

namespace neam::hydra::imgui
{
  /// \brief Duplicate imgui drawdata for proper render parallelism
  /// \note Any internal field are not duplicated.
  struct draw_data_t
  {
    draw_data_t(ImDrawData& src_draw_data)
    {
      draw_data = src_draw_data;
      draw_lists.resize(draw_data.CmdLists.size(), nullptr);
      for (uint32_t i = 0; i < (uint32_t)draw_data.CmdLists.size(); ++i)
      {
        draw_lists[i].VtxBuffer = draw_data.CmdLists[i]->VtxBuffer;
        draw_lists[i].IdxBuffer = draw_data.CmdLists[i]->IdxBuffer;
        draw_lists[i].CmdBuffer = draw_data.CmdLists[i]->CmdBuffer;
        draw_data.CmdLists[i] = &draw_lists[i];
      }
    }

    ImDrawData draw_data;
    std::vector<ImDrawList> draw_lists;
  };
}
