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
 * @brief   Declaration of CommandLineInterface class
 */
#ifndef CLI_COMMAND_LINE_INTERFACE_HPP
#define CLI_COMMAND_LINE_INTERFACE_HPP

#include <map>
#include <vector>
#include <functional>
#include <ostream>
#include <string>

namespace vasum {
namespace cli {

/**
 * Class that implements command pattern.
 */
class CommandLineInterface {

public:
    /**
     * @see CommandLineInterface::execute
     */
    typedef std::function<void(int, int, const char**)> ExecutorCallback;

    /**
     * @see CommandLineInterface::CommandLineInterface
     */
    typedef std::vector<std::pair<std::string, std::string>> ArgsSpec;

    /**
     * Dummy constructor (for stl usage)
     */
    CommandLineInterface() {}

    /**
     *  Construct command
     *
     *  @param executorCallback Callback function that will do the job
     *  @param usage Description of use
     *  @param usageInfo Description of the command
     *  @param argsSpec Description of arguments
     */
    CommandLineInterface(const ExecutorCallback& executorCallback,
                         const std::string& usage,
                         const std::string& usageInfo,
                         const ArgsSpec& argsSpec)
        : mExecutorCallback(executorCallback),
          mUsage(usage),
          mUsageInfo(usageInfo),
          mArgsSpec(argsSpec) {}

    /**
     * Print usage to stream
     *
     * @param out Output stream
     */
    void printUsage(std::ostream& out) const;

    /**
     * Do the work
     *
     * It calls the callback passed in constructor
     *
     * @param pos Points to element in argv where command was recognized (i.e. command name)
     * @param argc Number of elements in argv
     * @param argv Command line arguments
     */
    void execute(int pos, int argc, const char** argv);


private:
    const ExecutorCallback mExecutorCallback;
    const std::string mUsage;
    const std::string mUsageInfo;
    const ArgsSpec mArgsSpec;
};

/**
 * Parses command line arguments and call vsm_set_active_zone
 *
 * @see vsm_set_active_zone
 */
void set_active_zone(int pos, int argc, const char** argv);

/**
 * Parses command line arguments and call vsm_create_zone
 *
 * @see vsm_create_zone
 */
void create_zone(int pos, int argc, const char** argv);

/**
 * Parses command line arguments and call vsm_destroy_zone
 *
 * @see vsm_destroy_zone
 */
void destroy_zone(int pos, int argc, const char** argv);

/**
 * Parses command line arguments and call vsm_shutdown_zone
 *
 * @see vsm_shutdown_zone
 */
void shutdown_zone(int pos, int argc, const char** argv);

/**
 * Parses command line arguments and call vsm_start_zone
 *
 * @see vsm_start_zone
 */
void start_zone(int pos, int argc, const char** argv);

/**
 * Parses command line arguments and call vsm_lock_zone
 *
 * @see vsm_lock_zone
 */
void lock_zone(int pos, int argc, const char** argv);

/**
 * Parses command line arguments and call vsm_unlock_zone
 *
 * @see vsm_unlock_zone
 */
void unlock_zone(int pos, int argc, const char** argv);

/**
 * Parses command line arguments and prints list of zone with
 * some useful informations (id, state, terminal, root path)
 */
void get_zones_status(int pos, int argc, const char** argv);

/**
 * Parses command line arguments and call vsm_get_zone_ids
 *
 * @see vsm_get_zone_ids
 */
void get_zone_ids(int pos, int argc, const char** argv);

/**
 * Parses command line arguments and call vsm_get_active_zone_id
 *
 * @see vsm_get_active_zone_id
 */
void get_active_zone_id(int pos, int argc, const char** argv);

/**
 * Parses command line arguments and call vsm_lookup_zone_by_id
 *
 * @see vsm_lookup_zone_by_id
 */
void lookup_zone_by_id(int pos, int argc, const char** argv);

/**
 * Parses command line arguments and call vsm_grant_device
 *
 * @see vsm_grant_device
 */
void grant_device(int pos, int argc, const char** argv);

/**
 * Parses command line arguments and call vsm_revoke_device
 *
 * @see vsm_revoke_device
 */
void revoke_device(int pos, int argc, const char** argv);

/**
 * Parses command line arguments and call vsm_create_netdev_veth
 *
 * @see vsm_create_netdev_veth
 */
void create_netdev_veth(int pos, int argc, const char** argv);

/**
 * Parses command line arguments and call vsm_create_netdev_macvlan
 *
 * @see vsm_create_netdev_macvlan
 */
void create_netdev_macvlan(int pos, int argc, const char** argv);

/**
 * Parses command line arguments and call vsm_create_netdev_phys
 *
 * @see vsm_create_netdev_phys
 */
void create_netdev_phys(int pos, int argc, const char** argv);

/**
 * Parses command line arguments and call vsm_lookup_netdev_by_name
 *
 * @see vsm_lookup_netdev_by_name
 */
void lookup_netdev_by_name(int pos, int argc, const char** argv);

/**
 * Parses command line arguments and call vsm_destroy_netdev
 *
 * @see vsm_destroy_netdev
 */
void destroy_netdev(int pos, int argc, const char** argv);

/**
 * Parses command line arguments and prints result of vsm_zone_get_netdevs
 *
 * @see vsm_zone_get_netdevs
 */
void zone_get_netdevs(int pos, int argc, const char** argv);

/**
 * Parses command line arguments and prints result of vsm_netdev_get_ipv4_addr
 *
 * @see vsm_netdev_get_ipv4_addr
 */
void netdev_get_ipv4_addr(int pos, int argc, const char** argv);

/**
 * Parses command line arguments and and prints result of vsm_netdev_get_ipv6_addr
 *
 * @see vsm_netdev_get_ipv6_addr
 */
void netdev_get_ipv6_addr(int pos, int argc, const char** argv);

/**
 * Parses command line arguments and call vsm_netdev_set_ipv4_addr
 *
 * @see vsm_netdev_set_ipv4_addr
 */
void netdev_set_ipv4_addr(int pos, int argc, const char** argv);

/**
 * Parses command line arguments and call vsm_netdev_set_ipv6_addr
 *
 * @see vsm_netdev_set_ipv6_addr
 */
void netdev_set_ipv6_addr(int pos, int argc, const char** argv);

/**
 * Parses command line arguments and call vsm_netdev_up
 *
 * @see vsm_netdev_up
 */
void netdev_up(int pos, int argc, const char** argv);

/**
 * Parses command line arguments and call vsm_netdev_down
 *
 * @see vsm_netdev_down
 */
void netdev_down(int pos, int argc, const char** argv);

} // namespace cli
} // namespace vasum

#endif /* CLI_COMMAND_LINE_INTERFACE_HPP */
