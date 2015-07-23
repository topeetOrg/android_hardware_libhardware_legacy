/*
 * Copyright (C) 2008 The Android Open Source Project
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

#ifndef _WIFI_COMMON_H
#define _WIFI_COMMON_H

#include "libwpa_client/wpa_ctrl.h"
#include "cutils/properties.h"

#define PARA_LENGTH 64

typedef struct wpa_ctrl WPA_CTRL_T, *P_WPA_CTRL_T, **PP_WPA_CTRL_T;

typedef struct _SUPPLICANT_CTRL_T {
    struct wpa_ctrl* (*ctrl_open)(const char *ctrl_path);
    void (*ctrl_close)(struct wpa_ctrl *ctrl);
    int (*ctrl_request)(struct wpa_ctrl *ctrl, const char *cmd, size_t cmd_len, char *reply, size_t *reply_len, void (*msg_cb)(char *msg, size_t len));
    int (*ctrl_recv)(struct wpa_ctrl *ctrl, char *reply, size_t *reply_len);
    int (*ctrl_attach)(struct wpa_ctrl *ctrl);
} SUPPLICANT_CTRL_T, *P_SUPPLICANT_CTRL_T;


/*static supplicant parameter*/
typedef struct _SUPPLICANT_PARA_T {
    char acIfDefName[PARA_LENGTH];              //Default interface name
    char acIfPropName[PARA_LENGTH];             //Interface property name
    char acIfDir[PARA_LENGTH];                  //Supplicant path
    char acSuppName[PARA_LENGTH];               //Supplicant name
    char acSuppPropName[PARA_LENGTH];           //Supplicant property name
    char acSuppConfigTemplate[PARA_LENGTH];     //Supplicant config template file path
    char acSuppConfigFile[PARA_LENGTH];         //Supplicant config file path
    char acSuppDeamonCmd[PROPERTY_VALUE_MAX];   //Supplicant deamon start command
    char acSuppCtrlPrefix[PARA_LENGTH];         //Ctrl socket prefix
} SUPPLICANT_PARA_T, *P_SUPPLICANT_PARA_T;

typedef struct _SUPPLICANT_INFO_T {
    char acIfName[PROPERTY_VALUE_MAX];          //Interface name for using
    P_WPA_CTRL_T prCtrlConn;                    //Supplicant ctrl connection
    P_WPA_CTRL_T prMonitorConn;                 //Supplicant monitor connection
    SUPPLICANT_CTRL_T rSuppCtrl;                //Supplicant ctrl api
    int aucExitSockets[2];                      /* socket pair used to exit from a blocking read */
} SUPPLICANT_INFO_T, *P_SUPPLICANT_INFO_T;

int halDoCommand(const char *cmd);

int halDoMonitor(int sock);

int start_supplicant(const struct _SUPPLICANT_PARA_T *prTar, const char *pcIface);
int stop_supplicant(const struct _SUPPLICANT_PARA_T *prTar, P_SUPPLICANT_INFO_T prTarInfo);
int send_command(P_SUPPLICANT_INFO_T prTarInfo, const char *cmd, char *reply, size_t *reply_len);
int wait_for_event(P_SUPPLICANT_INFO_T prTarInfo, char *buf, size_t buflen);
int connect_to_supplicant(const struct _SUPPLICANT_PARA_T *prTar, P_SUPPLICANT_INFO_T prTarInfo);
void close_supplicant_connection(const struct _SUPPLICANT_PARA_T *prTar, P_SUPPLICANT_INFO_T prTarInfo);
int set_iface(const char *iface, int on);
int is_driver_loaded(const char *moduleTag, const char *propName);
int do_dhcp_client_request(
    const char *iface,
    int *ipaddr,
    int *gateway,
    int *mask,
    int *dns1,
    int *dns2,
    int *server,
    int *lease);

#endif

