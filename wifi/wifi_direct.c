/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. and/or its licensors.
 * Without the prior written permission of MediaTek inc. and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
 */
/* MediaTek Inc. (C) 2010. All rights reserved.
 *
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
 * RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER ON
 * AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
 * NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
 * SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
 * SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH
 * THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES
 * THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES
 * CONTAINED IN MEDIATEK SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK
 * SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND
 * CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
 * AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE,
 * OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
 * MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 */

/*
 * Copyright 2008, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>

#include "hardware_legacy/wifi_direct.h"
#include "hardware_legacy/wifi_common.h"

#define LOG_TAG "WifiDirect"
#include "cutils/log.h"
#include "cutils/memory.h"
#include "cutils/misc.h"
#include "cutils/properties.h"
#include "private/android_filesystem_config.h"
#ifdef HAVE_LIBC_SYSTEM_PROPERTIES
#define _REALLY_INCLUDE_SYS__SYSTEM_PROPERTIES_H_
#include <sys/_system_properties.h>
#endif

extern struct wpa_ctrl* p2p_ctrl_open(const char *ctrl_path);
extern void p2p_ctrl_close(struct wpa_ctrl *ctrl);
extern int p2p_ctrl_request(struct wpa_ctrl *ctrl, const char *cmd, size_t cmd_len, char *reply, size_t *reply_len, void (*msg_cb)(char *msg, size_t len));
extern int p2p_ctrl_recv(struct wpa_ctrl *ctrl, char *reply, size_t *reply_len);
extern int p2p_ctrl_attach(struct wpa_ctrl *ctrl);

const SUPPLICANT_PARA_T rDirectSuppPara = {
    "p2p0",
    "wifi.direct.interface",
    "/data/misc/wifi/p2p_supplicant",
    "p2p_supplicant",
    "init.svc.p2p_supplicant",
    "/system/etc/wifi/p2p_supplicant.conf",
    "/data/misc/wifi/p2p_supplicant.conf",
    "%s:-Dnl80211 -i%s -c%s -dd",
#ifdef CFG_SUPPORT_CONCURRENT_NETWORK
    "p2p_ctrl"
#else
    "wpa_ctrl"
#endif
};

SUPPLICANT_INFO_T rDirectSuppInfo = {
    "p2p0",
    NULL,
    NULL,
#ifdef CFG_SUPPORT_CONCURRENT_NETWORK
    {
        p2p_ctrl_open,
        p2p_ctrl_close,
        p2p_ctrl_request,
        p2p_ctrl_recv,
        p2p_ctrl_attach
    },
#else
    {   //Supplicant ctrl API
        wpa_ctrl_open,
        wpa_ctrl_close,
        wpa_ctrl_request,
        wpa_ctrl_recv,
        wpa_ctrl_attach
    },
#endif
    {                           //exit socket pair
        -1,
        -1
    }
};

int wifi_direct_load_driver()
{
    property_get(
        rDirectSuppPara.acIfPropName,
        rDirectSuppInfo.acIfName,
        rDirectSuppPara.acIfDefName);
    return halDoCommand("load p2p");
}

int wifi_direct_unload_driver()
{
    return halDoCommand("unload p2p");
}

int wifi_direct_start_supplicant()
{
   return start_supplicant(&rDirectSuppPara, rDirectSuppInfo.acIfName);
}

int wifi_direct_stop_supplicant()
{
    return stop_supplicant(&rDirectSuppPara, &rDirectSuppInfo);
}

int wifi_direct_connect_to_supplicant()
{
    return connect_to_supplicant(&rDirectSuppPara, &rDirectSuppInfo);
}

void wifi_direct_close_supplicant_connection()
{
    close_supplicant_connection(&rDirectSuppPara, &rDirectSuppInfo);
}

int wifi_direct_command(const char *command, char *reply, size_t *reply_len)
{
    return send_command(&rDirectSuppInfo, command, reply, reply_len);
}

int wifi_direct_dhcp_client_request(int *ipaddr, int *gateway, int *mask,
                    int *dns1, int *dns2, int *server, int *lease) {
    return do_dhcp_client_request(rDirectSuppInfo.acIfName, ipaddr, gateway, mask, dns1, dns2, server, lease);
}

#if 0
const char *get_dhcp_error_string() {
    return dhcp_lasterror();
}
#endif

int wifi_direct_wait_for_event(char *buf, size_t buflen)
{
    return wait_for_event(&rDirectSuppInfo, buf, buflen);
}


/*----------------------------------------------------------------------------*/
/*!
 * \brief    This function is to start DHCP server
 *
 * \param[in]
 *
 * \return  0:          success
 *          nonzero:    failure
 */
