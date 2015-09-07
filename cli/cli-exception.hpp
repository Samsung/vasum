/*
 *  Copyright (c) 2015 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *  Contact: Pawel Kubik <p.kubik@samsung.com>
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License
 */

/**
 * @file
 * @author  Pawel Kubik (p.kubik@samsung.com)
 * @brief   Exceptions for the cli
 */


#ifndef CLI_EXCEPTION_HPP
#define CLI_EXCEPTION_HPP

#include "base-exception.hpp"


namespace vasum {
namespace cli {


/**
 * Base class for exceptions in Vasum CLI
 */
struct CliException: public VasumException {

    explicit CliException(const std::string& error) : VasumException(error) {}
};

struct IOException: public CliException {

    explicit IOException(const std::string& error) : CliException(error) {}
};

} // namespace cli
} // namespace vasum

#endif // CLI_EXCEPTION_HPP
