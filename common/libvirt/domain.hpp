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
 * @brief   Declaration of the class wrapping libvirt domain
 */

#ifndef COMMON_LIBVIRT_DOMAIN_HPP
#define COMMON_LIBVIRT_DOMAIN_HPP

#include "libvirt/connection.hpp"

#include <libvirt/libvirt.h>


namespace security_containers {
namespace libvirt {


/**
 * A class wrapping libvirtd domain
 */
class LibvirtDomain {

public:
    LibvirtDomain(const std::string& configXML);
    ~LibvirtDomain();

    /**
     * @return The libvirt domain pointer
     */
    virDomainPtr get();

private:
    LibvirtConnection mCon;
    virDomainPtr mDom = NULL;
};


} // namespace libvirt
} // namespace security_containers


#endif // COMMON_LIBVIRT_DOMAIN_HPP
