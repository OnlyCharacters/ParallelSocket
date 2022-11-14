#include <stdio.h>
/*
	perror()
	printf()
	fprintf()
*/
#include <stdlib.h>
/*
	exit()
*/
#include <string.h>
/*
	memset()
*/
#include <unistd.h>
/*
    read()
    write()
    struct sockaddr
*/
#include <netinet/in.h>
/*
    struct sockaddr_in
*/
#include <arpa/inet.h>
/*
	htonl()
	htons()
	ntohl()
	ntohs()
	inet_pton()
*/
#include <sys/socket.h>
#include <sys/types.h>
/*
	socket()
	accept()
	connect()
	send()
	recv()
	sockaddr
	sockaddr_in
*/
#include <sys/epoll.h>
/*
	epoll()
*/
#include <errno.h>
/*
	errno
*/

#define MAX_EVENTS 8
#define MAX_LISTEN 8
#define SERV_PORT 30000

extern void
service(int sockfd);

extern void
init_service(void);

int main(int argc, char **argv)
{
	int		n;
	int 	conn_sock;
	int 	listen_sock;
	socklen_t	socklen;
	struct sockaddr_in6 servaddr6;

	if ((listen_sock = socket(AF_INET6, SOCK_STREAM, 0)) == -1)
	{
		perror("socket");
		exit(1);
	}

	if (setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, (const void *)&n , sizeof(int)) < 0) {
		perror("setsockopt");
		exit(1);
	}

	memset(&servaddr6, 0, sizeof(struct sockaddr_in6));

	servaddr6.sin6_family		= AF_INET6;
	inet_pton(AF_INET6, "::", &servaddr6.sin6_addr);
	servaddr6.sin6_port			= htons(SERV_PORT);


	socklen = sizeof(struct sockaddr_in6);
	if (bind(listen_sock, (struct sockaddr*)&servaddr6, socklen) == -1) {
		perror("bind6");
		exit(1);
	}

	if (listen(listen_sock, MAX_LISTEN) == -1) {
		perror("listen");
		exit(1);
	}
	init_service();



	for ( ; ; ) {
		// new comes
		conn_sock = accept(listen_sock, NULL, NULL);
		fprintf(stderr, "accept\n");
		if (conn_sock == -1) {
			perror("accept");
			exit(1);
		}

		service(conn_sock);
	}
}
