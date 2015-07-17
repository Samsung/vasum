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
#include <istream>
#include <sstream>
#include <fstream>
#include <iterator>
#include <iomanip>
#include <boost/algorithm/string/predicate.hpp>

#include <readline/readline.h>
#include <readline/history.h>

using namespace vasum::cli;

namespace {

static int interactiveMode = 0;
std::map<std::string, CommandLineInterface> commands = {
    {
        "lock_queue", {
            lock_queue,
            "lock_queue",
            "Exclusively lock the command queue",
            MODE_INTERACTIVE,
            {}
        }
    },
    {
        "unlock_queue", {
            unlock_queue,
            "unlock_queue",
            "Unlock the queue",
            MODE_INTERACTIVE,
            {}
        }
    },
    {
        "set_active_zone", {
            set_active_zone,
            "set_active_zone",
            "Set active (foreground) zone",
            MODE_COMMAND_LINE | MODE_INTERACTIVE,
            {{"zone_id", "id zone name"}}
        }
    },
    {
        "create_zone", {
            create_zone,
            "create_zone",
            "Create and add zone",
            MODE_COMMAND_LINE | MODE_INTERACTIVE,
            {{"zone_id", "id zone name"},
             {"[zone_tname]", "optional zone template name"}}
        }
    },
    {
        "destroy_zone", {
            destroy_zone,
            "destroy_zone",
            "Destroy zone",
            MODE_COMMAND_LINE | MODE_INTERACTIVE,
            {{"zone_id", "id zone name"}}
        }
    },
    {
        "shutdown_zone", {
            shutdown_zone,
            "shutdown_zone",
            "Shutdown zone",
            MODE_COMMAND_LINE | MODE_INTERACTIVE,
            {{"zone_id", "id zone name"}}
        }
    },
    {
        "start_zone", {
            start_zone,
            "start_zone",
            "Start zone",
            MODE_COMMAND_LINE | MODE_INTERACTIVE,
            {{"zone_id", "id zone name"}}
        }
    },
    {
        "console_zone", {
            console_zone,
            "console_zone",
            "Log into zone",
            MODE_COMMAND_LINE | MODE_INTERACTIVE,
            {{"zone_id", "id zone name"}}
        }
    },
    {
        "lock_zone", {
            lock_zone,
            "lock_zone",
            "Lock zone",
            MODE_COMMAND_LINE | MODE_INTERACTIVE,
            {{"zone_id", "id zone name"}}
        }
    },
    {
        "unlock_zone", {
            unlock_zone,
            "unlock_zone",
            "Unlock zone",
            MODE_COMMAND_LINE | MODE_INTERACTIVE,
            {{"zone_id", "id zone name"}}
        }
    },
    {
        "get_zones_status", {
            get_zones_status,
            "get_zones_status",
            "Get list of zone with some useful informations (id, state, terminal, root path)",
            MODE_COMMAND_LINE | MODE_INTERACTIVE,
            {}
        }
    },
    {
        "get_zone_ids", {
            get_zone_ids,
            "get_zone_ids",
            "Get all zone ids",
            MODE_COMMAND_LINE | MODE_INTERACTIVE,
            {}
        }
    },
    {
        "get_active_zone_id", {
            get_active_zone_id,
            "get_active_zone_id",
            "Get active (foreground) zone ids",
            MODE_COMMAND_LINE | MODE_INTERACTIVE,
            {}
        }
    },
    {
        "lookup_zone_by_id", {
            lookup_zone_by_id,
            "lookup_zone_by_id",
            "Prints informations about zone",
            MODE_COMMAND_LINE | MODE_INTERACTIVE,
            {{"zone_id", "id zone name"}}
        }
    },
    {
        "grant_device", {
            grant_device,
            "grant_device",
            "Grants access to the given device",
            MODE_COMMAND_LINE | MODE_INTERACTIVE,
            {{"zone_id", "id zone name"},
             {"device_name", " device name"}}
        }
    },
    {
        "revoke_device", {
            revoke_device,
            "revoke_device",
            "Revokes access to the given device",
            MODE_COMMAND_LINE | MODE_INTERACTIVE,
            {{"zone_id", "id zone name"},
             {"device_name", " device name"}}
        }
    },
    {
        "create_netdev_veth", {
            create_netdev_veth,
            "create_netdev_veth",
            "Create netdev in zone",
            MODE_COMMAND_LINE | MODE_INTERACTIVE,
            {{"zone_id", "id zone name"},
             {"zone_netdev_id", "network device id"},
             {"host_netdev_id", "host bridge id"}}
        }
    },
    {
        "create_netdev_macvlan", {
            create_netdev_macvlan,
            "create_netdev_macvlan",
            "Create netdev in zone",
            MODE_COMMAND_LINE | MODE_INTERACTIVE,
            {{"zone_id", "id zone name"},
             {"zone_netdev_id", "network device id"},
             {"host_netdev_id", "host bridge id"},
             {"mode", "macvlan mode (private, vepa, bridge, passthru)"}}
        }
    },
    {
        "create_netdev_phys", {
            create_netdev_phys,
            "create_netdev_phys",
            "Create/move netdev to zone",
            MODE_COMMAND_LINE | MODE_INTERACTIVE,
            {{"zone_id", "id zone name"},
             {"netdev_id", "network device name"}}
        }
    },
    {
        "lookup_netdev_by_name", {
            lookup_netdev_by_name,
            "lookup_netdev_by_name",
            "Get netdev flags",
            MODE_COMMAND_LINE | MODE_INTERACTIVE,
            {{"zone_id", "id zone name"},
             {"netdev_id", "network device name"}}
        }
    },
    {
        "destroy_netdev", {
            destroy_netdev,
            "destroy_netdev",
            "Destroy netdev in zone",
            MODE_COMMAND_LINE | MODE_INTERACTIVE,
            {{"zone_id", "id zone name"},
             {"netdev_id", "network device name"}}
        }
    },
    {
        "zone_get_netdevs", {
            zone_get_netdevs,
            "zone_get_netdevs",
            "List network devices in the zone",
            MODE_COMMAND_LINE | MODE_INTERACTIVE,
            {{"zone_id", "id zone name"}}
        }
    },
    {
        "netdev_get_ipv4_addr", {
            netdev_get_ipv4_addr,
            "netdev_get_ipv4_addr",
            "Get ipv4 address",
            MODE_COMMAND_LINE | MODE_INTERACTIVE,
            {{"zone_id", "id zone name"},
             {"netdev_id", "network device name"}}
        }
    },
    {
        "netdev_get_ipv6_addr", {
            netdev_get_ipv6_addr,
            "netdev_get_ipv6_addr",
            "Get ipv6 address",
            MODE_COMMAND_LINE | MODE_INTERACTIVE,
            {{"zone_id", "id zone name"},
             {"netdev_id", "network device name"}}
        }
    },
    {
        "netdev_set_ipv4_addr", {
            netdev_set_ipv4_addr,
            "netdev_set_ipv4_addr",
            "Set ipv4 address",
            MODE_COMMAND_LINE | MODE_INTERACTIVE,
            {{"zone_id", "id zone name"},
             {"netdev_id", "network device name"},
             {"address", "ipv4 address"},
             {"prefix_len", "bit length of prefix"}}
        }
    },
    {
        "netdev_set_ipv6_addr", {
            netdev_set_ipv6_addr,
            "netdev_set_ipv6_addr",
            "Set ipv6 address",
            MODE_COMMAND_LINE | MODE_INTERACTIVE,
            {{"zone_id", "id zone name"},
             {"netdev_id", "network device name"},
             {"address", "ipv6 address"},
             {"prefix_len", "bit length of prefix"}}
        }
    },
    {
        "netdev_up", {
            netdev_up,
            "netdev_up",
            "Turn up a network device in the zone",
            MODE_COMMAND_LINE | MODE_INTERACTIVE,
            {{"zone_id", "id zone name"},
             {"netdev_id", "network device id"}}
        }
    },
    {
        "netdev_down", {
            netdev_down,
            "netdev_down",
            "Turn down a network device in the zone",
            MODE_COMMAND_LINE | MODE_INTERACTIVE,
            {{"zone_id", "id zone name"},
             {"netdev_id", "network device id"}}
        }
    },
    {
        "zone_get_netdevs", {
            zone_get_netdevs,
            "zone_get_netdevs",
            "Turn down a network device in the zone",
            MODE_COMMAND_LINE | MODE_INTERACTIVE,
            {{"zone_id", "id zone name"},
             {"netdev_id", "network device id"}}
        }
    },
    {
        "clean_up_zones_root", {
            clean_up_zones_root,
            "clean_up_zones_root",
            "Clean up zones root directory",
            MODE_COMMAND_LINE | MODE_INTERACTIVE,
            {}
        }
    }
};

// wrappers for CommandLineInterface

void printUsage(std::ostream& out, const std::string& name, unsigned int mode)
{
    std::string n;
    if (!name.empty()) {
        n = name + " ";
    }

    if (mode == MODE_COMMAND_LINE) {
        out << "Usage: " << n << "[-h|-f <filename>|[<command> [-h|<args>]]\n"
            << "Called without parameters enters interactive mode.\n"
            << "Options:\n"
            << "-h            print help\n"
            << "-f <filename> read and execute commands from file\n\n";
    } else {
        out << "Usage: [-h|<command> [-h|<args>]]\n\n";
    }
    out << "command can be one of the following:\n";

    for (const auto& command : commands) {
        if (command.second.isAvailable(mode)) {
            out << "   " << std::setw(30) << std::left << command.second.getName()
                << command.second.getDescription() << "\n";
        }
    }

    out << "\nSee " << n << "command -h to read about a specific one.\n";
}

int connect()
{
    try {
        CommandLineInterface::connect();
    } catch (const std::runtime_error& ex) {
        std::cerr << ex.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

int disconnect()
{
    try {
        CommandLineInterface::disconnect();
    } catch (const std::runtime_error& ex) {
        std::cerr << ex.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

int executeCommand(const Args& argv, int mode)
{
    auto pair = commands.find(argv[0]);
    if (pair == commands.end()) {
        return EXIT_FAILURE;
    }

    CommandLineInterface& command = pair->second;
    if (!command.isAvailable(mode)) {
        return EXIT_FAILURE;
    }

    auto it = std::find(argv.begin(), argv.end(), std::string("-h"));
    if (it != argv.end()) {
        command.printUsage(std::cout);
        return EXIT_SUCCESS;
    }

    try {
        command.execute(argv);
    } catch (const std::runtime_error& ex) {
        std::cerr << ex.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

// readline completion support

char* object_generator(const char* text, int state)
{
    static std::vector<std::string> objs;
    static size_t len = 0, index = 0;

    if (state == 0) {
        objs.clear();
        len = ::strlen(text);
        index = 0;

        using namespace std::placeholders;
        VsmArrayString ids = NULL;
        try {
            CommandLineInterface::executeCallback(bind(vsm_get_zone_ids, _1, &ids));
        } catch (const std::runtime_error& ex) {
            // Quietly ignore, nothing we can do anyway
        }

        if (ids != NULL) {
            for (VsmString* id = ids; *id; ++id) {
                if (::strncmp(text, *id, len) == 0) {
                    objs.push_back(*id);
                }
            }

            vsm_array_string_free(ids);
        }
    }

    if (index < objs.size()) {
        return ::strdup(objs[index++].c_str());
    }

    return NULL;
}

char* cmd_generator(const char* text, int state)
{
    static std::vector<std::string> cmds;
    static size_t len = 0, index = 0;

    if (state == 0) {
        cmds.clear();
        len = ::strlen(text);
        index = 0;

        for (const auto& command : commands) {
            if (command.second.isAvailable(MODE_INTERACTIVE)) {
                const std::string& cmd = command.second.getName();
                if (::strncmp(text, cmd.c_str(), len) == 0) {
                    cmds.push_back(cmd);
                }
            }
        }
    }

    if (index < cmds.size()) {
        return ::strdup(cmds[index++].c_str());
    }

    return NULL;
}

char** completion(const char* text, int start, int /*end*/)
{
    char **matches = NULL;

    if (start == 0) {
        matches = ::rl_completion_matches(text, &cmd_generator);
    } else {
        matches = ::rl_completion_matches(text, &object_generator);
    }

    return matches;
}

static bool readline_from(const std::string& prompt, std::istream& stream, std::string& ln)
{
    if (interactiveMode) {
        char *cmd = ::readline(prompt.c_str());
        if (cmd == NULL) {
            return false;
        }
        ln = cmd;
        free(cmd);
    } else {
        std::getline(stream, ln);
        if (!stream.good()) {
            return false;
        }
    }

    if (interactiveMode && !ln.empty()) {
        ::add_history(ln.c_str());
    }

    return true;
}

static int processStream(std::istream& stream)
{
    if (connect() != EXIT_SUCCESS) {
        return EXIT_FAILURE;
    }

    std::string ln;
    while (readline_from(">>> ", stream, ln)) {
        if (ln.empty() || ln[0] == '#') { //skip empty line or comment
             continue;
        }

        std::istringstream iss(ln);
        Args argv{std::istream_iterator<std::string>{iss},
                  std::istream_iterator<std::string>{}};

        if (commands.count(argv[0]) == 0) {
            printUsage(std::cout, "", MODE_INTERACTIVE);
            continue;
        }

        executeCommand(argv, MODE_INTERACTIVE);
    }

    disconnect();
    return EXIT_SUCCESS;
}

static int processFile(const std::string& fn)
{
    std::ifstream stream(fn);
    if (!stream.good()) {
        //TODO: Turn on exceptions on stream (and in the entire file)
        std::cerr << "Can't open file " << fn << std::endl;
        return EXIT_FAILURE;
    }

    return processStream(stream);
}

int bashComplMode()
{
    for (const auto& command : commands) {
        if (command.second.isAvailable(MODE_COMMAND_LINE)) {
            std::cout << command.second.getName() << "\n";
        }
    }

    return EXIT_SUCCESS;
}

int cliMode(const int argc, const char** argv)
{
    if (std::string(argv[1]) == "-h") {
        printUsage(std::cout, argv[0], MODE_COMMAND_LINE);
        return EXIT_SUCCESS;
    }

    if (commands.find(argv[1]) == commands.end()) {
        printUsage(std::cout, argv[0], MODE_COMMAND_LINE);
        return EXIT_FAILURE;
    }

    //TODO: Connect when it is necessary
    //f.e. vasum-cli <command> -h doesn't need connection
    if (connect() != EXIT_SUCCESS) {
        return EXIT_FAILURE;
    }

    // pass all the arguments excluding argv[0] - the executable name
    Args commandArgs(argv+1, argv+argc);
    int rc = executeCommand(commandArgs, MODE_COMMAND_LINE);

    disconnect();
    return rc;
}

} // namespace


int main(const int argc, const char *argv[])
{
    if (argc > 1) {
        //process arguments
        if (std::string(argv[1]) == "--bash-completion") {
            return bashComplMode();
        }

        if (std::string(argv[1]) == "-f") {
            if (argc < 3) {
                std::cerr << "Filename expected" << std::endl;
                return EXIT_FAILURE;
            }
            return processFile(std::string(argv[2]));
        }

        return cliMode(argc, argv);
    }

    if (isatty(0) == 1) {
        interactiveMode = 1;
        ::rl_attempted_completion_function = completion;
    }
    return processStream(std::cin);
}
