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
 * @brief   Eventfd wrapper
 */

#ifndef COMMON_UTILS_EVENTFD_HPP
#define COMMON_UTILS_EVENTFD_HPP

namespace utils {

class EventFD {
public:

    EventFD();
    virtual ~EventFD();

    EventFD(const EventFD& eventfd) = delete;
    EventFD& operator=(const EventFD&) = delete;

    /**
    * @return event's file descriptor.
    */
    int getFD() const;

    /**
     * Send an event of a given value
     */
    void send();

    /**
     * Receives the signal.
     * Blocks if there is no event.
     */
    void receive();

private:
    int mFD;
};

} // namespace utils

#endif // COMMON_UTILS_EVENTFD_HPP
