/*
 *  Copyright (c) 2014 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *  Contact: Pawel Kubik (p.kubik@samsung.com)
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License
 */

/**
 * @file
 * @author   Pawel Kubik (p.kubik@samsung.com)
 * @defgroup libcargo-gvariant libcargo-gvariant
 * @brief    cargo GVariant interface
 */

#ifndef CARGO_GVARIANT_CARGO_GVARIANT_HPP
#define CARGO_GVARIANT_CARGO_GVARIANT_HPP

#include "cargo-gvariant/internals/to-gvariant-visitor.hpp"
#include "cargo-gvariant/internals/from-gvariant-visitor.hpp"

namespace cargo {

/*@{*/

/**
 * Fills the cargo structure with data stored in the GVariant
 *
 * @param gvariant      data in GVariant type
 * @param visitable     visitable structure to fill
 */
template <class Cargo>
void loadFromGVariant(GVariant* gvariant, Cargo& visitable)
{
    static_assert(internals::isVisitable<Cargo>::value, "Use CARGO_REGISTER macro");
    static_assert(!internals::isUnion<Cargo>::value, "Don't use CARGO_DECLARE_UNION in top level visitable");

    internals::FromGVariantVisitor visitor(gvariant);
    visitable.accept(visitor);
}

/**
 * Saves the visitable in a GVariant
 *
 * @param visitable   visitable structure to convert
 */
template <class Cargo>
GVariant* saveToGVariant(const Cargo& visitable)
{
    static_assert(internals::isVisitable<Cargo>::value, "Use CARGO_REGISTER macro");
    static_assert(!internals::isUnion<Cargo>::value, "Don't use CARGO_DECLARE_UNION in top level visitable");

    internals::ToGVariantVisitor visitor;
    visitable.accept(visitor);
    return visitor.toVariant();
}

} // namespace cargo

/*@}*/

#endif // CARGO_GVARIANT_CARGO_GVARIANT_HPP
