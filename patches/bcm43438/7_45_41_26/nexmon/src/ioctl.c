/***************************************************************************
 *                                                                         *
 *          ###########   ###########   ##########    ##########           *
 *         ############  ############  ############  ############          *
 *         ##            ##            ##   ##   ##  ##        ##          *
 *         ##            ##            ##   ##   ##  ##        ##          *
 *         ###########   ####  ######  ##   ##   ##  ##    ######          *
 *          ###########  ####  #       ##   ##   ##  ##    #    #          *
 *                   ##  ##    ######  ##   ##   ##  ##    #    #          *
 *                   ##  ##    #       ##   ##   ##  ##    #    #          *
 *         ############  ##### ######  ##   ##   ##  ##### ######          *
 *         ###########    ###########  ##   ##   ##   ##########           *
 *                                                                         *
 *            S E C U R E   M O B I L E   N E T W O R K I N G              *
 *                                                                         *
 * This file is part of NexMon.                                            *
 *                                                                         *
 * Copyright (c) 2016 NexMon Team                                          *
 *                                                                         *
 * NexMon is free software: you can redistribute it and/or modify          *
 * it under the terms of the GNU General Public License as published by    *
 * the Free Software Foundation, either version 3 of the License, or       *
 * (at your option) any later version.                                     *
 *                                                                         *
 * NexMon is distributed in the hope that it will be useful,               *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of          *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 * GNU General Public License for more details.                            *
 *                                                                         *
 * You should have received a copy of the GNU General Public License       *
 * along with NexMon. If not, see <http://www.gnu.org/licenses/>.          *
 *                                                                         *
 **************************************************************************/

#pragma NEXMON targetregion "patch"

#include <firmware_version.h>   // definition of firmware version macros
#include <debug.h>              // contains macros to access the debug hardware
#include <wrapper.h>            // wrapper definitions for functions that already exist in the firmware
#include <structs.h>            // structures that are used by the code in the firmware
#include <helper.h>             // useful helper functions
#include <patcher.h>            // macros used to craete patches such as BLPatch, BPatch, ...
#include <rates.h>              // rates used to build the ratespec for frame injection
#include <nexioctls.h>          // ioctls added in the nexmon patch
#include <capabilities.h>       // capabilities included in a nexmon patch
#include <sendframe.h>          // sendframe functionality

struct beacon {
  char dummy[40];
  char ssid_len;
  char ssid[32];
} beacon = {
  .dummy = { 
          0x80, 0x00, 0x00, 0x00, 0xff,
          0xff, 0xff, 0xff, 0xff, 0xff,
          0x00, 0x11, 0x22, 0x33, 0x44,
          0x55, 0x00, 0x11, 0x22, 0x33,
          0x44, 0x55, 0x00, 0x00, 0x00,
          0x00, 0x00, 0x00, 0x00, 0x00,
          0x00, 0x00, 0x00, 0x00, 0x00,
          0x00, 0x01, 0x01, 0x82, 0x00
  },
  .ssid_len = 5,
  .ssid = { 'H', 'E', 'L', 'L', 'O' }
};


void send_beacon(struct wlc_info *wlc)
{
  int len = sizeof(beacon) - 32 + beacon.ssid_len;
  sk_buff *p = pkt_buf_get_skb(wlc->osh, len + 202);
  struct beacon *beacon_skb;
  beacon_skb = (struct beacon *)skb_pull(p, 202);
  memcpy(beacon_skb, &beacon, len);
  sendframe(wlc, p, 1, 0);
}



struct hndrte_timer *t = 0;
void timer_handler(
                struct hndrte_timer * t)
{
        memcpy(beacon.ssid, "HELLO", 5);
        send_beacon(t->data);
        memcpy(beacon.ssid, "WORLD", 5);
        send_beacon(t->data);
}
//void timer_jam_handler(
//                struct hndrte_timer * t)
//{
//  struct wlc_info* wlc = t->data;
//  int len = sizeof(dummy_frame);
//  sk_buff *p = pkt_buf_get_skb(wlc->osh, len + 202);
//  char *beacon_skb;
//  beacon_skb = (char*)skb_pull(p, 202);
//  memcpy(beacon_skb, &dummy_frame, len);
//  sendframe(wlc, p, 1, 0);
//}

int handle_ct_ioctl(struct wlc_info* wlc, char* arg, int len) 
{
  if (!t) {
    t = hndrte_init_timer(0, wlc, timer_handler, 0);
    hndrte_add_timer(t, 100, 1);
  }
  return IOCTL_SUCCESS;
}

//int handle_jam_ioctl(struct wlc_info* wlc, char* arg, int len) 
//{
//  if (!t) {
//    t = hndrte_init_timer(0, wlc, timer_jam_handler, 0);
//    hndrte_add_timer(t, 20, 1);
//  }
//  return IOCTL_SUCCESS;
//}

int 
wlc_ioctl_hook(struct wlc_info *wlc, int cmd, char *arg, int len, void *wlc_if)
{
    int ret = IOCTL_ERROR;

    switch (cmd) {
        case NEX_GET_CAPABILITIES:
            if (len == 4) {
                memcpy(arg, &capabilities, 4);
                ret = IOCTL_SUCCESS;
            }
            break;
        case NEX_CT_EXPERIMENTS:
            ret = handle_ct_ioctl(wlc, arg, len);
            break;
        case NEX_WRITE_TO_CONSOLE:
            if (len > 0) {
                arg[len-1] = 0;
                printf("ioctl: %s\n", arg);
                ret = IOCTL_SUCCESS; 
            }
            break;

        default:
            ret = wlc_ioctl(wlc, cmd, arg, len, wlc_if);
    }

    return ret;
}

__attribute__((at(0x42924, "", CHIP_VER_BCM43438, FW_VER_7_45_41_26_r640327)))
GenericPatch4(wlc_ioctl_hook, wlc_ioctl_hook + 1);

