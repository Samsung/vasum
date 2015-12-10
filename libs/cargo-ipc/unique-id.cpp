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
 * @brief   Unique ID type definition
 */

#include "config.hpp"

#include "unique-id.hpp"

namespace cargo {
namespace ipc {

UniqueID::UniqueID()
{
    mTime.tv_sec = 0;
    mTime.tv_nsec = 0;
    ::uuid_clear(mUUID);
}

void UniqueID::generate()
{
    ::clock_gettime(CLOCK_REALTIME, &mTime);
    ::uuid_generate_random(mUUID);
}

bool UniqueID::operator==(const UniqueID& other) const
{
    return (mTime.tv_sec == other.mTime.tv_sec)
        && (mTime.tv_nsec == other.mTime.tv_nsec)
        && (::uuid_compare(mUUID, other.mUUID) == 0); // uuid_compare works just like strcmp
}

UniqueID::operator std::string() const
{
    char uuid[37]; // uuid_unparse assures that it will print 36 chars + terminating zero
    ::uuid_unparse(mUUID, uuid);
    return std::to_string(mTime.tv_sec) + '.' + std::to_string(mTime.tv_nsec) + ':' + uuid;
}

std::ostream& operator<<(std::ostream& str, const UniqueID& id)
{
    str << static_cast<std::string>(id);
    return str;
}

} // namespace ipc
} // namespace cargo
