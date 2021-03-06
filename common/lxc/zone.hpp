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

#ifndef COMMON_LXC_ZONE_HPP
#define COMMON_LXC_ZONE_HPP

#include <string>
#include <vector>
#include <functional>
#include <sys/types.h>

// fwd declaration of lxc internals
struct lxc_container;

namespace vasum {
namespace lxc {


/**
 * A class wrapping lxc container
 */
class LxcZone {
public:
    typedef std::function<int()> Call;

    enum class State {
        STOPPED,
        STARTING,
        RUNNING,
        STOPPING,
        ABORTING,
        FREEZING,
        FROZEN,
        THAWED
    };

    /**
     * LxcZone constructor
     * @param lxcPath path where zones lives
     * @param zoneName name of zone
     */
    LxcZone(const std::string& lxcPath, const std::string& zoneName);
    ~LxcZone();

    LxcZone(const LxcZone&) = delete;
    LxcZone& operator=(const LxcZone&) = delete;

    /**
     * Get zone name
     */
    std::string getName() const;

    /**
     * Get item from lxc config file
     * @throw LxcException if key not found
     */
    std::string getConfigItem(const std::string& key);

    /**
     * Is zone defined (created)?
     */
    bool isDefined();

    /**
     * String representation of state
     */
    static std::string toString(State state);

    /**
     * Get zone state
     */
    State getState();

    /**
     * Wait till zone is in specified state
     * @return false on timeout
     */
    bool waitForState(State state, int timeout);

    /**
     * Create zone
     * @param templatePath template from which zone will be created
     * @param argv additional template arguments
     */
    bool create(const std::string& templatePath, const char* const* argv);

    /**
     * Destroy zone
     */
    bool destroy();

    /**
     * Start zone
     * @param argv init process with arguments
     */
    bool start(const char* const* argv);

    /**
     * Immediate stop the zone
     * It kills all processes within this zone
     */
    bool stop();

    /**
     * Reboot zone
     */
    bool reboot();

    /**
     * Gracefully shutdown zone.
     */
    bool shutdown(int timeout);

    /**
     * Freeze (pause/lock) zone
     */
    bool freeze();

    /**
     * Unfreeze zone
     */
    bool unfreeze();

    /**
     * Get pid of init process
     */
    pid_t getInitPid() const;

    /**
     * Attach to the Zone and run the command binary
     *
     * This call will fork and execure binary
     *
     * @param argv command and its arguments
     */
    bool runInZone(const std::vector<std::string>& argv);

    /**
     * Create a file inside the zone and return it's file descriptor
     *
     * @param[in]  path  Path in container where file shall be created
     * @param[in]  flags Flags used when creating file. Note that the method already provides
     *                   O_CREAT and O_EXCL flags. User must provide the function with
     *                   O_RDWR/O_RDONLY/O_WRONLY flag explicitly.
     * @param[in]  mode  Permissions with which file is created
     * @param[out] fdPtr Pointer to opened file descriptor.
     *
     * @return True on success, false on failure.
     */
    bool createFile(const std::string& path,
                    const std::int32_t flags,
                    const std::int32_t mode,
                    int *fdPtr);

private:
    lxc_container* mLxcContainer;
    bool setRunLevel(int runLevel);
    void refresh();
};


} // namespace lxc
} // namespace vasum


#endif // COMMON_LXC_ZONE_HPP
