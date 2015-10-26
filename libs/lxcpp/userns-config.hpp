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
 * @brief   User namespace configuration
 */

#ifndef LXCPP_USERNS_CONFIG_HPP
#define LXCPP_USERNS_CONFIG_HPP

#include "config/config.hpp"
#include "config/fields.hpp"

#include <vector>
#include <string>


namespace lxcpp {


struct UserNSConfig {
    // TODO: Replace UserNSMap with std::tuple
    struct UserNSMap {
        unsigned min;
        unsigned max;
        unsigned num;

        UserNSMap() = default;
        UserNSMap(unsigned min, unsigned max, unsigned num)
            : min(min), max(max), num(num) {}

        CONFIG_REGISTER
        (
            min,
            max,
            num
        )
    };

    std::vector<UserNSMap> mUIDMaps;
    std::vector<UserNSMap> mGIDMaps;

    CONFIG_REGISTER
    (
        mUIDMaps,
        mGIDMaps
    )
};


} //namespace lxcpp


#endif // LXCPP_USERNS_CONFIG_HPP
