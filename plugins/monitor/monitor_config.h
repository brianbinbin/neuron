/**
 * NEURON IIoT System for Industry 4.0
 * Copyright (C) 2020-2022 EMQ Technologies Co., Ltd All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 **/

#ifndef NEURON_PLUGIN_MONITOR_CONFIG_H
#define NEURON_PLUGIN_MONITOR_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdlib.h>

#include "plugin.h"

typedef struct {
    char *   client_id;          // client id
    char *   event_topic_prefix; // event topic prefix
    uint64_t heartbeat_interval; // heartbeat interval
    char *   heartbeat_topic;    // heartbeat topic
    char *   host;               // broker host
    uint16_t port;               // broker port
    char *   username;           // user name
    char *   password;           // user password
    char *   ca;                 // CA
    char *   cert;               // client cert
    char *   key;                // client key
    char *   keypass;            // client key password
} monitor_config_t;

int  monitor_config_parse(neu_plugin_t *plugin, const char *setting,
                          monitor_config_t *config);
void monitor_config_fini(monitor_config_t *config);

#ifdef __cplusplus
}
#endif

#endif