/*----------------------------------------------------------------------------*/
#if 0
int wifi_direct_start_dhcp_server(int ipaddr_begin, int ipaddr_num, int gateway, int mask,
        int dns1, int dns2, int lease)
{
    FILE *fp;
    char dhcp_status[PROPERTY_VALUE_MAX] = {'\0'};

    int count = 200; /* wait at most 20 seconds for completion */
#ifdef HAVE_LIBC_SYSTEM_PROPERTIES
    const prop_info *pi;
    unsigned serial = 0;
#endif

    /* 1. DHCP pool parameter check */
    if(mask != 0 &&
            ((ipaddr_begin + ipaddr_num - 1) & mask) != (ipaddr_begin & mask)) {
        return -1;
    }
    /* 2. Check whether already running */
    else if (property_get(DHCPD_PROP_NAME, dhcp_status, NULL)
            && strcmp(dhcp_status, "running") == 0) {
        return 0;
    }

    // 3. prepare config.h
    if((fp = fopen(DHCPD_CONFIG_FILE, "w")) == NULL) {
        return -1;
    }

    // 4. prepare dhcp-range parameter
    fprintf(fp,
            "dhcp-range=%d.%d.%d.%d,%d.%d.%d.%d,%d.%d.%d.%d,%ds\n",
            (ipaddr_begin & 0xFF000000) >> 24,
            (ipaddr_begin & 0x00FF0000) >> 16,
            (ipaddr_begin & 0x0000FF00) >> 8,
            (ipaddr_begin & 0x000000FF),
            ((ipaddr_begin + ipaddr_num - 1) & 0xFF000000) >> 24,
            ((ipaddr_begin + ipaddr_num - 1) & 0x00FF0000) >> 16,
            ((ipaddr_begin + ipaddr_num - 1) & 0x0000FF00) >> 8,
            ((ipaddr_begin + ipaddr_num - 1) & 0x000000FF),
            (mask & 0xFF000000) >> 24,
            (mask & 0x00FF0000) >> 16,
            (mask & 0x0000FF00) >> 8,
            (mask & 0x000000FF),
            lease > 0 ? lease : 3600);

    // 5. prepare gateway (DHCP option #3)
    if(gateway != 0) {
        fprintf(fp,
                "dhcp-option=3,%d.%d.%d.%d\n",
                (gateway & 0xFF000000) >> 24,
                (gateway & 0x00FF0000) >> 16,
                (gateway & 0x0000FF00) >> 8,
                (gateway & 0x000000FF));
    }

    // 6. prepare nameserver (DHCP option #5)
    if(dns1 != 0) {
        if(dns2 != 0) {
            fprintf(fp,
                    "dhcp-option=5,%d.%d.%d.%d\n",
                    (dns1 & 0xFF000000) >> 24,
                    (dns1 & 0x00FF0000) >> 16,
                    (dns1 & 0x0000FF00) >> 8,
                    (dns1 & 0x000000FF));
        }
        else {
            fprintf(fp,
                    "dhcp-option=5,%d.%d.%d.%d,%d.%d.%d.%d\n",
                    (dns1 & 0xFF000000) >> 24,
                    (dns1 & 0x00FF0000) >> 16,
                    (dns1 & 0x0000FF00) >> 8,
                    (dns1 & 0x000000FF),
                    (dns2 & 0xFF000000) >> 24,
                    (dns2 & 0x00FF0000) >> 16,
                    (dns2 & 0x0000FF00) >> 8,
                    (dns2 & 0x000000FF));
        }
    }

    // 7. close file
    fclose(fp);

    // 8. start dnsmasq with system property
#ifdef HAVE_LIBC_SYSTEM_PROPERTIES
    /*
     * Get a reference to the status property, so we can distinguish
     * the case where it goes stopped => running => stopped (i.e.,
     * it start up, but fails right away) from the case in which
     * it starts in the stopped state and never manages to start
     * running at all.
     */
    pi = __system_property_find(DHCPD_PROP_NAME);
    if (pi != NULL) {
        serial = pi->serial;
    }
#endif

    property_set("ctl.start", DHCPD_NAME);
    sched_yield();

    while (count-- > 0) {
 #ifdef HAVE_LIBC_SYSTEM_PROPERTIES
        if (pi == NULL) {
            pi = __system_property_find(DHCPD_PROP_NAME);
        }
        if (pi != NULL) {
            __system_property_read(pi, NULL, dhcp_status);
            if (strcmp(dhcp_status, "running") == 0) {
                return 0;
            } else if (pi->serial != serial &&
                    strcmp(dhcp_status, "stopped") == 0) {
                return -1;
            }
        }
#else
        if (property_get(DHCPD_PROP_NAME, dhcp_status, NULL)) {
            if (strcmp(dhcp_status, "running") == 0)
                return 0;
        }
#endif
        usleep(100000);
    }
    return -1;
}

