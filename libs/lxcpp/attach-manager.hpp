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
 * @author  Jan Olszak (j.olszak@samsung.com)
 * @brief   Implementation of attaching to a container
 */

#ifndef LXCPP_ATTACH_MANAGER_HPP
#define LXCPP_ATTACH_MANAGER_HPP

#include "lxcpp/container-impl.hpp"
#include "utils/channel.hpp"

#include <string>

namespace lxcpp {

class AttachManager final {
public:
    AttachManager(lxcpp::ContainerImpl& container);
    ~AttachManager();

    /**
     * Runs the call in the container's context
     *
     * @param call function to run inside container
     * @param wdInContainer Current Work Directory. Path relative to container's root
     */
    void attach(Container::AttachCall& call,
                const std::string& wdInContainer);

private:
    const lxcpp::ContainerImpl& mContainer;

    // Methods for different stages of setting up the attachment
    static int child(void* data);

    void parent(utils::Channel& intermChannel,
                const pid_t pid);

    void interm(utils::Channel& intermChannel,
                const std::string& wdInContainer,
                Container::AttachCall& call);
};

} // namespace lxcpp

#endif // LXCPP_ATTACH_MANAGER_HPP