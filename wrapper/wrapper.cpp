/*
 *  Copyright (c) 2014 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *  Contact: Krzysztof Dynowski <k.dynowski@samsung.com>
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
 * @author  Krzysztof Dynowski (k.dynowski@samsung.com)
 * @brief   Vasum old API wrapper to slp client lib
 */

#define __VASUM_WRAPPER_SOURCE__
#include <vector>
#include <string.h>
#include <algorithm>

#include "config.hpp"
#include "vasum-client-impl.hpp"
#include "logger/logger.hpp"
#include "logger/backend-journal.hpp"

#include "wrapper-compatibility.h"

struct WrappedZone
{
    Client *client;
    VsmZone zone;
    struct vsm_zone vz;
    std::vector<vsm_netdev> netdevs;
};

struct WrappedContext
{
    Client *client;
    vsm_context hq_ctx;
    struct vsm_zone hq_root;
    std::vector<WrappedZone> zones;
};

static struct
{
    int done;
    int glib_stop;
} wrap;


#ifndef offsetof
#define offsetof(type, memb) ((size_t)&((type *)0)->memb)
#endif
#ifdef container_of
#undef container_of
#endif
#ifndef container_of
#define container_of(ptr, type, memb) (\
    (type *)((char *)(ptr) - offsetof(type, memb)))
#endif

#define UNUSED(x) ((void)(x))

#define vsm_attach_command_t vsm_attach_command_s
#define vsm_attach_options_t vsm_attach_options_s

void __attribute__ ((constructor)) wrapper_load(void);
void __attribute__ ((destructor)) wrapper_unload(void);
static void init_wrapper();
extern struct vasum_ops dummy_ops;

using namespace logger;
void wrapper_load(void)
{
    Logger::setLogLevel(LogLevel::TRACE);
    Logger::setLogBackend(new SystemdJournalBackend());
    LOGI("wrapper_load");
    init_wrapper();
}

void wrapper_unload(void)
{
    if (wrap.glib_stop) Client::vsm_stop_glib_loop();
    wrap.glib_stop = 0;
    LOGI("wrapper_unload");
}

static void callcheck()
{
    init_wrapper();
}

void init_wrapper()
{
    if (wrap.done) return ;
    memset(&wrap, 0, sizeof(wrap));
    wrap.done = 1;
    LOGS("");
}

static struct vsm_zone* wrap_vsm_zone(WrappedContext *w, VsmZone zone, bool create = false)
{
    if (zone == NULL) {
        return NULL;
    }
    for (auto& zw : w->zones) {
        if (zw.zone == zone) {
            return &zw.vz;
        }
    }
    if (create) {
        w->zones.push_back(WrappedZone());
        WrappedZone& zw = w->zones.back();
        zw.client = w->client;
        zw.zone = zone;
        zw.vz.name = zone->id;
        zw.vz.id = 0;
        zw.vz.type = NULL;
        zw.vz.user_data = NULL;
        zw.vz.rootfs_path = zone->rootfs_path;
        zw.vz.parent = &zw.vz;
        LOGI("return (create) zone " << zone->id);
        return &w->zones.back().vz;
    }
    LOGE("return zone NULL");
    return NULL;
}

static int wrap_error(VsmStatus st, const Client *c)
{
    if (st == VSMCLIENT_SUCCESS) LOGI("return success " << st);
    else LOGE("return error=" << st << ", msg=" << (c ? c->vsm_get_status_message() : "n/a"));
    switch (st) {
        case VSMCLIENT_SUCCESS: return VSM_ERROR_NONE;
        case VSMCLIENT_CUSTOM_ERROR: return -VSM_ERROR_GENERIC;
        case VSMCLIENT_IO_ERROR: return -VSM_ERROR_IO;
        case VSMCLIENT_OPERATION_FAILED: return -VSM_ERROR_NOT_PERMITTED;
        case VSMCLIENT_INVALID_ARGUMENT: return -VSM_ERROR_INVALID;
        case VSMCLIENT_OTHER_ERROR: return -VSM_ERROR_GENERIC;
    }
    return -VSM_ERROR_GENERIC;
}

