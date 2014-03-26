/*
 *  Copyright (c) 2000 - 2014 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *  Contact: Bumjin Im <bj.im@samsung.com>
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
 * @file    main.cpp
 * @author  Jan Olszak (j.olszak@samsung.com)
 * @brief   Main file for the Security Containers Daemon
 */

#include "utils-glib-loop.hpp"
#include "utils-latch.hpp"
#include "log.hpp"
#include <boost/program_options.hpp>
#include <iostream>
#include <signal.h>

namespace security_containers {

namespace {

utils::Latch signalLatch;

void signalHandler(int sig)
{
    LOGI("Got signal " << sig);
    signalLatch.set();
}

void runDaemon()
{
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    LOGI("Starting daemon...");
    {
        utils::ScopedGlibLoop loop;
        //TODO bootstrap
        LOGI("Daemon started");
        signalLatch.wait();
        LOGI("Stopping daemon...");
    }
    LOGI("Daemon stopped");
}

} // namespace

} // namespace security_containers

namespace {

const std::string PROGRAM_NAME_AND_VERSION =
    "Security Containers Server " PROGRAM_VERSION;

} // namespace

namespace po = boost::program_options;

int main(int argc, char* argv[])
{
    po::options_description desc("Allowed options");

    desc.add_options()
    ("help,h", "print this help")
    ("version,v", "show application version")
    ;

    po::variables_map vm;
    po::basic_parsed_options< char > parsed =
        po::command_line_parser(argc, argv).options(desc).allow_unregistered().run();

    std::vector<std::string> unrecognized_options =
        po::collect_unrecognized(parsed.options, po::include_positional);

    if (!unrecognized_options.empty()) {
        std::cout << "Unrecognized options: ";

        for (auto& uo : unrecognized_options) {
            std::cout << ' ' << uo;
        }

        std::cout << std::endl << std::endl;
        std::cout << desc << std::endl;

        return 1;
    }

    po::store(parsed, vm);
    po::notify(vm);

    if (vm.count("help")) {
        std::cout << desc << std::endl;
        return 0;
    } else if (vm.count("version")) {
        std::cout << PROGRAM_NAME_AND_VERSION << std::endl;
        return 0;
    }

    security_containers::runDaemon();

    return 0;
}

