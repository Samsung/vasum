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

#include <config.hpp>
#include "vasum-client-impl.hpp"
#include "utils.hpp"
#include "exception.hpp"
#include "host-ipc-connection.hpp"
#include "logger/logger.hpp"

#include <algorithm>
#include <vector>
#include <cstring>
#include <cassert>
#include <fstream>
#include <arpa/inet.h>
#include <linux/if.h>

#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>

using namespace std;
using namespace vasum;

namespace {

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

void convert(const api::ZoneInfoOut& info, VsmZone& zone)
{
    VsmZone vsmZone = reinterpret_cast<VsmZone>(malloc(sizeof(*vsmZone)));
    vsmZone->id = ::strdup(info.id.c_str());
    vsmZone->terminal = info.vt;
    vsmZone->state = getZoneState(info.state.c_str());
    vsmZone->rootfs_path = ::strdup(info.rootPath.c_str());
    zone = vsmZone;
}

string toString(const in_addr* addr)
{
    char buf[INET_ADDRSTRLEN];
    const char* ret = inet_ntop(AF_INET, addr, buf, INET_ADDRSTRLEN);
    if (ret == NULL) {
        throw InvalidArgumentException(getSystemErrorMessage());
    }
    return ret;
}

string toString(const in6_addr* addr)
{
    char buf[INET6_ADDRSTRLEN];
    const char* ret = inet_ntop(AF_INET6, addr, buf, INET6_ADDRSTRLEN);
    if (ret == NULL) {
        throw InvalidArgumentException(getSystemErrorMessage());
    }
    return ret;
}

bool readFirstLineOfFile(const string& path, string& ret)
{
    ifstream file(path);
    if (!file) {
        return false;
    }

    getline(file, ret);
    return true;
}

} //namespace

VsmStatus Client::vsm_start_glib_loop() noexcept
{
    // TPDP: Remove vsm_start_glib_loop from API
    return VSMCLIENT_SUCCESS;
}

VsmStatus Client::vsm_stop_glib_loop() noexcept
{
    // TPDP: Remove vsm_stop_glib_loop from API
    return VSMCLIENT_SUCCESS;
}

Client::Status::Status()
    : mVsmStatus(VSMCLIENT_SUCCESS), mMsg()
{
}

Client::Status::Status(VsmStatus status, const string& msg)
    : mVsmStatus(status), mMsg(msg)
{
}

Client::Client() noexcept
{
}

Client::~Client() noexcept
{
}

VsmStatus Client::coverException(const function<void(void)>& worker) noexcept
{
    try {
        worker();
        mStatus = Status(VSMCLIENT_SUCCESS);
    } catch (const vasum::IOException& ex) {
        mStatus = Status(VSMCLIENT_IO_ERROR, ex.what());
    } catch (const vasum::OperationFailedException& ex) {
        mStatus = Status(VSMCLIENT_OPERATION_FAILED, ex.what());
    } catch (const vasum::InvalidArgumentException& ex) {
        mStatus = Status(VSMCLIENT_INVALID_ARGUMENT, ex.what());
    } catch (const vasum::InvalidResponseException& ex) {
        mStatus = Status(VSMCLIENT_OTHER_ERROR, ex.what());
    } catch (const vasum::ClientException& ex) {
        mStatus = Status(VSMCLIENT_CUSTOM_ERROR, ex.what());
    } catch (const ipc::IPCUserException& ex) {
        mStatus = Status(VSMCLIENT_CUSTOM_ERROR, ex.what());
    } catch (const ipc::IPCException& ex) {
        mStatus = Status(VSMCLIENT_IO_ERROR, ex.what());
    } catch (const exception& ex) {
        mStatus = Status(VSMCLIENT_CUSTOM_ERROR, ex.what());
    }
    if (mStatus.mVsmStatus!=VSMCLIENT_SUCCESS) {
        LOGE("Exception: " << mStatus.mMsg);
    }
    return mStatus.mVsmStatus;
}

VsmStatus Client::createSystem() noexcept
{
    return coverException([&] {
        mHostClient.createSystem();
    });
}

VsmStatus Client::create(const string& address) noexcept
{
    return coverException([&] {
        mHostClient.create(address);
    });
}

epoll::EventPoll& Client::getEventPoll() noexcept
{
    return mHostClient.getDispatcher().getPoll();
}

