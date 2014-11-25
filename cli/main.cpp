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

using namespace security_containers::cli;

std::map<std::string, CommandLineInterface> commands = {
    {
        "set_active_container", {
            set_active_container,
            "set_active_container container_id",
            "Set active (foreground) container",
            {{"container_id", "id container name"}}
        }
    },
    {
        "create_domain", {
            create_domain,
            "create_domain container_id",
            "Create and add container",
            {{"container_id", "id container name"}}
        }
    },
    {
        "lock_domain", {
            lock_domain,
            "lock_domain container_id",
            "Lock container",
            {{"container_id", "id container name"}}
        }
    },
    {
        "unlock_domain", {
            unlock_domain,
            "unlock_domain container_id",
            "Unlock container",
            {{"container_id", "id container name"}}
        }
    },
    {
        "lookup_domain_by_id", {
            lookup_domain_by_id,
            "lookup_domain_by_id container_id",
            "Prints informations about domain",
            {{"container_id", "id container name"}}
        }
    }
};

void printUsage(std::ostream& out, const std::string& name)
{
    out << "Usage: " << name << " [command [args]]\n\n"
        << "command can be one of the following:\n";

    for (const auto& command : commands) {
        command.second.printUsage(out);
    }
}

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
    try {
        command.execute(1, argc, argv);
    } catch (const std::runtime_error& ex) {
        std::cerr << ex.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

