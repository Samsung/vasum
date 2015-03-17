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
#include "config/manager.hpp"

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

void HostConnection::setDestroyNetdevCallback(const DestroyNetdevCallback& callback)
{
    mDestroyNetdevCallback = callback;
}

void HostConnection::setDeleleNetdevIpAddressCallback(const DeleteNetdevIpAddressCallback& callback)
{
    mDeleteNetdevIpAddressCallback = callback;
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
        api::ZoneId zoneId;
        config::loadFromGVariant(parameters, zoneId);

        if (mSetActiveZoneCallback) {
            auto rb = std::make_shared<api::DbusMethodResultBuilder<api::Void>>(result);
            mSetActiveZoneCallback(zoneId, rb);
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
        api::ZoneId zoneId;
        config::loadFromGVariant(parameters, zoneId);

        if (mGetZoneInfoCallback) {
            auto rb = std::make_shared<api::DbusMethodResultBuilder<api::ZoneInfoOut>>(result);
            mGetZoneInfoCallback(zoneId, rb);
        }
        return;
    }

    if (methodName == api::host::METHOD_SET_NETDEV_ATTRS) {
        api::SetNetDevAttrsIn data;
        config::loadFromGVariant(parameters, data);

        if (mSetNetdevAttrsCallback) {
            auto rb = std::make_shared<api::DbusMethodResultBuilder<api::Void>>(result);
            mSetNetdevAttrsCallback(data, rb);
        }
        return;
    }

    if (methodName == api::host::METHOD_GET_NETDEV_ATTRS) {
        api::GetNetDevAttrsIn data;
        config::loadFromGVariant(parameters, data);

        if (mGetNetdevAttrsCallback) {
            auto rb = std::make_shared<api::DbusMethodResultBuilder<api::GetNetDevAttrs>>(result);
            mGetNetdevAttrsCallback(data, rb);
        }
        return;
    }

    if (methodName == api::host::METHOD_GET_NETDEV_LIST) {
        api::ZoneId data;
        config::loadFromGVariant(parameters, data);

        if (mGetNetdevListCallback) {
            auto rb = std::make_shared<api::DbusMethodResultBuilder<api::GetNetDevAttrs>>(result);
            mGetNetdevListCallback(data, rb);
        }
        return;
    }

    if (methodName == api::host::METHOD_CREATE_NETDEV_VETH) {
        api::CreateNetDevVethIn data;
        config::loadFromGVariant(parameters, data);

        if (mCreateNetdevVethCallback) {
            auto rb = std::make_shared<api::DbusMethodResultBuilder<api::Void>>(result);
            mCreateNetdevVethCallback(data, rb);
        }
        return;
    }

    if (methodName == api::host::METHOD_CREATE_NETDEV_MACVLAN) {
        api::CreateNetDevMacvlanIn data;
        config::loadFromGVariant(parameters, data);

        if (mCreateNetdevMacvlanCallback) {
            auto rb = std::make_shared<api::DbusMethodResultBuilder<api::Void>>(result);
            mCreateNetdevMacvlanCallback(data, rb);
        }
    }

    if (methodName == api::host::METHOD_CREATE_NETDEV_PHYS) {
        api::CreateNetDevPhysIn data;
        config::loadFromGVariant(parameters, data);

        if (mCreateNetdevPhysCallback) {
            auto rb = std::make_shared<api::DbusMethodResultBuilder<api::Void>>(result);
            mCreateNetdevPhysCallback(data, rb);
        }
    }

    if (methodName == api::host::METHOD_DESTROY_NETDEV) {
        api::DestroyNetDevIn data;
        config::loadFromGVariant(parameters, data);

        if (mDestroyNetdevCallback) {
            auto rb = std::make_shared<api::DbusMethodResultBuilder<api::Void>>(result);
            mDestroyNetdevCallback(data, rb);
        }
    }

    if (methodName == api::host::METHOD_DELETE_NETDEV_IP_ADDRESS) {
        api::DeleteNetdevIpAddressIn data;
        config::loadFromGVariant(parameters, data);
        if (mDeleteNetdevIpAddressCallback) {
            auto rb = std::make_shared<api::DbusMethodResultBuilder<api::Void>>(result);
            mDeleteNetdevIpAddressCallback(data, rb);
        }
    }

    if (methodName == api::host::METHOD_DECLARE_FILE) {
        api::DeclareFileIn data;
        config::loadFromGVariant(parameters, data);

        if (mDeclareFileCallback) {
            auto rb = std::make_shared<api::DbusMethodResultBuilder<api::Declaration>>(result);
            mDeclareFileCallback(data, rb);
        }
        return;
    }

    if (methodName == api::host::METHOD_DECLARE_MOUNT) {
        api::DeclareMountIn data;
        config::loadFromGVariant(parameters, data);

        if (mDeclareMountCallback) {
            auto rb = std::make_shared<api::DbusMethodResultBuilder<api::Declaration>>(result);
            mDeclareMountCallback(data, rb);
        }
        return;
    }

    if (methodName == api::host::METHOD_DECLARE_LINK) {
        api::DeclareLinkIn data;
        config::loadFromGVariant(parameters, data);

        if (mDeclareLinkCallback) {
            auto rb = std::make_shared<api::DbusMethodResultBuilder<api::Declaration>>(result);
            mDeclareLinkCallback(data, rb);
        }
        return;
    }

    if (methodName == api::host::METHOD_GET_DECLARATIONS) {
        api::ZoneId data;
        config::loadFromGVariant(parameters, data);

        if (mGetDeclarationsCallback) {
            auto rb = std::make_shared<api::DbusMethodResultBuilder<api::Declarations>>(result);
            mGetDeclarationsCallback(data, rb);
        }
        return;
    }

    if (methodName == api::host::METHOD_REMOVE_DECLARATION) {
        api::RemoveDeclarationIn data;
        config::loadFromGVariant(parameters, data);

        if (mRemoveDeclarationCallback) {
            auto rb = std::make_shared<api::DbusMethodResultBuilder<api::Void>>(result);
            mRemoveDeclarationCallback(data, rb);
        }
        return;
    }

    if (methodName == api::host::METHOD_CREATE_ZONE) {
        api::CreateZoneIn data;
        config::loadFromGVariant(parameters, data);

        if (mCreateZoneCallback) {
            auto rb = std::make_shared<api::DbusMethodResultBuilder<api::Void>>(result);
            mCreateZoneCallback(data, rb);
        }
        return;
    }

    if (methodName == api::host::METHOD_DESTROY_ZONE) {
        api::ZoneId data;
        config::loadFromGVariant(parameters, data);

        if (mDestroyZoneCallback) {
            auto rb = std::make_shared<api::DbusMethodResultBuilder<api::Void>>(result);
            mDestroyZoneCallback(data, rb);
        }
        return;
    }

    if (methodName == api::host::METHOD_SHUTDOWN_ZONE) {
        api::ZoneId data;
        config::loadFromGVariant(parameters, data);

        if (mShutdownZoneCallback) {
            auto rb = std::make_shared<api::DbusMethodResultBuilder<api::Void>>(result);
            mShutdownZoneCallback(data, rb);
        }
    }

    if (methodName == api::host::METHOD_START_ZONE) {
        api::ZoneId data;
        config::loadFromGVariant(parameters, data);

        if (mStartZoneCallback) {
            auto rb = std::make_shared<api::DbusMethodResultBuilder<api::Void>>(result);
            mStartZoneCallback(data, rb);
        }
    }

    if (methodName == api::host::METHOD_LOCK_ZONE) {
        api::ZoneId data;
        config::loadFromGVariant(parameters, data);

        if (mLockZoneCallback) {
            auto rb = std::make_shared<api::DbusMethodResultBuilder<api::Void>>(result);
            mLockZoneCallback(data, rb);
        }
        return;
    }

    if (methodName == api::host::METHOD_UNLOCK_ZONE) {
        api::ZoneId data;
        config::loadFromGVariant(parameters, data);

        if (mUnlockZoneCallback) {
            auto rb = std::make_shared<api::DbusMethodResultBuilder<api::Void>>(result);
            mUnlockZoneCallback(data, rb);
        }
        return;
    }

    if (methodName == api::host::METHOD_GRANT_DEVICE) {
        api::GrantDeviceIn data;
        config::loadFromGVariant(parameters, data);

        if (mGrantDeviceCallback) {
            auto rb = std::make_shared<api::DbusMethodResultBuilder<api::Void>>(result);
            mGrantDeviceCallback(data, rb);
        }
        return;
    }

    if (methodName == api::host::METHOD_REVOKE_DEVICE) {
        api::RevokeDeviceIn data;
        config::loadFromGVariant(parameters, data);

        if (mRevokeDeviceCallback) {
            auto rb = std::make_shared<api::DbusMethodResultBuilder<api::Void>>(result);
            mRevokeDeviceCallback(data, rb);
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
