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
    explicit Exception(const std::string& message)
        : std::runtime_error(message) {}
};

struct NotImplementedException: public Exception {
    explicit NotImplementedException(const std::string& message = "Functionality not yet implemented")
        : Exception(message) {}
};

struct ProcessSetupException: public Exception {
    explicit ProcessSetupException(const std::string& message = "Error while setting up a process")
        : Exception(message) {}
};

struct FileSystemSetupException: public Exception {
    explicit FileSystemSetupException(const std::string& message = "Error during a file system operation")
        : Exception(message) {}
};

struct EnvironmentSetupException: public Exception {
    explicit EnvironmentSetupException(const std::string& message = "Error during handling environment variables")
        : Exception(message) {}
};

struct CredentialSetupException: public Exception {
    explicit CredentialSetupException(const std::string& message = "Error during handling environment variables")
        : Exception(message) {}
};

struct CapabilitySetupException: public Exception {
    explicit CapabilitySetupException(const std::string& message = "Error during a capability operation")
        : Exception(message) {}
};

struct UtilityException: public Exception {
    explicit UtilityException(const std::string& message = "Error during an utility operation")
        : Exception(message) {}
};

struct TerminalException: public Exception {
    explicit TerminalException(const std::string& message = "Error during a terminal operation")
        : Exception(message) {}
};

struct BadArgument: public Exception {
    explicit BadArgument(const std::string& message = "Bad argument passed")
        : Exception(message) {}
};

struct NoSuchValue: public Exception {
    explicit NoSuchValue(const std::string& message = "Value not found")
        : Exception(message) {}
};

struct NetworkException : public Exception {
    explicit NetworkException (const std::string& message = "Error during setting up a network")
        : Exception(message) {}
};

struct ConfigureException: public Exception {
    explicit ConfigureException(const std::string& message = "Error while configuring a container")
        : Exception(message) {}
};

struct ProvisionException: public Exception {
    explicit ProvisionException(const std::string& message = "Provision error")
        : Exception(message) {}
};

} // namespace lxcpp

#endif // LXCPP_EXCEPTION_HPP
