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
 * @brief   Declaration of the class for managing one container
 */


#ifndef SERVER_CONTAINER_HPP
#define SERVER_CONTAINER_HPP

#include "container-config.hpp"
#include "container-admin.hpp"
#include "container-connection.hpp"
#include "container-connection-transport.hpp"
#include "network-admin.hpp"


#include <string>
#include <memory>
#include <thread>
#include <boost/regex.hpp>


namespace security_containers {


class Container {

public:
    Container(const std::string& containerConfigPath,
              const std::string& baseRunMountPointPath = "");
    Container(Container&&) = default;
    virtual ~Container();

    typedef ContainerConnection::NotifyActiveContainerCallback NotifyActiveContainerCallback;
    typedef ContainerConnection::DisplayOffCallback DisplayOffCallback;
    typedef ContainerConnection::FileMoveRequestCallback FileMoveRequestCallback;
    typedef ContainerConnection::ProxyCallCallback ProxyCallCallback;

    typedef std::function<void(const std::string& address)> DbusStateChangedCallback;
    typedef std::function<void(bool succeeded)> StartAsyncResultCallback;

    /**
     * Returns a vector of regexps defining files permitted to be
     * send to other containers using file move functionality
     */
    const std::vector<boost::regex>& getPermittedToSend() const;

    /**
     * Returns a vector of regexps defining files permitted to be
     * send to other containers using file move functionality
     */
    const std::vector<boost::regex>& getPermittedToRecv() const;

    // ContainerAdmin API

    /**
     * Get the container id
     */
    const std::string& getId() const;

    /**
     * Get the container privilege
     */
    int getPrivilege() const;

    /**
     * Boot the container to the background.
     */
    void start();

    /**
     * Boot the container to the background in separate thread. This function immediately exits
     * after container booting is started in another thread.
     *
     * @param callback Called after starting the container. Passes bool with result of starting.
     */
    void startAsync(const StartAsyncResultCallback& callback);

    /**
     * Try to shutdown the container, if failed, destroy it.
     */
    void stop();

    /**
     * Activate this container's VT
     *
     * @return Was activation successful?
     */
    bool activateVT();

    /**
     * Setup this container to be put in the foreground.
     * I.e. set appropriate scheduler level.
     */
    void goForeground();

    /**
     * Setup this container to be put in the background.
     * I.e. set appropriate scheduler level.
     */
    void goBackground();

    /**
     * Set if container should be detached on exit.
     *
     * This sends detach flag to ContainerAdmin object and disables unmounting tmpfs
     * in ContainerConnectionTransport.
     */
    void setDetachOnExit();

    /**
     * @return Is the container running?
     */
    bool isRunning();

    /**
     * Check if the container is stopped. It's NOT equivalent to !isRunning,
     * because it checks different internal libvirt's states. There are other states,
     * (e.g. paused) when the container isn't running nor stopped.
     *
     * @return Is the container stopped?
     */
    bool isStopped();

    /**
     * @return Is the container in a paused state?
     */
    bool isPaused();

    // ContainerConnection API

    /**
     * @return Is switching to default container after timeout allowed?
     */
    bool isSwitchToDefaultAfterTimeoutAllowed() const;

    /**
     * Register notification request callback
     */
    void setNotifyActiveContainerCallback(const NotifyActiveContainerCallback& callback);

    /**
     * Register callback used when switching to default container.
     */
    void setDisplayOffCallback(const DisplayOffCallback& callback);

    /**
     * Register proxy call callback
     */
    void setProxyCallCallback(const ProxyCallCallback& callback);

    /**
     * Send notification signal to this container
     *
     * @param container   name of container in which the notification occurred
     * @param application name of application that cause notification
     * @param message     message to be send to container
     */
    void sendNotification(const std::string& container,
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
    std::string getDbusAddress();

    /**
     * Get id of VT
     */
    int getVT() const;

private:
    ContainerConfig mConfig;
    std::vector<boost::regex> mPermittedToSend;
    std::vector<boost::regex> mPermittedToRecv;
    std::unique_ptr<ContainerConnectionTransport> mConnectionTransport;
    std::unique_ptr<NetworkAdmin> mNetworkAdmin;
    std::unique_ptr<ContainerAdmin> mAdmin;
    std::unique_ptr<ContainerConnection> mConnection;
    std::thread mReconnectThread;
    std::thread mStartThread;
    mutable std::recursive_mutex mReconnectMutex;
    NotifyActiveContainerCallback mNotifyCallback;
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


}


#endif // SERVER_CONTAINER_HPP
