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
 * @brief   Visitor for loading from FDStore
 */

#ifndef CARGO_FROM_FDSTORE_VISITOR_HPP
#define CARGO_FROM_FDSTORE_VISITOR_HPP

#include "cargo/is-visitable.hpp"
#include "cargo/fdstore.hpp"
#include "cargo/types.hpp"
#include "cargo/visit-fields.hpp"

#include <array>
#include <string>
#include <utility>
#include <vector>

namespace cargo {

class FromFDStoreVisitor {
public:
    explicit FromFDStoreVisitor(int fd)
        : mStore(fd)
    {
    }

    FromFDStoreVisitor(const FromFDStoreVisitor&) = default;

    FromFDStoreVisitor& operator=(const FromFDStoreVisitor&) = delete;

    template<typename T>
    void visit(const std::string&, T& value)
    {
        readInternal(value);
    }

private:
    FDStore mStore;

    void readInternal(std::string& value)
    {
        size_t size;
        readInternal(size);
        value.resize(size);
        mStore.read(&value.front(), size);
    }

    void readInternal(char* &value)
    {
        size_t size;
        readInternal(size);

        value = new char[size + 1];
        mStore.read(value, size);
        value[size] = '\0';
    }

    void readInternal(cargo::FileDescriptor& fd)
    {
        fd = mStore.receiveFD();
    }

    template<typename T, typename std::enable_if<std::is_arithmetic<T>::value, int>::type = 0>
    void readInternal(T& value)
    {
        mStore.read(&value, sizeof(T));
    }

    template<typename T, typename std::enable_if<isVisitable<T>::value, int>::type = 0>
    void readInternal(T& value)
    {
        FromFDStoreVisitor visitor(*this);
        value.accept(visitor);
    }

    template<typename T, typename std::enable_if<std::is_enum<T>::value, int>::type = 0>
    void readInternal(T& value)
    {
        readInternal(*reinterpret_cast<typename std::underlying_type<T>::type*>(&value));
    }

    template<typename T>
    void readInternal(std::vector<T>& values)
    {
        size_t vectorSize;
        readInternal(vectorSize);
        values.resize(vectorSize);

        for (T& value : values) {
            readInternal(value);
        }
    }

    template<typename T, std::size_t N>
    void readInternal(std::array<T, N>& values)
    {
        for (T& value : values) {
            readInternal(value);
        }
    }

    template<typename ... T>
    void readInternal(std::pair<T...>& values)
    {
        visitFields(values, this, std::string());
    }
};

} // namespace cargo

#endif // CARGO_FROM_FDSTORE_VISITOR_HPP
