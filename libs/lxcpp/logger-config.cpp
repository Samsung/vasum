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
 * @author  Lukasz Pawelczyk (l.pawelczyk@samsumg.com)
 * @brief   Logger configuration
 */

#include "config.hpp"

#include "lxcpp/logger-config.hpp"
#include "lxcpp/exception.hpp"

namespace lxcpp {


void LoggerConfig::set(const logger::LogType type,
                       const logger::LogLevel level,
                       const std::string &arg)
{
    if (type == logger::LogType::LOG_FILE || type == logger::LogType::LOG_PERSISTENT_FILE) {
        if (arg.empty()) {
            const std::string msg = "Path needs to be specified in the agument";
            LOGE(msg);
            throw BadArgument(msg);
        }
    }

    mType = type;
    mLevel = level;
    mArg = arg;
}


} //namespace lxcpp
