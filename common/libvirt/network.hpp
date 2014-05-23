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
 * @brief   Declaration of the class wrapping libvirt network
 */

#ifndef COMMON_LIBVIRT_NETWORK_HPP
#define COMMON_LIBVIRT_NETWORK_HPP

#include "libvirt/connection.hpp"

#include <libvirt/libvirt.h>


namespace security_containers {
namespace libvirt {


class LibvirtNetwork {

public:
    LibvirtNetwork(const std::string& configXML);
    ~LibvirtNetwork();

    /**
     * @return The libvirt network pointer
     */
    virNetworkPtr get();

    /**
     * @return libvirt network pointer is not NULL
     */
    operator bool() const;

private:
    LibvirtConnection mCon;
    virNetworkPtr mNet;
};


} // namespace libvirt
} // namespace security_containers


#endif // COMMON_LIBVIRT_NETWORK_HPP
