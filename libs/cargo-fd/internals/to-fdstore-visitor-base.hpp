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
 * @brief   Base of visitors for writing to a file descriptor
 */

#ifndef CARGO_FD_INTERNALS_TO_FDSTORE_VISITOR_BASE_HPP
#define CARGO_FD_INTERNALS_TO_FDSTORE_VISITOR_BASE_HPP

#include "cargo-fd/internals/fdstore.hpp"
#include "cargo/internals/is-visitable.hpp"
#include "cargo/internals/is-like-tuple.hpp"
#include "cargo/internals/visit-fields.hpp"

#include <array>
#include <cstring>
#include <string>
#include <vector>
#include <map>
#include <utility>


namespace cargo {

namespace internals {

/**
 * Base class for file descriptor writing visitors.
 *
 * A curiously recurring template pattern example. This
 * base-class provides a set of recursively called function templates. A child class can
 * modify the base class behaviour in certain cases by defining:
 *  - a set of functions that match those cases
 *  - a template function that match any other case and calls the base class function
 */
template<typename RecursiveVisitor>
class ToFDStoreVisitorBase {

public:
    template<typename T>
    void visit(const std::string&, T& value)
    {
        static_cast<RecursiveVisitor*>(this)->visitImpl(value);
    }

protected:
    FDStore mStore;

    explicit ToFDStoreVisitorBase(int fd)
        : mStore(fd)
    {
    }

    explicit ToFDStoreVisitorBase(const ToFDStoreVisitorBase&) = default;

    template<typename T>
    void visit(const T& value)
    {
        static_cast<RecursiveVisitor*>(this)->visitImpl(value);
    }

    template<typename T>
    void visitImpl(T& value)
    {
        writeInternal(value);
    }

private:
    void writeInternal(const std::string& value)
    {
        visit(value.size());
        mStore.write(value.c_str(), value.size());
    }

    void writeInternal(const char* &value)
    {
        size_t size = std::strlen(value);
        visit(size);
        mStore.write(value, size);
    }

    template<typename T, typename std::enable_if<std::is_enum<T>::value, int>::type = 0>
    void writeInternal(const T& value)
    {
        visit(static_cast<const typename std::underlying_type<T>::type>(value));
    }

    template<typename T, typename std::enable_if<std::is_arithmetic<T>::value, int>::type = 0>
    void writeInternal(const T& value)
    {
        mStore.write(&value, sizeof(T));
    }

    template<typename T, typename std::enable_if<isVisitable<T>::value, int>::type = 0>
    void writeInternal(const T& value)
    {
        RecursiveVisitor visitor(*this);
        value.accept(visitor);
    }

    template<typename T>
    void writeInternal(const std::vector<T>& values)
    {
        visit(values.size());
        for (const T& value: values) {
            visit(value);
        }
    }

    template<typename T, std::size_t N>
    void writeInternal(const std::array<T, N>& values)
    {
        for (const T& value: values) {
            visit(value);
        }
    }

    template<typename V>
    void writeInternal(const std::map<std::string, V>& values)
    {
        visit(values.size());
        for (const auto& value: values) {
            visit(value);
        }
    }

    template<typename T, typename std::enable_if<isLikeTuple<T>::value, int>::type = 0>
    void writeInternal(const T& values)
    {
        visitFields(values, this, std::string());
    }

};

} // namespace internals

} // namespace cargo

#endif // CARGO_FD_INTERNALS_TO_FDSTORE_VISITOR_BASE_HPP
