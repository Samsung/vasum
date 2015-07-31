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
 * @brief   Declaration of the Daemon, implementation of the logic of the daemon.
 */


#ifndef ZONE_DAEMON_DAEMON_HPP
#define ZONE_DAEMON_DAEMON_HPP

#include "daemon-connection.hpp"

#include <memory>


namespace vasum {
namespace zone_daemon {

class Daemon {
public:
    Daemon();
    virtual ~Daemon();

private:
    void onNameLostCallback() const;
    void onGainFocusCallback() const;
    void onLoseFocusCallback() const;
    std::unique_ptr<DaemonConnection> mConnectionPtr;
};


} // namespace zone_daemon
} // namespace vasum


#endif // ZONE_DAEMON_DAEMON_HPP
