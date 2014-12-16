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
 * @brief   Declaration of a class for checking permissions of proxy calls
 */


#ifndef SERVER_PROXY_CALL_POLICY_HPP
#define SERVER_PROXY_CALL_POLICY_HPP

#include "proxy-call-config.hpp"

#include <vector>


namespace vasum {


class ProxyCallPolicy {

public:
    ProxyCallPolicy(const std::vector<ProxyCallRule>& proxyCallRules);

    bool isProxyCallAllowed(const std::string& caller,
                            const std::string& target,
                            const std::string& targetBusName,
                            const std::string& targetObjectPath,
                            const std::string& targetInterface,
                            const std::string& targetMethod) const;

private:
    std::vector<ProxyCallRule> mProxyCallRules;
};


} // namespace vasum


#endif // SERVER_PROXY_CALL_POLICY_HPP