#endif

/*----------------------------------------------------------------------------*/
/*!
 * \brief    This function is to stop DHCP server
 *
 * \param[in] none
 *
 * \return  0:          success
 *          nonzero:    failure
 */
/*----------------------------------------------------------------------------*/

#if 0
int wifi_direct_stop_dhcp_server()
{
    char dhcp_status[PROPERTY_VALUE_MAX] = {'\0'};
    int count = 50; /* wait at most 5 seconds for completion */

    /* Check whether supplicant already stopped */
    if (property_get(DHCPD_PROP_NAME, dhcp_status, NULL)
        && strcmp(dhcp_status, "stopped") == 0) {
        return 0;
    }

    property_set("ctl.stop", DHCPD_NAME);
    sched_yield();

    while (count-- > 0) {
        if (property_get(DHCPD_PROP_NAME, dhcp_status, NULL)) {
            if (strcmp(dhcp_status, "stopped") == 0)
                return 0;
        }
        usleep(100000);
    }

    return -1;
}

#endif

/*----------------------------------------------------------------------------*/
/*!
 * \brief    This function is to start Wi-Fi Direct Internet-connection sharing
 *
 * \param[in] outgoing_interface
 *           pointer to string of outgoing interface (eth0/wlan0 ..)
 *
 * \return  0:          success
 *          nonzero:    failure
 */
/*----------------------------------------------------------------------------*/

#if 0
int wifi_direct_start_nat(char *outgoing_interface)
{
    FILE *fp;
    char cmd[256];

    // outgoing interface should not be NULL
    if(outgoing_interface == NULL) {
        return -1;
    }
    else if(strlen(outgoing_interface) >= (IFNAMSIZ-1)) {
        return -1;
    }
    else {
        strncpy(out_if, outgoing_interface, IFNAMSIZ-1);
    }

    // 1.
    fp = fopen("/proc/sys/net/ipv4/ip_forward", "r");
    if(fp == NULL) {
        return -1;
    }
    else {
        fputc('1', fp);
        fclose(fp);
    }

    // 2.
    snprintf(cmd,
            sizeof(cmd),
            "%s -t nat -A POSTROUTING -o %s -j MASQUERADE",
            LINUX_IPTABLES_BINARY,
            out_if);

    if(system(cmd)) {
        return -1;
    }

    // 3.
    snprintf(cmd,
            sizeof(cmd),
            "%s -A FORWARD -i %s -o %s -m state --state RELATED,ESTABLISHED -j ACCEPT",
            LINUX_IPTABLES_BINARY,
            iface,
            out_if);

    if(system(cmd)) {
        return -1;
    }

    // 4.
    snprintf(cmd,
            sizeof(cmd),
            "%s -A FORWARD -i %s -o %s -j ACCEPT",
            LINUX_IPTABLES_BINARY,
            iface,
            out_if);

    if(system(cmd)) {
        return -1;
    }

    return 0;
}

#endif

/*----------------------------------------------------------------------------*/
/*!
 * \brief    This function is to stop Wi-Fi Direct Internet-connection sharing
 *
 * \param[in] none
 *
 * \return  0:          success
 *          nonzero:    failure
 */
/*----------------------------------------------------------------------------*/

#if 0
int wifi_direct_stop_nat()
{
    char cmd[256];

    // not to restore ip_forward due to other modules may have enabled it as well
    // remove rules in reverse order without checking return value

    // 1.
    snprintf(cmd,
            sizeof(cmd),
            "%s -D FORWARD -i %s -o %s -j ACCEPT",
            LINUX_IPTABLES_BINARY,
            iface,
            out_if);
    system(cmd);

    // 2.
    snprintf(cmd,
            sizeof(cmd),
            "%s -D FORWARD -i %s -o %s -m state --state RELATED,ESTABLISHED -j ACCEPT",
            LINUX_IPTABLES_BINARY,
            iface,
            out_if);
    system(cmd);

    // 3.
    snprintf(cmd,
            sizeof(cmd),
            "%s -t nat -D POSTROUTING -o %s -j MASQUERADE",
            LINUX_IPTABLES_BINARY,
            out_if);
    system(cmd);

    return 0;
}
#endif


