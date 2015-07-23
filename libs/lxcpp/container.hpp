/*
 *  Copyright (c) 2015 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *  Contact: Mateusz Malicki <m.malicki2@samsung.com>
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
 * @author  Mateusz Malicki (m.malicki2@samsung.com)
 * @brief   Container interface
 */

#ifndef LXCPP_CONTAINER_HPP
#define LXCPP_CONTAINER_HPP

#include <string>

namespace lxcpp {

class Container {
public:
    virtual ~Container();

    virtual std::string getName() = 0;
    virtual void setName(const std::string& name) = 0;

    //Execution actions
    virtual void start() = 0;
    virtual void stop() = 0;
    virtual void freeze() = 0;
    virtual void unfreeze() = 0;
    virtual void reboot() = 0;
    virtual int getInitPid() = 0;

    //Filesystem actions
    virtual void create() = 0;
    virtual void destroy() = 0;
    virtual void setRootPath(const std::string& path) = 0;
    virtual std::string getRootPath() = 0;
};

} // namespace lxcpp

#endif // LXCPP_CONTAINER_HPP
