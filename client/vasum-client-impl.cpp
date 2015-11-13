/*
 *  Copyright (c) 2014 Samsung Electronics Co., Ltd All Rights Reserved
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
 * @brief   This file contains vasum-server's client implementation
 */

//TODO: Make dispatcher related function thread-safe.
//For now vsm_connect, vsm_get_dispatcher_type, vsm_set_dispatcher_type,
//vsm_get_poll_fd, vsm_enter_eventloop can't be used at the same time.
//TODO: Make vsm_get_status_message thread-safe version (vsm_get_status_message_r)

#include "config.hpp"
#include "vasum-client-impl.hpp"
#include "utils.hpp"
#include "exception.hpp"
#include "utils/exception.hpp"
#include "logger/logger.hpp"
#include "host-ipc-definitions.hpp"
#include "cargo-ipc/client.hpp"
#include "api/messages.hpp"

#include <algorithm>
#include <vector>
#include <memory>
#include <cstring>
#include <fstream>
#include <arpa/inet.h>
#include <linux/if.h>

#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>

using namespace utils;
using namespace vasum;

namespace {

const int TIMEOUT_INFINITE = -1;

VsmZoneState getZoneState(const char* state)
{
    if (strcmp(state, "STOPPED") == 0) {
        return STOPPED;
    } else if (strcmp(state, "STARTING") == 0) {
        return STARTING;
    } else if (strcmp(state, "RUNNING") == 0) {
        return RUNNING;
    } else if (strcmp(state, "STOPPING") == 0) {
        return STOPPING;
    } else if (strcmp(state, "ABORTING") == 0) {
        return ABORTING;
    } else if (strcmp(state, "FREEZING") == 0) {
        return FREEZING;
    } else if (strcmp(state, "FROZEN") == 0) {
        return FROZEN;
    } else if (strcmp(state, "THAWED") == 0) {
        return THAWED;
    } else if (strcmp(state, "LOCKED") == 0) {
        return LOCKED;
    } else if (strcmp(state, "MAX_STATE") == 0) {
        return MAX_STATE;
    } else if (strcmp(state, "ACTIVATING") == 0) {
        return ACTIVATING;
    }
    throw InvalidResponseException("Unknown state");
}

void convert(const api::VectorOfStrings& in, VsmArrayString& out)
{
    out = reinterpret_cast<char**>(calloc(in.values.size() + 1, sizeof(char*)));
    for (size_t i = 0; i < in.values.size(); ++i) {
        out[i] = ::strdup(in.values[i].c_str());
    }
}

void convert(const api::ZoneInfoOut& info, Zone& zone)
{
    Zone vsmZone = static_cast<Zone>(malloc(sizeof(*vsmZone)));
    vsmZone->id = ::strdup(info.id.c_str());
    vsmZone->terminal = info.vt;
    vsmZone->state = getZoneState(info.state.c_str());
    vsmZone->rootfs_path = ::strdup(info.rootPath.c_str());
    zone = vsmZone;
}

std::string toString(const in_addr* addr)
{
    char buf[INET_ADDRSTRLEN];
    const char* ret = inet_ntop(AF_INET, addr, buf, INET_ADDRSTRLEN);
    if (ret == NULL) {
        throw InvalidArgumentException(getSystemErrorMessage());
    }
    return ret;
}

std::string toString(const in6_addr* addr)
{
    char buf[INET6_ADDRSTRLEN];
    const char* ret = inet_ntop(AF_INET6, addr, buf, INET6_ADDRSTRLEN);
    if (ret == NULL) {
        throw InvalidArgumentException(getSystemErrorMessage());
    }
    return ret;
}

bool readFirstLineOfFile(const std::string& path, std::string& ret)
{
    std::ifstream file(path);
    if (!file) {
        return false;
    }

    getline(file, ret);
    return true;
}

} //namespace

