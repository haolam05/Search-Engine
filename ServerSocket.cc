/*
 * Copyright ©2023 Justin Hsia.  All rights reserved.  Permission is
 * hereby granted to students registered for University of Washington
 * CSE 333 for use solely during Winter Quarter 2023 for purposes of
 * the course.  No other use, copying, distribution, or modification
 * is permitted without prior written consent. Copyrights for
 * third-party components of this work must be honored.  Instructors
 * interested in reusing these course materials should contact the
 * author.
 */

#include <stdio.h>       // for snprintf()
#include <unistd.h>      // for close(), fcntl()
#include <sys/types.h>   // for socket(), getaddrinfo(), etc.
#include <sys/socket.h>  // for socket(), getaddrinfo(), etc.
#include <arpa/inet.h>   // for inet_ntop()
#include <netdb.h>       // for getaddrinfo()
#include <errno.h>       // for errno, used by strerror()
#include <string.h>      // for memset, strerror()
#include <iostream>      // for std::cerr, etc.

#include "./ServerSocket.h"

extern "C" {
  #include "libhw1/CSE333.h"
}

namespace hw4 {

ServerSocket::ServerSocket(uint16_t port) {
  port_ = port;
  listen_sock_fd_ = -1;
}

ServerSocket::~ServerSocket() {
  // Close the listening socket if it's not zero.  The rest of this
  // class will make sure to zero out the socket if it is closed
  // elsewhere.
  if (listen_sock_fd_ != -1)
    close(listen_sock_fd_);
  listen_sock_fd_ = -1;
}

bool ServerSocket::BindAndListen(int ai_family, int* const listen_fd) {
  // Use "getaddrinfo," "socket," "bind," and "listen" to
  // create a listening socket on port port_.  Return the
  // listening socket through the output parameter "listen_fd"
  // and set the ServerSocket data member "listen_sock_fd_"

  // STEP 1:
  // Populate the "hints" addrinfo structure for getaddrinfo().
  // ("man addrinfo")
  sock_family_ = ai_family;
  struct addrinfo hints;
  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_INET6;       // IPv6 (also handles IPv4 clients)
  hints.ai_socktype = SOCK_STREAM;  // stream
  hints.ai_flags = AI_PASSIVE;      // use wildcard "in6addr_any" address
  hints.ai_flags |= AI_V4MAPPED;    // use v4-mapped v6 if no v6 found
  hints.ai_protocol = IPPROTO_TCP;  // tcp protocol
  hints.ai_canonname = nullptr;
  hints.ai_addr = nullptr;
  hints.ai_next = nullptr;

  struct addrinfo* result;
  int res = getaddrinfo(nullptr, std::to_string(port_).c_str(),
                        &hints, &result);
  Verify333(res == 0);

  int listen_fd_val = -1;
  for (struct addrinfo* rp = result; rp != nullptr; rp = rp->ai_next) {
    listen_fd_val = socket(rp->ai_family,
                       rp->ai_socktype,
                       rp->ai_protocol);
    if (listen_fd_val == -1) {
      listen_fd_val = -1;
      continue;
    }

    int optval = 1;
    setsockopt(listen_fd_val, SOL_SOCKET, SO_REUSEADDR,
               &optval, sizeof(optval));

    if (bind(listen_fd_val, rp->ai_addr, rp->ai_addrlen) == 0) {
      break;
    }

    close(listen_fd_val);
    listen_fd_val = -1;
  }

  freeaddrinfo(result);

  if (listen_fd_val == -1) {
    return false;
  }

  if (listen(listen_fd_val, SOMAXCONN) != 0) {
    close(listen_fd_val);
    return false;
  }

  listen_sock_fd_ = listen_fd_val;
  *listen_fd = listen_fd_val;
  return true;
}

bool ServerSocket::Accept(int* const accepted_fd,
                          std::string* const client_addr,
                          uint16_t* const client_port,
                          std::string* const client_dns_name,
                          std::string* const server_addr,
                          std::string* const server_dns_name) const {
  // Accept a new connection on the listening socket listen_sock_fd_.
  // (Block until a new connection arrives.)  Return the newly accepted
  // socket, as well as information about both ends of the new connection,
  // through the various output parameters.

  // STEP 2:
  while (1) {
    struct sockaddr_storage caddr;
    socklen_t caddr_len = sizeof(caddr);
    int client_fd = accept(listen_sock_fd_,
                           reinterpret_cast<struct sockaddr*>(&caddr),
                           &caddr_len);
    if (client_fd < 0) {
      if ((errno == EINTR) || (errno == EAGAIN) || (errno == EWOULDBLOCK)) {
        continue;
      }
      return false;
    }

    // set fd
    *accepted_fd = client_fd;

    // set client_addr + client_port
    struct sockaddr *addr = reinterpret_cast<struct sockaddr*>(&caddr);
    if (addr->sa_family == AF_INET) {
      char astring[INET_ADDRSTRLEN];
      struct sockaddr_in* in4 = reinterpret_cast<struct sockaddr_in*>(addr);
      inet_ntop(AF_INET, &(in4->sin_addr), astring, INET_ADDRSTRLEN);
      *client_addr = astring;
      *client_port = ntohs(in4->sin_port);
    } else {
      char astring[INET6_ADDRSTRLEN];
      struct sockaddr_in6* in6 = reinterpret_cast<struct sockaddr_in6*>(addr);
      inet_ntop(AF_INET6, &(in6->sin6_addr), astring, INET6_ADDRSTRLEN);
      *client_addr = astring;
      *client_port = ntohs(in6->sin6_port);
    }

    // set client_dns
    char hostname[1024];  // ought to be big enough.
    getnameinfo(addr, sizeof(caddr), hostname, 1024, nullptr, 0, 0);
    *client_dns_name = hostname;

    // set server_addr + server_dns
    char hname[1024];
    hname[0] = '\0';
    if (sock_family_ == AF_INET) {
      struct sockaddr_in srvr;
      socklen_t srvrlen = sizeof(srvr);
      char addrbuf[INET_ADDRSTRLEN];
      getsockname(client_fd,
                  reinterpret_cast<struct sockaddr*>(&srvr),
                  &srvrlen);
      inet_ntop(AF_INET, &srvr.sin_addr, addrbuf, INET_ADDRSTRLEN);
      getnameinfo(reinterpret_cast<struct sockaddr*>(&srvr),
                  srvrlen, hname, 1024, nullptr, 0, 0);
      *server_addr = addrbuf;
      *server_dns_name = hname;
    } else {
      struct sockaddr_in6 srvr;
      socklen_t srvrlen = sizeof(srvr);
      char addrbuf[INET6_ADDRSTRLEN];
      getsockname(client_fd,
                  reinterpret_cast<struct sockaddr*>(&srvr),
                  &srvrlen);
      inet_ntop(AF_INET6, &srvr.sin6_addr, addrbuf, INET6_ADDRSTRLEN);
      getnameinfo(reinterpret_cast<struct sockaddr*>(&srvr),
                  srvrlen, hname, 1024, nullptr, 0, 0);
      *server_addr = addrbuf;
      *server_dns_name = hname;
    }

    return true;
  }
}

}  // namespace hw4
