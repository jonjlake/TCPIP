
/**
 * References: https://docs.microsoft.com/en-us/windows/win32/winsock/getting-started-with-winsock
 *
 *
 */

/*****************
 * Header includes
 *****************/
// Custom includes
#include "../checksums/checksums.h"
// Windows includes
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
// Std
#include <stdio.h>
#include <stdbool.h>

/*******
 * Other
 *******/
#pragma comment(lib, "Ws2_32.lib")


/*******************
 * Macro definitions
 *******************/
// Debug
#define PRINT_LOOP_PERIOD 10000
// Network config
#define LOCAL_IP_ADDR "192.168.1.5"
#define LISTEN_PORT 6500
#define FOREIGN_IP_ADDR "192.168.1.4"
#define FOREIGN_SEND_PORT 6500
// Socket buffer config
#define RECV_BUF_SIZE 11680
// Processing config
//#define DO_CHECKSUM


/*******
 * Enums
 *******/
typedef enum JON_RETCODE
{
	JON_RETCODE_FAILURE,
	JON_RETCODE_SUCCESS,
} JON_RETCODE;


typedef enum TCPIP_ROLE
{
	TCPIP_ROLE_LISTENER,
	TCPIP_ROLE_SENDER,
} TCPIP_ROLE;


/**********************
 * File scope variables
 **********************/
static TCPIP_ROLE fg_tcpip_role;


/********************* 
 * Function prototypes 
 *********************/
void process_connection(int new_fd);
void print_stats(uint32_t msgs_corr_cyc, uint32_t msgs_recvd_cyc, uint32_t msgs_corr_tot,
		uint32_t msgs_recvd_tot);
float calc_pct_corr(uint32_t msgs_corr, uint32_t msgs_recvd);


/**********************
 * Function definitions
 **********************/

/*
 * Desc
 *
 * @param argc
 * @param argv
 *
 * @return
 */
int main(int argc, char * argv[])
{
	int sockfd;
	
	if (JON_RETCODE_SUCCESS != process_argv(argc, argv))
	{
		printf("Failed to process args\n");
		return JON_RETCODE_FAILURE;
	}


	if (JON_RETCODE_SUCCESS != initialize_tcpip(&sockfd))
	{
		printf("TCPIP intialization failed\n");
		return -1;
	}

	switch (fg_tcpip_role)
	{
		case TCPIP_ROLE_LISTENER:
			if (JON_RETCODE_SUCCESS != run_listener(&sockfd))
			{
				printf("Failed to run listener\n");
				return -1;
			}
			break;
		case TCPIP_ROLE_SENDER:
			if (JON_RETCODE_SUCCESS != run_sender())
			{
				printf("Failed to run sender\n");
				return -1;
			}
			break;
		default:
			printf("Unknown role. Exiting.\n");
			return -1;
	}

	printf("Program ending\n");

	return 0;
}


JON_RETCODE run_sender(int sockfd)
{
	int send_ret;

	struct sockaddr_in send_sockaddr_in;
	int sockaddr_in_len = sizeof(send_sockaddr_in);
	char *msg = "hello!";
	int msg_len = strlen(msg);

	send_sockaddr_in.sin_family = AF_INET;
	send_sockaddr_in.sin_addr.s_addr = inet_addr(FOREIGN_IP_ADDR);
	send_sockaddr_in.sin_port = htons(FOREIGN_SEND_PORT);

	if (-1 == connect(sockfd, (struct sockaddr*)&send_sockaddr_in,
				sockaddr_in_len))
	{
		perror("Connect failed");
		return JON_RETCODE_FAILURE;
	}
	else
	{
		printf("Sender connected\n");
	}

	send_ret = send(sockfd, msg, msg_len, 0);
	if (send_ret == msg_len)
	{
		printf("Send success\n");
	}
	else if (send_ret > 0)
	{
		printf("Partial send success\n");
	}
	else if (send_ret == 0)
	{
		printf("Disconnected\n");
	}
	else
	{
		printf("Send error\n");
	}

	printf("Run sender unimplemented\n");
	return JON_RETCODE_SUCCESS;
}


