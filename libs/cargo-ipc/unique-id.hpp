/*
 *  Copyright (c) 2015 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *  Contact: Lukasz Kostyra <l.kostyra@samsung.com>
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
 * @author  Lukasz Kostyra (l.kostyra@samsung.com)
 * @brief   Unique ID type declaration
 */

#ifndef CARGO_IPC_UNIQUE_ID_HPP
#define CARGO_IPC_UNIQUE_ID_HPP

#include <ostream>
#include <chrono>
#include <uuid/uuid.h>
#include <time.h>

namespace cargo {
namespace ipc {

class UniqueID {
public:
    typedef struct timespec TimestampType;
    typedef uuid_t UUIDType;

    /**
     * Default constructor. Generates an empty ID.
     */
    UniqueID();

    /**
     * Generate new timestamp and UUID pair.
     */
    void generate();

    /**
     * Compare two IDs
     *
     * @param other Other ID to compare to.
     * @return True if both timestamp and UUID are equal, false otherwise.
     */
    bool operator==(const UniqueID& other) const;

    /**
     * Casts current ID to string
     */
    operator std::string() const;

    /**
     * Overloaded << operator for debugging purposes. Used in ut-uid.cpp tests.
     */
    friend std::ostream& operator<<(std::ostream& str, const UniqueID& id);


    TimestampType mTime;    ///< timestamp when generate() was called
    UUIDType mUUID;         ///< random UUID generated with libuuid
};

template <typename T>
class hash;

template <>
class hash<cargo::ipc::UniqueID>
{
public:
    std::size_t operator()(const cargo::ipc::UniqueID& id) const
    {
        char uuid[37]; // 36 chars for UUID + terminating zero
        ::uuid_unparse(id.mUUID, uuid);

        // STL does not provide correct hash implementation for char *
        // Instead, just convert it to string
        std::string uuids(uuid);

        return std::hash<time_t>()(id.mTime.tv_sec)
             ^ std::hash<long>()(id.mTime.tv_nsec)
             ^ std::hash<std::string>()(uuids);
    }
};

} // namespace ipc
} // namespace cargo

#endif // CARGO_IPC_UNIQUE_ID_HPP
