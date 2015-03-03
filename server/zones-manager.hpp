/*
 *  Copyright (c) 2014 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *  Contact: Jan Olszak <j.olszak@samsung.com>
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
 * @author  Jan Olszak (j.olszak@samsung.com)
 * @brief   Declaration of the class for managing many zones
 */


#ifndef SERVER_ZONES_MANAGER_HPP
#define SERVER_ZONES_MANAGER_HPP

#include "zone.hpp"
#include "zones-manager-config.hpp"
#include "host-connection.hpp"
#include "input-monitor.hpp"
#include "proxy-call-policy.hpp"
#include "utils/worker.hpp"
#include "api/method-result-builder.hpp"

#include <string>
#include <memory>


namespace vasum {


class ZonesManager final {

public:
    ZonesManager(const std::string& managerConfigPath);
    ~ZonesManager();

    /**
     * Create new zone.
     *
     * @param zoneId id of new zone
     * @param templateName a template name
     */
    void createZone(const std::string& zoneId, const std::string& templateName);

    /**
     * Destroy zone.
     *
     * @param zoneId id of the zone
     */
    void destroyZone(const std::string& zoneId);

    /**
     * Focus this zone, put it to the foreground.
     * Method blocks until the focus is switched.
     *
     * @param zoneId id of the zone
     */
    void focus(const std::string& zoneId);

    /**
     * Restore all the configured zones to the saved state
     */
    void restoreAll();

    /**
     * Shutdown all managed zones without changing the saved state
     */
    void shutdownAll();

    /**
     * @return Is the zone in a paused state?
     */
    bool isPaused(const std::string& zoneId);

    /**
     * @return Is the zone running?
     */
    bool isRunning(const std::string& zoneId);

    /**
     * @return Is the zone stopped?
     */
    bool isStopped(const std::string& zoneId);

    /**
     * @return id of the currently focused/foreground zone
     */
    std::string getRunningForegroundZoneId();

    /**
     * @return id of next to currently focused/foreground zone. If currently focused zone
     *         is last in zone map, id of fisrt zone from map is returned.
     */
    std::string getNextToForegroundZoneId();

    /**
     * Set whether ZonesManager should detach zones on exit
     */
    void setZonesDetachOnExit();

private:
    typedef std::recursive_mutex Mutex;
    typedef std::unique_lock<Mutex> Lock;

    utils::Worker::Pointer mWorker;
    Mutex mMutex; // used to protect mZones
    ZonesManagerConfig mConfig; //TODO make it const
    ZonesManagerDynamicConfig mDynamicConfig;
    HostConnection mHostConnection;
    // to hold InputMonitor pointer to monitor if zone switching sequence is recognized
    std::unique_ptr<InputMonitor> mSwitchingSequenceMonitor;
    std::unique_ptr<ProxyCallPolicy> mProxyCallPolicy;
    // like set but keep insertion order
    // smart pointer is needed because Zone is not moveable (because of mutex)
    typedef std::vector<std::unique_ptr<Zone>> Zones;
    Zones mZones;
    std::string mActiveZoneId;
    bool mDetachOnExit;

    Zones::iterator findZone(const std::string& id);
    Zone& getZone(const std::string& id);
    Zones::iterator getRunningForegroundZoneIterator();
    Zones::iterator getNextToForegroundZoneIterator();
    void focusInternal(Zones::iterator iter);

    void saveDynamicConfig();
    void updateDefaultId();
    void refocus();
    void switchingSequenceMonitorNotify();
    void generateNewConfig(const std::string& id,
                           const std::string& templatePath);
    std::string getTemplatePathForExistingZone(const std::string& id);
    int getVTForNewZone();
    void insertZone(const std::string& zoneId, const std::string& templatePath);

