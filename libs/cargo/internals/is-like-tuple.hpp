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
 * @author  Pawel Kubik (p.kubik@samsung.com)
 * @brief   Tuple or pair checker
 */

#ifndef CARGO_INTERNALS_IS_LIKE_TUPLE_HPP
#define CARGO_INTERNALS_IS_LIKE_TUPLE_HPP

#include <tuple>

namespace cargo {
namespace internals {

template<typename T>
struct isLikeTuple {
    static constexpr bool value = std::false_type::value;
};

template<typename ... R>
struct isLikeTuple<std::tuple<R...>> {
    static constexpr bool value = std::true_type::value;
};

template<typename ... R>
struct isLikeTuple<std::pair<R...>> {
    static constexpr bool value = std::true_type::value;
};

} // namespace internals
} // namespace cargo

#endif // CARGO_INTERNALS_IS_LIKE_TUPLE_HPP

