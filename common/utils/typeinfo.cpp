/*
 *  Copyright (c) 2014 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *  Contact: Jan Olszak (j.olszak@samsung.com)
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
 * @author  Jan Olszak (j.olszak@samsung.com)
 * @brief   Synchronization latch
 */

#include "config.hpp"

#include "utils/typeinfo.hpp"

#include <string>
#include <cxxabi.h>
#include <stdexcept>

namespace utils {

std::string getTypeName(const std::type_info& ti)
{
    int status;
    char* demangled = abi::__cxa_demangle(ti.name() , 0, 0, &status);
    if (status) {
        std::string message = "abi::__cxa_demangle failed with ret code: " + std::to_string(status);
        throw std::runtime_error(message);
    }

    std::string ret(demangled);
    free(demangled);
    return ret;
}

} // namespace utils
