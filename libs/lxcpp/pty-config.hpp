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
 * @brief   PTY terminal configuration
 */

#ifndef LXCPP_PTY_CONFIG_HPP
#define LXCPP_PTY_CONFIG_HPP

#include "cargo/fields.hpp"

#include <vector>
#include <string>


namespace lxcpp {


struct PTYConfig {
    cargo::FileDescriptor mMasterFD;
    std::string mPtsName;

    PTYConfig()
        : mMasterFD(-1)
    {}

    PTYConfig(const int masterFD, const std::string &ptsName)
        : mMasterFD(masterFD),
          mPtsName(ptsName)
    {}

    CARGO_REGISTER
    (
        mMasterFD,
        mPtsName
    )
};

struct PTYsConfig {
    unsigned int mCount;
    uid_t mUID;
    std::string mDevptsPath;
    std::vector<PTYConfig> mPTYs;

    PTYsConfig(const unsigned int count = 1, const uid_t UID = 0, const std::string &devptsPath = "")
        : mCount(count),
          mUID(UID),
          mDevptsPath(devptsPath)
    {}

    CARGO_REGISTER
    (
        mCount,
        mDevptsPath,
        mPTYs
    )
};


} //namespace lxcpp


#endif // LXCPP_PTY_CONFIG_HPP
