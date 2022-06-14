
// Client side C/C++ program to demonstrate Socket programming 
#include <stdio.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <unistd.h> 
#include <string.h> 
#include <sys/types.h>
#include <netdb.h>
#include <iostream>

#define PORT 8080 

int main(int argc, char const *argv[]) 
{ 
	int sock_fd = 0, valread; 
	struct sockaddr_in serv_addr; 
	char *hello = "Hello from client"; 
	char buffer[1024] = {0};

	std::string rem_port("8080");

	std::string rem_hostname("10.0.2.1");
	if ((sock_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) { 
	//if ((sock_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) { 
		printf("\n Socket creation error \n"); 
		return -1; 
	} 

	memset(&serv_addr, 0, sizeof(serv_addr)); 
	serv_addr.sin_family = AF_INET; 
	serv_addr.sin_port = htons(PORT); 
	serv_addr.sin_addr.s_addr = inet_addr("129.215.165.54");  // INADDR_BROADCAST; //INADDR_ANY;


	/*
	struct addrinfo *rem_addrinfo = nullptr;
	char port_str[16];
	snprintf(port_str, sizeof(port_str), "%u", rem_port);

	struct addrinfo hints;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_protocol = IPPROTO_UDP;
	*/


	/*
	int r = getaddrinfo(rem_hostname.c_str(), rem_port.c_str(), &hints, &rem_addrinfo);
	if (r != 0 || rem_addrinfo == nullptr) {
		char issue_msg[1000];
		sprintf(issue_msg, "Failed to resolve %s:%s. getaddrinfo error = %s.", rem_hostname.c_str(), rem_port.c_str(), gai_strerror(r));
		throw std::runtime_error(issue_msg);
	}
	std::cout << rem_addrinfo->ai_addr->sa_data << "\n";
	
	ssize_t _ret = sendto(sock_fd, hello, strlen(hello), 0, rem_addrinfo->ai_addr,
	   rem_addrinfo->ai_addrlen);
	   */
	for (int i = 0; i < 1000; i++) {
		fprintf(stdout, "Sending i = %d\n", i);
		ssize_t ret = sendto(sock_fd, hello, strlen(hello), 0, (const struct sockaddr *) &serv_addr,
			sizeof(serv_addr));
	}

	printf("Client: send\n"); 
	/*
	// Convert IPv4 and IPv6 addresses from text to binary form 
	if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0)  
	{ 
	printf("\nInvalid address/ Address not supported \n"); 
	return -1; 
	} 

	if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) 
	{ 
	printf("\nConnection Failed \n"); 
	return -1; 
	} 
	send(sock, hello, strlen(hello) , 0 ); 
	printf("Hello message sent\n"); 
	valread = read(sock, buffer, 1024); 
	printf("%s\n",buffer ); 
	*/
	return 0; 
} 

