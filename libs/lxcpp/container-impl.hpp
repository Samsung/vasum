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
 * @author  Mateusz Malicki (m.malicki2@samsung.com)
 * @brief   ContainerImpl main class
 */

#ifndef LXCPP_CONTAINER_IMPL_HPP
#define LXCPP_CONTAINER_IMPL_HPP

#include <sys/types.h>
#include <memory>

#include "lxcpp/container-config.hpp"
#include "lxcpp/container.hpp"
#include "lxcpp/namespace.hpp"
#include "lxcpp/guard/api.hpp"

#include "utils/inotify.hpp"

#include "cargo-ipc/epoll/thread-dispatcher.hpp"
#include "cargo-ipc/client.hpp"
#include "cargo-ipc/exception.hpp"

namespace lxcpp {
/**
 * Implementation of the Container interface.
 *
 * Implementation notes:
 *
 * ContainerImpl and cargo::ipc have lock mutexes.
 * To avoid deadlocks don't make ipc calls when ContainerImpl::mStateMutex is locked.
 */
class ContainerImpl : public virtual Container {
public:
    ContainerImpl(const std::string &name,
                  const std::string &rootPath,
                  const std::string &workPath);
    ~ContainerImpl();

    // Configuration
    const std::string& getName() const;
    const std::string& getRootPath() const;
    const std::string& getWorkPath() const;

    void setHostName(const std::string& hostname);

    pid_t getGuardPid() const;
    pid_t getInitPid() const;

    const std::vector<std::string>& getInit();
    void setInit(const std::vector<std::string> &init);

    void setLogger(const logger::LogType type,
                   const logger::LogLevel level,
                   const std::string &arg);

    void setTerminalCount(const unsigned int count);

    void addUIDMap(uid_t contID, uid_t hostID, unsigned num);
    void addGIDMap(gid_t contID, gid_t hostID, unsigned num);

    // Execution actions
    void start(const unsigned int timeoutMS);
    void stop(const unsigned int timeoutMS);
    void freeze();
    void unfreeze();
    void reboot();
    bool connect();

    // State
    Container::State getState();
    void setStartedCallback(const Container::Callback& callback);
    void setStoppedCallback(const Container::Callback& callback);
    void setConnectedCallback(const Container::Callback& callback);

    // Other
    int attach(const std::vector<std::string>& argv,
               const uid_t uid,
               const gid_t gid,
               const std::string& ttyPath,
               const std::vector<gid_t>& supplementaryGids,
               const unsigned long long capsToKeep,
               const std::string& workDirInContainer,
               const std::vector<std::string>& envToKeep,
               const std::vector<std::pair<std::string, std::string>>& envToSet);
    void console();

    // Network interfaces setup/config
    /**
     * adds interface configration.
     * throws NetworkException if zoneif name already on list
     */
    void addInterfaceConfig(InterfaceConfigType type,
                            const std::string& hostif,
                            const std::string& zoneif,
                            const std::vector<InetAddr>& addrs,
                            MacVLanMode mode);
    void addInetConfig(const std::string& ifname, const InetAddr& addr);

    // Network interfaces (runtime)
    std::vector<std::string> getInterfaces() const;
    NetworkInterfaceInfo getInterfaceInfo(const std::string& ifname) const;
    void createInterface(const std::string& hostif,
                         const std::string& zoneif,
                         InterfaceType type,
                         MacVLanMode mode);
    void moveInterface(const std::string& ifname);
    void destroyInterface(const std::string& ifname);
    void setUpInterface(const std::string& ifname);
    void setDownInterface(const std::string& ifname);
    void addInetAddr(const std::string& ifname, const InetAddr& addr);
    void delInetAddr(const std::string& ifname, const InetAddr& addr);

    // Provisioning
    void declareFile(const provision::File::Type type,
                     const std::string& path,
                     const int32_t flags,
                     const int32_t mode);
    const FileVector& getFiles() const;
    void removeFile(const provision::File& item);

    void declareMount(const std::string& source,
                      const std::string& target,
                      const std::string& type,
                      const int64_t flags,
                      const std::string& data);
    const MountVector& getMounts() const;
    void removeMount(const provision::Mount& item);

    void declareLink(const std::string& source,
                     const std::string& target);
    const LinkVector& getLinks() const;
    void removeLink(const provision::Link& item);

    // CGroups
    void addSubsystem(const std::string& name, const std::string& path);
    void addCGroup(const std::string& subsys,
                   const std::string& grpname,
                   const std::vector<CGroupParam>& comm,
                   const std::vector<CGroupParam>& params);

    // Environment variables
    void setEnv(const std::vector<std::pair<std::string, std::string>>& variables);

    // Linux capabilities
    void setCaps(const unsigned long long caps);

    // Kernel parameter (sysctl)
    void setKernelParameter(const std::string& name, const std::string& value);

    // Rlimit
    void setRlimit(const int type, const uint64_t soft, const uint64_t hard);

    // Namespaces
    void setNamespaces(const int namespaces);

    // UID/GIDS
    void setUser(const int uid, const int gid, const std::vector<int> additionalGids);

    // Device
    void addDevice(const std::string& path,
                   const char type,
                   const int64_t major,
                   const int64_t minor,
                   const std::string& permissions,
                   const uint32_t fileMode,
                   const uint32_t uid,
                   const uint32_t gid);

    // Hooks
    void addHook(const std::string& type,
                 const std::vector<std::string>& hook,
                 const std::vector<std::pair<std::string, std::string>>& env);


private:
    typedef std::unique_lock<std::mutex> Lock;
    mutable std::mutex mStateMutex;
    std::condition_variable mStateCondition;

    std::shared_ptr<ContainerConfig> mConfig;

    cargo::ipc::epoll::ThreadDispatcher mDispatcher;

    std::shared_ptr<cargo::ipc::Client> mClient;
    utils::Inotify mInotify;

    // Callbacks
    Container::Callback mStartedCallback;
    Container::Callback mStoppedCallback;
    Container::Callback mConnectedCallback;

    void setState(const Container::State state);

    /**
     * File was created/removed from the working directory
     */
    void onWorkFileEvent(const std::string& name, const uint32_t mask);

    /**
     * Guard was just started and tells that it's ready to receive commands.
     *
     * This is a method handler, not signal to avoid races.
     */
    cargo::ipc::HandlerExitCode onGuardReady(const cargo::ipc::PeerID,
                                             std::shared_ptr<api::Void>&,
                                             cargo::ipc::MethodResult::Pointer);

    /**
     * Host connected to an existing Guard.
     * Guard was previously started by another instance of ContainerImpl.
     * Guard sends back its configuration.
     */
    cargo::ipc::HandlerExitCode onGuardConnected(const cargo::ipc::PeerID,
                                                 std::shared_ptr<ContainerConfig>& data,
                                                 cargo::ipc::MethodResult::Pointer);

    /**
     * Configuration has been set in guard
     */
    void onConfigSet(cargo::ipc::Result<api::Void>&& result);

    /**
     * Guards just started Init and passes its PID
     */
    void onInitStarted(cargo::ipc::Result<api::Pid>&& result);

    /**
     * Guards tells that Init exited with some status.
     * Init could have been:
     * - stopped with stop()
     * - exited by itself
     */
    cargo::ipc::HandlerExitCode onInitStopped(const cargo::ipc::PeerID,
                                              std::shared_ptr<api::ExitStatus>& data,
                                              cargo::ipc::MethodResult::Pointer);
};

} // namespace lxcpp

#endif // LXCPP_CONTAINER_IMPL_HPP
