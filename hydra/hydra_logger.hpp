//
// file : hydra_logger.hpp
// in : file:///home/tim/projects/hydra/hydra/hydra_logger.hpp
//
// created by : Timothée Feuillet
// date: Sat Apr 23 2016 15:59:09 GMT+0200 (CEST)
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

#ifndef __N_2157120580611224779_2842314943_HYDRA_LOGGER_HPP__
#define __N_2157120580611224779_2842314943_HYDRA_LOGGER_HPP__

#ifndef HYDRA_NO_MESSAGES
#include <ntools/logger/logger.hpp>

#define HYDRA_LOG(type, ...)   neam::cr::out.log_fmt(::neam::cr::logger::severity::type, std::source_location::current(), __VA_ARGS__);
#define HYDRA_LOG_TPL(type, sloc, ...)   neam::cr::out.log_fmt(::neam::cr::logger::severity::type, sloc, __VA_ARGS__);

#else
#define HYDRA_LOG(type, ...) // NO LOGS
#endif

#endif // __N_2157120580611224779_2842314943_HYDRA_LOGGER_HPP__

