//
// created by : Timothée Feuillet
// date: 2023-10-7
//
//
// Copyright (c) 2023 Timothée Feuillet
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

#include <enfield/enfield.hpp>

#include <enfield/concept/serializable.hpp>
#include <enfield/component/data_holder.hpp>

#include "conf.hpp"

namespace neam::hydra::ecs
{
  using entity_id_t = id_t;

  /// \brief default hydra db-conf
  /// \note hydra is pure eccs (not the conservative one) as the hierarchical concept require the hierarchy component.
  using db_conf = conf::eccs;

  using database = enfield::database<db_conf>;
  using entity = enfield::entity<db_conf>;
  using entity_weak_ref = enfield::entity_weak_ref<db_conf>;
  using base = enfield::attached_object::base<db_conf>;
  template<typename T> using type_id = enfield::type_id<T, typename db_conf::attached_object_type>;

  // Shortcuts, avoiding specifying the db-conf for the default case
  template<typename T> using component = enfield::component<db_conf, T>;
  template<typename T> using internal_component = conf::internal_component<db_conf, T>;
  template<typename T> using ecs_concept = enfield::ecs_concept<db_conf, T>;
  template<typename T> using system = enfield::system<db_conf, T>;


  using serializable = enfield::concepts::serializable<db_conf>;

  template<typename Type, template<typename X> class... ConceptProviders>
  using data_holder = enfield::components::data_holder<db_conf, Type, ConceptProviders...>;

  template<template<typename X> class... ConceptProviders>
  using name_component = data_holder<std::string, ConceptProviders...>;
}

