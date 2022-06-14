#pragma once
#include <errno.h>

#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <netinet/in.h>
#include <resolv.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include "message.pb.h"
#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <strings.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string>
#include <iostream>

#include <netinet/tcp.h>
namespace util {

  void err(char* msg, int _errno = 0);
  static constexpr size_t message_size = 32768;	// 32K buffer for received messages

  size_t convertByteArrayToInt(char* b);

  void convertIntToByteArray(char* dst, int sz);



  // tcp socket connections initialisers
  using socket_fd = int;

  socket_fd tcp_sending_socket_init(int remote_port, std::string client_ip);


  socket_fd tcp_accepting_connection(socket_fd hsock, std::string& client_ip);

  socket_fd tcp_listening_socket_init(int host_port, std::string remote_ip);
}
