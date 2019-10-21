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

int server_fd;
int client_fd;

void clean_exit(int signum){
	
	/* TODO

	deal  elegantly  with  any  threads  that  have  been  created  
	as  well  as  any  open  sockets,  shared  memory  regions,  
	dynamically allocated memory and/or open files
	
	*/

	close(server_fd);
	close(client_fd);
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
	if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1){
		fprintf(stderr, "Failed to create listen socket\n");
		exit(1);
	}

	// bind server address and port to socket
	if (bind(server_fd, (struct sockaddr *)&addr, addrlen) == -1) {
		fprintf(stderr, "Failed to bind socket\n");
		close(server_fd);
		exit(1);
	}


	// set to listen for clients
	if (listen(server_fd, QUEUE_SIZE) == -1) {
		fprintf(stderr, "Failed to listen on socket\n");
		close(server_fd);
		exit(1);
	}	


	

	printf("Waiting for a client to connect...\n");

	client_fd = accept(server_fd, (struct sockaddr *)&addr, (socklen_t *)&addrlen);
	if (client_fd == -1){
		perror("accept");
	}

	printf("Connection made! Client ID: %d\n", client_fd);

	// send client welcome message
	char welcome_message[BUFFER_SIZE];	
	sprintf(welcome_message, "Welcome! Your client ID is %d", client_fd);
	send(client_fd, welcome_message, strlen(welcome_message), 0);


	while (1){
		// buffer for receiving messages from client
		char client_buffer[BUFFER_SIZE*2];

		// buffer for sending messages
		char server_buffer[BUFFER_SIZE*2];
	
		// clear buffers
		bzero(server_buffer, BUFFER_SIZE*2);
		bzero(client_buffer, BUFFER_SIZE*2);
		
		
		recv(client_fd, client_buffer, BUFFER_SIZE*2, 0); // receive client message

		// store client command from buffer
		char* buffer_cpy = strdup(client_buffer);
		char* client_command = strtok(buffer_cpy, " ");
		int cmd_len = strlen(client_command) + 1; // command string length, including space

		// store client arguments (following command) from buffer
		char client_args[BUFFER_SIZE*2];
		bzero(client_args, BUFFER_SIZE*2);
		for(int i = cmd_len; i < strlen(client_buffer); i++){
			client_args[i - cmd_len] = client_buffer[i];
		}
	
		printf("[CLIENT %d]: %s\n", client_fd, client_buffer); // display messaeck

		// feedback message for client	
		sprintf(server_buffer, "\n[SERVER]\nCommand: %s\nArguments: %s\n", client_command, client_args);	

		// send feedback
		send(client_fd, server_buffer, strlen(server_buffer), 0); // send feedback
	}

	close(server_fd);
	close(client_fd);



}

