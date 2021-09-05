#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<dirent.h>
#include<sys/stat.h>

//command
struct cmd{
	int his_num;
	char token[21];
	struct cmd *nxt;
};

//OPCODE HASH TABLE 20
struct opcode{
	char code[3];
	char mnemonic[10];
	char type[4];
	struct opcode *nxt;
};

//PROJ2 ASSEMBLE

//symbol node to make symbol table
struct sym{
	char symbol[10];
	int addr;
	struct sym *nxt;
};

//list node to make listing file
struct lst{
	//tp: 0-mnemonic, 1-lable, 2-variable, 3-comment, 4-base, 5-nobase
	int line, loc, fm, tp;
	int obj_exist, inp_num;
	char inp[5][30];
	char buf[100];
	char object_code[10];
	struct lst* nxt;
};

//modification node to be modified
struct modi{
	int addr, num;
	struct modi *nxt;
};

//function calls
void Memory_initialize();
void make_opcode_hash_table();
void help_f();
void dir_f();
void quit_f();
void history_update(char *str, int num);
void history_f();
void make_address(char *chr, int addr);
void dump_f(int *address, int num);
void dump_part(int address, int from, int to);
void dump_start_f(int start);
void dump_start_end_f(int start, int end);
void edit_f(int addr, int val);
void fill_f(int start, int end, int val);
void reset_f();
struct opcode* opcode_f(char *mnem);
void opcodelist_f();
int type_f(char *file_name);
int is_num(char num);
int ch_to_num(char num);
int input_param(char *chr, int *idx, char* last_input);
int is_letter(char* chr);
int is_mnemonic(char *chr);
int command_input();
