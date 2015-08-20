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
#include "vasum-client.h"
#include "utils/c-array.hpp"

#include <map>
#include <stdexcept>
#include <functional>
#include <ostream>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <vector>
#include <cassert>
#include <fcntl.h>
#include <linux/if_link.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>

using namespace std;

namespace vasum {
namespace cli {

VsmClient CommandLineInterface::client = NULL;

namespace {

std::string zoneStateToString(const VsmZoneState& state)
{
    std::string name;
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

    return name;
}

std::string netdevTypeToString(const VsmNetdevType& netdevType)
{
    std::string type;
    switch (netdevType) {
        case VSMNETDEV_VETH: type = "VETH"; break;
        case VSMNETDEV_PHYS: type = "PHYS"; break;
        case VSMNETDEV_MACVLAN: type = "MACVLAN"; break;
    }
    return type;
}

std::string netdevToString(const VsmNetdev& netdev)
{
    std::string out = std::string("Name: ") + vsm_netdev_get_name(netdev)
        + std::string("\nType: ") + netdevTypeToString(vsm_netdev_get_type(netdev));
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

void buildZoneList(std::vector<std::string>& list)
{
    using namespace std::placeholders;
    VsmArrayString ids;

    CommandLineInterface::executeCallback(bind(vsm_get_zone_ids, _1, &ids));
    for (VsmString* id = ids; *id; ++id) {
        list.push_back(*id);
    }
    vsm_array_string_free(ids);
}

void buildNetdevList(const std::string& zone,std::vector<std::string>& list)
{
    using namespace std::placeholders;
    VsmArrayString ids;

    CommandLineInterface::executeCallback(bind(vsm_zone_get_netdevs, _1, zone.c_str(), &ids));
    for (VsmString* id = ids; *id; ++id) {
        list.push_back(*id);
    }
    vsm_array_string_free(ids);
}

} // namespace

const std::vector<std::string> CommandLineInterface::buildCompletionList(const Args& a) const
{
    std::vector<std::string> v;

    if (a.size() > mArgsSpec.size() + 1) {
        return v;
    }

    ArgSpec as = mArgsSpec[a.size() - 2];
    string::size_type s = 0U;
    string::size_type e = s;
    while (e != string::npos) {
        e = as.format.find('|', s);
        std::string ss = as.format.substr(s, e - s);
        s = e + 1;

        if (ss == "{ZONE}") {
            buildZoneList(v);
        }
        else if (ss == "{NETDEV}") {
            buildNetdevList(a[a.size()-2],v); // zone name must precede netdev
        }
        else if (ss.length() > 0) {
            v.push_back(ss);
        }
    }

    return v;
}

void CommandLineInterface::connect()
{
    VsmStatus status;
    if (CommandLineInterface::client != nullptr) {
        return;
    }

    CommandLineInterface::client = vsm_client_create();
    if (CommandLineInterface::client == nullptr) {
        throw runtime_error("Can't create client");
    }

    status = vsm_connect(client);
    if (VSMCLIENT_SUCCESS != status) {
        string msg = vsm_get_status_message(CommandLineInterface::client);
        vsm_client_free(CommandLineInterface::client);
        CommandLineInterface::client = nullptr;
        throw runtime_error(msg);
    }
}

void CommandLineInterface::disconnect()
{
    string msg;
    VsmStatus status;

    if (CommandLineInterface::client == nullptr) {
        return ;
    }

    status = vsm_disconnect(CommandLineInterface::client);
    if (VSMCLIENT_SUCCESS != status) {
        msg = vsm_get_status_message(CommandLineInterface::client);
    }

    vsm_client_free(CommandLineInterface::client);
    CommandLineInterface::client = nullptr;

    if (VSMCLIENT_SUCCESS != status) {
        throw runtime_error(msg);
    }
}

void CommandLineInterface::executeCallback(const function<VsmStatus(VsmClient)>& fun)
{
    CommandLineInterface::connect();

    VsmStatus status = fun(CommandLineInterface::client);
    if (VSMCLIENT_SUCCESS != status) {
        throw runtime_error(vsm_get_status_message(CommandLineInterface::client));
    }
}

const std::string& CommandLineInterface::getName() const
{
    return mName;
}

const std::string& CommandLineInterface::getDescription() const
{
    return mDescription;
}

void CommandLineInterface::printUsage(std::ostream& out) const
{
    out << "Syntax\n";
    out << "\t" << mName;
    for (const auto& args : mArgsSpec) {
        out << " " << args.name;
    }

    out << "\n\n"
        "Description\n"
        "\t" << mDescription << "\n";

    if (!mArgsSpec.empty()) {
        out << "\n"
            "Options\n";
        for (const auto& args : mArgsSpec) {
            out << "\t" << args.name << " -- ";
            const std::string& d=args.description;
            std::stringstream ss(d);
            std::string item;
            std::getline(ss, item);
            out << item << std::endl;
            while (std::getline(ss, item))
                out << "\t\t" << item << std::endl;
        }
    }
    out << "\n";
}

bool CommandLineInterface::isAvailable(unsigned int mode) const
{
    return (mAvailability & mode) == mode;
}

void CommandLineInterface::execute(const Args& argv) const
{
    mExecutorCallback(argv);
}


void lock_queue(const Args& /*argv*/)
{
    CommandLineInterface::executeCallback(vsm_lock_queue);
}

void unlock_queue(const Args& /*argv*/)
{
    CommandLineInterface::executeCallback(vsm_unlock_queue);
}

void set_active_zone(const Args& argv)
{
    using namespace std::placeholders;

    if (argv.size() < 2) {
        throw runtime_error("Not enough parameters");
    }

    CommandLineInterface::executeCallback(bind(vsm_set_active_zone, _1, argv[1].c_str()));
}

void create_zone(const Args& argv)
{
    using namespace std::placeholders;

    if (argv.size() < 2) {
        throw runtime_error("Not enough parameters");
    }

    if (argv.size() >= 3 && !argv[2].empty()) {
        CommandLineInterface::executeCallback(bind(vsm_create_zone, _1, argv[1].c_str(), argv[2].c_str()));
    } else {
        CommandLineInterface::executeCallback(bind(vsm_create_zone, _1, argv[1].c_str(), nullptr));
    }
}

void destroy_zone(const Args& argv)
{
    using namespace std::placeholders;

    if (argv.size() < 2) {
        throw runtime_error("Not enough parameters");
    }

    CommandLineInterface::executeCallback(bind(vsm_destroy_zone, _1, argv[1].c_str(), 1));
}

void shutdown_zone(const Args& argv)
{
    using namespace std::placeholders;

    if (argv.size() < 2) {
        throw runtime_error("Not enough parameters");
    }

    CommandLineInterface::executeCallback(bind(vsm_shutdown_zone, _1, argv[1].c_str()));
}

void start_zone(const Args& argv)
{
    using namespace std::placeholders;

    if (argv.size() < 2) {
        throw runtime_error("Not enough parameters");
    }

    CommandLineInterface::executeCallback(bind(vsm_start_zone, _1, argv[1].c_str()));
}

void console_zone(const Args& argv)
{
    using namespace std::placeholders;

    if (argv.size() < 2) {
        throw runtime_error("Not enough parameters");
    }

    VsmZone zone;
    CommandLineInterface::executeCallback(bind(vsm_lookup_zone_by_id, _1, argv[1].c_str(), &zone));

    if (zoneStateToString(vsm_zone_get_state(zone)) != "RUNNING") {
        vsm_zone_free(zone);
        throw runtime_error("Zone '" + argv[1] + "' is not running");
    }

    std::string zonesPath = vsm_zone_get_rootfs(zone);
    std::string zoneRootFs = argv[1] + "/rootfs";
    zonesPath.erase(zonesPath.length()- zoneRootFs.length(), zoneRootFs.length());

    vsm_zone_free(zone);

    utils::CStringArrayBuilder args;
    args.add("lxc-console")
        .add("-t 0")
        .add("-n").add(argv[1].c_str())
        .add("-P").add(zonesPath.c_str());

    if (!execv("/usr/bin/lxc-console", const_cast<char* const*>(args.c_array()))) {
        throw runtime_error("Could not log into zone");
    }
}

void lock_zone(const Args& argv)
{
    using namespace std::placeholders;

    if (argv.size() < 2) {
        throw runtime_error("Not enough parameters");
    }

    CommandLineInterface::executeCallback(bind(vsm_lock_zone, _1, argv[1].c_str()));
}

void unlock_zone(const Args& argv)
{
    using namespace std::placeholders;

    if (argv.size() < 2) {
        throw runtime_error("Not enough parameters");
    }

    CommandLineInterface::executeCallback(bind(vsm_unlock_zone, _1, argv[1].c_str()));
}

void get_zones_status(const Args& argv)
{
    using namespace std::placeholders;

    VsmArrayString ids;
    VsmString activeId;
    Table table;

    if (argv.size() < 2) {
        CommandLineInterface::executeCallback(bind(vsm_get_zone_ids, _1, &ids));
    }
    else {
        ids = reinterpret_cast<char**>(calloc(argv.size(), sizeof(char*)));
        for (unsigned i = 1; i<argv.size(); ++i) {
            ids[i - 1] = ::strdup(argv[i].c_str());
        }
    }


    CommandLineInterface::executeCallback(bind(vsm_get_active_zone_id, _1, &activeId));
    table.push_back({"Active", "Id", "State", "Terminal", "Root"});
    for (VsmString* id = ids; *id; ++id) {
        VsmZone zone;
        CommandLineInterface::executeCallback(bind(vsm_lookup_zone_by_id, _1, *id, &zone));
        assert(string(vsm_zone_get_id(zone)) == string(*id));
        table.push_back({string(vsm_zone_get_id(zone)) == string(activeId) ? "YES" : "NO",
                         vsm_zone_get_id(zone),
                         zoneStateToString(vsm_zone_get_state(zone)),
                         to_string(vsm_zone_get_terminal(zone)),
                         vsm_zone_get_rootfs(zone)});
        vsm_zone_free(zone);
    }
    vsm_string_free(activeId);
    vsm_array_string_free(ids);
    cout << table << endl;
}

void get_zone_ids(const Args& /*argv*/)
{
    using namespace std::placeholders;

    VsmArrayString ids;
    CommandLineInterface::executeCallback(bind(vsm_get_zone_ids, _1, &ids));
    string delim;
    for (VsmString* id = ids; *id; ++id) {
        cout << delim << *id;
        delim = ", ";
    }
    cout << endl;
    vsm_array_string_free(ids);
}

void get_active_zone(const Args& /*argv*/)
{
    using namespace std::placeholders;

    VsmString id;
    CommandLineInterface::executeCallback(bind(vsm_get_active_zone_id, _1, &id));
    cout << id << endl;
    vsm_string_free(id);
}

void grant_device(const Args& argv)
{
    using namespace std::placeholders;

    if (argv.size() < 3) {
        throw runtime_error("Not enough parameters");
    }

    uint32_t flags = O_RDWR;
    CommandLineInterface::executeCallback(bind(vsm_grant_device, _1, argv[1].c_str(), argv[2].c_str(), flags));
}

void revoke_device(const Args& argv)
{
    using namespace std::placeholders;

    if (argv.size() < 3) {
        throw runtime_error("Not enough parameters");
    }

    CommandLineInterface::executeCallback(bind(vsm_revoke_device, _1, argv[1].c_str(), argv[2].c_str()));
}

void create_netdev(const Args& argv)
{
    using namespace std::placeholders;

    if (argv.size() < 3) {
        throw runtime_error("Not enough parameters");
    }

    std::string nettype = argv[2];
    if (nettype == "phys") {
        if (argv.size() < 4) {
            throw runtime_error("Not enough parameters");
        }
        CommandLineInterface::executeCallback(bind(vsm_create_netdev_phys,
                  _1,
                  argv[1].c_str(),
                  argv[3].c_str()));
    }
    else if (nettype == "veth") {
        if (argv.size() < 5) {
            throw runtime_error("Not enough parameters");
        }
        CommandLineInterface::executeCallback(bind(vsm_create_netdev_veth,
                  _1,
                  argv[1].c_str(),
                  argv[3].c_str(),
                  argv[4].c_str()));
    }
    else if (nettype == "macvlan") {
        if (argv.size() < 6) {
            throw runtime_error("Not enough parameters");
        }
        CommandLineInterface::executeCallback(bind(vsm_create_netdev_macvlan,
                  _1,
                  argv[1].c_str(),
                  argv[3].c_str(),
                  argv[4].c_str(),
                  macvlanFromString(argv[5].c_str())));
    }
    else
        throw runtime_error("Wrong nettype option " + nettype);
}

void destroy_netdev(const Args& argv)
{
    using namespace std::placeholders;

    if (argv.size() < 3) {
        throw runtime_error("Not enough parameters");
    }
    CommandLineInterface::executeCallback(bind(vsm_destroy_netdev,
                  _1,
                  argv[1].c_str(),
                  argv[2].c_str()));
}

void netdev_list(const Args& argv)
{
    using namespace std::placeholders;

    if (argv.size() < 2) {
        throw runtime_error("Not enough parameters");
    }
    if (argv.size() < 3) {
        VsmArrayString ids;
        CommandLineInterface::executeCallback(bind(vsm_zone_get_netdevs,
                  _1,
                  argv[1].c_str(),
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
    else {
        VsmNetdev netdev = NULL;
        char buf[INET_ADDRSTRLEN|INET6_ADDRSTRLEN];
        CommandLineInterface::executeCallback(bind(vsm_lookup_netdev_by_name,
                  _1,
                  argv[1].c_str(),
                  argv[2].c_str(),
                  &netdev));
        cout << netdevToString(netdev) << endl;
        vsm_netdev_free(netdev);

        VsmAddrList addrs = NULL;
        CommandLineInterface::executeCallback(bind(vsm_netdev_get_ip_addr,
                  _1,
                  argv[1].c_str(),
                  argv[2].c_str(),
                  &addrs));
        unsigned listsize = vsm_addrlist_size(addrs);
        for (unsigned i=0; i < listsize; ++i) {
            int type=vsm_addrlist_get_type(addrs, i);
            if (inet_ntop(type, vsm_addrlist_get_addr(addrs, i), buf, INET6_ADDRSTRLEN) == NULL) {
                throw runtime_error("Wrong address received ["+std::to_string(i)+"] type="+std::to_string(type));
            }
            cout << buf << "/" << vsm_addrlist_get_prefix(addrs, i) << endl;
        }
        vsm_addrlist_free(addrs);
    }
}

void netdev_add_ip_addr(const Args& argv)
{
    using namespace std::placeholders;
    if (argv.size() < 5) {
        throw runtime_error("Not enough parameters");
    }
    if (argv[3].find(':') == std::string::npos) {
        in_addr addr;
        if (inet_pton(AF_INET, argv[3].c_str(), &addr) != 1) {
            throw runtime_error("Wrong address format");
        };
        CommandLineInterface::executeCallback(bind(vsm_netdev_add_ipv4_addr,
                  _1,
                  argv[1].c_str(),
                  argv[2].c_str(),
                  &addr,
                  stoi(argv[4].c_str())));
    }
    else {
        in6_addr addr;
        if (inet_pton(AF_INET6, argv[3].c_str(), &addr) != 1) {
            throw runtime_error("Wrong address format");
        };
        CommandLineInterface::executeCallback(bind(vsm_netdev_add_ipv6_addr,
                  _1,
                  argv[1].c_str(),
                  argv[2].c_str(),
                  &addr,
                  stoi(argv[4].c_str())));
    }
}

void netdev_del_ip_addr(const Args& argv)
{
    using namespace std::placeholders;
    if (argv.size() < 5) {
        throw runtime_error("Not enough parameters");
    }
    if (argv[3].find(':') == std::string::npos) {
        in_addr addr;
        if (inet_pton(AF_INET, argv[3].c_str(), &addr) != 1) {
            throw runtime_error("Wrong address format");
        };
        CommandLineInterface::executeCallback(bind(vsm_netdev_del_ipv4_addr,
                  _1,
                  argv[1].c_str(),
                  argv[2].c_str(),
                  &addr,
                  stoi(argv[4].c_str())));
    }
    else {
        in6_addr addr;
        if (inet_pton(AF_INET6, argv[3].c_str(), &addr) != 1) {
            throw runtime_error("Wrong address format");
        };
        CommandLineInterface::executeCallback(bind(vsm_netdev_del_ipv6_addr,
                  _1,
                  argv[1].c_str(),
                  argv[2].c_str(),
                  &addr,
                  stoi(argv[4].c_str())));
    }
}

void netdev_up(const Args& argv)
{
    using namespace std::placeholders;

    if (argv.size() < 3) {
        throw runtime_error("Not enough parameters");
    }
    CommandLineInterface::executeCallback(bind(vsm_netdev_up,
                  _1,
                  argv[1].c_str(),
                  argv[2].c_str()));
}

void netdev_down(const Args& argv)
{
    using namespace std::placeholders;

    if (argv.size() < 3) {
        throw runtime_error("Not enough parameters");
    }
    CommandLineInterface::executeCallback(bind(vsm_netdev_down,
                  _1,
                  argv[1].c_str(),
                  argv[2].c_str()));
}

void clean_up_zones_root(const Args& /* argv */)
{
    using namespace std::placeholders;

    CommandLineInterface::executeCallback(bind(vsm_clean_up_zones_root, _1));
}

} // namespace cli
} // namespace vasum
