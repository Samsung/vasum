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
 * @brief   Visitor for reading from an internet socket file descriptor
 */

#ifndef CARGO_FD_INTERNALS_FROM_FDSTORE_INTERNET_VISITOR_HPP
#define CARGO_FD_INTERNALS_FROM_FDSTORE_INTERNET_VISITOR_HPP

#include "cargo-fd/internals/from-fdstore-visitor-base.hpp"

#include <endian.h>
#include <cstring>

namespace cargo {

namespace internals {

/**
 * Internet file descriptor reading visitor.
 */
class FromFDStoreInternetVisitor : public FromFDStoreVisitorBase<FromFDStoreInternetVisitor> {
public:
    explicit FromFDStoreInternetVisitor(int fd)
        : FromFDStoreVisitorBase(fd)
    {
    }

    FromFDStoreInternetVisitor(FromFDStoreVisitorBase<FromFDStoreInternetVisitor>& visitor)
        : FromFDStoreVisitorBase<FromFDStoreInternetVisitor>(visitor)
    {
    }

    template<typename T>
    void visitImpl(T& value)
    {
        readInternal(value);
    }

private:
    template<typename T,
             typename std::enable_if<std::is_arithmetic<T>::value
                                     && sizeof(T) == 2, int>::type = 0>
    void readInternal(T& value)
    {
        uint16_t rawValue;
        mStore.read(&rawValue, sizeof(T));
        rawValue = be16toh(rawValue);
        ::memcpy(&value, &rawValue, sizeof(value));
    }

    template<typename T,
             typename std::enable_if<std::is_arithmetic<T>::value
                                     && sizeof(T) == 4, int>::type = 0>
    void readInternal(T& value)
    {
        uint32_t rawValue;
        mStore.read(&rawValue, sizeof(T));
        rawValue = be32toh(rawValue);
        ::memcpy(&value, &rawValue, sizeof(value));
    }

    template<typename T,
             typename std::enable_if<std::is_arithmetic<T>::value
                                     && sizeof(T) == 8, int>::type = 0>
    void readInternal(T& value)
    {
        uint64_t rawValue;
        mStore.read(&rawValue, sizeof(T));
        rawValue = be64toh(rawValue);
        ::memcpy(&value, &rawValue, sizeof(value));;
    }

    template<typename T,
             typename std::enable_if<!std::is_arithmetic<T>::value
                                     || sizeof(T) == 1, int>::type = 0>
    void readInternal(T& v)
    {
        FromFDStoreVisitorBase<FromFDStoreInternetVisitor>::visitImpl(v);
    }
};

} // namespace internals

} // namespace cargo

#endif // CARGO_FD_INTERNALS_FROM_FDSTORE_INTERNET_VISITOR_HPP
