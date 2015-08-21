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
 * @author  Mateusz Malicki (m.malicki2@samsung.com)
 * @brief   lxcpp's exceptions definitions
 */

#ifndef LXCPP_EXCEPTION_HPP
#define LXCPP_EXCEPTION_HPP

#include <stdexcept>

namespace lxcpp {

/**
 * Base class for exceptions in lxcpp
 */
struct Exception: public std::runtime_error {
    Exception(const std::string& message)
        : std::runtime_error(message) {}
};

struct NotImplementedException: public Exception {
    NotImplementedException(const std::string& message = "Functionality not yet implemented")
        : Exception(message) {}
};

struct ProcessSetupException: public Exception {
    ProcessSetupException(const std::string& message = "Error during setting up a process")
        : Exception(message) {}
};

struct FileSystemSetupException: public Exception {
    FileSystemSetupException(const std::string& message = "Error during a file system operation")
        : Exception(message) {}
};

struct CapabilitySetupException: public Exception {
    CapabilitySetupException(const std::string& message = "Error during a capability operation")
        : Exception(message) {}
};

struct BadArgument: public Exception {
    BadArgument(const std::string& message = "Bad argument passed")
        : Exception(message) {}
};

struct NetworkException : public Exception {
    NetworkException (const std::string& message = "Error during setting up a network")
        : Exception(message) {}
};

} // namespace lxcpp

#endif // LXCPP_EXCEPTION_HPP
