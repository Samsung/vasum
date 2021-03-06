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

#include <vasum-client.h>

#include <map>
#include <vector>
#include <functional>
#include <ostream>
#include <string>

namespace vasum {
namespace cli {

#define MODE_COMMAND_LINE  (1 << 0)
#define MODE_INTERACTIVE   (1 << 1)

typedef std::vector<std::string> Args;

struct ArgSpec {
    std::string name;
    std::string description;
    std::string format;
};

/**
 * Class that implements command pattern.
 */
class CommandLineInterface {

public:
    /**
     * @see CommandLineInterface::execute
     */
    typedef std::function<void(const Args& argv)> ExecutorCallback;

    /**
     * @see CommandLineInterface::CommandLineInterface
     */
    typedef std::vector<ArgSpec> ArgsSpec;

    /**
     * Dummy constructor (for stl usage)
     */
    CommandLineInterface()
        : mAvailability(0) {}

    /**
     *  Construct command
     *
     *  @param executorCallback Callback function that will do the job
     *  @param name Command name
     *  @param description Command Description
     *  @param availability Command availability (bit field)
     *  @param argsSpec Description of arguments
     *  @remark Possible bits for availability: MODE_COMMAND_LINE, MODE_INTERACTIVE
     */
    CommandLineInterface(const ExecutorCallback& executorCallback,
                         const std::string& name,
                         const std::string& description,
                         const unsigned int& availability,
                         const ArgsSpec& argsSpec)
        : mExecutorCallback(executorCallback),
          mName(name),
          mDescription(description),
          mAvailability(availability),
          mArgsSpec(argsSpec) {}

    /**
     * Set the class (static) in a connected state.
     * This persistent connection to the vasum client is required
     * for calls like lock/unlock queue to work.
     */
    static void connect();

    /**
     * Disconnect the class from a vasum client.
     */
    static void disconnect();

    /**
     * Execute a callback passing the connected VsmClient.
     *
     * @param fun A callback to execute
     */
    static void executeCallback(const std::function<VsmStatus(VsmClient)>& fun);

    /**
     * Get the command name
     */
    const std::string& getName() const;

    /**
     * Get the command description
     */
    const std::string& getDescription() const;

    /**
     * Print usage to stream
     *
     * @param out Output stream
     */
    void printUsage(std::ostream& out) const;

    /**
     * Check if the command is available in specific mode
     *
     * @param mode A mode to check the command's availability in
     */
    bool isAvailable(unsigned int mode) const;

    /**
     * Do the work
     *
     * It calls the callback passed in constructor
     *
     * @param argv Command line arguments
     */
    void execute(const Args& argv) const;

    const std::vector<std::string> buildCompletionList(const Args& argv) const;

private:
    static VsmClient client;

    const ExecutorCallback mExecutorCallback;
    const std::string mName;
    const std::string mDescription;
    const unsigned int mAvailability;
    const ArgsSpec mArgsSpec;
};

/**
 * Parses command line arguments and call vsm_set_active_zone
 *
 * @see vsm_set_active_zone
 */
void lock_queue(const Args& argv);

/**
 * Parses command line arguments and call vsm_set_active_zone
 *
 * @see vsm_set_active_zone
 */
void unlock_queue(const Args& argv);

/**
 * Parses command line arguments and call vsm_set_active_zone
 *
 * @see vsm_set_active_zone
 */
void set_active_zone(const Args& argv);

/**
 * Parses command line arguments and call vsm_create_zone
 *
 * @see vsm_create_zone
 */
void create_zone(const Args& argv);

/**
 * Parses command line arguments and call vsm_destroy_zone
 *
 * @see vsm_destroy_zone
 */
void destroy_zone(const Args& argv);

/**
 * Parses command line arguments and call vsm_shutdown_zone
 *
 * @see vsm_shutdown_zone
 */
void shutdown_zone(const Args& argv);

/**
 * Parses command line arguments and call vsm_start_zone
 *
 * @see vsm_start_zone
 */
void start_zone(const Args& argv);

/**
 * Parses command line arguments and call lxc-console
 *
 */
void console_zone(const Args& argv);

/**
 * Parses command line arguments and call vsm_lock_zone
 *
 * @see vsm_lock_zone
 */
void lock_zone(const Args& argv);

/**
 * Parses command line arguments and call vsm_unlock_zone
 *
 * @see vsm_unlock_zone
 */
void unlock_zone(const Args& argv);

/**
 * Parses command line arguments and prints list of zone with
 * some useful informations (id, state, terminal, root path)
 */
void get_zones_status(const Args& argv);

/**
 * Parses command line arguments and call vsm_get_zone_ids
 *
 * @see vsm_get_zone_ids
 */
void get_zone_ids(const Args& argv);

/**
 * Parses command line arguments and call vsm_get_active_zone_id
 *
 * @see vsm_get_active_zone_id
 */
void get_active_zone(const Args& argv);

/**
 * Parses command line arguments and call vsm_grant_device
 *
 * @see vsm_grant_device
 */
void grant_device(const Args& argv);

/**
 * Parses command line arguments and call vsm_revoke_device
 *
 * @see vsm_revoke_device
 */
void revoke_device(const Args& argv);

/**
 * Parses command line arguments and call vsm_create_netdev_*
 *
 * @see vsm_create_netdev_veth,vsm_create_netdev_macvlan,vsm_create_netdev_phys
 */
void create_netdev(const Args& argv);

/**
 * Parses command line arguments and call vsm_destroy_netdev
 *
 * @see vsm_destroy_netdev
 */
void destroy_netdev(const Args& argv);

/**
 * Parses command line arguments and prints result of vsm_zone_get_netdevs,
 *  vsm_lookup_netdev_by_name, vsm_netdev_get_ipv4_addr, vsm_netdev_get_ipv6_addr
 *
 * @see vsm_zone_get_netdevs, vsm_lookup_netdev_by_name,
 * @see vsm_netdev_get_ipv4_addr, vsm_netdev_get_ipv6_addr
 */
void netdev_list(const Args& argv);

/**
 * Parses command line arguments and call vsm_netdev_set_ipv4_addr, vsm_netdev_set_ipv6_addr
 *
 * @see vsm_netdev_set_ipv4_addr, vsm_netdev_set_ipv6_addr
 */
void netdev_add_ip_addr(const Args& argv);

/**
 * Parses command line arguments and call vsm_netdev_del_ipv4_addr, vsm_netdev_del_ipv6_addr
 *
 * @see vsm_netdev_del_ipv4_addr, vsm_netdev_del_ipv6_addr
 */
void netdev_del_ip_addr(const Args& argv);

/**
 * Parses command line arguments and call vsm_netdev_up
 *
 * @see vsm_netdev_up
 */
void netdev_up(const Args& argv);

/**
 * Parses command line arguments and call vsm_netdev_down
 *
 * @see vsm_netdev_down
 */
void netdev_down(const Args& argv);

/**
 * Parses command line arguments and call vsm_clean_up_zones_root
 *
 * @see vsm_clean_up_zones_root
 */
void clean_up_zones_root(const Args& argv);

} // namespace cli
} // namespace vasum

#endif /* CLI_COMMAND_LINE_INTERFACE_HPP */
