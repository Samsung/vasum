/*
 *  Copyright (c) 2014 Samsung Electronics Co., Ltd All Rights Reserved
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
 * @author   Pawel Kubik (p.kubik@samsung.com)
 * @defgroup libcargo-fd libcargo-fd
 * @brief    cargo file descriptor interface
 */

#ifndef CARGO_FD_CARGO_FD_HPP
#define CARGO_FD_CARGO_FD_HPP

#include "cargo-fd/internals/to-fdstore-visitor.hpp"
#include "cargo-fd/internals/to-fdstore-internet-visitor.hpp"
#include "cargo-fd/internals/from-fdstore-visitor.hpp"
#include "cargo-fd/internals/from-fdstore-internet-visitor.hpp"


namespace cargo {

/*@{*/

/**
 * Load binary data from a file/socket/pipe represented by the fd
 *
 * @param fd        file descriptor
 * @param visitable visitable structure to load
 */
template <class Cargo>
void loadFromFD(const int fd, Cargo& visitable)
{
    static_assert(internals::isVisitable<Cargo>::value, "Use CARGO_REGISTER macro");

    internals::FromFDStoreVisitor visitor(fd);
    visitable.accept(visitor);
}

/**
 * Save binary data to a file/socket/pipe represented by the fd
 *
 * @param fd        file descriptor
 * @param visitable visitable structure to save
 */
template <class Cargo>
void saveToFD(const int fd, const Cargo& visitable)
{
    static_assert(internals::isVisitable<Cargo>::value, "Use CARGO_REGISTER macro");

    internals::ToFDStoreVisitor visitor(fd);
    visitable.accept(visitor);
}

/**
 * Load binary data from an internet socket represented by the fd
 *
 * @param fd        file descriptor
 * @param visitable visitable structure to load
 */
template <class Cargo>
void loadFromInternetFD(const int fd, Cargo& visitable)
{
    static_assert(internals::isVisitable<Cargo>::value, "Use CARGO_REGISTER macro");

    internals::FromFDStoreInternetVisitor visitor(fd);
    visitable.accept(visitor);
}

/**
 * Save binary data to an internet socket represented by the fd
 *
 * @param fd        file descriptor
 * @param visitable visitable structure to save
 */
template <class Cargo>
void saveToInternetFD(const int fd, const Cargo& visitable)
{
    static_assert(internals::isVisitable<Cargo>::value, "Use CARGO_REGISTER macro");

    internals::ToFDStoreInternetVisitor visitor(fd);
    visitable.accept(visitor);
}

} // namespace cargo

/*@}*/

#endif // CARGO_FD_CARGO_FD_HPP
