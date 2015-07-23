/*
 * Copyright (C) 2010 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <dirent.h>

#include "hardware_legacy/wifi.h"
#include "libwpa_client/wpa_ctrl.h"

#include "private/android_filesystem_config.h"

#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <net/if_arp.h>		    /* For ARPHRD_ETHER */
#include <sys/socket.h>		    /* For AF_INET & struct sockaddr */
#include <netinet/in.h>         /* For struct sockaddr_in */
#include <netinet/if_ether.h>
#include <linux/wireless.h>


#include <cutils/sockets.h>
#include <private/android_filesystem_config.h>

#define LOG_TAG "WifiCommon"
#include "cutils/log.h"
#include "cutils/memory.h"
#include "cutils/misc.h"
#include "cutils/properties.h"

#include "hardware_legacy/wifi_common.h"

#ifdef HAVE_LIBC_SYSTEM_PROPERTIES
#define _REALLY_INCLUDE_SYS__SYSTEM_PROPERTIES_H_
#include <sys/_system_properties.h>
#endif


#define HAL_DAEMON_CMD              "hal"
#define HAL_DAEMON_NAME             "hald"
#define HAL_DAEMON_CMD_LENGTH       255

#define SET_IFACE_DELAY                 300000
#define SET_IFACE_POLLING_LOOP          20
#define CONNECT_TO_SUPP_DELAY           300000
#define CONNECT_TO_SUPP_POLLING_LOOP    20

#define MODULE_FILE                 "/proc/modules"

extern int ifc_init();
extern void ifc_close();
extern int ifc_up(const char *name);
extern int ifc_down(const char *name);
extern char *dhcp_lasterror();
extern void get_dhcp_info();
extern int do_dhcp();

int halDoCommand(const char *cmd)
{
    int sock;
    char *final_cmd;

    if ((sock = socket_local_client(HAL_DAEMON_NAME,
                                     ANDROID_SOCKET_NAMESPACE_RESERVED,
                                     SOCK_STREAM)) < 0) {
        LOGE("Error connecting (%s)", strerror(errno));
        //exit(4);
        /*return error if hald is not existing*/
        return errno;
    }

    asprintf(&final_cmd, "%s %s", HAL_DAEMON_CMD, cmd);

    LOGD("Hal cmd: \"%s\"", final_cmd);

    if (write(sock, final_cmd, strlen(final_cmd) + 1) < 0) {
        free(final_cmd);
	close(sock);
        LOGE("Hal cmd error: \"%s\"", final_cmd);
        return errno;
    }
    free(final_cmd);
    return halDoMonitor(sock);
}


int halDoMonitor(int sock)
{
    char *buffer = malloc(4096);

    while(1) {
        fd_set read_fds;
        struct timeval to;
        int rc = 0;

        to.tv_sec = 10;
        to.tv_usec = 0;

        FD_ZERO(&read_fds);
        FD_SET(sock, &read_fds);

        if ((rc = select(sock +1, &read_fds, NULL, NULL, &to)) < 0) {
            LOGE("Error in select (%s)", strerror(errno));
            free(buffer);
            close(sock);
            return errno;
        } else if (!rc) {
            continue;
            LOGE("[TIMEOUT]");
            close(sock);
            return ETIMEDOUT;
        } else if (FD_ISSET(sock, &read_fds)) {
            memset(buffer, 0, 4096);
            if ((rc = read(sock, buffer, 4096)) <= 0) {
                if (rc == 0) {
                    LOGE("Lost connection to Hald - did it crash?");
                }
                else {
                    LOGE("Error reading data (%s)", strerror(errno));
                }
                close(sock);
                free(buffer);
                if (rc == 0) {
                    return ECONNRESET;
                }
                return errno;
            }

            int offset = 0;
            int i = 0;

            for (i = 0; i < rc; i++) {
                if (buffer[i] == '\0') {
                    int code;
                    char tmp[4];

                    strncpy(tmp, buffer + offset, 3);
                    tmp[3] = '\0';
                    code = atoi(tmp);

                    LOGD("Hal cmd response code: \"%d\"", code);
                    if (code >= 200 && code < 600) {
                        int ret = 0;

                        switch(code) {
                            /*the requested action did not take place.*/
                            case 400:
                            case 500:
                            case 501:
                                ret = -1;
                                break;
                            /*Requested action has been successfully completed*/
                            default:
                                ret = 0;
                                break;
                        }

                        close(sock);
                        free(buffer);
                        return ret;
                    }
                    offset = i + 1;
                }
            }
        }
    }
    close(sock);
    free(buffer);
    return 0;
}