const char* Client::vsm_get_status_message() const noexcept
{
    return mStatus.mMsg.c_str();
}

VsmStatus Client::vsm_get_status() const noexcept
{
    return mStatus.mVsmStatus;
}

VsmStatus Client::vsm_get_zone_dbuses(VsmArrayString* /*keys*/, VsmArrayString* /*values*/) noexcept
{
    return coverException([&] {
        //TODO: Remove vsm_get_zone_dbuses from API
        throw OperationFailedException("Not implemented");
    });
}

VsmStatus Client::vsm_get_zone_ids(VsmArrayString* array) noexcept
{
    assert(array);

    return coverException([&] {
        api::ZoneIds zoneIds;
        mHostClient.callGetZoneIds(zoneIds);
        convert(zoneIds, *array);
    });
}

VsmStatus Client::vsm_get_active_zone_id(VsmString* id) noexcept
{
    assert(id);

    return coverException([&] {
        api::ZoneId zoneId;
        mHostClient.callGetActiveZoneId(zoneId);
        *id = ::strdup(zoneId.value.c_str());
    });
}

VsmStatus Client::vsm_lookup_zone_by_pid(int pid, VsmString* id) noexcept
{
    assert(id);

    return coverException([&] {
        const string path = "/proc/" + to_string(pid) + "/cpuset";

        string cpuset;
        if (!readFirstLineOfFile(path, cpuset)) {
            throw InvalidArgumentException("Process not found");
        }

        string zoneId;
        if (!parseZoneIdFromCpuSet(cpuset, zoneId)) {
            throw OperationFailedException("unknown format of cpuset");
        }

        *id = ::strdup(zoneId.c_str());
    });
}

