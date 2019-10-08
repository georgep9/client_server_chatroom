#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>

#define BUFFER_SIZE 1024
#define QUEUE_SIZE 10

int sockfd;

void clean_exit(int signum){
	
	/* TODO

	deal  elegantly  with  any  threads  that  have  been  created  
	as  well  as  any  open  sockets,  shared  memory  regions,  
	dynamically allocated memory and/or open files
	
	*/

	close(sockfd);
	exit(1);

}

int main(int argc, const char** argv){


	// exit signal handler
	signal(SIGINT,clean_exit);

	// set port number
	uint16_t PORT_NUMBER = 12345;
	if (argc == 2){ PORT_NUMBER = atoi(argv[1]); }

	// setup server address and port
	struct sockaddr_in addr;
	int addrlen = sizeof(addr);	
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(PORT_NUMBER);

	// create socket file descriptor
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1){
		fprintf(stderr, "Failed to create listen socket\n");
		exit(1);
	}

	// bind server address and port to socket
	if (bind(sockfd, (struct sockaddr *)&addr, addrlen) == -1) {
		fprintf(stderr, "Failed to bind socket\n");
		close(sockfd);
		exit(1);
	}


	// set to listen for clients
	if (listen(sockfd, QUEUE_SIZE) == -1) {
		fprintf(stderr, "Failed to listen on socket\n");
		close(sockfd);
		exit(1);
	}	

	int num_clients = 0; // number of clients

	// main loop
	while(1){
		int client_fd = accept(sockfd, (struct sockaddr *)&addr, (socklen_t *)&addrlen);
		if (client_fd == -1){
			perror("accept");
			continue;
		}
		
		num_clients++;

		char welcome_message[BUFFER_SIZE];
		sprintf(welcome_message, "Welcome! Your client ID is %d\n", num_clients);

		send(client_fd, welcome_message, strlen(welcome_message), 0);

	}
}