int init_iface(const char *ifname)
{
    int s, ret = 0;
    struct iwreq wrq;
    char buf[33];

    s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s < 0) {
        perror("socket(AF_INET,SOCK_DGRAM)");
        return -1;
    }

    LOGD("[%s] init_iface: set mode\n", ifname);

    memset(&wrq, 0, sizeof(struct iwreq));
    strncpy(wrq.ifr_name, ifname, IFNAMSIZ);
    wrq.u.mode = IW_MODE_INFRA;

    if (ioctl(s, SIOCSIWMODE, &wrq) < 0){
        perror("ioctl(SIOCSIWMODE)");
        ret = -1;
    }
    close(s);
    return ret;
}


int set_iface(const char *iface, int on)
{
    int u4Count = 0;

    if(ifc_init() != 0) {
        LOGE("[%s] interface set %d failed", iface, on);
        return -1;
    }
    if(on) {
        while(ifc_up(iface) == -1) {
            LOGD("[%s] interface is not ready, wait %dus", iface, SET_IFACE_DELAY);
            sched_yield();
            usleep(SET_IFACE_DELAY);
            if (++u4Count >= SET_IFACE_POLLING_LOOP) {
                LOGE("[%s] interface set %d failed", iface, on);
                ifc_close();
                return -1;
            }
        }
        LOGD("[%s] interface is up", iface);
        init_iface(iface);
    }
    else {
        ifc_down(iface);
        LOGD("[%s] interface is down", iface);
    }
    ifc_close();
    return 0;
}


static int ensure_config_file_exists(const char *configFile, const char *configTemplate)
{
    char buf[2048];
    int srcfd, destfd;
    int nread;

    if (access(configFile, R_OK|W_OK) == 0) {
        return 0;
    } else if (errno != ENOENT) {
        LOGE("Cannot access \"%s\": %s", configFile, strerror(errno));
        return -1;
    }

    srcfd = open(configTemplate, O_RDONLY);
    if (srcfd < 0) {
        LOGE("Cannot open \"%s\": %s", configTemplate, strerror(errno));
        return -1;
    }

    destfd = open(configFile, O_CREAT|O_WRONLY, 0660);
    if (destfd < 0) {
        close(srcfd);
        LOGE("Cannot create \"%s\": %s", configFile, strerror(errno));
        return -1;
    }

    while ((nread = read(srcfd, buf, sizeof(buf))) != 0) {
        if (nread < 0) {
            LOGE("Error reading \"%s\": %s", configTemplate, strerror(errno));
            close(srcfd);
            close(destfd);
            unlink(configFile);
            return -1;
        }
        write(destfd, buf, nread);
    }

    close(destfd);
    close(srcfd);

    /* chmod is needed because open() didn't set permisions properly */
    if (chmod(configFile, 0660) < 0) {
        LOGE("Error changing permissions of %s to 0660: %s",
             configFile, strerror(errno));
        unlink(configFile);
        return -1;
    }

    if (chown(configFile, AID_SYSTEM, AID_WIFI) < 0) {
        LOGE("Error changing group ownership of %s to %d: %s",
             configFile, AID_WIFI, strerror(errno));
        unlink(configFile);
        return -1;
    }
    return 0;
}


void close_supplicant_ctrl_connection(P_SUPPLICANT_INFO_T prTarInfo)
{
    LOGD("[%s] Close CTRL connection", prTarInfo->acIfName);
    if (prTarInfo->prCtrlConn != NULL) {
        prTarInfo->rSuppCtrl.ctrl_close(prTarInfo->prCtrlConn);
        prTarInfo->prCtrlConn = NULL;
    }
}


