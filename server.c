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
#define MAX_CHANNELS 256
#define MAX_MESSAGES 1000

// file descriptors (socket handles)
int server_fd;
int client_fd;

// buffer for receiving messages from client
char client_buffer[BUFFER_SIZE*2];
	
// buffer for sending messages
char server_buffer[BUFFER_SIZE*2];

// structure for channel
typedef struct channel_t {
	bool subscribed;
	int total_messages;
	int unread_messages;
	int read_messages;
	char messages[MAX_MESSAGES][BUFFER_SIZE];
} channel;

// initialize array of channels
channel channels[MAX_CHANNELS] = {{
	.subscribed = false,
	.total_messages = 0,
	.unread_messages = 0,
	.read_messages = 0
}};

// server functions
void clean_exit(int sig);	
void process_buffer(char* buffer);
void view_channels();
void sub(char* id);
void unsub(char* id);
void next(char* id);
void livefeed(char* id);
void send_message(char* id, char message[BUFFER_SIZE]);
void bye();
int check_id(char* id);


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

	// given the command string, run corresponding function 	
	if (strcmp(command, "CHANNELS") == 0) { view_channels(); }
	else if (strcmp(command, "SUB") == 0) { sub(id); }
	else if (strcmp(command, "UNSUB") == 0) { unsub(id); }
	else if (strcmp(command, "NEXT") == 0) { next(id); }
	else if (strcmp(command, "LIVEFEED") == 0) { livefeed(id); }
	else if (strcmp(command, "SEND") == 0) { send_message(id, message); }
	else if (strcmp(command, "BYE") == 0) { bye(); }
	else { printf("Invalid command or TODO\n"); }
	
}


void view_channels(){

	// send channel information if subscribed	
	for (int i = 0; i<MAX_CHANNELS;i++){
		if (channels[i].subscribed == true){
			printf("%d\n", i);			
			memset(server_buffer, 0, sizeof(server_buffer));
			sprintf(server_buffer,"Channel: %d \tTotal messages sent : %d \tTotal messages read : %d \t Total messages unread: %d ", i,channels[i].total_messages,channels[i].read_messages,channels[i].unread_messages);
			send(client_fd, server_buffer, BUFFER_SIZE, 0);
		}
	}

	// flag for client to finish reading channels information
	memset(server_buffer, 0, sizeof(server_buffer));
	sprintf(server_buffer, "-1");
	send(client_fd, server_buffer, BUFFER_SIZE, 0);

}


void sub(char* id){

	bzero(server_buffer, sizeof(server_buffer));
	
	// if id is not provided, send error message
	if (id == NULL){
		send(client_fd, "Please provide a channel ID.\n", BUFFER_SIZE, 0);
		return;
	}

	// returns id as integer if valid, or -1 if invalid
	int id_int = check_id(id);	
	// if invalid, discontinue
	if (id_int == -1) { return; }

	// if channel is already subscribed to, inform client
	if (channels[id_int].subscribed == true){
		sprintf(server_buffer, "Already subscribed to channel %s", id);
		send(client_fd, server_buffer, BUFFER_SIZE, 0);
		return;
	}
	
	channels[id_int].subscribed = true;
	sprintf(server_buffer, "Subscribed to channel %s.", id);
	send(client_fd, server_buffer, BUFFER_SIZE, 0);	
}	


void unsub(char* id){
	// TODO
}


void next(char* id){
	// TODO
}


void livefeed(char* id){
	// TODO
}


void send_message(char* id, char message[BUFFER_SIZE]){
	
	// returns id as integer if valid, or -1 if invalid 
	int id_int = check_id(id);
	// if invalid, discontinue
	if (id_int == -1) { return; }
	printf("h\n");	
	

	// index in array of messages
	int index = channels[id_int].total_messages;
	// store message at index of messages, char. by char.	
	bzero(channels[id_int].messages[index], BUFFER_SIZE);
	for (int i = 0; i < BUFFER_SIZE; i++){
		channels[id_int].messages[index][i] = message[i];
	}

	printf("1\n");
	// increment counts
	channels[id_int].total_messages += 1;
	channels[id_int].unread_messages += 1;
	printf("2\n");
	// inform client the message has been sent
	bzero(server_buffer, sizeof(server_buffer));
	sprintf(server_buffer, "Message sent to channel %s.\n", id);
	send(client_fd, server_buffer, BUFFER_SIZE, 0);
/*	
channel channels[MAX_CHANNELS] = {{
	.subscribed = false,
	.total_messages = 0,
	.unread_messages = 0,
	.read_messages = 0
}};*/



}


void bye(){
	// TODO
}


// returns id as integer if valid, or -1 if invalid
int check_id(char* id){

	char* flag; // flag for valid number
	int id_int = strtol(id, &flag, 0); // id as int

	// if channel is invalid, send error message and return -1
	if (*flag != '\0' || id_int < 0 || id_int > 255){
		sprintf(server_buffer, "Invalid channel: %s\n", id);
		send(client_fd, server_buffer, BUFFER_SIZE, 0);
		return -1;
	}
	
	// return valid id integer
	return id_int;

}











