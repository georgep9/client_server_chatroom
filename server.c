#include <stdlib.h>
#include <stdio.h>
#include <signal.h>

void shutdown(int signum){
	
	/* TODO

	deal  elegantly  with  any  threads  that  have  been  created  
	as  well  as  any  open  sockets,  shared  memory  regions,  
	dynamically allocated memory and/or open files
	
	*/

	printf("test\n");
	exit(0);

}

int main(int argc, const char** argv){


	signal(SIGINT,shutdown);

	// main loop
	while(1){
		
	}
}