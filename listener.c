
/**
 * References: https://docs.microsoft.com/en-us/windows/win32/winsock/getting-started-with-winsock
 *
 *
 */

/* Custom includes */
#include "../checksums/checksums.h"

/* Windows includes */
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>

/* Std */
#include <stdio.h>
#include <stdbool.h>

#pragma comment(lib, "Ws2_32.lib")

#define MYPORT 6500

#define RECV_BUF_SIZE 11680

int main(int argc, char * argv[])
{
	int sockfd, retval;
	struct sockaddr_in my_addr;

	/* From tutorial */
	struct addrinfo *result = NULL,
			*ptr = NULL,
			hints;

	WSADATA wsaData;
	int iResult;

	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
	if (iResult != 0)
	{
		printf("WSAStartup failed: %d\n", iResult);
		return 1;
	}

	/* AF_UNSPEC so IPv6 or IPv4 addresses can be returned */
/*	ZeroMemory( &hints, sizeof(hints) );
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	iResult = getaddrinfo(argv[1], DEFAULT_PORT, &hints, &result);
	if (iResult != 0)
	{
		printf("getaddrinfo failed: %d\n", iResult);
		WSACleanup();
		return 1;
	}*/

	sockfd = socket(PF_INET, SOCK_STREAM, 0);
	if (-1 == sockfd)
	{
		//printf("socket() call failed with errno %s\n", strerror(errno));
		perror("socket() error");
//		return -1;
	}
	printf("Socket created\n");

	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(MYPORT);
	my_addr.sin_addr.s_addr = inet_addr("192.168.1.5");
	memset(my_addr.sin_zero, '\0', sizeof my_addr.sin_zero);

	retval = bind(sockfd, (struct sockaddr *)&my_addr, sizeof my_addr);
	if (-1 == retval)
	{
//		printf("bind() call failed\n");
		perror("bind() error");
		return -1;
	}
	printf("Bound\n");

	retval = listen(sockfd, 1);
	if (-1 == retval)
	{
		printf("listen() call failed with errno %d\n", errno);
		return -1;
	}
	else
	{
		int new_fd;
		//struct sockaddr_storage their_addr;
		struct sockaddr their_addr;	
		//socklen_t addr_size;
		int addr_size;

		printf("Listening\n");

		new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &addr_size);
		if (-1 == new_fd)
		{
			printf("accept() call failed with errno %d\n", errno);
			return -1;
		}
		else
		{
			char recv_buf[RECV_BUF_SIZE];
			int recv_bytes;

			printf("Accepted connection\n");
			
			while(1)
			{
				recv_bytes = recv(new_fd, recv_buf, sizeof(recv_buf), 0);
				if (-1 == recv_bytes)
				{
					printf("recv() failed\n");
					return -1;
				}	
				else if (0 == recv_bytes)
				{
					printf("Connection closed\n");
					break;
				}
				else
				{
					printf("Received %d bytes\n", recv_bytes);
				}		
			}
		}
	}

	printf("Hello, world!\n");

	return 0;
}

bool checksum_is_correct(char *input_string, uint32_t input_size, uint32_t num_checksum_bytes)
{
	gen_checksum_invsum(input_string, input_size, num_checksum_bytes);

	return true;
}
