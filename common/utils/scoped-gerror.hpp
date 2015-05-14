/*
 *  Copyright (c) 2014 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *  Contact: Jan Olszak <j.olszak@samsung.com>
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
 * @brief   Declaration of the wrapper for GError
 */

#ifndef COMMON_SCOPED_GERROR_HPP
#define COMMON_SCOPED_GERROR_HPP

#include <iostream>
#include <gio/gio.h>

namespace utils {

class ScopedGError {
public:
    ScopedGError();
    ScopedGError(ScopedGError&&);
    ~ScopedGError();

    ScopedGError(const ScopedGError&) = delete;
    ScopedGError& operator=(const ScopedGError&) = delete;

    /**
     * Strip the error
     */
    bool strip();

    /**
     * Is error pointer NULL?
     */
    operator bool () const;

    /**
     * @return pointer to the GError
     */
    GError** operator& ();

    /**
     * @return  the GError
     */
    const GError* operator->() const;

    /**
     * Writes out the error message
     * @param os the output stream
     * @param e  error to write out
     */
    friend std::ostream& operator<<(std::ostream& os, const ScopedGError& e);

private:
    GError* mError;

};

} // namespace utils

#endif // COMMON_SCOPED_GERROR_HPP
