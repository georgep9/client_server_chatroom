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

// The Server address struct and its length
struct sockaddr_in addr;
int addrlen = sizeof(addr);

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

int subscriptions = 0; // amount of channels subscribed to

// linked list for keeping track of message ordering of
// different channels
typedef struct next_node  next_node_t;
struct next_node {
	int channel_id;
	next_node_t* next;
};
next_node_t *head;
next_node_t *end;

// server functions
void connect_client();
void runtime();
void process_buffer(char* buffer);
void view_channels();
void sub(char* id);
void unsub(char* id);
void next(char* id);
void livefeed(char* id);
void send_message(char* id, char message[BUFFER_SIZE]);
void bye();
void clear_old_messages(int id);
void livefeed_all();
int check_id(char* id);

void clean_exit(int sig);	


int main(int argc, const char** argv){

	// exit signal handler
	signal(SIGINT,clean_exit);
	
	// set port number
	uint16_t PORT_NUMBER = 12345;
	if (argc == 2){ PORT_NUMBER = atoi(argv[1]); }

	// setup server address and port	
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

	connect_client();
	////////////////////////////// THE WHILE LOOP WAS PREVIOUSLY HERE
	runtime();
	/////////////////////////////////////////////////////////////

	close(server_fd);
	close(client_fd);

}

void connect_client(){
	
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
	bzero(welcome_message, BUFFER_SIZE);
	sprintf(welcome_message, "Welcome! Your client ID is %d", client_fd);
	send(client_fd, welcome_message, BUFFER_SIZE, 0);

}

void runtime(){


	while (1){

		// clear buffers
		bzero(server_buffer, sizeof(server_buffer));
		bzero(client_buffer, sizeof(client_buffer));
		
		
		recv(client_fd, client_buffer, sizeof(client_buffer), 0); // receive client message
		
		// client has ungracefully disconnected	
		if (strlen(client_buffer) == 0) {
			bye();
			continue;
		}
		
		printf("\n[CLIENT %d] -------------\n", client_fd);
		process_buffer(client_buffer);

	}
}

void process_buffer(char* buffer){
	
	// get command string from buffer
	char* buffer_cpy = strdup(buffer);
	char* command = strtok(buffer_cpy, " ");

	// get channel id string, NULL if not provided
	char* id = strtok(NULL, " "); // id following command

	char message[BUFFER_SIZE]; // buffer to store message string
	bzero(message, BUFFER_SIZE);

	// attempt to store message string if id is provided	
	if (id != NULL){		
		// starting position for message
		int start = strlen(command) + 1 + strlen(id) + 1; 
		// iterate and store characters from buffer, following start position 
		for (int i = start; i < strlen(buffer); i++){
			message[i - start] = buffer[i]; 
		}	
	}		
		
	// display feedback
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
		strncpy(server_buffer, "Please provide a channel ID.", sizeof(server_buffer));
		send(client_fd, server_buffer, BUFFER_SIZE, 0);
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
	clear_old_messages(id_int);
	sprintf(server_buffer, "Subscribed to channel %s.", id);
	send(client_fd, server_buffer, BUFFER_SIZE, 0);	

	subscriptions += 1;


}	


void unsub(char* id){


	bzero(server_buffer, sizeof(server_buffer));
	// if id is not provided, send error message
	if (id == NULL){
		strncpy(server_buffer, "Please provide a channel ID.", sizeof(server_buffer));
		send(client_fd, server_buffer, BUFFER_SIZE, 0);
		return;
	}

	// returns id as integer if valid, or -1 if invalid
	int id_int = check_id(id);	
	// if invalid, discontinue
	if (id_int == -1) { return; }

	// if channel is already subscribed to, unsub
	if (channels[id_int].subscribed == false){
		sprintf(server_buffer, "Not subscribed to channel %s.", id);
		send(client_fd, server_buffer, BUFFER_SIZE, 0);
		return;
	}
	
	channels[id_int].subscribed = false;
	sprintf(server_buffer, "Unsubscribed to channel %s", id);
	send(client_fd, server_buffer, BUFFER_SIZE, 0);

	subscriptions -= 1;

	// remove sent messages of unsubscribed channel from linked list
	next_node_t* curr = head;
	next_node_t* prev = curr;
	next_node_t* temp;
	for (; curr!=NULL; curr=curr->next){
		if (curr->channel_id == id_int){
			if (curr == head){
				if (curr != end){
					temp = curr;
					head = head->next;
					free(temp);
				} else {
					free(curr);
					head = NULL;
					end = NULL;
					break;
				}
			}
			else if (curr == end){
				end = prev;
				free(curr);
				break;
			}
			else {
				temp = curr;
				prev->next = curr->next;
				free(temp);
				curr = prev;
			}
		}
		prev = curr;
	}

}


