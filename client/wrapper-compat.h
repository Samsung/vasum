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
 * @brief   Vasum old API compatibility functions
 */

#ifndef __CLIENT_WRAPPER_COMPAT__
#define __CLIENT_WRAPPER_COMPAT__

#include "vasum.h"
#include <sys/epoll.h>

extern "C" {
typedef int (*fp_create_zone) (vsm_context_h ctx, const char *zone_name,
                   const char *template_name, int flag);
typedef int (*fp_destroy_zone) (vsm_context_h ctx, const char *zone_name,
                int force);
typedef int (*fp_start_zone) (vsm_context_h ctx, const char *zone_name);
typedef int (*fp_shutdown_zone) (vsm_context_h ctx, const char *zone_name,
                 int force);
typedef int (*fp_lock_zone) (vsm_context_h ctx, const char *zone_name,
                 int shutdown);
typedef int (*fp_unlock_zone) (vsm_context_h ctx, const char *zone_name);
typedef int (*fp_set_foreground) (vsm_zone_h zone);
typedef vsm_zone_h(*fp_get_foreground) (vsm_context_h ctx);
typedef int (*fp_iterate_zone) (vsm_context_h ctx, vsm_zone_iter_cb callback,
                void *user_data);
typedef vsm_zone_h(*fp_lookup_zone_by_name) (vsm_context_h ctx,
                         const char *name);
typedef vsm_zone_h(*fp_lookup_zone_by_pid) (vsm_context_h ctx, pid_t pid);
typedef int (*fp_attach_zone) (vsm_context_h ctx, const char *zone_name,
                   vsm_attach_command_s * command,
                   vsm_attach_options_s * opt,
                   pid_t * attached_process);
typedef int (*fp_attach_zone_wait) (vsm_context_h ctx, const char *zone_name,
                    vsm_attach_command_s * command,
                    vsm_attach_options_s * opt);
typedef vsm_zone_h(*fp_join_zone) (vsm_zone_h zone);
typedef int (*fp_is_equivalent_zone) (vsm_context_h ctx, pid_t pid);
typedef int (*fp_get_host_pid) (vsm_zone_h zone, pid_t pid);
typedef int (*fp_grant_device) (vsm_zone_h zone, const char *path,
                uint32_t flags);
typedef int (*fp_revoke_device) (vsm_zone_h zone, const char *path);
typedef int (*fp_declare_file) (vsm_context_h ctx, vsm_fso_type_t ftype,
                const char *path, int flags, vsm_mode_t mode);
typedef int (*fp_declare_link) (vsm_context_h ctx, const char *source,
                const char *target);
struct vasum_ops {
    fp_create_zone create_zone;
    fp_destroy_zone destroy_zone;
    fp_start_zone start_zone;
    fp_shutdown_zone shutdown_zone;
    fp_lock_zone lock_zone;
    fp_unlock_zone unlock_zone;
    fp_set_foreground set_foreground;
    fp_get_foreground get_foreground;
    fp_iterate_zone iterate_zone;
    fp_lookup_zone_by_name lookup_zone_by_name;
    fp_lookup_zone_by_pid lookup_zone_by_pid;
    fp_attach_zone attach_zone;
    fp_attach_zone_wait attach_zone_wait;
    fp_join_zone join_zone;
    fp_get_host_pid get_host_pid;
    fp_is_equivalent_zone is_equivalent_zone;
    fp_grant_device grant_device;
    fp_revoke_device revoke_device;
    fp_declare_file declare_file;
    fp_declare_link declare_link;
};

#define SERVICEPATH "\0/domain-control/service.sock"
struct mainloop {
    int     epfd;
    pthread_mutex_t ml_mutex;
    pthread_rwlock_t lock;
    struct adt_list watches;
};
struct mxe_emple;
struct mxe_endpoint;
struct mxe_emple {
    int signature;
    int refcnt;
    void *callback;
    void *data;
    struct mxe_proxy *proxy;
    struct adt_list queue;
};
struct mxe_engine {
    void *data;
    struct mainloop *mainloop;
    pthread_rwlock_t endpoint_lock;
    struct adt_list endpoints;
};
struct mxe_endpoint {
    int fd;
    int type;
    struct mxe_engine *engine;
    pthread_rwlock_t queue_lock;
    pthread_mutex_t rd_mutex;
    pthread_mutex_t wr_mutex;
    struct adt_list queue;
    struct adt_list list;
};

typedef enum {
    ML_EVT_IN   = EPOLLIN,
    ML_EVT_OUT  = EPOLLOUT,
    ML_EVT_RDHUP    = EPOLLRDHUP,
    ML_EVT_ERROR    = EPOLLERR,
    ML_EVT_HUP  = EPOLLHUP,
    ML_EVT_ET   = EPOLLET
}mainloop_event;

typedef struct vsm_context {
    struct mxe_endpoint *signal_channel;
    struct mxe_endpoint *manage_method_channel;
    struct mxe_endpoint *unpriv_method_channel;
    vsm_error_e error;
    pthread_rwlock_t lock;
    struct adt_list listeners;
    struct vsm_zone *root_zone;
    struct vsm_zone *foreground_zone;
    struct adt_list sc_listeners;
    struct adt_list ev_listeners;
    const struct vasum_ops *vsm_ops;
} vsm_context_s;

typedef struct vsm_zone {
    struct vsm_zone *parent;
    char *name;
    char *type;
    int terminal;
    vsm_zone_state_t state;
    char *rootfs_path;
    pthread_rwlock_t lock;
    struct adt_list children;
    struct adt_list devices;
    struct adt_list netdevs;
    void *user_data;
    struct adt_list list;
    struct vsm_context *ctx;
    int id;
} vsm_zone_s;

typedef struct vsm_netdev {
    struct vsm_zone *zone;
    char *name;
    vsm_netdev_type_t type;
    struct adt_list list;
} vsm_netdev_s;


typedef int (*dev_enumerator)(int type, int major, int minor, void *data);
typedef int (*mainloop_callback)(int fd, mainloop_event event, void *data, struct mainloop *mainloop);

struct mainloop *mainloop_create(void);
struct mxe_endpoint *mxe_create_client(struct mxe_engine *engine, const char * /*addr*/);
struct mxe_engine *mxe_prepare_engine(struct mainloop *mainloop, void *data);

int wait_for_pid_status(pid_t  pid);

int vsm_add_state_changed_callback(vsm_context_h  /*ctx*/, vsm_zone_state_changed_cb  /*callback*/, void * /*user_data*/);
int vsm_del_state_changed_callback(vsm_context_h  /*ctx*/, int  /*id*/);
const char * vsm_get_zone_rootpath(vsm_zone_h  /*zone*/);
const char * vsm_get_zone_name(vsm_zone_h  /*zone*/);
int vsm_is_host_zone(vsm_zone_h  /*zone*/);
vsm_zone_h vsm_join_zone(vsm_zone_h zone);
int vsm_canonicalize_path(const char *input_path, char **output_path);

} //extern "C"

#endif
