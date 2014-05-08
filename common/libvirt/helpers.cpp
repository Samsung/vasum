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

#include "libvirt/helpers.hpp"
#include "log/logger.hpp"

#include <mutex>
#include <libvirt/virterror.h>


namespace security_containers {
namespace libvirt {


namespace {

std::once_flag gInitFlag;

/**
 * This function intentionally is not displaying any errors,
 * we log them ourselves elsewhere.
 * It is however displaying warnings for the time being so we can
 * learn whether such situations occur.
 */
void libvirtErrorFunction(void* /*userData*/, virErrorPtr error)
{
    if (error->level == VIR_ERR_WARNING) {
        LOGW("LIBVIRT reported a warning: \n" << error->message);
    }
}

} // namespace

void libvirtInitialize(void)
{
    std::call_once(gInitFlag, []() {
            virInitialize();
            virSetErrorFunc(NULL, &libvirtErrorFunction);
        });
}


std::string libvirtFormatError(void)
{
    virErrorPtr error = virGetLastError();

    if (error == NULL) {
        return std::string();
    }

    return "Libvirt error: " + std::string(error->message);
}


} // namespace libvirt
} // namespace security_containers