static void init_context_wrap(WrappedContext *w)
{
    Client::vsm_start_glib_loop();
    wrap.glib_stop = 1;
    w->client = new Client();
    VsmStatus st = w->client->createSystem();
    wrap_error(st, w->client);

    memset(&w->hq_ctx, 0, sizeof(w->hq_ctx));
    memset(&w->hq_root, 0, sizeof(w->hq_root));

    vsm_context *ctx = &w->hq_ctx;
    adt_init_list(&ctx->listeners);
    //init root_zone
    ctx->root_zone = &w->hq_root;
    ctx->root_zone->name = (char*)"";
    ctx->root_zone->id = 0;
    ctx->root_zone->rootfs_path = (char*)"/";

    ctx->root_zone->terminal = -1;
    ctx->root_zone->state = VSM_ZONE_STATE_RUNNING;
    ctx->root_zone->user_data = ctx->root_zone;

    ctx->root_zone->parent = ctx->root_zone;
    ctx->root_zone->ctx = ctx;

    pthread_rwlock_init(&ctx->root_zone->lock, NULL);
    adt_init_list(&ctx->root_zone->netdevs);
    adt_init_list(&ctx->root_zone->devices);
    adt_init_list(&ctx->root_zone->children);

    pthread_rwlock_init(&ctx->lock, NULL);
    adt_init_list(&ctx->listeners);
    adt_init_list(&ctx->sc_listeners);
    adt_init_list(&ctx->ev_listeners);

    //struct mainloop *mainloop = mainloop_create();
    //struct mxe_engine *engine = mxe_prepare_engine(mainloop, ctx);
    //struct mxe_endpoint *ep = mxe_create_client(engine, SERVICEPATH);

    ctx->foreground_zone = ctx->root_zone;
    ctx->vsm_ops = &dummy_ops;
    ctx->error = VSM_ERROR_NONE;
    //ctx->data = ep;
}

