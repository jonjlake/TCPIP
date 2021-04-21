
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

#define PRINT_LOOP_PERIOD 100000

#define SERVER_IP_ADDR "192.168.1.4"
#define SERVER_PORT 6500

#define RECV_BUF_SIZE 11680

//#define DO_CHECKSUM

/* Function prototypes */
void process_connection(int new_fd);
void print_stats(uint32_t msgs_corr_cyc, uint32_t msgs_recvd_cyc, uint32_t msgs_corr_tot,
		uint32_t msgs_recvd_tot);
float calc_pct_corr(uint32_t msgs_corr, uint32_t msgs_recvd);

int main(int argc, char * argv[])
{
	int sockfd, retval;
	struct sockaddr_in srv_addr;

	/* From tutorial */
	struct addrinfo *result = NULL,
			*ptr = NULL,
			hints;

	WSADATA wsaData;
	int iResult;

	// sockopt stuff
	BOOL bOptVal = TRUE;
	int bOptLen = sizeof(BOOL);

	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
	if (iResult != 0)
	{
		printf("WSAStartup failed: %d\n", iResult);
		return 1;
	}

	sockfd = socket(PF_INET, SOCK_STREAM, 0);
	if (-1 == sockfd)
	{
		perror("socket() error");
		return -1;
	}
	printf("Socket created\n");

	srv_addr.sin_family = AF_INET;
	srv_addr.sin_port = htons(SERVER_PORT);
	srv_addr.sin_addr.s_addr = inet_addr(SERVER_IP_ADDR);
	memset(srv_addr.sin_zero, '\0', sizeof srv_addr.sin_zero);

	if (-1 == connect(sockfd, (struct sockaddr*)&srv_addr, sizeof(srv_addr)))
	{
		printf("Connection failed\n");
		return -1;
	}

	process_connection(sockfd);	

	return 0;
}

void process_connection(int new_fd)
{
	char recv_buf[RECV_BUF_SIZE];
	int recv_bytes;
	uint32_t msgs_recvd_tot = 0;
	uint32_t msgs_corr_tot = 0;
	uint32_t msgs_recvd_cyc = 0;
	uint32_t msgs_corr_cyc = 0;
	uint32_t loop_count = 0;
	uint64_t bytes_recvd_cyc = 0;
	uint64_t bytes_recvd_tot = 0;

	printf("Accepted connection\n");

	while(1)
	{
		loop_count++;

		//recv_bytes = recv(new_fd, recv_buf, sizeof(recv_buf), 0);
		recv_bytes = recv(new_fd, recv_buf, RECV_BUF_SIZE, 0);
		if (0 < recv_bytes)
		{
			msgs_recvd_cyc++;
			/* Succees! Test message now */
#ifdef DO_CHECKSUM	
			if (!checksum_correct(recv_buf, recv_bytes, 1))
#else
			if (0) // Check number of bytes instead?
#endif
			{
			//	printf("CS incorrect: %d bytes\n", recv_bytes);
			}
			else
			{
				msgs_corr_cyc++;
				bytes_recvd_cyc += recv_bytes;
			}
			//printf("Received %d bytes\n", recv_bytes);
			if (0 == loop_count % PRINT_LOOP_PERIOD)
			{
				bytes_recvd_tot += bytes_recvd_cyc;
				printf("Bytes recvd cyc: %f M tot: %f M\n", (float)bytes_recvd_cyc / 1e6, (float)bytes_recvd_tot / 1e6);
				msgs_corr_tot += msgs_corr_cyc;
				msgs_recvd_tot += msgs_recvd_cyc;
				print_stats(msgs_corr_cyc, msgs_recvd_cyc, msgs_corr_tot,
						msgs_recvd_tot);
				msgs_corr_cyc = 0;
				msgs_recvd_cyc = 0;
				bytes_recvd_cyc = 0;
			}
		}
		else if (-1 == recv_bytes)
		{
			printf("recv() failed\n");
			//return -1;
			return;
		}	
		else if (0 == recv_bytes)
		{
			printf("Connection closed\n");
			break;
		}
	}
	printf("Shutting down connection\n");
	shutdown(new_fd, SD_BOTH);
}

float calc_pct_corr(uint32_t msgs_corr, uint32_t msgs_recvd)
{
	if (0 == msgs_corr || 0 == msgs_recvd)
	{
		return 0;
	}

	return 100.0 * (float) msgs_corr / (float) msgs_recvd;
}

void print_stats(uint32_t msgs_corr_cyc, uint32_t msgs_recvd_cyc, uint32_t msgs_corr_tot,
		uint32_t msgs_recvd_tot)
{
	float pct_corr_cyc, pct_corr_tot;

	pct_corr_cyc = calc_pct_corr(msgs_corr_cyc, msgs_recvd_cyc);
	pct_corr_tot = calc_pct_corr(msgs_corr_tot, msgs_recvd_tot);

	printf("Success: CYC: %f (%u/%u) TOT: %f (%u/%u)\n", 
			pct_corr_cyc, msgs_corr_cyc, msgs_recvd_cyc,
			pct_corr_tot, msgs_corr_tot, msgs_recvd_tot);	
}
