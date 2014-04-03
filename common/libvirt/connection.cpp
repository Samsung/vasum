/*
 *  Copyright (c) 2014 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *  Contact: Lukasz Pawelczyk <l.pawelczyk@partner.samsung.com>
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
 * @author  Lukasz Pawelczyk (l.pawelczyk@partner.samsung.com)
 * @brief   Implementation of the class wrapping connection to libvirtd
 */

#include "log/logger.hpp"
#include "libvirt/connection.hpp"
#include "libvirt/exception.hpp"


namespace security_containers {
namespace libvirt {


LibvirtConnection::LibvirtConnection(const std::string& uri)
{
    mCon = virConnectOpen(uri.c_str());

    if (mCon == NULL) {
        LOGE("Failed to open connection to libvirtd");
        throw LibvirtOperationException();
    }
}

LibvirtConnection::~LibvirtConnection()
{
    if (virConnectClose(mCon) < 0) {
        LOGE("Error while disconnecting from libvirt");
    };
}

virConnectPtr LibvirtConnection::get()
{
    return mCon;
}


} // namespace libvirt
} // namespace security_containers
