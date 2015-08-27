/*
 *  Copyright (C) 2015 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License version 2.1 as published by the Free Software Foundation.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

/**
 * @file
 * @author  Jan Olszak (j.olszak@samsung.com)
 * @brief   Handling environment variables
 */

#include "lxcpp/environment.hpp"
#include "lxcpp/exception.hpp"

#include "logger/logger.hpp"
#include "utils/exception.hpp"

#include <stdlib.h>

namespace lxcpp {

void clearenvExcept(const std::vector<std::string>& names)
{
    // Backup keeps pairs (name,value)
    std::vector<std::pair< std::string,std::string>> backup;
    for(const auto& name: names) {
        try {
            backup.push_back({name, lxcpp::getenv(name)});
        } catch(const NoSuchValue&) {
            // Continue if there's no such variable
        }
    }

    lxcpp::clearenv();

    // Restore backup
    lxcpp::setenv(backup);
}

void clearenv()
{
    if(::clearenv()) {
        const std::string msg = "clearenv() failed: " +
                                utils::getSystemErrorMessage();
        LOGE(msg);
        throw EnvironmentSetupException(msg);
    }
}

std::string getenv(const std::string& name)
{
    const char* value = ::getenv(name.c_str());
    if (!value) {
        const std::string msg = "getenv() failed: No such name";
        LOGW(msg);
        throw NoSuchValue(msg);
    }
    return value;
}

void setenv(const std::string& name, const std::string& value)
{
    if (-1 == ::setenv(name.c_str(),
                       value.c_str(),
                       1 /*write if exists*/)) {
        const std::string msg = "setenv() failed. Not all env set. " +
                                utils::getSystemErrorMessage();
        LOGE(msg);
        throw EnvironmentSetupException(msg);
    }
}

void setenv(const std::vector<std::pair<std::string, std::string>>& variables)
{
    for(const auto& variable: variables) {
        lxcpp::setenv(variable.first, variable.second);
    }
}

} // namespace lxcpp
