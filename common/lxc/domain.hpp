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

    LxcDomain(const std::string& lxcPath, const std::string& domainName);
    ~LxcDomain();

    LxcDomain(const LxcDomain&) = delete;
    LxcDomain& operator=(const LxcDomain&) = delete;

    std::string getName() const;

    std::string getConfigItem(const std::string& key);

    bool isDefined();

    State getState();

    bool create(const std::string& templatePath);
    bool destroy();

    bool start(const char* const* argv);
    bool stop();
    bool reboot();
    bool shutdown(int timeout);

    bool freeze();
    bool unfreeze();
private:
    lxc_container* mContainer;

    bool setRunLevel(int runLevel);
};


} // namespace lxc
} // namespace security_containers


#endif // COMMON_LXC_DOMAIN_HPP
