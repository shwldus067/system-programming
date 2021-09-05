#include "20171264.h"
#include "loader.h"

//PROJ3
struct est *est_head=NULL;
struct est *est_tail=NULL;
int reftable[256]={0,};

struct est* find_estab(char *symb){	//find symbol in estab
	struct est *tmp=est_head;
	while(tmp!=NULL){
		if(strcmp(tmp->name, symb)==0)	return tmp;
		tmp=tmp->nxt;
	}
	tmp=NULL;
	return tmp;
}
void estab_update(struct est *new_est){	//update estab
	if(est_head==NULL){	//first symbol
		est_head=new_est;
		est_tail=new_est;
		return;
	}
	else{
		est_tail->nxt=new_est;
		est_tail=new_est;
	}
}
void print_estab(){	//print estab
	int total_len=0;
	printf("%-15s %-15s %-15s %-15s\n", "control", "symbol", "address", "length");
	printf("%-15s %-15s\n", "section", "name");
	printf("------------------------------------------------------------\n");
	struct est *estmp=est_head;
	while(estmp!=NULL){
		if(estmp->len<0){	//symbol name
			printf("%-15s %-15s %04X\n", " ", estmp->name, estmp->addr);
		}else{	//control section
			printf("%-15s %-15s %04X%11s %04X%11s\n", estmp->name, " ", estmp->addr, " ", estmp->len, " ");
			total_len+=estmp->len;
		}
		estmp=estmp->nxt;
	}
	printf("------------------------------------------------------------\n");
	printf("%-15s %-15s %-15s %04X\n", " ", " ", "total length", total_len);
}
int pass1(char *file, int *csaddr){	//linking loader pass 1
	FILE *fp=fopen(file, "r");
	if(fp==NULL)	return -1;
	char s[101];
	int len=0, tt;
	while(fgets(s, sizeof(s), fp)!=NULL){	//get one line from file
		if(s[0]=='H'){	//header record
			struct est *new_est=(struct est*)malloc(sizeof(struct est));
			new_est->nxt=NULL;
			sscanf(s, "H%6s%06X%06X", new_est->name, &tt, &new_est->len);
			len=new_est->len;
			new_est->addr=*csaddr;
			estab_update(new_est);	//store the program name in estab
		}
		else if(s[0]=='D'){	//define record
			int size=strlen(s)-1;
			for(int i=1;i<size;i+=12){
				//make new estab node
				struct est *new_est=(struct est*)malloc(sizeof(struct est));
				int defaddr=0;
				new_est->nxt=NULL;
				sscanf(s+i, "%6s%06X", new_est->name, &defaddr);
				new_est->addr=*csaddr+defaddr;
				new_est->len=-1;
				estab_update(new_est);	//store the symbol in estab
			}
		}
	}
	*csaddr+=len;	//set the next csaddr
	fclose(fp);
	return 0;
}
int pass2(char *file, int *csaddr){	//linkin loader pass 2
	FILE *fp=fopen(file, "r");
	if(fp==NULL)	return -1;
	char s[100], name[10];
	int len, tt;
	while(fgets(s, sizeof(s), fp)!=NULL){	//get one line from file
		if(s[0]=='H'){	//header record
			sscanf(s, "H%6s%06X%06X", name, &tt, &len);
			reftable[1]=*csaddr;	//set reference number of program name
		}
		else if(s[0]=='R'){	//reference record
			int refnum, cnt;
			char *ptr=s+1;
			while(sscanf(ptr, "%02X%6s%n", &refnum, name, &cnt)>0){	//read reference data
				ptr+=cnt;
				struct est *estmp=find_estab(name);	//find externeal reference
				if(estmp==NULL){
					return -1;	//not in estab
				}
				reftable[refnum]=estmp->addr;	//set reference name
			}
		}
		else if(s[0]=='T'){	//text record
			int taddr, tlen, tval;
			sscanf(s, "T%06X%02X", &taddr, &tlen);	//read start address & length
			for(int i=0;i<tlen;++i){
				sscanf(s+9+i*2, "%02X", &tval);	//read data
				edit_f(*csaddr+taddr+i, tval);	//write data
			}
		}
		else if(s[0]=='M'){	//modification record
			int daddr, dlen, refnum, mval;
			unsigned int newval=0, tmp;
			char sign;
			sscanf(s, "M%06X%02X%c%02X", &daddr, &dlen, &sign, &refnum);	//read data
			mval=reftable[refnum];
			for(int i=0;i<3;++i){	//read a word
				tmp=(unsigned int)read_memory(*csaddr+daddr+i);
				newval<<=8;
				newval+=tmp;
			}
			if(dlen==5){	//if len==5, modify 5*4 bits
				if(sign=='+'){
					newval=((newval+mval)&0xFFFFF)+(newval&0xF00000);
				}else{
					newval=((newval-mval)&0xFFFFF)+(newval&0xF00000);
				}
			}else if(dlen==6){	//if len==6, modify 6*4 bits
				if(sign=='+'){
					newval+=mval;
				}else{
					newval-=mval;
				}
			}
			//store the modified value
			edit_f(*csaddr+daddr, (unsigned char)(newval>>16));
			edit_f(*csaddr+daddr+1, (unsigned char)(newval>>8));
			edit_f(*csaddr+daddr+2, (unsigned char)(newval));
		}
	}
	*csaddr+=len;	//set the next csaddr
	fclose(fp);
	return 0;
}
int loader_f(char (*file)[40], int cnt, int addr){	//linking loader
	for(int i=0;i<256;++i)	reftable[i]=0;	//reset the reference table
	struct est *estmp;
	est_tail=NULL;
	while(est_head!=NULL){	//free all the estab
		estmp=est_head;
		est_head=est_head->nxt;
		free(estmp);
	}
	int csaddr=addr;
	for(int i=0;i<cnt;++i){	//pass 1 for all files
		if(pass1(file[i], &csaddr)!=0){
			printf("loader pass1 error in file %d\n", i);
			return -1;
		}
	}
	csaddr=addr;
	for(int i=0;i<cnt;++i){	//pass2 for all files
		if(pass2(file[i], &csaddr)!=0){
			printf("loader pass2 error in file %d\n", i);
			return -1;
		}
	}
	print_estab();	//print estab
	est_tail=NULL;
	while(est_head!=NULL){	//free all the estab
		estmp=est_head;
		est_head=est_head->nxt;
		free(estmp);
	}
	return 0;
}
