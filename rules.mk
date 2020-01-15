#
# Copyright (C) 2020 The Miot Connecitity Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

GLOBAL_INCLUDES += $(LOCAL_DIR)

MODULE_SRCS := \
  $(LOCAL_DIR)/api/oc_base64.c \
  $(LOCAL_DIR)/api/oc_blockwise.c \
  $(LOCAL_DIR)/api/oc_buffer.c \
  $(LOCAL_DIR)/api/oc_client_api.c \
  $(LOCAL_DIR)/api/oc_clock.c \
  $(LOCAL_DIR)/api/oc_collection.c \
  $(LOCAL_DIR)/api/oc_core_res.c \
  $(LOCAL_DIR)/api/oc_discovery.c \
  $(LOCAL_DIR)/api/oc_endpoint.c \
  $(LOCAL_DIR)/api/oc_helpers.c \
  $(LOCAL_DIR)/api/oc_introspection.c \
  $(LOCAL_DIR)/api/oc_main.c \
  $(LOCAL_DIR)/api/oc_mnt.c \
  $(LOCAL_DIR)/api/oc_network_events.c \
  $(LOCAL_DIR)/api/oc_rep.c \
  $(LOCAL_DIR)/api/oc_resource_factory.c \
  $(LOCAL_DIR)/api/oc_ri.c \
  $(LOCAL_DIR)/api/oc_server_api.c \
  $(LOCAL_DIR)/api/oc_session_events.c \
  $(LOCAL_DIR)/api/oc_swupdate.c \
  $(LOCAL_DIR)/api/oc_uuid.c \
  $(LOCAL_DIR)/api/c-timestamp/timestamp_compare.c \
  $(LOCAL_DIR)/api/c-timestamp/timestamp_format.c \
  $(LOCAL_DIR)/api/c-timestamp/timestamp_parse.c \
  $(LOCAL_DIR)/api/c-timestamp/timestamp_tm.c \
  $(LOCAL_DIR)/api/c-timestamp/timestamp_valid.c \
  $(LOCAL_DIR)/util/oc_etimer.c \
  $(LOCAL_DIR)/util/oc_list.c \
  $(LOCAL_DIR)/util/oc_mem_trace.c \
  $(LOCAL_DIR)/util/oc_memb.c \
  $(LOCAL_DIR)/util/oc_mmem.c \
  $(LOCAL_DIR)/util/oc_process.c \
  $(LOCAL_DIR)/util/oc_timer.c \
  $(LOCAL_DIR)/messaging/coap/coap.c \
  $(LOCAL_DIR)/messaging/coap/coap_signal.c \
  $(LOCAL_DIR)/messaging/coap/engine.c \
  $(LOCAL_DIR)/messaging/coap/observe.c \
  $(LOCAL_DIR)/messaging/coap/separate.c \
  $(LOCAL_DIR)/messaging/coap/transactions.c

MODULE_CFLAGS := \
  -DOC_CLIENT -DOC_SERVER \
  -DOC_IPV4 -DOC_TCP \
  -DOC_BLUETOOTH

MODULE_DEPS += \
  third_party/lib/iotivity-lite/deps/tinycbor

include $(BUILD_SYSTEM)/module.mk
