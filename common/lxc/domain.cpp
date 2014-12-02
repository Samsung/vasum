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
 * @brief   Lxc zone
 */

// Define macro USE_EXEC if you are goind to use valgrind
// TODO Consider always using exec to control lxc. Advantages:
//      - 'ps' output is more descriptive with lxc-start
//      - lxc library is not tested well with multithread programs
//        (there are still some issues like using umask etc.)
//      - it could be possible not to be uid 0 in this process
//      - fork + unshare does not work with traced process (e.g. under valgrind)

#include "config.hpp"
#include "logger/logger.hpp"
#include "lxc/domain.hpp"
#include "lxc/exception.hpp"
#ifdef USE_EXEC
#include "utils/execute.hpp"
#include "utils/c-array.hpp"
#endif

#include <lxc/lxccontainer.h>
#include <sys/stat.h>

#include <utils/initctl.hpp>
#include <sys/wait.h>

#include <map>

namespace security_containers {
namespace lxc {

namespace {
#define ITEM(X) {#X, LxcZone::State::X},
    const std::map<std::string, LxcZone::State> STATE_MAP = {
        ITEM(STOPPED)
        ITEM(STARTING)
        ITEM(RUNNING)
        ITEM(STOPPING)
        ITEM(ABORTING)
        ITEM(FREEZING)
        ITEM(FROZEN)
        ITEM(THAWED)
    };
#undef ITEM
} // namespace

std::string LxcZone::toString(State state)
{
#define CASE(X) case LxcZone::State::X: return #X;
    switch (state) {
    CASE(STOPPED)
    CASE(STARTING)
    CASE(RUNNING)
    CASE(STOPPING)
    CASE(ABORTING)
    CASE(FREEZING)
    CASE(FROZEN)
    CASE(THAWED)
    }
#undef CASE
    throw LxcException("Invalid state");
}

LxcZone::LxcZone(const std::string& lxcPath, const std::string& zoneName)
  : mContainer(nullptr)
{
    mContainer = lxc_container_new(zoneName.c_str(), lxcPath.c_str());
    if (!mContainer) {
        LOGE("Could not initialize lxc zone " << zoneName << " in path " << lxcPath);
        throw LxcException("Could not initialize lxc zone");
    }
}

LxcZone::~LxcZone()
{
    lxc_container_put(mContainer);
}

std::string LxcZone::getName() const
{
    return mContainer->name;
}

std::string LxcZone::getConfigItem(const std::string& key)
{
    char buffer[1024];
    int len = mContainer->get_config_item(mContainer, key.c_str(), buffer, sizeof(buffer));
    if (len < 0) {
        LOGE("Key '" << key << "' not found in zone " << getName());
        throw LxcException("Key not found");
    }
    return buffer;
}

bool LxcZone::isDefined()
{
    return mContainer->is_defined(mContainer);
}

LxcZone::State LxcZone::getState()
{
    const std::string str = mContainer->state(mContainer);
    return STATE_MAP.at(str);
}

bool LxcZone::create(const std::string& templatePath, const char* const* argv)
{
#ifdef USE_EXEC
    utils::CStringArrayBuilder args;
    args.add("lxc-create")
        .add("-n").add(mContainer->name)
        .add("-t").add(templatePath.c_str())
        .add("-P").add(mContainer->config_path);

    while (*argv) {
        args.add(*argv++);
    }

    if (!utils::executeAndWait("/usr/bin/lxc-create", args.c_array())) {
        LOGE("Could not create zone " << getName());
        return false;
    }

    refresh();
    return true;
#else
    if (!mContainer->create(mContainer,
                            templatePath.c_str(),
                            NULL, NULL, 0,
                            const_cast<char* const*>(argv))) {
        LOGE("Could not create zone " << getName());
        return false;
    }
    return true;
#endif
}

bool LxcZone::destroy()
{
    if (!mContainer->destroy(mContainer)) {
        LOGE("Could not destroy zone " << getName());
        return false;
    }
    return true;
}

bool LxcZone::start(const char* const* argv)
{
#ifdef USE_EXEC
    if (mContainer->is_running(mContainer)) {
        LOGE("Already started " << getName());
        return false;
    }

    utils::CStringArrayBuilder args;
    args.add("lxc-start")
        .add("-d")
        .add("-n").add(mContainer->name)
        .add("-P").add(mContainer->config_path)
        .add("--");

    while (*argv) {
        args.add(*argv++);
    }

    if (!utils::executeAndWait("/usr/bin/lxc-start", args.c_array())) {
        LOGE("Could not start zone " << getName());
        return false;
    }

    refresh();

    // we have to check status because lxc-start runs in daemonized mode
    if (!mContainer->is_running(mContainer)) {
        LOGE("Could not start init in zone " << getName());
        return false;
    }
    return true;
#else
    if (mContainer->is_running(mContainer)) {
        LOGE("Already started " << getName());
        return false;
    }
    if (!mContainer->want_daemonize(mContainer, true)) {
        LOGE("Could not configure zone " << getName());
        return false;
    }
    if (!mContainer->start(mContainer, false, const_cast<char* const*>(argv))) {
        LOGE("Could not start zone " << getName());
        return false;
    }
    return true;
#endif
}

bool LxcZone::stop()
{
    if (!mContainer->stop(mContainer)) {
        LOGE("Could not stop zone " << getName());
        return false;
    }
    return true;
}

bool LxcZone::reboot()
{
    if (!mContainer->reboot(mContainer)) {
        LOGE("Could not reboot zone " << getName());
        return false;
    }
    return true;
}

bool LxcZone::shutdown(int timeout)
{
    State state = getState();
    if (state == State::STOPPED) {
        return true;
    }
    if (state != State::RUNNING) {
        LOGE("Could not gracefully shutdown zone " << getName());
        return false;
    }

#ifdef USE_EXEC
    utils::CStringArrayBuilder args;
    std::string timeoutStr = std::to_string(timeout);
    args.add("lxc-stop")
        .add("-n").add(mContainer->name)
        .add("-P").add(mContainer->config_path)
        .add("-t").add(timeoutStr.c_str())
        .add("--nokill");

    if (!utils::executeAndWait("/usr/bin/lxc-stop", args.c_array())) {
        LOGE("Could not gracefully shutdown zone " << getName() << " in " << timeout << "s");
        return false;
    }

    refresh();
    return true;
#else
    // try shutdown by sending poweroff to init
    if (setRunLevel(utils::RUNLEVEL_POWEROFF)) {
        if (!waitForState(State::STOPPED, timeout)) {
            LOGE("Could not gracefully shutdown zone " << getName() << " in " << timeout << "s");
            return false;
        }
        return true;
    }
    LOGW("SetRunLevel failed for zone " + getName());

    // fallback for other inits like bash: lxc sends 'lxc.haltsignal' signal to init
    if (!mContainer->shutdown(mContainer, timeout)) {
        LOGE("Could not gracefully shutdown zone " << getName() << " in " << timeout << "s");
        return false;
    }
    return true;
#endif
}

bool LxcZone::freeze()
{
    if (!mContainer->freeze(mContainer)) {
        LOGE("Could not freeze zone " << getName());
        return false;
    }
    return true;
}

bool LxcZone::unfreeze()
{
    if (!mContainer->unfreeze(mContainer)) {
        LOGE("Could not unfreeze zone " << getName());
        return false;
    }
    return true;
}

bool LxcZone::waitForState(State state, int timeout)
{
    if (!mContainer->wait(mContainer, toString(state).c_str(), timeout)) {
        LOGD("Timeout while waiting for state " << toString(state) << " of zone " << getName());
        return false;
    }
    return true;
}

bool LxcZone::setRunLevel(int runLevel)
{
    auto callback = [](void* param) -> int {
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

void LxcZone::refresh()
{
    //TODO Consider make LxcZone state-less
    std::string zoneName = mContainer->name;
    std::string lxcPath = mContainer->config_path;
    lxc_container_put(mContainer);
    mContainer = lxc_container_new(zoneName.c_str(), lxcPath.c_str());
}


} // namespace lxc
} // namespace security_containers
