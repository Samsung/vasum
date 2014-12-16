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
 * @brief   Declaration of a class for communication transport between zone and server
 */


#ifndef SERVER_ZONE_CONNECTION_TRANSPORT_HPP
#define SERVER_ZONE_CONNECTION_TRANSPORT_HPP

#include "dbus/connection.hpp"


namespace vasum {


/**
 * This class provides a communication transport between zone and server.
 * The object lifecycle should cover lifecycle of a zone.
 */
class ZoneConnectionTransport {
public:
    ZoneConnectionTransport(const std::string& runMountPoint);
    ~ZoneConnectionTransport();

    /**
     * Gets dbus addres. Will block until address is available.
     */
    std::string acquireAddress() const;

    /**
     * Set whether object should detach from transport filesystem on exit
     */
    void setDetachOnExit();

private:
    std::string mRunMountPoint;
    bool mDetachOnExit;
};


} // namespace vasum


#endif // SERVER_ZONE_CONNECTION_TRANSPORT_HPP
