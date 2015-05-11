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

#include "host-dbus-connection.hpp"
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


HostDbusConnection::HostDbusConnection()
    : mNameAcquired(false)
    , mNameLost(false)
{
    LOGT("Connecting to host system DBUS");
    mDbusConnection = dbus::DbusConnection::createSystem();

    LOGT("Setting DBUS name");
    mDbusConnection->setName(api::dbus::BUS_NAME,
                             std::bind(&HostDbusConnection::onNameAcquired, this),
                             std::bind(&HostDbusConnection::onNameLost, this));

    if (!waitForName(NAME_ACQUIRED_TIMEOUT)) {
        LOGE("Could not acquire dbus name: " << api::dbus::BUS_NAME);
        throw HostConnectionException("Could not acquire dbus name: " + api::dbus::BUS_NAME);
    }

    LOGT("Registering DBUS interface");
    using namespace std::placeholders;
    mDbusConnection->registerObject(api::dbus::OBJECT_PATH,
                                    api::dbus::DEFINITION,
                                    std::bind(&HostDbusConnection::onMessageCall,
                                              this, _1, _2, _3, _4, _5));

    mSubscriptionId = mDbusConnection->signalSubscribe(std::bind(&HostDbusConnection::onSignalCall,
                                                                 this, _1, _2, _3, _4, _5),
                                                       std::string(),
                                                       api::dbus::INTERFACE);
    LOGD("Connected");
}

HostDbusConnection::~HostDbusConnection()
{
    mDbusConnection->signalUnsubscribe(mSubscriptionId);
}

bool HostDbusConnection::waitForName(const unsigned int timeoutMs)
{
    std::unique_lock<std::mutex> lock(mNameMutex);
    mNameCondition.wait_for(lock,
                            std::chrono::milliseconds(timeoutMs),
    [this] {
        return mNameAcquired || mNameLost;
    });

    return mNameAcquired;
}

void HostDbusConnection::onNameAcquired()
{
    std::unique_lock<std::mutex> lock(mNameMutex);
    mNameAcquired = true;
    mNameCondition.notify_one();
}

void HostDbusConnection::onNameLost()
{
    std::unique_lock<std::mutex> lock(mNameMutex);
    mNameLost = true;
    mNameCondition.notify_one();

    if (mNameAcquired) {
        // TODO implement reconnecting
        LOGE("TODO Reconnect !!!");
    }
}

void HostDbusConnection::setProxyCallCallback(const ProxyCallCallback& callback)
{
    mProxyCallCallback = callback;
}

void HostDbusConnection::setGetZoneConnectionsCallback(const GetZoneConnectionsCallback& callback)
{
    mGetZoneConnectionsCallback = callback;
}

void HostDbusConnection::setGetZoneIdsCallback(const GetZoneIdsCallback& callback)
{
    mGetZoneIdsCallback = callback;
}

void HostDbusConnection::setGetActiveZoneIdCallback(const GetActiveZoneIdCallback& callback)
{
    mGetActiveZoneIdCallback = callback;
}

void HostDbusConnection::setGetZoneInfoCallback(const GetZoneInfoCallback& callback)
{
    mGetZoneInfoCallback = callback;
}

void HostDbusConnection::setSetNetdevAttrsCallback(const SetNetdevAttrsCallback& callback)
{
    mSetNetdevAttrsCallback = callback;
}

void HostDbusConnection::setGetNetdevAttrsCallback(const GetNetdevAttrsCallback& callback)
{
    mGetNetdevAttrsCallback = callback;
}

void HostDbusConnection::setGetNetdevListCallback(const GetNetdevListCallback& callback)
{
    mGetNetdevListCallback = callback;
}

void HostDbusConnection::setCreateNetdevVethCallback(const CreateNetdevVethCallback& callback)
{
    mCreateNetdevVethCallback = callback;
}

void HostDbusConnection::setCreateNetdevMacvlanCallback(const CreateNetdevMacvlanCallback& callback)
{
    mCreateNetdevMacvlanCallback = callback;
}

void HostDbusConnection::setCreateNetdevPhysCallback(const CreateNetdevPhysCallback& callback)
{
    mCreateNetdevPhysCallback = callback;
}

void HostDbusConnection::setDestroyNetdevCallback(const DestroyNetdevCallback& callback)
{
    mDestroyNetdevCallback = callback;
}

void HostDbusConnection::setDeleteNetdevIpAddressCallback(const DeleteNetdevIpAddressCallback& callback)
{
    mDeleteNetdevIpAddressCallback = callback;
}

void HostDbusConnection::setDeclareFileCallback(const DeclareFileCallback& callback)
{
    mDeclareFileCallback = callback;
}

