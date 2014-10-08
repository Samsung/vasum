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
 * @brief   Declaration of the class for administrating single network
 */


#ifndef SERVER_NETWORK_ADMIN_HPP
#define SERVER_NETWORK_ADMIN_HPP

#include "container-config.hpp"

//#include "libvirt/network-filter.hpp"
//#include "libvirt/network.hpp"


namespace security_containers {


class NetworkAdmin {

public:

    NetworkAdmin(const ContainerConfig& config);
    virtual ~NetworkAdmin();

    /**
     * Get the network id
     */
    const std::string& getId() const;

    /**
     * Start network.
     */
    void start();

    /**
     * Stop network.
     */
    void stop();

    /**
     * @return Is the network active?
     */
    bool isActive();

    /**
     * Set whether container should be detached on exit.
     */
    void setDetachOnExit();


private:
    const ContainerConfig& mConfig;
    //libvirt::LibvirtNWFilter mNWFilter;
    //libvirt::LibvirtNetwork mNetwork;
    const std::string mId;
    bool mDetachOnExit;
};


} // namespace security_containers


#endif // SERVER_NETWORK_ADMIN_HPP
