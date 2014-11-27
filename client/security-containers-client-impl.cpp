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
 * @brief   This file contains security-containers-server's client implementation
 */

#include <config.hpp>
#include "security-containers-client-impl.hpp"
#include "utils.hpp"
#include <dbus/connection.hpp>
#include <dbus/exception.hpp>
#include <utils/glib-loop.hpp>
#include <host-dbus-definitions.hpp>
#include <container-dbus-definitions.hpp>

#include <cstring>
#include <cassert>
#include <fstream>

using namespace std;
using namespace dbus;
using namespace security_containers;
using namespace security_containers::utils;

namespace {

const DbusInterfaceInfo HOST_INTERFACE(api::host::BUS_NAME,
                                       api::host::OBJECT_PATH,
                                       api::host::INTERFACE);
const DbusInterfaceInfo CONTAINER_INTERFACE(api::container::BUS_NAME,
                                            api::container::OBJECT_PATH,
                                            api::container::INTERFACE);

unique_ptr<ScopedGlibLoop> gGlibLoop;

void toDict(GVariant* in, VsmArrayString* keys, VsmArrayString* values)
{
    assert(in);
    assert(keys);
    assert(values);

    typedef char* key_type;
    typedef char* value_type;

    GVariantIter iter;
    value_type value;
    key_type key;
    gsize size = g_variant_n_children(in);
    key_type* outk = (key_type*)calloc(size + 1, sizeof(key_type));
    value_type* outv = (value_type*)calloc(size + 1, sizeof(value_type));

    g_variant_iter_init(&iter, in);
    for (int i = 0; g_variant_iter_loop(&iter, "{ss}", &key, &value); i++) {
        outk[i] = strdup(key);
        outv[i] = strdup(value);
    }
    *keys = outk;
    *values = outv;
}

void toBasic(GVariant* in, char** str)
{
    assert(in);
    assert(str);

    gsize length;
    const gchar* src = g_variant_get_string(in, &length);
    char* buf = strndup(src, length);
    *str = buf;
}

VsmDomainState getDomainState(const char* state)
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
    assert(!"UNKNOWN STATE");
    return (VsmDomainState)-1;
}

void toBasic(GVariant* in, VsmDomain* domain)
{
    const char* id;
    const char* path;
    const char* state;
    int terminal;
    VsmDomain vsmDomain = (VsmDomain)malloc(sizeof(*vsmDomain));
    g_variant_get(in, "(siss)", &id, &terminal, &state, &path);
    vsmDomain->id = strdup(id);
    vsmDomain->terminal = terminal;
    vsmDomain->state = getDomainState(state);
    vsmDomain->rootfs_path = strdup(path);
    *domain = vsmDomain;
}

template<typename T>
void toArray(GVariant* in, T** scArray)
{
    assert(in);
    assert(scArray);

    gsize size = g_variant_n_children(in);
    T* ids = (T*)calloc(size + 1, sizeof(T));

    GVariantIter iter;
    GVariant* child;

    g_variant_iter_init(&iter, in);
    for (int i = 0; (child = g_variant_iter_next_value(&iter)); i++) {
        toBasic(child, &ids[i]);
        g_variant_unref(child);
    }
    *scArray = ids;
}

VsmStatus toStatus(const std::exception& ex)
{
    if (typeid(DbusCustomException) == typeid(ex)) {
        return VSMCLIENT_CUSTOM_ERROR;
    } else if (typeid(DbusIOException) == typeid(ex)) {
        return VSMCLIENT_IO_ERROR;
    } else if (typeid(DbusOperationException) == typeid(ex)) {
        return VSMCLIENT_OPERATION_FAILED;
    } else if (typeid(DbusInvalidArgumentException) == typeid(ex)) {
        return VSMCLIENT_INVALID_ARGUMENT;
    } else if (typeid(DbusException) == typeid(ex)) {
        return VSMCLIENT_OTHER_ERROR;
    }
    return VSMCLIENT_OTHER_ERROR;
}

bool readFirstLineOfFile(const std::string& path, std::string& ret)
{
    std::ifstream file(path);
    if (!file) {
        return false;
    }

    std::getline(file, ret);
    return true;
}

} //namespace

VsmStatus Client::vsm_start_glib_loop() noexcept
{
    try {
        if (!gGlibLoop) {
            gGlibLoop.reset(new ScopedGlibLoop());
        }
    } catch (const exception&) {
        return VSMCLIENT_OTHER_ERROR;
    }

    return VSMCLIENT_SUCCESS;
}

