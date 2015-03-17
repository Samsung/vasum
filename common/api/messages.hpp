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
 * @brief   Host's internal IPC messages declaration
 */


#ifndef COMMON_API_MESSAGES
#define COMMON_API_MESSAGES

#include "config/fields.hpp"
#include <string>
#include <vector>

namespace vasum {
namespace api {

struct Void {
    CONFIG_REGISTER_EMPTY
};

struct String {
    std::string value;

    CONFIG_REGISTER
    (
        value
    )
};

struct StringPair {
    std::string first;
    std::string second;

    CONFIG_REGISTER
    (
        first,
        second
    )
};

struct VectorOfStrings {
    std::vector<std::string> values;

    CONFIG_REGISTER
    (
        values
    )
};

struct VectorOfStringPairs {
    std::vector<api::StringPair> values;

    CONFIG_REGISTER
    (
        values
    )
};

typedef api::String ZoneId;
typedef api::String Declaration;
typedef api::String FileMoveRequestStatus;
typedef api::StringPair GetNetDevAttrsIn;
typedef api::StringPair CreateNetDevPhysIn;
typedef api::StringPair RemoveDeclarationIn;
typedef api::StringPair CreateZoneIn;
typedef api::StringPair RevokeDeviceIn;
typedef api::StringPair DestroyNetDevIn;
typedef api::VectorOfStrings ZoneIds;
typedef api::VectorOfStrings Declarations;
typedef api::VectorOfStrings NetDevList;
typedef api::VectorOfStringPairs Dbuses;
typedef api::VectorOfStringPairs GetNetDevAttrs;

struct ZoneInfoOut {
    std::string id;
    int vt;
    std::string state;
    std::string rootPath;

    CONFIG_REGISTER
    (
        id,
        vt,
        state,
        rootPath
    )
};

struct SetNetDevAttrsIn {
    std::string id; // Zone's id
    std::string netDev;
    std::vector<StringPair> attrs;

    CONFIG_REGISTER
    (
        id,
        netDev,
        attrs
    )
};

struct CreateNetDevVethIn {
    std::string id;
    std::string zoneDev;
    std::string hostDev;

    CONFIG_REGISTER
    (
        id,
        zoneDev,
        hostDev
    )
};

struct CreateNetDevMacvlanIn {
    std::string id;
    std::string zoneDev;
    std::string hostDev;
    uint32_t mode;

    CONFIG_REGISTER
    (
        id,
        zoneDev,
        hostDev,
        mode
    )
};

struct DeleteNetdevIpAddressIn {
    std::string zone;
    std::string netdev;
    std::string ip;

    CONFIG_REGISTER
    (
        zone,
        netdev,
        ip
    )
};

struct DeclareFileIn {
    std::string zone;
    int32_t type;
    std::string path;
    int32_t flags;
    int32_t mode;

    CONFIG_REGISTER
    (
        zone,
        type,
        path,
        flags,
        mode
    )
};

struct DeclareMountIn {
    std::string source;
    std::string zone;
    std::string target;
    std::string type;
    uint64_t flags;
    std::string data;

    CONFIG_REGISTER
    (
        source,
        zone,
        target,
        type,
        flags,
        data
    )
};

struct DeclareLinkIn
{
    std::string source;
    std::string zone;
    std::string target;

    CONFIG_REGISTER
    (
        source,
        zone,
        target
    )
};

struct GrantDeviceIn
{
    std::string id;
    std::string device;
    uint32_t flags;

    CONFIG_REGISTER
    (
        id,
        device,
        flags
    )
};

} // namespace api
} // namespace vasum

#endif // COMMON_API_MESSAGES