    void handleNotifyActiveZoneCall(const std::string& caller,
                                    const std::string& appliaction,
                                    const std::string& message,
                                    api::MethodResultBuilder::Pointer result);
    void handleDisplayOffCall(const std::string& caller);
    void handleFileMoveCall(const std::string& srcZoneId,
                               const std::string& dstZoneId,
                               const std::string& path,
                               api::MethodResultBuilder::Pointer result);
    void handleProxyCall(const std::string& caller,
                         const std::string& target,
                         const std::string& targetBusName,
                         const std::string& targetObjectPath,
                         const std::string& targetInterface,
                         const std::string& targetMethod,
                         GVariant* parameters,
                         dbus::MethodResultBuilder::Pointer result);
    void handleGetZoneDbusesCall(api::MethodResultBuilder::Pointer result);
    void handleDbusStateChanged(const std::string& zoneId, const std::string& dbusAddress);
    void handleGetZoneIdsCall(api::MethodResultBuilder::Pointer result);
    void handleGetActiveZoneIdCall(api::MethodResultBuilder::Pointer result);
    void handleGetZoneInfoCall(const std::string& id, api::MethodResultBuilder::Pointer result);
    void handleSetNetdevAttrsCall(const std::string& zone,
                                  const std::string& netdev,
                                  const std::vector<std::tuple<std::string, std::string>>& attrs,
                                  api::MethodResultBuilder::Pointer result);
    void handleGetNetdevAttrsCall(const std::string& zone,
                                  const std::string& netdev,
                                  api::MethodResultBuilder::Pointer result);
    void handleGetNetdevListCall(const std::string& zone,
                                 api::MethodResultBuilder::Pointer result);
    void handleCreateNetdevVethCall(const std::string& zone,
                                    const std::string& zoneDev,
                                    const std::string& hostDev,
                                    api::MethodResultBuilder::Pointer result);
    void handleCreateNetdevMacvlanCall(const std::string& zone,
                                       const std::string& zoneDev,
                                       const std::string& hostDev,
                                       const uint32_t& mode,
                                       api::MethodResultBuilder::Pointer result);
    void handleCreateNetdevPhysCall(const std::string& zone,
                                    const std::string& devId,
                                    api::MethodResultBuilder::Pointer result);
    void handleDestroyNetdevCall(const std::string& zone,
                                 const std::string& devId,
                                 api::MethodResultBuilder::Pointer result);
    void handleDeclareFileCall(const std::string& zone,
                               const int32_t& type,
                               const std::string& path,
                               const int32_t& flags,
                               const int32_t& mode,
                               api::MethodResultBuilder::Pointer result);
    void handleDeclareMountCall(const std::string& source,
                                const std::string& zone,
                                const std::string& target,
                                const std::string& type,
                                const uint64_t& flags,
                                const std::string& data,
                                api::MethodResultBuilder::Pointer result);
    void handleDeclareLinkCall(const std::string& source,
                               const std::string& zone,
                               const std::string& target,
                               api::MethodResultBuilder::Pointer result);
    void handleGetDeclarationsCall(const std::string& zone,
                                   api::MethodResultBuilder::Pointer result);
    void handleRemoveDeclarationCall(const std::string& zone,
                                     const std::string& declarationId,
                                     api::MethodResultBuilder::Pointer result);
    void handleSetActiveZoneCall(const std::string& id,
                                 api::MethodResultBuilder::Pointer result);
    void handleCreateZoneCall(const std::string& id,
                              const std::string& templateName,
                              api::MethodResultBuilder::Pointer result);
    void handleDestroyZoneCall(const std::string& id,
                               api::MethodResultBuilder::Pointer result);
    void handleShutdownZoneCall(const std::string& id,
                                api::MethodResultBuilder::Pointer result);
    void handleStartZoneCall(const std::string& id,
                             api::MethodResultBuilder::Pointer result);
    void handleLockZoneCall(const std::string& id,
                            api::MethodResultBuilder::Pointer result);
    void handleUnlockZoneCall(const std::string& id,
                              api::MethodResultBuilder::Pointer result);
    void handleGrantDeviceCall(const std::string& id,
                               const std::string& device,
                               uint32_t flags,
                               api::MethodResultBuilder::Pointer result);
    void handleRevokeDeviceCall(const std::string& id,
                                const std::string& device,
                                api::MethodResultBuilder::Pointer result);
};


} // namespace vasum


#endif // SERVER_ZONES_MANAGER_HPP
