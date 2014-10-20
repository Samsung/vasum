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
#ifndef CLI_COMMAND_LINE_INTERFACE_HPP
#define CLI_COMMAND_LINE_INTERFACE_HPP

#include <map>
#include <functional>
#include <ostream>
#include <string>

namespace security_containers {
namespace cli {

/**
 * Class that implements command pattern.
 */
class CommandLineInterface {

public:
    /**
     * @see CommandLineInterface::execute
     */
    typedef std::function<void(int, int, const char**)> ExecutorCallback;

    /**
     * @see CommandLineInterface::CommandLineInterface
     */
    typedef std::map<std::string, std::string> ArgsSpec;

    /**
     * Dummy constructor (for stl usage)
     */
    CommandLineInterface() {}

    /**
     *  Construct command
     *
     *  @param executorCallback Callback function that will do the job
     *  @param usage Description of use
     *  @param usageInfo Description of the command
     *  @param argsSpec Description of arguments
     */
    CommandLineInterface(const ExecutorCallback& executorCallback,
                    const std::string& usage,
                    const std::string& usageInfo,
                    const ArgsSpec& argsSpec)
        : mExecutorCallback(executorCallback),
          mUsage(usage),
          mUsageInfo(usageInfo),
          mArgsSpec(argsSpec) {}

    /**
     * Print usage to stream
     *
     * @param out Output stream
     */
    void printUsage(std::ostream& out) const;

    /**
     * Do the work
     *
     * It calls the callback passed in constructor
     *
     * @param pos Points to element in argv where command was recognized (i.e. command name)
     * @param argc Number of elements in argv
     * @param argv Command line arguments
     */
    void execute(int pos, int argc, const char** argv);


private:
    const ExecutorCallback mExecutorCallback;
    const std::string mUsage;
    const std::string mUsageInfo;
    const ArgsSpec mArgsSpec;
};

/**
 * Parses command line arguments and call sc_set_active_container
 *
 * @see sc_set_active_container
 */
void set_active_container(int pos, int argc, const char** argv);

/**
 * Parses command line arguments and call sc_add_container
 *
 * @see sc_add_container
 */
void add_container(int pos, int argc, const char** argv);

} // namespace cli
} // namespace security_containers

#endif /* CLI_COMMAND_LINE_INTERFACE_HPP */
