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
 * @author  Lukasz Pawelczyk (l.pawelczyk@samsung.com)
 * @brief   Main file for the Guard process (libexec)
 */

#include "lxcpp/guard/guard.hpp"

#include "utils/fd-utils.hpp"

#include <iostream>
#include <unistd.h>

int main(int argc, char *argv[])
{
    if (argc == 1) {
        std::cout << "This file should not be executed by hand" << std::endl;
        ::_exit(EXIT_FAILURE);
    }

    // NOTE: this might not be required now, but I leave it here not to forget.
    // We need to investigate this with vasum and think about possibility of
    // poorly written software that leaks file descriptors and might use LXCPP.
#if 0
    for(int fd = 3; fd < ::sysconf(_SC_OPEN_MAX); ++fd) {
        if (fd != channelFD) {
            utils::close(fd);
        }
    }
#endif

    try {
        int fd = std::stoi(argv[1]);
        lxcpp::Guard guard(fd);
        return guard.execute();
    } catch(std::exception& e) {
        // LOGE("Unexpected: " << utils::getTypeName(e) << ": " << e.what());
        return EXIT_FAILURE;
    }
}
