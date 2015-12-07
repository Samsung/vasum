/*
 *  Copyright (c) 2015 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *  Contact: Krzysztof Dynowski (k.dynowski@samsumg.com)
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
 * @brief   C arguments array builder (null terminated array of strings)
 */

#ifndef COMMON_UTILS_C_ARGS_HPP
#define COMMON_UTILS_C_ARGS_HPP

#include <vector>
#include <iostream>

namespace utils {

class CArgsBuilder {
public:
    CArgsBuilder(): mArray(1, nullptr) {}

    template<typename T>
    CArgsBuilder& add(T v) {
        return add(std::to_string(v));
    }

    CArgsBuilder& add(const std::vector<std::string>& v) {
        mArray.reserve(v.size() + 1);
        for (const auto& a : v) {
            add(a.c_str());
        }
        return *this;
    }

    CArgsBuilder& add(const std::string& v) {
        mCached.push_back(v);
        return add(mCached.back().c_str());
    }

    CArgsBuilder& add(char *v) {
        return add(const_cast<const char *>(v));
    }

    CArgsBuilder& add(const char *v) {
        mArray.back() = v;
        mArray.push_back(nullptr);
        return *this;
    }

    const char* const* c_array() const {
        return mArray.data();
    }

    size_t size() const {
        return mArray.size() - 1;
    }

    bool empty() const {
        return size() == 0;
    }

    const char *operator[](int i) const {
        return mArray[i];
    }

    const char* const* begin() const {
        return &*mArray.begin();
    }

    const char* const* end() const {
        return &*(mArray.end() - 1);
    }
private:
    std::vector<const char *> mArray;
    std::vector<std::string> mCached;
};

} // namespace utils


#endif // COMMON_UTILS_C_ARGS_HPP
