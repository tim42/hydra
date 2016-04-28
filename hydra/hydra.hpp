//
// file : hydra.hpp
// in : file:///home/tim/projects/hydra/hydra/hydra.hpp
//
// created by : Timothée Feuillet
// date: Sat Apr 23 2016 15:19:07 GMT+0200 (CEST)
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

#ifndef __N_396519513304418946_3159317424_HYDRA_HPP__
#define __N_396519513304418946_3159317424_HYDRA_HPP__

// a logger for hydra (define HYDRA_NO_MESSAGES to disable logs)
#include "hydra_logger.hpp"
// a reflective support for hydra (define HYDRA_NO_REFLECTIVE_SUPPORT to disable this)
#include "hydra_reflective.hpp"

#include "hydra_exception.hpp"

#include "init/bootstrap.hpp"
#include "init/feature_requesters/gen_feature_requester.hpp"

#include <glm/glm.hpp>

// include main components
#include "threading/threading.hpp"

namespace neam
{
  namespace hydra
  {
  } // namespace hydra
} // namespace neam

#endif // __N_396519513304418946_3159317424_HYDRA_HPP__
