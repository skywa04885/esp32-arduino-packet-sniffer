/*
  Copyright Cybox 2020 - Written by Luke A.C.A. Rieff
*/

#ifndef _SERVER_CONN_H
#define _SERVER_CONN_H

#include "default.h"
#include "cbxpkt.h"

class ServerConnection {
public:
  ServerConnection(const char *addr, uint32_t port);

  int32_t writePacket(const uint8_t *buffer, int32_t size);
  int32_t initConn();

  void closeConn();
private:
  const char *m_Address;
  uint32_t m_Port;

  int32_t m_FD;
  struct sockaddr_in m_SocketAddr;
  bool m_Connected;
};

#endif