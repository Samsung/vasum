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
 * @author  Krzysztof Dynowski (k.dynowski@samsumg.com)
 * @brief   Actions on network interace in the container
 */

#ifndef LXCPP_NETWORK_HPP
#define LXCPP_NETWORK_HPP

#include "lxcpp/container.hpp"
#include <vector>

namespace lxcpp {

struct Attr {
    std::string name;
    std::string value;
};

typedef std::vector<Attr> Attrs;


/// Network operations to be performed on given container and interface
/// operates on netlink device
class NetworkInterface {
public:
    NetworkInterface(Container& c, const std::string& ifname) :
        mContainer(c),
        mIfname(ifname)
    {
        // TODO: Remove temporary usage
        (void) mContainer;
    }

    //Network actions on Container
    void create(const std::string& hostif, InterfaceType type, MacVLanMode mode=MacVLanMode::PRIVATE);
    void destroy();

    NetStatus status();
    void up();
    void down();
    void setAttrs(const Attrs& attrs);
    const Attrs getAttrs() const;

    static std::vector<std::string> getInterfaces(pid_t initpid);

private:
    void createVeth(const std::string& hostif);
    void createBridge(const std::string& hostif);
    void createMacVLan(const std::string& hostif, MacVLanMode mode);
    void move(const std::string& hostif);

    Container& mContainer;       ///< Container to operate on
    const std::string mIfname;   ///< network interface name inside zone
};

} // namespace lxcpp

#endif // LXCPP_NETWORK_HPP
