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

#include <assert.h>

#include <scs-container.hpp>
#include <scs-exception.hpp>
#include <scs-log.hpp>

using namespace security_containers;

Container::Container()
{
    connect();
}


Container::~Container()
{
    disconnect();
}


void
Container::connect()
{
    assert(mVir == NULL);

    mVir = virConnectOpen("lxc://");
    if (mVir == NULL) {
        LOGE("Failed to open connection to lxc://");
        throw ConnectionException();
    }
};


void
Container::disconnect()
{
    if (mVir == NULL) {
        return;
    }

    if (virConnectClose(mVir) < 0) {
        LOGE("Error during unconnecting from libvirt");
    };
    mVir = NULL;
};


void
Container::start()
{
    assert(mVir != NULL);
    assert(mDom != NULL);

    if (mIsRunning) {
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

    mIsRunning = true;
};


void
Container::stop()
{
    assert(mVir != NULL);
    assert(mDom != NULL);

    if (!mIsRunning) {
        return;
    }
    // Forceful termination of the guest
    u_int flags = VIR_DOMAIN_DESTROY_DEFAULT;

    if (virDomainDestroyFlags(mDom, flags) < 0) {
        LOGE("Error during domain stopping");
        throw DomainOperationException();
    }

    mIsRunning = false;
};


void
Container::define(const char* configXML)
{
    assert(mVir != NULL);

    if (configXML) {
        mDom = virDomainDefineXML(mVir, configXML);
    } else {
        mDom = virDomainDefineXML(mVir, mDefaultConfigXML.c_str());
    }

    if (mDom == NULL) {
        LOGE("Error during domain defining");
        throw DomainOperationException();
    }
};


void
Container::undefine()
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
};
