/*******************************************************************************
* Simple FIFO Order Server Implementation
*
* Description:
*     A server implementation designed to process client requests in First In,
*     First Out (FIFO) order. The server binds to the specified port number
*     provided as a parameter upon launch.
*
* Usage:
*     <build directory>/server <port_number>
*
* Parameters:
*     port_number - The port number to bind the server to.
*
* Author:
*     Renato Mancuso
*
* Affiliation:
*     Boston University
*
* Creation Date:
*     September 10, 2023
*
* Notes:
*     Ensure to have proper permissions and available port before running the
*     server. The server relies on a FIFO mechanism to handle requests, thus
*     guaranteeing the order of processing. For debugging or more details, refer
*     to the accompanying documentation and logs.
*
*******************************************************************************/

#define _GNU_SOURCE
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sched.h>
#include <signal.h>

/* Needed for wait(...) */
#include <sys/types.h>
#include <sys/wait.h>

/* Include struct definitions and other libraries that need to be
 * included by both client and server */
#include "common.h"

#define BACKLOG_COUNT 100
#define USAGE_STRING				\
	"Missing parameter. Exiting.\n"		\
	"Usage: %s <port_number>\n"

/* 4KB of stack for the worker thread */
#define STACK_SIZE (4096)

/* Main logic of the worker thread */
/* IMPLEMENT HERE THE MAIN FUNCTION OF THE WORKER */
int worker_main(void *arg) {
	(void)arg;
	struct timespec current_tsc;
	clock_gettime(CLOCK_MONOTONIC, &current_tsc);
	printf("[#WORKER#] %f Worker Thread Alive!\n", TSPEC_TO_DOUBLE(current_tsc));

	struct timespec rem, wait_a_sec;
	wait_a_sec.tv_sec = 1;
	wait_a_sec.tv_nsec = 0;

	while(1) {
		// 1. 忙
		busywait_timespec(wait_a_sec);

		// 2. 说
		clock_gettime(CLOCK_MONOTONIC, &current_tsc);
		printf("[#WORKER#] %f Still Alive!\n", TSPEC_TO_DOUBLE(current_tsc));

		// 3. 睡
		nanosleep(&wait_a_sec, &rem);
	}
}


/* Main function to handle connection with the client. This function
 * takes in input conn_socket and returns only when the connection
 * with the client is interrupted. */
void handle_connection(int conn_socket)
{
	/* The connection with the client is alive here. Let's start
	 * the worker thread. */

	void * child_stack = malloc(STACK_SIZE);
	void * bottom_child_stack = child_stack + STACK_SIZE;
	int result = clone(worker_main, bottom_child_stack, CLONE_THREAD | CLONE_VM | CLONE_SIGHAND | CLONE_FS | CLONE_FILES | CLONE_SYSVSEM, NULL);
	if (result == -1) {
		perror("clone failed");
		free(child_stack); // Free the allocated memory
        close(conn_socket);
		return;
	} 

	/* We are ready to proceed with the rest of the request
	 * handling logic. */

	struct request request;
    struct response response;
	ssize_t received, sent;
    struct timespec tsc_receipt, tsc_completion;

	 while (1) {
        received = recv(conn_socket, &request, sizeof(request), 0);

        if (received <= 0) break;
        
        clock_gettime(CLOCK_MONOTONIC, &tsc_receipt);

		// 执行request
        get_elapsed_busywait(request.req_length.tv_sec, request.req_length.tv_nsec);

        clock_gettime(CLOCK_MONOTONIC, &tsc_completion);

        response.req_id = request.req_id;

        sent = send(conn_socket, &response, sizeof(response), 0);

        if (sent <= 0) break;

		// 计算
        printf("R%lu:%f,%f,%f,%f\n",
               request.req_id,
               TSPEC_TO_DOUBLE(request.req_timestamp),
               TSPEC_TO_DOUBLE(request.req_length),
               TSPEC_TO_DOUBLE(tsc_receipt),
               TSPEC_TO_DOUBLE(tsc_completion));
	 }
	 free(child_stack);
	 close(conn_socket);
}


/* Template implementation of the main function for the FIFO
 * server. The server must accept in input a command line parameter
 * with the <port number> to bind the server to. */
int main (int argc, char ** argv) {
	int sockfd, retval, accepted, optval;
	in_port_t socket_port;
	struct sockaddr_in addr, client;
	struct in_addr any_address;
	socklen_t client_len;

	/* Get port to bind our socket to */
	if (argc > 1) {
		socket_port = strtol(argv[1], NULL, 10);
		printf("INFO: setting server port as: %d\n", socket_port);
	} else {
		ERROR_INFO();
		fprintf(stderr, USAGE_STRING, argv[0]);
		return EXIT_FAILURE;
	}

	/* Now onward to create the right type of socket */
	sockfd = socket(AF_INET, SOCK_STREAM, 0);

	if (sockfd < 0) {
		ERROR_INFO();
		perror("Unable to create socket");
		return EXIT_FAILURE;
	}

	/* Before moving forward, set socket to reuse address */
	optval = 1;
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (void *)&optval, sizeof(optval));

	/* Convert INADDR_ANY into network byte order */
	any_address.s_addr = htonl(INADDR_ANY);

	/* Time to bind the socket to the right port  */
	addr.sin_family = AF_INET;
	addr.sin_port = htons(socket_port);
	addr.sin_addr = any_address;

	/* Attempt to bind the socket with the given parameters */
	retval = bind(sockfd, (struct sockaddr *)&addr, sizeof(struct sockaddr_in));

	if (retval < 0) {
		ERROR_INFO();
		perror("Unable to bind socket");
		return EXIT_FAILURE;
	}

	/* Let us now proceed to set the server to listen on the selected port */
	retval = listen(sockfd, BACKLOG_COUNT);

	if (retval < 0) {
		ERROR_INFO();
		perror("Unable to listen on socket");
		return EXIT_FAILURE;
	}

	/* Ready to accept connections! */
	printf("INFO: Waiting for incoming connection...\n");
	client_len = sizeof(struct sockaddr_in);
	accepted = accept(sockfd, (struct sockaddr *)&client, &client_len);

	if (accepted == -1) {
		ERROR_INFO();
		perror("Unable to accept connections");
		return EXIT_FAILURE;
	}

	/* Ready to handle the new connection with the client. */
	handle_connection(accepted);

	close(sockfd);
	return EXIT_SUCCESS;

}
