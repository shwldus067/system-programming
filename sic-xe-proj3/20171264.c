#include "20171264.h"
#include "assemble.h"
#include "loader.h"
#include "run.h"

//PROJ1
static struct cmd *history_head=NULL;
static struct cmd *history_tail=NULL;
static int his_cnt=1;
static int running=1;

//memory space 16*65536 bytes, 65536 rows(lines) & 16 cols(index)
#define max_mem 1048576
#define max_chr 256
#define max_input 10000000
static char hexadecimal[16]={'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
static unsigned char Memory[max_mem];
static int last_addr=0;

static struct opcode *OPCODE[20];

//PROJ2
static struct sym *symbol_table;

//PROJ3
static int progaddr=0, loaded=0;	//program start address & check the program was loaded
unsigned char read_memory(int addr){	//read the memory at the address
	return Memory[addr];
}
int find_format(int op){	//to run the program find format of the opcode
	char code[3], tmp;
	tmp=(char)(op%16);
	if(tmp>9)	code[1]=tmp-10+'A';
	else	code[1]=tmp+'0';
	tmp=(char)(op>>4);
	if(tmp>9) code[0]=tmp-10+'A';
	else code[0]=tmp+'0';
	code[2]=0;
	struct opcode *cur;
	for(int i=0;i<20;++i){	//check all opcode nodes
		cur=OPCODE[i];
		while(cur!=NULL){
			if(strncmp(cur->code, code, 2)==0){
				if(strncmp(cur->type, "1", 1)==0)	return 1;
				else if(strncmp(cur->type, "2", 1)==0)	return 2;
				else	return 3;
			}
			cur=cur->nxt;
		}
	}
	return -1;
}

//initialize memory 0
void Memory_initialize(){
	for(int i=0;i<max_mem;++i)	Memory[i]=0;
}

void make_opcode_hash_table(){	//initialize opcode hash table
	FILE *fo=fopen("opcode.txt", "r");
	char op_c[3], op_m[10], op_t[4];
	int hash;
	for(int i=0;i<58;++i){
		fscanf(fo, "%s", op_c);
		fscanf(fo, "%s", op_m);
		fscanf(fo, "%s", op_t);
		hash=((op_m[0]-'A')%20);
		struct opcode *cur=(struct opcode*)malloc(sizeof(struct opcode));
		strncpy(cur->code, op_c, strlen(op_c)+1);
		strncpy(cur->mnemonic, op_m, strlen(op_m)+1);
		strncpy(cur->type, op_t, strlen(op_t)+1);
		cur->nxt=OPCODE[hash];
		OPCODE[hash]=cur;
	}
	fclose(fo);
}

/*command function*/

/*Shell related operations*/
//help function
void help_f(){
	printf("h[elp]\n");
	printf("d[ir]\n");
	printf("q[uit]\n");
	printf("hi[story]\n");
	printf("du[mp] [start, end]\n");
	printf("e[dit] address, value\n");
	printf("f[ill] start, end, value\n");
	printf("reset\n");
	printf("opcode mnemonic\n");
	printf("opcodelist\n");
	printf("assemble filename\n");	//PROJ2
	printf("type filename\n");
	printf("symbol\n");
	printf("progaddr [address]\n");	//PROJ3
	printf("loader [object filename1] [object filename2] [object filename3]\n");
	printf("bp [address]\n");
	printf("bp clear\n");
	printf("bp\n");
	printf("run\n");
	return;
}
//dir function
void dir_f(){
	DIR *dir;
	struct dirent *dd;
	struct stat st;
	dir=opendir("./");
	if(dir!=NULL){
		while((dd=readdir(dir))!=NULL){
			stat(dd->d_name, &st);
			if(S_ISDIR(st.st_mode))	printf("\t%s/\n", dd->d_name);
			else if(S_IXUSR & st.st_mode)	printf("\t%s*\n", dd->d_name);
			else	printf("\t%s\n", dd->d_name);
		}
		closedir(dir);
	}
	else{
		printf("[ERROR] Cannot Open Directory\n");
	}
}
//quit function, free memory & quit the program
void quit_f(){
	Memory_initialize();
	struct cmd *cur=history_head;
	struct cmd *tmp;
	struct opcode *OP, *tp;
	while(cur!=NULL){
		tmp=cur->nxt;
		free(cur);
		cur=tmp;
	}
	for(int i=0;i<20;++i){
		OP=OPCODE[i];
		while(OP!=NULL){
			tp=OP->nxt;
			free(OP);
			OP=tp;
		}
	}
	running=0;
}
//history function
void history_update(char *str, int num){	//make linked list history
	struct cmd *new_cmd=(struct cmd*)malloc(sizeof(struct cmd));
	new_cmd->his_num=his_cnt++;
	strncpy(new_cmd->token, str, num);
	new_cmd->token[num]=0;
	new_cmd->nxt=NULL;
	if(history_head==NULL)	history_head=new_cmd;
	else	history_tail->nxt=new_cmd;
	history_tail=new_cmd;
}
void history_f(){	//print history
	struct cmd *cur=history_head;
	while(cur!=NULL){
		printf("\t%d\t%s\n", cur->his_num, cur->token);
		cur=cur->nxt;
	}
}

/*Memory related operations*/
//dump fuunction
void make_address(char *chr, int addr){	//make memory address(0~FFFFF) in 5
	for(int k=4;k>=0;--k){
		chr[k]=hexadecimal[addr%16];
		addr/=16;
	}
}
void dump_f(int *address, int num){	//print num lines from address(1line: 16bytes)
	if(num==0)	return;
	if(*address>=max_mem){
		printf("[ERROR] Wrong Command\n");
		return;
	}
	char addr[6], mem[3];
	int tmp_mem;
	addr[5]=mem[2]=0;
	for(int i=0;i<num;++i){
		make_address(addr, *address);
		printf("%s ", addr);
		for(int j=0;j<16;++j){
			tmp_mem=Memory[*address+j];
			mem[0]=hexadecimal[tmp_mem/16%16];
			mem[1]=hexadecimal[tmp_mem%16];
			printf("%s ", mem);
		}
		printf("; ");
		for(int a=0;a<16;++a){
			tmp_mem=Memory[*address+a];
			if(31<tmp_mem && tmp_mem<127)	printf("%c", tmp_mem);
			else	printf(".");
		}
		printf("\n");
		*address+=16;
		if(*address==max_mem){
			*address=0;
			break;
		}
	}
}
void dump_part(int address, int from, int to){	//print memory from~to index in one line(address)
	if(address>=max_mem){
		printf("[ERROR] Wrong Command\n");
		return;
	}
	char addr[6], mem[3];
	addr[5]=mem[2]=0;
	int tmp_mem;
	make_address(addr, address);
	printf("%s ", addr);
	for(int i=0;i<from;++i)	printf("   ");
	for(int k=from;k<to;++k){
		tmp_mem=Memory[address+k];
		mem[0]=hexadecimal[tmp_mem/16%16];
		mem[1]=hexadecimal[tmp_mem%16];
		printf("%s ", mem);
	}
	for(int i=to;i<16;++i)	printf("   ");
	printf("; ");
	for(int i=0;i<from;++i)	printf(".");
	for(int a=from;a<to;++a){
		tmp_mem=Memory[address+a];
		if(31<tmp_mem && tmp_mem<127)	printf("%c", tmp_mem);
		else	printf(".");
	}
	for(int i=to;i<16;++i)	printf(".");
	printf("\n");
}
void dump_start_f(int start){	//du[mp] start
	if(start>=max_mem){
		printf("[ERROR] Wrong Command\n");
		return;
	}
	int first_line=start/16*16, first_idx=start%16;
	dump_part(first_line, first_idx, 16);	//print first line
	first_line+=16;
	if(first_line==max_mem)	return;
	dump_f(&first_line, 9);	//print 9 lines
	if(first_idx==0)	return;
	if(first_line==0)	return;
	dump_part(first_line, 0, first_idx);	//print last line
}
void dump_start_end_f(int start, int end){	//du[mp] start, end
	if(end>=max_mem){
		printf("[ERROR] Wrong Command\n");
		return;
	}
	int first_line=start/16*16, first_idx=start%16;
	int last_line=end/16*16, last_idx=end%16;
	if(first_line==last_line){
		dump_part(first_line, first_idx, last_idx+1);	//if from and to are in one line
		return;
	}
	dump_part(first_line, first_idx, 16);	//print first line
	first_line+=16;
	if(first_line==max_mem)	return;
	dump_f(&first_line, (last_line-first_line)/16);	//print first+1~last-1 lines
	if(first_line==0)	return;
	dump_part(last_line, 0, last_idx+1);	//print last line
}
//edit function
void edit_f(int addr, int val){
	if(addr>=max_mem){
		printf("[ERROR] Wrong Command\n");
		return;
	}
	if(val>=max_chr){
		printf("[ERROR] Wrong Command\n");
		return;
	}
	Memory[addr]=val;
}
//fill function
void fill_f(int start, int end, int val){
	if(start>=max_mem){
		printf("[ERROR] Wrong Command\n");
		return;
	}
	if(val>=max_chr){
		printf("[ERROR] Wrong Command\n");
		return;
	}
	for(int a=start;a<=end;++a){
		Memory[a]=val;
	}
}
//reset function
void reset_f(){
	Memory_initialize();
}

/*OPCODE related operation*/
//opcode function	//PROJ2 revised
struct opcode* opcode_f(char *mnem){
	int hash=(mnem[0]-'A')%20;
	struct opcode *cur=OPCODE[hash];
	while(cur!=NULL){
		if(strncmp(mnem, cur->mnemonic, 7)==0){	//find opcode
			return cur;
		}
		cur=cur->nxt;
	}
	return NULL;
}
//opcodelist function
void opcodelist_f(){
	struct opcode *cur;
	for(int i=0;i<20;++i){
		cur=OPCODE[i];
		printf("\t%d :", i);
		if(cur!=NULL){
			printf(" [%s, %s]", cur->mnemonic, cur->code);
			cur=cur->nxt;
		}
		while(cur!=NULL){
			printf(" -> [%s, %s]", cur->mnemonic, cur->code);
			cur=cur->nxt;
		}
		printf("\n");
	}
}

//PROJ2
//type filename function
int type_f(char *file_name){
	FILE *fp=fopen(file_name, "r");
	if(fp==NULL){
		return -1;
	}
	char chr;
	while((chr=fgetc(fp))!=EOF){
		printf("%c", chr);
	}
	printf("\n");
	fclose(fp);
	return 0;
}

/*input functions*/
int is_num(char num){	//check hexadecimal(true:1, false:0)
	if('0'<=num && num<='9')	return 1;
	if('a'<=num && num<='f')	return 1;
	if('A'<=num && num<='F')	return 1;
	return 0;
}
int ch_to_num(char num){	//change hexadecimal number from char to int 
	if('0'<=num && num<='9')	return num-'0';
	if('a'<=num && num<='f')	return num-'a'+10;
	if('A'<=num && num<='F')	return num-'A'+10;
	return -1;
}
//input parameter (-2:input error, -1:no parameter, else:input value)
int input_param(char *chr, int *idx, char* last_input){
	char inp=*last_input;
	int pre=*idx, ret=0;
	while(inp==' ' || inp=='\t')	inp=getchar();
	if(inp!='\n'){
		while(is_num(inp)){
			chr[(*idx)++]=inp;
			ret*=16;
			ret+=ch_to_num(inp);
			if(ret>=max_input){
				return -2;
			}
			inp=getchar();
		}
		if(pre==*idx)	return -2;
		if(inp==' ' || inp=='\t' || inp==',' || inp=='\n'){
			*last_input=inp;
			return ret;
		}
		return -2;
	}
	*last_input=inp;
	return -1;
}
int is_letter(char* chr){	//check lowercase letter
	if('a'<=*chr && *chr<='z')	return 1;
	return 0;
}
int is_mnemonic(char *chr){	//check mnemonic(capital)
	if('A'<=*chr && *chr<='Z')	return 1;
	return 0;
}
//input command
int command_input(){
	static char command[101]={0, };
	int command_index=0, wrong_command=0;
	int first=0, second=0;
	char command_in;
	while((command_in=getchar())==' ' || command_in=='\t');
	if(!is_letter(&command_in)){
		while(command_in!='\n')	command_in=getchar();
		return 0;
	}
	while(command_index<100 && is_letter(&command_in)){
		command[command_index++]=command_in;
		command_in=getchar();
	}
	command[command_index]=0;
	//find right command
	if(strncmp(command, "help", 5)==0 || strncmp(command, "h", 2)==0){
		while(command_in!='\n'){
			if(!(command_in==' ' || command_in=='\t'))	wrong_command=1;
			command_in=getchar();
		}
		if(wrong_command)	return 0;
		history_update(command, command_index);
		help_f();
	}
	else if(strncmp(command, "dir", 4)==0 || strncmp(command, "d", 2)==0){
		while(command_in!='\n'){
			if(!(command_in==' ' || command_in=='\t'))	wrong_command=1;
			command_in=getchar();
		}
		if(wrong_command)	return 0;
		history_update(command, command_index);
		dir_f();
	}
	else if(strncmp(command, "quit", 5)==0 || strncmp(command, "q", 2)==0){
		while(command_in!='\n'){
			if(!(command_in==' ' || command_in=='\t'))	wrong_command=1;
			command_in=getchar();
		}
		if(wrong_command)	return 0;
		history_update(command, command_index);
		quit_f();
	}
	else if(strncmp(command, "history", 8)==0 || strncmp(command, "hi", 3)==0){
		while(command_in!='\n'){
			if(!(command_in==' ' || command_in=='\t'))	wrong_command=1;
			command_in=getchar();
		}
		if(wrong_command)	return 0;
		history_update(command, command_index);
		history_f();
	}
	else if(strncmp(command, "dump", 5)==0 || strncmp(command, "du", 3)==0){
		if(!(command_in==' ' || command_in=='\t' || command_in=='\n')){
			while(command_in!='\n')	command_in=getchar();
			return 0;
		}
		//input parameter 
		first=0, second=0;
		int st_addr=0, ed_addr=0;
		if(command_in!='\n'){
			command[command_index++]=' ';
			//input first param
			st_addr=input_param(command, &command_index, &command_in);
			if(st_addr==-2)	wrong_command=1;
			else if(st_addr!=-1){
				if(command_in=='\n')	first=1;
				else{
					while(command_in==' ' || command_in=='\t')
						command_in=getchar();
					if(command_in=='\n')	first=1;
					else if(command_in==','){
						command[command_index++]=',';
						command[command_index++]=' ';
						command_in=getchar();
						//input second param
						ed_addr=input_param(command, &command_index, &command_in);
						if(ed_addr==-2)	wrong_command=1;
						else if(ed_addr!=-1){
							if(command_in=='\n')	second=1;
							else{
								while(command_in==' ' || command_in=='\t')
									command_in=getchar();
								if(command_in=='\n')	second=1;
								else	wrong_command=1;
							}
						}
						else	wrong_command=1;
					}
					else	wrong_command=1;
				}
			}
			else	command_index--;
		}
		while(command_in!='\n')	command_in=getchar();
		if(wrong_command)	return 0;
		command[command_index]=0;
		if(second){
			if(st_addr>ed_addr)	return 0;
			if(ed_addr>=max_mem)	return 0;
			history_update(command, command_index);
			dump_start_end_f(st_addr, ed_addr);
		}
		else{
			if(st_addr>=max_mem)	return 0;
			history_update(command, command_index);
			if(first)	dump_start_f(st_addr);
			else	dump_f(&last_addr, 10);
		} 
	}
	else if(strncmp(command, "edit", 5)==0 || strncmp(command, "e", 2)==0){
		if(!(command_in==' ' || command_in=='\t')){
			while(command_in!='\n')	command_in=getchar();
			return 0;
		}
		//input parameter 
		int edit_addr, edit_val;
		if(command_in=='\n')	return 0;
		command[command_index++]=' ';
		//input first param		
		edit_addr=input_param(command, &command_index, &command_in);
		if(edit_addr==-2)	wrong_command=1;
		else if(edit_addr!=-1){
			if(command_in=='\n')	return 0;
			else{
				while(command_in==' ' || command_in=='\t')
					command_in=getchar();
				if(command_in=='\n')	return 0;
				else if(command_in==','){
					command[command_index++]=',';
					command[command_index++]=' ';
					command_in=getchar();
					//input second param
					edit_val=input_param(command, &command_index, &command_in);
					if(edit_val==-2)	wrong_command=1;
					else if(edit_val!=-1){
						if(command_in!='\n'){
							while(command_in==' ' || command_in=='\t')
								command_in=getchar();
							if(command_in!='\n')	wrong_command=1;
						}
					}
					else	return 0;
				}
				else	wrong_command=1;
			}
		}
		else	return 0;
		while(command_in!='\n')	command_in=getchar();
		if(wrong_command)	return 0;
		command[command_index]=0;
		if(edit_addr>=max_mem)	return 0;
		if(edit_val>=max_chr)	return 0;
		history_update(command, command_index);
		edit_f(edit_addr, edit_val);
	}
	else if(strncmp(command, "fill", 5)==0 || strncmp(command, "f", 2)==0){
		if(!(command_in==' ' || command_in=='\t')){
			while(command_in!='\n')	command_in=getchar();
			return 0;
		}
		//input parameter 
		int fill_st, fill_ed, fill_val;
		if(command_in=='\n')	return 0;
		command[command_index++]=' ';
		//input first param
		fill_st=input_param(command, &command_index, &command_in);
		if(fill_st==-2)	wrong_command=1;
		else if(fill_st!=-1){
			if(command_in=='\n')	return 0;
			else{
				while(command_in==' ' || command_in=='\t')
					command_in=getchar();
				if(command_in=='\n')	return 0;
				else if(command_in==','){
					command[command_index++]=',';
					command[command_index++]=' ';
					command_in=getchar();
					//input second param
					fill_ed=input_param(command, &command_index, &command_in);
					if(fill_ed==-2)	wrong_command=1;
					else if(fill_ed!=-1){
						if(command_in=='\n')	return 0;
						else{
							while(command_in==' ' || command_in=='\t')
								command_in=getchar();
							if(command_in=='\n')	return 0;
							else if(command_in==','){
								command[command_index++]=',';
								command[command_index++]=' ';
								command_in=getchar();
								//input third param
								fill_val=input_param(command, &command_index, &command_in);
								if(fill_val==-2)	wrong_command=1;
								else if(fill_val!=-1){
									if(command_in!='\n'){
										while(command_in==' ' || command_in=='\t')
											command_in=getchar();
										if(command_in!='\n')	wrong_command=1;
									}
								}
								else	return 0;
							}
							else	wrong_command=1;
						}
					}
					else	return 0;
				}
				else	wrong_command=1;
			}
		}
		else	return 0;
		while(command_in!='\n')	command_in=getchar();
		if(wrong_command)	return 0;
		command[command_index]=0;
		if(fill_st>fill_ed)	return 0;
		if(fill_ed>=max_mem)	return 0;
		if(fill_val>=max_chr)	return 0;
		history_update(command, command_index);
		fill_f(fill_st, fill_ed, fill_val);
	}
	else if(strncmp(command, "reset", 6)==0){
		while(command_in!='\n'){
			if(!(command_in==' ' || command_in=='\t'))	wrong_command=1;
			command_in=getchar();
		}
		if(wrong_command)	return 0;
		history_update(command, command_index);
		reset_f();
	}
	else if(strncmp(command, "opcode", 7)==0){
		//input mnemonic 
		if(command_in=='\n')	return 0;
		command[command_index++]=' ';
		while(command_in==' ' || command_in=='\t')	command_in=getchar();
		int cnt=0;
		char mnemonic_param[8];
		while(is_mnemonic(&command_in)){
			command[command_index++]=command_in;
			mnemonic_param[cnt++]=command_in;
			command_in=getchar();
			if(cnt>7){
				wrong_command=1;
				break;
			}
		}
		while(command_in!='\n'){
			if(!(command_in==' ' || command_in=='\t'))	wrong_command=1;
			command_in=getchar();
		}
		if(wrong_command)	return 0;
		mnemonic_param[cnt]=0;
		struct opcode *f_opcode=opcode_f(mnemonic_param);
		if(f_opcode==NULL)	return 0;
		char *opcode_hex=f_opcode->code;
		int opcode_num=opcode_hex[1]*256+opcode_hex[0];
		history_update(command, command_index);
		printf("\topcode is %c%c\n", opcode_num%256, opcode_num/256%256);
	}
	else if(strncmp(command, "opcodelist", 11)==0){
		while(command_in!='\n'){
			if(!(command_in==' ' || command_in=='\t'))	wrong_command=1;
			command_in=getchar();
		}
		if(wrong_command)	return 0;
		history_update(command, command_index);
		opcodelist_f();
	}
	//PROJ2
	else if(strncmp(command, "type", 5)==0){	//type
		if(command_in=='\n')	return 0;
		command[command_index++]=' ';
		while(command_in==' ' || command_in=='\t')	command_in=getchar();
		int cnt=0;
		char filename[40];
		while(command_in!='\n'){
			filename[cnt++]=command_in;
			command[command_index++]=command_in;
			command_in=getchar();
		}
		filename[cnt]=0;
		int ret=type_f(filename);
		if(ret==-1)	printf("Wrong file name : %s\n", filename);
		else history_update(command, command_index);
	}
	else if(strncmp(command, "assemble", 9)==0){	//assemble
		if(command_in=='\n')	return 0;
		command[command_index++]=' ';
		while(command_in==' ' || command_in=='\t')	command_in=getchar();
		int cnt=0;
		char filename[40];
		while(command_in!='\n'){
			filename[cnt++]=command_in;
			command[command_index++]=command_in;
			command_in=getchar();
		}
		filename[cnt]=0;
		int ret=assemble_f(filename, &symbol_table);
		if(ret!=-1){
			history_update(command, command_index);
		}
	}
	else if(strncmp(command, "symbol", 7)==0){	//symbol
		while(command_in!='\n'){
			if(!(command_in==' ' || command_in=='\t'))	wrong_command=1;
			command_in=getchar();
		}
		if(wrong_command)	return 0;
		history_update(command, command_index);
		symbol_f(symbol_table);
	}
	//PROJ3
	else if(strncmp(command, "progaddr", 9)==0){	//program address
		if(!(command_in==' ' || command_in=='\t' || command_in=='\n')){
			while(command_in!='\n')	command_in=getchar();
			return 0;
		}
		//input parameter 
		int progaddr_n=0;
		if(command_in!='\n'){
			command[command_index++]=' ';
			progaddr_n=input_param(command, &command_index, &command_in);
			if(progaddr_n<0){
				while(command_in!='\n')	command_in=getchar();
				if(progaddr_n==-2)	return 0;
				command_index--;
			}
			else{
				while(command_in!='\n'){
					if(!(command_in==' ' || command_in=='\t'))	progaddr_n=-1;
					command_in=getchar();
				}
				if(progaddr_n==-1)	return 0;
				progaddr=progaddr_n;	//set new progaddr
			}
		}
		command[command_index]=0;
		history_update(command, command_index);
	}
	else if(strncmp(command, "loader", 7)==0){	//linking loader
		if(command_in=='\n')	return 0;
		command[command_index++]=' ';
		int cnt=0, filenum=0;
		char filename[4][40];
		//read files to link & load
		while(filenum<4){
			while(command_in==' ' || command_in=='\t')	command_in=getchar();
			if(command_in=='\n')	break;
			cnt=0;
			while(!(command_in==' ' || command_in=='\t' || command_in=='\n')){
				filename[filenum][cnt++]=command_in;
				command[command_index++]=command_in;
				command_in=getchar();
			}
			filename[filenum][cnt]=0;
			filenum++;
			if(command_in=='\n')	break;
		}
		while(command_in!='\n')	command_in=getchar();
		if(filenum==0 || filenum>3)	return 0;
		//load program
		int ret=loader_f(filename, filenum, progaddr);
		if(ret!=-1){
			command[command_index]=0;
			history_update(command, command_index);
			loaded=1;
		}
	}
	else if(strncmp(command, "bp", 3)==0){	//break point
		while(command_in==' ' || command_in=='\t'){
			command_in=getchar();
		}
		if(command_in=='\n'){
			print_bp();	//print all break points
			command[command_index]=0;
			history_update(command, command_index);
			return 1;
		}
		command[command_index++]=' ';
		int paramidx=0, bpaddr=0;
		char param[10];
		//input parameter
		while(paramidx<10 && command_in!='\n'){
			param[paramidx++]=command[command_index++]=command_in;
			command_in=getchar();
		}
		if(command_in!='\n'){	//wrong command
			while(command_in!='\n')	command_in=getchar();
			return 0;
		}
		param[paramidx]=0;
		if(strncmp(param, "clear", 6)==0){
			clear_bp();	//clear break points
		}else{
			for(int i=0;i<paramidx;++i){
				if(!is_num(param[i]))	return 0;
				bpaddr=bpaddr*16+ch_to_num(param[i]);
			}
			set_bp(bpaddr);	//set break point
		}
		command[command_index]=0;
		history_update(command, command_index);
	}
	else if(strncmp(command, "run", 4)==0){
		while(command_in==' ' || command_in=='\t'){
			command_in=getchar();
		}
		if(command_in!='\n'){	//wrong command
			while(command_in!='\n')	command_in=getchar();
			return 0;
		}
		command[command_index]=0;
		history_update(command, command_index);
		run_f(&loaded);	//run the program(copy.obj)
	}
	else{
		while(command_in!='\n')	command_in=getchar();
		return 0;
	}
	return 1;
}
int main(){	//initialize opcode hash table, memory
	make_opcode_hash_table();
	Memory_initialize();
	while(running){	//if running==1, input command
		printf("sicsim> ");
		if(!command_input()){
			printf("[ERROR] Wrong Command\n");
		}
	}
	return 0;
}
