# client-server-chatroom

## Build

```bash
make clean
make
```
## Usage

### Connect

First the server should be hosted before clients can connect.
```bash
./server [PORT]
```
If no port is supplied, the default port of 12345 is used. An example server setup:
```bash
./server 12345
```

Clients should now be able to connect using a seperate terminal.
```bash
./client [ADDRESS] [PORT]
```
An example for when the server is hosted locally on port 12345:
```bash
./client 127.0.0.1 12345
```

Both server and client terminals should indicate a connection has been made. 

### Commands
