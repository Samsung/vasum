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
 * @brief   Implementation of a class for checking permissions of proxy calls
 */

#include "config.hpp"

#include "proxy-call-policy.hpp"


namespace vasum {

namespace {
const std::string ANY = "*";

inline bool match(const std::string& rule, const std::string& value) {
    // simple matching, change to regex if it turns out to be insufficient
    return rule == ANY || rule == value;
}

} // namespace


ProxyCallPolicy::ProxyCallPolicy(const std::vector<ProxyCallRule>& proxyCallRules)
    : mProxyCallRules(proxyCallRules)
{
}

bool ProxyCallPolicy::isProxyCallAllowed(const std::string& caller,
                                         const std::string& target,
                                         const std::string& targetBusName,
                                         const std::string& targetObjectPath,
                                         const std::string& targetInterface,
                                         const std::string& targetMethod) const
{
    for (const ProxyCallRule& rule : mProxyCallRules) {
        if (match(rule.caller, caller)
                && match(rule.target, target)
                && match(rule.targetBusName, targetBusName)
                && match(rule.targetObjectPath, targetObjectPath)
                && match(rule.targetInterface, targetInterface)
                && match(rule.targetMethod, targetMethod)) {
            return true;
        }
    }

    return false;
}


} // namespace vasum