void HostDbusConnection::setDeclareMountCallback(const DeclareMountCallback& callback)
{
    mDeclareMountCallback = callback;
}

void HostDbusConnection::setDeclareLinkCallback(const DeclareLinkCallback& callback)
{
    mDeclareLinkCallback = callback;
}

void HostDbusConnection::setGetDeclarationsCallback(const GetDeclarationsCallback& callback)
{
    mGetDeclarationsCallback = callback;
}

void HostDbusConnection::setRemoveDeclarationCallback(const RemoveDeclarationCallback& callback)
{
    mRemoveDeclarationCallback =  callback;
}

void HostDbusConnection::setSetActiveZoneCallback(const SetActiveZoneCallback& callback)
{
    mSetActiveZoneCallback = callback;
}

void HostDbusConnection::setCreateZoneCallback(const CreateZoneCallback& callback)
{
    mCreateZoneCallback = callback;
}

void HostDbusConnection::setDestroyZoneCallback(const DestroyZoneCallback& callback)
{
    mDestroyZoneCallback = callback;
}

void HostDbusConnection::setShutdownZoneCallback(const ShutdownZoneCallback& callback)
{
    mShutdownZoneCallback = callback;
}

void HostDbusConnection::setStartZoneCallback(const StartZoneCallback& callback)
{
    mStartZoneCallback = callback;
}

void HostDbusConnection::setLockZoneCallback(const LockZoneCallback& callback)
{
    mLockZoneCallback = callback;
}

void HostDbusConnection::setUnlockZoneCallback(const UnlockZoneCallback& callback)
{
    mUnlockZoneCallback = callback;
}

void HostDbusConnection::setGrantDeviceCallback(const GrantDeviceCallback& callback)
{
    mGrantDeviceCallback = callback;
}

void HostDbusConnection::setRevokeDeviceCallback(const RevokeDeviceCallback& callback)
{
    mRevokeDeviceCallback = callback;
}

void HostDbusConnection::setNotifyActiveZoneCallback(const NotifyActiveZoneCallback& callback)
{
    mNotifyActiveZoneCallback = callback;
}

void HostDbusConnection::setSwitchToDefaultCallback(const SwitchToDefaultCallback& callback)
{
    mSwitchToDefaultCallback = callback;
}

void HostDbusConnection::setFileMoveCallback(const FileMoveCallback& callback)
{
    mFileMoveCallback = callback;
}

