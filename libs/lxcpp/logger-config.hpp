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

#ifndef LXCPP_LOGGER_CONFIG_HPP
#define LXCPP_LOGGER_CONFIG_HPP

#include "config/config.hpp"
#include "config/fields.hpp"
#include "logger/logger.hpp"


namespace lxcpp {


/**
 * Logger configuration
 */
struct LoggerConfig {
private:
    int mType;
    int mLevel;
    std::string mArg;

public:
    void set(const logger::LogType type,
             const logger::LogLevel level,
             const std::string &arg = "");

    logger::LogType getType() const {return static_cast<logger::LogType>(mType);}
    logger::LogLevel getLevel() const {return static_cast<logger::LogLevel>(mLevel);}
    std::string getArg() const {return mArg;}

    CONFIG_REGISTER
    (
        mType,
        mLevel,
        mArg
    )
};


} //namespace lxcpp


#endif // LXCPP_LOGGER_CONFIG_HPP
