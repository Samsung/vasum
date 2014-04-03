/*
 *  Copyright (c) 2014 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *  Contact: Lukasz Pawelczyk <l.pawelczyk@partner.samsung.com>
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
 * @author  Lukasz Pawelczyk (l.pawelczyk@partner.samsung.com)
 * @brief   Exceptions for the server
 */


#ifndef COMMON_UTILS_EXCEPTION_HPP
#define COMMON_UTILS_EXCEPTION_HPP

#include "base-exception.hpp"


namespace security_containers {


/**
 * Base class for exceptions in utils
 */
struct UtilsException: public SecurityContainersException {
    using SecurityContainersException::SecurityContainersException;
};


}


#endif // COMMON_UTILS_EXCEPTION_HPP
