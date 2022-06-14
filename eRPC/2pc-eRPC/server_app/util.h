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
#if 0
namespace util {

  void err(char* msg, int _errno = 0) {
    fprintf(stderr, "[Error] %s", msg);
    if (errno != 0)
      fprintf(stderr, " (errno: %d)\n", _errno);
    exit(-1);
  }


  static constexpr size_t message_size = 32768;	// 32K buffer for received messages
 // static std::string client_ip;


  size_t convertByteArrayToInt(char* b) {
    return (b[0] << 24)
      + ((b[1] & 0xFF) << 16)
      + ((b[2] & 0xFF) << 8)
      + (b[3] & 0xFF);
  }


  void convertIntToByteArray(char* dst, int sz) {
    auto tmp = dst;
    tmp[0] = (sz >> 24) & 0xFF;
    tmp[1] = (sz >> 16) & 0xFF;
    tmp[2] = (sz >> 8) & 0xFF;
    tmp[3] = sz & 0xFF;
  }



  // tcp socket connections initialisers
  using socket_fd = int;

  socket_fd tcp_sending_socket_init(int remote_port, std::string client_ip) {
    struct sockaddr_in java_client;
    socket_fd fd = -1;
    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
      err("socket");
    }

    bzero(&java_client, sizeof(java_client));

    java_client.sin_family = AF_INET;
    java_client.sin_port = htons(remote_port);
    // inet_aton("127.0.0.1", &java_client.sin_addr);
    inet_aton(client_ip.c_str(), &java_client.sin_addr);

    int one = 1;

	setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one));
    if (connect(fd, (struct sockaddr *)&java_client, sizeof(struct sockaddr)) == -1) {
      fprintf(stderr, "connect %s\n", strerror(errno));
      exit(1);
    }

    // sleep(10);

    fprintf(stdout, "connected to %d - %s\n", remote_port, client_ip.c_str());

    return fd;

  }


  socket_fd tcp_accepting_connection(socket_fd hsock, std::string& client_ip) {
    socklen_t addr_size = 0;
    sockaddr_in sadr;

    socket_fd csock = -1;

    // Now lets do the server stuff

    addr_size = sizeof(sockaddr_in);
    printf("waiting for a connection\n");
    if ((csock = accept(hsock, (sockaddr*)&sadr, &addr_size)) == -1){
      fprintf(stderr, "Error listening %s\n", strerror(errno));
    }
    else { 
      printf("---------------------\nReceived connection from %s\n", inet_ntoa(sadr.sin_addr));
      client_ip = inet_ntoa(sadr.sin_addr);
    }


    return csock;

  }


  socket_fd tcp_listening_socket_init(int host_port, std::string remote_ip) {
    struct sockaddr_in my_addr;
    socket_fd hsock;
    int * p_int ;

    hsock = socket(AF_INET, SOCK_STREAM, 0);
    if(hsock == -1){
      printf("Error initializing socket %d\n", errno);
      exit(1);
    }

    p_int = (int*)malloc(sizeof(int));
    *p_int = 1;

    if ((setsockopt(hsock, SOL_SOCKET, SO_REUSEADDR, (char*)p_int, sizeof(int)) == -1 )||
        (setsockopt(hsock, SOL_SOCKET, SO_KEEPALIVE, (char*)p_int, sizeof(int)) == -1 )) {
      printf("Error setting options %d\n", errno);
      free(p_int);
      exit(1);
    }

    free(p_int);

    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(host_port);

    my_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(hsock, (sockaddr*)&my_addr, sizeof(my_addr)) == -1) {
      fprintf(stderr,"Error binding to socket, make sure nothing else is listening on this port %d\n",errno);
      exit(1);
    }

    if (listen(hsock, 1024) == -1) {
      fprintf(stderr, "Error listening -- %d\n", errno);
      exit(1);
    }

    // Now lets do the server stuff

    /*
       addr_size = sizeof(sockaddr_in);
       printf("waiting for a connection\n");
       if ((csock = accept(hsock, (sockaddr*)&sadr, &addr_size)) == -1){
       fprintf(stderr, "Error listening %d\n",errno);
       exit(1);
       }

       printf("---------------------\nReceived connection from %s\n", inet_ntoa(sadr.sin_addr));

       return csock;
       */
    return hsock;

  }
}

#endif

namespace util {
  void err(char* msg, int _errno = 0);


  static constexpr size_t message_size = 32768;	// 32K buffer for received messages
 // static std::string client_ip;


  size_t convertByteArrayToInt(char* b);

  void convertIntToByteArray(char* dst, int sz);


  // tcp socket connections initialisers
  using socket_fd = int;

  socket_fd tcp_accepting_connection(socket_fd hsock, std::string& client_ip);
  socket_fd tcp_sending_socket_init(int remote_port, std::string client_ip);

  socket_fd tcp_listening_socket_init(int host_port, std::string remote_ip);
}
