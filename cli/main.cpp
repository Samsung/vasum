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
#include <boost/filesystem.hpp>

#include <readline/readline.h>
#include <readline/history.h>

using namespace vasum::cli;

namespace fs =  boost::filesystem;

namespace {

static int interactiveMode = 0;
std::vector<CommandLineInterface> commands = {
    {
        create_zone,
        "create",
        "Create and add zone",
        MODE_COMMAND_LINE | MODE_INTERACTIVE,
        {{"zone_id", "zone name", ""},
         {"[zone_tname]", "optional zone template name", ""}}
    },
    {
        destroy_zone,
        "destroy",
        "Destroy zone",
        MODE_COMMAND_LINE | MODE_INTERACTIVE,
        {{"zone_id", "zone name", "{ZONE}"}}
    },
    {
        start_zone,
        "start",
        "Start zone",
        MODE_COMMAND_LINE | MODE_INTERACTIVE,
        {{"zone_id", "zone name", "{ZONE}"}}
    },
    {
        console_zone,
        "console",
        "Attach to zone text console",
        MODE_COMMAND_LINE | MODE_INTERACTIVE,
        {{"zone_id", "zone name", "{ZONE}"}}
    },
    {
        shutdown_zone,
        "shutdown",
        "Shutdown zone",
        MODE_COMMAND_LINE | MODE_INTERACTIVE,
        {{"zone_id", "zone name", "{ZONE}"}}
    },
    {
        lock_zone,
        "suspend",
        "Suspend (lock) zone",
        MODE_COMMAND_LINE | MODE_INTERACTIVE,
        {{"zone_id", "zone name", "{ZONE}"}}
    },
    {
        unlock_zone,
        "resume",
        "Resume (unlock) zone",
        MODE_COMMAND_LINE | MODE_INTERACTIVE,
        {{"zone_id", "zone name", "{ZONE}"}}
    },
    {
        set_active_zone,
        "set-active",
        "Set active (foreground) zone",
        MODE_COMMAND_LINE | MODE_INTERACTIVE,
        {{"zone_id", "zone name", "{ZONE}"}}
    },
    {
        get_active_zone,
        "get-active",
        "Get active (foreground) zone",
        MODE_COMMAND_LINE | MODE_INTERACTIVE,
        {}
    },
    {
        get_zone_ids,
        "list",
        "Get available zone ids",
        MODE_COMMAND_LINE | MODE_INTERACTIVE,
        {}
    },
    {
        get_zones_status,
        "status",
        "List status for one or all zones (id, state, terminal, root path)",
        MODE_COMMAND_LINE | MODE_INTERACTIVE,
        {{"[zone_id]", "zone name", "{ZONE}"}}
    },
    {
        clean_up_zones_root,
        "clean",
        "Clean up zones root directory",
        MODE_COMMAND_LINE | MODE_INTERACTIVE,
        {}
    },
    {
        grant_device,
        "device-grant",
        "Grants access to the given device",
        MODE_COMMAND_LINE | MODE_INTERACTIVE,
        {{"zone_id", "zone name", "{ZONE}"},
         {"device", "device name", ""}}
    },
    {
        revoke_device,
        "device-revoke",
        "Revokes access to the given device",
        MODE_COMMAND_LINE | MODE_INTERACTIVE,
        {{"zone_id", "zone name", "{ZONE}"},
         {"device", "device name", ""}}
    },
    {
        create_netdev,
        "net-create",
        "Create network interface in zone",
        MODE_COMMAND_LINE | MODE_INTERACTIVE,
        {
         {"zone_id", "zone name", "{ZONE}"},
         {"netdevtype", "interface  type", "macvlan|phys|veth"},
         {"zone_netdev", "interface name (eth0)", "eth0|eth1"},
         {"host_netdev", "bridge name (virbr0)", "virbr0|virbr1"},
         {"mode", "macvlan mode (private, vepa, bridge, passthru)", "private|vepa|bridge|passthru"}}
    },
    {
        destroy_netdev,
        "net-destroy",
        "Destroy netdev in zone",
        MODE_COMMAND_LINE | MODE_INTERACTIVE,
        {{"zone_id", "zone name", "{ZONE}"},
         {"netdev", "interface name (eth0)", "{NETDEV}"}}
    },
    {
        netdev_list,
        "net-list",
        "List network devices in the zone",
        MODE_COMMAND_LINE | MODE_INTERACTIVE,
        {{"zone_id", "zone name", "{ZONE}"},
         {"[netdev]", "interface name (eth0)", "{NETDEV}"}}
    },
    {
        netdev_up,
        "net-up",
        "Setup a network device in the zone up",
        MODE_COMMAND_LINE | MODE_INTERACTIVE,
        {{"zone_id", "zone name", "{ZONE}"},
         {"netdev", "interface name (eth0)", "{NETDEV}"}}
    },
    {
        netdev_down,
        "net-down",
        "Setup a network device in the zone down",
        MODE_COMMAND_LINE | MODE_INTERACTIVE,
        {{"zone_id", "zone name", "{ZONE}"},
         {"netdev", "interface name (eth0)", "{NETDEV}"}}
    },
    {
        netdev_add_ip_addr,
        "net-ip-add",
        "Add ip/mask address to network interface",
        MODE_COMMAND_LINE | MODE_INTERACTIVE,
        {{"zone_id", "zone name", "{ZONE}"},
         {"netdev", "interface name (eth0)", "{NETDEV}"},
         {"ip", "address IPv4 or IPv6", ""},
         {"prefix", "mask length in bits", "24"}}
    },
    {
        netdev_del_ip_addr,
        "net-ip-del",
        "Del ip/mask address from network interface",
        MODE_COMMAND_LINE | MODE_INTERACTIVE,
        {{"zone_id", "zone name", "{ZONE}"},
         {"netdev", "interface name (eth0)", "{NETDEV}"},
         {"ip", "address IPv4 or IPv6", ""},
         {"prefix", "mask length in bits", "24"}}
    },
    {
        lock_queue,
        "qlock",
        "Exclusively lock the command queue",
        MODE_INTERACTIVE,
        {}
    },
    {
        unlock_queue,
        "qunlock",
        "Unlock the queue",
        MODE_INTERACTIVE,
        {}
    },
};

std::map<std::string,const CommandLineInterface> commandMap;

// wrappers for CommandLineInterface

void printUsage(std::ostream& out, const std::string& name, unsigned int mode)
{
    const std::vector<std::string> addLineBefore = {"device-grant", "net-create", "qlock"};
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
        if (command.isAvailable(mode)) {
            if (std::find(addLineBefore.begin(), addLineBefore.end(), command.getName()) != addLineBefore.end()) {
                out << std::endl;
            }
            out << "   " << std::setw(25) << std::left << command.getName()
                << command.getDescription() << std::endl;
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
    auto pair = commandMap.find(argv[0]);
    if (pair == commandMap.end()) {
        return EXIT_FAILURE;
    }

    const CommandLineInterface& command = pair->second;
    if (!command.isAvailable(mode)) {
        std::cerr << "Command not available in this mode" << std::endl;
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
const std::vector<std::string> buildComplList(const Args& argv);
char *completion_generator(const char* text, int state)
{
    static std::vector<std::string> list;
    static unsigned index = 0;
    if (state == 0) {
        list.clear();
        index = 0;

        char *ln = rl_line_buffer;
        std::istringstream iss(ln);
        Args argv{std::istream_iterator<std::string>{iss},
                  std::istream_iterator<std::string>{}};

        size_t len = strlen(text);
        if (len == 0 && argv.size() > 0) {
            argv.push_back("");
        }

        const std::vector<std::string>& l = buildComplList(argv);
        for (const auto &i : l) {
            if (strncmp(text, i.c_str(), len) == 0) {
                list.push_back(i);
            }
        }
    }
    if (index < list.size()) {
        return ::strdup(list[index++].c_str());
    }

    return NULL;
}

char** completion(const char* text, int /*start*/, int /*end*/)
{
    ::rl_attempted_completion_over = 1; //disable default completion
    return ::rl_completion_matches(text, &completion_generator);
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

    int rc = EXIT_FAILURE;
    std::string ln;
    while (readline_from(">>> ", stream, ln)) {
        if (ln.empty() || ln[0] == '#') { //skip empty line or comment
             continue;
        }

        std::istringstream iss(ln);
        Args argv{std::istream_iterator<std::string>{iss},
                  std::istream_iterator<std::string>{}};

        if (commandMap.count(argv[0]) == 0) {
            printUsage(std::cout, "", MODE_INTERACTIVE);
            continue;
        }

        rc = executeCommand(argv, MODE_INTERACTIVE);
        if (rc == EXIT_FAILURE && !interactiveMode) {
            break;
        }
    }

    return rc;
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

void printList(const std::vector<std::string>& list)
{
    for (const auto& i : list) {
        std::cout << i << std::endl;
    }
}

const std::vector<std::string> buildComplList(const Args& argv)
{
    if (argv.size() < 2) {
        std::vector<std::string> list;
        for (const auto& command : commands) {
            if (command.isAvailable(MODE_COMMAND_LINE)) {
                list.push_back(command.getName());
            }
        }
        return list;
    } else {
        std::string cmd = argv[0];
        if (commandMap.find(cmd) != commandMap.end()) {
            return commandMap[cmd].buildCompletionList(argv);
        }
        return std::vector<std::string>();
    }
}

int bashComplMode(int argc, const char *argv[])
{
    int rc = EXIT_FAILURE;
    try {
        Args args(argv, argv + argc);
        printList(buildComplList(args));
        rc = EXIT_SUCCESS;
    } catch (const std::runtime_error& ex) {
    }

    return rc;
}

int cliMode(const int argc, const char** argv)
{
    if (std::string(argv[1]) == "-h") {
        printUsage(std::cout, argv[0], MODE_COMMAND_LINE);
        return EXIT_SUCCESS;
    }

    if (commandMap.find(argv[1]) == commandMap.end()) {
        printUsage(std::cout, argv[0], MODE_COMMAND_LINE);
        return EXIT_FAILURE;
    }

    // pass all the arguments excluding argv[0] - the executable name
    Args commandArgs(argv + 1, argv + argc);
    int rc = executeCommand(commandArgs, MODE_COMMAND_LINE);

    return rc;
}

fs::path getHomePath() {
    const char *h = ::getenv("HOME");
    return fs::path(h ? h : "");
}


} // namespace


int main(const int argc, const char *argv[])
{
    for (const auto& command : commands) {
        commandMap.insert(std::pair<std::string,const CommandLineInterface>(command.getName(),command));
    }

    int rc = EXIT_FAILURE;
    if (argc > 1) {

        //process arguments
        if (std::string(argv[1]) == "--bash-completion") {
            rc = bashComplMode(argc - 2, argv + 2);
        } else if (std::string(argv[1]) == "-f") {
            if (argc < 3) {
                std::cerr << "Filename expected" << std::endl;
                rc = EXIT_FAILURE;
            }
            else {
                rc = processFile(std::string(argv[2]));
            }
        } else {
            rc = cliMode(argc, argv);
        }

    } else {
        fs::path historyfile(".vsm_history");

        if (isatty(0) == 1) {
            fs::path home = getHomePath();
            if (!home.empty()) {
                historyfile = home / historyfile;
            }
            ::read_history(historyfile.c_str());

            interactiveMode = 1;
            ::rl_attempted_completion_function = completion;
        }

        rc = processStream(std::cin);

        if (interactiveMode) {
            ::write_history(historyfile.c_str());
        }
    }

    disconnect();
    return rc;
}
