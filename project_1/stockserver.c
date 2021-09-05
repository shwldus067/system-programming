#include "data.h"

int echo(int connfd);

int main(int argc, char **argv) 
{
    int listenfd, connfd;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;  /* Enough space for any address */ 
	 //line:netp:echoserveri:sockaddrstorage
    char client_hostname[MAXLINE], client_port[MAXLINE];

	//for select
	fd_set master;
	fd_set read_fds;
	int fdmax, clientcnt=0;
	
    if (argc != 2) {
		fprintf(stderr, "usage: %s <port>\n", argv[0]);
		exit(0);
    }

    listenfd = Open_listenfd(argv[1]);
    
    FD_SET(listenfd, &master);
    fdmax=listenfd;
    
    while(1){
    	read_fds=master;
    	if(select(fdmax+1, &read_fds, NULL, NULL, NULL)==-1){
    		perror("select");
    		exit(1);
		}
		for(int i=0;i<=fdmax;i++){
			if(FD_ISSET(i, &read_fds)){
				if(i==listenfd){	//client connection request
					clientlen = sizeof(struct sockaddr_storage); 
					connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
					if(connfd==-1){
						perror("accept");
					}else{
						clientcnt++;
						if(clientcnt==1)	read_stock();
						FD_SET(connfd, &master);
						if(connfd>fdmax){
							fdmax=connfd;
						}
						Getnameinfo((SA *) &clientaddr, clientlen, client_hostname, MAXLINE, 
									client_port, MAXLINE, 0);
						printf("Connected to (%s, %s)\n", client_hostname, client_port);
					}
				}
				else{	//client send message
					int nbyte=echo(i);
					if(nbyte<=0){	//error or connection exit
						if(nbyte<0)	perror("rio_readlineb");
						printf("close connectioin\n");
						Close(i);
						FD_CLR(i, &master);
						clientcnt--;
						if(clientcnt==0)	free_stock();
					}
				}
			}
		}
	}
	
	/*
    while (1) {
		clientlen = sizeof(struct sockaddr_storage); 
		connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
		Getnameinfo((SA *) &clientaddr, clientlen, client_hostname, MAXLINE, 
					client_port, MAXLINE, 0);
		printf("Connected to (%s, %s)\n", client_hostname, client_port);
		echo(connfd);
		Close(connfd);
    }*/
    exit(0);
}
/* $end echoserverimain */
