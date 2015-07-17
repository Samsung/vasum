/*
 *  Copyright (c) 2014 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *  Contact: Jan Olszak <j.olszak@samsung.com>
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
 * @author  Jan Olszak (j.olszak@samsung.com)
 * @brief   Main file for the Vasum Daemon
 */

// Always log to console in DEBUG mode
#if !defined(LOG_TO_CONSOLE) && !defined(NDEBUG)
#define LOG_TO_CONSOLE
#endif

#include "config.hpp"

#include "exception.hpp"
#include "server.hpp"

#include "logger/logger.hpp"
#include "logger/backend-stderr.hpp"
#include "logger/backend-journal.hpp"
#include "logger/backend-syslog.hpp"
#include "utils/typeinfo.hpp"
#include "utils/signal.hpp"

#include <boost/program_options.hpp>
#include <iostream>

using namespace logger;
using namespace vasum;

namespace po = boost::program_options;


namespace {

const std::string PROGRAM_NAME_AND_VERSION =
    "Vasum Server " PROGRAM_VERSION;

const std::string CONFIG_PATH = "/etc/vasum/daemon.conf";


} // namespace

int main(int argc, char* argv[])
{
    bool runAsRoot = false;

    try {
        po::options_description desc("Allowed options");

        desc.add_options()
        ("help,h", "print this help")
        ("root,r", "Don't drop root privileges at startup")
        ("version,v", "show application version")
        ("log-level,l", po::value<std::string>()->default_value("DEBUG"), "set log level")
        ("check,c", "check runtime environment and exit")
        ;

        po::variables_map vm;
        po::basic_parsed_options< char > parsed =
            po::command_line_parser(argc, argv).options(desc).allow_unregistered().run();

        std::vector<std::string> unrecognized_options =
            po::collect_unrecognized(parsed.options, po::include_positional);

        if (!unrecognized_options.empty()) {
            std::cerr << "Unrecognized options: ";

            for (auto& uo : unrecognized_options) {
                std::cerr << ' ' << uo;
            }

            std::cerr << std::endl << std::endl;
            std::cerr << desc << std::endl;

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
        } else if (vm.count("check")) {
            std::cout << "Checking runtime environment..." << std::endl;
            return Server::checkEnvironment() ? 0 : 1;
        }

        Logger::setLogLevel(vm["log-level"].as<std::string>());
#ifdef LOG_TO_CONSOLE
        Logger::setLogBackend(new StderrBackend());
#elif HAVE_SYSTEMD
        Logger::setLogBackend(new SystemdJournalBackend());
#else
        Logger::setLogBackend(new SyslogBackend());
#endif

        runAsRoot = vm.count("root") > 0;

    } catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }

    try {
        // Block all signals
        // Server will unblock handled signals
        utils::signalBlockAllExcept({SIGTERM});

        // TODO: SIGTERM used by lxc, get rid of this
        utils::signalIgnore({SIGTERM});

        LOGI("Starting daemon...");
        Server server(CONFIG_PATH);
        server.run(runAsRoot);
        server.reloadIfRequired(argv);
        LOGI("Daemon stopped");

    } catch (std::exception& e) {
        LOGE("Unexpected: " << utils::getTypeName(e) << ": " << e.what());
        return 1;
    }

    return 0;
}

