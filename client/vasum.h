/*
 * Container Control Framework
 *
 * Copyright (c) 2000 - 2014 Samsung Electronics Co., Ltd All Rights Reserved
 * 
 * Contact: Krzysztof Dynowski <k.dynowski@samsung.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */
#ifndef __VASUM_H__
#define __VASUM_H__

#include <unistd.h>
#include <limits.h>
#include <inttypes.h>
#include <sys/types.h>
#include <sys/mount.h>

#include "sc_adt.h"

#ifndef API
#define API __attribute__((visibility("default")))
#endif // API

#ifdef __cplusplus
extern "C" {
#endif

/*
 * @file        vasum.h
 * @version     0.2
 * @brief       This file contains APIs of the Container control Framework
 */

/*
 * <tt>
 *
 * Revision History:
 *
 * </tt>
 */

/**
 * @addtogroup ZONE_CONTROL Zone Control
 * @{
*/

typedef enum {
	VSM_ERROR_NONE,			/* The operation was successful */
	VSM_ERROR_GENERIC,		/* Non-specific cause */
	VSM_ERROR_INVALID,		/* Invalid argument */
	VSM_ERROR_CANCELED,		/* The requested operation was cancelled */
	VSM_ERROR_ABORTED,		/* Operation aborted */
	VSM_ERROR_REFUSED,		/* Connection refused */
	VSM_ERROR_EXIST,		/* Target exists */
	VSM_ERROR_BUSY,			/* Resource is busy */
	VSM_ERROR_IO,			/* I/O error*/
	VSM_ERROR_TIMEOUT,		/* Timer expired */
	VSM_ERROR_OVERFLOW,		/* Value too large to be stored in data type */
	VSM_ERROR_OUT_OF_MEMORY,	/* No memory space */
	VSM_ERROR_OUT_OF_RANGE,		/* Input is out of range */
	VSM_ERROR_NOT_PERMITTED,		/* Operation not permitted */
	VSM_ERROR_NOT_IMPLEMENTED,	/* Function is not implemented yet */
	VSM_ERROR_NOT_SUPPORTED,	/* Operation is not supported */
	VSM_ERROR_ACCESS_DENIED,	/* Access privilege is not sufficient */
	VSM_ERROR_NO_OBJECT,		/* Object not found */
	VSM_ERROR_BAD_STATE,		/* Bad state */
	VSM_MAX_ERROR = VSM_ERROR_BAD_STATE
}vsm_error_t;

/**
 *@brief struct vsm_context keeps track of an execution state of the container control framework.
 */
struct vsm_context {
	/// — Used for internal —
	void		 *data;
	/// Error code
	vsm_error_t	error;
	/// RWLock for list members
	pthread_rwlock_t lock;
	/// List of callback function for changing state
	struct adt_list	listeners;
	/// Root(host) of zones (except for stopped zones)
	struct vsm_zone *root_zone;
	/// Foreground zone 
	struct vsm_zone *foreground_zone;
};

/**
 * @brief Get last error string.
 *
 * \par Synopsis:
 * \code
 * #include <vasum.h>
 *
 * const char *vsm_error_string(struct vsm_context *ctx);
 * \endcode
 *
 * \par Description:
 * vsm_error_string return last error string for debug or logging.
 *
 * \param[in] ctx vsm context
 *
 * \return string for last error in context.
 *
 * \par Known issues/bugs:
 * None
 *
 * \pre None
 *
 * \post None
 *
 * \see struct vsm_context
 *
 * \remarks None
 *
 * \par Sample code:
 * \code
 * #include <vasum.h>
 * ...
 * struct vsm_context *ctx;
 * struct vsm_zone * zone;
 *
 * ctx = vsm_create_context();
 * zone = vsm_lookup_zone_by_pid(ctx, pid);
 * if(zone == NULL)
 * {
 * 		fprintf(stderr, "API Failed : %s", vsm_error_string(ctx));
 * }
 * ...
 * \endcode
 *
*/

API const char *vsm_error_string(struct vsm_context *ctx);

/**
 * @brief Create vsm context
 *
 * \par Synopsis:
 * \code
 * #include <vasum.h>
 *
 * struct vsm_context *vsm_create_context(void *)
 * \endcode
 *
 * \par Description:
 * The vsm context is an abstraction of the logical connection between the zone controller and it's clients.
 * The vsm context must be initialized before attempting to use almost any of the APIs,
 * and it should be finalized when interaction with the zone controller is no longer required.\n
 * \n
 * A call to vsm_create_context() makes a connection to zone controller(vsm-zone-svc),
 * and creates a new instance of struct vsm_context called vsm context.\n
 *
 * \param None
 *
 * \return An instance of vsm context on success, or NULL on error (in which case, errno is set appropriately)
 *
 * \par Known issues/bugs:
 * None
 *
 * \pre vsm-zone-svc must be started 
 *
 * \post None
 *
 * \see vsm_cleanup_context()
 *
 * \remarks None
 *
 * \par Sample code:
 * \code
 * #include <vasum.h>
 * ...
 * struct vsm_context *ctx;
 *
 * ctx = vsm_create_context();
 * if(ctx == NULL)
 * {
 *      printf("Error has occurred\n");
 *      exit(0);
 * }
 * ...
 * \endcode
 *
*/
API struct vsm_context *vsm_create_context(void);

/**
 * @brief Cleanup zone control context
 *
 * \par Synopsis:
 * \code
 * #include <vasum.h>
 *
 * int vsm_cleanup_context(struct vsm_context *ctx)
 *\endcode
 *
 * \par Description:
 * vsm_cleanup_context() finalizes vsm context and release all resources allocated to the vsm context.\n
 * \n
 * This function should be called if interaction with the zone controller is no longer required.
 *
 * \param[in] ctx vsm context
 *
 * \return 0 on success, or -1 on error.
 *
 * \par Known issues/bugs:
 * None
 *
 * \pre vsm context must be initialized by vsm_create_context()
 *
 * \post vsm context will not be valid after calling this API
 *
 * \see vsm_create_context()
 *
 * \remarks None
 *
 * \par Sample code:
 * \code
 * #include <vasum.h>
 * ...
 * int retval;
 * struct vsm_context *ctx;
 * ...
 * retval = vsm_cleanup_context(ctx);
 * if(retval < 0)
 * {
 *      printf("Error has occurred\n");
 *      exit(0);
 * }
 * ...
 * \endcode
 *
*/
API int vsm_cleanup_context(struct vsm_context *ctx);

/**
 * @brief Get file descriptor associated with event dispatcher of zone control context
 *
 * \par Synopsis:
 * \code
 * #include <vasum>
 *
 * int vsm_get_poll_fd(struct vsm_context *ctx)
 * \endcode
 *
 * \par Description:
 * The function vsm_get_poll_fd() returns the file descriptor associated with the event dispatcher of vsm context.\n
 * The file descriptor can be bound to another I/O multiplexing facilities like epoll, select, and poll.
 *
 * \param[in] ctx vsm context
 *
 * \return On success, a nonnegative file descriptor is returned. On error -1 is returned.
 *
 * \par Known issues/bugs:
 * None
 *
 * \pre vsm context must be initialized by vsm_create_context()
 *
 * \post None
 *
 * \see vsm_create_context(), vsm_enter_eventloop()
 *
 * \remarks None
 *
 * \par Sample code:
 * \code
 * #include <vasum.h>
 * ...
 * int fd;
 * struct vsm_context *ctx;
 * ...
 * fd = vsm_get_poll_fd(ctx);
 * while (1) {
 * 	...
 * 	epoll_wait(fd, events, MAX_EPOLL_EVENTS, -1);
 * 	...
 * 	if (vsm_enter_eventloop(ctx, 0, 0) < 0)
 * 		break;
 * 	...
 * }
 * ...
 * \endcode
 *
*/
API int vsm_get_poll_fd(struct vsm_context *ctx);
/**
 * @brief Wait for an I/O event on a VSM context
 *
 * \par Synopsis:
 * \code
 * #incldue <vasum.h>
 * 
 * int vsm_enter_eventloop(struct vsm_context *ctx, int flags, int timeout)
 * \endcode
 * 
 * \par Description:
 * vsm_enter_eventloop() waits for event on the vsm context.\n
 *\n
 * The call waits for a maximum time of timout milliseconds. Specifying a timeout of -1 makes vsm_enter_eventloop() wait indefinitely,
 * while specifying a timeout equal to zero makes vsm_enter_eventloop() to return immediately even if no events are available.
 *
 * \param[in] ctx vsm context
 * \param[in] flags Reserved
 * \param[in] timeout Timeout time (milisecond), -1 is infinite
 *
 * \return 0 on success, or -1 on error.
 *
 * \par Known issues/bugs:
 * None
 *
 * \pre vsm context must be initialized by vsm_create_context()
 *
 * \post None
 *
 * \see vsm_create_context(), vsm_get_poll_fd()
 *
 * \remarks None
 *
 * \par Sample code:
 * \code
 * #include <vasum.h>
 * ...
 * struct vsm_context *ctx;
 * ...
 * while (vsm_enter_eventloop(ctx, 0, -1)) {
 * ...
 * }
 * ...
 * \endcode
 *
*/
API int vsm_enter_eventloop(struct vsm_context *ctx, int flags, int timeout);

/**
 * @brief Definition for default zone name.
 * Default zone is started at boot sequence by systemd.
*/
#define VSM_DEFAULT_ZONE "personal"

/**
 * @brief Definition for zone states.
 * This definition shows the available states.
 * Zone is started from VSM_ZONE_STATE_STOPPED state. During start up, zone controller instantiates zone and
 * starts the first process in the zone.
 * Once the first process has started, the state of the zone will not be changed unless
 * the process is killed.
*/

typedef enum {
	VSM_ZONE_STATE_STOPPED,
	VSM_ZONE_STATE_STARTING,
	VSM_ZONE_STATE_RUNNING,
	VSM_ZONE_STATE_STOPPING,
	VSM_ZONE_STATE_ABORTING,
	VSM_ZONE_STATE_FREEZING,
	VSM_ZONE_STATE_FROZEN,
	VSM_ZONE_STATE_THAWED,
	VSM_ZONE_MAX_STATE = VSM_ZONE_STATE_THAWED
} vsm_zone_state_t;

typedef enum {
	VSM_ZONE_EVENT_NONE,
	VSM_ZONE_EVENT_CREATED,
	VSM_ZONE_EVENT_DESTROYED,
	VSM_ZONE_EVENT_SWITCHED,
	VSM_ZONE_MAX_EVENT = VSM_ZONE_EVENT_SWITCHED
} vsm_zone_event_t;

/**
 * @brief Zone structure which is used to represent an instance of container.
 */
struct vsm_zone {
	/// Parent
	struct vsm_zone *parent;
	/// Name (null-terminated string)
	char *name;
	/// Zone type (null-terminated string)
	char *type;
	/// Domain's Virtual Terminal number
	int terminal;
	/// State
	vsm_zone_state_t state;
	/// Container's rootfs path
	char *rootfs_path;
	/// RWLock for list members
	pthread_rwlock_t lock;
	/// List of child zones
	struct adt_list children;
	/// List of devices in zone
	struct adt_list devices;
	/// List of network devices in zone
	struct adt_list netdevs;
	/// Pointer of user-defined data for clients.
	void *user_data;
	///List head
	struct adt_list list;
};

/**
 * @brief Create a new persistent zone
 *
 * \par Synopsis
 * \code
 * #include <vasum.h>
 *
 * int vsm_create_zone(struct vsm_context *ctx, const char *name, const char *template_name, int flag)
 * \endcode
 *
 * \par Description:
 * vsm_create_zone() triggers zone controller to create new persistent zone.\n
 * \n
 * The zone controller distinguishes two types of zones: peristent and transient.
 * Persistent zones exist indefinitely in storage.
 * While, transient zones are created and started on-the-fly.\n
 * \n
 * Creating zone requires template.
 * In general, creating a zone involves constructing root file filesystem and 
 * generating configuration files which is used to feature the zone.
 * Moreover, it often requires downloading tools and applications pages,
 * which are not available in device.
 * In that reason, this Domain Separation Framework delegates this work to templates.
 *
 * \param[in] ctx vsm context
 * \param[in] zone_name zone name
 * \param[in] template_name template name to be used for constructing the zone
 * \param[in] flag Reserved
 *
 * \return 0 on success, or negative integer error code on error.
 *
 * \par Known issues/bugs:
 * None
 *
 * \pre vsm context must be initialized by vsm_create_context()
 *
 * \post None
 *
 * \see vsm_create_context(), vsm_start_zone(), vsm_shutdown_zone(), vsm_destroy_zone()
 *
 * \remarks By default, a zone is in an inactive state even if it has succefully created.
 * Thus, the zone should be unlocked explicitly to start its execution.\n
 *
 * \par Sample code:
 * \code
 * #include <vasum.h>
 * ...
 * int retval;
 * struct vsm_context *ctx;
 * ...
 * retval = vsm_create_zone(ctx, "personal", "tizen", 0);
 * if(retval < 0)
 * {
 *      printf("Error has occurred\n");
 *      exit(0);
 * }
 * ...
 * \endcode
 *
*/
API int vsm_create_zone(struct vsm_context *ctx, const char *zone_name, const char *template_name, int flag);
/**
 * @brief Destroy persistent zone
 * 
 * \par Synopsis:
 * \code
 * #include <vasum.h>
 *
 * int vsm_destroy_zone(struct vsm_context *ctx, const char *name, int force)
 * \endcode
 *
 * \par Description:
 * The function vsm_destroy_zone() triggers zone controller to destroy persistent zone corresponding to the given name.\n
 * All data stored in the repository of the zone are also deleted if force argument is set.
 * Once the zone repository is deleted, it cannot be recovered.
 *
 * \param[in] ctx vsm context
 * \param[in] zone_name Domain name
 * \param[in] force
 * - 0 is Remaining the zone when the zone is running
 * - non-zero is Destroy the zone even though the zone is running
 *
 * \return 0 on success, or negative integer error code on error.
 *
 * \par Known issues/bugs:
 * Parameter "force" is not implemented yet, now it works as non-zero.
 *
 * \pre vsm context must be initialized by vsm_create_context()
 *
 * \post None
 *
 * \see vsm_create_context(), vsm_create_zone(), vsm_shutdown_zone(), vsm_destroy_zone()
 *
 * \remarks None
 *
 * \par Sample code:
 * \code
 * #include <vasum.h>
 * ...
 * int retval;
 * struct vsm_context *ctx;
 * ...
 * retval = vsm_destroy_zone(ctx, "personal", 1);
 * if(retval < 0)
 * {
 *      printf("Error has occurred\n");
 *      exit(0);
 * }
 * ...
 * \endcode
 *
*/
API int vsm_destroy_zone(struct vsm_context *ctx, const char *zone_name, int force);
/**
 * @brief Start an execution of a persistent zone
 *
 * \par Synopsis:
 * \code
 * #include <vasum.h>
 *
 * int vsm_start_zone(struct vsm_context *ctx, const char *name)
 * \endcode
 * 
 * \par Description:
 * The function vsm_start_zone() triggers zone controller to start zone corresponding to the given name.\n
 * Caller can speficy the type of zones: transient and peristent.
 * Persistent zones need to be defined before being start up(see vsm_create_zone())
 * While, transient zones are created and started on-the-fly.
 *
 * \param[in] ctx vsm context
 * \param[in] zone_name zone name
 *
 * \return 0 on success, or negative integer error code on error.
 *
 * \par Known issues/bugs:
 * None
 *
 * \pre vsm context must be initialized by vsm_create_context()
 *
 * \post None
 *
 * \see vsm_create_context(), vsm_shutdown_zone(), vsm_create_zone(), vsm_destroy_zone()
 *
 * \remarks None
 *
 * \par Sample code:
 * \code
 * #include <vasum.h>
 * ...
 * int retval;
 * struct vsm_context *ctx;
 * ...
 * retval = vsm_start_zone(ctx, "personal");
 * if(retval < 0)
 * {
 *      printf("Error has occurred\n");
 *      exit(0);
 * }
 * ...
 * \endcode
 *
*/
API int vsm_start_zone(struct vsm_context *ctx, const char *zone_name);
/**
 * @brief Stop an execution of running zone
 *
 * \par Synopsys
 * \code
 * #include <vasum.h>
 *
 * int vsm_shutdown_zone(struct vsm_context * ctx, struct zone * zone, int force)
 * \endcode
 *
 * \par Description:
 * vsm_shutdown_zone() triggers zone controller to stop execution of the zone corresponsing to the given zone.\n
 *
 * \par Important notes:
 * None
 *
 * \param[in] ctx vsm context
 * \param[in] zone_name zone name
 * \param[in] force option to shutdown.
 *            Without if 0 send SIGPWR signal to init process in zone,
              otherwise, terminate all processes in zone forcefully.
 *
 * \return 0 on success, or negative integer error code on error.
 *
 * \par Known issues/bugs:
 * None
 *
 * \pre vsm context must be initialized by vsm_create_context()
 *
 * \post None
 *
 * \see vsm_create_context(), vsm_cleanup_context(), vsm_create_zone(), vsm_destroy_zone(), vsm_start_zone()
 *
 * \remarks None
 *
 * \par Sample code:
 * \code
 * #include <vasum.h>
 * ...
 * int retval;
 * struct vsm_context *ctx;
 * ...
 * retval = vsm_shutdown_zone(ctx, "zone", 1);
 * if(retval < 0)
 * {
 *      printf("Error has occurred\n");
 *      exit(0);
 * }
 * ...
 * \endcode
 *
*/
API int vsm_shutdown_zone(struct vsm_context *ctx, const char *zone_name, int force);

/**
 * @brief Prevent starting zone
 *
 * \par Synopsys
 * \code
 * #include <vasum.h>
 *
 * int vsm_lock_zone(struct vsm_context *ctx, const char *name, int shutdown);
 * \endcode
 *
 * \par Description:
 * Prevent starting a zone
 *
 * \param[in] ctx vsm context
 * \param[in] zone_name zone name
 *
 * \return 0 on success, or negative integer error code on error.
 *
 * \par Known issues/bugs:
 * None
 *
 * \pre vsm context must be initialized by vsm_create_context()
 *
 * \post None
 *
 * \see vsm_create_context(), vsm_cleanup_context(), vsm_create_zone(), vsm_start_zone(), vsm_unlock_zone()
 *
 * \remarks None
 *
 * \par Sample code:
 * \code
 * #include <vasum.h>
 * ...
 * struct vsm_context *ctx;
 * ...
 * ret = vsm_lock_zone(ctx, "zone");
 * if(ret < 0)
 * {
 *      printf("Error has occurred\n");
 *      exit(0);
 * }
 * ...
 * \endcode
 *
*/
API int vsm_lock_zone(struct vsm_context *ctx, const char *zone_name, int shutdown);

/**
 * @brief Allow a zone to be launched
 *
 * \par Synopsys
 * \code
 * #include <vasum.h>
 *
 * int vsm_unlock_zone(struct vsm_context *ctx, const char *name)
 * \endcode
 *
 * \par Description:
 * Remove lock applied to the zone corresponding to the given name.
 *
 * \param[in] ctx vsm context
 * \param[in] zone_name zone name
 *
 * \return 0 on success, or negative integer error code on error.
 *
 * \par Known issues/bugs:
 * None
 *
 * \pre vsm context must be initialized by vsm_create_context()
 *
 * \post None
 *
 * \see vsm_create_context(), vsm_cleanup_context(), vsm_lock_zone(), vsm_create_zone(), vsm_start_zone()
 *
 * \remarks None
 *
 * \par Sample code:
 * \code
 * #include <vasum.h>
 * ...
 * struct vsm_context *ctx;
 * ...
 * if (vsm_unlock_zone(ctx, "zone") < 0) {
 *      printf("Error has occurred\n");
 *      exit(0);
 * }
 * ...
 * \endcode
 *
*/
API int vsm_unlock_zone(struct vsm_context *ctx, const char *zone_name);


API int vsm_set_foreground(struct vsm_zone * zone);


typedef struct vsm_attach_command_t {
	/** @brief excutable file path */
	char * exec;
	/** @brief arguments including the executable binary itself in argv[0] */
	char ** argv;
} vsm_attach_command_t;

/**
 * @brief Domain attach option
 */
typedef struct vsm_attach_options_t {
	/** @brief user id */
	uid_t uid;
	/** @brief group id  */
	gid_t gid;
	/** @brief number of environment variables */
	int env_num;
	/** @brief environment variables */
	char **extra_env;
} vsm_attach_options_t;

#define VSM_ATTACH_OPT_DEFAULT \
{ \
	/*.uid =*/ (uid_t)0, \
	/*.gid =*/ (gid_t)0, \
	/*.env_num=*/ 0, \
	/*.extra_env = */ NULL \
}

/**
 * @brief Launch a process in a running zone.
 *
 * \par Synopsys
 * \code
 * #include <vasum.h>
 *
 * int vsm_attach_zone(struct vsm_context *ctx, const char * name, vsm_attach_command_t * command, vsm_attach_options_t * opt)
 * \endcode
 *
 * \par Description:
 * Execute specific command inside the container with given arguments and environment
 *
 * \param[in] ctx vsm context
 * \param[in] zone_name zone name
 * \param[in] command vsm attach command
 * \param[in] opt vsm attach options (can be NULL)
 *
 * \return On sucess, exit code of attached process.
 *         otherwise, a negative value.
 *
 * \par Known issues/bugs:
 *
 * \pre None
 *
 * \post None
 *
 * \see vsm_start_zone()
 *
 * \remarks None
 *
 * \par Sample code:
 * \code
 * #include <vsm.h>
 * ...
 * struct vsm_context *ctx;
 * struct vsm_zone *zone;
 * vsm_attach_command_t comm;
 * vsm_attach_options_t opts = VSM_ATTACH_OPT_DEFAULT;
 * char * attach_argv[] = {"-al", "/proc/"};
 *
 * comm.exec = "ls";
 * comm.argc = 2;
 * comm.argv = attach_argv;
 *
 * opts.uid = 5000;
 * opts.gid = 5000;
 * opts.env_num = 0;
 *
 * ...
 * res = vsm_attach_zone(ctx, zone, &comm,  &opts);
 * ...
 * \endcode
 *
 */

API int vsm_attach_zone(struct vsm_context *ctx, struct vsm_zone *zone, vsm_attach_command_t * command, vsm_attach_options_t * opt, pid_t *attached_process);


API int vsm_attach_zone_wait(struct vsm_context *ctx, struct vsm_zone *zone, vsm_attach_command_t * command, vsm_attach_options_t * opt);

/**
 * @brief Interate all zone instances which are in running state
 *
 * \par Synopsys
 * \code
 * #include <vasum.h>
 *
 * int vsm_iterate_zone(struct vsm_context *ctx, void (*callback)(struct vsm_zone *zone, void *user_data), void *user_data)
 * \endcode
 *
 * \par Description:
 * The function vsm_iterate_zone() traverses all zones which are in running state, and callback function will be called on every entry.
 *
 * \param[in] ctx vsm context
 * \param[in] callback Function to be executed in iteration, which can be NULL
 * \param[in] user_data Parameter to be passed to callback function
 *
 * \return 0 on success, or negative integer error code on error.
 *
 * \par Known issues/bugs:
 * None
 *
 * \pre vsm context must be initialized by vsm_create_context()
 *
 * \post None
 *
 * \see vsm_create_context(), vsm_cleanup_context(), vsm_lock_zone(), vsm_unlock_zone(), vsm_start_zone(), vsm_shutdown_zone()
 *
 * \remarks None
 *
 * \par Sample code:
 * \code
 * #include <vasum.h>
 * ...
 * struct vsm_context *ctx;
 * ...
 * void callback(struct vsm_zone *zone, void *user_data)
 * {
 * 	printf("Zone : %s\n", zone->name);
 * }
 * ...
 * zone = vsm_iterate_zone(ctx, callback, NULL);
 * if(netdev == NULL)
 * {
 *      printf("Error has occurred\n");
 *      exit(0);
 * }
 * ...
 * \endcode
 *
*/
API int vsm_iterate_zone(struct vsm_context *ctx, void (*callback)(struct vsm_zone *zone, void *user_data), void *user_data);

/**
 * @brif Find zone corresponding to the name
 *
 * \par Synopsys
 * \code
 * #include <vasum>
 *
 * struct zone *vsm_lookup_zone_by_name(struct vsm_context *ctx, const char *path)
 * \endcode
 *
 * \par Description:
 * The function vsm_lookup_zone_by_name() looks for the zone instance corresponding to the given name.
 *
 * \param[in] ctx vsm context
 * \param[in] path zone path to search
 *
 * \return Zone instance on success, or NULL on error.
 *
 * \par Known issues/bugs:
 * None
 *
 * \pre vsm context must be initialized by vsm_create_context()
 *
 * \post None
 *
 * \see vsm_create_context(), vsm_cleanup_context(), vsm_lookup_zone_by_pid(), vsm_lookup_zone_by_terminal(), vsm_iterate_zone() 
 *
 * \remarks None
 *
 * \par Sample code:
 * \code
 * #include <vasum.h>
 * ...
 * struct vsm_zone *zone;
 * struct vsm_context *ctx;
 * ...
 * zone = vsm_lookup_zone_by_name(ctx, "personal");
 * if(zone == NULL)
 * {
 *      printf("Knox isn't exist or isn't running\n");
 * } else {
 *	printf("%s is found", zone->name);
 * }
 * ...
 * \endcode
 *
*/
API struct vsm_zone *vsm_lookup_zone_by_name(struct vsm_context *ctx, const char *path);

/**
 * @brief Find zone instance associated with the process id
 *
 * \par Synopsis:
 * \code
 * #include <vasum.h>
 * 
 * struct zone * vsm_lookup_zone_by_pid(struct vsm_context *ctx, pid_t pid)
 * \endcode
 *
 * \par Description:
 * The function vsm_lookup_zone_by_pid() looks for the zone instance associated with the given pid.
 *
 * \param[in] ctx vsm context
 * \param[in] pid Process id
 *
 * \return Zone instance on success, or NULL on error.
 *
 * \par Known issues/bugs:
 * None
 *
 * \pre vsm context must be initialized by vsm_create_context()
 *
 * \post None
 *
 * \see vsm_create_context(), vsm_cleanup_context(), vsm_lookup_zone_by_name(), vsm_lookup_zone_by_terminal(), vsm_iterate_zone() 
 *
 * \remarks None
 *
 * \par Sample code:
 * \code
 * #include <vasum.h>
 * ...
 * struct vsm_zone *zone;
 * struct vsm_context *ctx;
 * ...
 * zone = vsm_lookup_zone_by_pid(ctx, 1208);
 * if(zone == NULL)
 * {
 *      printf("Not found\n");
 * } else {
 *	printf("%s is found", zone->name);
 * }
 * ...
 * \endcode
 *
*/
API struct vsm_zone *vsm_lookup_zone_by_pid(struct vsm_context *ctx, pid_t pid);

/**
 * @brief Find zone instance associated with the given terminal
 *
 * \par Synopsis:
 * \code
 * #include <vasum.h>
 *
 * struct zone *vsm_lookup_zone_by_terminal_id(struct vsm_context *ctx, int terminal)
 * \endcode
 *
 * \par Purpose:
 * The function vsm_lookup_zone_by_terminal_id() search zone instance which is associated with the given terminal.
 * In general, the terminal id would be tty number.
 *
 * \param[in] ctx vsm context
 * \param[in] terminal Terminal id to search
 *
 * \return Zone instance on success, or NULL on error.
 *
 * \par Known issues/bugs:
 * None
 *
 * \pre vsm context must be initialized by vsm_create_context()
 *
 * \post None
 *
 * \see vsm_create_context(), vsm_cleanup_context(), vsm_start_context(), vsm_shutdown_context(), vsm_lookup_zone_by_name(), vsm_lookup_zone_by_pid(), vsm_iterate_zone() 
 *
 * \remarks None
 *
 * \par Sample code:
 * \code
 * #include <vasum.h>
 * ...
 * struct vsm_zone *zone;
 * struct vsm_context *ctx;
 * ...
 * zone = vsm_lookup_zone_by_terminal_id(ctx, 2);
 * if(zone == NULL)
 * {
 *      printf("Not found\n");
 * } else {
 *	printf("%s is found", zone->name);
 * }
 * ...
 * \endcode
 *
*/
API struct vsm_zone *vsm_lookup_zone_by_terminal_id(struct vsm_context *ctx, int terminal);

/**
 * @brief Definition for zone event callback function.
 */
typedef int (*vsm_zone_state_cb)(struct vsm_zone *zone, vsm_zone_state_t state, vsm_zone_event_t event, void *user_data);

/**
 * @brief Register a callback to be notified when zone state has changed
 *
 * \par Synopsis:
 * \code
 * #include <vasum.h>
 *
 * int vsm_add_state_callback(struct vsm_context *ctx, zone_state_cb callback, void *user_data)
 * \endcode
 *
 * \par Description:
 * Register a callback to be notified when zone state event occurs.
 *
 * \param[in] ctx vsm context
 * \param[in] callback Callback function to invoke when zone satte event occurs
 *
 * \return Callback ID on success, or negative integer error code on error.
 *
 * \par Known issues/bugs:
 * None
 *
 * \pre vsm context must be initialized by vsm_create_context()
 *
 * \post None
 *
 * \see vsm_create_context(), vsm_cleanup_context(), vsm_del_state_callback(), vsm_enter_eventloop(), vsm_get_poll_fd()
 *
 * \remarks None
 *
 * \par Sample code:
 * \code
 * #include <vasum.h>
 * ...
 * int retval;
 * struct vsm_context *ctx;
 * ...
 * void callback(struct zone *zone, vsm_zone_state_t state, vsm_zone_event_t event, void *user_data)
 * {
 * 	if (state == VSM_ZONE_STATE_RUNNING)
 * 		printf("Domain(%s) is running\n", zone->name);
 * }
 * ...
 * retval = vsm_add_state_callback(ctx, callback, NULL);
 * if(retval < 0)
 * {
 *      printf("Error has occurred\n");
 *      exit(0);
 * }
 * ...
 * \endcode
 *
*/
API int vsm_add_state_callback(struct vsm_context *ctx, vsm_zone_state_cb callback, void *user_data);
/**
 * @brief Remove zone event callback
 * 
 * \par Synopsis
 * \code
 * #include <vasum.h>
 *
 * \endcode
 *
 * \par Description:
 * Remove an event callback from the zone control context.
 *
 * \par Important notes:
 * None
 *
 * \param[in] ctx vsm context
 * \param[in] handle Callback Id to remove
 *
 * \return 0 on success, or negative integer error code on error.
 *
 * \par Known issues/bugs:
 * None
 *
 * \pre vsm context must be initialized by vsm_create_context()
 *
 * \post None
 *
 * \see vsm_create_context(), vsm_cleanup_context(), vsm_add_state_callback()
 *
 * \remarks None
 *
 * \par Sample code:
 * \code
 * #include <vasum.h>
 * ...
 * int id;
 * struct vsm_context *ctx;
 * ...
 * void callback(struct zone *zone, vsm_zone_state_t state, vsm_zone_event_t event, void *user_data)
 * {
 * 	if (state == VSM_ZONE_STATE_RUNNING)
 * 		printf("Domain(%s) is running\n", zone->name);
 * }
 * ...
 * id = vsm_add_state_callback(ctx, callback, NULL);
 * ...
 * vsm_del_state_callback(ctx, id);
 * ...
 * \endcode
 *
*/
API int vsm_del_state_callback(struct vsm_context *ctx, int handle);

/// @}

/**
 * @addtogroup DEVICE_CONTROL
 * @{
*/

/**
 * @brief Grant device to zone
 *
 * \par Synopsys:
 * \code
 * #include <vasum.h>
 *
 * int vsm_grant_device(struct vsm_zone *zone, const char *name, uint32_t flags)
 * \endcode
 *
 * \par Description:
 * The function vsm_grant_device() permits the zone using the device corresponding to the given name.
 *
 * \param[in] zone Zone
 * \param[in] name Path of device to grant
 * \param[in] flags O_RDWR, O_WRONLY, O_RDONLY
 *
 * \return 0 on success, or negative integer error code on error.
 *
 * \par Known issues/bugs:
 * None
 *
 * \pre None
 *
 * \post None
 *
 * \see vsm_revoke_device()
 *
 * \remarks None
 *
 * \par Sample code:
 * \code
 * #include <vasum.h>
 * ...
 * int retval;
 * struct vsm_zone *zone;
 * ...
 * retval = vsm_grant_device(zone, "/dev/dri/card0", O_RDWR);
 * if(retval < 0)
 * {
 *      printf("Error has occurred\n");
 *      exit(0);
 * }
 * ...
 * \endcode
 *
*/
API int vsm_grant_device(struct vsm_zone *dom, const char *name, uint32_t flags);
/**
 * @brief Revoke device from the zone
 * 
 * \par Synopsis:
 * \code
 * #include <vasum.h>
 *
 * int vsm_revoke_device(struct vsm_zone *zone, const char *name)
 * \endcode
 *
 * \par Description:
 *
 * \param[in] zone Zone
 * \param[in] name Host-side path of device to revoke
 *
 * \return 0 on success, or negative integer error code on error.
 *
 * \par Known issues/bugs:
 * None
 *
 * \pre vsm context must be initialized by vsm_create_context()
 *
 * \post None
 *
 * \see vsm_grant_device()
 *
 * \remarks None
 *
 * \par Sample code:
 * \code
 * #include <vasum.h>
 * ...
 * int retval;
 * struct vsm_zone *zone;
 * ...
 * retval = vsm_revoke_device(dom, "/dev/samsung_sdb");
 * if(retval < 0)
 * {
 *      printf("Error has occurred\n");
 *      exit(0);
 * }
 * ...
 * \endcode
 *
*/
API int vsm_revoke_device(struct vsm_zone *dom, const char *name);

/// @}

/**
 * @addtogroup NETWORK_CONTROL
 * @{
*/

/**
 * @brief Types of virtual network interfaces
 */
typedef enum {
	///Virtual Ethernet(veth), this type device will be attached to host-side network bridge
	VSM_NETDEV_VETH,
	///Physical device
	VSM_NETDEV_PHYS,
	///Mac VLAN, this type isn't implemented yet
	VSM_NETDEV_MACVLAN
} vsm_netdev_type_t;

/**
 * @brief Network device information
 */
struct vsm_netdev {
	/// Domain
	struct vsm_zone *zone;
	/// Device name
	char *name;
	/// Type
	vsm_netdev_type_t type;

	///List head
	struct adt_list list;
};

/**
 * @brief Create a new network device and assisgn it to zone
 *
 * \par Synopsis:
 * \code
 * #include <vasum.h>
 *
 * struct vsm_netdev *vsm_create_netdev(struct vsm_zone *zone, vsm_netdev_type_t type, const char *target, const char *netdev)
 * \endcode
 *
 * \par Description:
 * This function creates net netdev instance and assign it to the given zone.
 *
 * \param[in] zone Zone
 * \param[in] type Type of network device to be create
 * \param[in] target
 * - If type is veth, this will be bridge name to attach.
 * - If type is phys, this will be ignored.
 * \param[in] netdev Name of network device to be create
 *
 * \return Network devce on success, or NULL on error.
 *
 * \par Known issues/bugs:
 * None
 *
 * \pre vsm context must be initialized by vsm_create_context()
 *
 * \post None
 *
 * \see vsm_create_netdev()
 *
 * \remarks None
 *
 * \par Sample code:
 * \code
 * #include <vasum.h>
 * ...
 * int retval;
 * struct vsm_netdev *netdev;
 * ...
 * netdev = vsm_create_netdev(zone, VSM_NETDEV_VETH, "virbr0", "vethKnox");
 * if(retval < 0)
 * {
 *      printf("Error has occurred\n");
 *      exit(0);
 * }
 * ...
 * \endcode
 *
 * Macvlan isn't implemented yet.
*/
API struct vsm_netdev *vsm_create_netdev(struct vsm_zone *zone, vsm_netdev_type_t type, const char *target, const char *netdev);
/**
 * @brief Removes a network device from zone control context
 *
 * \par Synopsis:
 * \code
 * #include <vasum.h>
 *
 * int vsm_destroy_netdev(struct vsm_zone *zone, struct vsm_netdev *netdev)
 * \endcode
 *
 * \par Description:
 * This function remove the given netdev instance from the zone control context.
 *
 * \param[in] zone zone
 * \param[in] netdev network device to be removed
 *
 * \return 0 on success, or negative integer error code on error.
 *
 * \par Known issues/bugs:
 * None
 *
 * \pre vsm context must be initialized by vsm_create_context()
 *
 * \post None
 *
 * \see vsm_destroy_netdev()
 *
 * \remarks None
 *
 * \par Sample code:
 * \code
 * #include <vasum.h>
 * ...
 * struct vsm_zone *zone;
 * struct vsm_netdev *netdev;
 * ...
 * netdev = vsm_create_netdev(zone, VSM_NETDEV_VETH, "virbr0", "vethKnox");
 * if(netdev == NULL)
 * {
 *      printf("Error has occurred\n");
 *      exit(0);
 * }
 * ...
 * vsm_destroy_netdev(zone, netdev);
 * ...
 * \endcode
 *
*/
API int vsm_destroy_netdev(struct vsm_zone *zone, struct vsm_netdev *netdev);

/**
 * @brief Iterates network devices found in the zone vsm context and executes callback() on each interface.
 *
 * \par Synopsis:
 * \code
 * #include <vasum.h>
 * 
 * int vsm_iterate_netdev(struct vsm_zone *zone, void (*callback)(struct vsm_netdev *, void *user_data), void *user_data)
 * \endcode
 *
 * \par Description:
 * vsm_destroy_netdev() scans all network interfaces which are registered in the vsm context, and calls callback() on each entry.
 *
 * \param[in] zone Zone
 * \param[in] callback Function to be executed in iteration, which can be NULL
 * \param[in] user_data Parameter to be delivered to callback function
 *
 * \return 0 on success, or negative integer error code on error.
 *
 * \par Known issues/bugs:
 *
 * \pre vsm context must be initialized by vsm_create_context()
 *
 * \post None
 *
 * \see vsm_create_netdev(), vsm_destroy_netdev()
 *
 * \remarks None
 *
 * \par Sample code:
 * \code
 * #include <vasum.h>
 * ...
 * struct zone *zone;
 * ...
 * void callback(struct vsm_neatdev *netdev, void *user_data)
 * {
 * 	printf("Network device : %s\n", netdev->name);
 * }
 * ...
 * netdev = vsm_iterate_netdev(zone, callback, NULL);
 * if(netdev == NULL)
 * {
 *      printf("Error has occurred\n");
 *      exit(0);
 * }
 * ...
 * \endcode
 *
*/
API int vsm_iterate_netdev(struct vsm_zone *zone, void (*callback)(struct vsm_netdev *, void *user_data), void *user_data);

/**
 * @brief Find a network device corresponding to the name
 *
 * \par Synopsis:
 * \code
 * #include <vasum.h>
 *
 * struct zone_netdev *vsm_lookup_netdev_by_name(struct vsm_zone *zone, const char *name)
 * \endcode
 *
 * \par Description:
 * The function vsm_lookup_netdev_by_name() looks for a network interface corresponding to the given name.
 *
 * \param[in] zone Zone
 * \param[in] name Network device name to search
 *
 * \return Network device on success, or NULL on error.
 *
 * \par Known issues/bugs:
 * None
 *
 * \pre vsm context must be initialized by vsm_create_context()
 *
 * \post None
 *
 * \see vsm_iterate_netdev()
 *
 * \remarks None
 *
 * \par Sample code:
 * \code
 * #include <vasum.h>
 * ...
 * struct vsm_zone *zone;
 * struct vsm_netdev *netdev;
 * ...
 * netdev = vsm_lookup_netdev(zone, "vethKnox");
 * if(netdev == NULL)
 * {
 *      printf("Network device isn't exist\n");
 * } else {
 *	printf("There is %s in %s", netdev->name);
 * }
 * ...
 * \endcode
 *
*/
API struct vsm_netdev *vsm_lookup_netdev_by_name(struct vsm_zone *zone, const char *name);

/// @}

/**
 * @addtogroup RESOURCE_PROVISIONING
 * @{
 */

typedef enum {
	VSM_FSO_TYPE_DIR,
	VSM_FSO_TYPE_REG,
	VSM_FSO_TYPE_FIFO,
	VSM_FSO_TYPE_SOCK,
	VSM_FSO_TYPE_DEV,
	VSM_FSO_MAX_TYPE = VSM_FSO_TYPE_DEV
} vsm_fso_type_t;

typedef mode_t vsm_mode_t;

/**
 * @brief Register file system object to vsm context.
 *
 * \par Synopsis:
 * \code
 * #include <vasum.h>
 *
 * int vsm_declare_file(struct vsm_context *ctx, vsm_fso_type_t ftype, const char *path, int flags, vsm_mode_t mode)
 * \endcode
 *
 * \par Description:
 * Register file system object to vsm context. Once the object is registered to the context, it will be populated when
 * any zone is started.\n
 *
 * \param[in] ctx vsm context
 * \param[in] type Type of file system object
 * \param[in] path Path for the file system object
 * \param[in] flags Flasg
 * \param[in] mode mode
 *
 * \return zero on success, or negative value on error.
 *
 * \par Known issues/bugs:
 * None
 *
 * \pre vsm context must be initialized by vsm_create_context()
 *
 * \post None
 *
 * \see vsm_declare_mount(), vsm_declare_link()
 *
 * \remarks None
 *
 * \par Sample code:
 * \code
 * #include <vasum.h>
 * ...
 * int ret;
 * struct vsm_zone *zone;
 * ...
 * ret = vsm_declare_file(zone, VSM_FSO_TYPE_DIR, "/vethKnox", 0666);
 * if(ret < 0)
 * {
 *      printf("Failed to declare file system object\n");
 * }
 *
 * ...
 * \endcode
 *
*/

API int vsm_declare_file(struct vsm_context *ctx, vsm_fso_type_t ftype, const char *path, int flags, vsm_mode_t mode);

/**
 * @brief Register file system object to vsm context.
 *
 * \par Synopsis:
 * \code
 * #include <vasum.h>
 *
 * int vsm_declare_mount(struct vsm_context *ctx, const char *source, const char *target, const char *fstype, unsigned long flags, const void *data)
 * \endcode
 *
 * \par Description:
 * Register mount to vsm context. Once the mount is registered to the context, it will be populated when
 * any zone is started.\n
 *
 * \param[in] ctx vsm context
 * \param[in] source filesystem source
 * \param[in] target mount point
 * \param[in] fstype filesystem type
 * \param[in] flags mount options
 * \param[in] data filesystem specific data
 *
 * \return zero on success, or negative value on error.
 *
 * \par Known issues/bugs:
 * None
 *
 * \pre vsm context must be initialized by vsm_create_context()
 *
 * \post None
 *
 * \see vsm_decalre_file(), vsm_declare_link()
 *
 * \remarks None
 *
 * \par Sample code:
 * \code
 * #include <vasum.h>
 * ...
 * int ret;
 * struct vsm_zone *zone;
 * ...
 * ret = vsm_declare_file(zone, "/dev/block0", "/mnt/target", "ext4", 0, NULL);
 * if(ret < 0)
 * {
 *      printf("Failed to declare file system object\n");
 * }
 *
 * ...
 * \endcode
 *
*/

API int vsm_declare_mount(struct vsm_context *ctx, const char *source, const char *target, const char *fstype, unsigned long flags, const void *data);

/**
 * @brief Declare hardlink to vsm context.
 *
 * \par Synopsis:
 * \code
 * #include <vasum.h>
 *
 * int vsm_declare_link(struct vsm_context *ctx, const char *source, const char *target)
 * \endcode
 *
 * \par Description:
 *  Declare hardlink to the vsm context.\n
 * In general, this function is used to create shared socket.
 *
 * \param[in] ctx vsm context
 * \param[in] source source
 * \param[in] target target
 *
 * \return zero on success, or negative value on error.
 *
 * \par Known issues/bugs:
 * None
 *
 * \pre vsm context must be initialized by vsm_create_context()
 *
 * \post None
 *
 * \see vsm_declare_file(), vsm_declare_mount()
 *
 * \remarks None
 *
 * \par Sample code:
 * \code
 * #include <vasum.h>
 * ...
 * int ret;
 * struct vsm_zone *zone;
 * ...
 * ret = vsm_declare_link(zone, "/tmp/sock", "/tmp/sock");
 * if(ret < 0)
 * {
 *      printf("Failed to declare file system object\n");
 * }
 *
 * ...
 * \endcode
 *
*/

API int vsm_declare_link(struct vsm_context *ctx, const char *source, const char *target);

/// @}

#ifdef __cplusplus
}
#endif

#endif /*! __VASUM_H__ */
