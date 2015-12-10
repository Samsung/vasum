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
 * @brief   Terminal configuration
 */

#ifndef LXCPP_TERMINAL_CONFIG_HPP
#define LXCPP_TERMINAL_CONFIG_HPP

#include "cargo/fields.hpp"

#include <vector>
#include <string>


namespace lxcpp {


struct TerminalConfig {
    cargo::FileDescriptor masterFD;
    std::string ptsName;

    TerminalConfig()
        : masterFD(-1)
    {}

    TerminalConfig(const int masterFD, const std::string &ptsName)
        : masterFD(masterFD),
          ptsName(ptsName)
    {}

    CARGO_REGISTER
    (
        masterFD,
        ptsName
    )
};

struct TerminalsConfig {
    unsigned int count;
    std::vector<TerminalConfig> PTYs;

    TerminalsConfig(const unsigned int count = 1)
        : count(count)
    {}

    CARGO_REGISTER
    (
        count,
        PTYs
    )
};


} //namespace lxcpp


#endif // LXCPP_TERMINAL_CONFIG_HPP
