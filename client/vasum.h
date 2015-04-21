/*
 * Vasum : Tizen Zone Control Framework
 *
 * Copyright (c) 2000 - 2014 Samsung Electronics Co., Ltd All Rights Reserved
 * 
 * Contact: Keunhwan Kwak <kh243.kwak@samsung.com>
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

#include "vasum_list.h"

#ifndef API
#define API __attribute__((visibility("default")))
#endif // API

#ifdef __cplusplus
extern "C" {
#endif
/*
 * @file        vasum.h
 * @version     0.3
 * @brief       This file contains APIs of the Zone control Framework
 */

/*
 * <tt>
 * Revision History:
 *   2014-09-01	sungbae you    First created
 *   2014-10-07	sungbae you    First doxygen commented
 *   2015-03-19	kuenhwan Kwak  doxygen revise
 * </tt>
 */

/**
 * @addtogroup CONTEXT vasum context
 * @{
*/

/**
 *@brief vasum handle for interact with vasum server process. This is opaque data type.
 */
typedef struct vsm_context* vsm_context_h;

/**
 * @brief Create vsm context
 * \par Description:
 * The vsm context is an abstraction of the logical connection between the zone controller and it's clients.
 * and vsm_context_h object should be finalized when interaction with the vasum server is no longer required.\n
 * \return An instance of vsm context on success, or NULL
 * \retval vsm_context_h     successful
 * \retval NULL               get vasum context failed.
 * \par Known issues/bugs:
 *  Only a host process has permission for vsm_create_context();
 * \pre vsm-zone-svc must be started
 * \see vsm_cleanup_context()
*/
API vsm_context_h vsm_create_context(void);

/**
 * @brief Cleanup zone control context
 * \par Description:
 * vsm_cleanup_context() finalizes vsm context and release all resources allocated to the vsm context.\n
 * This function should be called if interaction with the zone controller is no longer required.
 * \param[in] ctx vsm context
 * \return #VSM_ERROR_NONE on success.
 * \pre vsm_context_h must be initialized by vsm_create_context()
 * \post vsm context_h will not be valid after calling this API
 * \see vsm_create_context()
*/
API int vsm_cleanup_context(vsm_context_h ctx);

/**
 * @brief Get file descriptor associated with event dispatcher of zone control context
 * \par Description:
 * The function vsm_get_poll_fd() returns the file descriptor associated with the event dispatcher of vsm context.\n
 * The file descriptor can be bound to another I/O multiplexing facilities like epoll, select, and poll.
 * \param[in] ctx vsm context
 * \return On success, a nonnegative file descriptor is returned. On negative error code is returned.
 * \retval fd nonnegative integer fd value for getting vasum event and refresh vsm_context_h.
 * \retval #VSM_ERROR_INVALID invalid vsm_context_h
 * \pre vsm_context_h must be initialized by vsm_create_context()
 * \see vsm_create_context()
*/
API int vsm_get_poll_fd(vsm_context_h ctx);

/**
 * @brief Wait for an I/O event on a VSM context
 * \par Description:
 * vsm_enter_eventloop() waits for event on the vsm context.\n
 *\n
 * The call waits for a maximum time of timout milliseconds. Specifying a timeout of -1 makes vsm_enter_eventloop() wait indefinitely,
 * while specifying a timeout equal to zero makes vsm_enter_eventloop() to return immediately even if no events are available.
 * \param[in] ctx vsm context
 * \param[in] flags Reserved
 * \param[in] timeout Timeout time (milisecond), -1 is infinite
 * \return 0 on success,  or negative error code.
 * \pre vsm context must be initialized by vsm_create_context()
 * \see vsm_create_context(), vsm_get_poll_fd()
*/
API int vsm_enter_eventloop(vsm_context_h ctx, int flags, int timeout);

/**
 * @brief Enumeration for vasum error.
 */
typedef enum {
	VSM_ERROR_NONE,			/**< The operation was successful */
	VSM_ERROR_GENERIC,		/**< Non-specific cause */
	VSM_ERROR_INVALID,		/**< Invalid argument */
	VSM_ERROR_CANCELED,		/**< The requested operation was cancelled */
	VSM_ERROR_ABORTED,		/**< Operation aborted */
	VSM_ERROR_REFUSED,		/**< Connection refused */
	VSM_ERROR_EXIST,		/**< Target exists */
	VSM_ERROR_BUSY,			/**< Resource is busy */
	VSM_ERROR_IO,			/**< I/O error*/
	VSM_ERROR_TIMEOUT,		/**< Timer expired */
	VSM_ERROR_OVERFLOW,		/**< Value too large to be stored in data type */
	VSM_ERROR_OUT_OF_MEMORY,	/**< No memory space */
	VSM_ERROR_OUT_OF_RANGE,		/**< Input is out of range */
	VSM_ERROR_NOT_PERMITTED,		/**< Operation not permitted */
	VSM_ERROR_NOT_IMPLEMENTED,	/**< Function is not implemented yet */
	VSM_ERROR_NOT_SUPPORTED,	/**< Operation is not supported */
	VSM_ERROR_ACCESS_DENIED,	/**< Access privilege is not sufficient */
	VSM_ERROR_NO_OBJECT,		/**< Object not found */
	VSM_ERROR_BAD_STATE,		/**< Bad state */
	VSM_MAX_ERROR = VSM_ERROR_BAD_STATE
}vsm_error_e;

