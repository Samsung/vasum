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
 * @brief   Lxc domain
 */

#include "config.hpp"
#include "logger/logger.hpp"
#include "lxc/domain.hpp"
#include "lxc/exception.hpp"

#include <lxc/lxccontainer.h>
#include <sys/stat.h>

namespace security_containers {
namespace lxc {


LxcDomain::LxcDomain(const std::string& lxcPath, const std::string& domainName)
  : mContainer(nullptr)
{
    mContainer = lxc_container_new(domainName.c_str(), lxcPath.c_str());
    if (!mContainer) {
        LOGE("Could not initialize lxc domain " << domainName << " in path " << lxcPath);
        throw LxcException("Could not initialize lxc domain");
    }
}

LxcDomain::~LxcDomain()
{
    lxc_container_put(mContainer);
}

std::string LxcDomain::getName() const
{
    return mContainer->name;
}

std::string LxcDomain::getConfigItem(const std::string& key)
{
    char buffer[1024];
    int len = mContainer->get_config_item(mContainer, key.c_str(), buffer, sizeof(buffer));
    if (len < 0) {
        LOGE("Key '" + key + "' not found in domain " + getName());
        throw LxcException("Key not found");
    }
    return buffer;
}

bool LxcDomain::isDefined()
{
    return mContainer->is_defined(mContainer);
}

bool LxcDomain::isRunning()
{
    return mContainer->is_running(mContainer);
}

std::string LxcDomain::getState()
{
    return mContainer->state(mContainer);
}

void LxcDomain::create(const std::string& templatePath)
{
    if (!mContainer->create(mContainer, templatePath.c_str(), NULL, NULL, 0, NULL)) {
        LOGE("Could not create domain " + getName());
        throw LxcException("Could not create domain");
    }
}

void LxcDomain::destroy()
{
    if (!mContainer->destroy(mContainer)) {
        LOGE("Could not destroy domain " + getName());
        throw LxcException("Could not destroy domain");
    }
}

void LxcDomain::start(const char* argv[])
{
    if (!mContainer->start(mContainer, false, const_cast<char**>(argv))) {
        LOGE("Could not start domain " + getName());
        throw LxcException("Could not start domain");
    }
}

void LxcDomain::stop()
{
    if (!mContainer->stop(mContainer)) {
        LOGE("Could not stop domain " + getName());
        throw LxcException("Stop domain failed");
    }
}

void LxcDomain::reboot()
{
    if (!mContainer->reboot(mContainer)) {
        LOGE("Could not reboot domain " + getName());
        throw LxcException("Reboot domain failed");
    }
}

void LxcDomain::shutdown(int timeout)
{
    if (!mContainer->shutdown(mContainer, timeout)) {
        LOGE("Could not gracefully shutdown domain " + getName() + " in " << timeout << "s");
        throw LxcException("Shutdown domain failed");
    }
}

} // namespace lxc
} // namespace security_containers
