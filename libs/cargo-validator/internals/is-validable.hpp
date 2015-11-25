/*
 *  Copyright (c) 2015 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *  Contact: Maciej Karpiuk (m.karpiuk2@samsung.com)
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
 * @author  Maciej Karpiuk (m.karpiuk2@samsung.com)
 * @brief   Internal validation helper
 */

#ifndef CARGO_VALIDATOR_INTERNALS_IS_VALIDABLE_HPP
#define CARGO_VALIDATOR_INTERNALS_IS_VALIDABLE_HPP

#include <type_traits>
#include "cargo-validator/internals/validator-visitor.hpp"

namespace cargo {

template <typename T>
struct isValidableHelper__ {
    template <typename C> static std::true_type
    test(decltype(std::declval<C>().template accept(cargo::validator::ValidatorVisitor()))*);

    template <typename C> static std::false_type
    test(...);

    static constexpr bool value = std::is_same<decltype(test<T>(0)), std::true_type>::value;
};

/**
 * Helper for compile-time checking against existance of method 'accept' with ValidatorVisitor arg.
 */
template <typename T>
struct isValidable : public std::integral_constant<bool, isValidableHelper__<T>::value> {};

} // namespace cargo

#endif // CARGO_VALIDATOR_INTERNALS_IS_VALIDABLE_HPP
