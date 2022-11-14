#include <stdlib.h>
#include <stdio.h>
#include <string.h>
/*
    strerror()
*/
#include <unistd.h>
/*
    read()
    write()
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
#include <errno.h>
/*
	errno
*/

#define MAX_CONN 3

struct Client {
    int v4fd;
    int v6fd;
    unsigned long id;
};

struct Client*
NewClient(void) {
    struct Client* cli;
    cli = (struct Client*)calloc(1, sizeof(struct Client));
    return cli;
}

// For main process to handle v4 and v6 client
struct Client* clients[MAX_CONN];

void
DeleClient(struct Client* cli) {
    free(cli);
}

void
init_service(void)
{
    for (int i = 0; i < MAX_CONN; i++)
        clients[i] = NewClient();
}

ssize_t
Read(int fd, void *buf, size_t size) {
   ssize_t retval;
   while (retval = read(fd, buf, size), retval == -1 && errno == EINTR) ;
   return retval;
}

ssize_t
Writen(int fd, const void *vptr, size_t n)
{
	size_t		nleft;
	ssize_t		nwritten;
	const char	*ptr;

	ptr = vptr;
	nleft = n;
	while (nleft > 0) {
		if ( (nwritten = write(fd, ptr, nleft)) <= 0) {
			if (nwritten < 0 && errno == EINTR)
				nwritten = 0;
			else
				return(-1);
		}

		nleft -= nwritten;
		ptr   += nwritten;
	}
	return(n);
}

void
handle_client(struct Client* cli)
{
    ssize_t nread;
    char buff[64];
    // char temp[128];
    char v4[INET6_ADDRSTRLEN];
    char v6[INET6_ADDRSTRLEN];
    char sndbuff[256];
    socklen_t socklen;
    struct sockaddr_in6 addr4;
    struct sockaddr_in6 addr6;

    memset(&addr4, 0, sizeof(struct sockaddr_in6));
    memset(&addr6, 0, sizeof(struct sockaddr_in6));

    socklen = sizeof(struct sockaddr_in6);
    if (getpeername(cli->v4fd, (struct sockaddr*)&addr4, &socklen) != 0) {
        perror("getsockname");
    }
    if (getsockname(cli->v6fd, (struct sockaddr*)&addr6, &socklen) != 0) {
        perror("getsockname");
    }

    inet_ntop(AF_INET6, (struct sockaddr*)&addr4.sin6_addr, v4, socklen);
    inet_ntop(AF_INET6, (struct sockaddr*)&addr6.sin6_addr, v6, socklen);

    for ( ; ; ) {
        if ((nread = Read(cli->v4fd, buff, sizeof(buff))) < 0) {
            perror("Read");
            close(cli->v4fd);
            close(cli->v6fd);
            exit(0);
        }
        // Client EOF
        else if (nread == 0) {
            close(cli->v4fd);
            close(cli->v6fd);
            exit(0);
        }
        // Read Data and send ACK
        else {
            char* p = strchr(buff, '\n');
            if (p) *p = '\0';
            fprintf(stderr, "client ID %ld send %s with ipv4 %s\n", cli->id, buff, v4);

            snprintf(sndbuff, sizeof(sndbuff), "You are ID %ld, sent msg %s with v4 %s, server send back to you with v6 %s\n",
                cli->id, buff, v4, v6);

            Writen(cli->v6fd, sndbuff, strlen(sndbuff));
        }
    }
}

void
service(int fd) {
    int i;
    char buff[64];
    ssize_t nread;
    unsigned long id;
    socklen_t socklen;
    struct sockaddr_in6 addr6;

    socklen = sizeof(struct sockaddr_in6);
    memset(&addr6, 0, sizeof(struct sockaddr_in6));


    if ((nread = Read(fd, buff, sizeof(buff))) < 0) {
        perror("Read");
        close(fd);
    }
    // Client EOF
    else if (nread == 0)
        close(fd);
    // Read Data
    else {
        id = strtoul(buff, NULL, 10);

         if (id == 0) {
            fprintf(stderr, "Error request\n");
            close(fd);
        }

        if (getpeername(fd, (struct sockaddr*)&addr6, &socklen) != 0) {
            perror("getsockname");
            close(fd);
        }

        fprintf(stderr, "ID %ld comes with ip %s\n", id,
            inet_ntop(AF_INET6, &addr6.sin6_addr, buff, socklen));

        // Check if someone who has same id
        for (i = 0; i < MAX_CONN; i++) {
            // Found
            if (clients[i]->id == id) {
                if (IN6_IS_ADDR_V4MAPPED(&addr6.sin6_addr) == 1) // This is an ipv4 address
                    clients[i]->v4fd = fd;
                else // This is an ipv6 address
                    clients[i]->v6fd = fd;
                if (fork() == 0) {
                    close(3);
                    handle_client(clients[i]);
                }
                else {
                    clients[i]->id = 0;
                    close(clients[i]->v4fd);
                    close(clients[i]->v6fd);
                }
                break;
            }
        }
        // No same id, new comes
        if (i == MAX_CONN) {
            for (i = 0; i < MAX_CONN; i++) {
                if (clients[i]->id == 0) {
                    clients[i]->id = id;
                    if (IN6_IS_ADDR_V4MAPPED(&addr6.sin6_addr) == 1) // This is an ipv4 address
                        clients[i]->v4fd = fd;
                    else // This is an ipv6 address
                        clients[i]->v6fd = fd;
                    break;
                }
            }
        }
        // buffer if full, close this socket
        if (i == MAX_CONN)
            close(fd);
    }
}
