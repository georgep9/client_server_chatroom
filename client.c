#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 1024

int server_fd;

// safely exit client process
void clean_exit(int sig){
	
	// TODO

	close(server_fd);
	exit(1);
}

int main(int argc, char **argv){

	signal(SIGINT, clean_exit);

	// buffer for receiving messages from server
	char server_buffer[BUFFER_SIZE];

	// buffer for sending messages
	char client_buffer[BUFFER_SIZE];
	
	// expect 2 input arguments
	if (argc < 3) {
		printf("Please provide hostname and port.\n");	
		exit(1);
	}
	// assign hostname and port values
	char* server_name = argv[1];
	int PORT = atoi(argv[2]);
	
	// create client socket
	if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1){
		fprintf(stderr, "Failed to create socket\n");
		exit(1);	
	}

	// setup struct of server address and port
	struct sockaddr_in server_addr;
	inet_pton(AF_INET, server_name, &server_addr.sin_addr);
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(PORT);

	// connect to server socket
	if (connect(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
		fprintf(stderr, "Failed to connect.\n");
		close(server_fd);
		exit(1);
	}

	// read and print welcome message from server
	read(server_fd, server_buffer, BUFFER_SIZE);
	printf("%s\n", server_buffer);

	while (1){

		// clear buffers
		memset(server_buffer, 0, sizeof(server_buffer));
		memset(client_buffer, 0, sizeof(client_buffer));
	
		printf("Input: ");
		
		// message from user input
		char* check = fgets(client_buffer, BUFFER_SIZE, stdin); 
		if (check == NULL || strlen(check) == 1){
			continue; // error or only newline
		 }

		// remove newline char
		client_buffer[strlen(client_buffer) - 1] = 0;	

		send(server_fd, client_buffer, strlen(client_buffer), 0); // send to server

		recv(server_fd, server_buffer, BUFFER_SIZE, 0); // feedback from server
		
		printf("%s\n", server_buffer); // display feedback from server
	}

	close(server_fd);

}