/**
 * @brief Get last vasum error code from vsm_context.
 * \param[in] ctx vsm context
 * \return vasum error enum value.
 * \par Known issues/bugs:
 *     thread non-safe
 * \see vsm_error_string()
*/
API vsm_error_e vsm_last_error(vsm_context_h ctx);

/**
 * @brief Get vasum error string.
 * \par Description:
 * return string pointer for vasum error string.
 * \param[in] vsm_error_e error.
 * \return string pointer value represent to error code.
 * \warning Do not free returned pointer.
*/
API const char *vsm_error_string(vsm_error_e error);

/// @}

/**
 * @addtogroup LIFECYCLE Vasum Lifecycle
 * @{
 */

/**
 * @brief vsm_zone_h opaque datatype which is used to represent an instance of zone.
 */

typedef struct vsm_zone* vsm_zone_h;

/**
 * @brief Definition for default zone name.
 * Default zone is started at boot sequence by systemd.
*/
#define VSM_DEFAULT_ZONE "personal"


/**
 * @brief Create a new persistent zone
 * \par Description:
 * vsm_create_zone() triggers zone controller to create new persistent zone.\n\n
 * The zone controller distinguishes two types of zones: peristent and transient.
 * Persistent zones exist indefinitely in storage.
 * While, transient zones are created and started on-the-fly.\n\n
 * Creating zone requires template.\n
 * In general, creating a zone involves constructing root file filesystem and
 * generating configuration files which is used to feature the zone.
 * Moreover, it often requires downloading tools and applications pages,
 * which are not available in device.
 * In that reason, Vasum Framework delegates this work to templates.
 * \param[in] ctx vsm context
 * \param[in] zone_name zone name
 * \param[in] template_name template name to be used for constructing the zone
 * \param[in] flag Reserved
 * \return 0 on success, or negative integer error code on error.
 * \retval #VSM_ERROR_NONE     Successful
 * \retval #VSM_ERROR_INVALID  Invalid arguments
 * \retval #VSM_ERROR_EXIST    The same name zone is existed.
 * \retval #VSM_ERROR_GENERIC  Lxc failed to create zone.
 * \retval #VSM_ERROR_IO       Database access failed
*/
API int vsm_create_zone(vsm_context_h ctx, const char *zone_name, const char *template_name, int flag);

/**
 * @brief Destroy persistent zone
 * \par Description:
 * The function vsm_destroy_zone() triggers zone controller to destroy persistent zone corresponding to the given name.\n
 * All data stored in the repository of the zone are also deleted if force argument is set.
 * Once the zone repository is deleted, it cannot be recovered.
 * \param[in] ctx vsm context
 * \param[in] zone_name zone name
 * \param[in] force forced flags
 * - 0 : do not destory running zone.
 * - non-zero : if zone is running, shutdown forcefully, and destroy.
 * \return 0 on success, or negative integer error code on error.i
 * \retval #VSM_ERROR_NONE       Successful
 * \retval #VSM_ERROR_BUSY       Target zone is running
 * \retval #VSM_ERROR_NO_OBJECT  Target zone does not exist
 * \retval #VSM_ERROR_GENERIC    Lxc failed to destroy zone
 * \retval #VSM_ERROR_IO         Database access failed
*/
API int vsm_destroy_zone(vsm_context_h ctx, const char *zone_name, int force);

/**
 * @brief Start an execution of a persistent zone
 * \par Description:
 * The function vsm_start_zone() triggers zone controller to start zone corresponding to the given name.\n
 * Caller can speficy the type of zones: transient and peristent.
 * Persistent zones need to be defined before being start up(see vsm_create_zone())
 * While, transient zones are created and started on-the-fly.
 * \param[in] ctx vsm context
 * \param[in] zone_name zone name
 * \return 0 on success, or negative integer error code on error.
 * \retval #VSM_ERROR_NONE       Successful
 * \retval #VSM_ERROR_INVALID    Invalid argument.
 * \retval #VSM_ERROR_BUSY       Target zone is already running state.
 * \retval #VSM_ERROR_NO_OBJECT  Target zone does not exist
 * \retval #VSM_ERROR_GENERIC    Lxc failed to start zone
*/
API int vsm_start_zone(vsm_context_h ctx, const char *zone_name);

