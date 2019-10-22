#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <stdbool.h>

#define BUFFER_SIZE 1024
#define QUEUE_SIZE 10
#define MAX_CHANNELS 255

int server_fd;
int client_fd;

// buffer for receiving messages from client
char client_buffer[BUFFER_SIZE*2];
	
// buffer for sending messages
char server_buffer[BUFFER_SIZE*2];

// struct for channels
typedef struct channel_data_t{
	bool is_subscribed;
	int total_messages;
	int unread_messages;
	int read_messages;
	char next_string;
} channel_data;

// initiate channels
channel_data channel[MAX_CHANNELS];

void clean_exit(int sig);	

void process_buffer(char* buffer);
void channels();
void sub(char* id);
void unsub(char* id);
void next(char* id);
void livefeed(char* id);
void send_message(char* id, char* message);
void bye();

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

	printf("Connection made! Client ID: %d\n\n", client_fd);

	// send client welcome message
	char welcome_message[BUFFER_SIZE];	
	sprintf(welcome_message, "Welcome! Your client ID is %d", client_fd);
	send(client_fd, welcome_message, strlen(welcome_message), 0);


	while (1){

		// clear buffers
		bzero(server_buffer, BUFFER_SIZE*2);
		bzero(client_buffer, BUFFER_SIZE*2);
		
		
		recv(client_fd, client_buffer, BUFFER_SIZE*2, 0); // receive client message
		
		printf("\n[CLIENT %d] -------------\n", client_fd);
		process_buffer(client_buffer);

		// send feedback
		send(client_fd, server_buffer, strlen(server_buffer), 0); // send feedback
	}

	close(server_fd);
	close(client_fd);



}


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

void process_buffer(char* buffer){
	
	// get command string from buffer
	char* buffer_cpy = strdup(buffer);
	char* command = strtok(buffer_cpy, " ");

	// get folllowing id, NULL if not
	char* id = strtok(NULL, " "); // id following command

	char message[BUFFER_SIZE]; // for message string
	bzero(message, BUFFER_SIZE);

	// attempt to store mesasge string if id is provided	
	if (id != NULL){		
		// starting position for message
		int start = strlen(command) + 1 + strlen(id) + 1; 
		// iterate and store characters from buffer, following start position 
		for (int i = start; i < strlen(buffer); i++){
			message[i - start] = buffer[i]; 
		}	
	}		
	
	// display and send feedback
	char feedback[BUFFER_SIZE*2];
	sprintf(feedback, "\nCommand: %s\nChannel ID: %s\nMessage: %s\n\n", command, id, message);
	printf("%s", feedback);

	
	if (strcmp(command, "CHANNELS") == 0){ channels(); }
	else if (strcmp(command, "SUB") == 0){ sub(id); }
	else if (strcmp(command, "UNSUB") == 0){ unsub(id); }
	else {
		printf("Invalid command or TODO\n");
	}

	
}

void channels(){
	

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

void sub(char* id){
	// TODO
}

void unsub(char* id){
	// TODO
}


