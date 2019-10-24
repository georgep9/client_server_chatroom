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

// buffer for receiving messages from server
char server_buffer[BUFFER_SIZE*2];

// buffer for sending messages
char client_buffer[BUFFER_SIZE*2];
	
// safely exit client process
void clean_exit(int sig){
	
	// TODO

	close(server_fd);
	exit(1);
}

void process_commands(char* buffer);
void channels_prompt();
void sub_unsub_prompt();
void next_prompt();
void send_prompt();
void livefeed_prompt();
void runtime();
void end_feed();
void connect_client(char **argv);

int main(int argc, char **argv){
	signal(SIGINT, clean_exit);

	// expect 2 input arguments
	if (argc < 3) {
		printf("Please provide hostname and port.\n");	
		exit(1);
	}

	///////////////////////////////////////////////////////////
	connect_client(argv);
	////////////////////////////// THE WHILE LOOP WAS PREVIOUSLY HERE
	runtime();
	/////////////////////////////////////////////////////////////

	close(server_fd);
}

void process_commands(char* buffer){
	char* buffer_cpy = strdup(buffer);
	char* command = strtok(buffer_cpy, " ");
	
	if (strcmp(command, "CHANNELS") == 0) { 
		channels_prompt(); 
	}
	else if (strcmp(command, "SUB") == 0 || strcmp(command, "UNSUB") == 0){
		sub_unsub_prompt();
	}
	else if (strcmp(command, "NEXT") == 0){
		next_prompt();
	}
	else if (strcmp(command, "LIVEFEED") == 0) { livefeed_prompt(); } 
	else if (strcmp(command, "SEND") == 0) { send_prompt(); } 
	else if (strcmp(command, "BYE") == 0){ clean_exit(SIGINT);}
	else{
		printf("Invalid command or TODO\n");
	}

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

void next_prompt(){
	
	bzero(server_buffer, sizeof(server_buffer));
	recv(server_fd, server_buffer, BUFFER_SIZE, 0);
	printf("%s\n", server_buffer);
}	

void send_prompt(){
	bzero(server_buffer, sizeof(server_buffer));
	recv(server_fd, server_buffer, BUFFER_SIZE, 0);
	printf("%s\n", server_buffer);
}

void livefeed_prompt(){
	signal(SIGINT,end_feed);

	while (strcmp(server_buffer," \n\n") != 0){

		bzero(server_buffer, sizeof(server_buffer));
		bzero(client_buffer, sizeof(client_buffer));

		recv(server_fd, server_buffer, BUFFER_SIZE, 0);
		
		if (strcmp(server_buffer," \n") != 0 && strcmp(server_buffer," \n\n") != 0 ){
			printf("%s\n", server_buffer);
		}

		// send to server blank messages because it is expecting a terminating -1 (which is sent after SIGINT is flagged)
		sprintf(client_buffer," \n");
		send(server_fd, client_buffer, strlen(client_buffer), 0);
	}
	
}

void runtime(){
	signal(SIGINT, clean_exit);
	while (1){

		// clear buffers
		bzero(server_buffer, BUFFER_SIZE*2);
		bzero(client_buffer, BUFFER_SIZE*2);
	
		printf("\nInput: ");
		
		// message from user input
		char* check = fgets(client_buffer, BUFFER_SIZE*2, stdin); 
		if (check == NULL || strlen(check) == 1){
			continue; // error or only newline
		 }
		
		// remove newline char
		client_buffer[strlen(client_buffer) - 1] = 0;	
		
		process_commands(client_buffer);
		send(server_fd, client_buffer, strlen(client_buffer), 0); // send to server		
	
	}
}

void end_feed(){
	signal(SIGINT, clean_exit);
	sprintf(client_buffer,"-1");
	send(server_fd,client_buffer,strlen(client_buffer), 0);

	return;

	//runtime();
}

void connect_client(char **argv){
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
	char welcome_message[BUFFER_SIZE];
	read(server_fd, welcome_message, BUFFER_SIZE);
	printf("%s\n", welcome_message);
}
	