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
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <vector>
#include <fcntl.h>
#include <cassert>
#include <linux/if_link.h>
#include <arpa/inet.h>

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

template<typename T>
string stringAsInStream(const T& value)
{
    std::ostringstream stream;
    stream << value;
    return stream.str();
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

typedef vector<vector<string>> Table;

ostream& operator<<(ostream& out, const Table& table)
{
    vector<size_t> sizes;
    for (const auto& row : table) {
        if (sizes.size() < row.size()) {
            sizes.resize(row.size());
        }
        for (size_t i = 0; i < row.size(); ++i) {
            sizes[i] = max(sizes[i], row[i].length());
        }
    }

    for (const auto& row : table) {
        for (size_t i = 0; i < row.size(); ++i) {
            out << left << setw(sizes[i]+2) << row[i];
        }
        out << "\n";
    }

    return out;
}

enum macvlan_mode macvlanFromString(const std::string& mode) {
    if (mode == "private") {
        return MACVLAN_MODE_PRIVATE;
    }
    if (mode == "vepa") {
        return MACVLAN_MODE_VEPA;
    }
    if (mode == "bridge") {
        return MACVLAN_MODE_BRIDGE;
    }
    if (mode == "passthru") {
        return MACVLAN_MODE_PASSTHRU;
    }
    throw runtime_error("Unsupported macvlan mode");
}

} // namespace

