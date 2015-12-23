/*
 *  Copyright (c) 2014-2015 Samsung Electronics Co., Ltd All Rights Reserved
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
 * @brief   Base of visitors for reading from a file descriptor
 */

#ifndef CARGO_FD_INTERNALS_FROM_FDSTORE_VISITOR_BASE_HPP
#define CARGO_FD_INTERNALS_FROM_FDSTORE_VISITOR_BASE_HPP

#include "cargo-fd/internals/fdstore.hpp"
#include "cargo/internals/is-visitable.hpp"
#include "cargo/internals/is-like-tuple.hpp"
#include "cargo/internals/visit-fields.hpp"

#include <array>
#include <string>
#include <utility>
#include <vector>
#include <map>

namespace cargo {

namespace internals {

/**
 * Base class for file descriptor reading visitors.
 *
 * A curiously recurring template pattern example. This
 * base-class provides a set of recursively called function templates. A child class can
 * modify the base class behaviour in certain cases by defining:
 *  - a set of functions that match those cases
 *  - a template function that match any other case and calls the base class function
 */
template<typename RecursiveVisitor>
class FromFDStoreVisitorBase {
public:
    template<typename T>
    void visit(const std::string&, T& value)
    {
        static_cast<RecursiveVisitor*>(this)->visitImpl(value);
    }

protected:
    FDStore mStore;

    explicit FromFDStoreVisitorBase(int fd)
        : mStore(fd)
    {
    }

    FromFDStoreVisitorBase(const FromFDStoreVisitorBase&) = default;

    template<typename T>
    void visit(T& value)
    {
        static_cast<RecursiveVisitor*>(this)->visitImpl(value);
    }

    template<typename T>
    void visitImpl(T& value)
    {
        readInternal(value);
    }

private:
    void readInternal(std::string& value)
    {
        size_t size;
        visit(size);
        value.resize(size);
        mStore.read(&value.front(), size);
    }

    void readInternal(char* &value)
    {
        size_t size;
        visit(size);

        value = new char[size + 1];
        mStore.read(value, size);
        value[size] = '\0';
    }

    template<typename T, typename std::enable_if<std::is_arithmetic<T>::value, int>::type = 0>
    void readInternal(T& value)
    {
        mStore.read(&value, sizeof(T));
    }

    template<typename T, typename std::enable_if<isVisitable<T>::value, int>::type = 0>
    void readInternal(T& value)
    {
        RecursiveVisitor visitor(*this);
        value.accept(visitor);
    }

    template<typename T, typename std::enable_if<std::is_enum<T>::value, int>::type = 0>
    void readInternal(T& value)
    {
        visit(*reinterpret_cast<typename std::underlying_type<T>::type*>(&value));
    }

    template<typename T>
    void readInternal(std::vector<T>& values)
    {
        size_t vectorSize;
        visit(vectorSize);
        values.resize(vectorSize);

        for (T& value : values) {
            visit(value);
        }
    }

    template<typename T, std::size_t N>
    void readInternal(std::array<T, N>& values)
    {
        for (T& value : values) {
            visit(value);
        }
    }

    template<typename V>
    void readInternal(std::map<std::string, V>& values)
    {
        size_t mapSize;
        visit(mapSize);

        for (size_t i = 0; i < mapSize; ++i) {
            std::pair<std::string, V> val;
            visit(val);
            values.insert(std::move(val));
        }
    }

    template<typename T, typename std::enable_if<isLikeTuple<T>::value, int>::type = 0>
    void readInternal(T& values)
    {
        visitFields(values, this, std::string());
    }
};

} // namespace internals

} // namespace cargo

#endif // CARGO_FD_INTERNALS_FROM_FDSTORE_VISITOR_BASE_HPP