/**
 * @brief Stop an execution of running zone
 * \par Description:
 * Send request to stop running zone.\n
 * \param[in] ctx vsm context
 * \param[in] zone_name zone name
 * \param[in] force option to shutdown.
 *	- 0 : send SIGPWR signal to init process of target zone.
 *  - non-zero : terminate all processes in target zone.
 * \return 0 on success, or negative integer error code on error.
 * \retval #VSM_ERROR_NONE       Successful
 * \retval #VSM_ERROR_INVALID    Invalid argument.
 * \retval #VSM_ERROR_BAD_STATE  Failed to lookup running zone.
 * \retval #VSM_ERROR_NO_OBJECT  Target zone does not exist
 * \retval #VSM_ERROR_GENERIC    Lxc failed to destroy zone
*/
API int vsm_shutdown_zone(vsm_context_h ctx, const char *zone_name, int force);

/**
 * @brief Shutdown zone and prevent starting zone
 * \par Description:
 *  Prevent starting target zone.
 *  If target zone is running, shutdown the zone first.
 * \param[in] ctx vsm context
 * \param[in] zone_name zone name
 * \return 0 on success, or negative integer error code on error.
 * \retval #VSM_ERROR_NONE       Successful
 * \retval #VSM_ERROR_NO_OBJECT  Target zone does not exist
 * \retval #VSM_ERROR_GENERIC    Locking failed.
*/
API int vsm_lock_zone(vsm_context_h ctx, const char *zone_name, int shutdown);

/**
 * @brief Allow a zone to be launched
 * \par Description:
 * Remove lock applied to the zone corresponding to the given name.
 * \param[in] ctx vsm context
 * \param[in] zone_name zone name
 * \return 0 on success, or negative integer error code on error.
 * \retval #VSM_ERROR_NONE       Successful
 * \retval #VSM_ERROR_GENERIC    Unlocking failed.
*/
API int vsm_unlock_zone(vsm_context_h ctx, const char *zone_name);

/**
 * @brief Switch target zone to foreground
 * \par Description:
 * Switch target zone to foreground on device.
 * \param[in] zone vsm_zone_h
 * \return 0 on success, or negative integer error code on error.
 * \retval #VSM_ERROR_NONE       Successful
 * \retval #VSM_ERROR_INVALID    vsm_zone_h is invalid
 */
API int vsm_set_foreground(vsm_zone_h zone);

/**
 * @brief Get current foreground zone on device
 * \par Description:
 * Get foreground zone handle.
 * \param[in] ctx vsm_context_h
 * \return 0 on success, or negative integer error code on error.
 * \retval #VSM_ERROR_NONE       Successful
 * \retval #VSM_ERROR_INVALID    vsm_context_h is invalid
 */
API vsm_zone_h vsm_get_foreground(vsm_context_h ctx);

/// @}
/**
 * @addtogroup ACCESS Vasum Access
 * @{
*/

/**
 * @brief Definition for zone states.
 * This definition shows the available states.
*/
typedef enum {
	VSM_ZONE_STATE_STOPPED,  /**< Zone stopped */
	VSM_ZONE_STATE_STARTING, /**< Zone is prepare for running */
	VSM_ZONE_STATE_RUNNING,  /**< Zone is running on device */
	VSM_ZONE_STATE_STOPPING, /**< Zone is stopping by request */
	VSM_ZONE_STATE_ABORTING, /**< Zone is failed to start */
	VSM_ZONE_STATE_FREEZING, /**< Reserved State */
	VSM_ZONE_STATE_FROZEN,   /**< Reserved State */
	VSM_ZONE_STATE_THAWED,   /**< Reserved State */
	VSM_ZONE_MAX_STATE = VSM_ZONE_STATE_THAWED
} vsm_zone_state_t;


/**
 * @brief Definition for zone events
*/
typedef enum {
	VSM_ZONE_EVENT_NONE,      /**< Zone has no event */ 
	VSM_ZONE_EVENT_CREATED,   /**< Zone is created */
	VSM_ZONE_EVENT_DESTROYED, /**< Zone is destroted */
	VSM_ZONE_EVENT_SWITCHED,  /**< Foreground is switched */
	VSM_ZONE_MAX_EVENT = VSM_ZONE_EVENT_SWITCHED
} vsm_zone_event_t;


/**
 * @brief Definition for zone iteration callback function.
 */
typedef void (*vsm_zone_iter_cb)(vsm_zone_h zone, void *user_data);
/**
 * @brief Definition for zone state changed callback function.
 */
typedef int (*vsm_zone_state_changed_cb)(vsm_zone_h zone, vsm_zone_state_t state,  void *user_data);
/**
 * @brief Definition for zone event callback function.
 */
typedef int (*vsm_zone_event_cb)(vsm_zone_h zone, vsm_zone_event_t event, void *user_data);

