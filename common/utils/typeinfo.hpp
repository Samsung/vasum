/*
 *  Copyright (c) 2014 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *  Contact: Jan Olszak (j.olszak@samsung.com)
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
 * @author  Jan Olszak (j.olszak@samsung.com)
 * @brief   Synchronization latch
 */

#ifndef COMMON_TYPE_INFO_HPP
#define COMMON_TYPE_INFO_HPP

#include <string>
#include <typeinfo>

namespace utils {

std::string getTypeName(const std::type_info& ti);

template<class T> std::string getTypeName(const T& t)
{
    return getTypeName(typeid(t));
}

} // namespace utils


#endif // COMMON_TYPE_INFO_HPP
