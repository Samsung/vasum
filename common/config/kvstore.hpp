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

#ifndef COMMON_CONFIG_KVSTORE_HPP
#define COMMON_CONFIG_KVSTORE_HPP

#include "config/sqlite3/connection.hpp"
#include "config/sqlite3/statement.hpp"

#include <algorithm>
#include <initializer_list>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <vector>
#include <atomic>

namespace config {

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
     * @param key string regexp of the stored values
     *
     * @return Does this key exist in the database
     */
    bool exists(const std::string& key);

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
    template<typename T>
    void set(const std::string& key, const T& value)
    {
        return setInternal(key, value);
    }

    /**
     * Gets the value corresponding to the key.
     * Uses stringstreams to parse.
     *
     * @param key string key of the value
     * @tparam T = std::string desired type of the return value
     * @return value corresponding to the key
     */
    template<typename T = std::string>
    T get(const std::string& key)
    {
        return getInternal(key, static_cast<T*>(nullptr));
    }

    /**
     * Returns all stored keys.
     */
    std::vector<std::string> getKeys();

private:
    typedef std::lock_guard<std::recursive_mutex> Lock;

    std::recursive_mutex mMutex;
    size_t mTransactionDepth;
    bool mIsTransactionCommited;

    void setInternal(const std::string& key, const std::string& value);
    void setInternal(const std::string& key, const std::initializer_list<std::string>& values);
    void setInternal(const std::string& key, const std::vector<std::string>& values);
    template<typename T>
    void setInternal(const std::string& key, const T& value);
    template<typename T>
    void setInternal(const std::string& key, const std::vector<T>& values);

    std::string getInternal(const std::string& key, std::string*);
    std::vector<std::string> getInternal(const std::string& key, std::vector<std::string>*);
    template<typename T>
    T getInternal(const std::string& key, T*);
    template<typename T>
    std::vector<T> getInternal(const std::string& key, std::vector<T>*);

    std::string mPath;
    sqlite3::Connection mConn;
    std::unique_ptr<sqlite3::Statement> mGetValueStmt;
    std::unique_ptr<sqlite3::Statement> mGetKeyExistsStmt;
    std::unique_ptr<sqlite3::Statement> mGetIsEmptyStmt;
    std::unique_ptr<sqlite3::Statement> mGetValueListStmt;
    std::unique_ptr<sqlite3::Statement> mSetValueStmt;
    std::unique_ptr<sqlite3::Statement> mRemoveValuesStmt;
    std::unique_ptr<sqlite3::Statement> mGetKeysStmt;

    void setupDb();
    void prepareStatements();
    void createFunctions();
};

namespace {
template<typename T>
std::string toString(const T& value)
{
    std::ostringstream oss;
    oss << value;
    return oss.str();
}

template<typename T>
T fromString(const std::string& strValue)
{
    std::istringstream iss(strValue);
    T value;
    iss >> value;
    return value;
}

} // namespace

template<typename T>
void KVStore::setInternal(const std::string& key, const T& value)
{
    setInternal(key, toString(value));
}

template<typename T>
void KVStore::setInternal(const std::string& key, const std::vector<T>& values)
{
    std::vector<std::string> strValues(values.size());

    std::transform(values.begin(),
                   values.end(),
                   strValues.begin(),
                   toString<T>);

    setInternal(key, strValues);
}

template<typename T>
T KVStore::getInternal(const std::string& key, T*)
{
    return fromString<T>(getInternal(key, static_cast<std::string*>(nullptr)));
}

template<typename T>
std::vector<T> KVStore::getInternal(const std::string& key, std::vector<T>*)
{
    std::vector<std::string> strValues = getInternal(key, static_cast<std::vector<std::string>*>(nullptr));
    std::vector<T> values(strValues.size());

    std::transform(strValues.begin(),
                   strValues.end(),
                   values.begin(),
                   fromString<T>);

    return values;
}

/**
 * Concatenates all parameters into one std::string.
 * Uses '.' to connect the terms.
 * @param args components of the string
 * @tparam delim optional delimiter
 * @tparam Args any type implementing str
 * @return string created from he args
 */
template<char delim = '.', typename Arg1, typename ... Args>
std::string key(const Arg1& a1, const Args& ... args)
{
    std::string ret = toString(a1);
    std::initializer_list<std::string> strings {toString(args)...};
    for (const std::string& s : strings) {
        ret += delim + s;
    }

    return ret;
}

/**
 * Function added for key function completeness.
 *
 * @tparam delim = '.' parameter not used, added for consistency
 * @return empty string
 */
template<char delim = '.'>
std::string key()
{
    return std::string();
}

} // namespace config

#endif // COMMON_CONFIG_KVSTORE_HPP


