#include "server_connection.h"

ServerConnection::ServerConnection(const char *addr, uint32_t port):
  m_Address(addr), m_Port(port)
{}

int32_t ServerConnection::writePacket(const uint8_t *buffer, int32_t size) {
  char writeBuff[8];
  for (int32_t i = 0; i < size; ++i) {
    uint8_t len = sprintf(writeBuff, "%02x", buffer[i]);
    if (write(this->m_FD, writeBuff, len) < 0) return -1;
  }
  
  if (write(this->m_FD, "\n", 1) < 0) return -1;
  return 0;
}

int32_t ServerConnection::initConn() {
  int32_t rc;

  /* Configures the socket structure, we also convert the IP to binary */
  memset(reinterpret_cast<void *>(&this->m_SocketAddr), 0x0, sizeof (struct sockaddr_in));
  this->m_SocketAddr.sin_family = AF_INET;
  this->m_SocketAddr.sin_addr.s_addr = inet_addr(GLOBAL_SERVER_IP);
  this->m_SocketAddr.sin_port = htons(GLOBAL_SERVER_PORT);

  /* Gets the socket file descriptor, and prints serial error if this fails */
  this->m_FD = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (this->m_FD < 0) return -1;

  /* Connects the socket to the server, and checks for further errors */
  rc = connect(this->m_FD, reinterpret_cast<struct sockaddr *>(
    &this->m_SocketAddr), sizeof (struct sockaddr_in));
  if (rc != 0) return -1;

  return 0;
}

void ServerConnection::closeConn() {
  shutdown(this->m_FD, SHUT_RDWR);
}