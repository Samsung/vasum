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
 * @author  Piotr Bartosiewicz (p.bartosiewi@partner.samsung.com)
 * @brief   Internal configuration helper
 */

#ifndef CARGO_INTERNALS_IS_VISITABLE_HPP
#define CARGO_INTERNALS_IS_VISITABLE_HPP

#include <type_traits>

namespace cargo {
namespace internals {

template <typename T>
struct isVisitableHelper__ {
    struct Visitor {};

    template <typename C> static std::true_type
    test(decltype(std::declval<C>().template accept(Visitor()))*);

    template <typename C> static std::false_type
    test(...);

    static constexpr bool value = std::is_same<decltype(test<T>(0)), std::true_type>::value;
};

/**
 * Helper for compile-time checking against existance of template method 'accept'.
 */
template <typename T>
struct isVisitable : public std::integral_constant<bool, isVisitableHelper__<T>::value> {};

} // namespace internals
} // namespace cargo

#endif // CARGO_INTERNALS_IS_VISITABLE_HPP
