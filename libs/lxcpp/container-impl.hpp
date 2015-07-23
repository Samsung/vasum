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
 * @brief   ContainerImpl main class
 */

#ifndef LXCPP_CONTAINER_IMPL_HPP
#define LXCPP_CONTAINER_IMPL_HPP

#include <lxcpp/container.hpp>

namespace lxcpp {

class ContainerImpl : public virtual Container {
public:
    ContainerImpl();
    ~ContainerImpl();

    std::string getName();
    void setName(const std::string& name);

    //Execution actions
    void start();
    void stop();
    void freeze();
    void unfreeze();
    void reboot();
    int getInitPid();

    //Filesystem actions
    void create();
    void destroy();
    void setRootPath(const std::string& path);
    std::string getRootPath();
};

} // namespace lxcpp

#endif // LXCPP_CONTAINER_IMPL_HPP
