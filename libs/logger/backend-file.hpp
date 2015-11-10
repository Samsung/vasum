/*
 *  Copyright (c) 2015 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *  Contact: Roman Kubiak (r.kubiak@samsung.com)
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
 * @author  Roman Kubiak (r.kubiak@samsung.com)
 * @brief   File backend for logger
 */

#ifndef LOGGER_BACKEND_FILE_HPP
#define LOGGER_BACKEND_FILE_HPP

#include "logger/backend.hpp"

namespace logger {

class FileBackend : public LogBackend {
public:
    FileBackend(const std::string& filePath) : mfilePath(filePath) {}
    void log(LogLevel logLevel,
             const std::string& file,
             const unsigned int& line,
             const std::string& func,
             const std::string& message) override;
private:
    std::string mfilePath;
};

} // namespace logger

#endif // LOGGER_BACKEND_FILE_HPP
