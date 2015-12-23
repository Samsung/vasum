/*
 *  Copyright (c) 2015 Samsung Electronics Co., Ltd All Rights Reserved
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
 * @brief   Visitor for writing to an internet socket file descriptor
 */

#ifndef CARGO_FD_INTERNALS_TO_FDSTORE_INTERNET_VISITOR_HPP
#define CARGO_FD_INTERNALS_TO_FDSTORE_INTERNET_VISITOR_HPP

#include "to-fdstore-visitor-base.hpp"
#include "cargo/types.hpp"

#include <endian.h>

namespace cargo {

namespace internals {

/**
 * Internet file descriptor writing visitor.
 */
class ToFDStoreInternetVisitor : public ToFDStoreVisitorBase<ToFDStoreInternetVisitor> {
public:
    explicit ToFDStoreInternetVisitor(int fd)
        : ToFDStoreVisitorBase(fd)
    {
    }

    ToFDStoreInternetVisitor(ToFDStoreVisitorBase<ToFDStoreInternetVisitor>& visitor)
        : ToFDStoreVisitorBase<ToFDStoreInternetVisitor>(visitor)
    {
    }

    template<typename T>
    void visitImpl(T& value)
    {
        writeInternal(value);
    }

private:
    template<typename T,
             typename std::enable_if<std::is_arithmetic<T>::value
                                     && sizeof(T) == 2, int>::type = 0>
    void writeInternal(T& value)
    {
        const uint16_t leValue = htobe16(*reinterpret_cast<const uint16_t*>(&value));
        mStore.write(&leValue, sizeof(T));
    }

    template<typename T,
             typename std::enable_if<std::is_arithmetic<T>::value
                                     && sizeof(T) == 4, int>::type = 0>
    void writeInternal(T& value)
    {
        const uint32_t leValue = htobe32(*reinterpret_cast<const uint32_t*>(&value));
        mStore.write(&leValue, sizeof(T));
    }

    template<typename T,
             typename std::enable_if<std::is_arithmetic<T>::value
                                     && sizeof(T) == 8, int>::type = 0>
    void writeInternal(T& value)
    {
        const uint64_t leValue = htobe64(*reinterpret_cast<const uint64_t*>(&value));
        mStore.write(&leValue, sizeof(T));
    }

    template<typename T,
             typename std::enable_if<!std::is_arithmetic<T>::value
                                     || sizeof(T) == 1, int>::type = 0>
    void writeInternal(T& v)
    {
        ToFDStoreVisitorBase<ToFDStoreInternetVisitor>::visitImpl(v);
    }
};

} // namespace internals

} // namespace cargo

#endif // CARGO_FD_INTERNALS_TO_FDSTORE_INTERNET_VISITOR_HPP
