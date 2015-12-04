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
 * @brief  Definition of a class for key-value storage in a sqlite3 database
 */

#include "config.hpp"

#include "cargo-sqlite/internals/kvstore.hpp"
#include "cargo/exception.hpp"

#include <exception>
#include <limits>
#include <memory>
#include <set>
#include <cassert>
#include <cstring>

namespace cargo {

namespace internals {

namespace {

const int AUTO_DETERM_SIZE = -1;
const int FIRST_COLUMN = 0;

struct ScopedReset {
    ScopedReset(std::unique_ptr<sqlite3::Statement>& stmtPtr)
        : mStmtPtr(stmtPtr) {}
    ~ScopedReset()
    {
        mStmtPtr->reset();
    }
private:
    std::unique_ptr<sqlite3::Statement>& mStmtPtr;
};

/**
 * Escape characters used by the GLOB function.
 */
void sqliteEscapeStr(::sqlite3_context* context, int argc, ::sqlite3_value** values)
{
    char* inBuff = (char*)sqlite3_value_text(values[0]);
    if (argc != 1 || inBuff == NULL) {
        sqlite3_result_error(context, "SQL function escapeSequence() called with invalid arguments.\n", -1);
        return;
    }

    std::string in(inBuff);
    static const std::set<char> toEscape({'?', '*', '[', ']'});

    // Compute the out size
    auto isEscapeChar = [&](char c) {
        return toEscape.count(c) == 1;
    };
    size_t numEscape = std::count_if(in.begin(),
                                     in.end(),
                                     isEscapeChar);
    if (numEscape == 0) {
        sqlite3_result_text(context, in.c_str(), AUTO_DETERM_SIZE, SQLITE_TRANSIENT);
    }

    // Escape characters
    std::string out(in.size() + 2 * numEscape, 'x');
    for (size_t i = 0, j = 0;
            i < in.size();
            ++i, ++j) {
        if (isEscapeChar(in[i])) {
            out[j] = '[';
            ++j;
            out[j] = in[i];
            ++j;
            out[j] = ']';
        } else {
            out[j] = in[i];
        }
    }
    sqlite3_result_text(context, out.c_str(), AUTO_DETERM_SIZE, SQLITE_TRANSIENT);
}

} // namespace

KVStore::Transaction::Transaction(KVStore& kvStore)
    : mLock(kvStore.mMutex)
    , mKVStore(kvStore)
    , mIsOuter(kvStore.mTransactionDepth == 0)
{
    if (mKVStore.mIsTransactionCommited) {
        throw CargoException("Previous transaction is not closed");
    }
    if (mIsOuter) {
        mKVStore.mConn.exec("BEGIN EXCLUSIVE TRANSACTION");
    }
    ++mKVStore.mTransactionDepth;
}

KVStore::Transaction::~Transaction()
{
    --mKVStore.mTransactionDepth;
    if (mIsOuter) {
        if (mKVStore.mIsTransactionCommited) {
            mKVStore.mIsTransactionCommited = false;
        } else {
            mKVStore.mConn.exec("ROLLBACK TRANSACTION");
        }
    }
}

void KVStore::Transaction::commit()
{
    if (mKVStore.mIsTransactionCommited) {
        throw CargoException("Transaction already commited");
    }
    if (mIsOuter) {
        mKVStore.mConn.exec("COMMIT TRANSACTION");
        mKVStore.mIsTransactionCommited = true;
    }
}

KVStore::KVStore(const std::string& path)
    : mTransactionDepth(0),
      mIsTransactionCommited(false),
      mPath(path),
      mConn(path)
{
    setupDb();
    createFunctions();
    prepareStatements();
}

KVStore::~KVStore()
{
    assert(mTransactionDepth == 0);
}

void KVStore::set(const std::string& key, const std::string& value)
{
    Transaction transaction(*this);
    ScopedReset scopedReset(mSetValueStmt);

    ::sqlite3_bind_text(mSetValueStmt->get(), 1, key.c_str(), AUTO_DETERM_SIZE, SQLITE_STATIC);
    ::sqlite3_bind_text(mSetValueStmt->get(), 2, value.c_str(), AUTO_DETERM_SIZE, SQLITE_STATIC);


    if (::sqlite3_step(mSetValueStmt->get()) != SQLITE_DONE) {
        throw CargoException("Error during stepping: " + mConn.getErrorMessage());
    }
    transaction.commit();
}

std::string KVStore::get(const std::string& key)
{
    Transaction transaction(*this);
    ScopedReset scopedReset(mGetValueStmt);

    ::sqlite3_bind_text(mGetValueStmt->get(), 1, key.c_str(), AUTO_DETERM_SIZE, SQLITE_TRANSIENT);

    int ret = ::sqlite3_step(mGetValueStmt->get());
    if (ret == SQLITE_DONE) {
        throw NoKeyException("No value corresponding to the key: " + key + "@" + mPath);
    }
    if (ret != SQLITE_ROW) {
        throw CargoException("Error during stepping: " + mConn.getErrorMessage());
    }

    std::string value = reinterpret_cast<const char*>(
            sqlite3_column_text(mGetValueStmt->get(), FIRST_COLUMN));

    transaction.commit();
    return value;
}

void KVStore::setupDb()
{
    // called only from ctor, transaction is not needed
    mConn.exec("CREATE TABLE IF NOT EXISTS data (key TEXT PRIMARY KEY, value TEXT NOT NULL)");
}

void KVStore::prepareStatements()
{
    mGetValueStmt.reset(
        new sqlite3::Statement(mConn, "SELECT value FROM data WHERE key = ? LIMIT 1"));
    mGetKeyExistsStmt.reset(
        new sqlite3::Statement(mConn, "SELECT 1 FROM data WHERE key = ?1 LIMIT 1"));
    mGetKeyPrefixExistsStmt.reset(
        new sqlite3::Statement(mConn, "SELECT 1 FROM data WHERE key = ?1 OR key GLOB escapeStr(?1) || '.*' LIMIT 1"));
    mGetIsEmptyStmt.reset(
        new sqlite3::Statement(mConn, "SELECT 1 FROM data LIMIT 1"));
    mSetValueStmt.reset(
        new sqlite3::Statement(mConn, "INSERT OR REPLACE INTO data (key, value) VALUES (?,?)"));
    mRemoveValuesStmt.reset(
        new sqlite3::Statement(mConn, "DELETE FROM data WHERE key = ?1  OR key GLOB escapeStr(?1) ||'.*' "));
    mGetKeysStmt.reset(
        new sqlite3::Statement(mConn, "SELECT key FROM data"));
}

void KVStore::createFunctions()
{
    int ret = sqlite3_create_function(mConn.get(), "escapeStr", 1, SQLITE_ANY, 0, &sqliteEscapeStr, 0, 0);
    if (ret != SQLITE_OK) {
        throw CargoException("Error during creating functions: " + mConn.getErrorMessage());
    }
}

void KVStore::clear()
{
    Transaction transaction(*this);
    mConn.exec("DELETE FROM data");
    transaction.commit();
}

bool KVStore::isEmpty()
{
    Transaction transaction(*this);
    ScopedReset scopedReset(mGetIsEmptyStmt);

    int ret = ::sqlite3_step(mGetIsEmptyStmt->get());
    if (ret != SQLITE_DONE && ret != SQLITE_ROW) {
        throw CargoException("Error during stepping: " + mConn.getErrorMessage());
    }

    transaction.commit();
    return ret == SQLITE_DONE;
}

bool KVStore::exists(const std::string& key)
{
    Transaction transaction(*this);
    ScopedReset scopedReset(mGetKeyExistsStmt);

    ::sqlite3_bind_text(mGetKeyExistsStmt->get(), 1, key.c_str(), AUTO_DETERM_SIZE, SQLITE_TRANSIENT);

    int ret = ::sqlite3_step(mGetKeyExistsStmt->get());
    if (ret != SQLITE_DONE && ret != SQLITE_ROW) {
        throw CargoException("Error during stepping: " + mConn.getErrorMessage());
    }

    transaction.commit();
    return ret == SQLITE_ROW;
}

bool KVStore::prefixExists(const std::string& key)
{
    Transaction transaction(*this);
    ScopedReset scopedReset(mGetKeyPrefixExistsStmt);

    ::sqlite3_bind_text(mGetKeyPrefixExistsStmt->get(), 1, key.c_str(), AUTO_DETERM_SIZE, SQLITE_TRANSIENT);

    int ret = ::sqlite3_step(mGetKeyPrefixExistsStmt->get());
    if (ret != SQLITE_DONE && ret != SQLITE_ROW) {
        throw CargoException("Error during stepping: " + mConn.getErrorMessage());
    }

    transaction.commit();
    return ret == SQLITE_ROW;
}

void KVStore::remove(const std::string& key)
{
    Transaction transaction(*this);
    ScopedReset scopedReset(mRemoveValuesStmt);

    ::sqlite3_bind_text(mRemoveValuesStmt->get(), 1, key.c_str(), AUTO_DETERM_SIZE, SQLITE_STATIC);

    if (::sqlite3_step(mRemoveValuesStmt->get()) != SQLITE_DONE) {
        throw CargoException("Error during stepping: " + mConn.getErrorMessage());
    }
    transaction.commit();
}

std::vector<std::string> KVStore::getKeys()
{
    Transaction transaction(*this);
    ScopedReset scopedReset(mGetKeysStmt);

    std::vector<std::string> result;

    for (;;) {
        int ret = ::sqlite3_step(mGetKeysStmt->get());
        if (ret == SQLITE_DONE) {
            break;
        }
        if (ret != SQLITE_ROW) {
            throw CargoException("Error during stepping: " + mConn.getErrorMessage());
        }
        const char* key = reinterpret_cast<const char*>(sqlite3_column_text(mGetKeysStmt->get(),
                                                                            FIRST_COLUMN));
        result.push_back(key);
    }

    transaction.commit();
    return result;
}

} // namespace internals

} // namespace cargo
