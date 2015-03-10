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
 * @brief   Implementation of a class for communication with server
 */

#include "config.hpp"

#include "host-connection.hpp"
#include "host-dbus-definitions.hpp"
#include "exception.hpp"
#include "api/dbus-method-result-builder.hpp"
#include "api/messages.hpp"

#include "logger/logger.hpp"


namespace vasum {

namespace {

// Timeout in ms for waiting for dbus name.
// Can happen if glib loop is busy or not present.
// TODO: this should be in host's configuration file
const unsigned int NAME_ACQUIRED_TIMEOUT = 5 * 1000;

} // namespace


HostConnection::HostConnection()
    : mNameAcquired(false)
    , mNameLost(false)
{
    LOGT("Connecting to host system DBUS");
    mDbusConnection = dbus::DbusConnection::createSystem();

    LOGT("Setting DBUS name");
    mDbusConnection->setName(api::host::BUS_NAME,
                             std::bind(&HostConnection::onNameAcquired, this),
                             std::bind(&HostConnection::onNameLost, this));

    if (!waitForName(NAME_ACQUIRED_TIMEOUT)) {
        LOGE("Could not acquire dbus name: " << api::host::BUS_NAME);
        throw HostConnectionException("Could not acquire dbus name: " + api::host::BUS_NAME);
    }

    LOGT("Registering DBUS interface");
    using namespace std::placeholders;
    mDbusConnection->registerObject(api::host::OBJECT_PATH,
                                    api::host::DEFINITION,
                                    std::bind(&HostConnection::onMessageCall,
                                            this, _1, _2, _3, _4, _5));

    LOGD("Connected");
}

HostConnection::~HostConnection()
{
}

bool HostConnection::waitForName(const unsigned int timeoutMs)
{
    std::unique_lock<std::mutex> lock(mNameMutex);
    mNameCondition.wait_for(lock,
                            std::chrono::milliseconds(timeoutMs),
                            [this] {
                                return mNameAcquired || mNameLost;
                            });

    return mNameAcquired;
}

void HostConnection::onNameAcquired()
{
    std::unique_lock<std::mutex> lock(mNameMutex);
    mNameAcquired = true;
    mNameCondition.notify_one();
}

void HostConnection::onNameLost()
{
    std::unique_lock<std::mutex> lock(mNameMutex);
    mNameLost = true;
    mNameCondition.notify_one();

    if (mNameAcquired) {
        // TODO implement reconnecting
        LOGE("TODO Reconnect !!!");
    }
}

void HostConnection::setProxyCallCallback(const ProxyCallCallback& callback)
{
    mProxyCallCallback = callback;
}

void HostConnection::setGetZoneDbusesCallback(const GetZoneDbusesCallback& callback)
{
    mGetZoneDbusesCallback = callback;
}

void HostConnection::setGetZoneIdsCallback(const GetZoneIdsCallback& callback)
{
    mGetZoneIdsCallback = callback;
}

void HostConnection::setGetActiveZoneIdCallback(const GetActiveZoneIdCallback& callback)
{
    mGetActiveZoneIdCallback = callback;
}

void HostConnection::setGetZoneInfoCallback(const GetZoneInfoCallback& callback)
{
    mGetZoneInfoCallback = callback;
}

void HostConnection::setSetNetdevAttrsCallback(const SetNetdevAttrsCallback& callback)
{
    mSetNetdevAttrsCallback = callback;
}

void HostConnection::setGetNetdevAttrsCallback(const GetNetdevAttrsCallback& callback)
{
    mGetNetdevAttrsCallback = callback;
}

void HostConnection::setGetNetdevListCallback(const GetNetdevListCallback& callback)
{
    mGetNetdevListCallback = callback;
}

void HostConnection::setCreateNetdevVethCallback(const CreateNetdevVethCallback& callback)
{
    mCreateNetdevVethCallback = callback;
}

void HostConnection::setCreateNetdevMacvlanCallback(const CreateNetdevMacvlanCallback& callback)
{
    mCreateNetdevMacvlanCallback = callback;
}

void HostConnection::setCreateNetdevPhysCallback(const CreateNetdevPhysCallback& callback)
{
    mCreateNetdevPhysCallback = callback;
}

void HostConnection::setDeclareFileCallback(const DeclareFileCallback& callback)
{
    mDeclareFileCallback = callback;
}

void HostConnection::setDeclareMountCallback(const DeclareMountCallback& callback)
{
    mDeclareMountCallback = callback;
}

void HostConnection::setDeclareLinkCallback(const DeclareLinkCallback& callback)
{
    mDeclareLinkCallback = callback;
}

void HostConnection::setGetDeclarationsCallback(const GetDeclarationsCallback& callback)
{
    mGetDeclarationsCallback = callback;
}

void HostConnection::setRemoveDeclarationCallback(const RemoveDeclarationCallback& callback)
{
    mRemoveDeclarationCallback =  callback;
}

void HostConnection::setSetActiveZoneCallback(const SetActiveZoneCallback& callback)
{
    mSetActiveZoneCallback = callback;
}

void HostConnection::setCreateZoneCallback(const CreateZoneCallback& callback)
{
    mCreateZoneCallback = callback;
}

void HostConnection::setDestroyZoneCallback(const DestroyZoneCallback& callback)
{
    mDestroyZoneCallback = callback;
}

void HostConnection::setShutdownZoneCallback(const ShutdownZoneCallback& callback)
{
    mShutdownZoneCallback = callback;
}

void HostConnection::setStartZoneCallback(const StartZoneCallback& callback)
{
    mStartZoneCallback = callback;
}

void HostConnection::setLockZoneCallback(const LockZoneCallback& callback)
{
    mLockZoneCallback = callback;
}

void HostConnection::setUnlockZoneCallback(const UnlockZoneCallback& callback)
{
    mUnlockZoneCallback = callback;
}

void HostConnection::setGrantDeviceCallback(const GrantDeviceCallback& callback)
{
    mGrantDeviceCallback = callback;
}

void HostConnection::setRevokeDeviceCallback(const RevokeDeviceCallback& callback)
{
    mRevokeDeviceCallback = callback;
}


void HostConnection::onMessageCall(const std::string& objectPath,
                                   const std::string& interface,
                                   const std::string& methodName,
                                   GVariant* parameters,
                                   dbus::MethodResultBuilder::Pointer result)
{
    if (objectPath != api::host::OBJECT_PATH || interface != api::host::INTERFACE) {
        return;
    }

    if (methodName == api::host::METHOD_SET_ACTIVE_ZONE) {
        const gchar* id = NULL;
        g_variant_get(parameters, "(&s)", &id);

        if (mSetActiveZoneCallback) {
            auto rb = std::make_shared<api::DbusMethodResultBuilder<api::Void>>(result);
            mSetActiveZoneCallback(id, rb);
        }
        return;
    }

    if (methodName == api::host::METHOD_GET_ZONE_DBUSES) {
        if (mGetZoneDbusesCallback) {
            auto rb = std::make_shared<api::DbusMethodResultBuilder<api::Dbuses>>(result);
            mGetZoneDbusesCallback(rb);
        }
        return;
    }

    if (methodName == api::METHOD_PROXY_CALL) {
        const gchar* target = NULL;
        const gchar* targetBusName = NULL;
        const gchar* targetObjectPath = NULL;
        const gchar* targetInterface = NULL;
        const gchar* targetMethod = NULL;
        GVariant* rawArgs = NULL;
        g_variant_get(parameters,
                      "(&s&s&s&s&sv)",
                      &target,
                      &targetBusName,
                      &targetObjectPath,
                      &targetInterface,
                      &targetMethod,
                      &rawArgs);
        dbus::GVariantPtr args(rawArgs, g_variant_unref);

        if (mProxyCallCallback) {
            mProxyCallCallback(target,
                               targetBusName,
                               targetObjectPath,
                               targetInterface,
                               targetMethod,
                               args.get(),
                               result);
        }
        return;
    }

    if (methodName == api::host::METHOD_GET_ZONE_ID_LIST) {
        if (mGetZoneIdsCallback) {
            auto rb = std::make_shared<api::DbusMethodResultBuilder<api::ZoneIds>>(result);
            mGetZoneIdsCallback(rb);
        }
        return;
    }

    if (methodName == api::host::METHOD_GET_ACTIVE_ZONE_ID) {
        if (mGetActiveZoneIdCallback) {
            auto rb = std::make_shared<api::DbusMethodResultBuilder<api::ZoneId>>(result);
            mGetActiveZoneIdCallback(rb);
        }
        return;
    }

    if (methodName == api::host::METHOD_GET_ZONE_INFO) {
        const gchar* id = NULL;
        g_variant_get(parameters, "(&s)", &id);

        if (mGetZoneInfoCallback) {
            auto rb = std::make_shared<api::DbusMethodResultBuilder<api::ZoneInfo>>(result);
            mGetZoneInfoCallback(id, rb);
        }
        return;
    }

    if (methodName == api::host::METHOD_SET_NETDEV_ATTRS) {
        const gchar* zone = NULL;
        const gchar* netdev = NULL;
        GVariantIter* iter;
        g_variant_get(parameters, "(&s&sa(ss))", &zone, &netdev, &iter);
        gchar* key = NULL;
        gchar* value = NULL;
        std::vector<std::tuple<std::string, std::string>> attrs;
        while (g_variant_iter_loop(iter, "(&s&s)", &key, &value)) {
            attrs.push_back(std::make_tuple<std::string, std::string>(key, value));
        }
        g_variant_iter_free(iter);
        if (mSetNetdevAttrsCallback) {
            auto rb = std::make_shared<api::DbusMethodResultBuilder<api::Void>>(result);
            mSetNetdevAttrsCallback(zone, netdev, attrs, rb);
        }
        return;
    }

    if (methodName == api::host::METHOD_GET_NETDEV_ATTRS) {
        const gchar* zone = NULL;
        const gchar* netdev = NULL;
        g_variant_get(parameters, "(&s&s)", &zone, &netdev);
        if (mGetNetdevAttrsCallback) {
            auto rb = std::make_shared<api::DbusMethodResultBuilder<api::NetDevAttrs>>(result);
            mGetNetdevAttrsCallback(zone, netdev, rb);
        }
        return;
    }

    if (methodName == api::host::METHOD_GET_NETDEV_LIST) {
        const gchar* zone = NULL;
        g_variant_get(parameters, "(&s)", &zone);
        if (mGetNetdevListCallback) {
            auto rb = std::make_shared<api::DbusMethodResultBuilder<api::NetDevList>>(result);
            mGetNetdevListCallback(zone, rb);
        }
        return;
    }


    if (methodName == api::host::METHOD_CREATE_NETDEV_VETH) {
        const gchar* id = NULL;
        const gchar* zoneDev = NULL;
        const gchar* hostDev = NULL;
        g_variant_get(parameters, "(&s&s&s)", &id, &zoneDev, &hostDev);
        if (mCreateNetdevVethCallback) {
            auto rb = std::make_shared<api::DbusMethodResultBuilder<api::Void>>(result);
            mCreateNetdevVethCallback(id, zoneDev, hostDev, rb);
        }
        return;
    }

    if (methodName == api::host::METHOD_CREATE_NETDEV_MACVLAN) {
        const gchar* id = NULL;
        const gchar* zoneDev = NULL;
        const gchar* hostDev = NULL;
        guint32 mode;
        g_variant_get(parameters, "(&s&s&su)", &id, &zoneDev, &hostDev, &mode);
        if (mCreateNetdevMacvlanCallback) {
            auto rb = std::make_shared<api::DbusMethodResultBuilder<api::Void>>(result);
            mCreateNetdevMacvlanCallback(id, zoneDev, hostDev, mode, rb);
        }
    }

    if (methodName == api::host::METHOD_CREATE_NETDEV_PHYS) {
        const gchar* id = NULL;
        const gchar* devId = NULL;
        g_variant_get(parameters, "(&s&s)", &id, &devId);
        if (mCreateNetdevPhysCallback) {
            auto rb = std::make_shared<api::DbusMethodResultBuilder<api::Void>>(result);
            mCreateNetdevPhysCallback(id, devId, rb);
        }
    }

    if (methodName == api::host::METHOD_DECLARE_FILE) {
        const gchar* zone;
        int32_t type;
        const gchar* path;
        int32_t flags;
        int32_t mode;
        g_variant_get(parameters, "(&si&sii)", &zone, &type, &path, &flags, &mode);

        if (mDeclareFileCallback) {
            auto rb = std::make_shared<api::DbusMethodResultBuilder<api::Declaration>>(result);
            mDeclareFileCallback(zone, type, path, flags, mode, rb);
        }
        return;
    }

    if (methodName == api::host::METHOD_DECLARE_MOUNT) {
        const gchar* source;
        const gchar* zone;
        const gchar* target;
        const gchar* type;
        uint64_t flags;
        const gchar* data;
        g_variant_get(parameters,
                      "(&s&s&s&st&s)",
                      &source,
                      &zone,
                      &target,
                      &type,
                      &flags,
                      &data);

        if (mDeclareMountCallback) {
            auto rb = std::make_shared<api::DbusMethodResultBuilder<api::Declaration>>(result);
            mDeclareMountCallback(source, zone, target, type, flags, data, rb);
        }
        return;
    }

    if (methodName == api::host::METHOD_DECLARE_LINK) {
        const gchar* source;
        const gchar* zone;
        const gchar* target;
        g_variant_get(parameters, "(&s&s&s)", &source, &zone, &target);

        if (mDeclareLinkCallback) {
            auto rb = std::make_shared<api::DbusMethodResultBuilder<api::Declaration>>(result);
            mDeclareLinkCallback(source, zone, target, rb);
        }
        return;
    }

    if (methodName == api::host::METHOD_GET_DECLARATIONS) {
        const gchar* zone;
        g_variant_get(parameters, "(&s)", &zone);

        if (mGetDeclarationsCallback) {
            auto rb = std::make_shared<api::DbusMethodResultBuilder<api::Declarations>>(result);
            mGetDeclarationsCallback(zone, rb);
        }
        return;
    }

    if (methodName == api::host::METHOD_REMOVE_DECLARATION) {
        const gchar* zone;
        const gchar* declarationId;
        g_variant_get(parameters, "(&s&s)", &zone, &declarationId);

        if (mRemoveDeclarationCallback) {
            auto rb = std::make_shared<api::DbusMethodResultBuilder<api::Void>>(result);
            mRemoveDeclarationCallback(zone, declarationId, rb);
        }
        return;
    }

    if (methodName == api::host::METHOD_CREATE_ZONE) {
        const gchar* id = NULL;
        const gchar* templateName = NULL;
        g_variant_get(parameters, "(&s&s)", &id, &templateName);

        if (mCreateZoneCallback) {
            auto rb = std::make_shared<api::DbusMethodResultBuilder<api::Void>>(result);
            mCreateZoneCallback(id, templateName, rb);
        }
        return;
    }

    if (methodName == api::host::METHOD_DESTROY_ZONE) {
        const gchar* id = NULL;
        g_variant_get(parameters, "(&s)", &id);

        if (mDestroyZoneCallback) {
            auto rb = std::make_shared<api::DbusMethodResultBuilder<api::Void>>(result);
            mDestroyZoneCallback(id, rb);
        }
        return;
    }

    if (methodName == api::host::METHOD_SHUTDOWN_ZONE) {
        const gchar* id = NULL;
        g_variant_get(parameters, "(&s)", &id);

        if (mShutdownZoneCallback) {
            auto rb = std::make_shared<api::DbusMethodResultBuilder<api::Void>>(result);
            mShutdownZoneCallback(id, rb);
        }
    }

    if (methodName == api::host::METHOD_START_ZONE) {
        const gchar* id = NULL;
        g_variant_get(parameters, "(&s)", &id);

        if (mStartZoneCallback) {
            auto rb = std::make_shared<api::DbusMethodResultBuilder<api::Void>>(result);
            mStartZoneCallback(id, rb);
        }
    }

    if (methodName == api::host::METHOD_LOCK_ZONE) {
        const gchar* id = NULL;
        g_variant_get(parameters, "(&s)", &id);

        if (mLockZoneCallback) {
            auto rb = std::make_shared<api::DbusMethodResultBuilder<api::Void>>(result);
            mLockZoneCallback(id, rb);
        }
        return;
    }

    if (methodName == api::host::METHOD_UNLOCK_ZONE) {
        const gchar* id = NULL;
        g_variant_get(parameters, "(&s)", &id);

        if (mUnlockZoneCallback) {
            auto rb = std::make_shared<api::DbusMethodResultBuilder<api::Void>>(result);
            mUnlockZoneCallback(id, rb);
        }
        return;
    }

    if (methodName == api::host::METHOD_GRANT_DEVICE) {
        const gchar* id = NULL;
        const gchar* device = NULL;
        uint32_t flags;
        g_variant_get(parameters, "(&s&su)", &id, &device, &flags);

        if (mGrantDeviceCallback) {
            auto rb = std::make_shared<api::DbusMethodResultBuilder<api::Void>>(result);
            mGrantDeviceCallback(id, device, flags, rb);
        }
        return;
    }

    if (methodName == api::host::METHOD_REVOKE_DEVICE) {
        const gchar* id = NULL;
        const gchar* device = NULL;
        g_variant_get(parameters, "(&s&s)", &id, &device);

        if (mRevokeDeviceCallback) {
            auto rb = std::make_shared<api::DbusMethodResultBuilder<api::Void>>(result);
            mRevokeDeviceCallback(id, device, rb);
        }
        return;
    }
}

void HostConnection::proxyCallAsync(const std::string& busName,
                                    const std::string& objectPath,
                                    const std::string& interface,
                                    const std::string& method,
                                    GVariant* parameters,
                                    const dbus::DbusConnection::AsyncMethodCallCallback& callback)
{
    mDbusConnection->callMethodAsync(busName,
                                     objectPath,
                                     interface,
                                     method,
                                     parameters,
                                     std::string(),
                                     callback);
}

void HostConnection::signalZoneDbusState(const std::string& zoneId,
                                         const std::string& dbusAddress)
{
    GVariant* parameters = g_variant_new("(ss)", zoneId.c_str(), dbusAddress.c_str());
    mDbusConnection->emitSignal(api::host::OBJECT_PATH,
                                api::host::INTERFACE,
                                api::host::SIGNAL_ZONE_DBUS_STATE,
                                parameters);
}


} // namespace vasum
