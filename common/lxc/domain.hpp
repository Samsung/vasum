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

#ifndef COMMON_LXC_DOMAIN_HPP
#define COMMON_LXC_DOMAIN_HPP

#include <string>

// fwd declaration of lxc internals
struct lxc_container;

namespace security_containers {
namespace lxc {


/**
 * A class wwapping lxc container
 */
class LxcDomain {
public:
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
     * LxcDomain constructor
     * @param lxcPath path where containers lives
     * @param domainName name of domain
     */
    LxcDomain(const std::string& lxcPath, const std::string& domainName);
    ~LxcDomain();

    LxcDomain(const LxcDomain&) = delete;
    LxcDomain& operator=(const LxcDomain&) = delete;

    /**
     * Get domain name
     */
    std::string getName() const;

    /**
     * Get item from lxc config file
     * @throw LxcException if key not found
     */
    std::string getConfigItem(const std::string& key);

    /**
     * Is domain defined (created)?
     */
    bool isDefined();

    /**
     * String representation of state
     */
    static std::string toString(State state);

    /**
     * Get domain state
     */
    State getState();

    /**
     * Wait till domain is in specified state
     * @return false on timeout
     */
    bool waitForState(State state, int timeout);

    /**
     * Create domain
     * @param templatePath template from which domain will be created
     * @param argv additional template arguments
     */
    bool create(const std::string& templatePath, const char* const* argv);

    /**
     * Destroy domain
     */
    bool destroy();

    /**
     * Start domain
     * @param argv init process with arguments
     */
    bool start(const char* const* argv);

    /**
     * Immediate stop the domain
     * It kills all processes within this domain
     */
    bool stop();

    /**
     * Reboot domain
     */
    bool reboot();

    /**
     * Gracefully shutdown domain.
     */
    bool shutdown(int timeout);

    /**
     * Freeze (pause/lock) domain
     */
    bool freeze();

    /**
     * Unfreeze domain
     */
    bool unfreeze();
private:
    lxc_container* mContainer;

    bool setRunLevel(int runLevel);
    void refresh();
};


} // namespace lxc
} // namespace security_containers


#endif // COMMON_LXC_DOMAIN_HPP
