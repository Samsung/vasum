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
 * @brief  Definition of the class managing a sqlite3 database connection
 */

#include "config.hpp"

#include "cargo-sqlite/sqlite3/connection.hpp"
#include "cargo/exception.hpp"

namespace cargo {
namespace sqlite3 {

Connection::Connection(const std::string& path)
{
    if (path.empty()) {
        // Sqlite creates temporary database in case of empty path
        // but we want to forbid this.
        throw CargoException("Error opening the database: empty path");
    }
    if (::sqlite3_open_v2(path.c_str(),
                          &mDbPtr,
                          SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE,
                          NULL) != SQLITE_OK) {
        throw CargoException("Error opening the database: " + getErrorMessage());
    }

    if (mDbPtr == NULL) {
        throw CargoException("Error opening the database: Unable to allocate memory.");
    }
}

Connection::~Connection()
{
    ::sqlite3_close(mDbPtr);
}

void Connection::exec(const std::string& query)
{
    char* mess;
    if (::sqlite3_exec(mDbPtr, query.c_str(), 0, 0, &mess) != SQLITE_OK) {
        throw CargoException("Error during executing statement " + std::string(mess));
    }
}

::sqlite3* Connection::get()
{
    return mDbPtr;
}

std::string Connection::getErrorMessage()
{
    return std::string(sqlite3_errmsg(mDbPtr));
}

} // namespace sqlite3
} // namespace cargo
