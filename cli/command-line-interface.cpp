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
 * @brief   Definition of CommandLineInterface class
 */

#include "config.hpp"
#include "command-line-interface.hpp"
#include <vasum-client.h>

#include <map>
#include <stdexcept>
#include <functional>
#include <ostream>
#include <iostream>

using namespace std;

namespace vasum {
namespace cli {

namespace {

/**
 * Invoke specific function on VsmClient
 *
 * @param fun Function to be called. It must not throw any exception.
 */
void one_shot(const function<VsmStatus(VsmClient)>& fun)
{
    string msg;
    VsmStatus status;
    VsmClient client;

    status = vsm_start_glib_loop();
    if (VSMCLIENT_SUCCESS != status) {
        throw runtime_error("Can't start glib loop");
    }

    client = vsm_client_create();
    if (NULL == client) {
        msg = "Can't create client";
        goto finish;
    }

    status = vsm_connect(client);
    if (VSMCLIENT_SUCCESS != status) {
        msg = vsm_get_status_message(client);
        goto finish;
    }

    status = fun(client);
    if (VSMCLIENT_SUCCESS != status) {
        msg = vsm_get_status_message(client);
        goto finish;
    }

finish:
    vsm_client_free(client);
    vsm_stop_glib_loop();
    if (! msg.empty()) {
        throw runtime_error(msg);
    }
}

ostream& operator<<(ostream& out, const VsmZoneState& state)
{
    const char* name;
    switch (state) {
        case STOPPED: name = "STOPPED"; break;
        case STARTING: name = "STARTING"; break;
        case RUNNING: name = "RUNNING"; break;
        case STOPPING: name = "STOPPING"; break;
        case ABORTING: name = "ABORTING"; break;
        case FREEZING: name = "FREEZING"; break;
        case FROZEN: name = "FROZEN"; break;
        case THAWED: name = "THAWED"; break;
        case LOCKED: name = "LOCKED"; break;
        case MAX_STATE: name = "MAX_STATE"; break;
        case ACTIVATING: name = "ACTIVATING"; break;
        default: name = "MAX_STATE (ERROR)";
    }

    out << name;
    return out;
}

ostream& operator<<(ostream& out, const VsmZone& zone)
{
    out << "Name: " << zone->id
        << "\nTerminal: " << zone->terminal
        << "\nState: " << zone->state
        << "\nRoot: " << zone->rootfs_path;
    return out;
}

} // namespace

void CommandLineInterface::printUsage(std::ostream& out) const
{
    out << mUsage << "\n\n"
        << "\tDescription\n"
        << "\t\t" << mUsageInfo << "\n\n"
        << "\tOptions\n";
    for (const auto& args : mArgsSpec) {
        out << "\t\t" << args.first << " -- " << args.second << "\n";
    }
    out << "\n";
}

void CommandLineInterface::execute(int pos, int argc, const char** argv)
{
    mExecutorCallback(pos, argc, argv);
}


void set_active_zone(int pos, int argc, const char** argv)
{
    using namespace std::placeholders;

    if (argc <= pos + 1) {
        throw runtime_error("Not enough parameters");
    }

    one_shot(bind(vsm_set_active_zone, _1, argv[pos + 1]));
}

void create_zone(int pos, int argc, const char** argv)
{
    using namespace std::placeholders;

    if (argc <= pos + 1) {
        throw runtime_error("Not enough parameters");
    }

    one_shot(bind(vsm_create_zone, _1, argv[pos + 1], nullptr));
}

void destroy_zone(int pos, int argc, const char** argv)
{
    using namespace std::placeholders;

    if (argc <= pos + 1) {
        throw runtime_error("Not enough parameters");
    }

    one_shot(bind(vsm_destroy_zone, _1, argv[pos + 1], 1));
}

void lock_zone(int pos, int argc, const char** argv)
{
    using namespace std::placeholders;

    if (argc <= pos + 1) {
        throw runtime_error("Not enough parameters");
    }

    one_shot(bind(vsm_lock_zone, _1, argv[pos + 1]));
}

void unlock_zone(int pos, int argc, const char** argv)
{
    using namespace std::placeholders;

    if (argc <= pos + 1) {
        throw runtime_error("Not enough parameters");
    }

    one_shot(bind(vsm_unlock_zone, _1, argv[pos + 1]));
}

void lookup_zone_by_id(int pos, int argc, const char** argv)
{
    using namespace std::placeholders;
    if (argc <= pos + 1) {
        throw runtime_error("Not enough parameters");
    }

    VsmZone zone;
    one_shot(bind(vsm_lookup_zone_by_id, _1, argv[pos + 1], &zone));
    cout << zone << endl;
    vsm_zone_free(zone);
}

} // namespace cli
} // namespace vasum