/**
 * @brief Interate all zone instances which are in running state
 * \par Description:
 * This API traverses all zones which are in running state, and callback function will be called on every entry.
 * \param[in] ctx vsm context
 * \param[in] zone_name zone name string
 * \param[in] callback Function to be executed in iteration, which can be NULL
 * \param[in] user_data Parameter to be passed to callback function
 * \return 0 on success, or negative integer error code on error.
 * \retval #VSM_ERROR_NONE           Successful
 * \retval #VSM_ERROR_OUT_OF_MEMORY  Zone handle allocation failed
 * \retval #VSM_ERROR_GENERIC        Zone initialize failed
 * \remark In case of callback and is NULL, 
 *  This API refresh vsm_context which means reloading current running zone to vsm_context again.
*/
API int vsm_iterate_zone(vsm_context_h ctx, vsm_zone_iter_cb callback, void *user_data);

/**
 * @brief Find zone corresponding to the name
 * The function vsm_lookup_zone_by_name() looks for the zone instance corresponding to the given name.
 * \param[in] ctx vsm context
 * \param[in] path zone path to search
 * \return Zone instance on success, or NULL on error.
 * \retval vsm_zone_h  Successful 
 * \retval NULL        Failed to lookup
 * \pre  vsm_context_h have to bind by vsm_enter_eventloop() or vsm_context_h does not have current zone status. 
 * \see vsm_create_context(), vsm_enter_eventloop()
*/
API vsm_zone_h vsm_lookup_zone_by_name(vsm_context_h ctx, const char *name);

/**
 * @brief Find zone instance associated with the process id
 * \par Description:
 * The function vsm_lookup_zone_by_pid() looks for the zone instance associated with the given pid.
 * \param[in] ctx vsm context
 * \param[in] pid Process id
 * \return Zone instance on success, or NULL on error.
 * \retval vsm_zone_h  Successful 
 * \retval NULL        Failed to lookup
 * \pre  vsm_context_h have to bind by vsm_enter_eventloop() or vsm_context_h does not have current zone status. 
 * \see vsm_create_context(), vsm_enter_eventloop()
*/
API vsm_zone_h vsm_lookup_zone_by_pid(vsm_context_h ctx, pid_t pid);

/**
 * @brief Register zone status changed callback
 * \par Description:
 * Register a callback function for zone status change.
 * \param[in] ctx vsm context
 * \param[in] callback Callback function to invoke when zone satte event occurs
 * \return Callback handle on success, or negative error code on error.
 * \retval handle nonnegative handle id for callback.
 * \retval #VSM_ERROR_OUT_OF_MEMORY  Callback hanlder allocation failed.
 * \pre  vsm_context_h have to bind by vsm_enter_eventloop() or callback function does not called. 
 * \see vsm_create_context(), vsm_enter_eventloop(), vsm_del_state_changed_callback()
*/
API int vsm_add_state_changed_callback(vsm_context_h ctx, vsm_zone_state_changed_cb callback, void *user_data);

/**
 * @brief Deregister zone status changed callback handler
 * \par Description:
 * Remove an event callback from the zone control context.
 * \param[in] ctx vsm context
 * \param[in] handle Callback Id to remove
 * \return 0 on success, or negative integer error code on error.
 * \retval #VSM_ERROR_NONE Successful
 * \retval #VSM_ERROR_NO_OBJECT Failed to lookup callback handler
*/
API int vsm_del_state_changed_callback(vsm_context_h ctx, int handle);

/**
 * @brief Register zone event callback
 * \par Description:
 * Register a callback function for zone event.
 * \param[in] ctx vsm context
 * \param[in] callback callback function to invoke when zone event occurs
 * \return positive callback handle on success, or negative error code on error.
 * \retval handle nonnegative handle id for callback.
 * \retval #VSM_ERROR_OUT_OF_MEMORY  Callback hanlder allocation failed.
 * \pre  vsm_context_h have to bind by vsm_enter_eventloop() or callback function does not called.
 * \see vsm_create_context(), vsm_enter_eventloop(), vsm_del_state_changed_callback()
*/
API int vsm_add_event_callback(vsm_context_h ctx, vsm_zone_event_cb callback, void *user_data);

/**
 * @brief Deregister zone event callback handler
 * \par Description:
 * Remove an event callback from the zone control context.
 * \param[in] ctx vsm context
 * \param[in] handle callback handle to remove
 * \return 0 on success, or negative integer error code on error.
 * \retval #VSM_ERROR_NONE Successful
 * \retval #VSM_ERROR_NO_OBJECT Failed to lookup callback handler
*/
API int vsm_del_event_callback(vsm_context_h ctx, int handle);

/**
 * @brief Zone attach parameters
 * Arguments are same as linux system-call execv()
 */
typedef struct vsm_attach_command_s {
	char * exec;      /**< Program binary path */
	char ** argv;     /**< An array of argument pointers to null-terminated strings include program path */
} vsm_attach_command_s;

/**
 * @brief Zone attach option
 */
