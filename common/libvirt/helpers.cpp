/*
 *  Copyright (c) 2014 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *  Contact: Lukasz Pawelczyk <l.pawelczyk@partner.samsung.com>
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
 * @author  Lukasz Pawelczyk (l.pawelczyk@partner.samsung.com)
 * @brief   A function helpers for the libvirt library
 */

#include "config.hpp"
#include "libvirt/helpers.hpp"
#include "log/logger.hpp"

#include <mutex>
#include <libvirt/virterror.h>
#include <libvirt-glib/libvirt-glib-event.h>


namespace security_containers {
namespace libvirt {


namespace {

std::once_flag gInitFlag;

/**
 * This function intentionally is not displaying any errors,
 * we log them ourselves elsewhere.
 * It is however displaying warnings for the time being so we can
 * learn whether such situations occur.
 */
void libvirtErrorFunction(void* /*userData*/, virErrorPtr error)
{
    if (error->level == VIR_ERR_WARNING) {
        LOGW("LIBVIRT reported a warning: \n" << error->message);
    }
}

} // namespace

void libvirtInitialize(void)
{
    std::call_once(gInitFlag, []() {
            virInitialize();
            virSetErrorFunc(NULL, &libvirtErrorFunction);
            gvir_event_register();
        });
}


std::string libvirtFormatError(void)
{
    virErrorPtr error = virGetLastError();

    if (error == NULL) {
        return std::string();
    }

    return "Libvirt error: " + std::string(error->message);
}

std::string libvirtEventToString(const int eventId)
{
    switch(eventId) {
    case VIR_DOMAIN_EVENT_DEFINED:
        return "Defined";
    case VIR_DOMAIN_EVENT_UNDEFINED:
        return "Undefined";
    case VIR_DOMAIN_EVENT_STARTED:
        return "Started";
    case VIR_DOMAIN_EVENT_SUSPENDED:
        return "Suspended";
    case VIR_DOMAIN_EVENT_RESUMED:
        return "Resumed";
    case VIR_DOMAIN_EVENT_STOPPED:
        return "Stopped";
    case VIR_DOMAIN_EVENT_SHUTDOWN:
        return "Shutdown";
    case VIR_DOMAIN_EVENT_PMSUSPENDED:
        return "PM Suspended";
    case VIR_DOMAIN_EVENT_CRASHED:
        return "Crashed";
    default:
        return "Unknown EventId";
    }
}

std::string libvirtEventDetailToString(const int eventId, const int detailId)
{
    switch (eventId) {
    case VIR_DOMAIN_EVENT_DEFINED:
        switch (detailId) {
        case VIR_DOMAIN_EVENT_DEFINED_ADDED:
            return "Added";
        case VIR_DOMAIN_EVENT_DEFINED_UPDATED:
            return "Updated";
        default:
            return "Unknown detail";
        }
    case VIR_DOMAIN_EVENT_UNDEFINED:
        switch (detailId) {
        case VIR_DOMAIN_EVENT_UNDEFINED_REMOVED:
            return "Removed";
        default:
            return "Unknown detail";
        }
    case VIR_DOMAIN_EVENT_STARTED:
        switch (detailId) {
        case VIR_DOMAIN_EVENT_STARTED_BOOTED:
            return "Booted";
        case VIR_DOMAIN_EVENT_STARTED_MIGRATED:
            return "Migrated";
        case VIR_DOMAIN_EVENT_STARTED_RESTORED:
            return "Restored";
        case VIR_DOMAIN_EVENT_STARTED_FROM_SNAPSHOT:
            return "From Snapshot";
        case VIR_DOMAIN_EVENT_STARTED_WAKEUP:
            return "Wakeup";
        default:
            return "Unknown detail";
        }
    case VIR_DOMAIN_EVENT_SUSPENDED:
        switch (detailId) {
        case VIR_DOMAIN_EVENT_SUSPENDED_PAUSED:
            return "Paused";
        case VIR_DOMAIN_EVENT_SUSPENDED_MIGRATED:
            return "Migrated";
        case VIR_DOMAIN_EVENT_SUSPENDED_IOERROR:
            return "IO Error";
        case VIR_DOMAIN_EVENT_SUSPENDED_WATCHDOG:
            return "Watchdog";
        case VIR_DOMAIN_EVENT_SUSPENDED_RESTORED:
            return "Restored";
        case VIR_DOMAIN_EVENT_SUSPENDED_FROM_SNAPSHOT:
            return "From Snapshot";
        case VIR_DOMAIN_EVENT_SUSPENDED_API_ERROR:
            return "API Error";
        default:
            return "Unknown detail";
        }
    case VIR_DOMAIN_EVENT_RESUMED:
        switch (detailId) {
        case VIR_DOMAIN_EVENT_RESUMED_UNPAUSED:
            return "Unpaused";
        case VIR_DOMAIN_EVENT_RESUMED_MIGRATED:
            return "Migrated";
        case VIR_DOMAIN_EVENT_RESUMED_FROM_SNAPSHOT:
            return "From Snapshot";
        default:
            return "Unknown detail";
        }
    case VIR_DOMAIN_EVENT_STOPPED:
        switch (detailId) {
        case VIR_DOMAIN_EVENT_STOPPED_SHUTDOWN:
            return "Shutdown";
        case VIR_DOMAIN_EVENT_STOPPED_DESTROYED:
            return "Destroyed";
        case VIR_DOMAIN_EVENT_STOPPED_CRASHED:
            return "Crashed";
        case VIR_DOMAIN_EVENT_STOPPED_MIGRATED:
            return "Migrated";
        case VIR_DOMAIN_EVENT_STOPPED_SAVED:
            return "Failed";
        case VIR_DOMAIN_EVENT_STOPPED_FAILED:
            return "Failed";
        case VIR_DOMAIN_EVENT_STOPPED_FROM_SNAPSHOT:
            return "From Snapshot";
        default:
            return "Unknown detail";
        }
    case VIR_DOMAIN_EVENT_SHUTDOWN:
        switch (detailId) {
        case VIR_DOMAIN_EVENT_SHUTDOWN_FINISHED:
            return "Finished";
        default:
            return "Unknown detail";
        }
    case VIR_DOMAIN_EVENT_PMSUSPENDED:
        switch (detailId) {
        case VIR_DOMAIN_EVENT_PMSUSPENDED_MEMORY:
            return "Memory";
        case VIR_DOMAIN_EVENT_PMSUSPENDED_DISK:
            return "Disk";
        default:
            return "Unknown detail";
        }
    case VIR_DOMAIN_EVENT_CRASHED:
        switch (detailId) {
        case VIR_DOMAIN_EVENT_CRASHED_PANICKED:
            return "Panicked";
        default:
            return "Unknown detail";
        }
    default:
        return "Unknown event";
    }
}


} // namespace libvirt
} // namespace security_containers
