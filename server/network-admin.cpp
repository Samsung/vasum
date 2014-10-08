/*
 *  Copyright (c) 2014 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *  Contact: Piotr Bartosiewicz <p.bartosiewi@partner.samsung.com>
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
 * @author  Piotr Bartosiewicz (p.bartosiewi@partner.samsung.com)
 * @brief   Implementation of class for administrating single network
 */

#include "config.hpp"

#include "network-admin.hpp"
#include "exception.hpp"

//#include "libvirt/helpers.hpp"
#include "logger/logger.hpp"
#include "utils/fs.hpp"

#include <cassert>


namespace security_containers {

namespace {

//std::string getNetworkName(virNetworkPtr net)
//{
//    assert(net);
//
//    const char* name = virNetworkGetName(net);
//    if (name == nullptr) {
//        LOGE("Failed to get the network's id:\n"
//             << libvirt::libvirtFormatError());
//        throw ContainerOperationException();
//    }
//
//    return name;
//}

} // namespace


NetworkAdmin::NetworkAdmin(const ContainerConfig& config)
    : mConfig(config),
      //mNWFilter(utils::readFileContent(mConfig.networkFilterConfig)),
      //mNetwork(utils::readFileContent(mConfig.networkConfig)),
      mId("TODO"),//mId(getNetworkName(mNetwork.get())),
      mDetachOnExit(false)
{
    LOGD(mId << ": Instantiating NetworkAdmin object");
}


NetworkAdmin::~NetworkAdmin()
{
    LOGD(mId << ": Destroying NetworkAdmin object...");
    // Try to stop
    if (!mDetachOnExit) {
        try {
            stop();
        } catch (ServerException&) {
            LOGE(mId << ": Failed to stop the network");
        }
    }

    LOGD(mId << ": NetworkAdmin object destroyed");
}


const std::string& NetworkAdmin::getId() const
{
    return mId;
}


void NetworkAdmin::start()
{
//    assert(mNetwork);
//
//    LOGD(mId << ": Starting...");
//    if (isActive()) {
//        LOGD(mId << ": Already running - nothing to do...");
//        return;
//    }
//
//    if (virNetworkCreate(mNetwork.get()) < 0) {
//        LOGE(mId << ": Failed to start the network\n"
//             << libvirt::libvirtFormatError());
//        throw ContainerOperationException();
//    }
//
//    LOGD(mId << ": Started");
}


void NetworkAdmin::stop()
{
//    assert(mNetwork);
//
//    LOGD(mId << ": Stopping procedure started...");
//    if (!isActive()) {
//        LOGD(mId << ": Already crashed/down/off - nothing to do");
//        return;
//    }
//
//    if (virNetworkDestroy(mNetwork.get()) < 0) {
//        LOGE(mId << ": Failed to destroy the network\n"
//             << libvirt::libvirtFormatError());
//        throw ContainerOperationException();
//    }
//
//    LOGD(mId << ": Stopping procedure ended");
}


bool NetworkAdmin::isActive()
{
//    assert(mNetwork);
//    int ret = virNetworkIsActive(mNetwork.get());
//    if (ret < 0) {
//        LOGE(mId << ": Failed to get network state\n"
//            << libvirt::libvirtFormatError());
//        throw ContainerOperationException();
//    }
//    return ret > 0;
    return false;
}


void NetworkAdmin::setDetachOnExit()
{
//    mDetachOnExit = true;
//    mNWFilter.setDetachOnExit();
}


} // namespace security_containers