typedef struct vsm_attach_options_s {
	uid_t uid;        /**< requested uid*/
	gid_t gid;        /**< requested gid*/
	int env_num;      /**< requested environ count */
	char **extra_env; /**< requested environ string pointer array. */
} vsm_attach_options_s;

/**
 * @brief default attach options
 * \n
 * uid = root\n
 * gid = root\n
 * env = no extra env\n
 */

#define VSM_ATTACH_OPT_DEFAULT {  (uid_t)0, (gid_t)0,  0, NULL }

/**
 * @brief Launch a process in a running zone.
 * \par Description:
 * Execute specific command inside the zone with given arguments and environment
 * \param[in] zone vsm_zone_h
 * \param[in] command vsm attach command
 * \param[in] opt vsm attach options (can be NULL), using VSM_ATTACH_OPT_DEFAULT
 * \param[out] attached process pid
 * \return On sucess 0, otherwise, a negative integer error code on error
 * \retval #VSM_ERROR_NONE           Successful
 * \retval #VSM_ERROR_INVALID        Invalid arguments
 * \retval #VSM_ERROR_NO_OBJECT      Target zone is not running state
 * \retval #VSM_ERROR_GENERIC        IPC failed
 */
API int vsm_attach_zone(vsm_context_h ctx, const char * zone_name, vsm_attach_command_s * command, vsm_attach_options_s * opt, pid_t *attached_process);

/**
 * @brief Launch a process in a running zone and wait till child process exited.
 * \par Description:
 * Execute specific command inside the zone with given arguments and environment
 * \param[in] zone vsm_zone_h
 * \param[in] command vsm attach command
 * \param[in] opt vsm attach options (can be NULL), using VSM_ATTACH_OPT_DEFAULT
 * \return On sucess waitpid exit code or attached process, or a negative error code
 * \retval #VSM_ERROR_NONE           Successful
 * \retval #VSM_ERROR_INVALID        Invalid arguments
 * \retval #VSM_ERROR_NO_OBJECT      Target zone is not running state
 * \retval #VSM_ERROR_GENERIC        IPC failed or waitpid error
 */
API int vsm_attach_zone_wait(vsm_context_h ctx, const char * zone_name, vsm_attach_command_s * command, vsm_attach_options_s * opt);

/**
 * @brief Get name string of zone.
 * \par Description:
 * Get name string of zone.
 * \param[in] zone vsm zone handle
 * \return a pointer of zone name string on sucess or NULL on fail.
 * \retval name string value of zone.
 * \retval NULL vsm_zone_h is invalid.
 * \warning Do not modify or free returned pointer memroy.
 *          This memory will be cleanup by vsm_cleanup context()
*/
API const char * vsm_get_zone_name(vsm_zone_h zone);

/**
 * @brief Get zone root ablsolute path on host side.
 * \par Description:
 * Get rootpath string of zone.
 * \param[in] zone vsm zone handle
 * \return a pointer of zone rootpath string on sucess or NULL on fail.
 * \retval rootpath string value of zone.
 * \retval NULL vsm_zone_h is invalid.
 * \warning Do not modify or free returned memroy.
 *          This memory will be cleanup by vsm_cleanup context()
*/
API const char * vsm_get_zone_rootpath(vsm_zone_h zone);

/**
 * @brief Get type of running zone.
 * \par Description:
 * Get type string of zone. This value is defined in template file when using vsm_create_zone()
 * \param[in] zone vsm zone handle
 * \return a pointer of zone path string on sucess or NULL on fail.
 * \retval rootpath string value of zone.
 * \retval NULL vsm_zone_h is invalid.
 * \see vsm_create_zone()
 * \warning Do not modify or free returned memroy.
 *          This memory will be cleanup by vsm_cleanup context()
*/
API const char * vsm_get_zone_type(vsm_zone_h zone);

/**
 * @brief Check zone handle, a host or a normal zone by zone handle.
 * \param[in] zone vsm zone handle
 * \return positive integer on host case, or 0 on normal zone.
 * \retval positive target zone is host zone.
 * \retval NULL     target zone is NOT host.
*/
API int vsm_is_host_zone(vsm_zone_h zone);

/**
 * @brief get zone state.
 * \par Description:
 * Get zone state value by zone handle.
 * \param[in] zone vsm zone handle
 * \return vsm_zone_state_t value, or negative value.
 * \retval state vsm_zone_state_t value of zone
 * \retval #VSM_ERROR_INVALID vsm_zone_h is invalid
*/

API vsm_zone_state_t vsm_get_zone_state(vsm_zone_h zone);

/**
 * @brief get zone id by handle.
 * \par Description:
 * Get zone id value by zone handle.
 * \param[in] zone vsm zone handle
 * \return interger id value of zone or negative error.
 * \retval id nonnegative interger value.
 * \retval #VSM_ERROR_INVALID vsm_zone_h is invalid
 * \remarks id '0' is reserved for host.
*/
API int vsm_get_zone_id(vsm_zone_h zone);


