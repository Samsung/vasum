/*
 *  Copyright (c) 2014 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *  Contact: Piotr Bartosiewicz (p.bartosiewi@partner.samsung.com)
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
 * @author  Krzysztof Dynowski (k.dynowski@samsumg.com)
 * @brief   Internal configuration helper
 */

#ifndef CARGO_INTERNALS_IS_UNION_HPP
#define CARGO_INTERNALS_IS_UNION_HPP

#include "cargo/internals/is-visitable.hpp"

namespace cargo {
namespace internals {

// generic member checker, start
template <typename T, typename F>
struct has_member_impl {
    template <typename C>
    static std::true_type check(typename F::template checker__<C>* =0);

    template <typename C>
    static std::false_type check(...);

    static const bool value = std::is_same<decltype(check<T>(0)), std::true_type>::value;
};

template <typename T, typename F>
struct has_member : public std::integral_constant<bool, has_member_impl<T, F>::value> {};
// generic member checker, end


template <typename X>
struct check_union : isVisitable<X> {
    template <typename T,
         //list of function union must implement
         const X& (T::*)() const = &T::as,
         X& (T::*)(const X& src) = &T::set,
         bool (T::*)() = &T::isSet
    >
    struct checker__ {};
};
template<typename T>
struct isUnion : has_member<T, check_union<T>> {};

//Note:
// unfortunately, above generic has_member can't be used for isVisitable
// because Vistable need 'accept' OR 'accept const', while has_member make exect match
// e.g accept AND accept const

} // namespace internals
} // namespace cargo

#endif // CARGO_INTERNALS_IS_UNION_HPP

