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
typedef api::VectorOfStrings ZoneIds;
typedef api::VectorOfStrings Declarations;
typedef api::VectorOfStrings NetDevList;
typedef api::VectorOfStringPairs Dbuses;
typedef api::VectorOfStringPairs NetDevAttrs;

struct ZoneInfo {
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

// struct MethodSetActiveZoneConfig {
//     std::string id;

//     CONFIG_REGISTER
//     (
//         id
//     )
// };

// struct MethodGetZoneDbusesConfig {
//     CONFIG_REGISTER_EMPTY
// };

// struct MethodGetZoneIdListConfig {
//     CONFIG_REGISTER_EMPTY
// };


// struct MethodGetActiveZoneIdConfig {
//     CONFIG_REGISTER_EMPTY
// };


// struct MethodGetZoneInfoConfig {
//     std::string id;

//     CONFIG_REGISTER
//     (
//         id
//     )
// };

// struct MethodSetNetDevAttrsConfig {
//     std::string zone;
//     std::string netdev;

//     struct Store {
//         std::string key;
//         std::string value;

//         CONFIG_REGISTER
//         (
//             key,
//             value
//         )
//     };

//     std::vector<Store> attrs;

//     CONFIG_REGISTER
//     (
//         zone,
//         netdev,
//         attrs
//     )
// };

// struct MethodGetNetDevAttrsConfig {
//     std::string zone;
//     std::string netdev;

//     CONFIG_REGISTER
//     (
//         zone,
//         netdev
//     )
// };

// struct MethodGetNetDevListConfig {
//     std::string zone;

//     CONFIG_REGISTER
//     (
//         zone
//     )
// };

// struct MethodCreateNetDevVethConfig {
//     std::string id;
//     std::string zoneDev;
//     std::string hostDev;

//     CONFIG_REGISTER
//     (
//         id,
//         zoneDev,
//         hostDev
//     )
// };

// struct MethodCreateNetDevMacvlanConfig {
//     std::string id;
//     std::string zoneDev;
//     std::string hostDev;

//     CONFIG_REGISTER
//     (
//         id,
//         zoneDev,
//         hostDev
//     )
// };

// struct MethodCreateNetDevPhysConfig {
//     std::string id;
//     std::string devId;

//     CONFIG_REGISTER
//     (
//         id,
//         devId
//     )
// };

// struct MethodGetDeclareFileConfig {
//     std::string zone;
//     int32_t type;
//     std::string path;
//     int32_t flags;
//     int32_t mode;

//     CONFIG_REGISTER
//     (
//         zone,
//         type,
//         path,
//         flags,
//         mode
//     )
// };

// struct MethodGetDeclareMountConfig {
//     std::string source;
//     std::string zone;
//     std::string target;
//     uint64_t flags;
//     std::string data;

//     CONFIG_REGISTER
//     (
//         source,
//         zone,
//         target,
//         flags,
//         data
//     )
// };

// struct MethodGetDeclareLinkConfig {
//     std::string source;
//     std::string zone;
//     std::string target;

//     CONFIG_REGISTER
//     (
//         source,
//         zone,
//         target
//     )
// };

// struct MethodGetDeclarationConfig {
//     std::string zone;
//     std::string declarationId;

//     CONFIG_REGISTER
//     (
//         zone,
//         declarationId
//     )
// };

// struct MethodRemoveDeclarationConfig {
//     std::string id;
//     std::string declarationId;

//     CONFIG_REGISTER
//     (
//         id,
//         declarationId
//     )
// };

// struct MethodCreateZoneConfig {
//     std::string id;
//     std::string templateName;

//     CONFIG_REGISTER
//     (
//         id,
//         templateName
//     )
// };

// struct MethodDestroyZoneConfig {
//     std::string id;

//     CONFIG_REGISTER
//     (
//         id
//     )
// };


// struct MethodShutdownZoneConfig {
//     std::string id;

//     CONFIG_REGISTER
//     (
//         id
//     )
// };

// struct MethodStartZoneConfig {
//     std::string id;

//     CONFIG_REGISTER
//     (
//         id
//     )
// };

// struct MethodLockZoneConfig {
//     std::string id;

//     CONFIG_REGISTER
//     (
//         id
//     )
// };

// struct MethodUnlockZoneConfig {
//     std::string id;

//     CONFIG_REGISTER
//     (
//         id
//     )
// };

// struct MethodGrantDeviceConfig {
//     std::string id;
//     std::string device;
//     uint32_t flags;

//     CONFIG_REGISTER
//     (
//         id,
//         device,
//         flags
//     )
// };

// struct MethodRevokeDeviceConfig {
//     std::string id;
//     std::string device;

//     CONFIG_REGISTER
//     (
//         id,
//         device
//     )
// };

// TODO: Agregate configs if it makes sense. For example: MethodLockZoneConfig and MethodUnlockZoneConfig


// Zone:
// struct MethodNotifyActiveZoneConfig {
//     std::string application;
//     std::string message;

//     CONFIG_REGISTER
//     (
//         application,
//         message
//     )
// };

// struct MethodFileMoveRequest {
//     std::string destination;
//     std::string path;

//     CONFIG_REGISTER
//     (
//         destination,
//         path
//     )
// };

// struct MethodFileMoveRequestResult {
//     std::string result;

//     CONFIG_REGISTER
//     (
//         result
//     )
// };

} // namespace api
} // namespace vasum

#endif // COMMON_API_MESSAGES
