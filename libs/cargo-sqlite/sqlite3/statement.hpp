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
 * @brief  Declaration of the class managing a sqlite3 statement
 */

#ifndef CARGO_SQLITE_SQLITE3_STATEMENT_HPP
#define CARGO_SQLITE_SQLITE3_STATEMENT_HPP

#include "cargo-sqlite/sqlite3/connection.hpp"

#include <sqlite3.h>
#include <string>

namespace cargo {
namespace sqlite3 {

struct Statement {

    /**
     * @param connRef reference to the Connection object
     * @param query query to be executed
     */
    Statement(sqlite3::Connection& connRef, const std::string& query);
    ~Statement();

    /**
     * @return pointer to the sqlite3 statement
     */
    sqlite3_stmt* get();

    /**
     * Clears the bindings and resets the statement.
     * After this the statement can be executed again
     */
    void reset();

private:
    ::sqlite3_stmt* mStmtPtr;
    sqlite3::Connection& mConnRef;
};

} // namespace sqlite3
} // namespace cargo

#endif // CARGO_SQLITE_SQLITE3_STATEMENT_HPP
