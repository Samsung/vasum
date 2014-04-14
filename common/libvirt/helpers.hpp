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
 * @brief   A function helpers for the libvirt library
 */

#ifndef COMMON_LIBVIRT_HELPERS_HPP
#define COMMON_LIBVIRT_HELPERS_HPP

#include <string>


namespace security_containers {
namespace libvirt {


/**
 * Initialize libvirt library in a thread safety manner
 */
void libvirtInitialize(void);

/**
 * Formats libvirt's last error.
 */
std::string libvirtFormatError(void);


} // namespace libvirt
} // namespace security_containers


#endif // COMMON_LIBVIRT_HELPERS_HPP