/**
 * @brief Set userdata pointer value in vsm_zone_h.
 * \par Description:
 * Get zone id value by zone handle.
 * \param[in] zone vsm zone handle
 * \return On success 0, or negative error.
 * \retval #VSM_ERROR_NONE     Successful.
 * \retval #VSM_ERROR_INVALID  vsm_zone_h is invalid
 * \warning This userdata have to be free before vsm_cleanup_context() or VSM_ZONE_STATE_STOPPED callback handler
*/
API int vsm_set_userdata(vsm_zone_h zone, void * userdata);

/**
 * @brief Set userdata pointer value in vsm_zone_h.
 * \par Description:
 * Get zone id value by zone handle.
 * \param[in] zone vsm zone handle
 * \return On success pointer of userdata, or NULL pointer
 * \retval userdata pointer of userdata.
 * \retval NULL invalid vsm_zone_h.
 * \remark initial value is pointer of self vsm_zone_h
*/
API void * vsm_get_userdata(vsm_zone_h zone);

/**
 * @brief join current process into zone.
 * \par Synopsys:
 * Change self peer credential to target zone
 * \param[in] vsm_zone_h zone
 * \return before vsm_zone on success, or NULL on error.
 * \retval vsm_zone_h before zone handle, If caller process running in host, then host handle returned.
 * \retval NULL       invalid zone handle.
 * \pre parameter vsm zone must be lookup by vsm lookup APIs
 * \post  After finish target zone procedure, must join again before zone by same API.
 * \warning This function is not thread-safe. \n
 *   All requests of threads in same process are considered target zone request to other host processes.
 */
API vsm_zone_h vsm_join_zone(vsm_zone_h zone);

/**
 * @brief get canonical file path based on current zone.
 * \par Description:
 *    get canonical file path based on current zone.
 * \param[in] input_path requested zone path
 * \param[out] output_path string pointer for canonicalized output path 
 * \return int positive string length of output_path, or negative error code on error.
 * \retval #VSM_ERROR_INVALID Invalid arguments.
 * \retval #VSM_ERROR_GENERIC gethostname() is failed.
 * \post Out buffer(*output_path) have to be freed by caller after use.
 * \remarks This API can call inside zone without vsm_context_h
 *   It means this API can call library side for apps.
 */
API int vsm_canonicalize_path(const char *input_path, char **output_path);

/**
 * @brief Check current environment, a host or virtualized zone.
 * \par Description:
 *    Check current caller process environment.
 * \return positive integer on running in zone, or 0 on running in host.
 * \retval NULL     Current process running in host.
 * \remarks This API can call inside zone without vsm_context_h
 *   It means this API can call library side for apps.
 */
API int vsm_is_virtualized(void);

/**
 * @brief Check equivalence between caller and target pid.
 * \par Description:
 *    Check API caller process and target pid running in same zone.
 * \retval NULL       target process is running in another zone.
 * \retval 1        target process is running in same zone.
 * \retval -1       failed to check target process.
 * \remarks This API can check real zone of target pid.
            Real zone is not changed by even join API.
 */
API int vsm_is_equivalent_zone(vsm_context_h ctx, pid_t pid);

/**
 * @brief Translate zone pid to host pid.
 * \par Description:
 *    This API would translate zone namespace pid to host namespace pid.
 * \param[in] zone  the zone of target process
 * \param[in] pid   target process id of zone namespace
 * \return positive pid or negative error code.
 * \retval pid                       translated host pid of zone process.
 * \retval #VSM_ERROR_NO_OBJECT      No such process in a target zone.
 * \retval #VSM_ERROR_NOT_PERMITTED  Permission denied to get target pid info.
 * \retval #VSM_ERROR_NOT_SUPPORTED  Kernel config is not enabled about pid namespace.
 * \retval #VSM_ERROR_INVALID        Arguments is invalid.
 * \retval #VSM_ERROR_IO             Failed to get process info.
 * \retval #VSM_ERROR_GENERIC        Failed to matching zone pid to host pid.
 */
API int vsm_get_host_pid(vsm_zone_h zone, pid_t pid);


/// @}

/**
 * @addtogroup NETWORK Vasum Network
 * @{
*/

/**
 * @brief Types of virtual network interfaces
 */
typedef enum {
	VSM_NETDEV_VETH, /**< Virtual Ethernet(veth), this type device will be attached to host-side network bridge */
	VSM_NETDEV_PHYS, /**< Physical device */
	VSM_NETDEV_MACVLAN /**< Mac VLAN, this type isn't implemented yet */
} vsm_netdev_type_t;

typedef enum {
	VSM_NETDEV_ADDR_IPV4, /**< IPV4 Address family */
	VSM_NETDEV_ADDR_IPV6 /**< IPV6 Address family */
} vsm_netdev_addr_t;

/**
 * @brief Network device information
 */
typedef struct vsm_netdev *vsm_netdev_h;