void close_supplicant_connection(const struct _SUPPLICANT_PARA_T * prTar, P_SUPPLICANT_INFO_T prTarInfo)
{
    char supp_status[PROPERTY_VALUE_MAX] = {'\0'};
    int count = 50; /* wait at most 5 seconds to ensure init has stopped stupplicant */

    LOGD("[%s] Close CTRL & MONITOR connection", prTarInfo->acIfName);
    if (prTarInfo->prCtrlConn != NULL) {
        prTarInfo->rSuppCtrl.ctrl_close(prTarInfo->prCtrlConn);
        prTarInfo->prCtrlConn = NULL;
    }
    if (prTarInfo->prMonitorConn != NULL) {
        prTarInfo->rSuppCtrl.ctrl_close(prTarInfo->prMonitorConn);
        prTarInfo->prMonitorConn = NULL;
    }
    if (prTarInfo->aucExitSockets[0] >= 0) {
        close(prTarInfo->aucExitSockets[0]);
        prTarInfo->aucExitSockets[0] = -1;
    }

    if (prTarInfo->aucExitSockets[1] >= 0) {
        close(prTarInfo->aucExitSockets[1]);
        prTarInfo->aucExitSockets[1] = -1;
    }

    while (count-- > 0) {
        if (property_get(prTar->acSuppPropName, supp_status, NULL)) {
            if (strcmp(supp_status, "stopped") == 0)
                return;
        }
        usleep(100000);
    }

}


/**
 * ctrl_cleanup() - Delete any local UNIX domain socket files that
 * may be left over from clients that were previously connected to
 * wpa_supplicant. This keeps these files from being orphaned in the
 * event of crashes that prevented them from being removed as part
 * of the normal orderly shutdown.
 */
void ctrl_cleanup(const char *local_socket_prefix)
{
    DIR *dir;
    struct dirent entry;
    struct dirent *result;
    size_t dirnamelen;
    size_t maxcopy;
    char pathname[PATH_MAX];
    char *namep;
    char *local_socket_dir = CONFIG_CTRL_IFACE_CLIENT_DIR;
    //char *local_socket_prefix = CONFIG_CTRL_IFACE_CLIENT_PREFIX;

    if ((dir = opendir(local_socket_dir)) == NULL)
        return;

    dirnamelen = (size_t)snprintf(pathname, sizeof(pathname), "%s/", local_socket_dir);
    if (dirnamelen >= sizeof(pathname)) {
        closedir(dir);
        return;
    }
    namep = pathname + dirnamelen;
    maxcopy = PATH_MAX - dirnamelen;
    while (readdir_r(dir, &entry, &result) == 0 && result != NULL) {
        if (strncmp(entry.d_name, local_socket_prefix, strlen(local_socket_prefix)) == 0) {
            if (strlcpy(namep, entry.d_name, maxcopy) < maxcopy) {
                unlink(pathname);
            }
        }
    }
    closedir(dir);
}


