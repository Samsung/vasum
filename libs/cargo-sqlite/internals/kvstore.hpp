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
 * @brief  Declaration of a class for key-value storage in a sqlite3 database
 */

#ifndef CARGO_SQLITE_INTERNALS_KVSTORE_HPP
#define CARGO_SQLITE_INTERNALS_KVSTORE_HPP

#include "cargo-sqlite/sqlite3/connection.hpp"
#include "cargo-sqlite/sqlite3/statement.hpp"

#include <algorithm>
#include <initializer_list>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <vector>
#include <atomic>
#include <utility>

namespace cargo {

namespace internals {

class KVStore {

public:
    /**
     * A guard struct for thread synchronization and transaction management.
     */
    class Transaction {
    public:
        Transaction(KVStore& store);
        ~Transaction();

        Transaction(const Transaction&) = delete;
        Transaction& operator=(const Transaction&) = delete;

        void commit();
    private:
        std::unique_lock<std::recursive_mutex> mLock;
        KVStore& mKVStore;
        bool mIsOuter;
    };

    /**
     * @param path configuration database file path
     */
    explicit KVStore(const std::string& path);
    ~KVStore();

    KVStore(const KVStore&) = delete;
    KVStore& operator=(const KVStore&) = delete;

    /**
     * Clears all the stored data
     */
    void clear();

    /**
     * @return Is there any data stored
     */
    bool isEmpty();

    /**
     * @param key string of the stored value
     *
     * @return Does this key exist in the database
     */
    bool exists(const std::string& key);

    /**
     * @param key string of the stored value
     *
     * @return Does a key starting with an argument exist in the database
     */
    bool prefixExists(const std::string& key);

    /**
     * Removes values corresponding to the passed key.
     * Many values may correspond to one key, so many values may
     * need to be deleted
     *
     * @param key string regexp of the stored values
     */
    void remove(const std::string& key);

    /**
     * Stores a single value corresponding to the passed key
     *
     * @param key string key of the value
     * @param value value corresponding to the key
     */
    void set(const std::string& key, const std::string& value);

    /**
     * Gets the value corresponding to the key.
     *
     * @param key string key of the value
     * @return value corresponding to the key
     */
    std::string get(const std::string& key);

    /**
     * Returns all stored keys.
     */
    std::vector<std::string> getKeys();

private:
    typedef std::lock_guard<std::recursive_mutex> Lock;

    std::recursive_mutex mMutex;
    size_t mTransactionDepth;
    bool mIsTransactionCommited;

    std::string mPath;
    sqlite3::Connection mConn;
    std::unique_ptr<sqlite3::Statement> mGetValueStmt;
    std::unique_ptr<sqlite3::Statement> mGetKeyExistsStmt;
    std::unique_ptr<sqlite3::Statement> mGetKeyPrefixExistsStmt;
    std::unique_ptr<sqlite3::Statement> mGetIsEmptyStmt;
    std::unique_ptr<sqlite3::Statement> mGetValueListStmt;
    std::unique_ptr<sqlite3::Statement> mSetValueStmt;
    std::unique_ptr<sqlite3::Statement> mRemoveValuesStmt;
    std::unique_ptr<sqlite3::Statement> mGetKeysStmt;

    void setupDb();
    void prepareStatements();
    void createFunctions();
};

} // namespace internals

} // namespace cargo

#endif // CARGO_SQLITE_INTERNALS_KVSTORE_HPP