/**
 * @brief Definition for zone network devince iteration callback function.
 */
typedef void (*vsm_zone_netdev_iter)(struct vsm_netdev *, void *user_data);

/**
 * @brief Create a new network device and assisgn it to zone
 * \par Description:
 * This function creates net netdev instance and assign it to the given zone.
 * \param[in] zone Zone
 * \param[in] type Type of network device to be create
 * \param[in] target
 * - If type is veth, this will be bridge name to attach.
 * - If type is phys, this will be ignored.
 * \param[in] netdev Name of network device to be create
 * \return Network devce on success, or NULL on error.
 * \retval vsm_netdev_h New net device handle
 * \retval NULL         Failed to create netdev
 * \see vsm_create_netdev()
 * \par Known Issues:
 *  Macvlan is not implemented yet.
 */
API vsm_netdev_h vsm_create_netdev(vsm_zone_h zone, vsm_netdev_type_t type, const char *target, const char *netdev);

/**
 * @brief Removes a network device from zone
 * \par Description:
 * This function remove the given netdev instance from the zone control context.
 * \param[in] netdev network device to be removed
 * \return 0 on success, or negative integer error code on error.
 * \retval #VSM_ERROR_NONE           Successful
 * \retval #VSM_ERROR_INVALID        Invalid arguments
 * \retval #VSM_ERROR_GENERIC        Failed to interact with vasum server
 * \see vsm_destroy_netdev()
 */
API int vsm_destroy_netdev(vsm_netdev_h netdev);

/**
 * @brief Iterates network devices found in the zone vsm context and executes callback() on each interface.
 * \par Description:
 * vsm_destroy_netdev() scans all network interfaces which are registered in the vsm context, and calls callback() on each entry.
 * \param[in] zone Zone
 * \param[in] callback Function to be executed in iteration, which can be NULL
 * \param[in] user_data Parameter to be delivered to callback function
 * \return 0 on success, or negative integer error code on error.
 * \retval #VSM_ERROR_NONE           Successful
 * \retval #VSM_ERROR_INVALID        Invalid arguments
 * \retval #VSM_ERROR_GENERIC        Failed to interact with vasum server
 * \see vsm_create_netdev(), vsm_destroy_netdev()
 */
API int vsm_iterate_netdev(vsm_zone_h zone, vsm_zone_netdev_iter callback, void *user_data);

/**
 * @brief Find a network device corresponding to the name
 * \par Description:
 * The function vsm_lookup_netdev_by_name() looks for a network interface.
 * \param[in] name Network device name to search
 * \return Network device on success, or NULL on error.
 * \retval vsm_netdev_h Target net device handle
 * \retval NULL         Failed to lookup netdev
 */

API vsm_netdev_h vsm_lookup_netdev_by_name(vsm_zone_h zone, const char *name);

/**
 * @brief Turn up a network device in the zone
 * \par Description:
 * This function turns up the given netdev instance in the zone.
 * \param[in] netdev network device to be removed
 * \return 0 on success, or negative integer error code on error.
 * \retval #VSM_ERROR_NONE           Successful
 * \retval #VSM_ERROR_INVALID        Invalid arguments
 * \retval #VSM_ERROR_GENERIC        Failed to interact with vasum server
 * \see vsm_down_netdev()
 */
API int vsm_up_netdev(vsm_netdev_h netdev);

/**
 * @brief Turn down a network device in the zone
 * \par Description:
 * This function turns down the given netdev instance in the zone.
 * \param[in] netdev network device to be removed
 * \return 0 on success, or negative integer error code on error.
 * \retval #VSM_ERROR_NONE           Successful
 * \retval #VSM_ERROR_INVALID        Invalid arguments
 * \retval #VSM_ERROR_GENERIC        Failed to interact with vasum server
 * \see vsm_down_netdev()
 */
API int vsm_down_netdev(vsm_netdev_h netdev);

/**
 * @brief Get ip address from a network device
 * \par Description:
 * The function vsm_get_ip_addr_netdev() get ip address from a network interface
 * \param[in] netdev Network device to get address
 * \param[in] family Address family
 * \param[out] addr Buffer to get address from a network device
 * \param[out] size Size of buffer
 * \return 0 on success, or negative integer error code on error.
 * \retval #VSM_ERROR_NONE           Successful
 * \retval #VSM_ERROR_INVALID        Invalid arguments
 * \retval #VSM_ERROR_OVERFLOW       Parameter overflow
 * \retval #VSM_ERROR_GENERIC        Failed to interact with vasum server
 * \see vsm_set_ip_addr_netdev()
 */
API int vsm_get_ip_addr_netdev(vsm_netdev_h netdev, vsm_netdev_addr_t addr_family, char *addr, int size);