void next(char* id){
		
	bzero(server_buffer, sizeof(server_buffer));


	if (id == NULL){
		if (subscriptions == 0){
			strncpy(server_buffer, "Not subscribed to any channels.", BUFFER_SIZE);
			send(client_fd, server_buffer, BUFFER_SIZE, 0);
		}
		else if (head != NULL){
			char head_id[3];
			int id_int = head->channel_id;
			sprintf(head_id, "%d", id_int);
			next(head_id);
		}
		else {
			strncpy(server_buffer, " ", BUFFER_SIZE);
			send(client_fd, server_buffer, BUFFER_SIZE, 0); 
		}	
		
		return;
	}


	// returns id as integer if valid, or -1 if invalid
	int id_int = check_id(id);	
	// if invalid, discontinue
	if (id_int == -1) { return; }


	if (channels[id_int].subscribed == false){
		sprintf(server_buffer, "Not subscribed to channel %s.", id);
		send(client_fd, server_buffer, BUFFER_SIZE, 0);
		return;
	}

	// index of next unread message
	int index = channels[id_int].total_messages - channels[id_int].unread_messages; 	
	// if none unread, message is empty string of index awaiting SEND

	// load unread message into server buffer
	sprintf(server_buffer, "%d: %s", id_int, channels[id_int].messages[index]);
	
	// adjust counts if index is not for empty string awaiting SEND
	if (index != channels[id_int].total_messages){
		channels[id_int].read_messages += 1;
		channels[id_int].unread_messages -= 1;	
	} 

	send(client_fd, server_buffer, BUFFER_SIZE, 0); // send message


	// update linked list of channels order of sent messages	
	next_node_t* temp = head;
	next_node_t* prev = head;
	for (; temp != NULL; temp=temp->next){
		if (temp->channel_id == id_int){
			if (temp == head){
				if (head != end){
					head = head->next;
					free(temp);
				}
				else { // if node is head & end
					free(temp);
					head = NULL; 
					end = NULL; 
				}
			} 
			else if (temp == end){
				end = prev;
				end->next = NULL;
				free(temp);
			}
			else {
				prev->next = temp->next;
				free(temp);	
			}
			break;
		}
		prev = temp;
	}	
}


void livefeed(char* id){
	// Here we loop through the NEXT command for the specified channel
	// Once all the messages are sent, the loop will continue, and only execute ...
	// NEXT when the number of unread messages is > than 0

	// parameterless LIVEFEED
	
	if (id == NULL){
		livefeed_all();
		return;
	}

	// returns id as integer if valid, or -1 if invalid
	int id_int = check_id(id);	
	// if invalid, discontinue
	if (id_int == -1) { 
		
		// Sends livestream disabling sequence to client
		sprintf(server_buffer, "-1");
		send(client_fd, server_buffer, BUFFER_SIZE, 0);
		return; 
	}

	if (channels[id_int].subscribed == false){
		sprintf(server_buffer, "Not subscribed to channel %s.", id);
		send(client_fd, server_buffer, BUFFER_SIZE, 0);

		// Sends livestream disabling sequence to client
		sprintf(server_buffer, "-1");
		send(client_fd, server_buffer, BUFFER_SIZE, 0);
		return;
	}

	while (strcmp(client_buffer,"-1") != 0)
	{
		bzero(client_buffer, BUFFER_SIZE*2);
		if (channels[id_int].unread_messages > 0){
			next(id);
		}else{
			sprintf(server_buffer," \n");
			send(client_fd,server_buffer, BUFFER_SIZE,0);
			//printf("All messages sent\n");
		}
		recv(client_fd, client_buffer, BUFFER_SIZE*2, 0); // receive client message
	

		// client has ungracefully disconnected
		if (strlen(client_buffer) == 0) {
			bye();
			return;		
		}
	}

	// Will return client to normal operation
	bzero(server_buffer, sizeof(server_buffer));
	sprintf(server_buffer,"-1");
	send(client_fd,server_buffer, BUFFER_SIZE,0);
	
}

