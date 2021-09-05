/*
 * echo - read and echo text lines until client closes connection
 */
/* $begin echo */
#include "data.h"

int echo(int connfd) 
{
    int n=0, id, val;
    char buf[MAXLINE], wbuf[MAXLINE];
    struct item *stock=NULL;
    rio_t rio;

    Rio_readinitb(&rio, connfd);
//	while((n = Rio_readlineb(&rio, buf, MAXLINE)) != 0) {
	if((n = Rio_readlineb(&rio, buf, MAXLINE)) != 0) {
		memset(wbuf, 0, sizeof(wbuf));
		printf("server received %d bytes\n", n);
		if(strncmp(buf, "show", 4)==0){
			show_stock(connfd);
		}else if(strncmp(buf, "buy", 3)==0){
			sscanf(buf+4, "%d %d", &id, &val);
			stock=search_stock(id);
			if(stock!=NULL){
				int ret=writer(-val, stock);
				if(ret<0)	sprintf(wbuf, "Not enough left stock\n");
				else	sprintf(wbuf, "[buy] success\n");
			}else{
				sprintf(wbuf, "no such stock\n");
			}
			wbuf[strlen(wbuf)]='\n';
			Rio_writen(connfd, wbuf, strlen(wbuf));
		}else if(strncmp(buf, "sell", 4)==0){
			sscanf(buf+4, "%d %d", &id, &val);
			stock=search_stock(id);
			if(stock!=NULL){
				int ret=writer(val, stock);
				if(ret<0)	sprintf(wbuf, "[sell] something wrong\n");
				else	sprintf(wbuf, "[sell] success\n");
			}else{
				sprintf(wbuf, "no such stock\n");
			}
			wbuf[strlen(wbuf)]='\n';
			Rio_writen(connfd, wbuf, strlen(wbuf));
		}else if(strncmp(buf, "exit", 4)==0){
			Rio_writen(connfd, "", 0);
		}
		else{
			sprintf(wbuf, "there's no matching\n");
			wbuf[strlen(wbuf)]='\n';
			Rio_writen(connfd, wbuf, strlen(wbuf));
		}
    }
    return n;
}
/* $end echo */

