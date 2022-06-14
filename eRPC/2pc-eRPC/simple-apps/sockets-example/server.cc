
// Server side C/C++ program to demonstrate Socket programming 
#include <unistd.h> 
#include <stdio.h> 
#include <sys/socket.h> 
#include <stdlib.h> 
#include <netinet/in.h> 
#include <string.h> 
#include <iostream>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 8080 

int main(int argc, char const *argv[]) { 
	int server_fd, new_socket, valread; 
	struct sockaddr_in address; 
	char buffer[1024] = {0}; 
	char *hello = "Hello from server"; 
	// Creating socket file descriptor 
	if ((server_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == 0) { 
	// if ((server_fd = socket(AF_INET, SOCK_DGRAM, 0)) == 0) { 
		perror("socket failed"); 
		exit(EXIT_FAILURE); 
	} 
	struct timeval tv;
	tv.tv_sec = 10;
	tv.tv_usec = 100 * 1000;
	if (setsockopt(server_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
		perror("setsockopt"); 
		exit(EXIT_FAILURE); 
	}

	address.sin_family = AF_INET; 
	address.sin_addr.s_addr = inet_addr("129.215.165.54");// INADDR_BROADCAST; //INADDR_ANY; inet_addr()
	address.sin_port = htons(PORT); 

	// Forcefully attaching socket to the port 8080 
	if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) { 
		perror("bind failed"); 
		exit(EXIT_FAILURE); 
	}
	
	std::cout << "Server: recv\n";

	if (recv(server_fd, buffer, 1024, 0) >= 0) 
		std::cout << buffer << "\n";

	if (recv(server_fd, buffer, 1024, 0) >= 0) 
		std::cout << buffer << "\n";

	if (recv(server_fd, buffer, 1024, 0) >= 0) 
		std::cout << buffer << "\n";

	std::cout << "Server: end" << "\n";

	return 0; 
} 

