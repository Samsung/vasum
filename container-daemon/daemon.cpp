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
 * @brief   Definition of the Daemon, implementation of the logic of the daemon.
 */

#include "config.hpp"

#include "daemon.hpp"

#include "logger/logger.hpp"


namespace security_containers {
namespace container_daemon {


Daemon::Daemon()
{
    mConnectionPtr.reset(new DaemonConnection(std::bind(&Daemon::onNameLostCallback, this),
                                              std::bind(&Daemon::onGainFocusCallback, this),
                                              std::bind(&Daemon::onLoseFocusCallback, this)));

}

Daemon::~Daemon()
{
}

void Daemon::onNameLostCallback()
{
    //TODO: Try to reconnect or close the daemon.
    LOGE("Dbus name lost");
}

void Daemon::onGainFocusCallback()
{
    LOGD("Gained Focus");
}

void Daemon::onLoseFocusCallback()
{
    LOGD("Lost Focus");

}

} // namespace container_daemon
} // namespace security_containers