int start_supplicant(const struct _SUPPLICANT_PARA_T * prTar, const char *pcIface)
{
    char daemon_cmd[PROPERTY_VALUE_MAX];
    char supp_status[PROPERTY_VALUE_MAX] = {'\0'};
    int count = 200; /* wait at most 20 seconds for completion */
#ifdef HAVE_LIBC_SYSTEM_PROPERTIES
    const prop_info *pi;
    unsigned serial = 0;
#endif

    /* Check whether already running */
    if (property_get(prTar->acSuppPropName, supp_status, NULL)
            && strcmp(supp_status, "running") == 0) {
        return 0;
    }

    /* Before starting the daemon, make sure its config file exists */
    if (ensure_config_file_exists(prTar->acSuppConfigFile, prTar->acSuppConfigTemplate) < 0) {
        LOGE("[%s] %s will not be enabled", pcIface, prTar->acSuppName);
        return -1;
    }

    /*Set interface UP (ifconfig "iface" up)*/
    if(set_iface(pcIface, 1) < 0 ) {
        /*If interface up failed, skip the following actions*/
    	return -1;
    }

    /* Clear out any stale socket files that might be left over. */
    LOGD("[%s] clear out stale sockets with prefix \"%s\" in %s", pcIface, prTar->acSuppCtrlPrefix, CONFIG_CTRL_IFACE_CLIENT_DIR);
    ctrl_cleanup(prTar->acSuppCtrlPrefix);

    LOGI("[%s] start %s", pcIface, prTar->acSuppName);
#ifdef HAVE_LIBC_SYSTEM_PROPERTIES
    /*
     * Get a reference to the status property, so we can distinguish
     * the case where it goes stopped => running => stopped (i.e.,
     * it start up, but fails right away) from the case in which
     * it starts in the stopped state and never manages to start
     * running at all.
     */
    pi = __system_property_find(prTar->acSuppPropName);
    if (pi != NULL) {
        serial = pi->serial;
    }
#endif
    property_get(prTar->acIfPropName, (char *)pcIface, prTar->acIfDefName);
    snprintf(daemon_cmd, PROPERTY_VALUE_MAX, prTar->acSuppDeamonCmd, prTar->acSuppName, pcIface, prTar->acSuppConfigFile);
    property_set("ctl.start", daemon_cmd);
    LOGD("[%s] supplicant start command: \"%s\"", pcIface, daemon_cmd);
    //property_set("ctl.start", prTar->acSuppName);
    sched_yield();

    while (count-- > 0) {
#ifdef HAVE_LIBC_SYSTEM_PROPERTIES
        if (pi == NULL) {
            pi = __system_property_find(prTar->acSuppPropName);
        }
        if (pi != NULL) {
            __system_property_read(pi, NULL, supp_status);
            if (strcmp(supp_status, "running") == 0) {
                return 0;
            } else if (pi->serial != serial &&
                    strcmp(supp_status, "stopped") == 0) {
                return -1;
            }
        }
#else
        if (property_get(prTar->acSuppPropName, supp_status, NULL)) {
            if (strcmp(supp_status, "running") == 0)
                return 0;
        }
#endif
        usleep(100000);
    }
    return -1;
}


int stop_supplicant(const struct _SUPPLICANT_PARA_T *prTar, P_SUPPLICANT_INFO_T prTarInfo)
{
    char supp_status[PROPERTY_VALUE_MAX] = {'\0'};
    int count = 50; /* wait at most 5 seconds for completion */

    //close_supplicant_ctrl_connection(prTarInfo);

    LOGI("[%s] stop %s", prTarInfo->acIfName, prTar->acSuppName);

    /* Check whether supplicant already stopped */
    if (property_get(prTar->acSuppPropName, supp_status, NULL)
        && strcmp(supp_status, "stopped") == 0) {
        return 0;
    }

    property_set("ctl.stop", prTar->acSuppName);
    sched_yield();

    while (count-- > 0) {
        if (property_get(prTar->acSuppPropName, supp_status, NULL)) {
            if (strcmp(supp_status, "stopped") == 0) {
                /* cp.wu 1) clear IFF_UP */
                /*Set interface DOWN (ifconfig "iface" down)*/
				/* workaround CR:ALPS00076516 */
                //if(set_iface(prTarInfo->acIfName, 0) < 0) {
                    /*If interface down failed, skip the following actions*/
                    //return -1;
                //}
                return 0;
            }
        }
        usleep(100000);
    }
    return -1;
}


