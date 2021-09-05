#include "csapp.h"

struct item{
	int ID;
	int left_stock;
	int price;
	int readcnt;
	sem_t mutex, w;
};

struct node{
	struct item *data;
	struct node *left;
	struct node *right;
};
extern struct node *root;

struct stk{
	struct node *data;
	struct stk *pre;
};

struct item* search_stock(int ID);
void insert_stock(struct item *newitem);
void read_stock(void);
void reader(char (*buf)[], struct item *out);
int writer(int val, struct item *out);
void show_stock(int connfd);
void free_stock();
