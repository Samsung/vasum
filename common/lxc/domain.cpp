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

#include <utils/initctl.hpp>
#include <sys/wait.h>

#include <map>

namespace security_containers {
namespace lxc {

namespace {
    const std::map<std::string, LxcDomain::State> STATE_MAP = {
        {"STOPPED", LxcDomain::State::STOPPED},
        {"STARTING", LxcDomain::State::STARTING},
        {"RUNNING", LxcDomain::State::RUNNING},
        {"STOPPING", LxcDomain::State::STOPPING},
        {"ABORTING", LxcDomain::State::ABORTING},
        {"FREEZING", LxcDomain::State::FREEZING},
        {"FROZEN", LxcDomain::State::FROZEN},
        {"THAWED", LxcDomain::State::THAWED}
    };
} // namespace

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

LxcDomain::State LxcDomain::getState()
{
    const std::string str = mContainer->state(mContainer);
    return STATE_MAP.at(str);
}

bool LxcDomain::create(const std::string& templatePath)
{
    if (!mContainer->create(mContainer, templatePath.c_str(), NULL, NULL, 0, NULL)) {
        LOGE("Could not create domain " + getName());
        return false;
    }
    return true;
}

bool LxcDomain::destroy()
{
    if (!mContainer->destroy(mContainer)) {
        LOGE("Could not destroy domain " + getName());
        return false;
    }
    return true;
}

bool LxcDomain::start(const char* const* argv)
{
    if (mContainer->is_running(mContainer)) {
        LOGE("Already started " + getName());
        return false;
    }
    if (!mContainer->want_daemonize(mContainer, true)) {
        LOGE("Could not configure domain " + getName());
        return false;
    }
    if (!mContainer->start(mContainer, false, const_cast<char* const*>(argv))) {
        LOGE("Could not start domain " + getName());
        return false;
    }
    return true;
}

bool LxcDomain::stop()
{
    if (!mContainer->stop(mContainer)) {
        LOGE("Could not stop domain " + getName());
        return false;
    }
    return true;
}

bool LxcDomain::reboot()
{
    if (!mContainer->reboot(mContainer)) {
        LOGE("Could not reboot domain " + getName());
        return false;
    }
    return true;
}

bool LxcDomain::shutdown(int timeout)
{
    State state = getState();
    if (state == State::STOPPED) {
        return true;
    }
    if (state != State::RUNNING) {
        LOGE("Could not gracefully shutdown domain " << getName());
        return false;
    }

    // try shutdown by sending poweroff to init
    if (setRunLevel(utils::RUNLEVEL_POWEROFF)) {
        if (!mContainer->wait(mContainer, "STOPPED", timeout)) {
            LOGE("Could not gracefully shutdown domain " + getName() + " in " << timeout << "s");
            return false;
        }
        return true;
    }
    LOGW("SetRunLevel failed for domain " + getName());

    // fallback for other inits like bash: lxc sends 'lxc.haltsignal' signal to init
    if (!mContainer->shutdown(mContainer, timeout)) {
        LOGE("Could not gracefully shutdown domain " + getName() + " in " << timeout << "s");
        return false;
    }
    return true;
}

bool LxcDomain::freeze()
{
    if (!mContainer->freeze(mContainer)) {
        LOGE("Could not freeze domain " + getName());
        return false;
    }
    return true;
}

bool LxcDomain::unfreeze()
{
    if (!mContainer->unfreeze(mContainer)) {
        LOGE("Could not unfreeze domain " + getName());
        return false;
    }
    return true;
}

bool LxcDomain::setRunLevel(int runLevel)
{
    auto callback = [](void* param) {
        utils::RunLevel runLevel = *reinterpret_cast<utils::RunLevel*>(param);
        return utils::setRunLevel(runLevel) ? 0 : 1;
    };

    lxc_attach_options_t options = LXC_ATTACH_OPTIONS_DEFAULT;
    pid_t pid;
    int ret = mContainer->attach(mContainer, callback, &runLevel, &options, &pid);
    if (ret != 0) {
        return false;
    }
    int status;
    if (waitpid(pid, &status, 0) < 0) {
        return false;
    }
    return status == 0;
}


} // namespace lxc
} // namespace security_containers
