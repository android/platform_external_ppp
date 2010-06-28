

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
#include <cutils/properties.h>
#include "vpn-routes.h"

static int get_default_gateway(struct in_addr* in_gateway)
{
    FILE *f = 0;
    int ret = 0;
    if((f = fopen("/proc/net/route", "r"))!= NULL){
        char device[64];
        uint32_t address;
        uint32_t gateway;
        uint32_t netmask;
        fscanf(f, "%*[^\n]\n");
        while (fscanf(f, "%63s%X%X%*X%*d%*u%*d%X%*d%*u%*u\n", device, &address,
                &gateway, &netmask) == 4) {
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
    struct rtentry route = {
        .rt_dst     = {.sa_family = AF_INET},
        .rt_genmask = {.sa_family = AF_INET},
        .rt_gateway = {.sa_family = AF_INET},
        .rt_flags   = RTF_UP | RTF_GATEWAY,
    };
    FILE *f = 0;
    int s = -1;
    int ppp_routes_count = 0;
    int exclude_routes_count = 0;
    int routes_count = 0;
    struct rtentry *route_entries = 0;

    if(argc > 6) {
        struct in_addr ppp_gateway;
        struct in_addr default_gateway;
        if(get_default_gateway(&default_gateway) &&
                inet_aton(argv[5], &ppp_gateway)) {
            create_route_entries(argv[6], &ppp_gateway, &default_gateway,
                    &route_entries, &ppp_routes_count, &exclude_routes_count);
            routes_count = ppp_routes_count + exclude_routes_count;
        }
    }

    errno = EINVAL;
    if (argc > 5 && inet_aton(argv[5], in_addr(&route.rt_gateway)) &&
            (s = socket(AF_INET, SOCK_DGRAM, 0)) != -1 &&
            (f = fopen("/proc/net/route", "r")) != NULL &&
            0 == ppp_routes_count) {
        /* No split tunneling provided, redirect all traffic to VPN*/
        uint32_t *address = &in_addr(&route.rt_dst)->s_addr;
        uint32_t *netmask = &in_addr(&route.rt_genmask)->s_addr;
        char device[64];

        fscanf(f, "%*[^\n]\n");
        while (fscanf(f, "%63s%X%*X%*X%*d%*u%*d%X%*d%*u%*u\n",
                device, address, netmask) == 3) {
            if (strcmp(argv[1], device)) {
                uint32_t bit = ntohl(*netmask);
                bit = htonl(bit ^ (1 << 31 | bit >> 1));
                if (bit) {
                    *netmask |= bit;

                    if (ioctl(s, SIOCADDRT, &route) == -1 && errno != EEXIST) {
                        break;
                    }

                    *address ^= bit;

                    if (ioctl(s, SIOCADDRT, &route) == -1 && errno != EEXIST) {
                        break;
                    }
                   errno = 0;
                }
            }
        }
    }

    // Additional split tunneling configuration provided
    if(routes_count && route_entries && (s != -1)) {
        int i = 0;
        for(i = 0; i < routes_count; i++) {
            if (ioctl(s, SIOCADDRT, &route_entries[i]) == -1 && errno != EEXIST) {
                __android_log_print(ANDROID_LOG_ERROR, "ip-up-vpn",
                        "Failed to add split tunneling route: errno=%d", errno);
               break;
            }
            errno = 0;
        }
    }

    if (!errno) {
        char *dns = getenv("DNS1");
        property_set("vpn.dns1", dns ? dns : "");
        dns = getenv("DNS2");
        property_set("vpn.dns2", dns ? dns : "");
        property_set("vpn.status", "ok");
        __android_log_print(ANDROID_LOG_INFO, "ip-up-vpn",
                "Traffic is now redirected to %s", argv[5]);
    } else {
        property_set("vpn.status", "error");
        __android_log_write(ANDROID_LOG_ERROR, "ip-up-vpn", strerror(errno));
    }

    if(route_entries)
        free(route_entries);

    if(f)
        fclose(f);

    if(s != -1)
        close(s);

    return errno;
}
