#include "data.h"
struct node *root;

struct item* search_stock(int ID){
	struct node *cur=root;
	while(cur!=NULL){
		if(cur->data->ID==ID)	return cur->data;
		else if(cur->data->ID>ID)	cur=cur->left;
		else	cur=cur->right;
	}
	return NULL;
}

void insert_stock(struct item *newitem){
	struct node *cur=root, *pre=NULL;
	struct node *tmp=(struct node*)malloc(sizeof(struct node));
	tmp->data=newitem;
	tmp->left=tmp->right=NULL;
	if(root==NULL){
		root=tmp;
		return;
	}
	int dir=0;
	while(cur!=NULL){
		pre=cur;
		if(cur->data->ID<newitem->ID)	cur=cur->right, dir=0;
		else cur=cur->left, dir=1;
	}
	if(dir==0)	pre->right=tmp;
	else	pre->left=tmp;
}

void read_stock(){
	char buf[MAXLINE];
	FILE *fp;
	fp=fopen("stock.txt", "r");
	root=NULL;
	int id, lstock, price;
	while(fgets(buf, sizeof(buf), fp)!=NULL){
		sscanf(buf, "%d %d %d", &id, &lstock, &price);
		struct item *newitem=(struct item*)malloc(sizeof(struct item));
		newitem->ID=id;
		newitem->left_stock=lstock;
		newitem->price=price;
		newitem->readcnt=0;
		Sem_init(&(newitem->mutex), 0, 1);
		Sem_init(&(newitem->w), 0, 1);
		insert_stock(newitem);
	}
	fclose(fp);
}

void reader(char (*buf)[], struct item *out){
	char buff[MAXLINE];
	memset(buff, 0, sizeof(buff));
	P(&(out->mutex));
	out->readcnt++;
	if(out->readcnt==1)	P(&(out->w));
	V(&(out->mutex));
	//read
	sprintf(buff, "%d %d %d\n", out->ID, out->left_stock, out->price);
	strcat(*buf, buff);
	P(&(out->mutex));
	out->readcnt--;
	if(out->readcnt==0)	V(&(out->w));
	V(&(out->mutex));
}

int writer(int val, struct item *out){
	int ret=-1;
	P(&(out->w));
	if(out->left_stock+val>=0)	out->left_stock+=val, ret=0;
	V(&(out->w));
	return ret;
}

void show_stock(int connfd){
	struct stk *top=NULL;
	struct node *cur=root;
	char buf[MAXLINE];
	memset(buf, 0, sizeof(buf));
	while(1){
		while(cur!=NULL){
			struct stk *tmp=(struct stk*)malloc(sizeof(struct stk));
			tmp->data=cur;
			tmp->pre=top;
			top=tmp;
			cur=cur->left;
		}
		if(top==NULL)	break;
		struct stk *tmp1=top;
		struct item *out=tmp1->data->data;
		cur=tmp1->data->right;
		top=top->pre;
		free(tmp1);
		reader(&buf, out);
	}
	buf[strlen(buf)]='\n';
	Rio_writen(connfd, buf, strlen(buf));
}

void free_stock(){
	FILE *fp;
	fp=fopen("stock.txt", "w");
	struct stk *top=(struct stk*)malloc(sizeof(struct stk)), *tmp, *tmp1;
	struct item *out;
	top->data=root;
	top->pre=NULL;
	while(top!=NULL){
		tmp=top;
		top=top->pre;
		out=tmp->data->data;
		fprintf(fp, "%d %d %d\n", out->ID, out->left_stock, out->price);
		if(tmp->data->right){
			tmp1=(struct stk*)malloc(sizeof(struct stk));
			tmp1->data=tmp->data->right;
			tmp1->pre=top;
			top=tmp1;
		}
		if(tmp->data->left){
			tmp1=(struct stk*)malloc(sizeof(struct stk));
			tmp1->data=tmp->data->left;
			tmp1->pre=top;
			top=tmp1;
		}
		free(tmp->data->data);
		free(tmp->data);
		free(tmp);
	}
	fclose(fp);
}
