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
#include <security-containers-client.h>

#include <map>
#include <stdexcept>
#include <functional>
#include <ostream>

using namespace std;

namespace security_containers {
namespace cli {

namespace {

/**
 * Invoke specific function on ScClient
 *
 * @param fun Function to be called. It must not throw any exception.
 */
void one_shot(const function<ScStatus(ScClient)>& fun)
{
    string msg;
    ScStatus status;
    ScClient client;

    status = sc_start_glib_loop();
    if (SCCLIENT_SUCCESS != status) {
        throw runtime_error("Can't start glib loop");
    }

    client = sc_client_create();
    if (NULL == client) {
        msg = "Can't create client";
        goto finish;
    }

    status = sc_connect(client);
    if (SCCLIENT_SUCCESS != status) {
        msg = sc_get_status_message(client);
        goto finish;
    }

    status = fun(client);
    if (SCCLIENT_SUCCESS != status) {
        msg = sc_get_status_message(client);
        goto finish;
    }

finish:
    sc_client_free(client);
    sc_stop_glib_loop();
    if (! msg.empty()) {
        throw runtime_error(msg);
    }
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


void set_active_container(int pos, int argc, const char** argv)
{
    using namespace std::placeholders;

    if (argc <= pos + 1) {
        throw runtime_error("Not enough parameters");
    }

    one_shot(bind(sc_set_active_container, _1, argv[pos + 1]));
}

void add_container(int pos, int argc, const char** argv)
{
    using namespace std::placeholders;

    if (argc <= pos + 1) {
        throw runtime_error("Not enough parameters");
    }

    one_shot(bind(sc_add_container, _1, argv[pos + 1]));
}

} // namespace cli
} // namespace security_containers