#define IS_SET(param)                                            \
    if (!param) {                                                \
        throw InvalidArgumentException(#param " is not set");    \
    }

Client::Status::Status()
    : mVsmStatus(VSMCLIENT_SUCCESS), mMsg()
{
}

Client::Status::Status(VsmStatus status, const std::string& msg)
    : mVsmStatus(status), mMsg(msg)
{
}

Client::Client() noexcept
{
}

Client::~Client() noexcept
{
}

bool Client::isConnected() const
{
    return mClient && mClient->isStarted();
}

bool Client::isInternalDispatcherEnabled() const
{
    return static_cast<bool>(mInternalDispatcher);
}

cargo::ipc::epoll::EventPoll& Client::getEventPoll() const
{
    if ((mInternalDispatcher && mEventPoll) || (!mInternalDispatcher && !mEventPoll)) {
        throw OperationFailedException("Can't determine dispatcher method");
    }

    if (isInternalDispatcherEnabled()) {
        return mInternalDispatcher->getPoll();
    } else {
        return *mEventPoll;
    }
}

VsmStatus Client::coverException(const std::function<void(void)>& worker) noexcept
{
    try {
        worker();
        mStatusMutex.lock();
        mStatus = Status(VSMCLIENT_SUCCESS);
    } catch (const IOException& ex) {
        mStatus = Status(VSMCLIENT_IO_ERROR, ex.what());
    } catch (const OperationFailedException& ex) {
        mStatus = Status(VSMCLIENT_OPERATION_FAILED, ex.what());
    } catch (const InvalidArgumentException& ex) {
        mStatus = Status(VSMCLIENT_INVALID_ARGUMENT, ex.what());
    } catch (const InvalidResponseException& ex) {
        mStatus = Status(VSMCLIENT_OTHER_ERROR, ex.what());
    } catch (const ClientException& ex) {
        mStatus = Status(VSMCLIENT_CUSTOM_ERROR, ex.what());
    } catch (const cargo::ipc::IPCUserException& ex) {
        mStatus = Status(VSMCLIENT_CUSTOM_ERROR, ex.what());
    } catch (const cargo::ipc::IPCException& ex) {
        mStatus = Status(VSMCLIENT_IO_ERROR, ex.what());
    } catch (const std::exception& ex) {
        mStatus = Status(VSMCLIENT_CUSTOM_ERROR, ex.what());
    }
    VsmStatus ret = mStatus.mVsmStatus;
    mStatusMutex.unlock();
    return ret;
}

VsmStatus Client::connectSystem() noexcept
{
    return connect(HOST_IPC_SOCKET);
}

VsmStatus Client::connect(const std::string& address) noexcept
{
    return coverException([&] {
        if (!mInternalDispatcher && !mEventPoll) {
            vsm_set_dispatcher_type(VSMDISPATCHER_INTERNAL);
        }
        mClient.reset(new cargo::ipc::Client(getEventPoll(), address));
        mClient->start();
    });
}

VsmStatus Client::disconnect() noexcept
{
    return coverException([&] {
        mClient.reset();
    });
}

VsmStatus Client::vsm_get_poll_fd(int* fd) noexcept
{
    return coverException([&] {
        IS_SET(fd);
        if (isInternalDispatcherEnabled()) {
            throw OperationFailedException("Can't get event fd from internal dispatcher");
        }
        *fd =  mEventPoll->getPollFD();
    });
}

VsmStatus Client::vsm_enter_eventloop(int /* flags */, int timeout) noexcept
{
    return coverException([&] {
        if (isInternalDispatcherEnabled()) {
            throw OperationFailedException("Can't enter to event loop of internal dispatcher");
        }
        mEventPoll->dispatchIteration(timeout);
    });
}

VsmStatus Client::vsm_set_dispatcher_type(VsmDispacherType dispacher) noexcept
{
    return coverException([&] {
        if (isConnected()) {
            throw OperationFailedException("Can't change dispacher");
        }
        switch (dispacher) {
            case VSMDISPATCHER_INTERNAL:
                mInternalDispatcher.reset(new cargo::ipc::epoll::ThreadDispatcher());
                mEventPoll.reset();
                break;
            case VSMDISPATCHER_EXTERNAL:
                mEventPoll.reset(new cargo::ipc::epoll::EventPoll());
                mInternalDispatcher.reset();
                break;
            default:
                throw OperationFailedException("Unsupported EventDispacher type");
        }
    });
}

VsmStatus Client::vsm_get_dispatcher_type(VsmDispacherType* dispacher) noexcept
{
    return coverException([&] {
        IS_SET(dispacher);

        if (isInternalDispatcherEnabled()) {
            *dispacher = VSMDISPATCHER_INTERNAL;
        } else {
            *dispacher = VSMDISPATCHER_EXTERNAL;
        }
    });
}

const char* Client::vsm_get_status_message() const noexcept
{
    return mStatus.mMsg.c_str();
}

VsmStatus Client::vsm_get_status() const noexcept
{
    std::lock_guard<std::mutex> lock(mStatusMutex);
    return mStatus.mVsmStatus;
}

VsmStatus Client::vsm_get_zone_dbuses(VsmArrayString* /*keys*/, VsmArrayString* /*values*/) noexcept
{
    return coverException([&] {
        //TODO: Remove vsm_get_zone_dbuses from API
        throw OperationFailedException("Not implemented");
    });
}

VsmStatus Client::vsm_lock_queue() noexcept
{
    return coverException([&] {
        *mClient->callSync<api::Void, api::Void>(
            vasum::api::cargo::ipc::METHOD_LOCK_QUEUE,
            std::make_shared<api::Void>());
    });
}

VsmStatus Client::vsm_unlock_queue() noexcept
{
    return coverException([&] {
        *mClient->callSync<api::Void, api::Void>(
            vasum::api::cargo::ipc::METHOD_UNLOCK_QUEUE,
            std::make_shared<api::Void>());
    });
}

VsmStatus Client::vsm_get_zone_ids(VsmArrayString* array) noexcept
{
    return coverException([&] {
        IS_SET(array);

        api::ZoneIds zoneIds = *mClient->callSync<api::Void, api::ZoneIds>(
            vasum::api::cargo::ipc::METHOD_GET_ZONE_ID_LIST,
            std::make_shared<api::Void>());
        convert(zoneIds, *array);
    });
}

VsmStatus Client::vsm_get_active_zone_id(VsmString* id) noexcept
{
    return coverException([&] {
        IS_SET(id);

        api::ZoneId zoneId = *mClient->callSync<api::Void, api::ZoneId>(
            api::cargo::ipc::METHOD_GET_ACTIVE_ZONE_ID,
            std::make_shared<api::Void>());
        *id = ::strdup(zoneId.value.c_str());
    });
}

VsmStatus Client::vsm_lookup_zone_by_pid(int pid, VsmString* id) noexcept
{
    return coverException([&] {
        IS_SET(id);

        const std::string path = "/proc/" + std::to_string(pid) + "/cpuset";

        std::string cpuset;
        if (!readFirstLineOfFile(path, cpuset)) {
            throw InvalidArgumentException("Process not found");
        }

        std::string zoneId;
        if (!parseZoneIdFromCpuSet(cpuset, zoneId)) {
            throw OperationFailedException("unknown format of cpuset");
        }

        *id = ::strdup(zoneId.c_str());
    });
}

VsmStatus Client::vsm_lookup_zone_by_id(const char* id, Zone* zone) noexcept
{
    return coverException([&] {
        IS_SET(id);
        IS_SET(zone);

        api::ZoneInfoOut info = *mClient->callSync<api::ZoneId, api::ZoneInfoOut>(
            api::cargo::ipc::METHOD_GET_ZONE_INFO,
            std::make_shared<api::ZoneId>(api::ZoneId{ id }));
        convert(info, *zone);
    });
}

VsmStatus Client::vsm_lookup_zone_by_terminal_id(int, VsmString*) noexcept
{
    return coverException([&] {
        //TODO: Implement vsm_lookup_zone_by_terminal_id
        throw OperationFailedException("Not implemented");
    });
}

VsmStatus Client::vsm_set_active_zone(const char* id) noexcept
{
    return coverException([&] {
        IS_SET(id);

        mClient->callSync<api::ZoneId, api::Void>(
            api::cargo::ipc::METHOD_SET_ACTIVE_ZONE,
            std::make_shared<api::ZoneId>(api::ZoneId{ id }));
    });
}

VsmStatus Client::vsm_create_zone(const char* id, const char* tname) noexcept
{
    return coverException([&] {
        IS_SET(id);

        std::string template_name = tname ? tname : "default";
        mClient->callSync<api::CreateZoneIn, api::Void>(
            api::cargo::ipc::METHOD_CREATE_ZONE,
            std::make_shared<api::CreateZoneIn>(api::CreateZoneIn{ id, template_name }),
            TIMEOUT_INFINITE);
    });
}

VsmStatus Client::vsm_destroy_zone(const char* id) noexcept
{
    return coverException([&] {
        IS_SET(id);
        mClient->callSync<api::ZoneId, api::Void>(
            api::cargo::ipc::METHOD_DESTROY_ZONE,
            std::make_shared<api::ZoneId>(api::ZoneId{ id }),
            TIMEOUT_INFINITE);
    });
}

VsmStatus Client::vsm_shutdown_zone(const char* id) noexcept
{
    return coverException([&] {
        IS_SET(id);
        mClient->callSync<api::ZoneId, api::Void>(
            api::cargo::ipc::METHOD_SHUTDOWN_ZONE,
            std::make_shared<api::ZoneId>(api::ZoneId{ id }),
            TIMEOUT_INFINITE);
    });
}

VsmStatus Client::vsm_start_zone(const char* id) noexcept
{
    return coverException([&] {
        IS_SET(id);
        mClient->callSync<api::ZoneId, api::Void>(
            api::cargo::ipc::METHOD_START_ZONE,
            std::make_shared<api::ZoneId>(api::ZoneId{ id }),
            TIMEOUT_INFINITE);
    });
}

VsmStatus Client::vsm_lock_zone(const char* id) noexcept
{
    return coverException([&] {
        IS_SET(id);
        mClient->callSync<api::ZoneId, api::Void>(
            api::cargo::ipc::METHOD_LOCK_ZONE,
            std::make_shared<api::ZoneId>(api::ZoneId{ id }),
            TIMEOUT_INFINITE);
    });
}

VsmStatus Client::vsm_unlock_zone(const char* id) noexcept
{
    return coverException([&] {
        IS_SET(id);
        mClient->callSync<api::ZoneId, api::Void>(
            api::cargo::ipc::METHOD_UNLOCK_ZONE,
            std::make_shared<api::ZoneId>(api::ZoneId{ id }));
    });
}

VsmStatus Client::vsm_add_state_callback(VsmZoneDbusStateFunction /* zoneDbusStateCallback */,
                                    void* /* data */,
                                    VsmSubscriptionId* /* subscriptionId */) noexcept
{
    return coverException([&] {
        //TODO: Implement vsm_add_state_callback
        throw OperationFailedException("Not implemented");
    });
}

VsmStatus Client::vsm_del_state_callback(VsmSubscriptionId subscriptionId) noexcept
{
    return coverException([&] {
        mClient->removeMethod(subscriptionId);
    });
}

VsmStatus Client::vsm_grant_device(const char* id, const char* device, uint32_t flags) noexcept
{
    return coverException([&] {
        IS_SET(id);
        IS_SET(device);

        mClient->callSync<api::GrantDeviceIn, api::Void>(
            api::cargo::ipc::METHOD_GRANT_DEVICE,
            std::make_shared<api::GrantDeviceIn>(api::GrantDeviceIn{ id, device, flags }));
    });
}

VsmStatus Client::vsm_revoke_device(const char* id, const char* device) noexcept
{
    return coverException([&] {
        IS_SET(id);
        IS_SET(device);

        mClient->callSync<api::RevokeDeviceIn, api::Void>(
            api::cargo::ipc::METHOD_REVOKE_DEVICE,
            std::make_shared<api::RevokeDeviceIn>(api::RevokeDeviceIn{ id, device }));
    });
}

VsmStatus Client::vsm_zone_get_netdevs(const char* id, VsmArrayString* netdevIds) noexcept
{
    return coverException([&] {
        IS_SET(id);
        IS_SET(netdevIds);

        api::NetDevList netdevs = *mClient->callSync<api::ZoneId, api::NetDevList>(
            api::cargo::ipc::METHOD_GET_NETDEV_LIST,
            std::make_shared<api::ZoneId>(api::ZoneId{ id }));
        convert(netdevs, *netdevIds);
    });
}

VsmStatus Client::vsm_netdev_get_ip_addr(const char* id,
                                         const char* netdevId,
                                         std::vector<InetAddr>& addrs) noexcept
{
    using namespace boost::algorithm;

    return coverException([&] {
        IS_SET(id);
        IS_SET(netdevId);

        addrs.clear();

        api::GetNetDevAttrs attrs = *mClient->callSync<api::GetNetDevAttrsIn, api::GetNetDevAttrs>(
            api::cargo::ipc::METHOD_GET_NETDEV_ATTRS,
            std::make_shared<api::GetNetDevAttrsIn>(api::GetNetDevAttrsIn{ id, netdevId }));

        for (const auto &attr : attrs.values) {
            InetAddr addr;
            if (attr.first == "ipv4") {
                addr.type = AF_INET;
            }
            else if (attr.first == "ipv6") {
                addr.type = AF_INET6;
            }
            else continue;

            std::vector<std::string> addrAttrs;
            for(const auto& addrAttr : split(addrAttrs, attr.second, is_any_of(","))) {
                size_t pos = addrAttr.find(":");
                if (pos == std::string::npos) continue;

                if (addrAttr.substr(0, pos) == "prefixlen") {
                    addr.prefix = atoi(addrAttr.substr(pos + 1).c_str());
                }
                else if (addrAttr.substr(0, pos) == "ip") {
                    if (inet_pton(addr.type, addrAttr.substr(pos + 1).c_str(), &addr.addr) != 1) {
                        addr.type = -1;
                        break;
                    }
                }
            }
            if (addr.type >= 0)
                addrs.push_back(addr);
        }
    });
}

VsmStatus Client::vsm_netdev_get_ipv4_addr(const char* id,
                                      const char* netdevId,
                                      struct in_addr* addr) noexcept
{
    std::vector<InetAddr> addrs;
    VsmStatus st=vsm_netdev_get_ip_addr(id, netdevId, addrs);
    for (const auto& a : addrs) {
        if (a.type == AF_INET) {
            memcpy(addr, &a.addr, sizeof(*addr));
            break;
        }
    }
    return st;
}

VsmStatus Client::vsm_netdev_get_ipv6_addr(const char* id,
                                      const char* netdevId,
                                      struct in6_addr* addr) noexcept
{
    std::vector<InetAddr> addrs;
    VsmStatus st=vsm_netdev_get_ip_addr(id, netdevId, addrs);
    for (const auto& a : addrs) {
        if (a.type == AF_INET6) {
            memcpy(addr, &a.addr, sizeof(*addr));
            break;
        }
    }
    return st;
}

VsmStatus Client::vsm_netdev_add_ipv4_addr(const char* id,
                                      const char* netdevId,
                                      struct in_addr* addr,
                                      int prefix) noexcept
{
    return coverException([&] {
        IS_SET(id);
        IS_SET(netdevId);
        IS_SET(addr);

        std::string value = "ip:" + toString(addr) + ",""prefixlen:" + std::to_string(prefix);
        mClient->callSync<api::SetNetDevAttrsIn, api::Void>(
            api::cargo::ipc::METHOD_SET_NETDEV_ATTRS,
            std::make_shared<api::SetNetDevAttrsIn>(
                api::SetNetDevAttrsIn{ id, netdevId, { { "ipv4", value } }  }));
    });
}

VsmStatus Client::vsm_netdev_add_ipv6_addr(const char* id,
                                      const char* netdevId,
                                      struct in6_addr* addr,
                                      int prefix) noexcept
{
    return coverException([&] {
        IS_SET(id);
        IS_SET(netdevId);
        IS_SET(addr);

        std::string value = "ip:" + toString(addr) + ",""prefixlen:" + std::to_string(prefix);
        mClient->callSync<api::SetNetDevAttrsIn, api::Void>(
            api::cargo::ipc::METHOD_SET_NETDEV_ATTRS,
            std::make_shared<api::SetNetDevAttrsIn>(
                api::SetNetDevAttrsIn{ id, netdevId, { { "ipv6", value } }  }));
    });
}

VsmStatus Client::vsm_netdev_del_ipv4_addr(const char* id,
                                      const char* netdevId,
                                      struct in_addr* addr,
                                      int prefix) noexcept
{
    return coverException([&] {
        IS_SET(id);
        IS_SET(netdevId);
        IS_SET(addr);

        //CIDR notation
        std::string ip = toString(addr) + "/" + std::to_string(prefix);
        mClient->callSync<api::DeleteNetdevIpAddressIn, api::Void>(
            api::cargo::ipc::METHOD_DELETE_NETDEV_IP_ADDRESS,
            std::make_shared<api::DeleteNetdevIpAddressIn>(
                api::DeleteNetdevIpAddressIn{ id, netdevId, ip }));
    });
}

VsmStatus Client::vsm_netdev_del_ipv6_addr(const char* id,
                                      const char* netdevId,
                                      struct in6_addr* addr,
                                      int prefix) noexcept
{
    return coverException([&] {
        IS_SET(id);
        IS_SET(netdevId);
        IS_SET(addr);

        //CIDR notation
        std::string ip = toString(addr) + "/" + std::to_string(prefix);
        mClient->callSync<api::DeleteNetdevIpAddressIn, api::Void>(
            api::cargo::ipc::METHOD_DELETE_NETDEV_IP_ADDRESS,
            std::make_shared<api::DeleteNetdevIpAddressIn>(
                api::DeleteNetdevIpAddressIn{ id, netdevId, ip }));
    });
}


VsmStatus Client::vsm_netdev_up(const char* id, const char* netdevId) noexcept
{
    return coverException([&] {
        IS_SET(id);
        IS_SET(netdevId);

        mClient->callSync<api::SetNetDevAttrsIn, api::Void>(
            api::cargo::ipc::METHOD_SET_NETDEV_ATTRS,
            std::make_shared<api::SetNetDevAttrsIn>(
                api::SetNetDevAttrsIn{ id, netdevId, { { "flags", std::to_string(IFF_UP) },
                                                       { "change", std::to_string(IFF_UP) }  }  }));
    });
}

VsmStatus Client::vsm_netdev_down(const char* id, const char* netdevId) noexcept
{
    return coverException([&] {
        IS_SET(id);
        IS_SET(netdevId);

        mClient->callSync<api::SetNetDevAttrsIn, api::Void>(
            api::cargo::ipc::METHOD_SET_NETDEV_ATTRS,
            std::make_shared<api::SetNetDevAttrsIn>(
                api::SetNetDevAttrsIn{ id, netdevId, { { "flags", std::to_string(~IFF_UP) },
                                                       { "change", std::to_string(IFF_UP) }  }  }));
    });
}

VsmStatus Client::vsm_create_netdev_veth(const char* id,
                                    const char* zoneDev,
                                    const char* hostDev) noexcept
{
    return coverException([&] {
        IS_SET(id);
        IS_SET(zoneDev);
        IS_SET(hostDev);

        mClient->callSync<api::CreateNetDevVethIn, api::Void>(
            api::cargo::ipc::METHOD_CREATE_NETDEV_VETH,
            std::make_shared<api::CreateNetDevVethIn>(
                api::CreateNetDevVethIn{ id, zoneDev, hostDev }));
    });
}

VsmStatus Client::vsm_create_netdev_macvlan(const char* id,
                                       const char* zoneDev,
                                       const char* hostDev,
                                       enum macvlan_mode mode) noexcept
{
    return coverException([&] {
        IS_SET(id);
        IS_SET(zoneDev);
        IS_SET(hostDev);

        mClient->callSync<api::CreateNetDevMacvlanIn, api::Void>(
            api::cargo::ipc::METHOD_CREATE_NETDEV_MACVLAN,
            std::make_shared<api::CreateNetDevMacvlanIn>(
                api::CreateNetDevMacvlanIn{ id, zoneDev, hostDev, mode }));
    });
}

VsmStatus Client::vsm_create_netdev_phys(const char* id, const char* devId) noexcept
{
    return coverException([&] {
        IS_SET(id);
        IS_SET(devId);

        mClient->callSync<api::CreateNetDevPhysIn, api::Void>(
            api::cargo::ipc::METHOD_CREATE_NETDEV_PHYS,
            std::make_shared<api::CreateNetDevPhysIn>(
                api::CreateNetDevPhysIn{ id, devId }));
    });
}

VsmStatus Client::vsm_lookup_netdev_by_name(const char* id,
                                       const char* netdevId,
                                       Netdev* netdev) noexcept
{
    using namespace boost::algorithm;

    return coverException([&] {
        IS_SET(id);
        IS_SET(netdevId);
        IS_SET(netdev);

        api::GetNetDevAttrs attrs = *mClient->callSync<api::GetNetDevAttrsIn, api::GetNetDevAttrs>(
            api::cargo::ipc::METHOD_GET_NETDEV_ATTRS,
            std::make_shared<api::GetNetDevAttrsIn>(api::GetNetDevAttrsIn{ id, netdevId }));
        auto it = find_if(attrs.values.begin(),
                          attrs.values.end(),
                          [](const api::StringPair& entry) {
                return entry.first == "type";
        });

        VsmNetdevType type;
        if (it == attrs.values.end()) {
            throw OperationFailedException("Can't fetch netdev type");
        }

        switch (stoi(it->second)) {
            case 1<<0  /*IFF_802_1Q_VLAN*/: type = VSMNETDEV_VETH; break;
            case 1<<21 /*IFF_MACVLAN*/: type = VSMNETDEV_MACVLAN; break;
            default:
                throw InvalidResponseException("Unknown netdev type: " + it->second);
        }

        *netdev = static_cast<Netdev>(malloc(sizeof(**netdev)));
        (*netdev)->name = ::strdup(id);
        (*netdev)->type = type;
    });
}

VsmStatus Client::vsm_destroy_netdev(const char* id, const char* devId) noexcept
{
    return coverException([&] {
        IS_SET(id);
        IS_SET(devId);

        mClient->callSync<api::DestroyNetDevIn, api::Void>(
            api::cargo::ipc::METHOD_DESTROY_NETDEV,
            std::make_shared<api::DestroyNetDevIn>(api::DestroyNetDevIn{ id, devId }));
    });
}

VsmStatus Client::vsm_declare_file(const char* id,
                              VsmFileType type,
                              const char *path,
                              int32_t flags,
                              mode_t mode,
                              VsmString* declarationId) noexcept
{
    return coverException([&] {
        IS_SET(id);
        IS_SET(path);

        api::Declaration declaration = *mClient->callSync<api::DeclareFileIn, api::Declaration>(
            api::cargo::ipc::METHOD_DECLARE_FILE,
            std::make_shared<api::DeclareFileIn>(
                api::DeclareFileIn{ id, type, path, flags, (int)mode }));
        if (declarationId != NULL) {
            *declarationId = ::strdup(declaration.value.c_str());
        }
    });
}

VsmStatus Client::vsm_declare_mount(const char *source,
                               const char* id,
                               const char *target,
                               const char *type,
                               uint64_t flags,
                               const char *data,
                               VsmString* declarationId) noexcept
{
    return coverException([&] {
        IS_SET(source);
        IS_SET(id);
        IS_SET(target);
        IS_SET(type);
        if (!data) {
            data = "";
        }

        api::Declaration declaration = *mClient->callSync<api::DeclareMountIn, api::Declaration>(
            api::cargo::ipc::METHOD_DECLARE_MOUNT,
            std::make_shared<api::DeclareMountIn>(
                api::DeclareMountIn{ source, id, target, type, flags, data }));
        if (declarationId != NULL) {
            *declarationId = ::strdup(declaration.value.c_str());
        }
    });
}

VsmStatus Client::vsm_declare_link(const char* source,
                              const char* id,
                              const char* target,
                              VsmString* declarationId) noexcept
{
    return coverException([&] {
        IS_SET(source);
        IS_SET(id);
        IS_SET(target);

        api::Declaration declaration = *mClient->callSync<api::DeclareLinkIn, api::Declaration>(
            api::cargo::ipc::METHOD_DECLARE_LINK,
            std::make_shared<api::DeclareLinkIn>(api::DeclareLinkIn{ source, id, target }));
        if (declarationId != NULL) {
            *declarationId = ::strdup(declaration.value.c_str());
        }
    });
}

VsmStatus Client::vsm_list_declarations(const char* id, VsmArrayString* declarations) noexcept
{
    return coverException([&] {
        IS_SET(id);
        IS_SET(declarations);

        api::Declarations declarationsOut = *mClient->callSync<api::ZoneId, api::Declarations>(
            api::cargo::ipc::METHOD_GET_DECLARATIONS,
            std::make_shared<api::ZoneId>(api::ZoneId{ id }));
        convert(declarationsOut, *declarations);
    });
}

VsmStatus Client::vsm_remove_declaration(const char* id, VsmString declaration) noexcept
{
    return coverException([&] {
        IS_SET(id);
        IS_SET(declaration);

        mClient->callSync<api::RemoveDeclarationIn, api::Void>(
            api::cargo::ipc::METHOD_REMOVE_DECLARATION,
            std::make_shared<api::RemoveDeclarationIn>(api::RemoveDeclarationIn{ id, declaration }));
    });
}

VsmStatus Client::vsm_clean_up_zones_root() noexcept
{
    return coverException([&] {
        mClient->callSync<api::Void, api::Void>(
            api::cargo::ipc::METHOD_CLEAN_UP_ZONES_ROOT,
            std::make_shared<api::Void>());
    });
}
