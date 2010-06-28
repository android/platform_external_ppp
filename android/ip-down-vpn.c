

/*
 * Copyright (C) 2009 The Android Open Source Project
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
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <linux/route.h>

#include <android/log.h>
#include "vpn-routes.h"

static int get_default_gateway(struct in_addr* in_ppp_gateway, struct in_addr* in_gateway)
{
    FILE *f;
    int ret = 0;
    if((f = fopen("/proc/net/route", "r")) != NULL){
        char device[64];
        uint32_t address;
        uint32_t gateway;
        uint32_t netmask;
        fscanf(f, "%*[^\n]\n");
        while (fscanf(f, "%63s%X%X%*X%*d%*u%*d%X%*d%*u%*u\n", device, &address, &gateway, &netmask) == 4) {
            if(gateway == in_ppp_gateway->s_addr)
               continue;
            if(0 == (address & netmask)) {
                in_gateway->s_addr = gateway;
                ret = 1;
                break;
            }
        }
        fclose(f);
    }
    return ret;
}

int main(int argc, char **argv)
{
    int s = -1;
    int ppp_routes_count = 0;
    int exclude_routes_count = 0;
    int routes_count = 0;
    struct rtentry *route_entries = 0;

    if(argc > 6){
        struct in_addr ppp_gateway;
        struct in_addr default_gateway;

    if(inet_aton(argv[5], &ppp_gateway) && get_default_gateway(&ppp_gateway, &default_gateway)){
            create_route_entries(argv[6], &ppp_gateway, &default_gateway, &route_entries, &ppp_routes_count, &exclude_routes_count);
            routes_count = ppp_routes_count + exclude_routes_count;
        }
    }

    /* Additional split tunneling configuration provided
     * Removing routes to subnets excluded from VPN
     */
    if(routes_count && route_entries &&
      (s = socket(AF_INET, SOCK_DGRAM, 0)) != -1){
        int i;
        for(i = ppp_routes_count; i < routes_count; i++){
            if (ioctl(s, SIOCDELRT, &route_entries[i]) == -1){
               __android_log_print(ANDROID_LOG_ERROR, "ip-down-vpn",
                        "Failed to delete split tunneling route: errno=%d", errno);
            }
        }
    }

    if(route_entries)
       free(route_entries);

    if(s != -1)
       close(s);

    return 0;
}