JON_RETCODE initialize_tcpip(int *p_sockfd)
{
	WSADATA wsaData;
	int iResult;
	int sockfd;

	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
	if (iResult != 0)
	{
		printf("WSAStartup failed: %d\n", iResult);
		return JON_RETCODE_FAILURE;
	}

	sockfd = socket(PF_INET, SOCK_STREAM, 0);
	if (-1 == sockfd)
	{
		perror("socket() error");
		return JON_RETCODE_FAILURE;
	}
	printf("Socket created\n");

	*p_sockfd = sockfd;

	return JON_RETCODE_SUCCESS;
}


/*
 * Desc
 *
 * @param sockfd
 */
JON_RETCODE run_listener(int *p_sockfd)
{
	struct sockaddr_in my_addr;
	int retval;

	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(LISTEN_PORT);
	my_addr.sin_addr.s_addr = inet_addr(LOCAL_IP_ADDR);
	memset(my_addr.sin_zero, '\0', sizeof my_addr.sin_zero);

	retval = bind(*p_sockfd, (struct sockaddr *)&my_addr, sizeof my_addr);
	if (-1 == retval)
	{
		perror("bind() error");
		return JON_RETCODE_FAILURE;
	}
	printf("Bound\n");

	retval = listen(*p_sockfd, 1);
	if (-1 == retval)
	{
		printf("listen() call failed with errno %d\n", errno);
		return JON_RETCODE_FAILURE;
	}
	else
	{
		int new_fd;
		//struct sockaddr_storage their_addr;
		struct sockaddr their_addr;	
		//socklen_t addr_size;
		int addr_size;

		printf("Listening on %s:%u\n", LOCAL_IP_ADDR, LISTEN_PORT);

		while(1)
		{
			//new_fd = accept(*p_sockfd, (struct sockaddr *)&their_addr, &addr_size);
			new_fd = accept(*p_sockfd, NULL, NULL);	
			if (-1 == new_fd)
			{
				printf("accept() call failed with errno %d\n", errno);
				printf("Win: accept failed: %d\n", WSAGetLastError());
				closesocket(new_fd);
				WSACleanup();
				return JON_RETCODE_FAILURE;
			}
			else
			{
				process_connection(new_fd);	
			}
		}
	}
}


/*
 * Desc
 *
 * @param argc
 * @param argv
 *
 * @return
 */
JON_RETCODE process_argv(int argc, char *argv[])
{
	char *p_arg;
	int i, j;
	char *cmp_option[3] = {"listen", "send", NULL};
	TCPIP_ROLE cmp_roles[3] = {TCPIP_ROLE_LISTENER, TCPIP_ROLE_SENDER}; // Very hacky
	char **pp_cmp_opt;

	printf("Received %d arguments\n", argc - 1);
	
	for (i = 1; i < argc; i++)
	{
		p_arg = argv[i];

		j = 0;
		for (pp_cmp_opt = cmp_option; *pp_cmp_opt; pp_cmp_opt++, j++)
		{
			int arg_len = strlen(p_arg);
			if (arg_len != strlen(*pp_cmp_opt))
			{
				continue;
			}	
			if (0 == strncmp(*pp_cmp_opt, p_arg, arg_len))
			{
				fg_tcpip_role = cmp_roles[j];
				printf("Operating as TCP/IP role: %s\n", *pp_cmp_opt);
				return JON_RETCODE_SUCCESS;
			}
		}	
/*		switch (p_arg)
		{
			case "listen":
				printf("Operating as TCP/IP listener\n");
				fg_tcpip_role = TCPIP_ROLE_LISTENER;
				break;
			case "send":
				printf("Operating as TCP/IP sender\n");
				fg_tcpip_role = TCPIP_ROLE_SENDER;
				break;
			default:
				printf("Unknown argument %s\n", p_arg);
				return JON_RETCODE_FAILURE;
		}*/
	}	

	return JON_RETCODE_FAILURE;
}


/*
 * Desc
 *
 * @param new_fd
 */
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


/*
 * Desc
 *
 * @param msgs_corr
 * @param msgs_recvd
 *
 * @return
 */
float calc_pct_corr(uint32_t msgs_corr, uint32_t msgs_recvd)
{
	if (0 == msgs_corr || 0 == msgs_recvd)
	{
		return 0;
	}

	return 100.0 * (float) msgs_corr / (float) msgs_recvd;
}


/*
 * Desc
 *
 * @param msgs_corr_cyc
 * @param msgs_recvd_cyc
 * @param msgs_corr_tot
 * @param msgs_recvd_tot
 */
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
