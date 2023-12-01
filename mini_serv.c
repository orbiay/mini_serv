#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <printf.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <stdio.h>

typedef struct s_client {
	int		id;
	char	*msg;
}	t_client;

t_client	client[OPEN_MAX];
int			max;

void fatal_err()
{
	write(2, "fatal error\n", 12);
	exit(1);
}

void send_to_clients(char buffer[], int connfd, fd_set master, int server)
{
	for (int i = 0; i <= max; i++)
	{
		if (FD_ISSET(i, &master) && i != server && i != connfd)
			send(i, buffer, strlen(buffer), 0);
	}
}

// Bring this Function From main.c
int	extract_message(char **buf, char **msg)
{
	char	*newbuf;
	int		i;

	*msg = 0;
	if (*buf == 0)
		return (0);
	i = 0;
	while ((*buf)[i])
	{
		if ((*buf)[i] == '\n')
		{
			newbuf = calloc(1, sizeof(*newbuf) * (strlen(*buf + i + 1) + 1));
			if (newbuf == 0)
				return (-1);
			strcpy(newbuf, *buf + i + 1);
			*msg = *buf;
			(*msg)[i + 1] = 0;
			*buf = newbuf;
			return (1);
		}
		i++;
	}
	return (0);
}

char	*str_join(char *buf, char *add)
{
	char	*newbuf;
	int		len;

	if (buf == 0)
		len = 0;
	else
		len = strlen(buf);
	newbuf = malloc(sizeof(*newbuf) * (len + strlen(add) + 1));
	if (newbuf == 0)
		return (0);
	newbuf[0] = 0;
	if (buf != 0)
		strcat(newbuf, buf);
	free(buf);
	strcat(newbuf, add);
	return (newbuf);
}

int main(int ac, char **av)
{
	if (ac != 2)
	{
		write(2, "Wrong number of arguments\n", 26);
		exit(1);
	}
	int sockfd, next_id = 0, len;
	struct sockaddr_in servaddr, cli;
	fd_set master, fdset;
	// socket create and verification 
	sockfd = socket(AF_INET, SOCK_STREAM, 0); 
	if (sockfd < 0)
		fatal_err();
	FD_ZERO(&master);
	FD_ZERO(&fdset);
	FD_SET(sockfd, &master);
	bzero(&servaddr, sizeof(servaddr)); 
	// assign IP, PORT 
	servaddr.sin_family = AF_INET; 
	servaddr.sin_addr.s_addr = htonl(2130706433); //127.0.0.1
	servaddr.sin_port = htons(atoi(av[1]));
	// Binding newly created socket to given IP and verification 
	if ((bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr))) != 0)
		fatal_err();
	if (listen(sockfd, 10) != 0) 
		fatal_err();
	max = sockfd;
	len = sizeof(cli);
	while (1)
	{
		fdset = master;
		int r = select(max + 1, &fdset, 0,0,0);
		if (r <= 0)
			continue;
		for (int i=0;i<=max;i++)
		{
			char	buffer[200000];
			
			bzero(buffer, sizeof(buffer));
			if (FD_ISSET(i, &fdset) && i == sockfd)
			{
				//new incomming connection
				int connfd = accept(sockfd, (struct sockaddr *)&cli, (socklen_t*)&len);
				
				if (connfd < 0)
					continue;
				client[connfd].id = next_id;
				if (connfd > max)
					max = connfd;
				FD_SET(connfd, &master);
				sprintf(buffer, "server: client %d just arrived\n", next_id++);
				send_to_clients(buffer, connfd, master, sockfd);
			}
			else if (FD_ISSET(i, &fdset))
			{
				//somebody is sending a msg or leaving
				int res= recv(i, buffer, sizeof(buffer)-1, 0);
				
				if (res <= 0)
				{
					bzero(buffer, sizeof(buffer));
					sprintf(buffer, "server: client %d just left\n", client[i].id);
					send_to_clients(buffer, i, master, sockfd);
					FD_CLR(i, &master);
					close(i);
				}
				else
				{
					buffer[res] = 0;
					client[i].msg = str_join(client[i].msg, buffer);
					char	*msg = NULL;
					char	join[200100];
					while (extract_message(&client[i].msg, &msg))
					{
						bzero(join, sizeof(join));
						sprintf(join, "client %d: %s", client[i].id, msg);
						send_to_clients(join, i, master, sockfd);
						free(msg);
					}
				}
			}
		}
	}
}
