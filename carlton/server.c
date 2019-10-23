#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <time.h>

#define BUFFER_SIZE 1024
#define QUEUE_SIZE 10
#define MAX_CHANNELS 255

/////////////////////////////////////////////////////////////////////////////////////
// Global variable area
typedef struct channel_data_t{
	bool is_subscribed;
	int total_messages;
	int unread_messages;
	int read_messages;
	char next_string;
} channel_data;

channel_data channel[MAX_CHANNELS];

// Server socked ID
int server_fd;

// Socket ID for sending and receiving data
int client_fd;

// buffer for receiving messages from client
char client_buffer[BUFFER_SIZE];

// buffer for sending messages
char server_buffer[BUFFER_SIZE];

//////////////////////////////////////////////////
/// FUNCTION AREA

void delay(int s){
	int milli_seconds = 1000*s;

	clock_t start = clock();
	while (clock() < start + milli_seconds){

	}
}

void send_channel_data(){
	

	printf("ABout to send subscription data\n");

	for (int i = 0; i<MAX_CHANNELS;i++){
		if (channel[i].is_subscribed == true){
			memset(server_buffer, 0, sizeof(server_buffer));
			sprintf(server_buffer,"Channel: %d \tTotal messages sent : %d \tTotal messages read : %d \t Total messages unread: %d ", i,channel[i].total_messages,channel[i].read_messages,channel[i].unread_messages);
			send(client_fd, server_buffer, BUFFER_SIZE, 0);
		}
	}

	printf("I sent all the channel data\n");

	memset(server_buffer, 0, sizeof(server_buffer));
	sprintf(server_buffer, "-1");
	send(client_fd, server_buffer, BUFFER_SIZE, 0);
}

void check_option(char *choice_str){

	//TESTING FOR CHANNEL INPUT
	if (strcmp("CHANNELS",choice_str) == 0){

		//char channel_message[] = {"YOU WILL GET YOUR CHANNELS"};
		//send(client_fd, "YOU WILL GET YOUR CHANNELS", strlen("YOU WILL GET YOUR CHANNELS"), 0);
		send_channel_data();

		}
	else {
		
		memset(server_buffer, 0, sizeof(server_buffer));
		sprintf(server_buffer, "Invalid command. enter option on list\n");
		send(client_fd, server_buffer, strlen(server_buffer), 0);
		printf("sent nothing!\n");

	}
	
}


////////////////////////////////////////////////

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
	sprintf(server_buffer, "Welcome! Your client ID is %d", client_fd);
	send(client_fd, server_buffer, strlen(server_buffer), 0);
	
	while (1){

		// clear buffers
		memset(server_buffer, 0, sizeof(server_buffer));
		memset(client_buffer, 0, sizeof(client_buffer));
		
		recv(client_fd, client_buffer, BUFFER_SIZE, 0); // receive client message

		printf("[CLIENT %d]: %s\n", client_fd, client_buffer); // display messaeck
		
		check_option(client_buffer);
	}

	close(server_fd);
	close(client_fd);



}