int connect_to_supplicant(const struct _SUPPLICANT_PARA_T *prTar, P_SUPPLICANT_INFO_T prTarInfo)
{
    char ifname[256];
    char supp_status[PROPERTY_VALUE_MAX] = {'\0'};

    /* Make sure supplicant is running */
    if (!property_get(prTar->acSuppPropName, supp_status, NULL)
            || strcmp(supp_status, "running") != 0) {
        LOGE("[%s] %s not running, cannot connect", prTarInfo->acIfName, prTar->acSuppName);
        return -1;
    }

    LOGI("[%s] Connect to %s", prTarInfo->acIfName, prTar->acSuppName);

    snprintf(ifname, sizeof(ifname), "%s/%s", prTar->acIfDir, prTarInfo->acIfName);
	LOGD("[%s] ctrl_open %s", prTarInfo->acIfName, ifname);
    {
        int count = CONNECT_TO_SUPP_POLLING_LOOP;
        while (count-- > 0) {
            prTarInfo->prCtrlConn = prTarInfo->rSuppCtrl.ctrl_open(ifname);
            if (prTarInfo->prCtrlConn) {
                LOGD("[%s] Open control connection to %s successfully\n", prTarInfo->acIfName, ifname);
                break;
            }
            LOGD("[%s] Open control connection fail (%s). Sleep %dus\n", prTarInfo->acIfName, ifname, CONNECT_TO_SUPP_DELAY);
            sched_yield();
            usleep(CONNECT_TO_SUPP_DELAY);
        }
    }

    if (prTarInfo->prCtrlConn == NULL) {
        LOGE("[%s] Unable to open connection to supplicant on \"%s\": %s"
            , prTarInfo->acIfName
            , ifname
            , strerror(errno));
        return -1;
    }
    prTarInfo->prMonitorConn = prTarInfo->rSuppCtrl.ctrl_open(ifname);
    if (prTarInfo->prMonitorConn == NULL) {
        prTarInfo->rSuppCtrl.ctrl_close(prTarInfo->prCtrlConn);
        prTarInfo->prCtrlConn = NULL;
        return -1;
    }
    if (prTarInfo->rSuppCtrl.ctrl_attach(prTarInfo->prMonitorConn) != 0) {
        prTarInfo->rSuppCtrl.ctrl_close(prTarInfo->prMonitorConn);
        prTarInfo->rSuppCtrl.ctrl_close(prTarInfo->prCtrlConn);
        prTarInfo->prCtrlConn = prTarInfo->prMonitorConn = NULL;
        return -1;
    }
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, prTarInfo->aucExitSockets) == -1) {
        prTarInfo->rSuppCtrl.ctrl_close(prTarInfo->prMonitorConn);
        prTarInfo->rSuppCtrl.ctrl_close(prTarInfo->prCtrlConn);
        prTarInfo->prCtrlConn = prTarInfo->prMonitorConn = NULL;
        return -1;
    }

    LOGD("[%s] Connect_to_supplicant %s successfully.\n", prTarInfo->acIfName, ifname);

    return 0;
}


int send_command(P_SUPPLICANT_INFO_T prTarInfo, const char *cmd, char *reply, size_t *reply_len)
{
    int ret;

    if (prTarInfo->prCtrlConn == NULL) {
        LOGE("[%s] Not connected to supplicant - \"%s\" command dropped.\n", prTarInfo->acIfName, cmd);
        return -1;
    }
    LOGD("[%s] Issue cmd = '%s'\n", prTarInfo->acIfName, cmd);
    ret = prTarInfo->rSuppCtrl.ctrl_request(prTarInfo->prCtrlConn, cmd, strlen(cmd), reply, reply_len, NULL);
    if (ret == -2) {
        LOGD("[%s] cmd = '%s' ==> timed out.\n", prTarInfo->acIfName, cmd);
        /* unblocks the monitor receive socket for termination */
        write(prTarInfo->aucExitSockets[0], "T", 1);
        return -2;
    } else if (ret < 0) {
        LOGD("[%s] cmd = '%s' ==> failed. (ctrl socket failed).\n", prTarInfo->acIfName, cmd);
        return -1;
    } else if (strncmp(reply, "FAIL", 4) == 0) {
        LOGD("[%s] cmd = '%s' ==> failed. (supplicant reply 'FAIL').\n", prTarInfo->acIfName, cmd);
        return -1;
    }
    if (strncmp(cmd, "PING", 4) == 0) {
        reply[*reply_len] = '\0';
    }
    return 0;

}

int ctrl_recv(P_SUPPLICANT_INFO_T prTarInfo, char *reply, size_t *reply_len)
{
    int res;
    int ctrlfd = wpa_ctrl_get_fd(prTarInfo->prMonitorConn);
    struct pollfd rfds[2];

    memset(rfds, 0, 2 * sizeof(struct pollfd));
    rfds[0].fd = ctrlfd;
    rfds[0].events |= POLLIN;
    rfds[1].fd = prTarInfo->aucExitSockets[1];
    rfds[1].events |= POLLIN;
    res = poll(rfds, 2, -1);
    if (res < 0) {
        LOGE("[%s] Error poll = %d", prTarInfo->acIfName, res);
        return res;
    }
    if (rfds[0].revents & POLLIN) {
        return prTarInfo->rSuppCtrl.ctrl_recv(prTarInfo->prMonitorConn, reply, reply_len);
    } else {
        LOGD("[%s] Received on exit socket, terminate", prTarInfo->acIfName);
        return -1;
    }
    return 0;
}

