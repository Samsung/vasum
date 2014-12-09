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
        "lookup_zone_by_id", {
            lookup_zone_by_id,
            "lookup_zone_by_id zone_id",
            "Prints informations about zone",
            {{"zone_id", "id zone name"}}
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
    try {
        command.execute(1, argc, argv);
    } catch (const std::runtime_error& ex) {
        std::cerr << ex.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

