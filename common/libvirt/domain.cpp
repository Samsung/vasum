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
 * @brief   Implementation of the class wrapping libvirt domain
 */

#include "log/logger.hpp"
#include "libvirt/domain.hpp"
#include "libvirt/exception.hpp"

#include <cassert>


namespace security_containers {
namespace libvirt {


LibvirtDomain::LibvirtDomain(const std::string& configXML)
    : mCon(LIBVIRT_LXC_ADDRESS)
{
    mDom = virDomainDefineXML(mCon.get(), configXML.c_str());

    if (mDom == NULL) {
        LOGE("Error during domain defining");
        throw LibvirtOperationException();
    }
}

LibvirtDomain::~LibvirtDomain()
{
    if (virDomainUndefine(mDom) < 0) {
        LOGE("Error during domain undefine");
    }

    if (virDomainFree(mDom) < 0) {
        LOGE("Error during domain destruction");
    }
}

virDomainPtr LibvirtDomain::get()
{
    return mDom;
}


} // namespace libvirt
} // namespace security_containers
