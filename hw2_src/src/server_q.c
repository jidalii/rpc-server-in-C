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

/* Needed for semaphores */
#include <semaphore.h>

/* Include struct definitions and other libraries that need to be
 * included by both client and server */
#include "common.h"

#define BACKLOG_COUNT 100
#define USAGE_STRING				\
	"Missing parameter. Exiting.\n"		\
	"Usage: %s <port_number>\n"

/* 4KB of stack for the worker thread */
#define STACK_SIZE (4096)

/* START - Variables needed to protect the shared queue. DO NOT TOUCH */
sem_t * queue_mutex;
sem_t * queue_notify;
/* END - Variables needed to protect the shared queue. DO NOT TOUCH */

/* Max number of requests that can be queued */
#define QUEUE_SIZE 500

typedef struct Node{
	struct request_wrapped req_wrapped;
    struct Node* next;
} Node;

struct queue {
    /* IMPLEMENT ME */
	Node* front;
    Node* rear;
	int size;
	// int capacity;
};

struct worker_params {
    /* IMPLEMENT ME */
	struct queue * the_queue;
	int conn_socket;
};

/* Add a new request <request> to the shared queue <the_queue> */
int add_to_queue(struct request_wrapped to_add, struct queue * the_queue)
{
	int retval = 0;
	/* QUEUE PROTECTION INTRO START --- DO NOT TOUCH */
	sem_wait(queue_mutex);
	/* QUEUE PROTECTION INTRO END --- DO NOT TOUCH */

	/* WRITE YOUR CODE HERE! */
	/* MAKE SURE NOT TO RETURN WITHOUT GOING THROUGH THE OUTRO CODE! */

	// 1. the queue is full
	if(the_queue->size >= QUEUE_SIZE) {
		sem_post(queue_mutex);
		sem_post(queue_notify);
		return -1;
	}

	// 2. create Node object
	Node* new_req = (Node*)malloc(sizeof(Node));
	new_req->req_wrapped = to_add;
	new_req->next = NULL;
	

	// 3.1 if queue is empty
	if(the_queue->size == 0){
		the_queue->front = new_req;
		the_queue->rear = new_req;
		the_queue->size+=1;
	} 
	// 3.2 if queue is not empty
	else {
		the_queue->rear->next = new_req;
		the_queue->rear = new_req;
		the_queue->size+=1;
	}

	/* QUEUE PROTECTION OUTRO START --- DO NOT TOUCH */
	sem_post(queue_mutex);
	sem_post(queue_notify);
	/* QUEUE PROTECTION OUTRO END --- DO NOT TOUCH */
	
	return retval;
}

/* Add a new request <request> to the shared queue <the_queue> */
struct request_wrapped * get_from_queue(struct queue * the_queue)
{
	struct request_wrapped * retval;
	retval = (struct request_wrapped *)malloc(sizeof(struct request_wrapped));
	/* QUEUE PROTECTION INTRO START --- DO NOT TOUCH */
	sem_wait(queue_notify);
	sem_wait(queue_mutex);
	/* QUEUE PROTECTION INTRO END --- DO NOT TOUCH */

	/* WRITE YOUR CODE HERE! */
	/* MAKE SURE NOT TO RETURN WITHOUT GOING THROUGH THE OUTRO CODE! */
	
	// 1. if queue is empty, return NULL
	if(the_queue->size == 0) {
		sem_post(queue_mutex);
		return NULL; // not sure
	}
	// 2. find the first request in queue
	*retval = the_queue->front->req_wrapped;

	// 3. update the queue front
	the_queue->front = the_queue->front->next;

	// 4. update the queue size
	the_queue->size--;

	/* QUEUE PROTECTION OUTRO START --- DO NOT TOUCH */
	sem_post(queue_mutex);
	/* QUEUE PROTECTION OUTRO END --- DO NOT TOUCH */
	return retval;
}

/* Implement this method to correctly dump the status of the queue
 * following the format Q:[R<request ID>,R<request ID>,...] */
void dump_queue_status(struct queue * the_queue)
{
	/* QUEUE PROTECTION INTRO START --- DO NOT TOUCH */
	sem_wait(queue_mutex);
	/* QUEUE PROTECTION INTRO END --- DO NOT TOUCH */

	/* WRITE YOUR CODE HERE! */
	printf("Q:[");
	Node* trav = the_queue->front;
	while(trav != NULL) {
		printf("R%ld", trav->req_wrapped.req.req_id);
		if(trav->next != NULL) {
			printf(",");
		}
		trav = trav->next;
	}
	printf("]\n");

	/* QUEUE PROTECTION OUTRO START --- DO NOT TOUCH */
	sem_post(queue_mutex);
	/* QUEUE PROTECTION OUTRO END --- DO NOT TOUCH */
}


