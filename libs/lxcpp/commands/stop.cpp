/*
 *  Copyright (C) 2015 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License version 2.1 as published by the Free Software Foundation.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

/**
 * @file
 * @author  Jan Olszak (j.olszak@samsung.com)
 * @brief   Implementation of stopping a container
 */

#include "lxcpp/commands/stop.hpp"
#include "lxcpp/guard/api.hpp"
#include "lxcpp/exception.hpp"
#include "lxcpp/process.hpp"

#include "logger/logger.hpp"
#include "utils/signal.hpp"

namespace lxcpp {

Stop::Stop(std::shared_ptr<ContainerConfig>& config,
           std::shared_ptr<ipc::Client>& client)
    : mConfig(config),
      mClient(client)
{
}

Stop::~Stop()
{
}

void Stop::execute()
{
    std::string containerName = mConfig->mName;
    LOGD("Stopping container: " << containerName);
    auto callback = [containerName](ipc::Result<api::Void>&& result) {
        // TODO: Collect the returned init process status
        if (result.isValid()) {
            LOGI("Stopped container: " << containerName);
        }
    };
    mClient->callAsync<api::Void, api::Void>(api::METHOD_STOP, std::make_shared<api::Void>(), callback);

    // TODO: Wait till init stopped
}

} // namespace lxcpp
