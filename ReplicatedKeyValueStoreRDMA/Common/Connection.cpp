#include "Connection.hpp"
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

Buffer::Buffer(unsigned long long size) {
	this -> data = new char[size]();
	this -> currentSize = 0;
	this -> maxSize = size;
}

Buffer::~Buffer() {
	delete [] this -> data;
}

void Buffer::add(const char ch) {
	if (this -> currentSize == this -> maxSize) {
		unsigned int newMaxSize = this -> maxSize * 2 + 1;
		char* newData = (char*)malloc(newMaxSize * sizeof(char));
		memcpy(newData, this -> data, this -> currentSize);
		free(this -> data);
		this -> data = newData;
		this -> maxSize = newMaxSize;
	}
	this -> data[this -> currentSize] = ch;
	this -> currentSize++;
}

void Connection::die(std::string msg1, std::string msg2) {
	fprintf(stderr, "%s, %s\n", msg1, msg2);
	exit(0);
}

int Connection::Client::connect_to_server(char *server, const int port) {
	int clientfd;
	struct hostent *hp;
	struct sockaddr_in serveraddr;
	char errbuf[256]; /* for errors */

	/* create a socket */
	if((clientfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		die("Error creating socket: ", strerror(errno));
	}

	/* Fill in the server's IP address and port */
	if((hp = gethostbyname(server)) == NULL) {
		printf(errbuf, "%d", h_errno);
		die("DNS error: DNS error ", errbuf);
	}
	bzero((char *) &serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	bcopy((char *)hp->h_addr_list[0], (char *)&serveraddr.sin_addr.s_addr, hp->h_length);
	serveraddr.sin_port = htons(port);

	/* connect */
	if(connect(clientfd, (struct sockaddr *) &serveraddr, sizeof(serveraddr)) < 0) {
		die("Error connecting: ", strerror(errno));
	}
	return clientfd;
}

void Connection::shutdown(const int fd) {
  int rc;
	if((rc = close(fd)) < 0) {
		die("Close error: ", strerror(errno));
	}
}

int Connection::Server::open_server_socket(const int port) {
    int                listenfd;    /* the server's listening file descriptor */
    struct sockaddr_in addrs;       /* describes which clients we'll accept */
    int                optval = 1;  /* for configuring the socket */

    /* Create a socket descriptor */
    if((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        die("Error creating socket: ", strerror(errno));
    }

    /* Eliminates "Address already in use" error from bind. */
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval , sizeof(int)) < 0) {
        die("Error configuring socket: ", strerror(errno));
    }

    /* Listenfd will be an endpoint for all requests to the port from any IP address */
    bzero((char *) &addrs, sizeof(addrs));
    addrs.sin_family = AF_INET;
    addrs.sin_addr.s_addr = htonl(INADDR_ANY);
    addrs.sin_port = htons((unsigned short)port);
    if (bind(listenfd, (struct sockaddr *)&addrs, sizeof(addrs)) < 0) {
      die("Error in bind(): ", strerror(errno));
    }

    /* Make it a listening socket ready to accept connection requests */
    if(listen(listenfd, 1024) < 0) {
      die("Error in listen(): ", strerror(errno));
    }

    return listenfd;
}

int Connection::stream_data(const int socketid, const void* data, const long long size) {
	const char* buffer = (const char*)data;
	long long bytesRemaining = size;
	long long bytesSent;

	while (bytesRemaining > 0) {
		if ((bytesSent = write(socketid, buffer, bytesRemaining)) <= 0) {
			if (errno != EINTR) {
				fprintf(stderr, "Write error: %s\n", strerror(errno));
				return -1;
			}
			bytesSent = 0;
		}
		bytesRemaining -= bytesSent;
		buffer += bytesSent;
	}
	
	return 0;
}

Buffer* Connection::socket_get_line(const int socketid) {
	bool retry = true;
	char data = 0;
	int bytesRead;
	Buffer* message = new Buffer(BUFFER_SIZE);

	while (retry) {
		//read a single character
		if ((bytesRead = read(socketid, &data, 1)) < 0) {
			if (errno != EINTR) {
				fprintf(stderr, "Read error: %s\n", strerror(errno));
				delete message;
        return NULL;
			}
			continue;
		}
		
		//if nothing was read, connection ended
		if (bytesRead == 0) {
			break;
		}

		//add to buffer and check exit condition
		message -> add(data);
		if (data == '\n') {
			break;
		}
	}

	//if nothing was read, report an error
	if (message -> currentSize == 0) {
		fprintf(stderr, "Nothing read\n");
		delete message;
		return NULL;
	}

	return message;
}