VsmStatus Client::vsm_lookup_zone_by_id(const char* id, VsmZone* zone) noexcept
{
    assert(id);
    assert(zone);

    return coverException([&] {
        api::ZoneInfoOut info;
        mHostClient.callGetZoneInfo({ id }, info);
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
    assert(id);

    return coverException([&] {
        mHostClient.callSetActiveZone({ id });
    });
}

VsmStatus Client::vsm_create_zone(const char* id, const char* tname) noexcept
{
    assert(id);

    return coverException([&] {
        string template_name = tname ? tname : "default";
        mHostClient.callCreateZone({ id, template_name });
    });
}

VsmStatus Client::vsm_destroy_zone(const char* id) noexcept
{
    assert(id);

    return coverException([&] {
        mHostClient.callDestroyZone({ id });
    });
}

VsmStatus Client::vsm_shutdown_zone(const char* id) noexcept
{
    assert(id);

    return coverException([&] {
        mHostClient.callShutdownZone({ id });
    });
}

VsmStatus Client::vsm_start_zone(const char* id) noexcept
{
    assert(id);

    return coverException([&] {
        mHostClient.callStartZone({ id });
    });
}

VsmStatus Client::vsm_lock_zone(const char* id) noexcept
{
    assert(id);

    return coverException([&] {
        mHostClient.callLockZone({ id });
    });
}

VsmStatus Client::vsm_unlock_zone(const char* id) noexcept
{
    assert(id);

    return coverException([&] {
        mHostClient.callUnlockZone({ id });
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
        mHostClient.unsubscribe(subscriptionId);
    });
}

VsmStatus Client::vsm_grant_device(const char* id, const char* device, uint32_t flags) noexcept
{
    assert(id);
    assert(device);

    return coverException([&] {
        mHostClient.callGrantDevice({ id, device, flags });
    });
}

VsmStatus Client::vsm_revoke_device(const char* id, const char* device) noexcept
{
    assert(id);
    assert(device);

    return coverException([&] {
        mHostClient.callRevokeDevice({ id, device });
    });
}

VsmStatus Client::vsm_zone_get_netdevs(const char* id, VsmArrayString* netdevIds) noexcept
{
    assert(id);
    assert(netdevIds);

    return coverException([&] {
        api::NetDevList netdevs;
        mHostClient.callGetNetdevList({ id }, netdevs);
        convert(netdevs, *netdevIds);
    });
}

VsmStatus Client::vsm_netdev_get_ip_addr(const char* id,
                                         const char* netdevId,
                                         int type,
                                         void* addr) noexcept
{
    using namespace boost::algorithm;

    assert(id);
    assert(netdevId);
    assert(addr);

    return coverException([&] {
        api::GetNetDevAttrs attrs;
        mHostClient.callGetNetdevAttrs({ id, netdevId }, attrs);

        auto it = find_if(attrs.values.begin(),
                          attrs.values.end(),
                          [type](const api::StringPair& entry) {
                return entry.first == (type == AF_INET ? "ipv4" : "ipv6");
        });

        if (it != attrs.values.end()) {
            vector<string> addrAttrs;
            for(auto addrAttr : split(addrAttrs, it->second, is_any_of(","))) {
                size_t pos = addrAttr.find(":");
                if (addrAttr.substr(0, pos) == "ip") {
                    if (pos != string::npos && pos < addrAttr.length() &&
                        inet_pton(type, addrAttr.substr(pos + 1).c_str(), addr) == 1) {
                        //XXX: return only one address
                        return;
                    } else {
                        throw InvalidResponseException("Wrong address format returned");
                    }
                }
            }
        }
        throw OperationFailedException("Address not found");
    });
}

VsmStatus Client::vsm_netdev_get_ipv4_addr(const char* id,
                                      const char* netdevId,
                                      struct in_addr* addr) noexcept
{
    return vsm_netdev_get_ip_addr(id, netdevId, AF_INET, addr);
}

VsmStatus Client::vsm_netdev_get_ipv6_addr(const char* id,
                                      const char* netdevId,
                                      struct in6_addr* addr) noexcept
{
    return vsm_netdev_get_ip_addr(id, netdevId, AF_INET6, addr);
}

VsmStatus Client::vsm_netdev_set_ipv4_addr(const char* id,
                                      const char* netdevId,
                                      struct in_addr* addr,
                                      int prefix) noexcept
{
    assert(id);
    assert(netdevId);
    assert(addr);

    return coverException([&] {
        string value = "ip:" + toString(addr) + ",""prefixlen:" + to_string(prefix);
        mHostClient.callSetNetdevAttrs({ id, netdevId, { { "ipv4", value } }  } );
    });
}

VsmStatus Client::vsm_netdev_set_ipv6_addr(const char* id,
                                      const char* netdevId,
                                      struct in6_addr* addr,
                                      int prefix) noexcept
{
    assert(id);
    assert(netdevId);
    assert(addr);

    return coverException([&] {
        string value = "ip:" + toString(addr) + ",""prefixlen:" + to_string(prefix);
        mHostClient.callSetNetdevAttrs({ id, netdevId, { { "ipv6", value } }  } );
    });
}

VsmStatus Client::vsm_netdev_del_ipv4_addr(const char* id,
                                      const char* netdevId,
                                      struct in_addr* addr,
                                      int prefix) noexcept
{
    assert(id);
    assert(netdevId);
    assert(addr);

    return coverException([&] {
        //CIDR notation
        string ip = toString(addr) + "/" + to_string(prefix);
        mHostClient.callDeleteNetdevIpAddress({ id, netdevId, ip });
    });
}

VsmStatus Client::vsm_netdev_del_ipv6_addr(const char* id,
                                      const char* netdevId,
                                      struct in6_addr* addr,
                                      int prefix) noexcept
{
    assert(id);
    assert(netdevId);
    assert(addr);

    return coverException([&] {
        //CIDR notation
        string ip = toString(addr) + "/" + to_string(prefix);
        mHostClient.callDeleteNetdevIpAddress({ id, netdevId, ip });
    });
}


VsmStatus Client::vsm_netdev_up(const char* id, const char* netdevId) noexcept
{
    assert(id);
    assert(netdevId);

    return coverException([&] {
        mHostClient.callSetNetdevAttrs({ id, netdevId, { { "flags", to_string(IFF_UP) },
                                                          { "change", to_string(IFF_UP) }  }  } );
    });
}

VsmStatus Client::vsm_netdev_down(const char* id, const char* netdevId) noexcept
{
    assert(id);
    assert(netdevId);

    return coverException([&] {
        mHostClient.callSetNetdevAttrs({ id, netdevId, { { "flags", to_string(~IFF_UP) },
                                                          { "change", to_string(IFF_UP) }  }  } );
    });
}

VsmStatus Client::vsm_create_netdev_veth(const char* id,
                                    const char* zoneDev,
                                    const char* hostDev) noexcept
{
    assert(id);
    assert(zoneDev);
    assert(hostDev);

    return coverException([&] {
        mHostClient.callCreateNetdevVeth({ id, zoneDev, hostDev });
    });
}

VsmStatus Client::vsm_create_netdev_macvlan(const char* id,
                                       const char* zoneDev,
                                       const char* hostDev,
                                       enum macvlan_mode mode) noexcept
{
    assert(id);
    assert(zoneDev);
    assert(hostDev);

    return coverException([&] {
        mHostClient.callCreateNetdevMacvlan({ id, zoneDev, hostDev, mode });
    });
}

VsmStatus Client::vsm_create_netdev_phys(const char* id, const char* devId) noexcept
{
    assert(id);
    assert(devId);

    return coverException([&] {
        mHostClient.callCreateNetdevPhys({ id, devId });
    });
}

VsmStatus Client::vsm_lookup_netdev_by_name(const char* id,
                                       const char* netdevId,
                                       VsmNetdev* netdev) noexcept
{
    using namespace boost::algorithm;

    assert(id);
    assert(netdevId);
    assert(netdev);

    return coverException([&] {
        api::GetNetDevAttrs attrs;
        mHostClient.callGetNetdevAttrs({ id, netdevId }, attrs);
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

        *netdev = reinterpret_cast<VsmNetdev>(malloc(sizeof(**netdev)));
        (*netdev)->name = ::strdup(id);
        (*netdev)->type = type;
    });
}

VsmStatus Client::vsm_destroy_netdev(const char* id, const char* devId) noexcept
{
    assert(id);
    assert(devId);

    return coverException([&] {
        mHostClient.callDestroyNetdev({ id, devId });
    });
}

VsmStatus Client::vsm_declare_file(const char* id,
                              VsmFileType type,
                              const char *path,
                              int32_t flags,
                              mode_t mode,
                              VsmString* declarationId) noexcept
{
    assert(id);
    assert(path);

    return coverException([&] {
        api::Declaration declaration;
        mHostClient.callDeclareFile({ id, type, path, flags, (int)mode }, declaration);
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
    assert(source);
    assert(id);
    assert(target);
    assert(type);
    if (!data) {
        data = "";
    }

    return coverException([&] {
        api::Declaration declaration;
        mHostClient.callDeclareMount({ source, id, target, type, flags, data }, declaration);
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
    assert(source);
    assert(id);
    assert(target);

    return coverException([&] {
        api::Declaration declaration;
        mHostClient.callDeclareLink({ source, id, target }, declaration);
        if (declarationId != NULL) {
            *declarationId = ::strdup(declaration.value.c_str());
        }
    });
}

VsmStatus Client::vsm_list_declarations(const char* id, VsmArrayString* declarations) noexcept
{
    assert(id);
    assert(declarations);

    return coverException([&] {
        api::Declarations declarationsOut;
        mHostClient.callGetDeclarations({ id }, declarationsOut);
        convert(declarationsOut, *declarations);
    });
}

VsmStatus Client::vsm_remove_declaration(const char* id, VsmString declaration) noexcept
{
    assert(id);
    assert(declaration);

    return coverException([&] {
        mHostClient.callRemoveDeclaration({ id, declaration });
    });
}

VsmStatus Client::vsm_notify_active_zone(const char* /*application*/, const char* /*message*/) noexcept
{
    return coverException([&] {
        //TODO: Implement vsm_notify_active_zone
        throw OperationFailedException("Not implemented");
    });
}

VsmStatus Client::vsm_file_move_request(const char* /*destZone*/, const char* /*path*/) noexcept
{
    return coverException([&] {
        //TODO: Implement vsm_file_move_request
        throw OperationFailedException("Not implemented");
    });
}

VsmStatus Client::vsm_add_notification_callback(VsmNotificationFunction /*notificationCallback*/,
                                           void* /*data*/,
                                           VsmSubscriptionId* /*subscriptionId*/) noexcept
{
    return coverException([&] {
        //TODO: Implement vsm_add_notification_callback
        throw OperationFailedException("Not implemented");
    });
}

VsmStatus Client::vsm_del_notification_callback(VsmSubscriptionId subscriptionId) noexcept
{
    return coverException([&] {
        mHostClient.unsubscribe(subscriptionId);
    });
}

