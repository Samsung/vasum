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
 * @brief   Implementation of the class wrapping libvirt network
 */

#include "config.hpp"

#include "log/logger.hpp"
#include "libvirt/network.hpp"
#include "libvirt/helpers.hpp"
#include "libvirt/exception.hpp"


namespace security_containers {
namespace libvirt {


LibvirtNetwork::LibvirtNetwork(const std::string& configXML)
    : mCon(LIBVIRT_LXC_ADDRESS), mNet(nullptr)
{
    mNet = virNetworkDefineXML(mCon.get(), configXML.c_str());

    if (mNet == nullptr) {
        LOGE("Error while defining a network:\n"
             << libvirtFormatError());
        throw LibvirtOperationException();
    }
}

LibvirtNetwork::~LibvirtNetwork()
{
    if (virNetworkUndefine(mNet) < 0) {
        LOGE("Error while undefining the network:\n"
             << libvirtFormatError());
    }

    if (virNetworkFree(mNet) < 0) {
        LOGE("Error while destroying the network object:\n"
             << libvirtFormatError());
    }
}

virNetworkPtr LibvirtNetwork::get()
{
    return mNet;
}

LibvirtNetwork::operator bool() const
{
    return mNet != nullptr;
}

} // namespace libvirt
} // namespace security_containers
