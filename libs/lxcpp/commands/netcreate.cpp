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
 * @brief   Network configuration command
 */

#include "lxcpp/commands/netcreate.hpp"
#include "lxcpp/network.hpp"

namespace lxcpp {

namespace {
void createBridgeIfNeeded(const NetworkInterfaceConfig& interface) {
    NetworkInterface bridge(interface.getHostIf(), 0);

    if (!bridge.exists()) {
        bridge.create(InterfaceType::BRIDGE);
    }
    else {
        LOGD("bridge already exists, reusing it");
    }

    bridge.up();
    LOGI("adding IPs");
    std::vector<InetAddr> bra = bridge.getInetAddressList();
    for (const auto& addr : interface.getAddrList()) {
        InetAddr a(addr);
        if (std::find(bra.begin(), bra.end(), a) == bra.end()) {
            bridge.addInetAddr(addr);
        }
    }
}

void createVeth(const NetworkInterfaceConfig& interface, pid_t pid) {
    NetworkInterface veth1(interface.getZoneIf() + "-br" + std::to_string(pid), 0);
    NetworkInterface veth2(interface.getZoneIf(), 0);

    veth1.create(InterfaceType::VETH, veth2.getName());
    veth1.addToBridge(interface.getHostIf());
    veth1.up();

    veth2.moveToContainer(pid);
}
} // namespace

void NetCreateAll::execute()
{
    for (const auto& interface : mNetwork.getInterfaces()) {
        LOGI("Creating interface " + interface.getHostIf());
        if (interface.getType() == InterfaceType::BRIDGE) {
            createBridgeIfNeeded(interface);
        }
        else if (interface.getType() == InterfaceType::VETH) {
            createVeth(interface, mPid);
        }
        else { // generic
            NetworkInterface(interface.getHostIf()).create(interface.getType(), interface.getZoneIf(), interface.getMode());
        }
    }
}

void NetConfigureAll::execute()
{
    NetworkInterface("lo", 0).up();

    bool needDefaultRoute = true;

    for (const auto& interface : mNetwork.getInterfaces()) {
        if (interface.getType() == InterfaceType::VETH) {
            NetworkInterface networkInterface(interface.getZoneIf(), 0);

            Attrs attrs;
            if (interface.getMTU() > 0) {
                attrs.push_back(Attr{AttrName::MTU, std::to_string(interface.getMTU())});
            }
            if (!interface.getMACAddress().empty()) {
                attrs.push_back(Attr{AttrName::MAC, interface.getMACAddress()});
            }
            if (interface.getTxLength() > 0) {
                attrs.push_back(Attr{AttrName::TXQLEN, std::to_string(interface.getTxLength())});
            }
            networkInterface.setAttrs(attrs);
            networkInterface.up();

            // TODO: add container routing config to network configration
            // NOTE: temporary - calc gw as a first IP in the network
            InetAddr gw;
            for (const auto& addr : interface.getAddrList()) {
                networkInterface.addInetAddr(addr);
                // NOTE: prefix 31 is used only for p2p (RFC3021)
                if (gw.prefix == 0 && addr.type == InetAddrType::IPV4 && addr.prefix < 31) {
                    gw = addr;
                    unsigned mask = ((1 << addr.prefix) - 1) << (32 - addr.prefix);
                    unsigned net = ::ntohl(gw.getAddr<in_addr>().s_addr) & mask;
                    gw.getAddr<in_addr>().s_addr = ::htonl(net + 1);
                }
            }

            if (needDefaultRoute && gw.prefix > 0) {
                needDefaultRoute = false;
                gw.prefix = 0;
                networkInterface.addRoute(Route{
                    gw,             //dst - gateway
                    InetAddr(),     //src - not specified (prefix=0)
                    0,
                    "",
                    RoutingTable::UNSPEC
                });
            }
        }
    }

}

void NetInteraceCreate::execute()
{
    NetworkInterface networkInterface(mZoneIf);
    networkInterface.create(mType, mHostIf, mMode);
}

void NetInterfaceSetAttrs::execute()
{
    NetworkInterface networkInterface(mIfname);
    networkInterface.setAttrs(mAttrs);
}

void NetInterfaceAddInetAddr::execute()
{
    NetworkInterface networkInterface(mIfname);
    for (const auto& a : mAddrList) {
        networkInterface.addInetAddr(a);
    }
}

} // namespace lxcpp
