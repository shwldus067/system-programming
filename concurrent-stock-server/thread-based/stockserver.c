#include "data.h"
#include "sbuf.h"
#define NTHREADS 1024

int echo(int connfd);
sem_t mutex;
sbuf_t sbuf;
int clientcnt=0;

void *thread(void *vargp){
	Pthread_detach(pthread_self());
//	while(1){
		int connfd=sbuf_remove(&sbuf), n=0;
		P(&mutex);	//print connection information first 
		V(&mutex);
		while(1){
			n=echo(connfd);
			if(n==0)	break;
		}
		Close(connfd);
		printf("connection closed\n");
		P(&mutex);
		clientcnt--;
		if(clientcnt==0)	free_stock();
		V(&mutex);
//	}
	return NULL;
}

int main(int argc, char **argv) 
{
    int listenfd, connfd;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;  /* Enough space for any address */  //line:netp:echoserveri:sockaddrstorage
    char client_hostname[MAXLINE], client_port[MAXLINE];
	pthread_t tid;
    
		if (argc != 2) {
	fprintf(stderr, "usage: %s <port>\n", argv[0]);
	exit(0);
    }

    listenfd = Open_listenfd(argv[1]);
    sbuf_init(&sbuf, NTHREADS);
    for(int i=0;i<NTHREADS;i++)	Pthread_create(&tid, NULL, thread, NULL);
    Sem_init(&mutex, 0, 1);
    while (1) {
		clientlen = sizeof(struct sockaddr_storage); 
		connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
		P(&mutex);
		clientcnt++;
		if(clientcnt==1)	read_stock();
		sbuf_insert(&sbuf, connfd);
        Getnameinfo((SA *) &clientaddr, clientlen, client_hostname, MAXLINE, 
                    client_port, MAXLINE, 0);
        printf("Connected to (%s, %s)\n", client_hostname, client_port);
        V(&mutex);
//	echo(connfd);
//	Close(connfd);
    }
    exit(0);
}
/* $end echoserverimain */