VsmStatus Client::vsm_stop_glib_loop() noexcept
{
    try {
        gGlibLoop.reset();
    } catch (const exception&) {
        return VSMCLIENT_OTHER_ERROR;
    }
    return VSMCLIENT_SUCCESS;
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

VsmStatus Client::createSystem() noexcept
{
    try {
        mConnection = DbusConnection::createSystem();
        mStatus = Status();
    } catch (const exception& ex) {
        mStatus = Status(toStatus(ex), ex.what());
    }
    return vsm_get_status();
}

VsmStatus Client::create(const string& address) noexcept
{
    try {
        mConnection = DbusConnection::create(address);
        mStatus = Status();
    } catch (const exception& ex) {
        mStatus = Status(toStatus(ex), ex.what());
    }
    return vsm_get_status();
}

VsmStatus Client::callMethod(const DbusInterfaceInfo& info,
                             const string& method,
                             GVariant* args_in,
                             const string& args_spec_out,
                             GVariant** args_out)
{
    try {
        GVariantPtr ret = mConnection->callMethod(info.busName,
                                                  info.objectPath,
                                                  info.interface,
                                                  method,
                                                  args_in,
                                                  args_spec_out);
        if (args_out != NULL) {
            *args_out = ret.release();
        }
        mStatus = Status();
    } catch (const exception& ex)  {
        mStatus = Status(toStatus(ex), ex.what());
    }
    return vsm_get_status();
}

VsmStatus Client::signalSubscribe(const DbusInterfaceInfo& info,
                                  const string& name,
                                  SignalCallback signalCallback,
                                  VsmSubscriptionId* subscriptionId)
{
    auto onSignal = [=](const std::string& /*senderBusName*/,
                        const std::string & objectPath,
                        const std::string & interface,
                        const std::string & signalName,
                        GVariant * parameters) {
        if (objectPath == info.objectPath &&
            interface == info.interface &&
            signalName == name) {

            signalCallback(parameters);
        }
    };
    try {
        guint id = mConnection->signalSubscribe(onSignal, info.busName);
        if (subscriptionId) {
            *subscriptionId = id;
        }
        mStatus = Status();
    } catch (const std::exception& ex) {
        mStatus = Status(toStatus(ex), ex.what());
    }
    return vsm_get_status();
}

VsmStatus Client::signalUnsubscribe(VsmSubscriptionId id)
{
    try {
        mConnection->signalUnsubscribe(id);
        mStatus = Status();
    } catch (const std::exception& ex) {
        mStatus = Status(toStatus(ex), ex.what());
    }
    return vsm_get_status();
}

const char* Client::vsm_get_status_message() noexcept
{
    return mStatus.mMsg.c_str();
}

VsmStatus Client::vsm_get_status() noexcept
{
    return mStatus.mVsmStatus;
}

VsmStatus Client::vsm_get_container_dbuses(VsmArrayString* keys, VsmArrayString* values) noexcept
{
    assert(keys);
    assert(values);

    GVariant* out;
    VsmStatus ret = callMethod(HOST_INTERFACE,
                               api::host::METHOD_GET_CONTAINER_DBUSES,
                               NULL,
                               "(a{ss})",
                               &out);
    if (ret != VSMCLIENT_SUCCESS) {
        return ret;
    }
    GVariant* unpacked;
    g_variant_get(out, "(*)", &unpacked);
    toDict(unpacked, keys, values);
    g_variant_unref(unpacked);
    g_variant_unref(out);
    return ret;
}

VsmStatus Client::vsm_get_domain_ids(VsmArrayString* array) noexcept
{
    assert(array);

    GVariant* out;
    VsmStatus ret = callMethod(HOST_INTERFACE,
                               api::host::METHOD_GET_CONTAINER_ID_LIST,
                               NULL,
                               "(as)",
                               &out);
    if (ret != VSMCLIENT_SUCCESS) {
        return ret;
    }
    GVariant* unpacked;
    g_variant_get(out, "(*)", &unpacked);
    toArray(unpacked, array);
    g_variant_unref(unpacked);
    g_variant_unref(out);
    return ret;
}

VsmStatus Client::vsm_get_active_container_id(VsmString* id) noexcept
{
    assert(id);

    GVariant* out;
    VsmStatus ret = callMethod(HOST_INTERFACE,
                               api::host::METHOD_GET_ACTIVE_CONTAINER_ID,
                               NULL,
                               "(s)",
                               &out);
    if (ret != VSMCLIENT_SUCCESS) {
        return ret;
    }
    GVariant* unpacked;
    g_variant_get(out, "(*)", &unpacked);
    toBasic(unpacked, id);
    g_variant_unref(unpacked);
    g_variant_unref(out);
    return ret;
}

VsmStatus Client::vsm_lookup_domain_by_pid(int pid, VsmString* id) noexcept
{
    assert(id);

    const std::string path = "/proc/" + std::to_string(pid) + "/cpuset";

    std::string cpuset;
    if (!readFirstLineOfFile(path, cpuset)) {
        mStatus = Status(VSMCLIENT_INVALID_ARGUMENT, "Process not found");
        return vsm_get_status();
    }

    std::string containerId;
    if (!parseContainerIdFromCpuSet(cpuset, containerId)) {
        mStatus = Status(VSMCLIENT_OTHER_ERROR, "unknown format of cpuset");
        return vsm_get_status();
    }

    *id = strdup(containerId.c_str());
    mStatus = Status();
    return vsm_get_status();
}

VsmStatus Client::vsm_lookup_domain_by_id(const char* id, VsmDomain* domain) noexcept
{
    assert(id);
    assert(domain);

    GVariant* out;
    GVariant* args_in = g_variant_new("(s)", id);
    VsmStatus ret = callMethod(HOST_INTERFACE,
                               api::host::METHOD_GET_CONTAINER_INFO,
                               args_in,
                               "((siss))",
                               &out);
    if (ret != VSMCLIENT_SUCCESS) {
        return ret;
    }
    GVariant* unpacked;
    g_variant_get(out, "(*)", &unpacked);
    toBasic(unpacked, domain);
    g_variant_unref(unpacked);
    g_variant_unref(out);
    return ret;
}

VsmStatus Client::vsm_lookup_domain_by_terminal_id(int, VsmString*) noexcept
{
    mStatus = Status(VSMCLIENT_OTHER_ERROR, "Not implemented");
    return vsm_get_status();
}

VsmStatus Client::vsm_set_active_container(const char* id) noexcept
{
    assert(id);

    GVariant* args_in = g_variant_new("(s)", id);
    return callMethod(HOST_INTERFACE, api::host::METHOD_SET_ACTIVE_CONTAINER, args_in);
}

VsmStatus Client::vsm_create_domain(const char* id, const char* tname) noexcept
{
    assert(id);
    if (tname) {
        mStatus = Status(VSMCLIENT_OTHER_ERROR, "Named template isn't implemented");
        return vsm_get_status();
    }

    GVariant* args_in = g_variant_new("(s)", id);
    return callMethod(HOST_INTERFACE, api::host::METHOD_CREATE_CONTAINER, args_in);
}

VsmStatus Client::vsm_destroy_domain(const char* id) noexcept
{
    assert(id);
    GVariant* args_in = g_variant_new("(s)", id);
    return callMethod(HOST_INTERFACE, api::host::METHOD_DESTROY_CONTAINER, args_in);
}

VsmStatus Client::vsm_shutdown_domain(const char*) noexcept
{
    mStatus = Status(VSMCLIENT_OTHER_ERROR, "Not implemented");
    return vsm_get_status();
}

VsmStatus Client::vsm_start_domain(const char*) noexcept
{
    mStatus = Status(VSMCLIENT_OTHER_ERROR, "Not implemented");
    return vsm_get_status();
}

VsmStatus Client::vsm_lock_domain(const char*) noexcept
{
    mStatus = Status(VSMCLIENT_OTHER_ERROR, "Not implemented");
    return vsm_get_status();
}

VsmStatus Client::vsm_unlock_domain(const char*) noexcept
{
    mStatus = Status(VSMCLIENT_OTHER_ERROR, "Not implemented");
    return vsm_get_status();
}

VsmStatus Client::vsm_add_state_callback(VsmContainerDbusStateCallback containerDbusStateCallback,
                                         void* data,
                                         VsmSubscriptionId* subscriptionId) noexcept
{
    assert(containerDbusStateCallback);

    auto onSigal = [=](GVariant * parameters)
    {
        const char* container;
        const char* dbusAddress;
        g_variant_get(parameters, "(&s&s)", &container, &dbusAddress);
        containerDbusStateCallback(container, dbusAddress, data);
    };

    return signalSubscribe(HOST_INTERFACE,
                           api::host::SIGNAL_CONTAINER_DBUS_STATE,
                           onSigal,
                           subscriptionId);
}

VsmStatus Client::vsm_del_state_callback(VsmSubscriptionId subscriptionId) noexcept
{
    return signalUnsubscribe(subscriptionId);
}

VsmStatus Client::vsm_domain_grant_device(const char*, const char*, uint32_t) noexcept
{
    mStatus = Status(VSMCLIENT_OTHER_ERROR, "Not implemented");
    return vsm_get_status();
}

VsmStatus Client::vsm_revoke_device(const char*, const char*) noexcept
{
    mStatus = Status(VSMCLIENT_OTHER_ERROR, "Not implemented");
    return vsm_get_status();
}

VsmStatus Client::vsm_domain_get_netdevs(const char*, VsmArrayString*) noexcept
{
    mStatus = Status(VSMCLIENT_OTHER_ERROR, "Not implemented");
    return vsm_get_status();
}

VsmStatus Client::vsm_netdev_get_ipv4_addr(const char*, const char*, struct in_addr*) noexcept
{
    mStatus = Status(VSMCLIENT_OTHER_ERROR, "Not implemented");
    return vsm_get_status();
}

VsmStatus Client::vsm_netdev_get_ipv6_addr(const char*, const char*, struct in6_addr*) noexcept
{
    mStatus = Status(VSMCLIENT_OTHER_ERROR, "Not implemented");
    return vsm_get_status();
}

VsmStatus Client::vsm_netdev_set_ipv4_addr(const char*, const char*, struct in_addr*, int) noexcept
{
    mStatus = Status(VSMCLIENT_OTHER_ERROR, "Not implemented");
    return vsm_get_status();
}

VsmStatus Client::vsm_netdev_set_ipv6_addr(const char*, const char*, struct in6_addr*, int) noexcept
{
    mStatus = Status(VSMCLIENT_OTHER_ERROR, "Not implemented");
    return vsm_get_status();
}

VsmStatus Client::vsm_create_netdev(const char*, VsmNetdevType, const char*, const char*) noexcept
{
    mStatus = Status(VSMCLIENT_OTHER_ERROR, "Not implemented");
    return vsm_get_status();
}

VsmStatus Client::vsm_destroy_netdev(const char*, const char*) noexcept
{
    mStatus = Status(VSMCLIENT_OTHER_ERROR, "Not implemented");
    return vsm_get_status();
}

VsmStatus Client::vsm_lookup_netdev_by_name(const char*, const char*, VsmNetdev*) noexcept
{
    mStatus = Status(VSMCLIENT_OTHER_ERROR, "Not implemented");
    return vsm_get_status();
}

VsmStatus Client::vsm_notify_active_container(const char* application, const char* message) noexcept
{
    assert(application);
    assert(message);

    GVariant* args_in = g_variant_new("(ss)", application, message);
    return callMethod(CONTAINER_INTERFACE,
                      api::container::METHOD_NOTIFY_ACTIVE_CONTAINER,
                      args_in);
}

VsmStatus Client::vsm_file_move_request(const char* destContainer, const char* path) noexcept
{
    assert(destContainer);
    assert(path);

    GVariant* out;
    GVariant* args_in = g_variant_new("(ss)", destContainer, path);
    VsmStatus ret = callMethod(CONTAINER_INTERFACE,
                               api::container::METHOD_FILE_MOVE_REQUEST,
                               args_in,
                               "(s)",
                               &out);

    if (ret != VSMCLIENT_SUCCESS) {
        return ret;
    }
    const gchar* retcode = NULL;;
    g_variant_get(out, "(&s)", &retcode);
    if (strcmp(retcode, api::container::FILE_MOVE_SUCCEEDED.c_str()) != 0) {
        mStatus = Status(VSMCLIENT_CUSTOM_ERROR, retcode);
        g_variant_unref(out);
        return vsm_get_status();
    }
    g_variant_unref(out);
    return ret;
}

VsmStatus Client::vsm_add_notification_callback(VsmNotificationCallback notificationCallback,
                                                void* data,
                                                VsmSubscriptionId* subscriptionId) noexcept
{
    assert(notificationCallback);

    auto onSigal = [=](GVariant * parameters) {
        const char* container;
        const char* application;
        const char* message;
        g_variant_get(parameters, "(&s&s&s)", &container, &application, &message);
        notificationCallback(container, application, message, data);
    };

    return signalSubscribe(CONTAINER_INTERFACE,
                           api::container::SIGNAL_NOTIFICATION,
                           onSigal,
                           subscriptionId);
}

VsmStatus Client::vsm_del_notification_callback(VsmSubscriptionId subscriptionId) noexcept
{
    return signalUnsubscribe(subscriptionId);
}
