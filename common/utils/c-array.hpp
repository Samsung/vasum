/*
 *  Copyright (c) 2014 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *  Contact: Piotr Bartosiewicz <p.bartosiewi@partner.samsung.com>
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
 * @brief   C array helper
 */

#ifndef COMMON_UTILS_C_ARRAY_HPP
#define COMMON_UTILS_C_ARRAY_HPP

#include <vector>

namespace security_containers {
namespace utils {

template<typename T>
class CArrayBuilder {
public:
    CArrayBuilder(): mArray(1, nullptr) {}

    CArrayBuilder& add(const T& v) {
        mArray.back() = v;
        mArray.push_back(nullptr);
        return *this;
    }

    const T* c_array() const {
        return mArray.data();
    }

    size_t size() const {
        return mArray.size() - 1;
    }

    bool empty() const {
        return size() == 0;
    }

    const T* begin() const {
        return &*mArray.begin();
    }

    const T* end() const {
        return &*(mArray.end() - 1);
    }
private:
    std::vector<T> mArray;
};

typedef CArrayBuilder<const char*> CStringArrayBuilder;

} // namespace utils
} // namespace security_containers


#endif // COMMON_UTILS_C_ARRAY_HPP
