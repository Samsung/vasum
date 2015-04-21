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

#include "wrapper-compat.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <regex.h>
#include <limits.h>
#include <sys/mount.h>
#include <sys/xattr.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pthread.h>
#include <dirent.h>
#include <sys/socket.h>
#include <asm/unistd.h>

extern "C" {

// find_container_by_pid
char *find_container_by_pid(pid_t  /*pid*/) {
    return NULL;
}
// get_domain_pid
pid_t get_domain_pid(const char * /*name*/, const char * /*target*/) {
    return -1;
}

// mainloop_add_watch
int mainloop_add_watch(struct mainloop * /*mainloop*/, int  /*fd*/, mainloop_event  /*event*/, mainloop_callback  /*callback*/, void * /*data*/) {
    return 0;
}
// mainloop_cleanup
int mainloop_cleanup(struct mainloop * /*mainloop*/) {
    return 0;
}
// mainloop_create
struct mainloop *mainloop_create(void) {
    struct mainloop *mainloop = (struct mainloop *)malloc(sizeof(struct mainloop));
    mainloop->epfd = -1; // epoll_create(2);
    pthread_mutex_init(&mainloop->ml_mutex, NULL);
    pthread_rwlock_init(&mainloop->lock, NULL);
    adt_init_list(&mainloop->watches);
    return mainloop;
}
// mainloop_dispatch
int mainloop_dispatch(struct mainloop * /*mainloop*/, int  /*timeout*/) {
    return 0;
}
// mainloop_remove_watch
int mainloop_remove_watch(struct mainloop * /*mainloop*/, int  /*fd*/) {
    return 0;
}
// mainloop_run
int mainloop_run(struct mainloop * /*mainloop*/, int  /*timeout*/) {
    return 0;
}
// mxe_alloc_endpoint
struct mxe_endpoint *mxe_alloc_endpoint(struct mxe_engine * /*engine*/, int  /*fd*/, mainloop_callback  /*callback*/, int  /*type*/) {
    return NULL;
}
// mxe_broadcast
int mxe_broadcast(struct mxe_engine * /*engine*/, struct mxe_emple * /*emple*/, ...) {
    return 0;
}
// mxe_create_client
struct mxe_endpoint *mxe_create_client(struct mxe_engine *engine, const char * /*addr*/) {
    struct mxe_endpoint *ep = (struct mxe_endpoint *)malloc(sizeof(struct mxe_endpoint));
    ep->type = 1; //MXE_EPT_SLAVE
    ep->fd = -1; //sock_connect(addr);
    ep->engine = engine;
    pthread_rwlock_init(&ep->queue_lock, NULL);
    pthread_mutex_init(&ep->rd_mutex, NULL);
    pthread_mutex_init(&ep->wr_mutex, NULL);
    adt_init_list(&ep->queue);
    adt_init_list(&ep->list);
    return ep;
}
// mxe_create_server
struct mxe_endpoint *mxe_create_server(struct mxe_engine * /*engine*/, const char * /*addr*/) {
    return NULL;
}
// mxe_emple_factory
struct mxe_emple *mxe_emple_factory(struct mxe_proxy * /*proxy*/) {
    return NULL;
}
// mxe_finalize_engine
int mxe_finalize_engine(struct mxe_engine * /*engine*/) {
    return 0;
}
// mxe_free_endpoint
int mxe_free_endpoint(struct mxe_endpoint * /*ep*/) {
    return 0;
}
// mxe_invoke
int mxe_invoke(struct mxe_endpoint * /*ep*/, struct mxe_emple * /*emple*/, ...) {
    return 0;
}
// mxe_lookup_emple
struct mxe_emple *mxe_lookup_emple(struct mxe_endpoint * /*ep*/, int  /*signature*/) {
    return NULL;
}
// mxe_lookup_proxy
struct mxe_proxy *mxe_lookup_proxy(struct mxe_engine * /*engine*/, int  /*id*/) {
    return NULL;
}
// mxe_pop_integer
int mxe_pop_integer(struct mxe_message * /*msg*/) {
    return 0;
}
// mxe_pop_string
char *mxe_pop_string(struct mxe_message * /*msg*/) {
    return NULL;
}
// mxe_prepare_engine
struct mxe_engine *mxe_prepare_engine(struct mainloop *mainloop, void *data) {
    struct mxe_engine *engine = (struct mxe_engine *)malloc(sizeof(struct mxe_engine));
    engine->data = data;
    engine->mainloop = mainloop;
    pthread_rwlock_init(&engine->endpoint_lock, NULL);
    adt_init_list(&engine->endpoints);

    return engine;
}
// mxe_push_integer
int mxe_push_integer(struct mxe_message * /*msg*/, int  /*value*/) {
    return 0;
}
// mxe_push_string
int mxe_push_string(struct mxe_message * /*msg*/, const char * /*str*/) {
    return 0;
}
// mxe_register_proxy
int mxe_register_proxy(struct mxe_engine * /*engine*/, struct mxe_proxy * /*proxy*/) {
    return 0;
}
// mxe_reply_message (intenal)
int mxe_reply_message(struct mxe_endpoint * /*ep*/, struct mxe_message * /*origin*/, ...) {
    return 0;
}
// mxe_reset_payload
void mxe_reset_payload(struct mxe_message * /*msg*/) {
}
// mxe_wait_for_event
int mxe_wait_for_event(struct mxe_endpoint * /*ep*/, struct mxe_emple * /*emple*/) {
    return 0;
}
// sock_close_socket (intern)
int sock_close_socket(int  /*fd*/) {
    return 0;
}
// sock_connect
int sock_connect(const char * /*path*/) {
    return 0;
}
// sock_create_socket
int sock_create_socket(const char * /*path*/, int  /*type*/, int  /*flags*/) {
    return 0;
}
// sock_monitor_address
int sock_monitor_address(char * /*buffer*/, int  /*len*/, const char * /*lxcpath*/) {
    return 0;
}
// sock_recv_fd (intern)
int sock_recv_fd(int  /*fd*/, int * /*recvfd*/, void * /*data*/, size_t  /*size*/) {
    return 0;
}
// sock_send_fd
int sock_send_fd(int  /*fd*/, int  /*sendfd*/, void * /*data*/, size_t  /*size*/) {
    return 0;
}
// vasum_log
void vasum_log(int  /*type*/, const char *  /*tag*/, const char *  /*fmt*/ , ...) {
}

#define MAX_ERROR_MSG	0x1000
#define BUF_SIZE	4096

#define SMACK_LABEL_LEN 8
#define ERROR(...) do{}while(0)
#define WARN(...) do{}while(0)
#define DEBUG(...) do{}while(0)
#define INFO(...) do{}while(0)

// lib/utils.c
const char *const fso_type_strtab[] = {
	"Directory",
	"Regular file",
	"FIFO",
	"Socket",
	"Device node"
};

const char *fso_type_to_string(vsm_fso_type_t fso)
{
	if (fso < 0 || fso > VSM_FSO_MAX_TYPE) {
		return NULL;
	}

	return fso_type_strtab[fso];
}

int wait_for_pid_status(pid_t pid)
{
	int status, ret;

 again:
	ret = waitpid(pid, &status, 0);
	if (ret == -1) {
		if (errno == EINTR) {
			goto again;
		} else {
			ERROR("waitpid- pid : %d error : %s", pid,
			      strerror(errno));
			return -1;
		}
	}
	if (ret != pid)
		goto again;
	return status;
}

vsm_fso_type_t fso_string_to_type(char *str)
{
	int len;
	int i;
	for (i = 0; i <= VSM_FSO_MAX_TYPE; i++) {
		len = strlen(fso_type_strtab[i]);
		if (strncmp(str, fso_type_strtab[i], len) == 0)
			return static_cast<vsm_fso_type_t>(i);
	}

	return static_cast<vsm_fso_type_t>(-1);
}

int mkdir_p(const char *dir, mode_t mode)
{
	const char *tmp = dir;
	const char *orig = dir;
	char *makeme;

	do {
		dir = tmp + strspn(tmp, "/");
		tmp = dir + strcspn(dir, "/");
		makeme = strndup(orig, dir - orig);
		if (*makeme) {
			if (mkdir(makeme, mode) && errno != EEXIST) {
				free(makeme);
				return -1;
			}
		}
		free(makeme);
	} while (tmp != dir);

	return 0;
}

int lock_fd(int fd, int wait)
{
	int ret;
	struct flock f;

	while (1) {
		f.l_type = F_WRLCK;
		f.l_whence = SEEK_SET;
		f.l_start = 0;
		f.l_len = 0;

		if (wait)
			ret = fcntl(fd, F_SETLKW, &f);
		else
			ret = fcntl(fd, F_SETLK, &f);
		if (ret != -1)
			return 0;
		if (errno == EINTR)
			continue;
		return -1;
	}
}

int unlock_fd(int fd)
{
	struct flock f;
	f.l_type = F_UNLCK;
	f.l_whence = SEEK_SET;
	f.l_start = 0;
	f.l_len = 0;
	return fcntl(fd, F_SETLKW, &f);
}

int copy_smacklabel(const char * /*source*/, const char * /*dest*/)
{
	return 0;
}

int remove_file(char *path)
{
	struct stat path_stat;
	DIR *dp;
	struct dirent *d;
	int status = 0;

	if (lstat(path, &path_stat) < 0) {
		if (errno != ENOENT) {
			ERROR("Unable to stat : %s");
			return -1;
		}
	}

	if (S_ISDIR(path_stat.st_mode)) {
		if ((dp = opendir(path)) == NULL) {
			ERROR("Unable to opendir %s", path);
			return -1;
		}

		while ((d = readdir(dp)) != NULL) {
			char new_path[PATH_MAX];
			if (strcmp(d->d_name, ".") == 0 ||
			    strcmp(d->d_name, "..") == 0)
				continue;

			snprintf(new_path, PATH_MAX, "%s/%s", path, d->d_name);
			if (remove_file(new_path) < 0)
				status = -1;
		}

		if (closedir(dp) < 0) {
			ERROR("Unable to close dp : %s", path);
			return -1;
		}

		if (rmdir(path) < 0) {
			ERROR("Failed to remove dir : %s, cause: %s", path,
			      strerror(errno));
			return -1;
		}

	} else {
		if (unlink(path) < 0) {
			ERROR("Unable to remove %s", path);
			return -1;
		}
	}

	return status;
}

int copy_file(const char *source, const char *dest, int /*flags*/)
{
	int ret;
	FILE *sfp, *dfp;
	size_t nread, nwritten, size = BUF_SIZE;
	char buffer[BUF_SIZE];

	if ((sfp = fopen(source, "r")) == NULL) {
		ERROR("Unable to open source : %s", source);
		return -1;
	}

	if ((dfp = fopen(dest, "w+")) == NULL) {
		ERROR("Unable to open destination : %s", dest);
		fclose(sfp);
		return -1;
	}

	while (1) {
		nread = fread(buffer, 1, size, sfp);

		if (nread != size && ferror(sfp)) {
			ERROR("Read failed");
			return -1;
		} else if (nread == 0) {
			break;
		}

		nwritten = fwrite(buffer, 1, nread, dfp);

		if (nwritten != nread) {
			if (ferror(dfp))
				ERROR("write fail");
			else
				ERROR("Unable to write all data");
			return -1;
		}
	}

	fclose(sfp);
	fclose(dfp);

	ret = copy_smacklabel(source, dest);
	if (ret != 0) {
		ERROR("Unable to setting smack lable");
		return -1;
	}
	return 0;
}

int regex_compile(regex_t * r, const char *regex_text)
{
	int status = regcomp(r, regex_text, REG_EXTENDED | REG_NEWLINE);

	if (status != 0) {
		char error_message[MAX_ERROR_MSG];

		regerror(status, r, error_message, MAX_ERROR_MSG);
		DEBUG("Regex error compiling '%s': %s\n",
		      regex_text, error_message);
		return 1;
	}

	return 0;
}

int regex_match(regex_t * r, const char *to_match)
{
	const char *p = to_match;
	const int n_matches = 10;
	regmatch_t m[n_matches];

	while (1) {
		int i = 0;
		int nomatch = regexec(r, p, n_matches, m, 0);

		if (nomatch) {
			DEBUG("No more matches.\n");
			return nomatch;
		}

		for (i = 0; i < n_matches; i++) {
			int start;
			int finish;
            (void)start;
            (void)finish;

			if (m[i].rm_so == -1) {
				break;
			}

			start = m[i].rm_so + (p - to_match);
			finish = m[i].rm_eo + (p - to_match);
			if (i == 0) {
				INFO("$& is ");
			} else {
				INFO("$%d is ", i);
			}

			INFO("'%.*s' (bytes %d:%d)\n", (finish - start),
			     to_match + start, start, finish);
		}

		p += m[0].rm_eo;
	}

	return 0;
}

int get_peer_pid(int fd)
{
	struct ucred cred;
	socklen_t cr_len = sizeof(cred);
	if (getsockopt(fd, SOL_SOCKET, SO_PEERCRED, &cred, &cr_len) < 0) {
		return -1;
	}
	return cred.pid;
}

pid_t gettid(void)
{
	return syscall(__NR_gettid);
}

int set_smacklabel_fd(int fd, const char *xattr_name, const char *label)
{
    size_t len;
    int ret;

    if(fd < 0)
        return -1;

    len = strnlen(label, SMACK_LABEL_LEN + 1);
    if (len > SMACK_LABEL_LEN)
        return -1;

    ret = fsetxattr(fd, xattr_name, label, len+1, 0);
    if(ret != 0) {
        ERROR("Set Smack lable error : %s", strerror(errno));
    }
    return ret;
}

int set_smacklabel(const char *path, const char *xattr_name, const char *label)
{
    size_t len;
    int ret;

    if(path == NULL)
        return -1;

    len = strnlen(label, SMACK_LABEL_LEN + 1);
    if (len > SMACK_LABEL_LEN)
        return -1;

    ret = lsetxattr(path, xattr_name, label, len+1, 0);
    if(ret != 0) {
        ERROR("Set Smack lable error : %s", strerror(errno));
    }
    return ret;
}
char *get_self_smacklabel(void)
{
	int ret;
	int fd;
	const char *attr_path = "/proc/self/attr/current";
	char buffer[SMACK_LABEL_LEN+1];

	bzero(buffer, SMACK_LABEL_LEN+1);

	fd = open(attr_path, O_RDONLY);
	if( fd < 0) {
		return NULL;
	}

	ret = read(fd, buffer, SMACK_LABEL_LEN+1);
	close(fd);
	if (ret < 0) {
		return NULL;
	}

	if( ret > SMACK_LABEL_LEN) {
		//return NULL;
	}
    buffer[SMACK_LABEL_LEN]=0;

	return strdup(buffer);
}

int get_self_cpuset(char *name, int buf_sz)
{
	int fd;
	int lxc_len, ret;
	char cpuset_path[] = "/proc/self/cpuset";
	char current_name[NAME_MAX];

	fd = open(cpuset_path, O_RDONLY);
	if (fd < 0) {
		return 0;
	}

	ret = read(fd, current_name, NAME_MAX - 1);
	if (ret < 0) {
		close(fd);
		return -1;
	}

	current_name[ret - 1] = '\0';
	close(fd);

	lxc_len = sizeof("/lxc");
	if( ret < lxc_len) {
		name[0] = '/';
		name[1] = 0;
		return 1;
	} else {
		char *p;
		p = current_name + lxc_len;

		while(*p != '\0') {
			if(*p == '/') {
				*p = '\0';
				break;
			}
			p++;
		}
		snprintf(name, buf_sz, "%s", current_name+lxc_len);
	}

	return ret - lxc_len;
}


char * get_pid_cpuset(int pid)
{
	int fd;
	int ret;
	char cpuset_path[PATH_MAX];
	char current_name[NAME_MAX];

	snprintf(cpuset_path, PATH_MAX, "/proc/%d/cpuset", pid);

	ret = access(cpuset_path, F_OK | R_OK);
	if( ret != 0 )
		return NULL;

	fd = open(cpuset_path, O_RDONLY);
	if (fd < 0) {
		return NULL;
	}

	ret = read(fd, current_name, NAME_MAX - 1);
	if (ret < 0) {
		close(fd);
		return NULL;
	}

	current_name[ret - 1] = 0;
	close(fd);

	INFO("cpuset path : %s, value : %s", cpuset_path, current_name);

	return strdup(current_name);
}

char * read_namespace_link(const char *ns, int pid)
{
	char ns_path[PATH_MAX];
	char buf[NAME_MAX];
	int ret;

	snprintf(ns_path, PATH_MAX, "/proc/%d/ns/%s", pid, ns);

	ret = access(ns_path, F_OK);
	if(ret != 0)
		return NULL;

	ret = readlink(ns_path, buf, NAME_MAX);
	if( ret == -1 ) {
		ERROR("Failed to readlink ns file - [%s]", ns_path);
		return NULL;
	}

	buf[ret] = 0;

	INFO("Read ns link data -pid : %d data : %s", pid, buf);

	return strdup(buf);
}

// libs/device.c
#define DEV_ITERATE_CONTINUE    0
int dev_enumerate_nodes(const char *cname, dev_enumerator enumerator,
			void *data)
{
	int ret;
	FILE *fp;;
	char path[PATH_MAX], entry[64];

	ret = snprintf(path, sizeof(path),
		       "/sys/fs/cgroup/devices/lxc/%s/devices.list", cname);

	if (ret < 0) {
		ERROR("Failed to make pathname");
		return -1;
	}

	fp = fopen(path, "r");
	if (fp == NULL) {
		ERROR("File open failed: %s(%s)", path, strerror(errno));
		return -1;
	}

	while (fgets(entry, sizeof(entry), fp) != NULL) {
		int major, minor;
		char *next, *ptr = &entry[2];

		major = strtol(ptr, &next, 10);
		minor = strtol(++next, (char **)NULL, 10);

		ret = enumerator(entry[0], major, minor, data);
		if (ret != DEV_ITERATE_CONTINUE)
			break;
	}

	fclose(fp);

	return ret;
}

int dev_terminal_enumerator(int type, int major, int minor, void *data)
{
	int *dev = (int*)data;

	*dev = minor;
    (void)type;
    (void)major;
    
	INFO("Matched device: %c, %d, %d\n", type, major, minor);

	return 1;
}

// libs/namespace.c
pid_t get_init_pid(const char *name)
{
	char filename[PATH_MAX];
	FILE *fp;
	pid_t ret = -1;

	snprintf(filename, sizeof(filename),
			"/sys/fs/cgroup/devices/lxc/%s/cgroup.procs", name);

	fp = fopen(filename, "r");

	if (fp != NULL) {
		if (fscanf(fp, "%d", &ret) < 0) {
			ERROR("Failed to read %s\n", filename);
			ret = -2;
		}
		fclose(fp);
	} else {
		INFO("Unable to access %s\n", filename);
		ret = errno;
	}

	return ret;
}


pid_t get_zone_pid(const char *name, const char *target)
{
	char path[PATH_MAX];
	char cmd[PATH_MAX];
	int res = 0, len;
	pid_t ret = -1;
	FILE *fp;

	char *line = NULL;
	size_t line_len;

	snprintf(path, PATH_MAX,
		 "/sys/fs/cgroup/cpuset/lxc/%s/cgroup.procs", name);

	res = access(path, F_OK | R_OK);
	if (res != 0) {
		ERROR("Failed to acess zone cgroup file: %s", path);
		return -EINVAL;
	}

	if (target == NULL) {
		ERROR("Failed to lookup cmdline in zone proc");
		return -EINVAL;
	} else {
		len = strlen(target);
	}

	fp = fopen(path, "r");
	if (fp == NULL) {
		ERROR("Failed to open zone cgroup");
		return -1;
	}

	while (getline(&line, &line_len, fp) != -1) {
		int res;
		pid_t pid;
		FILE *cmdfp;
		char cmdpath[PATH_MAX];

		res = sscanf(line, "%d", &pid);
		if (res != 1) {
			ERROR("Failed to read %s\n", path);
			res = -1;
			goto out;
		}

		if (pid < 0)
			continue;

		snprintf(cmdpath, PATH_MAX, "/proc/%d/cmdline", pid);

		if (access(cmdpath, F_OK | R_OK) != 0)
			continue;

		cmdfp = fopen(cmdpath, "r");
		if (cmdfp == NULL) {
			ERROR("Unable to access %s\n", cmdpath);
			continue;
		}

		if (fscanf(cmdfp, "%s", cmd) < 0) {
			ERROR("Failed to read cmdline - pid : %d\n", pid);
			continue;
		}

		fclose(cmdfp);

		if (strncmp(cmd, target, len) == 0) {
			ret = pid;
			break;
		}
	}
 out:
	fclose(fp);
	return ret;
}

int open_ns(pid_t pid, const char *name)
{
	int fd, ret;
	char path[PATH_MAX];

	ret = snprintf(path, PATH_MAX, "/proc/%d/ns/%s", pid, name);
	if (ret < 0 || ret >= PATH_MAX) {
		ERROR("Failed to namespace - pid %d, ns: %s ", pid, name);
		return -EINVAL;
	}

	fd = open(path, O_RDONLY);
	if (fd < 0) {
		ERROR("failed to open %s\n", path);
		return -errno;
	}

	return fd;
}
/*
void enter_to_ns(pid_t pid, char *name)
{
	int newns;
	char *ns[] = { "mnt", "pid", "uts", "ipc", "net" };
	int flags[] = {
		CLONE_NEWNS, CLONE_NEWPID, CLONE_NEWUTS, CLONE_NEWIPC,
		CLONE_NEWNET
	};
	int size = sizeof(ns) / sizeof(char *);
	int i, idx = size;

	for (i = 0; i < size; i++) {
		if (strncmp(ns[i], name, strlen(ns[i])) == 0) {
			idx = i;
			break;
		}
	}

	if (idx == size) {
		ERROR("Invalid namespace - ns: %s", name);
		return;
	}

	newns = open_ns(pid, ns[i]);

	if (setns(newns, flags[i])) {
		ERROR("failed to setns");
		close(newns);
		return;
	}

	INFO("Enter namespace - init: %d , ns: %s", pid, name);

	close(newns);
	return;
}*/

// vasum/libs/vt.c
#include <linux/kd.h>
#include <linux/vt.h>
static int is_console(int fd)
{
	char arg;

	return (isatty(fd) &&
		(ioctl(fd, KDGKBTYPE, &arg) == 0) &&
		((arg == KB_101) || (arg == KB_84)));
}

static int open_console(const char *path)
{
	int fd;

	fd = open(path, O_RDWR);
	if (fd < 0) {
		fd = open(path, O_WRONLY);
	}
	if (fd < 0) {
		fd = open(path, O_RDONLY);
	}
	if (fd < 0) {
		return -1;
	}

	return fd;
}

int get_console_fd(const char *path)
{
	int fd;

	if (path) {
		fd = open_console(path);
		if (fd >= 0) {
			return fd;
		}

		return -1;
	}

	fd = open_console("/dev/tty0");
	if (fd >= 0) {
		return fd;
	}

	fd = open_console("/dev/console");
	if (fd >= 0) {
		return fd;
	}

	for (fd = 0; fd < 3; fd++) {
		if (is_console(fd)) {
			return fd;
		}
	}

	return -1;
}

int vt_switch_terminal(int id)
{
	int fd, ret = -1;

	fd = get_console_fd(NULL);
	if (fd < 0) {
		return -1;
	}

	if (ioctl(fd, VT_ACTIVATE, id) < 0) {
		goto out;
	}

	if (ioctl(fd, VT_WAITACTIVE, id) < 0) {
		goto out;
	}

	ret = 0;
 out:
	close(fd);
	return ret;
}

int vt_find_unused_terminal(void)
{
	int fd, nr = -1;

	fd = get_console_fd(NULL);
	if (fd < 0) {
		perror("Terminal open failed");
		return -1;
	}

	if (ioctl(fd, VT_OPENQRY, &nr) < 0) {
		perror("VT_OPENQRY failed");
		goto out;
	}

 out:
	close(fd);

	return nr;
}

int vt_query_active_terminal(void)
{
	int fd, ret = -1;
	struct vt_stat vtstat;

	fd = get_console_fd(NULL);
	if (fd < 0) {
		return -1;
	}

	if (ioctl(fd, VT_GETSTATE, &vtstat) < 0) {
		goto out;
	}

	ret = vtstat.v_active;
 out:
	close(fd);
	return ret;
}

// libs/parser.h
struct unit_keyword_callback {
    const char *name;
    int (*func) (int nargs, char **args);
};

struct unit_parser {
    struct unit_keyword_callback *kw;
};

int parse_stream(const char *name, struct unit_parser *parser);
// libs/parser.c
#define PARSER_MAXARGS	32

#define T_EOF           1
#define T_STATEMENT     2
#define T_ARGUMENT  	3
#define T_NEWLINE       7
#define T_NEWBLOCK      8

struct parser_context {
	struct unit_keyword_callback *kw;
};

struct parser_state {
	char *ptr;
	char *stmt;
	int line;
	int nexttoken;
	void *context;
};

static void parser_init_state(struct parser_state *state, char *line)
{
	state->line = 1;
	state->ptr = line;
	state->nexttoken = 0;
    state->stmt = NULL;
    state->context = NULL;
}

static struct unit_keyword_callback *keyword_lookup(struct parser_context *ctx,
						    const char *kw)
{
	int i;

	for (i = 0; ctx->kw[i].name != NULL; i++) {
		if (!strcmp(ctx->kw[i].name, kw)) {
			return &ctx->kw[i];
		}
	}

	return NULL;
}

static int tokenize(struct parser_state *state)
{
	char *x = state->ptr;
	char *s, *ss;

	if (state->nexttoken) {
		int t = state->nexttoken;
		state->nexttoken = 0;
		return t;
	}

 retry:
	state->stmt = s = x;
	ss = x + 1;
 resume:
	while (1) {
		switch (*x) {
		case 0:
			state->nexttoken = T_EOF;
			goto textdone;
		case '\\':
			x++;
			switch (*x) {
			case 0:
				goto textdone;
			case 'n':
				*s++ = '\n';
				break;
			case 'r':
				*s++ = '\r';
				break;
			case 't':
				*s++ = '\t';
				break;
			case '\\':
				*s++ = '\\';
				break;
			case '\r':
				/* \ <cr> <lf> -> line continuation */
				if (x[1] != '\n') {
					x++;
					continue;
				}
			case '\n':
				/* \ <lf> -> line continuation */
				state->line++;
				x++;
				/* eat any extra whitespace */
				while ((*x == ' ') || (*x == '\t'))
					x++;
				continue;
			default:
				/* unknown escape -- just copy */
				*s++ = *x++;
			}
			continue;
		case ',':
			x++;
			goto textdone;
		case '=':
			x++;
			if (ss == x) {
				goto retry;
			}
			goto textdone;
		case ' ':
		case '\t':
		case '\r':
			x++;
			if (ss == x) {
				goto retry;
			}
			goto textdone;
		case '\n':
			x++;
			if (ss == x) {
				state->ptr = x;
				return T_NEWLINE;
			}
			state->nexttoken = T_NEWLINE;
			goto textdone;
		case '\'':
		case '"':
			x++;
			for (;;) {
				switch (*x) {
				case 0:
					/* unterminated quoted thing */
					state->ptr = x;
					return T_EOF;
				case '\'':
				case '"':
					x++;
					goto resume;
				default:
					*s++ = *x++;
				}
			}
			break;
		case '[':
			x++;
			goto resume;
		case ']':
			x++;
			goto resume;
		case '#':
			while (*x && (*x != '\n'))
				x++;
			if (*x == '\n') {
				state->ptr = x + 1;
				return T_NEWLINE;
			} else {
				state->ptr = x;
				return T_EOF;
			}
			break;
		default:
			*s++ = *x++;
		}
	}

 textdone:
	state->ptr = x;
	*s = 0;
	return T_STATEMENT;
}

static int parse_statement(struct parser_context *ctx, int argc, char **argv,
			   int (*func) (int argc, char **argv))
{
	struct parser_state state;
	char *args[PARSER_MAXARGS];
	int i, nargs, done, rc;
	int ret = 0;
    (void)ctx;

	for (i = 0; i < argc; i++) {
		done = nargs = 0;
		parser_init_state(&state, argv[i]);

		while (!done) {
			int token = tokenize(&state);
			switch (token) {
			case T_EOF:
				if (nargs && func) {
					rc = func(nargs, args);
					if (rc < 0) {
						WARN("Key word callback error");
					}
					nargs = 0;
				}
				done = 1;
				break;
			case T_STATEMENT:
				if (nargs < PARSER_MAXARGS) {
					args[nargs++] = state.stmt;
				}
				break;
			}
		}
	}

	return ret;
}

int parse_stream_core(struct parser_context *ctx, char *s)
{
	struct unit_keyword_callback *kw;
	struct parser_state state;
	char *args[PARSER_MAXARGS];
	int nargs, rc;

	nargs = 0;
	parser_init_state(&state, s);

	for (;;) {
		int token = tokenize(&state);
		switch (token) {
		case T_EOF:
			return 0;
		case T_NEWLINE:
			if (nargs) {
				if ((kw = keyword_lookup(ctx, args[0])) != NULL) {
					rc = parse_statement(ctx, nargs - 1,
							     &args[1],
							     kw->func);
					if (rc < 0) {
						return -EINVAL;
					}
				}

				nargs = 0;
			}
			break;
		case T_STATEMENT:
			if (nargs < PARSER_MAXARGS) {
				args[nargs++] = state.stmt;
			}
			break;
		}
	}

	return 0;
}

/* reads a file, making sure it is terminated with \n \0 */
static char *open_stream(const char *name, unsigned int *_sz)
{
	int sz, fd;
	char *data = NULL;

	fd = open(name, O_RDONLY);
	if (fd < 0)
		return NULL;

	sz = lseek(fd, 0, SEEK_END);
	if (sz < 0)
		goto oops;

	if (lseek(fd, 0, SEEK_SET) != 0)
		goto oops;

	data = (char *)malloc(sz + 2);
	if (data == 0)
		goto oops;

	if (read(fd, data, sz) != sz)
		goto oops;

	close(fd);

	data[sz] = '\n';
	data[sz + 1] = 0;
	if (_sz)
		*_sz = sz;

	return data;

 oops:
	close(fd);
	if (data != 0)
		free(data);

	return NULL;
}

int parse_stream(const char *name, struct unit_parser *parser)
{
	char *stream;
	struct parser_context *ctx;

	ctx = (struct parser_context *)malloc(sizeof(struct parser_context));
	if (ctx == NULL) {
		return -ENOMEM;
	}

	ctx->kw = parser->kw;

	/* File open & return file context */
	stream = open_stream(name, NULL);
	if (stream == NULL) {
		free(ctx);
		return -1;
	}

	parse_stream_core(ctx, stream);

	free(stream);
	free(ctx);

	return 0;
}

// dummy-ops
static int dummy_create_zone(vsm_context_h  /*ctx*/, const char * /*zone_name*/,
			     const char * /*template*/, int  /*flags*/)
{
	return -VSM_ERROR_NOT_SUPPORTED;
}

static int dummy_destroy_zone(vsm_context_h  /*ctx*/, const char * /*zone_name*/, int  /*force*/)
{
	return -VSM_ERROR_NOT_SUPPORTED;
}

static int dummy_start_zone(vsm_context_h  /*ctx*/, const char * /*zone_name*/)
{
	return -VSM_ERROR_NOT_SUPPORTED;
}

static int dummy_shutdown_zone(vsm_context_h  /*ctx*/, const char * /*zone_name*/, int  /*force*/)
{
	return -VSM_ERROR_NOT_SUPPORTED;
}

static int dummy_lock_zone(vsm_context_h  /*ctx*/, const char * /*zone_name*/, int  /*shutdown*/)
{
	return -VSM_ERROR_NOT_SUPPORTED;
}

static int dummy_unlock_zone(vsm_context_h  /*ctx*/, const char * /*zone_name*/)
{
	return -VSM_ERROR_NOT_SUPPORTED;
}

static int dummy_set_foreground(vsm_zone_h  zone)
{
	if (zone == NULL)
		return -VSM_ERROR_INVALID;

	if (zone->parent == zone) {
		return VSM_ERROR_NONE;
	}
	return -VSM_ERROR_NO_OBJECT;
}

static vsm_zone_h dummy_get_foreground(vsm_context_h  ctx)
{
	if (ctx == NULL) {
		errno = EINVAL;
		return NULL;
	}

	return ctx->root_zone;
}

static int dummy_iterate_zone(vsm_context_h  ctx, vsm_zone_iter_cb  callback, void *user_data)
{
	if( callback) {
		callback(ctx->root_zone, user_data);
	}
	return VSM_ERROR_NONE;
}

static vsm_zone_h dummy_lookup_zone_by_name(vsm_context_h ctx, const char *name)
{
	if (strcmp(name, "") != 0) {
		errno = ESRCH;
		return NULL;
	}

	return ctx->root_zone;
}

static vsm_zone_h dummy_lookup_zone_by_pid(vsm_context_h ctx, pid_t  /*pid*/)
{
	if (ctx == NULL)
		return NULL;

	return ctx->root_zone;
}

static int dummy_attach_zone(vsm_context_h ctx, const char *zone_name,
			     vsm_attach_command_s * command,
			     vsm_attach_options_s * opts,
			     pid_t * attached_process)
{
	pid_t pid;
	struct vsm_attach_options_s options;

	if (command == NULL || command->exec == NULL || zone_name == NULL) {
		ERROR("Invalid arguments");
		ctx->error = VSM_ERROR_INVALID;
		return -VSM_ERROR_INVALID;
	}

	if (strcmp("", zone_name) != 0) {
		ctx->error = VSM_ERROR_INVALID;
		return -VSM_ERROR_INVALID;
	}

	if (opts == NULL) {
		opts = &options;
		opts->uid = getuid();
		opts->gid = getgid();
		opts->env_num = 0;
		opts->extra_env = NULL;
	}

	pid = fork();
	if (pid == 0) {
		if (opts->extra_env != NULL) {
			while (*opts->extra_env)
				putenv(*opts->extra_env++);
		}

		if (getuid() == 0 && opts->uid != 0) {
			if (setuid(opts->uid) != 0) {
				ERROR("Failed to set uid : %d", opts->uid);
			}
		} else {
			WARN("setuid() is not permitted");
		}

		if (getgid() == 0 && opts->gid != 0) {
			if (setgid(opts->gid) != 0) {
				ERROR("Failed to set gid : %d", opts->gid);
			}
		} else {
			WARN("setgid() is not permitted");
		}

		if (execvp(command->exec, command->argv) < 0) {
			ERROR("exevp failed : %s, %s", command->exec,
			      strerror(errno));
			exit(EXIT_FAILURE);
		}
	} else {
		*attached_process = pid;
	}

	return VSM_ERROR_NONE;
}

static int dummy_attach_zone_wait(vsm_context_h  ctx, const char *zone_name,
				  vsm_attach_command_s * command,
				  vsm_attach_options_s * opts)
{
	pid_t pid=0;
	int ret, status;

	ret = dummy_attach_zone(ctx, zone_name, command, opts, &pid);
	if (ret != VSM_ERROR_NONE) {
		ERROR("API Failed.");
		return ret;
	}

	status = wait_for_pid_status(pid);
	if (status == -1) {
		ctx->error = VSM_ERROR_GENERIC;
		return -VSM_ERROR_GENERIC;
	}

	INFO("attached process extied : pid - %d, exit code : %d", pid,
	     WEXITSTATUS(status));

	return status;
}

static vsm_zone_h dummy_join_zone(vsm_zone_h zone)
{
	if (zone == NULL) {
		errno = EINVAL;
		return NULL;
	}
	if (zone != zone->parent) {
		errno = EINVAL;
		return NULL;
	}

	return zone;
}

static int dummy_is_equivalent_zone(vsm_context_h  /*ctx*/, pid_t  /*pid*/)
{
	return 1;
}

static int dummy_get_host_pid(vsm_zone_h zone, pid_t pid)
{
	if(zone == zone->parent)
		return pid;

	return -VSM_ERROR_NO_OBJECT;
}

static int dummy_grant_device(vsm_zone_h  /*zone*/, const char * /*path*/, uint32_t  /*flags*/)
{
	return -VSM_ERROR_NOT_SUPPORTED;
}

static int dummy_revoke_device(vsm_zone_h  /*zone*/, const char * /*path*/)
{
	return -VSM_ERROR_NOT_SUPPORTED;
}

static int dummy_declare_file(vsm_context_h  /*ctx*/, vsm_fso_type_t  /*ftype*/,
			      const char * /*path*/, int  /*flags*/, vsm_mode_t  /*mode*/)
{
	return VSM_ERROR_NONE;
}

static int dummy_declare_link(vsm_context_h  /*ctx*/, const char *source,
			      const char * /*target*/)
{
	int ret;

	ret = access(source, F_OK);
	if (ret != 0)
		return -VSM_ERROR_NO_OBJECT;

	return VSM_ERROR_NONE;
}

struct vasum_ops dummy_ops;
static int dummy_ops_init(){
	dummy_ops.create_zone = dummy_create_zone;
	dummy_ops.destroy_zone = dummy_destroy_zone;
	dummy_ops.start_zone = dummy_start_zone;
	dummy_ops.shutdown_zone = dummy_shutdown_zone;
	dummy_ops.lock_zone = dummy_lock_zone;
	dummy_ops.unlock_zone = dummy_unlock_zone;
	dummy_ops.set_foreground = dummy_set_foreground;
	dummy_ops.get_foreground = dummy_get_foreground;
	dummy_ops.iterate_zone = dummy_iterate_zone;
	dummy_ops.lookup_zone_by_name = dummy_lookup_zone_by_name;
	dummy_ops.lookup_zone_by_pid = dummy_lookup_zone_by_pid;
	dummy_ops.attach_zone = dummy_attach_zone;
	dummy_ops.attach_zone_wait = dummy_attach_zone_wait;
	dummy_ops.join_zone = dummy_join_zone;
	dummy_ops.is_equivalent_zone = dummy_is_equivalent_zone;
	dummy_ops.get_host_pid = dummy_get_host_pid;
	dummy_ops.grant_device = dummy_grant_device;
	dummy_ops.revoke_device = dummy_revoke_device;
	dummy_ops.declare_file = dummy_declare_file;
	dummy_ops.declare_link = dummy_declare_link;
    return 0;
}
int dummy_ops_init_i = dummy_ops_init();

} //extern "C"
