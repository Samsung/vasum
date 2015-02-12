/*
 *  Copyright (c) 2015 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *  Contact: Mateusz Malicki <m.malicki2@samsung.com>
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
 * @author  Mateusz Malicki (m.malicki2@samsung.com)
 * @brief   Network devices management functions declaration
 */

#ifndef SERVER_NETDEV_HPP
#define SERVER_NETDEV_HPP

#include <string>
#include <linux/if_link.h>
#include <sys/types.h>

namespace vasum {
namespace netdev {

void createVeth(const pid_t& nsPid, const std::string& nsDev, const std::string& hostDev);
void createMacvlan(const pid_t& nsPid,
                   const std::string& nsDev,
                   const std::string& hostDev,
                   const macvlan_mode& mode);
void movePhys(const pid_t& nsPid, const std::string& devId);

} //namespace netdev
} //namespace vasum

#endif // SERVER_NETDEV_HPP
