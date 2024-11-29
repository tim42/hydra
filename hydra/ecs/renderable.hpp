//
// created by : Timothée Feuillet
// date: 2024-1-28
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

#include <enfield/enfield.hpp>

#include "types.hpp"


namespace neam::hydra::ecs::concepts
{
    /// \brief Renderable concept
    /// All entities that have data that has data for the rendering process should inherit from this component
    ///
    /// There is multiple categories of "things" that can be "rendered":
    ///  - those that simply provide data to a specific pass (like a mesh for a specific pass / category)
    ///    that data is specific to the target pass and how it manages its things (particle systems / meshes / ...)
    ///    tho hydra does provide a unified way to provide this information to rendering tasks
    ///    This describe data for a pass (specifically, multiple data for one / multiple passes)
    ///  -
    ///  -
    class renderable : public ecs_concept<renderable>
    {
      private:
        using ecs_concept = ecs::ecs_concept<renderable>;

        class concept_logic : public ecs_concept::base_concept_logic
        {
          protected:
            concept_logic(typename ecs_concept::base_t& _base) : ecs_concept::base_concept_logic(_base) {}

          public:

          protected:

          private:

          private:
            concept_logic* parent = nullptr;


            friend renderable;
        };

      public:
        template<typename ConceptProvider>
        class concept_provider : public concept_logic
        {
          protected:
            using renderable_t = concept_provider<ConceptProvider>;

            concept_provider(ConceptProvider& _p) : concept_logic(static_cast<typename ecs_concept::base_t&>(_p)) {}

            /// \brief Get the parent concept provider
            ConceptProvider* get_parent()
            {
              return static_cast<ConceptProvider*>(parent);
            }

            const ConceptProvider* get_parent() const
            {
              return static_cast<const ConceptProvider*>(parent);
            }

          private:
          protected:
          private:
        };

      public:
        renderable(typename ecs_concept::param_t p) : ecs_concept(p) {}


      private:

      private:
        bool everything_is_dirty = true;
        friend ecs_concept;
    };
}