void HostDbusConnection::onMessageCall(const std::string& objectPath,
                                   const std::string& interface,
                                   const std::string& methodName,
                                   GVariant* parameters,
                                   dbus::MethodResultBuilder::Pointer result)
{
    if (objectPath != api::dbus::OBJECT_PATH || interface != api::dbus::INTERFACE) {
        return;
    }

    if (methodName == api::dbus::METHOD_SET_ACTIVE_ZONE) {
        api::ZoneId zoneId;
        config::loadFromGVariant(parameters, zoneId);

        if (mSetActiveZoneCallback) {
            auto rb = std::make_shared<api::DbusMethodResultBuilder<api::Void>>(result);
            mSetActiveZoneCallback(zoneId, rb);
        }
        return;
    }

    if (methodName == api::dbus::METHOD_PROXY_CALL) {
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

    if (methodName == api::dbus::METHOD_GET_ZONE_ID_LIST) {
        if (mGetZoneIdsCallback) {
            auto rb = std::make_shared<api::DbusMethodResultBuilder<api::ZoneIds>>(result);
            mGetZoneIdsCallback(rb);
        }
        return;
    }

    if (methodName == api::dbus::METHOD_GET_ACTIVE_ZONE_ID) {
        if (mGetActiveZoneIdCallback) {
            auto rb = std::make_shared<api::DbusMethodResultBuilder<api::ZoneId>>(result);
            mGetActiveZoneIdCallback(rb);
        }
        return;
    }

    if (methodName == api::dbus::METHOD_GET_ZONE_INFO) {
        api::ZoneId zoneId;
        config::loadFromGVariant(parameters, zoneId);

        if (mGetZoneInfoCallback) {
            auto rb = std::make_shared<api::DbusMethodResultBuilder<api::ZoneInfoOut>>(result);
            mGetZoneInfoCallback(zoneId, rb);
        }
        return;
    }

    if (methodName == api::dbus::METHOD_SET_NETDEV_ATTRS) {
        api::SetNetDevAttrsIn data;
        config::loadFromGVariant(parameters, data);

        if (mSetNetdevAttrsCallback) {
            auto rb = std::make_shared<api::DbusMethodResultBuilder<api::Void>>(result);
            mSetNetdevAttrsCallback(data, rb);
        }
        return;
    }

    if (methodName == api::dbus::METHOD_GET_NETDEV_ATTRS) {
        api::GetNetDevAttrsIn data;
        config::loadFromGVariant(parameters, data);

        if (mGetNetdevAttrsCallback) {
            auto rb = std::make_shared<api::DbusMethodResultBuilder<api::GetNetDevAttrs>>(result);
            mGetNetdevAttrsCallback(data, rb);
        }
        return;
    }

    if (methodName == api::dbus::METHOD_GET_NETDEV_LIST) {
        api::ZoneId data;
        config::loadFromGVariant(parameters, data);

        if (mGetNetdevListCallback) {
            auto rb = std::make_shared<api::DbusMethodResultBuilder<api::NetDevList>>(result);
            mGetNetdevListCallback(data, rb);
        }
        return;
    }

    if (methodName == api::dbus::METHOD_CREATE_NETDEV_VETH) {
        api::CreateNetDevVethIn data;
        config::loadFromGVariant(parameters, data);

        if (mCreateNetdevVethCallback) {
            auto rb = std::make_shared<api::DbusMethodResultBuilder<api::Void>>(result);
            mCreateNetdevVethCallback(data, rb);
        }
        return;
    }

    if (methodName == api::dbus::METHOD_CREATE_NETDEV_MACVLAN) {
        api::CreateNetDevMacvlanIn data;
        config::loadFromGVariant(parameters, data);

        if (mCreateNetdevMacvlanCallback) {
            auto rb = std::make_shared<api::DbusMethodResultBuilder<api::Void>>(result);
            mCreateNetdevMacvlanCallback(data, rb);
        }
    }

    if (methodName == api::dbus::METHOD_CREATE_NETDEV_PHYS) {
        api::CreateNetDevPhysIn data;
        config::loadFromGVariant(parameters, data);

        if (mCreateNetdevPhysCallback) {
            auto rb = std::make_shared<api::DbusMethodResultBuilder<api::Void>>(result);
            mCreateNetdevPhysCallback(data, rb);
        }
    }

    if (methodName == api::dbus::METHOD_DESTROY_NETDEV) {
        api::DestroyNetDevIn data;
        config::loadFromGVariant(parameters, data);

        if (mDestroyNetdevCallback) {
            auto rb = std::make_shared<api::DbusMethodResultBuilder<api::Void>>(result);
            mDestroyNetdevCallback(data, rb);
        }
    }

    if (methodName == api::dbus::METHOD_DELETE_NETDEV_IP_ADDRESS) {
        api::DeleteNetdevIpAddressIn data;
        config::loadFromGVariant(parameters, data);
        if (mDeleteNetdevIpAddressCallback) {
            auto rb = std::make_shared<api::DbusMethodResultBuilder<api::Void>>(result);
            mDeleteNetdevIpAddressCallback(data, rb);
        }
    }

    if (methodName == api::dbus::METHOD_DECLARE_FILE) {
        api::DeclareFileIn data;
        config::loadFromGVariant(parameters, data);

        if (mDeclareFileCallback) {
            auto rb = std::make_shared<api::DbusMethodResultBuilder<api::Declaration>>(result);
            mDeclareFileCallback(data, rb);
        }
        return;
    }

    if (methodName == api::dbus::METHOD_DECLARE_MOUNT) {
        api::DeclareMountIn data;
        config::loadFromGVariant(parameters, data);

        if (mDeclareMountCallback) {
            auto rb = std::make_shared<api::DbusMethodResultBuilder<api::Declaration>>(result);
            mDeclareMountCallback(data, rb);
        }
        return;
    }

    if (methodName == api::dbus::METHOD_DECLARE_LINK) {
        api::DeclareLinkIn data;
        config::loadFromGVariant(parameters, data);

        if (mDeclareLinkCallback) {
            auto rb = std::make_shared<api::DbusMethodResultBuilder<api::Declaration>>(result);
            mDeclareLinkCallback(data, rb);
        }
        return;
    }

    if (methodName == api::dbus::METHOD_GET_DECLARATIONS) {
        api::ZoneId data;
        config::loadFromGVariant(parameters, data);

        if (mGetDeclarationsCallback) {
            auto rb = std::make_shared<api::DbusMethodResultBuilder<api::Declarations>>(result);
            mGetDeclarationsCallback(data, rb);
        }
        return;
    }

    if (methodName == api::dbus::METHOD_REMOVE_DECLARATION) {
        api::RemoveDeclarationIn data;
        config::loadFromGVariant(parameters, data);

        if (mRemoveDeclarationCallback) {
            auto rb = std::make_shared<api::DbusMethodResultBuilder<api::Void>>(result);
            mRemoveDeclarationCallback(data, rb);
        }
        return;
    }

    if (methodName == api::dbus::METHOD_CREATE_ZONE) {
        api::CreateZoneIn data;
        config::loadFromGVariant(parameters, data);

        if (mCreateZoneCallback) {
            auto rb = std::make_shared<api::DbusMethodResultBuilder<api::Void>>(result);
            mCreateZoneCallback(data, rb);
        }
        return;
    }

    if (methodName == api::dbus::METHOD_DESTROY_ZONE) {
        api::ZoneId data;
        config::loadFromGVariant(parameters, data);

        if (mDestroyZoneCallback) {
            auto rb = std::make_shared<api::DbusMethodResultBuilder<api::Void>>(result);
            mDestroyZoneCallback(data, rb);
        }
        return;
    }

    if (methodName == api::dbus::METHOD_SHUTDOWN_ZONE) {
        api::ZoneId data;
        config::loadFromGVariant(parameters, data);

        if (mShutdownZoneCallback) {
            auto rb = std::make_shared<api::DbusMethodResultBuilder<api::Void>>(result);
            mShutdownZoneCallback(data, rb);
        }
    }

    if (methodName == api::dbus::METHOD_START_ZONE) {
        api::ZoneId data;
        config::loadFromGVariant(parameters, data);

        if (mStartZoneCallback) {
            auto rb = std::make_shared<api::DbusMethodResultBuilder<api::Void>>(result);
            mStartZoneCallback(data, rb);
        }
    }

    if (methodName == api::dbus::METHOD_LOCK_ZONE) {
        api::ZoneId data;
        config::loadFromGVariant(parameters, data);

        if (mLockZoneCallback) {
            auto rb = std::make_shared<api::DbusMethodResultBuilder<api::Void>>(result);
            mLockZoneCallback(data, rb);
        }
        return;
    }

    if (methodName == api::dbus::METHOD_UNLOCK_ZONE) {
        api::ZoneId data;
        config::loadFromGVariant(parameters, data);

        if (mUnlockZoneCallback) {
            auto rb = std::make_shared<api::DbusMethodResultBuilder<api::Void>>(result);
            mUnlockZoneCallback(data, rb);
        }
        return;
    }

    if (methodName == api::dbus::METHOD_GRANT_DEVICE) {
        api::GrantDeviceIn data;
        config::loadFromGVariant(parameters, data);

        if (mGrantDeviceCallback) {
            auto rb = std::make_shared<api::DbusMethodResultBuilder<api::Void>>(result);
            mGrantDeviceCallback(data, rb);
        }
        return;
    }

    if (methodName == api::dbus::METHOD_REVOKE_DEVICE) {
        api::RevokeDeviceIn data;
        config::loadFromGVariant(parameters, data);

        if (mRevokeDeviceCallback) {
            auto rb = std::make_shared<api::DbusMethodResultBuilder<api::Void>>(result);
            mRevokeDeviceCallback(data, rb);
        }
        return;
    }

    if (methodName == api::dbus::METHOD_NOTIFY_ACTIVE_ZONE) {
        api::NotifActiveZoneIn data;
        config::loadFromGVariant(parameters, data);

        if (mNotifyActiveZoneCallback) {
            auto rb = std::make_shared<api::DbusMethodResultBuilder<api::Void>>(result);
            mNotifyActiveZoneCallback(data, rb);
        }
    }

    if (methodName == api::dbus::METHOD_FILE_MOVE_REQUEST) {
        api::FileMoveRequestIn data;
        config::loadFromGVariant(parameters, data);

        if (mFileMoveCallback) {
            auto rb = std::make_shared<api::DbusMethodResultBuilder<api::FileMoveRequestStatus>>(result);
            mFileMoveCallback(data, rb);
        }
    }
}

void HostDbusConnection::onSignalCall(const std::string& /* senderBusName */,
                                      const std::string& objectPath,
                                      const std::string& interface,
                                      const std::string& signalName,
                                      GVariant* /* parameters */)
{
    if (objectPath != api::dbus::OBJECT_PATH || interface != api::dbus::INTERFACE) {
        return;
    }

    if (signalName == api::dbus::SIGNAL_SWITCH_TO_DEFAULT) {
        if (mSwitchToDefaultCallback) {
            mSwitchToDefaultCallback();
        }
    }
}

void HostDbusConnection::proxyCallAsync(const std::string& busName,
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

void HostDbusConnection::sendNotification(const api::Notification& notify)
{
    GVariant* parameters = g_variant_new("(sss)",
                                         notify.zone.c_str(),
                                         notify.application.c_str(),
                                         notify.message.c_str());
    mDbusConnection->emitSignal(api::dbus::OBJECT_PATH,
                                api::dbus::INTERFACE,
                                api::dbus::SIGNAL_NOTIFICATION,
                                parameters);
}

} // namespace vasum
