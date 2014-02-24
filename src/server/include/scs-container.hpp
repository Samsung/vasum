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
 * @file    scs-container.hpp
 * @author  Jan Olszak (j.olszak@samsung.com)
 * @brief   Declaration of the class for managing one container
 */


#ifndef SECURITY_CONTAINERS_SERVER_CONTAINER_HPP
#define SECURITY_CONTAINERS_SERVER_CONTAINER_HPP

#include "scs-container-config.hpp"
#include <string>
#include <libvirt/libvirt.h>

namespace security_containers {

class Container {

public:
    Container();
    virtual ~Container();
    void define(const char* configXML = NULL);
    void undefine();
    void start();
    void stop();

private:
    ContainerConfig mConfig;   // container configuration

    virConnectPtr mVir = NULL; // pointer to the connection with libvirt
    virDomainPtr  mDom = NULL; // pointer to the domain

    bool mIsRunning = false; // is the domain now running

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
    void connect();
    void disconnect();

};
}
#endif // SECURITY_CONTAINERS_SERVER_CONTAINER_HPP