void livefeed_all(){

	bzero(client_buffer, sizeof(client_buffer));
	bzero(server_buffer, sizeof(server_buffer));
		
	if (subscriptions == 0) {
		sprintf(server_buffer, "Not subscribed to any channels.");
		send(client_fd, server_buffer, BUFFER_SIZE, 0);
		bzero(server_buffer, sizeof(server_buffer));
		sprintf(server_buffer, "-1");
		send(client_fd, server_buffer, BUFFER_SIZE, 0);
		// flags from client closing livefeed
		recv(client_fd, client_buffer, BUFFER_SIZE, 0);
		recv(client_fd, client_buffer, BUFFER_SIZE, 0);
		return;
	}
	
	while(strcmp(client_buffer, "-1") != 0){
		if (head != NULL){
			next(NULL);
		}
		else {
			bzero(server_buffer, sizeof(server_buffer));
			sprintf(server_buffer, " \n");
			send(client_fd, server_buffer, BUFFER_SIZE, 0);
		}
		bzero(client_buffer, sizeof(client_buffer));
		recv(client_fd, client_buffer, BUFFER_SIZE, 0);
		
		// if client has ungracefully disconnected
		if (strlen(client_buffer) == 0) {
			bye();
			return;		
		}
					
	}	
		
	// will return client to normal operation
	bzero(server_buffer, sizeof(server_buffer));
	sprintf(server_buffer,"-1");
	send(client_fd,server_buffer, BUFFER_SIZE,0);
	
}


void send_message(char* id, char message[BUFFER_SIZE]){
	
	// returns id as integer if valid, or -1 if invalid 
	int id_int = check_id(id);
	// if invalid, discontinue
	if (id_int == -1) { return; }

	// index in array of messages
	int index = channels[id_int].total_messages;
	// store message at index of messages, char. by char.	
	bzero(channels[id_int].messages[index], BUFFER_SIZE);
	sprintf(channels[id_int].messages[index], "%s", message);

	// inform client the message has been sent
	bzero(server_buffer, sizeof(server_buffer));
	sprintf(server_buffer, "Message sent to channel %s.", id);
	send(client_fd, server_buffer, BUFFER_SIZE, 0);

		
	// increment counts
	channels[id_int].total_messages += 1;
	channels[id_int].unread_messages += 1;
	
	if (channels[id_int].subscribed == true){
		next_node_t * new_next = (next_node_t*)malloc(sizeof(next_node_t));
		new_next->channel_id = id_int;		
		new_next->next = NULL;
		if (end != NULL){
			end->next = new_next;
		}
		else{
			head = new_next; 
		}
		end = new_next;
	}	

}


void bye(){
	for (int i = 0; i<=255;i++){
		channels[i].subscribed = false;
		channels[i].unread_messages = 0;
		channels[i].read_messages = 0;
	}
	printf("Client %d has disconnected.\n", client_fd);
	close(client_fd);
	connect_client();
}


// returns id as integer if valid, or -1 if invalid
int check_id(char* id){

	char* flag; // flag for valid number
	int id_int = strtol(id, &flag, 0); // id as int

	// if channel is invalid, send error message and return -1
	if (*flag != '\0' || id_int < 0 || id_int > 255){
		sprintf(server_buffer, "Invalid channel: %s", id);
		send(client_fd, server_buffer, BUFFER_SIZE, 0);
		return -1;
	}
	
	// return valid id integer
	return id_int;

}

void clear_old_messages(int id) {
	// This function is used inside SUB to clear any prehistoric messages associated with the channels
	// The NEXT command is not suposed to return any messages sent before subscription

	channels[id].read_messages = 0;
	channels[id].unread_messages = 0;

	// Cleaning away historical messages
	for (int i=0; i<MAX_MESSAGES;i++){
		bzero(channels[id].messages[i], sizeof(channels[id].messages[i]));
	}

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
