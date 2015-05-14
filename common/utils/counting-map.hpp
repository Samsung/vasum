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
 * @brief   Counting map
 */

#ifndef COMMON_UTILS_COUNTING_MAP_HPP
#define COMMON_UTILS_COUNTING_MAP_HPP

#include <unordered_map>

namespace utils {


/**
 * Structure used to count elements.
 * It's like multiset + count but is more efficient.
 */
template<class Key>
class CountingMap {
public:
    size_t increment(const Key& key)
    {
        auto res = mMap.insert(typename Map::value_type(key, 1));
        if (!res.second) {
            ++res.first->second;
        }
        return res.first->second;
    }

    size_t decrement(const Key& key)
    {
        auto it = mMap.find(key);
        if (it == mMap.end()) {
            return 0;
        }
        if (--it->second == 0) {
            mMap.erase(it);
            return 0;
        }
        return it->second;
    }

    void clear()
    {
        mMap.clear();
    }

    size_t get(const Key& key) const
    {
        auto it = mMap.find(key);
        return it == mMap.end() ? 0 : it->second;
    }

    bool empty() const
    {
        return mMap.empty();
    }
private:
    typedef std::unordered_map<Key, size_t> Map;
    Map mMap;
};


} // namespace utils


#endif // COMMON_UTILS_COUNTING_MAP_HPP
