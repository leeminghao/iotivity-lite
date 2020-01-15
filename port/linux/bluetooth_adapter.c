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

#include <errno.h>
#include <string.h>

#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#include "oc_config.h"
#include "oc_endpoint.h"
#include "port/oc_assert.h"
#include "util/oc_memb.h"
#include "bluetooth_adapter.h"

#include "lib/hci.h"
#include "lib/hci_lib.h"

OC_MEMB(device_eps, oc_endpoint_t, 2 * OC_MAX_NUM_DEVICES);

#define OC_L2CAP_LISTEN_BACKLOG 10
#define LIMIT_RETRY_CONNECT      5
#define L2CAP_CONNECT_TIMEOUT 5

static int
configure_l2cap_socket(int sock, struct sockaddr_l2 *addr)
{
  if (bind(sock, (struct sockaddr *)addr, sizeof (struct sockaddr_l2))) {
    OC_ERR("bind socket %d", errno);
    return -1;
  }
  if (listen(sock, OC_L2CAP_LISTEN_BACKLOG) < 0) {
    OC_ERR("listening socket %d", errno);
    return -1;
  }

  return 0;
}

static int
l2cap_le_connectivity_init(ip_context_t *dev)
{
  int sock;

  /* Set up source address */
  memset(&dev->bluetooth.server, 0, sizeof (struct sockaddr_l2));
  struct sockaddr_l2 *addr = (struct sockaddr_l2 *)&dev->bluetooth.server;
  addr->l2_family = AF_BLUETOOTH;
  addr->l2_cid = htobs(ATT_CID);
  addr->l2_bdaddr_type = BDADDR_LE_PUBLIC;
  bacpy(&addr->l2_bdaddr, BDADDR_ANY);

  sock = socket(PF_BLUETOOTH, SOCK_SEQPACKET, BTPROTO_L2CAP);
  if (sock < 0) {
    OC_ERR("failed to create L2CAP socket");
    return -1;
  }

  /* Set the security level */
  struct bt_security btsec;
  memset(&btsec, 0, sizeof (btsec));
#ifdef OC_SECURITY
  btsec.level = BT_SECURITY_HIGH;
#else
  btsec.level = BT_SECURITY_LOW;
#endif
  if (setsockopt(sock, SOL_BLUETOOTH, BT_SECURITY, &btsec, sizeof (btsec)) != 0) {
    OC_ERR("failed to set L2CAP security level");
    goto fail;
  }

  if (configure_l2cap_socket(sock, addr) < 0) {
    OC_ERR("set socket option in server socket");
    goto fail;
  }

  dev->bluetooth.server_sock = sock;

  OC_DBG("Successfully initialized Bluetooth adapter L2CAP for device %zd",
         dev->device);
  return 0;

fail:
  close(sock);
  return -1;
}

int oc_bluetooth_connectivity_init(ip_context_t *dev)
{
  OC_DBG("Initializing Bluetooth adapter for device %zd", dev->device);

  if (l2cap_le_connectivity_init(dev) != 0) {
    OC_ERR("could not initialize L2CAP for bluetooth");
  }

  OC_DBG("Successfully initialized Bluetooth adapter for device %zd", dev->device);

  return 0;
}

static int
dev_info(int s, int dev_id, long arg)
{
  ip_context_t *dev = (ip_context_t *)arg;
  oc_endpoint_t ep;
  struct hci_dev_info hdi = { .dev_id = dev_id };

  memset(&ep, 0, sizeof (oc_endpoint_t));
  ep.flags = GATT;
  ep.addr.bt.type = 0;
  if (ioctl(s, HCIGETDEVINFO, (void*)&hdi)) {
    OC_ERR("could not get hci dev info");
    return -1;
  }
  ba2str(&hdi.bdaddr, (char*)ep.addr.bt.address);

  OC_DBG("bluetooth mac address: %s", ep.addr.bt.address);

  oc_endpoint_t *new_ep = oc_memb_alloc(&device_eps);
  if (!new_ep) {
    OC_ERR("failed to oc_memb_alloc");
    return -1;
  }
  memcpy(new_ep, &ep, sizeof(oc_endpoint_t));
  oc_list_add(dev->eps, new_ep);

  return 0;
}

void oc_get_bluetooth_address(ip_context_t *dev)
{
  hci_for_each_dev(HCI_UP, dev_info, (long)dev);
}

void oc_bluetooth_add_socks_to_fd_set(ip_context_t *dev)
{
  FD_SET(dev->bluetooth.server_sock, &dev->rfds);
}

static int
accept_new_session(ip_context_t *dev, int fd, fd_set *setfds,
                   oc_endpoint_t *endpoint)
{
  int sock;
  struct sockaddr_l2 addr;
  socklen_t optlen;

  OC_DBG("Starting listening on ATT channel. Waiting for connections");

  memset(&addr, 0, sizeof (addr));
  optlen = sizeof (addr);
  sock = accept(fd, (struct sockaddr*)&addr, &optlen);
  if (sock < 0) {
    OC_ERR("failed to accept incoming L2CAP connection: %s", strerror(errno));
    return -1;
  }
  ba2str(&addr.l2_bdaddr, (char*)endpoint->addr.bt.address);

  OC_DBG("accepted incomming L2CAP connection: %s",
         (char*)endpoint->addr.bt.address);

  FD_CLR(fd, setfds);

  return 0;
}

