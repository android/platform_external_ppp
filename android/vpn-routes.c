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
#include <netinet/in.h>
#include <arpa/inet.h>
#include <linux/route.h>

#include <android/log.h>
#include "vpn-routes.h"

static inline void route_entry(struct rtentry *entry, struct in_addr* dest,
        struct in_addr* mask, struct in_addr* gateway)
{
     entry->rt_dst.sa_family = AF_INET;
     entry->rt_genmask.sa_family = AF_INET;
     entry->rt_gateway.sa_family = AF_INET;
     entry->rt_flags = RTF_UP | RTF_GATEWAY;
     memcpy(in_addr(&entry->rt_dst), dest, sizeof(struct in_addr));
     memcpy(in_addr(&entry->rt_genmask), mask, sizeof(struct in_addr));
     memcpy(in_addr(&entry->rt_gateway), gateway, sizeof(struct in_addr));
}

static int parse_route_entries(char* routes, struct in_addr* gateway,
        struct rtentry* entries)
{
    struct in_addr in_dest;
    struct in_addr in_mask;
    int count = 0;

    if(!routes)
        return 0;

    char* tok = strtok(routes, ",");
    while(tok) {
        char* mask = strchr(tok, '/');
        if(mask) {
            *(mask++) = 0;
            if(inet_aton(tok, &in_dest) && inet_aton(mask, &in_mask)) {
                route_entry(entries + count, &in_dest, &in_mask, gateway);
                count++;
            }
        }
        tok = strtok(0, ",");
    }
    return count;
}


int create_route_entries(char* routes, struct in_addr* ppp_gateway,
        struct in_addr* default_gateway, struct rtentry** route_entries,
        int* ppp_routes_count, int* exclude_routes_count)
{
    char* exclude_routes = 0;
    int   routes_count = 0;
    char  *pch = 0;

    if(!routes)
        return 0;

    exclude_routes = strchr(routes, ';');
    if(exclude_routes)
        *(exclude_routes++) = 0;

    /*Calculate number of rtentry structures to be populated*/
    pch = strchr(routes, ',');
    if(*routes) routes_count++;
    while(pch){
        routes_count++;
        pch = strchr(pch + 1, ',');
    }

    if(exclude_routes){
        if(*exclude_routes) routes_count++;
        pch = strchr(exclude_routes, ',');
        while(pch) {
            routes_count++;
            pch = strchr(pch + 1, ',');
        }
    }

    *route_entries = 0;
    if(!routes_count)
        return routes_count;

    *route_entries = (struct rtentry*) calloc(routes_count, sizeof(struct rtentry));
    if(0 == *route_entries)
        return 0;

    *ppp_routes_count = parse_route_entries(routes, ppp_gateway, *route_entries);
    *exclude_routes_count = parse_route_entries(exclude_routes, default_gateway,
            *route_entries + *ppp_routes_count);

    return *ppp_routes_count + *exclude_routes_count;
}

