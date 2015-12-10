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
 * @brief  Definition of the class managing a sqlite3 statement
 */

#include "config.hpp"

#include "cargo-sqlite/sqlite3/statement.hpp"
#include "cargo/exception.hpp"


namespace cargo {
namespace sqlite3 {

Statement::Statement(sqlite3::Connection& connRef, const std::string& query)
    : mConnRef(connRef)
{
    if (::sqlite3_prepare_v2(connRef.get(),
                             query.c_str(),
                             query.size(),
                             &mStmtPtr,
                             NULL)
            != SQLITE_OK) {
        throw CargoException("Error during preparing statement " +
                              mConnRef.getErrorMessage());
    }

    if (mStmtPtr == NULL) {
        throw CargoException("Wrong query: " + query);
    }
}

Statement::Statement::~Statement()
{
    if (::sqlite3_finalize(mStmtPtr) != SQLITE_OK) {
        throw CargoException("Error during finalizing statement " +
                              mConnRef.getErrorMessage());
    }
}

sqlite3_stmt* Statement::get()
{
    return mStmtPtr;
}

void Statement::reset()
{
    if (::sqlite3_clear_bindings(mStmtPtr) != SQLITE_OK) {
        throw CargoException("Error unbinding statement: " +
                              mConnRef.getErrorMessage());
    }

    if (::sqlite3_reset(mStmtPtr) != SQLITE_OK) {
        throw CargoException("Error reseting statement: " +
                              mConnRef.getErrorMessage());
    }
}

} // namespace sqlite3
} // namespace cargo
