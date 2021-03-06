/**
 * Copyright (C) 2015 Dato, Inc.
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license. See the LICENSE file for details.
 */
#include <stdint.h>
#include <nanosockets/zmq_msg_vector.hpp>
#include <nanosockets/print_zmq_error.hpp>
#include <serialization/oarchive.hpp>
#include <serialization/iarchive.hpp>

extern "C" {
#include <nanomsg/nn.h>
}

namespace graphlab {
namespace nanosockets {


int zmq_msg_vector::send(int socket, int timeout) {
  // check EINTR and retry
  int rc = 0;
  do {
    rc = send_impl(socket, timeout);
  } while (rc == EINTR);
  return rc;
}

int zmq_msg_vector::send(int socket) {
  // check EINTR and retry
  int rc = 0;
  do {
    rc = send_impl(socket);
  } while (rc == EINTR);
  return rc;
}

int zmq_msg_vector::recv(int socket, int timeout) {
  // check EINTR and retry
  int rc = 0;
  do {
    rc = recv_impl(socket, timeout);
  } while (rc == EINTR);
  return rc;
}

int zmq_msg_vector::recv(int socket) {
  // check EINTR and retry
  int rc = 0;
  do {
    rc = recv_impl(socket);
  } while (rc == EINTR);
  return rc;
}




int zmq_msg_vector::send_impl(int socket, int timeout) {
  // wait until I can send
  if (timeout > 0) {
    nn_pollfd pollitem;
    pollitem.fd = socket;
    pollitem.revents = 0;
    pollitem.events = NN_POLLOUT;

    int rc = nn_poll(&pollitem, 1, timeout);

    if (rc == -1) return nn_errno();
    else if (rc == 0) return EAGAIN;
  }
  std::string res;
  graphlab::oarchive oarc;
  oarc << msgs;
  int rc = nn_send(socket, oarc.buf, oarc.off, 0);
  if (oarc.buf) free(oarc.buf);
  if (rc == -1) {
    print_zmq_error("zmq_msg_vector Unexpected error in send");
    return nn_errno();
  }
  return 0;
}


int zmq_msg_vector::send_impl(int socket) {
  return send_impl(socket, -1);
}


int zmq_msg_vector::recv_impl(int socket, int timeout) {
  if (timeout > 0) {
    nn_pollfd pollitem;
    pollitem.fd = socket;
    pollitem.revents = 0;
    pollitem.events = NN_POLLIN;

    int rc = nn_poll(&pollitem, 1, timeout);

    if (rc == -1) return nn_errno();
    else if (rc == 0) return EAGAIN;
  }
  char* buf = NULL; 
  size_t len = NN_MSG;
  int rc = 0;
  while(1) {
    rc = nn_recv(socket, reinterpret_cast<void*>(&buf), len, 0);
    if (rc < 0) {
      if (nn_errno() == ETIMEDOUT || nn_errno() == EAGAIN || nn_errno() == EINTR) {
        continue;
      }
      if (buf) nn_freemsg(buf);
      print_zmq_error("zmq_msg_vector Unexpected error in recv");
      return nn_errno();
    } else if (rc >= 0) {
      break;
    }
  }
  // success
  graphlab::iarchive iarc(buf, rc);
  msgs.clear();
  iarc >> msgs;
  if (buf) nn_freemsg(buf);
  return 0;
}

int zmq_msg_vector::recv_impl(int socket) {
  return recv_impl(socket, -1);
}

}
}
