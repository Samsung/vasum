/*
 *  Copyright (c) 2014 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *  Contact: Mateusz Malicki <m.malicki@samsung.com>
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
 * @author  Mateusz Malicki (m.malicki2@samsung.com)
 * @brief   Function used to initialize C structures
 */

#ifndef COMMON_UTILS_MAKE_CLEAN_HPP
#define COMMON_UTILS_MAKE_CLEAN_HPP

#include <algorithm>
#include <type_traits>

namespace utils {

template<class T>
void make_clean(T& value)
{
    static_assert(std::is_pod<T>::value, "make_clean require trivial and standard-layout");
    std::fill_n(reinterpret_cast<char*>(&value), sizeof(value), 0);
}

template<class T>
T make_clean()
{
    T value;
    make_clean(value);
    return value;
}

} // namespace utils


#endif // COMMON_UTILS_MAKE_CLEAN_HPP