int wait_for_event(P_SUPPLICANT_INFO_T prTarInfo, char *buf, size_t buflen)
{
    size_t nread = buflen - 1;
    int fd;
    fd_set rfds;
    int result;
    struct timeval tval;
    struct timeval *tptr;

    if (prTarInfo->prMonitorConn == NULL) {
        LOGD("[%s] Connection closed\n", prTarInfo->acIfName);
        strncpy(buf, WPA_EVENT_TERMINATING " - connection closed", buflen-1);
        buf[buflen-1] = '\0';
        return strlen(buf);
    }

    result = ctrl_recv(prTarInfo, buf, &nread);
    if (result < 0) {
        LOGD("[%s] ctrl_recv failed: %s\n", prTarInfo->acIfName, strerror(errno));
        strncpy(buf, WPA_EVENT_TERMINATING " - recv error", buflen-1);
        buf[buflen-1] = '\0';
        return strlen(buf);
    }
    buf[nread] = '\0';
    LOGD("[%s] evt = \"%s\", result = %d, nread = %d \n", prTarInfo->acIfName, buf, result, nread);
    /* LOGD("wait_for_event: result=%d nread=%d string=\"%s\"\n", result, nread, buf); */
    /* Check for EOF on the socket */
    if (result == 0 && nread == 0) {
        /* Fabricate an event to pass up */
        LOGD("[%s] Received EOF on supplicant socket\n", prTarInfo->acIfName);
        strncpy(buf, WPA_EVENT_TERMINATING " - signal 0 received", buflen-1);
        buf[buflen-1] = '\0';
        return strlen(buf);
    }

    /*
     * Events strings are in the format
     *
     *     <N>CTRL-EVENT-XXX
     *
     * where N is the message level in numerical form (0=VERBOSE, 1=DEBUG,
     * etc.) and XXX is the event name. The level information is not useful
     * to us, so strip it off.
     */
    if (buf[0] == '<') {
        char *match = strchr(buf, '>');
        if (match != NULL) {
            nread -= (match+1-buf);
            memmove(buf, match+1, nread+1);
        }
    }

    return nread;
}

int is_driver_loaded(const char *moduleTag, const char *propName)
{
    char driver_status[PROPERTY_VALUE_MAX];
    FILE *proc;
    char line[sizeof(moduleTag)+10];

    if (!property_get(propName, driver_status, NULL)
            || strcmp(driver_status, "running") != 0) {
        LOGD("[%s] %s: \"%s\"\n", moduleTag, propName, driver_status);
        return 0;  /* driver is not RUNNING */
    }
    /*
     * If the property says the driver is RUNNING, check to
     * make sure that the property setting isn't just left
     * over from a previous manual shutdown or a runtime
     * crash.
     */
    if ((proc = fopen(MODULE_FILE, "r")) == NULL) {
        LOGW("Could not open %s: %s", MODULE_FILE, strerror(errno));
        property_set(propName, "unloaded");
        LOGD("[%s] %s: \"%s\"\n", moduleTag, propName, "unloaded");
        return 0;
    }
    while ((fgets(line, sizeof(line), proc)) != NULL) {
        if (strncmp(line, moduleTag, strlen(moduleTag)) == 0) {
            fclose(proc);
            LOGD("[%s] %s: \"%s\"\n", moduleTag, propName, driver_status);
            return 1;
        }
    }
    fclose(proc);
    property_set(propName, "unloaded");
    LOGD("[%s] %s: \"%s\"\n", moduleTag, propName, "unloaded");
    return 0;
}

int do_dhcp_client_request(const char *iface, int *ipaddr, int *gateway, int *mask,
                    int *dns1, int *dns2, int *server, int *lease) {

    LOGD("[%s] do_dhcp_client_request\n", iface);

    if (ifc_init() < 0)
        return -1;

    if (do_dhcp(iface) < 0) {
        ifc_close();
        return -1;
    }
    ifc_close();
    get_dhcp_info(ipaddr, gateway, mask, dns1, dns2, server, lease);
    return 0;
}