/**
 * @brief Set ip address to a network device
 * \par Description:
 * The function vsm_set_ip_addr_netdev() set ipv4 address to a network interface
 * \param[in] netdev Network device to set address
 * \param[in] family Address family
 * \param[in] addr IP address string to be set
 * \param[in] prefix prefix ( ex> 192.168.122.1/24, 24 is prefix )
 * \return 0 on success, or negative integer error code on error.
 * \retval #VSM_ERROR_NONE           Successful
 * \retval #VSM_ERROR_INVALID        Invalid arguments
 * \retval #VSM_ERROR_OVERFLOW       Parameter overflow
 * \retval #VSM_ERROR_GENERIC        Failed to interact with vasum server
 * \see vsm_get_ip_addr_netdev()
 */
API int vsm_set_ip_addr_netdev(vsm_netdev_h netdev, vsm_netdev_addr_t addr_family, const char *addr, int prefix);



/// @}

/**
 * @addtogroup DEVICE Vasum Device
 * @{
*/

/**
 * @brief Grant device to zone
 * \par Description:
 * Request permission device file node to target zone.
 * \param[in] zone vsm_zone_h
 * \param[in] name device node path
 * \param[in] flags requested permission O_RDWR, O_WRONLY, O_RDONLY
 * \return 0 on success, or negative integer error code on error.
 * \retval #VSM_ERROR_NONE           Successful
 * \retval #VSM_ERROR_INVALID        Invalid arguments
 * \retval #VSM_ERROR_NOT_PERMITTED  Change cgroup is not permitted
 * \retval #VSM_ERROR_GENERIC        Failed to interact with vasum server
 * \see vsm_revoke_device()
 */
API int vsm_grant_device(vsm_zone_h zone, const char *path, uint32_t flags);

/**
 * @brief Revoke device from the zone
 * \par Description:
 * Revoke device node permissions from target zone.
 * \param[in] zone vsm_zone_h
 * \param[in] name device node path
 * \return 0 on success, or negative integer error code on error.
 * \retval #VSM_ERROR_NONE           Successful
 * \retval #VSM_ERROR_INVALID        Invalid arguments
 * \retval #VSM_ERROR_NOT_PERMITTED  Change cgroup is not permitted
 * \retval #VSM_ERROR_GENERIC        Failed to interact with vasum server
 * \see vsm_grant_device()
 */
API int vsm_revoke_device(vsm_zone_h zone, const char *path);
/// @}


/**
 * @addtogroup RESOURCE Vasum Resource
 * @{
 */

/**
 * @brief Definition for declare file type.
*/
typedef enum {
	VSM_FSO_TYPE_DIR,  /**< Directoy type */
	VSM_FSO_TYPE_REG,  /**< Regular file type */
	VSM_FSO_TYPE_FIFO, /**< Fifo file type */
	VSM_FSO_TYPE_SOCK, /**< Socket file type */
	VSM_FSO_TYPE_DEV,  /**< Device node type */
	VSM_FSO_MAX_TYPE = VSM_FSO_TYPE_DEV
} vsm_fso_type_t;

/**
 * @brief Declare file mode.
*/

typedef mode_t vsm_mode_t;

/**
 * @brief Declare specific file object to every zone.
 * \par Description:
 * Declare host file system to every running zone.
 * In case of host target file exist, create new file in running zone. or create a new file in running zone. 
 * And add hook info in vsm-resource-provier for automatically link host target file to starting zone.
 * Smack labels are also copied as same as host labels.
 * \param[in] ctx vsm context
 * \param[in] type Type of file system object
 * \param[in] path Path for the file system object
 * \param[in] flags Flasg
 * \param[in] mode mode
 * \return zero on success, or negative value on error.
 * \retval #VSM_ERROR_NONE      successful.
 * \retval #VSM_ERROR_INVALID   Invalid file type or path.
 * \retval #VSM_ERROR_GENERIC   Error in vasum server side.
 * \retval #VSM_ERROR_NO_OBJECT Source file is not exist in host filesystem
*/
API int vsm_declare_file(vsm_context_h ctx, vsm_fso_type_t ftype, const char *path, int flags, vsm_mode_t mode);

/**
 * @brief Declare hardlink to every zone.
 * \par Description:
 * Declare hardlink to host file to every running zone.
 * And add hook info in vsm-resource-provier for automatically link host target file to starting zone.
 * In general, this function is used to share file host and all running zones.
 * Smack labels are also copied as same as host labels.
 * \param[in] ctx vsm context
 * \param[in] source source
 * \param[in] target target
 * \return zero on success, or negative value on error.
 * \retval #VSM_ERROR_NONE      successful
 * \retval #VSM_ERROR_INVALID   Invalid provision type to db. 
 * \retval #VSM_ERROR_GENERIC   Error in vasum server side.
 * \retval #VSM_ERROR_NO_OBJECT Source file is not exist in host filesystem
*/
API int vsm_declare_link(vsm_context_h ctx , const char *source, const char *target);

/// @}


#ifdef __cplusplus
}
#endif

#endif /*! __VASUM_H__ */
