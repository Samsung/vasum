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
    CArgsBuilder() { }

    template<typename T>
    CArgsBuilder& add(T v) {
        return add(std::to_string(v));
    }

    CArgsBuilder& add(const std::vector<std::string>& v) {
        mArray.reserve(size() + v.size());
        mArray.insert(mArray.end(), v.begin(), v.end());
        return *this;
    }

    CArgsBuilder& add(const std::string& v) {
        mArray.push_back(v);
        return *this;
    }

    CArgsBuilder& add(char *v) {
        return add(std::string(v));
    }

    CArgsBuilder& add(const char *v) {
        return add(std::string(v));
    }

    const char* const* c_array() const {
        regenerate();
        return mArgs.data();
    }

    size_t size() const {
        return mArray.size();
    }

    bool empty() const {
        return size() == 0;
    }

    const char *operator[](int i) const {
        return mArray[i].c_str();
    }

private:
    void regenerate() const
    {
        std::vector<const char*>& args = *const_cast<std::vector<const char*>*>(&mArgs);
        args.clear();
        args.reserve(mArray.size() + 1);
        for (const auto& a : mArray) {
            args.push_back(a.c_str());
        }
        args.push_back(nullptr);
    }

    std::vector<std::string> mArray;
    std::vector<const char*> mArgs;
};

} // namespace utils


#endif // COMMON_UTILS_C_ARGS_HPP
