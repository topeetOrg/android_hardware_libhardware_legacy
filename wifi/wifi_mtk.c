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
 *
 * The following software/firmware and/or related documentation ("MediaTek Software")
 * have been modified by MediaTek Inc. All revisions are subject to any receiver's
 * applicable license agreements with MediaTek Inc.
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

#define LOAD_WIFI_MODULE_ONCE
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

#include <net/if_arp.h>		    /* For ARPHRD_ETHER */
#include <sys/socket.h>		    /* For AF_INET & struct sockaddr */
#include <netinet/in.h>         /* For struct sockaddr_in */
#include <netinet/if_ether.h>
#include <linux/wireless.h>

#include "hardware_legacy/wifi.h"
#include "libwpa_client/wpa_ctrl.h"
#include "hardware_legacy/wifi_common.h"
#include "hardware_legacy/wifi_direct.h"


#define LOG_TAG "WifiHW"
#include "cutils/log.h"
#include "cutils/memory.h"
#include "cutils/misc.h"
#include "cutils/properties.h"
#include "private/android_filesystem_config.h"
#ifdef HAVE_LIBC_SYSTEM_PROPERTIES
#define _REALLY_INCLUDE_SYS__SYSTEM_PROPERTIES_H_
#include <sys/_system_properties.h>
#endif

extern char *dhcp_lasterror();
extern const SUPPLICANT_PARA_T rDirectSuppPara;
extern SUPPLICANT_INFO_T rDirectSuppInfo;

static const SUPPLICANT_PARA_T rWifiSuppPara = {
    //"sta",
    "wlan0",
    "wifi.interface",
    "/data/misc/wifi/wpa_supplicant",
    "wpa_supplicant",
    "init.svc.wpa_supplicant",
    "/system/etc/wifi/wpa_supplicant.conf",
    "/data/misc/wifi/wpa_supplicant.conf",
    "%s:-Dwext -i%s -c%s -dd",
    "wpa_ctrl_"
};

static SUPPLICANT_INFO_T rWifiSuppInfo = {
    "wlan0",                    //Interface name default value
    NULL,                       //Ctrl connection
    NULL,                       //Monitor connection
    {                           //Ctrl API
        wpa_ctrl_open,
        wpa_ctrl_close,
        wpa_ctrl_request,
        wpa_ctrl_recv,
        wpa_ctrl_attach
    },
    {                           //exit socket pair
        -1,
        -1
    }
};

static SUPPLICANT_PARA_T const *prSuppPara = &rWifiSuppPara;
static P_SUPPLICANT_INFO_T prSuppInfo = &rWifiSuppInfo;

int do_dhcp_request(int *ipaddr, int *gateway, int *mask,
                    int *dns1, int *dns2, int *server, int *lease) {
    return do_dhcp_client_request(prSuppInfo->acIfName, ipaddr, gateway, mask, dns1, dns2, server, lease);
}

const char *get_dhcp_error_string() {
    return dhcp_lasterror();
}

/*
* driver status transition
*
* [unloaded] => [ok] => [running] => [ok] => ...
*
* status detail
* [unloaded]:   driver module is not instered
* [ok]:         driver module is instered, but rfkill=0, card is not detected and driver is not ready to work
* [running]:    driver module is instered, rfkill=1, card is detected and driver is ready to work
* [error]:      load/unload driver failed
*/

int wifi_load_driver()
{
    property_get(
        rWifiSuppPara.acIfPropName,
        rWifiSuppInfo.acIfName,
        rWifiSuppPara.acIfDefName);

    if(halDoCommand("load wifi") == 0) {
        property_set("wlan.driver.status", "running");
        return 0;
    } else {
        property_set("wlan.driver.status", "error");
        return -1;
    }
}

int wifi_unload_driver()
{
    if (halDoCommand("unload wifi") == 0) {
        property_set("wlan.driver.status", "ok");
        return 0;
    } else {
        property_set("wlan.driver.status", "error");
        return -1;
    }
}

int wifi_start_supplicant()
{
#ifndef CFG_SUPPORT_CONCURRENT_NETWORK
    prSuppPara = &rWifiSuppPara;
    prSuppInfo = &rWifiSuppInfo;
#endif
    return start_supplicant(&rWifiSuppPara, rWifiSuppInfo.acIfName);
}

int wifi_start_p2p_supplicant()
{
#ifndef CFG_SUPPORT_CONCURRENT_NETWORK
    prSuppPara = &rDirectSuppPara;
    prSuppInfo = &rDirectSuppInfo;

    if(wifi_direct_load_driver()!= 0) {
        return -1;
    }

#endif
    return start_supplicant(&rDirectSuppPara, rDirectSuppInfo.acIfName);
}

int wifi_stop_supplicant()
{
    return stop_supplicant(prSuppPara, prSuppInfo);
}

int wifi_connect_to_supplicant()
{
    return connect_to_supplicant(prSuppPara, prSuppInfo);
}

int wifi_wait_for_event(char *buf, size_t buflen)
{
    return wait_for_event(prSuppInfo, buf, buflen);
}

void wifi_close_supplicant_connection()
{
    close_supplicant_connection(prSuppPara, prSuppInfo);

#ifndef CFG_SUPPORT_CONCURRENT_NETWORK
    if(prSuppPara == &rDirectSuppPara) {
        wifi_direct_unload_driver();
    }
#endif
}

/*Parameter should be synced with the mediatek/source/external/hald/DriverCtrl.cpp */
int is_wifi_driver_loaded()
{
    return is_driver_loaded("wlan", "wlan.driver.status");
}

int wifi_command(const char *command, char *reply, size_t *reply_len)
{
    return send_command(prSuppInfo, command, reply, reply_len);
}

/*Not in use now*/
const char *wifi_get_fw_path(int fw_type)
{
    return "NOT_IN_USE";
}

/*Not in use now*/
int wifi_change_fw_path(const char *fwpath)
{
    return 0;
}