extern "C" {
API void vsm_string_free(VsmString string);
API void vsm_array_string_free(VsmArrayString astring);

API vsm_context_h vsm_create_context(void)
{
    LOGS("");
    callcheck();
    WrappedContext *w = new WrappedContext();
    init_context_wrap(w);

    vsm_context *ctx = &w->hq_ctx;
    return ctx;
}

API int vsm_cleanup_context(vsm_context_h ctx)
{
    LOGS("");
    callcheck();
    WrappedContext *w = container_of(ctx, WrappedContext, hq_ctx);
    if (w->client != NULL) {
        delete w->client;
        w->client = NULL;
    }
    for (auto& zw : w->zones) {
        zw.netdevs.clear();
    }
    w->zones.clear();
    pthread_rwlock_destroy(&ctx->lock);
    delete w;
    return VSM_ERROR_NONE;
}

static const char *const vsm_error_strtab[] = {
    "No error",
    "Undefined error",
    "Invalid",
    "Operation cancelled",
    "Operation aborted",
    "Connection refused",
    "Object exists",
    "Resource busy",
    "Input/Output error",
    "Timeout",
    "Overflow",
    "Out of memory",
    "Out of range",
    "Operation not permitted",
    "Function not implemented",
    "Operation not supported",
    "Access denied",
    "No object found",
    "Bad state"
};

API vsm_error_e vsm_last_error(struct vsm_context *ctx)
{
    if (ctx)
        return ctx->error;
    return static_cast<vsm_error_e>(-1);
}

API int vsm_get_poll_fd(struct vsm_context *ctx)
{
    LOGS("");
    callcheck();
    WrappedContext *w = container_of(ctx, WrappedContext, hq_ctx);
    UNUSED(w);
    //FIXME Client should create Dispatcher and pass to IPCConnection
    //      now: IPCConnection has field ThreadWrapper
    //return w->client->getEventPoll().getPollFD();
    return -1;
}
API int vsm_enter_eventloop(struct vsm_context *ctx, int flags, int timeout)
{
    LOGS("");
    callcheck();
    UNUSED(flags);
    UNUSED(timeout);
    WrappedContext *w = container_of(ctx, WrappedContext, hq_ctx);
    UNUSED(w);
    //FIXME Client should create Dispatcher and pass to IPCConnection
    //      now: IPCConnection has field ThreadWrapper
    //TODO Use EventPoll from Dispatcher
    return 0;
}

API int vsm_create_zone(struct vsm_context *ctx, const char *zone_name, const char *template_name, int flag)
{
    LOGS("create_zone " << zone_name);
    callcheck();
    WrappedContext *w = container_of(ctx, WrappedContext, hq_ctx);
    UNUSED(flag);
    //template_name = NULL; //template name not supported by libvasum-client
    if (!w->client) return VSM_ERROR_GENERIC;
    VsmStatus st = w->client->vsm_create_zone(zone_name, template_name);
    if (st != VSMCLIENT_SUCCESS) {
        LOGE("vsm_create_zone(" << zone_name << ") = " << st);
    }
    return wrap_error(st, w->client);
}

API int vsm_destroy_zone(struct vsm_context *ctx, const char *zone_name, int force)
{
    LOGS("zone=" << zone_name);
    callcheck();
    WrappedContext *w = container_of(ctx, WrappedContext, hq_ctx);
    UNUSED(force);
    if (!w->client) return VSM_ERROR_GENERIC;
    VsmStatus st = w->client->vsm_destroy_zone(zone_name);
    if (st == VSMCLIENT_SUCCESS) {
        auto zonebyname = [zone_name](const WrappedZone& v) {return v.zone->id == zone_name;};
        auto zonelist = std::remove_if(w->zones.begin(), w->zones.end(), zonebyname);
        w->zones.erase(zonelist);
    }
    return wrap_error(st, w->client);
}

API int vsm_start_zone(struct vsm_context *ctx, const char *zone_name)
{
    LOGS("zone=" << zone_name);
    callcheck();
    WrappedContext *w = container_of(ctx, WrappedContext, hq_ctx);
    if (!w->client) return VSM_ERROR_GENERIC;
    VsmStatus st = w->client->vsm_start_zone(zone_name);
    return wrap_error(st, w->client);
}

API int vsm_shutdown_zone(struct vsm_context *ctx, const char *zone_name, int force)
{
    LOGS("zone=" << zone_name);
    callcheck();
    WrappedContext *w = container_of(ctx, WrappedContext, hq_ctx);
    UNUSED(force);
    if (!w->client) return VSM_ERROR_GENERIC;
    VsmStatus st = w->client->vsm_shutdown_zone(zone_name);
    return wrap_error(st, w->client);
}

API int vsm_lock_zone(struct vsm_context *ctx, const char *zone_name, int shutdown)
{
    LOGS("zone=" << zone_name);
    callcheck();
    WrappedContext *w = container_of(ctx, WrappedContext, hq_ctx);
    UNUSED(shutdown);
    if (!w->client) return VSM_ERROR_GENERIC;
    VsmStatus st = w->client->vsm_lock_zone(zone_name);
    return wrap_error(st, w->client);
}

API int vsm_unlock_zone(struct vsm_context *ctx, const char *zone_name)
{
    LOGS("zone=" << zone_name);
    callcheck();
    WrappedContext *w = container_of(ctx, WrappedContext, hq_ctx);
    if (!w->client) return VSM_ERROR_GENERIC;
    VsmStatus st = w->client->vsm_lock_zone(zone_name);
    return wrap_error(st, w->client);
}

API int vsm_set_foreground(struct vsm_zone *zone)
{
    LOGS("");
    callcheck();
    WrappedZone *w = container_of(zone, WrappedZone, vz);
    if (!w->client) return VSM_ERROR_GENERIC;
    VsmStatus st = w->client->vsm_set_active_zone(zone->name);
    return wrap_error(st, w->client);
}

//execute command in specified zone
API int vsm_attach_zone(struct vsm_context *ctx,
                        const char *zone_name,
                        vsm_attach_command_t *command,
                        vsm_attach_options_t *opts,
                        pid_t *attached_process)
{
    return dummy_ops.attach_zone(ctx, zone_name, command, opts,
                     attached_process);
}

//execute command in specified zone and wait
API int vsm_attach_zone_wait(struct vsm_context *ctx,
                             const char *zone_name,
                             vsm_attach_command_t *command,
                             vsm_attach_options_t *opts)
{
    return dummy_ops.attach_zone_wait(ctx, zone_name, command, opts);
}

API int vsm_iterate_zone(struct vsm_context *ctx, void (*callback)(struct vsm_zone *zone, void *user_data), void *user_data)
{
    LOGS("");
    callcheck();
    WrappedContext *w = container_of(ctx, WrappedContext, hq_ctx);
    if (!w->client) return -VSM_ERROR_GENERIC;
    callback(ctx->root_zone, user_data);
    for (auto& z : w->zones) {
        LOGI("iterate callback zone: " << z.zone->id);
        callback(&z.vz, user_data);
    }
    return 0;
}

API struct vsm_zone *vsm_lookup_zone_by_name(struct vsm_context *ctx, const char *path)
{
    LOGS("name=" << path);
    callcheck();
    WrappedContext *w = container_of(ctx, WrappedContext, hq_ctx);
    VsmZone zone;
    if (!w->client) return NULL;
    //CHECK if path is same as zone_name
    if (w->client->vsm_lookup_zone_by_id(path, &zone) != VSMCLIENT_SUCCESS)
        return NULL;
    return wrap_vsm_zone(w, zone, true);
}

//supposed return ref to internal struct
API struct vsm_zone *vsm_lookup_zone_by_pid(struct vsm_context *ctx, pid_t pid)
{
    LOGS("pid=" << pid);
    callcheck();
    WrappedContext *w = container_of(ctx, WrappedContext, hq_ctx);
    VsmZone zone;
    VsmString id;
    VsmStatus st;
    if (!w->client) return NULL;
    if ((st = w->client->vsm_lookup_zone_by_pid(pid, &id)) != VSMCLIENT_SUCCESS) {
        wrap_error(st, w->client);
        return NULL;
    }
    LOGI("found zone(pid=" << pid << ")='" << id << "'");
    if (::strcmp(id, "host") == 0) {
        return w->hq_ctx.root_zone;
    }
    w->client->vsm_lookup_zone_by_id(id, &zone); //zone is malloced
    return wrap_vsm_zone(w, zone);
}

API int vsm_add_state_changed_callback(struct vsm_context *ctx, vsm_zone_state_changed_cb callback, void *user_data)
{
    LOGS("");
    callcheck();
    WrappedContext *w = container_of(ctx, WrappedContext, hq_ctx);
    VsmSubscriptionId subscriptionId;

    auto dbus_cb = [=](const char* id, const char* dbusAddress, void* data) ->
    void {
        VsmZone zone;
        //TODO what are valid state, event
        vsm_zone_state_t t = VSM_ZONE_STATE_RUNNING;
        UNUSED(dbusAddress);
        w->client->vsm_lookup_zone_by_id(id, &zone);
        callback(wrap_vsm_zone(w, zone), t, data);
    };
    w->client->vsm_add_state_callback(dbus_cb, user_data, &subscriptionId);
    return (int)subscriptionId;
}

API int vsm_del_state_changed_callback(struct vsm_context *ctx, int handle)
{
    LOGS("");
    callcheck();
    WrappedContext *w = container_of(ctx, WrappedContext, hq_ctx);
    VsmSubscriptionId subscriptionId = (VsmSubscriptionId)handle;
    VsmStatus st = w->client->vsm_del_state_callback(subscriptionId);
    return wrap_error(st, w->client);
}

API int vsm_grant_device(struct vsm_zone *dom, const char *name, uint32_t flags)
{
    LOGS("");
    callcheck();
    WrappedZone *w = container_of(dom, WrappedZone, vz);
    const char *id = dom->name;
    VsmZone zone;
    w->client->vsm_lookup_zone_by_id(id, &zone);
    VsmStatus st = w->client->vsm_grant_device(id, name, flags);
    return wrap_error(st, w->client);
}

API int vsm_revoke_device(struct vsm_zone *dom, const char *name)
{
    LOGS("");
    callcheck();
    WrappedZone *w = container_of(dom, WrappedZone, vz);
    const char *id = dom->name;
    VsmStatus st = w->client->vsm_revoke_device(id, name);
    return wrap_error(st, w->client);
}

API struct vsm_netdev *vsm_create_netdev(struct vsm_zone *zone, vsm_netdev_type_t type, const char *target, const char *netdev)
{
    LOGS("");
    callcheck();
    UNUSED(zone);
    UNUSED(type);
    UNUSED(target);
    UNUSED(netdev);

    WrappedZone *w = container_of(zone, WrappedZone, vz);
    const char *id = zone->name;
    VsmStatus st;
    if (type == VSM_NETDEV_VETH)
        st = w->client->vsm_create_netdev_veth(id, target, netdev);
    else if (type == VSM_NETDEV_PHYS)
        st = w->client->vsm_create_netdev_phys(id, netdev);
    else if (type == VSM_NETDEV_MACVLAN) // macvlan_mode from if_link.h
        st = w->client->vsm_create_netdev_macvlan(id, target, netdev, MACVLAN_MODE_BRIDGE);
    else {
        LOGE("Invalid arguments");
        //ctx->error = VSM_ERROR_INVALID;
        return NULL;
    }

    if (st != VSMCLIENT_SUCCESS) {
        LOGE("vsm_create_netdev(" << netdev << ") = " << st);
        return NULL;
    }

    vsm_netdev vnd;
    vnd.zone = zone;
    vnd.name = (char*)netdev; //FIXME? copy content of string
    vnd.type = type;
    w->netdevs.push_back(vnd); //copy pushed to vector
    return &w->netdevs.back(); //pointer to struct on vector
}

API int vsm_destroy_netdev(vsm_netdev_h)
{
    LOGS("");
    return 0;
}

API int vsm_iterate_netdev(struct vsm_zone *zone, void (*callback)(struct vsm_netdev *, void *user_data), void *user_data)
{
    LOGS("");
    callcheck();
    WrappedZone *w = container_of(zone, WrappedZone, vz);
    for (auto nd : w->netdevs) {
        callback(&nd, user_data);
    }
    return 0;
}

API struct vsm_netdev *vsm_lookup_netdev_by_name(struct vsm_zone *zone, const char *name)
{
    LOGS("");
    callcheck();
    WrappedZone *w = container_of(zone, WrappedZone, vz);
    VsmNetdev nd;
    VsmStatus st = w->client->vsm_lookup_netdev_by_name(zone->name, name, &nd);
    if (st == VSMCLIENT_SUCCESS) {
        auto devbyname = [name](const vsm_netdev& v) {return ::strcmp(v.name, name) == 0;};
        auto devlist = std::find_if(w->netdevs.begin(), w->netdevs.end(), devbyname);
        if (devlist != w->netdevs.end()) {
            return &devlist[0];
        }
    }
    return NULL;
}

API int vsm_declare_file(struct vsm_context *ctx, vsm_fso_type_t ftype, const char *path, int flags, vsm_mode_t mode)
{
    LOGS("");
    callcheck();
/*  Old implementation is following: (but implemented in server)
    args.oldpath = oldpath;
    args.newpath = newpath;
    ret = iterate_running_zone("/sys/fs/cgroup/cpuset/lxc", file_resource, &args);
*/
    WrappedContext *w = container_of(ctx, WrappedContext, hq_ctx);
    VsmArrayString ids = NULL;
    VsmFileType type;
    switch (ftype) {
        case VSM_FSO_TYPE_DIR:  /**< Directoy type */
            type = VSMFILE_DIRECTORY;
            break;
        case VSM_FSO_TYPE_REG:  /**< Regular file type */
            type = VSMFILE_REGULAR;
            break;
        case VSM_FSO_TYPE_FIFO: /**< Fifo file type */
            type = VSMFILE_FIFO;
            break;
        case VSM_FSO_TYPE_SOCK: /**< Socket file type */
            return VSM_ERROR_NONE;
        case VSM_FSO_TYPE_DEV:  /**< Device node type */
            return VSM_ERROR_NONE;
        default:
            return VSM_ERROR_NONE;
    }
    w->client->vsm_get_zone_ids(&ids);
    if (ids != NULL) {
        for (VsmString* id = ids; *id; ++id) {
            VsmStatus st = w->client->vsm_declare_file(*id, type, path, (int32_t)flags, (mode_t)mode, NULL);
            if (st != VSMCLIENT_SUCCESS) {
                wrap_error(st, w->client);
            }
        }
    }
    vsm_array_string_free(ids);
    return VSM_ERROR_NONE;
}

API int vsm_declare_link(struct vsm_context *ctx, const char *source, const char *target)
{
    LOGS("src=" << source << ", dst=" << target);
    callcheck();
/*  Old implementation is following: (but implemented in server)
    args.oldpath = oldpath;
    args.newpath = newpath;
    ret = iterate_running_zone("/sys/fs/cgroup/cpuset/lxc", link_resource, &args);
*/
    WrappedContext *w = container_of(ctx, WrappedContext, hq_ctx);
    VsmArrayString ids = NULL;
    w->client->vsm_get_zone_ids(&ids);
    if (ids != NULL)
    for (VsmString* id = ids; *id; ++id) {
        VsmStatus st = w->client->vsm_declare_link(source, *id, target, NULL);
        if (st != VSMCLIENT_SUCCESS)
            wrap_error(st, w->client);
    }
    vsm_array_string_free(ids);
    return VSM_ERROR_NONE;
}

API int vsm_declare_mount(struct vsm_context *ctx,
                          const char *source,
                          const char *target,
                          const char *fstype,
                          unsigned long flags,
                          const void *data)
{
    LOGS("");
    callcheck();
/*  Old implementation is following: (but implemented in server)
    args.oldpath = oldpath;
    args.newpath = newpath;
    ret = iterate_running_zone("/sys/fs/cgroup/cpuset/lxc", mount_resource, &args);
*/
    WrappedContext *w = container_of(ctx, WrappedContext, hq_ctx);
    VsmArrayString ids = NULL;
    w->client->vsm_get_zone_ids(&ids);
    if (ids != NULL) {
        for (VsmString* id = ids; *id; ++id) {
            VsmStatus st = w->client->vsm_declare_mount(source, *id, target, fstype, flags, (const char *)data, NULL);
            if (st != VSMCLIENT_SUCCESS) {
                wrap_error(st, w->client);
            }
        }
    }
    vsm_array_string_free(ids);
    return VSM_ERROR_NONE;
}

API const char * vsm_get_zone_rootpath(vsm_zone_h  zone)
{
    LOGS("");
    return zone == NULL ? NULL : zone->rootfs_path;
}
API const char * vsm_get_zone_name(vsm_zone_h zone)
{
    LOGS("");
    return zone == NULL ? NULL : zone->name;
}
API int vsm_is_host_zone(vsm_zone_h zone)
{
    LOGS("");
    if (zone == NULL)
        return -VSM_ERROR_INVALID;

    return zone->parent == zone ? 1 : 0;
}
API vsm_zone_h vsm_join_zone(vsm_zone_h  /*zone*/)
{
    LOGS("");
    return NULL;
}
API int vsm_canonicalize_path(const char *input_path, char **output_path)
{
    LOGS(""<<input_path);
    *output_path = strdup(input_path);
    int len = strlen(input_path);
    return len;
}

// Note: incomaptible API, support newer
// API(v0.34) const char *vsm_error_string(struct vsm_context *ctx)
// API(v0.3.1) const char *vsm_error_string(vsm_error_e error)
API const char *vsm_error_string(vsm_error_e error)
{
    LOGS("");
    callcheck();
    if (error < 0 || error > VSM_MAX_ERROR) {
        return NULL;
    }
    return vsm_error_strtab[error];
}

API struct vsm_zone *vsm_lookup_zone_by_terminal_id(struct vsm_context *ctx, int terminal)
{
    LOGS("terminal=" << terminal);
    callcheck();
    WrappedContext *w = container_of(ctx, WrappedContext, hq_ctx);
    VsmZone zone;
    VsmString id;
    if (!w->client) return NULL;
    if (w->client->vsm_lookup_zone_by_terminal_id(terminal, &id) != VSMCLIENT_SUCCESS)
        return NULL;
    w->client->vsm_lookup_zone_by_id(id, &zone);
    return wrap_vsm_zone(w, zone);
}

API void vsm_array_string_free(VsmArrayString astring)
{
    if (!astring) {
        return;
    }
    for (char** ptr = astring; *ptr; ++ptr) {
        vsm_string_free(*ptr);
    }
    free(astring);
}

API void vsm_string_free(VsmString string)
{
    free(string);
}

API int vsm_add_event_callback(vsm_context_h, vsm_zone_event_cb, void*) {
    LOGS("");
    return 0;
}
API int vsm_del_event_callback(vsm_context_h, int) {
    LOGS("");
    return 0;
}
API int vsm_add_state_callback(vsm_context_h , vsm_zone_state_cb , void *) {
    LOGS("");
    return 0;
}
API int vsm_del_state_callback(vsm_context_h , int ) {
    LOGS("");
    return 0;
}
API int vsm_down_netdev(vsm_netdev_h) {
    LOGS("");
    return 0;
}
API vsm_zone* vsm_get_foreground(vsm_context_h ctx) {
    LOGS("");
    //return ((struct vasum_ops *)(ctx->vsm_ops))->get_foreground(ctx);
    return dummy_ops.get_foreground(ctx);
}
API int vsm_get_host_pid(vsm_zone_h, pid_t) {
    LOGS("");
    return 0;
}
API int vsm_get_ip_addr_netdev(vsm_netdev_h, vsm_netdev_addr_t, char*, int) {
    LOGS("");
    return 0;
}
API void* vsm_get_userdata(vsm_zone_h) {
    LOGS("");
    return NULL;
}
API int vsm_get_zone_id(vsm_zone_h zone) {
    LOGS("");
    if (zone == NULL)
        return -VSM_ERROR_INVALID;
    return zone->id;
}
API vsm_zone_state_t vsm_get_zone_state(vsm_zone_h zone) {
    LOGS("");
    if (zone == NULL)
        return static_cast<vsm_zone_state_t>(-VSM_ERROR_INVALID);
    return zone->state;
}
API int vsm_get_zone_terminal(vsm_zone_h) {
    LOGS("");
    return -VSM_ERROR_NOT_SUPPORTED;
}
API const char *vsm_get_zone_type(vsm_zone_h zone) {
    LOGS("");
    return zone == NULL ? NULL : zone->type;
}
API int vsm_is_equivalent_zone(vsm_context_h, pid_t) {
    LOGS("");
    return 0;
}
API int vsm_is_virtualized() {
    LOGS("");
    return 0; /* Running in Host */
}
// libs/network.c
API int vsm_set_ip_addr_netdev(vsm_netdev_h, vsm_netdev_addr_t, const char*, int) {
    LOGS("");
    return 0;
}
API int vsm_up_netdev(vsm_netdev_h) {
    LOGS("");
    return 0;
}
// libs/zone.c
API int vsm_set_userdata(vsm_zone_h, void*) {
    LOGS("");
    return 0;
}
API int vsm_state_change_watch_callback(struct vsm_context * /*ctx*/, char * /*name*/,
                    int  /*state*/, int  /*event*/) {
    LOGS("");
    return 0;
}
// libs/vsm_signal.c
API int vsm_signal_state_broadcast(struct mxe_engine * /*engine*/, const char * /*zone_name*/, int  /*state*/) {
    LOGS("");
    return 0;
}
API int vsm_signal_event_broadcast(struct mxe_engine * /*engine*/, const char * /*zone_name*/, int  /*event*/) {
    LOGS("");
    return 0;
}
} // extern "C"
