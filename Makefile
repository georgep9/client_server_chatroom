#compiler and flags
CC = gcc
CFLAGS = -Wall -pthread 

#headers
DEPS = 

#objects
SERVER_OBJ = server.o 
CLIENT_OBJ = client.o

#executables
BINARIES = server client

#targets to make
all: $(BINARIES)

#create object files
%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

#compile server binary
server: $(SERVER_OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

#compile client binary
client: $(CLIENT_OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

#remove binaries and object files
clean:
	rm -rf $(BINARIES) *.o

.PHONY: clean all
