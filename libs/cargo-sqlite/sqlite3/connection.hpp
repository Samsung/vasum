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
 * @author Jan Olszak (j.olszak@samsung.com)
 * @brief  Declaration of the class managing a sqlite3 database connection
 */

#ifndef CARGO_SQLITE_SQLITE3_CONNECTION_HPP
#define CARGO_SQLITE_SQLITE3_CONNECTION_HPP

#include <sqlite3.h>
#include <string>

namespace cargo {
namespace sqlite3 {

struct Connection {
    /**
     * @param path database file path
     */
    Connection(const std::string& path);
    Connection(const Connection&) = delete;
    ~Connection();

    /**
     * @return pointer to the corresponding sqlite3 database object
     */
    ::sqlite3* get();

    /**
     * @return last error message in the database
     */
    std::string getErrorMessage();

    /**
     * Executes the query in the database.
     *
     * @param query query to be executed
     */
    void exec(const std::string& query);

private:
    ::sqlite3* mDbPtr;
};

} // namespace sqlite3
} // namespace cargo

#endif // CARGO_SQLITE_SQLITE3_CONNECTION_HPP
