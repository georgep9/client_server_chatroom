#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <stdbool.h>

#define BUFFER_SIZE 1024

int server_fd;
struct sockaddr_in server_addr;

int parallel_fd; // parallel socket for handling NEXT and CHANNELS

// buffer for receiving messages from server
char server_buffer[BUFFER_SIZE*2];

// buffer for sending messages
char client_buffer[BUFFER_SIZE*2];

// buffers used for parallel socket (NEXT and LIVEFEED)
char pclient_buffer[BUFFER_SIZE];
char pserver_buffer[BUFFER_SIZE];

void connect_server(char **argv);
void* runtime(void* p);
void process_commands(char* buffer);
void channels_prompt();
void sub_unsub_prompt();
void* next_prompt(void* p);
void send_prompt();
void* livefeed_prompt(void* p);
void bye();
void end_feed();
void sigint_exit(int sig);

pthread_t next_thread;
pthread_t livefeed_thread;
bool parallel_is_running = false;

int main(int argc, char **argv){
	signal(SIGINT, bye);

	// expect 2 input arguments
	if (argc < 3) {
		printf("Please provide hostname and port.\n");	
		exit(1);
	}

	///////////////////////////////////////////////////////////
	connect_server(argv);
	////////////////////////////// THE WHILE LOOP WAS PREVIOUSLY HERE
	runtime(NULL); 
	/////////////////////////////////////////////////////////////

	close(server_fd);
	close(parallel_fd);
}

void connect_server(char **argv){
	// assign hostname and port values
	char* server_name = argv[1];
	int PORT = atoi(argv[2]);
	
	// create client socket
	if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1){
		fprintf(stderr, "Failed to create socket\n");
		exit(1);	
	}

	// setup struct of server address and port
	inet_pton(AF_INET, server_name, &server_addr.sin_addr);
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(PORT);

	// connect to server socket
	if (connect(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
		fprintf(stderr, "Failed to connect.\n");
		close(server_fd);
		exit(1);
	}

	parallel_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (parallel_fd == -1){
		perror("Failed to create parallel socket\n");
		exit(1);
	}
	if (connect(parallel_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1){
		perror("Failed to connect parallel socket\n");
		exit(1); 
	}


	// read and print welcome message from server
	char welcome_message[BUFFER_SIZE];
	read(server_fd, welcome_message, BUFFER_SIZE);
	printf("%s\n", welcome_message);

	
}

void* runtime(void* p){
	while (1){

		// clear buffers
		bzero(server_buffer, sizeof(server_buffer));
		bzero(client_buffer, sizeof(client_buffer));
		
		printf("\n");

		// attempt to store user input
		char* check = fgets(client_buffer, sizeof(client_buffer), stdin); 

		// error or EOF (ungraceful exit)
		if (check == NULL){
			bye();
		}

		// only newline
		if (strcmp(client_buffer, "\n") == 0){ continue; }
		
		// remove newline char
		client_buffer[strlen(client_buffer) - 1] = 0;	
		
		send(server_fd, client_buffer, sizeof(client_buffer), 0); // send to server		
		process_commands(client_buffer);
	
	}
	return NULL;
}

void process_commands(char* buffer){


	char* buffer_cpy = strdup(buffer);
	char* command = strtok(buffer_cpy, " ");
	
	if (strcmp(command, "CHANNELS") == 0) { channels_prompt(); }
	else if (strcmp(command, "SUB") == 0 || strcmp(command, "UNSUB") == 0){
		sub_unsub_prompt();
	}
	else if (strcmp(command, "SEND") == 0) { send_prompt(); } 
	else if (strcmp(command, "NEXT") == 0) {
		if (parallel_is_running == true){
			printf("Outstanding NEXT or LIVEFEED commands have been issued.\n");
		}
		else {
			pthread_create(&next_thread, NULL, next_prompt, NULL);
			parallel_is_running = true;
		}
	}
	else if (strcmp(command, "LIVEFEED") == 0){
		if (parallel_is_running == true){
			printf("Outstanding NEXT or LIVEFEED commands have been issued.\n");
		}
		else {
			pthread_create(&livefeed_thread, NULL, livefeed_prompt, NULL);
			parallel_is_running = true;
		}
	}
	else if (strcmp(command, "BYE") == 0){ bye(); }
	else if (strcmp(command, "STOP") == 0) {
		pthread_cancel(next_thread);
		pthread_cancel(livefeed_thread);
		parallel_is_running = false;
		signal(SIGINT, bye);
	}
	else { printf("Invalid command.\n"); }

}

void channels_prompt(){
	
	recv(server_fd,server_buffer,BUFFER_SIZE,0);
	while ((strcmp(server_buffer,"-1")) != 0 )
	{
		printf("%s\n",server_buffer);

		//Clear buffer and take next line of data
		memset(server_buffer, 0, sizeof(server_buffer));
		recv(server_fd,server_buffer,BUFFER_SIZE,0);
		
	}

}

void sub_unsub_prompt(){
	bzero(server_buffer, sizeof(server_buffer));
	recv(server_fd, server_buffer, BUFFER_SIZE, 0);
	printf("%s\n", server_buffer);
}

void* next_prompt(void* p){

	bzero(pserver_buffer, sizeof(pserver_buffer));
	recv(parallel_fd, pserver_buffer, BUFFER_SIZE, 0);
	printf("%s\n", pserver_buffer);
	
	parallel_is_running = false;
	
	return NULL;
}	

void send_prompt(){
	bzero(server_buffer, sizeof(server_buffer));
	recv(server_fd, server_buffer, BUFFER_SIZE, 0);
	printf("%s\n", server_buffer);
}

void* livefeed_prompt(void *p){
	
	signal(SIGINT,end_feed);

	bzero(pserver_buffer,sizeof(pserver_buffer));

	// if client exits livefeed, server will receive a "-1" and in
	// response will send "-1" back to indicate livefeed is over
	while (strcmp(pserver_buffer,"-1") != 0){
		
		// message to display or idling empty string
		bzero(pserver_buffer,sizeof(pserver_buffer));
		recv(parallel_fd, pserver_buffer, BUFFER_SIZE, 0);

		// if not idle empty string, or exit flag, display message
		if (strcmp(pserver_buffer," \n") != 0 && strcmp(pserver_buffer,"-1") != 0 ){
			printf("%s\n", pserver_buffer);
		}
		
		// send empty string back to server (livefeed continues)
		bzero(pclient_buffer,sizeof(pclient_buffer));
		sprintf(pclient_buffer," \n");
		send(parallel_fd, pclient_buffer, BUFFER_SIZE, 0);
	
	}

	signal(SIGINT, bye);

	parallel_is_running = false;
		
	return NULL;
}

void bye(){
	close(server_fd);
	close(parallel_fd);
	exit(1);
}



void end_feed(){
	bzero(client_buffer, sizeof(client_buffer));
	sprintf(client_buffer,"-1");
	send(parallel_fd,client_buffer,BUFFER_SIZE, 0);

	

	//runtime();
}

// safely exit client process
void sigint_exit(int sig){
	
	// TODO

	bzero(client_buffer, sizeof(client_buffer));
	strncpy(client_buffer, "BYE", BUFFER_SIZE);
	send(server_fd, client_buffer, sizeof(client_buffer), 0);
	
	bye();
}
	
