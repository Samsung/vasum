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

#include "logger/logger.hpp"
#include "libvirt/network-filter.hpp"
#include "libvirt/helpers.hpp"
#include "libvirt/exception.hpp"


namespace security_containers {
namespace libvirt {

LibvirtNWFilter::LibvirtNWFilter(const std::string& configXML)
    : mCon(LIBVIRT_LXC_ADDRESS), mNetFilter(nullptr),
      mDetachOnExit(false)
{
    mNetFilter = virNWFilterDefineXML(mCon.get(), configXML.c_str());

    if (mNetFilter == nullptr) {
        LOGE("Error while definig a network filter:\n"
             << libvirtFormatError());
        throw LibvirtOperationException();
    }
}

LibvirtNWFilter::~LibvirtNWFilter()
{
    if (!mDetachOnExit)
    {
        if (virNWFilterUndefine(mNetFilter) < 0) {
            LOGE("Error while undefining the network filter:\n"
                 << libvirtFormatError());
        }
    }

    if (virNWFilterFree(mNetFilter) < 0) {
        LOGE("Error while destroying the network filter object:\n"
             << libvirtFormatError());
    }
}

void LibvirtNWFilter::setDetachOnExit()
{
    mDetachOnExit = true;
}

virNWFilterPtr LibvirtNWFilter::get()
{
    return mNetFilter;
}

LibvirtNWFilter::operator bool() const
{
    return mNetFilter != nullptr;
}

} // namespace libvirt
} // namespace security_containers
