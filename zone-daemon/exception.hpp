/*
 *  Copyright (c) 2014 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *  Contact: Jan Olszak <j.olszak@samsung.com>
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
 * @brief   Exceptions for the zone-daemon
 */


#ifndef ZONE_DAEMON_EXCEPTION_HPP
#define ZONE_DAEMON_EXCEPTION_HPP

#include "base-exception.hpp"


namespace vasum {
namespace zone_daemon {

/**
 * Base class for exceptions in Vasum Zone Daemon
 */
struct ZoneDaemonException: public VasumException {

    ZoneDaemonException(const std::string& error) : VasumException(error) {}
};


} // zone_daemon
} // vasum


#endif // ZONE_DAEMON_EXCEPTION_HPP
