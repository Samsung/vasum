/*
 *  Copyright (C) 2015 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License version 2.1 as published by the Free Software Foundation.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

/**
 * @file
 * @author  Mateusz Malicki (m.malicki2@samsung.com)
 * @brief   ContainerImpl main class
 */

#ifndef LXCPP_CONTAINER_IMPL_HPP
#define LXCPP_CONTAINER_IMPL_HPP

#include "lxcpp/container.hpp"
#include "lxcpp/namespace.hpp"

#include "utils/channel.hpp"

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

    // Other
    void attach(Container::AttachCall& attachCall);

private:

    // Methods for different stages of setting up the attachment
    void attachParent(utils::Channel& channel, const pid_t pid);
    void attachIntermediate(utils::Channel& channel, Container::AttachCall& call);
    static int attachChild(void* data);

    pid_t mInitPid;
    std::vector<Namespace> mNamespaces;
};

} // namespace lxcpp

#endif // LXCPP_CONTAINER_IMPL_HPP
