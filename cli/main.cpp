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

#include "command-line-interface.hpp"

#include <cstdlib>
#include <map>
#include <stdexcept>
#include <string>
#include <iostream>
#include <algorithm>

using namespace vasum::cli;

namespace {

std::map<std::string, CommandLineInterface> commands = {
    {
        "set_active_zone", {
            set_active_zone,
            "set_active_zone zone_id",
            "Set active (foreground) zone",
            {{"zone_id", "id zone name"}}
        }
    },
    {
        "create_zone", {
            create_zone,
            "create_zone zone_id",
            "Create and add zone",
            {{"zone_id", "id zone name"}}
        }
    },
    {
        "destroy_zone", {
            destroy_zone,
            "destroy_zone zone_id",
            "Destroy zone",
            {{"zone_id", "id zone name"}}
        }
    },
    {
        "shutdown_zone", {
            shutdown_zone,
            "shutdown_zone zone_id",
            "Shutdown zone",
            {{"zone_id", "id zone name"}}
        }
    },
    {
        "start_zone", {
            start_zone,
            "start_zone zone_id",
            "Start zone",
            {{"zone_id", "id zone name"}}
        }
    },
    {
        "lock_zone", {
            lock_zone,
            "lock_zone zone_id",
            "Lock zone",
            {{"zone_id", "id zone name"}}
        }
    },
    {
        "unlock_zone", {
            unlock_zone,
            "unlock_zone zone_id",
            "Unlock zone",
            {{"zone_id", "id zone name"}}
        }
    },
    {
        "get_zones_status", {
            get_zones_status,
            "get_zones_status",
            "Get list of zone with some useful informations (id, state, terminal, root path)",
            {}
        }
    },
    {
        "get_zone_ids", {
            get_zone_ids,
            "get_zone_ids",
            "Get all zone ids",
            {}
        }
    },
    {
        "get_active_zone_id", {
            get_active_zone_id,
            "get_active_zone_id",
            "Get active (foreground) zone ids",
            {}
        }
    },
    {
        "lookup_zone_by_id", {
            lookup_zone_by_id,
            "lookup_zone_by_id zone_id",
            "Prints informations about zone",
            {{"zone_id", "id zone name"}}
        }
    },
    {
        "grant_device", {
            grant_device,
            "grant_device zone_id device_name",
            "Grants access to the given device",
            {{"zone_id", "id zone name"},
             {"device_name", " device name"}}
        }
    },
    {
        "revoke_device", {
            revoke_device,
            "revoke_device zone_id device_name",
            "Revokes access to the given device",
            {{"zone_id", "id zone name"},
             {"device_name", " device name"}}
        }
    },
    {
        "create_netdev_veth", {
            create_netdev_veth,
            "create_netdev_veth zone_id zone_netdev_id host_netdev_id",
            "Create netdev in zone",
            {{"zone_id", "id zone name"},
             {"zone_netdev_id", "network device id"},
             {"host_netdev_id", "host bridge id"}}
        }
    },
    {
        "create_netdev_macvlan", {
            create_netdev_macvlan,
            "create_netdev_macvlan zone_id zone_netdev_id host_netdev_id mode",
            "Create netdev in zone",
            {{"zone_id", "id zone name"},
             {"zone_netdev_id", "network device id"},
             {"host_netdev_id", "host bridge id"},
             {"mode", "macvlan mode (private, vepa, bridge, passthru)"}}
        }
    },
    {
        "create_netdev_phys", {
            create_netdev_phys,
            "create_netdev_phys zone_id netdev_id",
            "Create/move netdev to zone",
            {{"zone_id", "id zone name"},
             {"netdev_id", "network device name"}}
        }
    },
    {
        "lookup_netdev_by_name", {
            lookup_netdev_by_name,
            "lookup_netdev_by_name zone_id netdev_id",
            "Get netdev flags",
            {{"zone_id", "id zone name"},
             {"netdev_id", "network device name"}}
        }
    },
    {
        "destroy_netdev", {
            destroy_netdev,
            "destroy_netdev zone_id devId",
            "Destroy netdev in zone",
            {{"zone_id", "id zone name"},
             {"netdev_id", "network device name"}}
        }
    },
    {
        "zone_get_netdevs", {
            zone_get_netdevs,
            "zone_get_netdevs zone_id",
            "List network devices in the zone",
            {{"zone_id", "id zone name"}}
        }
    },
    {
        "netdev_get_ipv4_addr", {
            netdev_get_ipv4_addr,
            "netdev_get_ipv4_addr zone_id netdev_id",
            "Get ipv4 address",
            {{"zone_id", "id zone name"},
             {"netdev_id", "network device name"}}
        }
    },
    {
        "netdev_get_ipv6_addr", {
            netdev_get_ipv6_addr,
            "netdev_get_ipv6_addr zone_id netdev_id",
            "Get ipv6 address",
            {{"zone_id", "id zone name"},
             {"netdev_id", "network device name"}}
        }
    },
    {
        "netdev_set_ipv4_addr", {
            netdev_set_ipv4_addr,
            "netdev_set_ipv4_addr zone_id netdev_id address prefix_len",
            "Set ipv4 address",
            {{"zone_id", "id zone name"},
             {"netdev_id", "network device name"},
             {"address", "ipv4 address"},
             {"prefix_len", "bit length of prefix"}}
        }
    },
    {
        "netdev_set_ipv6_addr", {
            netdev_set_ipv6_addr,
            "netdev_set_ipv6_addr zone_id netdev_id address prefix_len",
            "Set ipv6 address",
            {{"zone_id", "id zone name"},
             {"netdev_id", "network device name"},
             {"address", "ipv6 address"},
             {"prefix_len", "bit length of prefix"}}
        }
    },
    {
        "netdev_up", {
            netdev_up,
            "netdev_up zone_id netdev_id",
            "Turn up a network device in the zone",
            {{"zone_id", "id zone name"},
             {"netdev_id", "network device id"}}
        }
    },
    {
        "netdev_down", {
            netdev_down,
            "netdev_down zone_id netdev_id",
            "Turn down a network device in the zone",
            {{"zone_id", "id zone name"},
             {"netdev_id", "network device id"}}
        }
    },
    {
        "zone_get_netdevs", {
            zone_get_netdevs,
            "zone_get_netdevs zone_id",
            "Turn down a network device in the zone",
            {{"zone_id", "id zone name"},
             {"netdev_id", "network device id"}}
        }
    }
};

void printUsage(std::ostream& out, const std::string& name)
{
    out << "Usage: " << name << " [command [-h|args]]\n\n"
        << "command can be one of the following:\n";

    for (const auto& command : commands) {
        command.second.printUsage(out);
    }
}

} // namespace

int main(const int argc, const char** argv)
{
    if (argc < 2) {
        printUsage(std::cout, argv[0]);
        return EXIT_FAILURE;
    }
    if (commands.count(argv[1]) == 0) {
        printUsage(std::cout, argv[0]);
        return EXIT_FAILURE;
    }

    CommandLineInterface& command = commands[argv[1]];
    auto it = std::find(argv, argv+argc, std::string("-h"));
    if (it != argv + argc) {
            command.printUsage(std::cout);
            return EXIT_SUCCESS;
    }

    try {
        command.execute(1, argc, argv);
    } catch (const std::runtime_error& ex) {
        std::cerr << ex.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

