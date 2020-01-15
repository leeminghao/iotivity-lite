/****************************************************************************
 *
 * Copyright 2020 XiaoMi All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied. See the License for the specific
 * language governing permissions and limitations under the License.
 *
 ****************************************************************************/

#ifndef BLUETOOTH_ADAPTER_H
#define BLUETOOTH_ADAPTER_H

#include "ipcontext.h"
#include "port/oc_connectivity.h"

#define ATT_CID 4

#ifdef __cplusplus
extern "C"
{
#endif

int oc_bluetooth_connectivity_init(ip_context_t *dev);

void oc_get_bluetooth_address(ip_context_t *dev);

void oc_bluetooth_add_socks_to_fd_set(ip_context_t *dev);

adapter_receive_state_t
oc_bluetooth_receive_message(ip_context_t *dev, fd_set *fds, oc_message_t *message);

int oc_bluetooth_send_buffer(ip_context_t *dev, oc_message_t *message);

#ifdef __cplusplus
}
#endif

#endif /* BLUETOOTH_ADAPTER_H */
