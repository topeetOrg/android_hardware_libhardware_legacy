/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. and/or its licensors.
 * Without the prior written permission of MediaTek inc. and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
 *
 * MediaTek Inc. (C) 2010. All rights reserved.
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

#ifndef _WIFI_HOTSPOT_H
#define _WIFI_HOTSPOT_H

#if __cplusplus
extern "C" {
#endif

#include <stdint.h>

/**
 * Load the Wi-Fi Direct driver.
 *
 * @return 0 on success, < 0 on failure.
 */
int wifi_hotspot_load_driver();

/**
 * Unload the Wi-Fi driver.
 *
 * @return 0 on success, < 0 on failure.
 */
int wifi_hotspot_unload_driver();

/**
 * Start supplicant.
 *
 * @return 0 on success, < 0 on failure.
 */
int wifi_hotspot_start_supplicant();

/**
 * Stop supplicant.
 *
 * @return 0 on success, < 0 on failure.
 */
int wifi_hotspot_stop_supplicant();

/**
 * Open a connection to supplicant.
 *
 * @return 0 on success, < 0 on failure.
 */
int wifi_hotspot_connect_to_supplicant();

/**
 * Close connection supplicant.
 *
 * @return 0 on success, < 0 on failure.
 */
void wifi_hotspot_close_supplicant_connection();

/**
 * wifi_wait_for_event() performs a blocking call to
 * get a Wi-Fi event and returns a string representing
 * a Wi-Fi event when it occurs.
 *
 * @param buf is the buffer that receives the event
 * @param len is the maximum length of the buffer
 *
 * @returns number of bytes in buffer, 0 if no
 * event (for instance, no connection), and less than 0
 * if there is an error.
 */
int wifi_hotspot_wait_for_event(char *buf, size_t len);

/**
 * wifi_command() issues a command to the Wi-Fi driver.
 *
 * Android extends the standard commands listed at
 * /link http://hostap.epitest.fi/wpa_supplicant/devel/ctrl_iface_page.html
 * to include support for sending commands to the driver:
 *
 * <table border="2" cellspacing="2" cellpadding="2">
 *   <tr>
 *     <td><strong>Command / Command summary</strong></td>
 *     <td><strong>Form of Response</strong></td>
 *     <td><strong>Processing</strong></td>
 *   </tr>
 *   <tr>
 *     <td>DRIVER START<BR>&nbsp;&nbsp;Turn on Wi-Fi Hardware</td>
 *     <td>OK if successful</td>
 *     <td>OK ? true : false</td>
 *   </tr>
 *   <tr>
 *     <td>DRIVER STOP<BR>&nbsp;&nbsp;Turn off Wi-Fi hardware</td>
 *     <td>OK if successful</td>
 *     <td>OK ? true : false</td>
 *   </tr>
 *   <tr>
 *     <td>DRIVER RSSI<BR>&nbsp;&nbsp;Return received signal strength indicator in -db for current AP</td>
 *     <td>&lt;ssid&gt; Rssi xx</td>
 *     <td>%*s %*s %d", &rssi</td>
 *   </tr>
 *   <tr>
 *     <td>DRIVER LINKSPEED<BR>&nbsp;&nbsp;Return link speed in MBPS</td>
 *     <td>LinkSpeed xx</td>
 *     <td>%*s %d", &linkspd</td>
 *   </tr>
 *   <tr>
 *     <td>DRIVER MACADDR<BR>&nbsp;&nbsp;Return mac address of the station</td>
 *     <td>Macaddr = xx.xx.xx.xx.xx.xx</td>
 *     <td>"%*s = %s", &macadr</td>
 *   </tr>
 *   <tr>
 *     <td>DRIVER SCAN-ACTIVE<BR>&nbsp;&nbsp;Set scan type to active</td>
 *     <td>"OK" if successful</td>
 *     <td>"OK" ? true : false</td>
 *   </tr>
 *   <tr>
 *     <td>DRIVER SCAN-PASSIVE<BR>&nbsp;&nbsp;Set scan type to passive</td>
 *     <td>"OK" if successful</td>
 *     <td>"OK" ? true : false</td>
 *   </tr>
 * </table>
 *
 * See libs/android_runtime/android_net_wifi_Wifi.cpp for more information
 * describing how these and other commands are invoked.
 *
 * @param command is the string command
 * @param reply is a buffer to receive a reply string
 * @param reply_len on entry, this is the maximum length of
 *        the reply buffer. On exit, the number of
 *        bytes in the reply buffer.
 *
 * @return 0 if successful, < 0 if an error.
 */
int wifi_hotspot_command(const char *command, char *reply, size_t *reply_len);

int wifi_hotspot_set_iface(int on);


#if __cplusplus
};  // extern "C"
#endif

#endif  // _WIFI_HOTSPOT_H
