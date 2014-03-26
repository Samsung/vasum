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
 * @file    scs-container-admin.cpp
 * @author  Jan Olszak (j.olszak@samsung.com)
 * @brief   Implementation of class for administrating one container
 */

#include "scs-container-admin.hpp"
#include "scs-exception.hpp"
#include "log.hpp"

#include <assert.h>
#include <string>
#include <fstream>
#include <streambuf>

namespace security_containers {

ContainerAdmin::ContainerAdmin(const std::string& libvirtConfigPath)
{
    connect();
    std::ifstream t(libvirtConfigPath);
    if (!t.is_open()) {
        LOGE("libvirt config file is missing");
        throw ConfigException();
    }
    std::string libvirtConfig((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());
    define(libvirtConfig);
}


ContainerAdmin::~ContainerAdmin()
{
    // Try to shutdown
    try {
        resume();
        shutdown();
    } catch (ServerException& e) {}

    // Destroy the container
    try {
        stop();
        undefine();
    } catch (ServerException& e) {
        LOGE("Failed to destroy the container!");
    }
    disconnect();
}


void ContainerAdmin::connect()
{
    assert(mVir == NULL);

    mVir = virConnectOpen("lxc://");
    if (mVir == NULL) {
        LOGE("Failed to open connection to lxc://");
        throw ConnectionException();
    }
}


void ContainerAdmin::disconnect()
{
    if (mVir == NULL) {
        return;
    }

    if (virConnectClose(mVir) < 0) {
        LOGE("Error during unconnecting from libvirt");
    };
    mVir = NULL;
}


std::string ContainerAdmin::getId()
{
    assert(mVir != NULL);
    assert(mDom != NULL);

    const char* id;
    if ((id = virDomainGetName(mDom)) == NULL) {
        LOGE("Failed to get container's id");
        throw DomainOperationException();
    }

    return id;
}


void ContainerAdmin::start()
{
    assert(mVir != NULL);
    assert(mDom != NULL);

    if (isRunning()) {
        return;
    }

    // Autodestroyed when connection pointer released
    // Any managed save file for this domain is discarded,
    // and the domain boots from scratch
    u_int flags = VIR_DOMAIN_START_AUTODESTROY;

    if (virDomainCreateWithFlags(mDom, flags) < 0) {
        LOGE("Failed to start the container");
        throw DomainOperationException();
    }
}


void ContainerAdmin::stop()
{
    assert(mVir != NULL);
    assert(mDom != NULL);

    if (!isRunning()) {
        return;
    }

    // Forceful termination of the guest
    u_int flags = VIR_DOMAIN_DESTROY_DEFAULT;

    if (virDomainDestroyFlags(mDom, flags) < 0) {
        LOGE("Error during domain stopping");
        throw DomainOperationException();
    }
}


void ContainerAdmin::shutdown()
{
    assert(mVir != NULL);
    assert(mDom != NULL);

    if (!isRunning()) {
        return;
    }

    if (virDomainShutdown(mDom) < 0) {
        LOGE("Error during domain shutdown");
        throw DomainOperationException();
    }
}


bool ContainerAdmin::isRunning()
{
    return getState() == VIR_DOMAIN_RUNNING;
}


bool ContainerAdmin::isStopped()
{
    int state = getState();
    return state == VIR_DOMAIN_SHUTDOWN ||
           state == VIR_DOMAIN_SHUTOFF ||
           state == VIR_DOMAIN_CRASHED;
}

void ContainerAdmin::define(const std::string& configXML)
{
    assert(mVir != NULL);

    if (!configXML.empty()) {
        mDom = virDomainDefineXML(mVir, configXML.c_str());
    } else {
        mDom = virDomainDefineXML(mVir, mDefaultConfigXML.c_str());
    }

    if (mDom == NULL) {
        LOGE("Error during domain defining");
        throw DomainOperationException();
    }
}


void ContainerAdmin::undefine()
{
    assert(mVir != NULL);
    assert(mDom != NULL);

    stop();

    // Remove domain configuration
    if (virDomainUndefine(mDom) < 0) {
        LOGE("Error during domain undefine");
        throw DomainOperationException();
    }

    if (virDomainFree(mDom) < 0) {
        LOGE("Error during domain destruction");
        throw DomainOperationException();
    }

    mDom = NULL;
}


void ContainerAdmin::suspend()
{
    assert(mVir != NULL);
    assert(mDom != NULL);

    if (isPaused()) {
        return;
    }

    if (isPMSuspended() || virDomainSuspend(mDom) < 0) {
        LOGE("Error during domain suspension");
        throw DomainOperationException();
    }
}


void ContainerAdmin::resume()
{
    assert(mVir != NULL);
    assert(mDom != NULL);

    if (!isPaused()) {
        return;
    }

    if (isPMSuspended() || virDomainResume(mDom) < 0) {
        LOGE("Error during domain resumming");
        throw DomainOperationException();
    }
}


bool ContainerAdmin::isPaused()
{
    return getState() == VIR_DOMAIN_PAUSED;
}


bool ContainerAdmin::isPMSuspended()
{
    return getState() == VIR_DOMAIN_PMSUSPENDED;
}


int ContainerAdmin::getState()
{
    assert(mVir != NULL);
    assert(mDom != NULL);

    int state;

    if (virDomainGetState(mDom, &state, NULL, 0)) {
        LOGE("Error during getting domain's state");
        throw DomainOperationException();
    }

    return state;
}

} // namespace security_containers