adapter_receive_state_t oc_bluetooth_receive_message(
    ip_context_t *dev, fd_set *fds, oc_message_t *message)
{
#define ret_with_code(status)                   \
  ret = status;                                 \
  goto oc_bluetooth_receive_message_done

  adapter_receive_state_t ret = ADAPTER_STATUS_ERROR;
  message->endpoint.device = dev->device;

  if (FD_ISSET(dev->bluetooth.server_sock, fds)) {
    message->endpoint.flags = GATT;
    if (accept_new_session(dev, dev->bluetooth.server_sock, fds, &message->endpoint) < 0) {
      OC_ERR("failed to accept new session");
      ret_with_code(ADAPTER_STATUS_ERROR);
    }
    ret_with_code(ADAPTER_STATUS_ACCEPT);
  }

oc_bluetooth_receive_message_done:
  return ret;
#undef ret_with_code
}

static int
get_session_socket(oc_endpoint_t *endpoint)
{
  // TODO
  int sock = -1;
  return sock;
}

static int
connect_nonb(int sockfd, const struct sockaddr *r, int r_len, int nsec)
{
  int flags, n, error;
  socklen_t len;
  fd_set rset, wset;
  struct timeval tval;

  flags = fcntl(sockfd, F_GETFL, 0);
  if (flags < 0) {
    return -1;
  }

  error = fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);
  if (error < 0) {
    return -1;
  }

  error = 0;
  if ((n = connect(sockfd, (struct sockaddr *)r, r_len)) < 0) {
    if (errno != EINPROGRESS)
      return -1;
  }

  /* Do whatever we want while the connect is taking place. */
  if (n == 0) {
    goto done; /* connect completed immediately */
  }

  FD_ZERO(&rset);
  FD_SET(sockfd, &rset);
  wset = rset;
  tval.tv_sec = nsec;
  tval.tv_usec = 0;

  if ((n = select(sockfd + 1, &rset, &wset, NULL, nsec ? &tval : NULL)) == 0) {
    /* timeout */
    errno = ETIMEDOUT;
    return -1;
  }

  if (FD_ISSET(sockfd, &rset) || FD_ISSET(sockfd, &wset)) {
    len = sizeof(error);
    if (getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &error, &len) < 0)
      return -1; /* Solaris pending error */
  } else {
    OC_DBG("select error: sockfd not set");
    return -1;
  }

done:
  if (error < 0) {
    close(sockfd); /* just in case */
    errno = error;
    return -1;
  } else {
    error = fcntl(sockfd, F_SETFL, flags); /* restore file status flags */
    if (error < 0) {
      return -1;
    }
  }
  return 0;
}

static int
initiate_new_session(ip_context_t *dev, oc_endpoint_t *endpoint)
{
  int sock = -1;
  uint8_t retry_count = 0;
  struct sockaddr_l2 srcaddr, dstaddr;
  struct bt_security btsec;

  while (retry_count < LIMIT_RETRY_CONNECT) {
    if (endpoint->flags & GATT) {
      sock = socket(PF_BLUETOOTH, SOCK_SEQPACKET, BTPROTO_L2CAP);
      if (sock < 0) {
        OC_ERR("could not create socket for new L2CAP session");
        return -1;
      }
    }

    /* Set up s ource address */
    memset(&srcaddr, 0, sizeof (srcaddr));
    srcaddr.l2_family = AF_BLUETOOTH;
    srcaddr.l2_cid = htobs(ATT_CID);
    srcaddr.l2_bdaddr_type = 0;
    bacpy(&srcaddr.l2_bdaddr, BDADDR_ANY);

    if (bind(sock, (struct sockaddr *)&srcaddr, sizeof (srcaddr)) < 0) {
      OC_ERR("failed to bind L2CAP socket");
      goto fail;
    }

    /* Set the security level */
    memset(&btsec, 0, sizeof (btsec));
#ifdef OC_SECURITY
    btsec.level = BT_SECURITY_HIGH;
#else
    btsec.level = BT_SECURITY_LOW;
#endif
    if (setsockopt(sock, SOL_BLUETOOTH, BT_SECURITY, &btsec, sizeof (btsec)) < 0) {
      OC_ERR("failed to set L2CAP security level");
      goto fail;
    }

    /* Set up destination address */
    memset(&dstaddr, 0, sizeof (dstaddr));
    dstaddr.l2_family = AF_BLUETOOTH;
    dstaddr.l2_cid = htobs(ATT_CID);
    dstaddr.l2_bdaddr_type = BDADDR_LE_PUBLIC;
    str2ba((char*)endpoint->addr.bt.address, &dstaddr.l2_bdaddr);

    int ret = connect_nonb(sock, (struct sockaddr *)&dstaddr, sizeof (dstaddr),
                           L2CAP_CONNECT_TIMEOUT);
    if (ret == 0) {
      break;
    }

    close(sock);
    retry_count++;
    OC_DBG("connect fail with %d. retry(%d)", ret, retry_count);
  }

  if (retry_count >= LIMIT_RETRY_CONNECT) {
    OC_ERR("could not initiate L2CAP connection");
    return -1;
  }

  OC_DBG("successfully initiated L2CAP connection");
  FD_SET(sock, &dev->rfds);

  return sock;

fail:
  close(sock);
  return -1;
}

int oc_bluetooth_send_buffer(ip_context_t *dev, oc_message_t *message)
{
  OC_DBG("bluetooth send buffer");

  int send_sock = get_session_socket(&message->endpoint);
  size_t bytes_sent = 0;
  if (send_sock < 0) {
    send_sock = initiate_new_session(dev, &message->endpoint);
    if (send_sock < 0) {
      OC_ERR("could not initialte new L2CAP session");
      goto oc_bluetooth_send_buffer_done;
    }
  }

oc_bluetooth_send_buffer_done:
  if (bytes_sent == 0) {
    return -1;
  }

  return bytes_sent;
}
