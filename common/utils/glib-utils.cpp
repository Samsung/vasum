/*
 *  Copyright (c) 2014 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *  Contact: Piotr Bartosiewicz <p.bartosiewi@partner.samsung.com>
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
 * @author  Piotr Bartosiewicz (p.bartosiewi@partner.samsung.com)
 * @brief   C++ wrapper of glib main loop
 */

#include "config.hpp"

#include "utils/glib-utils.hpp"
#include "utils/callback-wrapper.hpp"

#include <glib-object.h>

namespace utils {

namespace {

gboolean onIddle(gpointer data)
{
    const VoidCallback& callback = getCallbackFromPointer<VoidCallback>(data);
    callback();
    return FALSE;
}

} // namespace

void executeInGlibThread(const VoidCallback& callback, const CallbackGuard& guard)
{
    if (!callback) {
        return;
    }
    g_idle_add_full(G_PRIORITY_DEFAULT,
                    &onIddle,
                    utils::createCallbackWrapper(callback, guard.spawn()),
                    &utils::deleteCallbackWrapper<VoidCallback>);

}


} // namespace utils
