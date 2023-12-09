
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

typedef struct client {
    int id;
    int fd;
}t_clients;

t_clients clients[1024];
int max;

void fatal_error()
{
    write(2,"fatal error\n",12);
    exit(1);
}

/*This function made to send to all client except sender and server*/
void send_to_clients(char msg[],int sender,fd_set master,int server)
{
    for (int i = 0; i <= max ;i++)
    {
        if (FD_ISSET(i,&master) && sender != i && server != i)
            send(i,msg,strlen(msg),0);
    }
}

/*Bring this function from main.c*/
int extract_message(char **buf, char **msg)
{
	char	*newbuf;
	int	i;

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

int main (int ac,char **av){
    if(ac != 2)
    {
        write(2, "Wrong number of arguments\n", 26);
        exit(1);
    }
    int server,len;
    int counter = 0;
    struct sockaddr_in servaddr, cli;
    char buffer[200000],bufWrite[200000];
    char str[100];
    fd_set master,rset;
    FD_ZERO(&master);
    FD_ZERO(&rset);
    server = socket(AF_INET, SOCK_STREAM, 0);
    if (server < 0)
        fatal_error();
    max = server;
    FD_SET(server,&master);
    bzero(&servaddr, sizeof(servaddr));
    bzero(buffer, sizeof(buffer));
     bzero(str, sizeof(str));
    servaddr.sin_family = AF_INET; 
	servaddr.sin_addr.s_addr = htonl(2130706433); //127.0.0.1
	servaddr.sin_port = htons(atoi(av[1]));
    if ((bind(server, (const struct sockaddr *)&servaddr, sizeof(servaddr))) != 0) 
        fatal_error();
	if (listen(server, 10) != 0)
        fatal_error();
    len = sizeof(cli);
    while(1)
    {
        rset = master;
        int ret = select(max + 1,&rset,NULL,NULL,NULL);
        if (ret <= 0)
            continue;
        for (int i = 0;i <= max;i++)
        {
            if (FD_ISSET(i,&rset) && i == server)
            {
                int fd = accept(server, (struct sockaddr *)&cli, (socklen_t*)&len);
                if (fd < 0)
                    continue;
                clients[fd].id = counter++;
                clients[fd].fd = fd;
                if (max < fd)
                    max = fd;
                FD_SET(fd,&master);
                sprintf(str,"server: client %d just arrived\n",clients[fd].id);
                send_to_clients(str,fd,master,server);
            }
            else if (FD_ISSET(i,&rset))
            {
                int res = recv(i,buffer,200000,0);
                if (res <= 0)
                {
                    bzero(str,sizeof(str));
                    sprintf(str,"server: client %d just left\n",clients[i].id);
                    send_to_clients(str,i,master,server);
                    FD_CLR(i,&master);
                    close(i);
                }
                 else
                {
                    buffer[res] = '\0';
                    char *msg = buffer;
                    char *save = NULL;
                    char join[200100];
                    while (extract_message(&msg,&save))
                    {
                        bzero(join,sizeof(join));
                        sprintf(join,"client %d: %s",clients[i].id,save);
                        send_to_clients(join,i,master,server);
                    }
                    bzero(buffer,sizeof(buffer));
                }
            }
        }
    }
}
