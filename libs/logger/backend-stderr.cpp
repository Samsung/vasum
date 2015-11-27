/*
 *  Copyright (c) 2014 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *  Contact: Pawel Broda <p.broda@partner.samsung.com>
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
 * @author  Pawel Broda (p.broda@partner.samsung.com)
 * @brief   Stderr backend for logger
 */

#include "config.hpp"

#include "logger/backend-stderr.hpp"
#include "logger/formatter.hpp"

#include <iostream>
#include <boost/tokenizer.hpp>

namespace logger {

void StderrBackend::log(LogLevel logLevel,
                        const std::string& file,
                        const unsigned int& line,
                        const std::string& func,
                        const std::string& message)
{
    typedef boost::char_separator<char> charSeparator;
    typedef boost::tokenizer<charSeparator> tokenizer;

    // example log string
    // 06:52:35.123 [ERROR] src/util/fs.cpp:43 readFileContent: /file/file.txt is missing

    const std::string logColor = LogFormatter::getConsoleColor(logLevel);
    const std::string defaultColor = LogFormatter::getDefaultConsoleColor();
    const std::string header = LogFormatter::getHeader(logLevel, file, line, func);
    tokenizer tokens(message, charSeparator("\n"));
    for (const auto& messageLine : tokens) {
        if (!messageLine.empty()) {
            fprintf(stderr,
                    "%s%s %s%s\n",
                    mUseColours ? logColor.c_str() : "",
                    header.c_str(),
                    messageLine.c_str(),
                    mUseColours ? defaultColor.c_str() : "");
        }
    }
}

void StderrBackend::relog(LogLevel logLevel,
                          const std::string& file,
                          const unsigned int& line,
                          const std::string& func,
                          const std::istream& stream)
{
    const std::string header = LogFormatter::getHeader(logLevel, file, line, func);
    if (stream.good()) {
        std::cerr << "RELOG BEGIN: " << header << std::endl
                  << stream.rdbuf()
                  << "RELOG END: " << header  << std::endl;
    }
}

} // namespace logger
