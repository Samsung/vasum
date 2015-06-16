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
#include "lxc/zone.hpp"
#include "lxc/exception.hpp"
#include "utils/exception.hpp"
#include "utils/execute.hpp"
#include "utils/fd-utils.hpp"
#ifdef USE_EXEC
#include "utils/c-array.hpp"
#endif

#include <lxc/lxccontainer.h>
#include <sys/stat.h>

#include <utils/initctl.hpp>
#include <sys/wait.h>

#include <map>
#include <sys/socket.h>
#include <fcntl.h>
#include <unistd.h>

namespace vasum {
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

int execFunction(void* data) {
    // Executed by C code, so catch all exceptions
    try {
        return (*static_cast<std::function<int()>*>(data))();
    } catch(...) {
        return -1; // Non-zero on failure
    }
    return 0; // Success
}

} // namespace

std::string LxcZone::toString(State state) {
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
    : mLxcContainer(nullptr)
{
    mLxcContainer = lxc_container_new(zoneName.c_str(), lxcPath.c_str());
    if (!mLxcContainer) {
        LOGE("Could not initialize lxc zone " << zoneName << " in path " << lxcPath);
        throw LxcException("Could not initialize lxc zone");
    }
}

LxcZone::~LxcZone()
{
    lxc_container_put(mLxcContainer);
}

std::string LxcZone::getName() const
{
    return mLxcContainer->name;
}

std::string LxcZone::getConfigItem(const std::string& key)
{
    char buffer[1024];
    int len = mLxcContainer->get_config_item(mLxcContainer, key.c_str(), buffer, sizeof(buffer));
    if (len < 0) {
        LOGE("Key '" << key << "' not found in zone " << getName());
        throw LxcException("Key not found");
    }
    return buffer;
}

bool LxcZone::isDefined()
{
    return mLxcContainer->is_defined(mLxcContainer);
}

LxcZone::State LxcZone::getState()
{
    const std::string str = mLxcContainer->state(mLxcContainer);
    return STATE_MAP.at(str);
}

bool LxcZone::create(const std::string& templatePath, const char* const* argv)
{
#ifdef USE_EXEC
    utils::CStringArrayBuilder args;
    args.add("lxc-create")
    .add("-n").add(mLxcContainer->name)
    .add("-t").add(templatePath.c_str())
    .add("-P").add(mLxcContainer->config_path);

    if (*argv) {
        args.add("--");
    }

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
    if (!mLxcContainer->create(mLxcContainer,
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
    if (!mLxcContainer->destroy(mLxcContainer)) {
        LOGE("Could not destroy zone " << getName());
        return false;
    }
    return true;
}

bool LxcZone::start(const char* const* argv)
{
#ifdef USE_EXEC
    if (mLxcContainer->is_running(mLxcContainer)) {
        LOGE("Already started " << getName());
        return false;
    }

    utils::CStringArrayBuilder args;
    args.add("lxc-start")
    .add("-d")
    .add("-n").add(mLxcContainer->name)
    .add("-P").add(mLxcContainer->config_path);

    if (*argv) {
        args.add("--");
    }

    while (*argv) {
        args.add(*argv++);
    }

    if (!utils::executeAndWait("/usr/bin/lxc-start", args.c_array())) {
        LOGE("Could not start zone " << getName());
        return false;
    }

    refresh();

    // we have to check status because lxc-start runs in daemonized mode
    if (!mLxcContainer->is_running(mLxcContainer)) {
        LOGE("Could not start init in zone " << getName());
        return false;
    }
    return true;
#else
    if (mLxcContainer->is_running(mLxcContainer)) {
        LOGE("Already started " << getName());
        return false;
    }
    if (!mLxcContainer->want_daemonize(mLxcContainer, true)) {
        LOGE("Could not configure zone " << getName());
        return false;
    }
    if (!mLxcContainer->start(mLxcContainer, false, const_cast<char* const*>(argv))) {
        LOGE("Could not start zone " << getName());
        return false;
    }
    return true;
#endif
}

bool LxcZone::stop()
{
    if (!mLxcContainer->stop(mLxcContainer)) {
        LOGE("Could not stop zone " << getName());
        return false;
    }
    return true;
}

bool LxcZone::reboot()
{
    if (!mLxcContainer->reboot(mLxcContainer)) {
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
    .add("-n").add(mLxcContainer->name)
    .add("-P").add(mLxcContainer->config_path)
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
    if (!mLxcContainer->shutdown(mLxcContainer, timeout)) {
        LOGE("Could not gracefully shutdown zone " << getName() << " in " << timeout << "s");
        return false;
    }
    return true;
#endif
}

bool LxcZone::freeze()
{
    if (!mLxcContainer->freeze(mLxcContainer)) {
        LOGE("Could not freeze zone " << getName());
        return false;
    }
    return true;
}

bool LxcZone::unfreeze()
{
    if (!mLxcContainer->unfreeze(mLxcContainer)) {
        LOGE("Could not unfreeze zone " << getName());
        return false;
    }
    return true;
}

bool LxcZone::waitForState(State state, int timeout)
{
    if (!mLxcContainer->wait(mLxcContainer, toString(state).c_str(), timeout)) {
        LOGD("Timeout while waiting for state " << toString(state) << " of zone " << getName());
        return false;
    }
    return true;
}

pid_t LxcZone::getInitPid() const
{
    return mLxcContainer->init_pid(mLxcContainer);
}

bool LxcZone::setRunLevel(int runLevel)
{
    Call call = [runLevel]() -> int {
        return utils::setRunLevel(static_cast<utils::RunLevel>(runLevel)) ? 0 : 1;
    };
    return runInZone(call);
}

void LxcZone::refresh()
{
    //TODO Consider make LxcZone state-less
    std::string zoneName = mLxcContainer->name;
    std::string lxcPath = mLxcContainer->config_path;
    lxc_container_put(mLxcContainer);
    mLxcContainer = lxc_container_new(zoneName.c_str(), lxcPath.c_str());
}

bool LxcZone::runInZone(Call& call)
{
    lxc_attach_options_t options = LXC_ATTACH_OPTIONS_DEFAULT;
    options.attach_flags = LXC_ATTACH_REMOUNT_PROC_SYS |
                           LXC_ATTACH_DROP_CAPABILITIES |
                           LXC_ATTACH_SET_PERSONALITY |
                           LXC_ATTACH_LSM_EXEC |
                           LXC_ATTACH_LSM_NOW |
                           LXC_ATTACH_MOVE_TO_CGROUP;

    pid_t pid;
    int ret = mLxcContainer->attach(mLxcContainer,
                                    execFunction,
                                    &call,
                                    &options,
                                    &pid);
    if (ret != 0) {
        return false;
    }
    int status;
    if (!utils::waitPid(pid, status)) {
        return false;
    }
    return status == 0;
}

bool LxcZone::createFile(const std::string& path,
                         const std::int32_t flags,
                         const std::int32_t mode,
                         int *fdPtr)
{
    *fdPtr = -1;

    int sockets[2];
    if (::socketpair(AF_LOCAL, SOCK_STREAM, 0, sockets) < 0) {
        LOGE("Can't create socket pair: " << utils::getSystemErrorMessage());
        return false;
    }

    lxc::LxcZone::Call call = [&]()->int{
        utils::close(sockets[1]);

        int fd = ::open(path.c_str(), O_CREAT | O_EXCL | flags, mode);
        if (fd < 0) {
            LOGE("Error during file creation: " << utils::getSystemErrorMessage());
            utils::close(sockets[0]);
            return -1;
        }
        LOGT("Created file in zone with fd " << fd);
        utils::fdSend(sockets[0], fd);
        utils::close(fd);
        return 0;
    };

    runInZone(call);

    utils::close(sockets[0]);
    *fdPtr = utils::fdRecv(sockets[1]);
    utils::close(sockets[1]);

    return true;
}

} // namespace lxc
} // namespace vasum
