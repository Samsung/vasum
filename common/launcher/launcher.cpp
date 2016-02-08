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
 * @author  Krzysztof Dynowski (k.dynowski@samsumg.com)
 * @brief   execute function via binary (for use under fork)
 */

#include "config.hpp"

#include "utils/exception.hpp"
#include "utils/fd-utils.hpp"
#include "utils/initctl.hpp"
#include "utils/img.hpp"

#include <boost/filesystem.hpp>
#include <iostream>
#include <string>

#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

namespace {
void assertArgsCount(int n, int argc, const char *argv[])
{
    if (argc != n) {
       throw std::runtime_error(std::string("Wrong number of arguments for command ") + argv[1]);
    }
}

void createFile(int argc, const char *argv[])
{
    assertArgsCount(6, argc, argv);

    int socket = std::stoi(argv[2]);
    const char *path = argv[3];
    int flags = std::stoi(argv[4]);
    int mode = std::stoi(argv[5]);
    int fd = -1;

    try {
        fd = utils::open(path, O_CREAT | O_EXCL | flags, mode);
        utils::fdSend(socket, fd);
    } catch(...) {
        utils::close(socket);
        utils::close(fd);
        throw;
    }
    utils::close(fd);
}

void setRunLevel(int argc, const char *argv[])
{
    assertArgsCount(3, argc, argv);

    if (!utils::setRunLevel(static_cast<utils::RunLevel>(std::stoi(argv[2]))))
    {
        throw std::runtime_error(std::string("Set runlevel: ") + utils::getSystemErrorMessage());
    }
}

void copyImage(int argc, const char *argv[])
{
    assertArgsCount(4, argc, argv);

    std::string zoneImagePath = argv[2];
    std::string zonePathStr = argv[3];

    if (!utils::copyImageContents(zoneImagePath, zonePathStr)) {
        throw std::runtime_error(std::string("Copy conatents: ") + utils::getSystemErrorMessage());
    }
}

void removeAll(int argc, const char *argv[])
{
    assertArgsCount(3, argc, argv);

    namespace fs =  boost::filesystem;
    fs::remove_all(fs::path(argv[2]));
}

} // namespace

int main(int argc, const char *argv[])
{
    if (argc < 2) {
        std::cerr << "This app is not designed to run manually";
        return EXIT_FAILURE;
    }

    try {
        if (std::string("createfile") == argv[1]) {

            createFile(argc, argv);

        } else if (std::string("setrunlevel") == argv[1]) {

            setRunLevel(argc, argv);

        } else if (std::string("copyimage") == argv[1]) {

            copyImage(argc, argv);

        } else if (std::string("removeall") == argv[1]) {

            removeAll(argc, argv);

        } else {
            throw std::runtime_error(std::string("Function not supported: ") + argv[1]);
        }
    } catch (const std::exception& ex) {
        std::cerr << ex.what() << std::endl;
        return EXIT_FAILURE;
    }

    std::cerr << argv[1] << " success" << std::endl;
    return EXIT_SUCCESS;
}
