/*
 * echoclient.c - An echo client
 */
/* $begin echoclientmain */
#include "csapp.h"

int main(int argc, char **argv) 
{
    int clientfd;
    char *host, *port, buf[MAXLINE];
    rio_t rio;

    if (argc != 3) {
	fprintf(stderr, "usage: %s <host> <port>\n", argv[0]);
	exit(0);
    }
    host = argv[1];
    port = argv[2];

    clientfd = Open_clientfd(host, port);
    Rio_readinitb(&rio, clientfd);

    while (Fgets(buf, MAXLINE, stdin) != NULL) {
    	if(strncmp(buf, "exit", 4)==0){
				break;
			}
	Rio_writen(clientfd, buf, strlen(buf));
	int n=0;
	while((n=Rio_readlineb(&rio, buf, MAXLINE))!=0){
		if(n==1)	break;
		Fputs(buf, stdout);
	}
    }
    Close(clientfd); //line:netp:echoclient:close
    exit(0);
}
/* $end echoclientmain */
