#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>

#define QUEUE_SIZE 10

int socket_fd;

void clean_shutdown();

int main(int argc, const char** argv){

	// exit signal handler
	signal(SIGINT,clean_shutdown);

	// set port number
	uint16_t PORT_NUMBER = 12345;
	if (argc == 2){ PORT_NUMBER = 12345; }

	// setup server address and port
	struct sockaddr_in server_addr;
	int addr_length = sizeof(server_addr);	
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_port = htons(PORT_NUMBER);

	// create socket file descriptor
	if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1){
		fprintf(stderr, "Failed to create listen socket\n");
		exit(1);
	}

	// bind server address and port to socket
	if (bind(socket_fd, (struct sockaddr *)&server_addr, addr_length) == -1) {
		fprintf(stderr, "Failed to bind socket\n");
		close(socket_fd);
		exit(1);
	}


	// listen
	if (listen(socket_fd, QUEUE_SIZE) == -1) {
		fprintf(stderr, "Failed to listen on socket\n");
		close(socket_fd);
		exit(1);
	}	



	// main loop
	while(1){
	
	}
}

void clean_shutdown(int signum){
	
	/* TODO

	deal  elegantly  with  any  threads  that  have  been  created  
	as  well  as  any  open  sockets,  shared  memory  regions,  
	dynamically allocated memory and/or open files
	
	*/

	close(socket_fd);
	exit(1);

}


