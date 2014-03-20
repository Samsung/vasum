/*
 *  Copyright (c) 2000 - 2014 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *  Contact: Bumjin Im <bj.im@samsung.com>
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
 * @file    scs-container-admin.hpp
 * @author  Jan Olszak (j.olszak@samsung.com)
 * @brief   Declaration of the class for administrating one container
 */


#ifndef SECURITY_CONTAINERS_SERVER_CONTAINER_ADMIN_HPP
#define SECURITY_CONTAINERS_SERVER_CONTAINER_ADMIN_HPP

#include <string>
#include <libvirt/libvirt.h>

namespace security_containers {

class ContainerAdmin {

public:
    ContainerAdmin(const std::string& libvirtConfigPath);
    virtual ~ContainerAdmin();

    /**
     * Get the container id
     */
    std::string getId();

    /**
     * Boot the container
     */
    void start();

    /**
     * Forcefully stop the container.
     */
    void stop();

    /**
     * Gracefully shutdown the domain.
     * This method will NOT block untill domain is shut down,
     * because some configurations may ignore this.
     */
    void shutdown();

    /**
     * @return Is the process started?
     */
    bool isRunning();

    /**
     * Check if the container is stopped. It's NOT equivalent to !isRunning,
     * because it checks different internal libvirt's states. There are other states,
     * (e.g. paused) when the container isn't runnig nor stopped.
     *
     * @return Is the process stopped?
     */
    bool isStopped();

    /**
     * Suspends an active domain, the process is frozen
     * without further access to CPU resources and I/O,
     * but the memory used by the domain
     * at the hypervisor level will stay allocated
     */
    void suspend();

    /**
     * Resume the container after suspension.
     */
    void resume();

    /**
     * @return Is the container in a paused state?
     */
    bool isPaused();

private:
    // TODO: secure those pointers from exceptions (e.g. in constructor)
    virConnectPtr mVir = NULL; // pointer to the connection with libvirt
    virDomainPtr  mDom = NULL; // pointer to the domain

    // TODO: This is a temporary sollution.
    const std::string mDefaultConfigXML = "<domain type=\"lxc\">\
                                         <name>cnsl</name>\
                                         <memory>102400</memory>\
                                         <os>\
                                         <type>exe</type>\
                                         <init>/bin/sh</init>\
                                         </os>\
                                         <devices>\
                                         <console type=\"pty\"/>\
                                         </devices>\
                                         </domain>";

    void connect();    // fill mVir
    void disconnect(); // release mVir
    void define(const std::string& configXML); // containers can't share the same libvirt configuration
    void undefine();

    int  getState();   // get the libvirt's domain state
    bool isPMSuspended(); // the cotainer suspended by the power manager
};
}
#endif // SECURITY_CONTAINERS_SERVER_CONTAINER_ADMIN_HPP