void CommandLineInterface::printUsage(std::ostream& out) const
{
    out << mUsage << "\n\n"
        << "\tDescription\n"
        << "\t\t" << mUsageInfo << "\n";
    if (!mArgsSpec.empty()) {
        out << "\n\tOptions\n";
        for (const auto& args : mArgsSpec) {
            out << "\t\t" << args.first << " -- " << args.second << "\n";
        }
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

void shutdown_zone(int pos, int argc, const char** argv)
{
    using namespace std::placeholders;

    if (argc <= pos + 1) {
        throw runtime_error("Not enough parameters");
    }

    one_shot(bind(vsm_shutdown_zone, _1, argv[pos + 1]));
}

void start_zone(int pos, int argc, const char** argv)
{
    using namespace std::placeholders;

    if (argc <= pos + 1) {
        throw runtime_error("Not enough parameters");
    }

    one_shot(bind(vsm_start_zone, _1, argv[pos + 1]));
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

void get_zones_status(int /* pos */, int /* argc */, const char** /* argv */)
{
    using namespace std::placeholders;

    VsmArrayString ids;
    VsmString activeId;
    Table table;

    one_shot(bind(vsm_get_zone_ids, _1, &ids));
    one_shot(bind(vsm_get_active_zone_id, _1, &activeId));
    table.push_back({"Active", "Id", "State", "Terminal", "Root"});
    for (VsmString* id = ids; *id; ++id) {
        VsmZone zone;
        one_shot(bind(vsm_lookup_zone_by_id, _1, *id, &zone));
        assert(string(zone->id) == string(*id));
        table.push_back({string(zone->id) == string(activeId) ? "*" : "",
                         zone->id,
                         stringAsInStream(zone->state),
                         to_string(zone->terminal),
                         zone->rootfs_path});
        vsm_zone_free(zone);
    }
    vsm_string_free(activeId);
    vsm_array_string_free(ids);
    cout << table << endl;
}

void get_zone_ids(int /* pos */, int /* argc */, const char** /* argv */)
{
    using namespace std::placeholders;

    VsmArrayString ids;
    one_shot(bind(vsm_get_zone_ids, _1, &ids));
    string delim;
    for (VsmString* id = ids; *id; ++id) {
        cout << delim << *id;
        delim = ", ";
    }
    cout << endl;
    vsm_array_string_free(ids);
}

void get_active_zone_id(int /* pos */, int /* argc */, const char** /* argv */)
{
    using namespace std::placeholders;

    VsmString id;
    one_shot(bind(vsm_get_active_zone_id, _1, &id));
    cout << id << endl;
    vsm_string_free(id);
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

void grant_device(int pos, int argc, const char** argv)
{
    using namespace std::placeholders;

    if (argc <= pos + 2) {
        throw runtime_error("Not enough parameters");
    }

    uint32_t flags = O_RDWR;
    one_shot(bind(vsm_grant_device, _1, argv[pos + 1], argv[pos + 2], flags));
}

void revoke_device(int pos, int argc, const char** argv)
{
    using namespace std::placeholders;

    if (argc <= pos + 2) {
        throw runtime_error("Not enough parameters");
    }

    one_shot(bind(vsm_revoke_device, _1, argv[pos + 1], argv[pos + 2]));
}

void create_netdev_veth(int pos, int argc, const char** argv)
{
    using namespace std::placeholders;

    if (argc <= pos + 3) {
        throw runtime_error("Not enough parameters");
    }
    one_shot(bind(vsm_create_netdev_veth,
                  _1,
                  argv[pos + 1],
                  argv[pos + 2],
                  argv[pos + 3]));
}

void create_netdev_macvlan(int pos, int argc, const char** argv)
{
    using namespace std::placeholders;

    if (argc <= pos + 4) {
        throw runtime_error("Not enough parameters");
    }
    one_shot(bind(vsm_create_netdev_macvlan,
                  _1,
                  argv[pos + 1],
                  argv[pos + 2],
                  argv[pos + 3],
                  macvlanFromString(argv[pos + 4])));
}

void create_netdev_phys(int pos, int argc, const char** argv)
{
    using namespace std::placeholders;

    if (argc <= pos + 2) {
        throw runtime_error("Not enough parameters");
    }
    one_shot(bind(vsm_create_netdev_phys,
                  _1,
                  argv[pos + 1],
                  argv[pos + 2]));
}

void zone_get_netdevs(int pos, int argc, const char** argv)
{
    using namespace std::placeholders;

    if (argc <= pos + 1) {
        throw runtime_error("Not enough parameters");
    }
    VsmArrayString ids;
    one_shot(bind(vsm_zone_get_netdevs,
                  _1,
                  argv[pos + 1],
                  &ids));
    string delim;
    for (VsmString* id = ids; *id; ++id) {
        cout << delim << *id;
        delim = ", ";
    }
    if (delim.empty()) {
        cout << "There is no network device in zone";
    }
    cout << endl;
    vsm_array_string_free(ids);
}

void netdev_get_ipv4_addr(int pos, int argc, const char** argv)
{
    using namespace std::placeholders;

    if (argc <= pos + 2) {
        throw runtime_error("Not enough parameters");
    }
    in_addr addr;
    one_shot(bind(vsm_netdev_get_ipv4_addr,
                  _1,
                  argv[pos + 1],
                  argv[pos + 2],
                  &addr));
    char buf[INET_ADDRSTRLEN];
    if (inet_ntop(AF_INET, &addr, buf, INET_ADDRSTRLEN) == NULL) {
        throw runtime_error("Server gave the wrong address format");
    }
    cout << buf << endl;
}

void netdev_get_ipv6_addr(int pos, int argc, const char** argv)
{
    using namespace std::placeholders;

    if (argc <= pos + 2) {
        throw runtime_error("Not enough parameters");
    }
    in6_addr addr;
    one_shot(bind(vsm_netdev_get_ipv6_addr,
                  _1,
                  argv[pos + 1],
                  argv[pos + 2],
                  &addr));
    char buf[INET6_ADDRSTRLEN];
    if (inet_ntop(AF_INET6, &addr, buf, INET6_ADDRSTRLEN) == NULL) {
        throw runtime_error("Server gave the wrong address format");
    }
    cout << buf << endl;
}

void netdev_set_ipv4_addr(int pos, int argc, const char** argv)
{
    using namespace std::placeholders;

    if (argc <= pos + 4) {
        throw runtime_error("Not enough parameters");
    }
    in_addr addr;
    if (inet_pton(AF_INET, argv[pos + 3], &addr) != 1) {
        throw runtime_error("Wrong address format");
    };
    one_shot(bind(vsm_netdev_set_ipv4_addr,
                  _1,
                  argv[pos + 1],
                  argv[pos + 2],
                  &addr,
                  stoi(argv[pos + 4])));
}

void netdev_set_ipv6_addr(int pos, int argc, const char** argv)
{
    using namespace std::placeholders;

    if (argc <= pos + 4) {
        throw runtime_error("Not enough parameters");
    }
    in6_addr addr;
    if (inet_pton(AF_INET6, argv[pos + 3], &addr) != 1) {
        throw runtime_error("Wrong address format");
    };
    one_shot(bind(vsm_netdev_set_ipv6_addr,
                  _1,
                  argv[pos + 1],
                  argv[pos + 2],
                  &addr,
                  stoi(argv[pos + 4])));
}

void netdev_up(int pos, int argc, const char** argv)
{
    using namespace std::placeholders;

    if (argc <= pos + 2) {
        throw runtime_error("Not enough parameters");
    }
    one_shot(bind(vsm_netdev_up,
                  _1,
                  argv[pos + 1],
                  argv[pos + 2]));
}

void netdev_down(int pos, int argc, const char** argv)
{
    using namespace std::placeholders;

    if (argc <= pos + 2) {
        throw runtime_error("Not enough parameters");
    }
    one_shot(bind(vsm_netdev_down,
                  _1,
                  argv[pos + 1],
                  argv[pos + 2]));
}

} // namespace cli
} // namespace vasum
