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
 * @brief   Default visitor for writing to a file descriptor
 */

#ifndef CARGO_FD_INTERNALS_TO_FDSTORE_VISITOR_HPP
#define CARGO_FD_INTERNALS_TO_FDSTORE_VISITOR_HPP

#include "to-fdstore-visitor-base.hpp"

#include "cargo/types.hpp"

namespace cargo {

namespace internals {

/**
 * Default file descriptor writing visitor.
 */
class ToFDStoreVisitor : public ToFDStoreVisitorBase<ToFDStoreVisitor> {
public:
    explicit ToFDStoreVisitor(int fd)
        : ToFDStoreVisitorBase(fd)
    {
    }

    ToFDStoreVisitor(ToFDStoreVisitorBase<ToFDStoreVisitor>& visitor)
        : ToFDStoreVisitorBase<ToFDStoreVisitor>(visitor)
    {
    }

    template<typename T>
    void visitImpl(T& value)
    {
        writeInternal(value);
    }

private:
    void writeInternal(const cargo::FileDescriptor& fd)
    {
        mStore.sendFD(fd.value);
    }

    template<typename T>
    void writeInternal(T& v)
    {
        ToFDStoreVisitorBase<ToFDStoreVisitor>::visitImpl(v);
    }
};

} // namespace internals

} // namespace cargo

#endif // CARGO_FD_INTERNALS_TO_FDSTORE_VISITOR_HPP
