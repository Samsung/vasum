/*
 *  Copyright (c) 2014 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *  Contact: Mateusz Malicki <m.malicki2@samsung.com>
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
 * @author  Mateusz Malicki (m.malicki2@samsung.com)
 * @brief   This file contains security-containers-server's client implementation
 */

#include <config.hpp>
#include "security-containers-client-impl.hpp"
#include <dbus/connection.hpp>
#include <dbus/exception.hpp>
#include <utils/glib-loop.hpp>
#include <host-dbus-definitions.hpp>

#include <memory>
#include <cstring>
#include <cassert>

using namespace std;
using namespace dbus;
using namespace security_containers;
using namespace security_containers::utils;

namespace {

const string SCCLIENT_SUCCESS_MSG;
const string SCCLIENT_EXCEPTION_MSG = "unspecified exception";

const DbusInterfaceInfo hostDbusInterfaceInfo(api::host::BUS_NAME,
                                              api::host::OBJECT_PATH,
                                              api::host::INTERFACE);

unique_ptr<ScopedGlibLoop> loop;

void toDict(GVariant* in, ScArrayString* keys, ScArrayString* values)
{
    assert(in);
    assert(keys);
    assert(values);

    typedef char* key_type;
    typedef char* value_type;

    GVariantIter iter;
    value_type value;
    key_type key;
    gsize size = g_variant_n_children(in);
    key_type* outk = (key_type*)calloc(size + 1, sizeof(key_type));
    value_type* outv = (value_type*)calloc(size + 1, sizeof(value_type));

    g_variant_iter_init(&iter, in);
    for (int i = 0; g_variant_iter_loop(&iter, "{ss}", &key, &value); i++) {
        outk[i] = strdup(key);
        outv[i] = strdup(value);
    }
    *keys = outk;
    *values = outv;
}

ScStatus toStatus(const std::exception& ex)
{
    if (typeid(DbusCustomException) == typeid(ex)) {
        return SCCLIENT_DBUS_CUSTOM_EXCEPTION;
    } else if (typeid(DbusIOException) == typeid(ex)) {
        return SCCLIENT_DBUS_IO_EXCEPTION;
    } else if (typeid(DbusOperationException) == typeid(ex)) {
        return SCCLIENT_DBUS_OPERATION_EXCEPTION;
    } else if (typeid(DbusInvalidArgumentException) == typeid(ex)) {
        return SCCLIENT_DBUS_INVALID_ARGUMENT_EXCEPTION;
    } else if (typeid(DbusException) == typeid(ex)) {
        return SCCLIENT_DBUS_EXCEPTION;
    }
    return SCCLIENT_RUNTIME_EXCEPTION;
}

} //namespace

ScStatus Client::sc_start() noexcept
{
    try {
        if (!loop) {
            loop.reset(new ScopedGlibLoop());
        }
    } catch (const exception& ex) {
        return toStatus(ex);
    } catch (...) {
        return SCCLIENT_EXCEPTION;
    }

    return SCCLIENT_SUCCESS;
}

ScStatus Client::sc_stop() noexcept
{
    try {
        if (loop) {
            loop.reset();
        }
    } catch (const exception& ex) {
        return toStatus(ex);
    } catch (...) {
        return SCCLIENT_EXCEPTION;
    }
    return SCCLIENT_SUCCESS;
}

Client::Status::Status(ScStatus status)
    : mScStatus(status)
{
    switch (status) {
        case SCCLIENT_EXCEPTION:
            mMsg = SCCLIENT_EXCEPTION_MSG;
            break;
        case SCCLIENT_SUCCESS:
            mMsg = SCCLIENT_SUCCESS_MSG;
            break;
        default:
            assert(!"Logic error. You must specify a message.");
            mMsg = std::string();
    }
}

Client::Status::Status(ScStatus status, const std::string& msg)
    : mScStatus(status), mMsg(msg)
{
}

Client::Client() noexcept
    : mStatus(SCCLIENT_SUCCESS)
{
}

Client::~Client() noexcept
{
}

ScStatus Client::createSystem() noexcept
{
    try {
        mConnection = DbusConnection::createSystem();
        mStatus = Status(SCCLIENT_SUCCESS);
    } catch (const exception& ex)  {
        mStatus = Status(toStatus(ex), ex.what());
    } catch (...) {
        mStatus = Status(SCCLIENT_EXCEPTION);
    }
    return sc_get_status();
}

ScStatus Client::create(const string& address) noexcept
{
    try {
        mConnection = DbusConnection::create(address);
        mStatus = Status(SCCLIENT_SUCCESS);
    } catch (const exception& ex)  {
        mStatus = Status(toStatus(ex), ex.what());
    } catch (...) {
        mStatus = Status(SCCLIENT_EXCEPTION);
    }
    return sc_get_status();
}

ScStatus Client::callMethod(const DbusInterfaceInfo& info,
                            const string& method,
                            GVariant* args_in,
                            const string& args_spec_out,
                            GVariant** args_out)
{
    try {
        GVariantPtr ret = mConnection->callMethod(info.busName,
                                                  info.objectPath,
                                                  info.interface,
                                                  method,
                                                  args_in,
                                                  args_spec_out);
        if (args_out != NULL) {
            *args_out = ret.release();
        }
        mStatus = Status(SCCLIENT_SUCCESS);
    } catch (const exception& ex)  {
        mStatus = Status(toStatus(ex), ex.what());
    } catch (...) {
        mStatus = Status(SCCLIENT_EXCEPTION);
    }
    return sc_get_status();
}

const char* Client::sc_get_status_message() noexcept
{
    return mStatus.mMsg.c_str();
}

ScStatus Client::sc_get_status() noexcept
{
    return mStatus.mScStatus;
}

ScStatus Client::sc_get_container_dbuses(ScArrayString* keys, ScArrayString* values) noexcept
{
    assert(keys);
    assert(values);

    GVariant* out;
    ScStatus ret = callMethod(hostDbusInterfaceInfo,
                              api::host::METHOD_GET_CONTAINER_DBUSES,
                              NULL,
                              "(a{ss})",
                              &out);
    if (ret != SCCLIENT_SUCCESS) {
        return ret;
    }
    GVariant* unpacked;
    g_variant_get(out, "(*)", &unpacked);
    toDict(unpacked, keys, values);
    g_variant_unref(unpacked);
    g_variant_unref(out);
    return ret;
}
