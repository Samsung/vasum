/*
 *  Copyright (c) 2014 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *  Contact: Michal Witanowski <m.witanowski@samsung.com>
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
 * @author  Michal Witanowski (m.witanowski@samsung.com)
 * @brief   Implementaion of environment setup routines that require root privileges
 */

#include "config.hpp"

#include "utils/environment.hpp"
#include "logger/logger.hpp"

#include <cap-ng.h>
#include <grp.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <cstring>


namespace security_containers {
namespace utils {


bool setSuppGroups(const std::vector<std::string>& groups)
{
    std::vector<gid_t> gids;

    for (const std::string& group : groups) {
        // get GID from name
        struct group* grp = ::getgrnam(group.c_str());
        if (grp == NULL) {
            LOGE("getgrnam failed to find group '" << group << "'");
            return false;
        }

        LOGD("'" << group << "' group ID: " << grp->gr_gid);
        gids.push_back(grp->gr_gid);
    }

    if (::setgroups(gids.size(), gids.data()) != 0) {
        LOGE("setgroups() failed: " << strerror(errno));
        return false;
    }

    return true;
}

bool dropRoot(uid_t uid, gid_t gid, const std::vector<unsigned int>& caps)
{
    ::capng_clear(CAPNG_SELECT_BOTH);

    for (const auto cap : caps) {
        if (::capng_update(CAPNG_ADD, static_cast<capng_type_t>(CAPNG_EFFECTIVE |
                                                                CAPNG_PERMITTED |
                                                                CAPNG_INHERITABLE), cap)) {
            LOGE("Failed to set capability: " << ::capng_capability_to_name(cap));
            return false;
        }
    }

    if (::capng_change_id(uid, gid, static_cast<capng_flags_t>(CAPNG_CLEAR_BOUNDING))) {
        LOGE("Failed to change process user");
        return false;
    }

    return true;
}

bool launchAsRoot(const std::function<bool()>& func)
{
    // TODO optimize if getuid() == 0
    pid_t pid = fork();
    if (pid < 0) {
        LOGE("Fork failed: " << strerror(errno));
        return false;
    }

    if (pid == 0) {
        if (::setuid(0) < 0) {
            LOGW("Failed to become root: " << strerror(errno));
            ::exit(EXIT_FAILURE);
        }

        try {
            if (!func()) {
                LOGE("Failed to successfully execute func");
                ::exit(EXIT_FAILURE);
            }
        } catch (const std::exception& e) {
            LOGE("Failed to successfully execute func: " << e.what());
            ::exit(EXIT_FAILURE);
        }

        ::exit(EXIT_SUCCESS);
    }

    int result;
    if (::waitpid(pid, &result, 0) < 0) {
        LOGE("waitpid failed: " << strerror(errno));
        return false;
    }
    if (result != 0) {
        LOGE("Function launched as root failed with result " << result);
        return false;
    }

    return true;
}


} // namespace utils
} // namespace security_containers