/* Main logic of the worker thread */
/* IMPLEMENT HERE THE MAIN FUNCTION OF THE WORKER */
int worker_main(void *arg) {
	struct worker_params * worker_params = (struct worker_params *)arg;
	// struct request_wrapped * current_req_w;
	struct request_wrapped * current_req_w;
	struct request request;
	struct queue * current_queue;
    struct response response;
	ssize_t received, sent;
    struct timespec tsc_start, tsc_receipt, tsc_completion;
	int conn_socket;

	current_queue = worker_params->the_queue;
	conn_socket = worker_params->conn_socket;

	while(1){
		current_req_w = get_from_queue(current_queue);
		
		request = current_req_w->req;

		// if(current_req_w == NULL) break;

		clock_gettime(CLOCK_MONOTONIC, &tsc_start);

		// get_elapsed_busywait(current_req_w->req.req_length.tv_sec, current_req_w->req.req_length.tv_nsec);
		get_elapsed_busywait(current_req_w->req.req_length.tv_sec, current_req_w->req.req_length.tv_nsec);

		clock_gettime(CLOCK_MONOTONIC, &tsc_completion);
		
		tsc_receipt = current_req_w->tsc_receipt;
		

		printf("R%lu:%f,%f,%f,%f,%f\n",
               request.req_id,
               TSPEC_TO_DOUBLE(request.req_timestamp),
               TSPEC_TO_DOUBLE(request.req_length),
			   TSPEC_TO_DOUBLE(tsc_receipt),
               TSPEC_TO_DOUBLE(tsc_start),
               TSPEC_TO_DOUBLE(tsc_completion)
			   );

		dump_queue_status(current_queue);

		sent = send(conn_socket, &response, sizeof(response), 0);
		if (sent <= 0) break;
	}
}


/* Main function to handle connection with the client. This function
 * takes in input conn_socket and returns only when the connection
 * with the client is interrupted. */
void handle_connection(int conn_socket)
{
	struct request * req;
	struct request_wrapped * req_w;
	struct queue * the_queue;
	size_t in_bytes;
	
	struct worker_params * params;
	struct timespec tsc_receipt;

	req = (struct request *)malloc(sizeof(struct request));
	req_w = (struct request_wrapped *)malloc(sizeof(struct request_wrapped));
	params = (struct worker_params *)malloc(sizeof(struct worker_params));


	/* The connection with the client is alive here. Let's
	 * initialize the shared queue. */

	/* IMPLEMENT HERE ANY QUEUE INITIALIZATION LOGIC */
	the_queue = (struct queue *)malloc(sizeof(struct queue));
	the_queue->front = NULL;
	the_queue->rear = NULL;
	the_queue->size = 0;

	/* Queue ready to go here. Let's start the worker thread. */

	/* IMPLEMENT HERE THE LOGIC TO START THE WORKER THREAD. */
	void * child_stack = malloc(STACK_SIZE);

	params->conn_socket = conn_socket;
	params->the_queue = the_queue;

	// void * bottom_child_stack = child_stack + STACK_SIZE;
	int result = clone(worker_main, child_stack+STACK_SIZE, CLONE_THREAD | CLONE_VM | CLONE_SIGHAND | CLONE_FS | CLONE_FILES | CLONE_SYSVSEM, params);
	if (result == -1) {
		perror("clone failed");
		free(child_stack); // Free the allocated memory
		free(req);
		free(req_w);
		free(params);
		free(the_queue);
        close(conn_socket);
		return;
	} 

	/* We are ready to proceed with the rest of the request
	 * handling logic. */

	/* REUSE LOGIC FROM HW1 TO HANDLE THE PACKETS */

	do {
		in_bytes = recv(conn_socket, req, sizeof(struct request), 0);
		/* SAMPLE receipt_timestamp HERE */
		clock_gettime(CLOCK_MONOTONIC, &tsc_receipt);
		
		/* Don't just return if in_bytes is 0 or -1. Instead
		 * skip the response and break out of the loop in an
		 * orderly fashion so that we can de-allocate the req
		 * and resp varaibles, and shutdown the socket. */
		if (in_bytes > 0) {
			req_w->tsc_receipt = tsc_receipt;
			req_w->req = *req;
			add_to_queue(*req_w, the_queue);
		}
	} while (in_bytes > 0);

	/* PERFORM ORDERLY DEALLOCATION AND OUTRO HERE */
	free(req);
	free(req_w);
	free(params);
	free(the_queue);
	free(child_stack);
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

	/* Initialize queue protection variables. DO NOT TOUCH. */
	queue_mutex = (sem_t *)malloc(sizeof(sem_t));
	queue_notify = (sem_t *)malloc(sizeof(sem_t));
	retval = sem_init(queue_mutex, 0, 1);
	if (retval < 0) {
		ERROR_INFO();
		perror("Unable to initialize queue mutex");
		return EXIT_FAILURE;
	}
	retval = sem_init(queue_notify, 0, 0);
	if (retval < 0) {
		ERROR_INFO();
		perror("Unable to initialize queue notify");
		return EXIT_FAILURE;
	}
	/* DONE - Initialize queue protection variables. DO NOT TOUCH */

	/* Ready to handle the new connection with the client. */
	handle_connection(accepted);

	free(queue_mutex);
	free(queue_notify);

	close(sockfd);
	return EXIT_SUCCESS;

}
