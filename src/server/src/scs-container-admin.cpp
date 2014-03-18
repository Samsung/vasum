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
 * @file    scs-container.cpp
 * @author  Jan Olszak (j.olszak@samsung.com)
 * @brief   Implementation of class for managing one container
 */

#include "scs-container-admin.hpp"
#include "scs-exception.hpp"
#include "scs-log.hpp"

#include <assert.h>
#include <string>

namespace security_containers {

Container::Container(const std::string& configXML)
{
    connect();
    define(configXML);
}


Container::~Container()
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


void Container::connect()
{
    assert(mVir == NULL);

    mVir = virConnectOpen("lxc://");
    if (mVir == NULL) {
        LOGE("Failed to open connection to lxc://");
        throw ConnectionException();
    }
}


void Container::disconnect()
{
    if (mVir == NULL) {
        return;
    }

    if (virConnectClose(mVir) < 0) {
        LOGE("Error during unconnecting from libvirt");
    };
    mVir = NULL;
}


void Container::start()
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


void Container::stop()
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


void Container::shutdown()
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


bool Container::isRunning()
{
    return getState() == VIR_DOMAIN_RUNNING;
}


bool Container::isStopped()
{
    int state = getState();
    return state == VIR_DOMAIN_SHUTDOWN ||
           state == VIR_DOMAIN_SHUTOFF ||
           state == VIR_DOMAIN_CRASHED;
}

void Container::define(const std::string& configXML)
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


void Container::undefine()
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


void Container::suspend()
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


void Container::resume()
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


bool Container::isPaused()
{
    return getState() == VIR_DOMAIN_PAUSED;
}


bool Container::isPMSuspended()
{
    return getState() == VIR_DOMAIN_PMSUSPENDED;
}


int Container::getState()
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
