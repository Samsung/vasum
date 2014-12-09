/*
 *  Copyright (c) 2014 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *  Contact: Lukasz Pawelczyk <l.pawelczyk@partner.samsung.com>
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
 * @author  Lukasz Pawelczyk (l.pawelczyk@partner.samsung.com)
 * @brief   Declaration of the class for managing one zone
 */


#ifndef SERVER_ZONE_HPP
#define SERVER_ZONE_HPP

#include "zone-config.hpp"
#include "zone-admin.hpp"
#include "zone-connection.hpp"
#include "zone-connection-transport.hpp"
#include "zone-provision.hpp"
#include "utils/worker.hpp"

#include <string>
#include <memory>
#include <thread>
#include <boost/regex.hpp>


namespace vasum {


class Zone {

public:
    /**
     * Zone constructor
     * @param zonesPath directory where zones are defined (lxc configs, rootfs etc)
     * @param zoneConfigPath path for zones config
     * @param lxcTemplatePrefix directory where templates are stored
     * @param baseRunMountPointPath base directory for run mount point
     */
    Zone(const utils::Worker::Pointer& worker,
         const std::string& zonesPath,
         const std::string& zoneConfigPath,
         const std::string& lxcTemplatePrefix,
         const std::string& baseRunMountPointPath);
    Zone(Zone&&) = default;
    virtual ~Zone();

    typedef ZoneConnection::NotifyActiveZoneCallback NotifyActiveZoneCallback;
    typedef ZoneConnection::DisplayOffCallback DisplayOffCallback;
    typedef ZoneConnection::FileMoveRequestCallback FileMoveRequestCallback;
    typedef ZoneConnection::ProxyCallCallback ProxyCallCallback;

    typedef std::function<void(const std::string& address)> DbusStateChangedCallback;
    typedef std::function<void(bool succeeded)> StartAsyncResultCallback;

    /**
     * Returns a vector of regexps defining files permitted to be
     * send to other zones using file move functionality
     */
    const std::vector<boost::regex>& getPermittedToSend() const;

    /**
     * Returns a vector of regexps defining files permitted to be
     * send to other zones using file move functionality
     */
    const std::vector<boost::regex>& getPermittedToRecv() const;

    // ZoneAdmin API

    /**
     * Get the zone id
     */
    const std::string& getId() const;

    /**
     * Get the zone privilege
     */
    int getPrivilege() const;

    /**
     * Boot the zone to the background.
     */
    void start();

    /**
     * Boot the zone to the background in separate thread. This function immediately exits
     * after zone booting is started in another thread.
     *
     * @param callback Called after starting the zone. Passes bool with result of starting.
     */
    void startAsync(const StartAsyncResultCallback& callback);

    /**
     * Try to shutdown the zone, if failed, destroy it.
     */
    void stop();

    /**
     * Activate this zone's VT
     *
     * @return Was activation successful?
     */
    bool activateVT();

    /**
     * Setup this zone to be put in the foreground.
     * I.e. set appropriate scheduler level.
     */
    void goForeground();

    /**
     * Setup this zone to be put in the background.
     * I.e. set appropriate scheduler level.
     */
    void goBackground();

    /**
     * Set if zone should be detached on exit.
     *
     * This sends detach flag to ZoneAdmin object and disables unmounting tmpfs
     * in ZoneConnectionTransport.
     */
    void setDetachOnExit();

    /**
     * Set if zone should be destroyed on exit.
     */
    void setDestroyOnExit();

    /**
     * @return Is the zone running?
     */
    bool isRunning();

    /**
     * Check if the zone is stopped. It's NOT equivalent to !isRunning,
     * because it checks different internal libvirt's states. There are other states,
     * (e.g. paused) when the zone isn't running nor stopped.
     *
     * @return Is the zone stopped?
     */
    bool isStopped();

    /**
     * Suspend zone.
     */
    void suspend();

    /**
     * Resume zone.
     */
    void resume();

    /**
     * @return Is the zone in a paused state?
     */
    bool isPaused();

    // ZoneConnection API

    /**
     * @return Is switching to default zone after timeout allowed?
     */
    bool isSwitchToDefaultAfterTimeoutAllowed() const;

    /**
     * Register notification request callback
     */
    void setNotifyActiveZoneCallback(const NotifyActiveZoneCallback& callback);

    /**
     * Register callback used when switching to default zone.
     */
    void setDisplayOffCallback(const DisplayOffCallback& callback);

    /**
     * Register proxy call callback
     */
    void setProxyCallCallback(const ProxyCallCallback& callback);

    /**
     * Send notification signal to this zone
     *
     * @param zone   name of zone in which the notification occurred
     * @param application name of application that cause notification
     * @param message     message to be send to zone
     */
    void sendNotification(const std::string& zone,
                          const std::string& application,
                          const std::string& message);

    /**
     * Register file move request callback
     */
    void setFileMoveRequestCallback(const FileMoveRequestCallback& callback);

    /**
     * Register dbus state changed callback
     */
    void setDbusStateChangedCallback(const DbusStateChangedCallback& callback);

    /**
     * Make a proxy call
     */
    void proxyCallAsync(const std::string& busName,
                        const std::string& objectPath,
                        const std::string& interface,
                        const std::string& method,
                        GVariant* parameters,
                        const dbus::DbusConnection::AsyncMethodCallCallback& callback);

    /**
     * Get a dbus address
     */
    std::string getDbusAddress() const;

    /**
     * Get id of VT
     */
    int getVT() const;

    /**
     * Declare file, directory or pipe that will be created while zone startup
     */
    void declareFile(const int32_t& type,
                     const std::string& path,
                     const int32_t& flags,
                     const int32_t& mode);
    /**
     * Declare mount that will be created while zone startup
     */
    void declareMount(const std::string& source,
                      const std::string& target,
                      const std::string& type,
                      const int64_t& flags,
                      const std::string& data);
    /**
     * Declare link that will be created while zone startup
     */
    void declareLink(const std::string& source,
                     const std::string& target);

   /**
     * Get zone root path
     */
    std::string getRootPath() const;

private:
    utils::Worker::Pointer mWorker;
    ZoneConfig mConfig;
    std::vector<boost::regex> mPermittedToSend;
    std::vector<boost::regex> mPermittedToRecv;
    std::unique_ptr<ZoneConnectionTransport> mConnectionTransport;
    std::unique_ptr<ZoneAdmin> mAdmin;
    std::unique_ptr<ZoneConnection> mConnection;
    std::unique_ptr<ZoneProvision> mProvision;
    mutable std::recursive_mutex mReconnectMutex;
    NotifyActiveZoneCallback mNotifyCallback;
    DisplayOffCallback mDisplayOffCallback;
    FileMoveRequestCallback mFileMoveCallback;
    ProxyCallCallback mProxyCallCallback;
    DbusStateChangedCallback mDbusStateChangedCallback;
    std::string mDbusAddress;
    std::string mRunMountPoint;

    void onNameLostCallback();
    void reconnectHandler();
    void connect();
    void disconnect();
};


} // namespace vasum

#endif // SERVER_ZONE_HPP
