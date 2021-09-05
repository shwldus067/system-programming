#include "20171264.h"
#include "assemble.h"

struct sym *symtab;

struct lst *list_head;
struct lst *list_tail;

struct modi *modi_head;
struct modi *modi_tail;

//object file
char object_title[30];
int program_length;
int return_address;
int BASE_R=-1;

char hexatable[16]={'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

void free_symtab(){	//free symbol table
	struct sym *sym_tmp;
	while(symtab!=NULL){
		sym_tmp=symtab;
		symtab=symtab->nxt;
		free(sym_tmp);
	}
}
void free_lists(){
	//free list, modification lists 
	struct lst *lst_tmp;
	while(list_head!=NULL){
		lst_tmp=list_head;
		list_head=list_head->nxt;
		free(lst_tmp);
	}
	struct modi *modi_tmp;
	while(modi_head!=NULL){
		modi_tmp=modi_head;
		modi_head=modi_head->nxt;
		free(modi_tmp);
	}
}
struct sym* find_symbol(char *symb){	//find symbol
	struct sym *tmp=symtab;
	while(tmp!=NULL){
		if(strcmp(tmp->symbol, symb)==0)	return tmp;
		tmp=tmp->nxt;
	}
	tmp=NULL;
	return tmp;
}
void symbol_update(char *symb, int addr){	//update symbol table
	struct sym *new_sym=(struct sym*)malloc(sizeof(struct sym));
	new_sym->addr=addr;
	strncpy(new_sym->symbol, symb, strlen(symb));
	new_sym->symbol[strlen(symb)]=0;
	new_sym->nxt=NULL;
	if(symtab==NULL){	//first symbol
		symtab=new_sym;
		return;
	}
	struct sym *tmp=symtab, *pre=NULL;
	while(tmp!=NULL){	//find right position to be in ascending order
		if(strncmp(tmp->symbol, symb, strlen(symb)+1)>0){
			new_sym->nxt=tmp;
			if(pre==NULL){
				symtab=new_sym;
			}else{
				pre->nxt=new_sym;
			}
			return;
		}
		pre=tmp;
		tmp=tmp->nxt;
	}
	pre->nxt=new_sym;
}
void modification_update(struct lst *list){	//update modification list
	struct modi* new_modi=(struct modi*)malloc(sizeof(struct modi));
	new_modi->nxt=NULL;
	new_modi->num=5;
	new_modi->addr=list->loc+1;
	if(modi_head==NULL)	modi_head=new_modi;
	else	modi_tail->nxt=new_modi;
	modi_tail=new_modi;
}
int pass_1(char *file){	//assemble pass 1
	FILE *fp=fopen(file, "r");
	if(fp==NULL){
		printf("File open error\n");
		return -2;
	}
	char input[100], *token;
	int cnt=0, addr=0, size=0, line=0, start_a=0, idx=0;
	while(1){	//read
		fgets(input, sizeof(input), fp);
		size=strlen(input);
		if(input[size-1]=='\n'){
			size--;
			input[size]=0;
		}
		line+=5;
		idx=1;
		struct lst *new_lst=(struct lst*)malloc(sizeof(struct lst));	//new list
		new_lst->line=line;
		new_lst->nxt=NULL;
		new_lst->tp=0;
		new_lst->fm=0;
		new_lst->obj_exist=0;
		new_lst->inp_num=0;
		new_lst->loc=addr;
		if(input[0]=='.'){	//comment
			strncpy(new_lst->buf, input, size);
			new_lst->tp=3;
		}
		else{
			token=strtok(input, " ");
			if(token==NULL)	continue;
			cnt=0;
			while(token!=NULL){	//input parsing
				if(cnt==5){
					++cnt;break;
				}
				strncpy(new_lst->inp[cnt], token, sizeof(token)+1);
				token=strtok(NULL, " ");
				cnt++;
			}
			if(cnt==6){
				fclose(fp);
				return line;	//input error
			}
			new_lst->inp_num=cnt;
			if(strncmp(new_lst->inp[1], "START", 6)==0){	//asm start
				if(start_a){
					fclose(fp);
					return line;
				}
				strncpy(object_title, new_lst->inp[0], strlen(new_lst->inp[0])+1);
				size=strlen(new_lst->inp[2]);
				for(int i=0;i<size;++i){
					addr=addr*16+ch_to_num(new_lst->inp[2][i]);
				}
				new_lst->loc=addr;
				start_a=1;
				list_head=new_lst;
				list_tail=new_lst;
				continue;
			}
			else if(strncmp(new_lst->inp[0], "END", 4)==0){	//asm end
				program_length=addr-(list_head->loc);
				struct sym *end_sym=find_symbol(new_lst->inp[1]);
				if(end_sym==NULL){
				printf("NO symbol\n");
					fclose(fp);
					return line;
				}
				return_address=end_sym->addr;
				list_tail->nxt=new_lst;
				break;
			}else if(strncmp(new_lst->inp[0], "BASE", 5)==0){ //set base
				new_lst->obj_exist=1;
				new_lst->tp=4;
			}else if(strncmp(new_lst->inp[0], "NOBASE", 7)==0){	//no base
				new_lst->obj_exist=1;
				new_lst->tp=5;
			}else{
				char mne[30];
				strncpy(mne, new_lst->inp[0], strlen(new_lst->inp[0])+1);
				struct opcode *f_opcode;
				if(mne[0]=='+')	f_opcode=opcode_f(mne+1);
				else	f_opcode=opcode_f(mne);
				if(f_opcode==NULL){	//label
					if(find_symbol(mne)!=NULL){
						fclose(fp);
						return line; //duplicate symbol
					}
					symbol_update(mne, addr);
					printf("%s\n", mne);
					++idx;
					new_lst->tp=1;
					strncpy(mne, new_lst->inp[1], strlen(new_lst->inp[1])+1);
					if(mne[0]=='+')	f_opcode=opcode_f(mne+1);
					else	f_opcode=opcode_f(mne);
				}
				if(f_opcode!=NULL){	//opcode exists
					if(strncmp(f_opcode->type, "1", 1)==0)	addr+=1, new_lst->fm=1;
					else if(strncmp(f_opcode->type, "2", 1)==0)	addr+=2, new_lst->fm=2;
					else{
						if(mne[0]=='+')	addr+=4, new_lst->fm=4;
						else	addr+=3, new_lst->fm=3;
					}
					new_lst->obj_exist=1;
				}
				else{	//directives
					new_lst->tp=2;
					if(strncmp(mne, "WORD", 5)==0){
						addr+=3;
						new_lst->obj_exist=1;
					}else if(strncmp(mne, "RESW", 5)==0){
						int oper=0, len=strlen(new_lst->inp[idx]);
						for(int i=0;i<len;++i){
							oper=oper*10+(new_lst->inp[idx][i]-'0');
						}
						addr+=3*oper;
					}else if(strncmp(mne, "RESB", 5)==0){
						int oper=0, len=strlen(new_lst->inp[idx]);
						for(int i=0;i<len;++i){
							oper=oper*10+(new_lst->inp[idx][i]-'0');
						}
						addr+=oper;
					}else if(strncmp(mne, "BYTE", 5)==0){
						int word=2, len=0;
						while(new_lst->inp[idx][word]!='\''){
							len++;word++;
						}
						if(new_lst->inp[idx][0]=='X')	len/=2;	//hexadecimal number
						addr+=len;
						new_lst->obj_exist=1;
					}else if(strncmp(mne, "BASE", 5)==0){	//set base
						new_lst->obj_exist=1;
						new_lst->tp=4;
					}else if(strncmp(mne, "NOBASE", 7)==0){	//no base
						new_lst->obj_exist=1;
						new_lst->tp=5;
					}else{
						fclose(fp);
						return line;
					}
				}
			}
		}
		list_tail->nxt=new_lst;	//line list
		list_tail=new_lst;
		input[0]=0;
	}
	fclose(fp);
	return 0;
}
int pass_2(){	//assemble pass 2
	struct lst *tmp=list_head;
	while(tmp->nxt!=NULL){
		if(tmp->obj_exist==0){	//no object code
			tmp=tmp->nxt;
			continue;
		}
		if(tmp->fm>0){	//opcode
			int idx=0, obj_idx=2, reg_n=0;
			if(tmp->tp>0)	idx=1;	//label
			char mne[30];
			strncpy(mne, tmp->inp[idx], strlen(tmp->inp[idx])+1);
			struct opcode *f_opcode;
			if(mne[0]=='+')	f_opcode=opcode_f(mne+1);
			else	f_opcode=opcode_f(mne);
			strncpy(tmp->object_code, f_opcode->code, 2);	//store opcode
			if(tmp->fm==1){	//format 1
				if(tmp->inp_num-idx>1)	return tmp->line;
				tmp->object_code[2]=0;
			}else if(tmp->fm==2){	//format 2
				if(tmp->inp_num-idx>3)	return tmp->line;
				if(strncmp(mne, "SVC", 4)==0){	//SVC n
					if(tmp->inp_num-idx!=2)	return tmp->line;
					++idx;
					strncpy(mne, tmp->inp[idx], strlen(tmp->inp[idx]));
					int len=strlen(mne);
					for(int i=0;i<len;++i){
						reg_n=reg_n*10+mne[i];
					}
					if(reg_n>16)	return tmp->line;
					tmp->object_code[2]=hexatable[reg_n];
					tmp->object_code[3]='0';
				}
				for(int i=idx+1;i<tmp->inp_num;++i){	//find register
					if(tmp->inp[i][0]=='A')	tmp->object_code[obj_idx++]='0';
					else if(tmp->inp[i][0]=='X')	tmp->object_code[obj_idx++]='1';
					else if(tmp->inp[i][0]=='L')	tmp->object_code[obj_idx++]='2';
					else if(tmp->inp[i][0]=='P' && tmp->inp[i][1]=='C')	tmp->object_code[obj_idx++]='8';
					else if(tmp->inp[i][0]=='S' && tmp->inp[i][1]=='W')	tmp->object_code[obj_idx++]='9';
					else if(tmp->inp[i][0]=='B')	tmp->object_code[obj_idx++]='3';
					else if(tmp->inp[i][0]=='S')	tmp->object_code[obj_idx++]='4';
					else if(tmp->inp[i][0]=='T')	tmp->object_code[obj_idx++]='5';
					else if(tmp->inp[i][0]=='F')	tmp->object_code[obj_idx++]='6';
					if(strncmp(mne, "SHIFT", 5)==0){ //SHIFTL or SHIFTR r1, n
						if(tmp->inp_num-idx!=3)	return tmp->line;
						idx+=2;
						strncpy(mne, tmp->inp[idx], strlen(tmp->inp[idx]));
						int len=strlen(mne);
						for(int i=0;i<len;++i){
							reg_n=reg_n*10+mne[i];
						}
						if(reg_n>16)	return tmp->line;
						tmp->object_code[3]=hexatable[reg_n];
						obj_idx=4;
						break;
					}
				}
				if(obj_idx==3)	tmp->object_code[obj_idx]='0';	//fill '0' 
				tmp->object_code[4]=0;
			}else{	//format 3/4
				//RSUB has no operand & set pc register
				if(strncmp(mne, "RSUB", 5)==0){
					strncpy(tmp->object_code+1, "F0000", 6);
					tmp=tmp->nxt;
					continue;
				}
				int len, o_st=0, op_addr, hex;
				char operand[30];
				int ni=3, xbpe=0;
				++idx;
				len=strlen(tmp->inp[idx]);
				strncpy(operand, tmp->inp[idx], len+1);
				if(operand[len-1]==',' && tmp->inp[idx+1][0]=='X'){	//indexed addressing
					operand[len-1]=0, len--;
					xbpe|=8;
				}
				if(mne[0]=='+')	xbpe|=1;	//extended instruction
				if(operand[0]=='#')	ni=1, o_st=1;	//immediate addressing
				if(operand[0]=='@')	ni=2, o_st=1;	//indirect addressing
				struct sym *symt=find_symbol(operand+o_st);
				if(symt!=NULL){	//in symtab
					if(tmp->fm==4){	//format 4
						op_addr=symt->addr;
						modification_update(tmp);
					}else{	//format 3
						op_addr=symt->addr-tmp->nxt->loc;
						if(-2048<=op_addr && op_addr<2048){	//pc relative
							if(op_addr<0)	op_addr+=4096;
							xbpe|=2;
						}else{
							op_addr=symt->addr-BASE_R;
							if(BASE_R!=-1 && (0<=op_addr && op_addr<4096)){	//base relative
								xbpe|=4;
							}else{
								return tmp->line;
							}
						}
					}
				}else{ //not in symtab
					if(operand[0]=='#'){
						op_addr=0;
						for(int i=1;i<len;++i){
							if(operand[i]<'0' || operand[i]>'9')	return tmp->line;
							op_addr=op_addr*10+ch_to_num(operand[i]);
						}
					}else{
						return tmp->line;
					}
				}
				//fill opcode 
				if(tmp->fm==4){	//format 4
					hex=1<<16;
					if(op_addr<0 || op_addr>=hex)	return tmp->line;
					for(int i=3;i<8;++i){
						tmp->object_code[i]=hexatable[op_addr/hex];
						op_addr%=hex;
						hex>>=4;
					}
					tmp->object_code[8]=0;
				}else{	//format 3
					hex=1<<8;
					for(int i=3;i<6;++i){
						tmp->object_code[i]=hexatable[op_addr/hex];
						op_addr%=hex;
						hex>>=4;
					}
					tmp->object_code[6]=0;
				}
				//set nixbpe registers
				char op_last=tmp->object_code[1];
				tmp->object_code[1]=hexatable[ch_to_num(op_last)+ni];
				tmp->object_code[2]=hexatable[xbpe];
			}
		}else{	//WORD, BYTE, BASE, NOBASE directives
			int idx=0;
			struct sym *sym_tmp;
			if(tmp->tp==4){	//BASE
				if(strncmp(tmp->inp[0], "BASE", 5)!=0)	idx=1;
				++idx;
				if(tmp->inp_num-idx!=1)	return tmp->line;
				sym_tmp=find_symbol(tmp->inp[idx]);
				if(sym_tmp!=NULL){	//exists in symbol table
					BASE_R=sym_tmp->addr;
				}else{	//not in symbol table 
					return tmp->line;
				}
				tmp->obj_exist=0;
			}else if(tmp->tp==5){	//NOBASE
				BASE_R=-1;
				tmp->obj_exist=0;
			}else if(tmp->tp==2){
				if(tmp->inp_num!=3)	return tmp->line;
				if(strncmp(tmp->inp[1], "WORD", 5)==0){	//word
					int num=0, len=strlen(tmp->inp[1]), hex=1<<20;
					for(int i=0;i<len;++i){
						num=num*10+(tmp->inp[1][i]-'0');
					}
					if(num>=(1<<23))	return tmp->line;
					for(int i=0;i<6;++i){
						tmp->object_code[i]=hexatable[num/hex];
						num%=hex;
						hex>>=4;
					}
					tmp->object_code[6]=0;
				}else if(strncmp(tmp->inp[1], "BYTE", 5)==0){	//byte
					int len=strlen(tmp->inp[2]), op_idx=0;
					char b_chr;
					if(tmp->inp[2][0]=='X'){	//hexadecimal value
						for(int i=2;i<len-1;++i){
							tmp->object_code[i-2]=tmp->inp[2][i];
						}
						tmp->object_code[len-2]=0;
					}else if(tmp->inp[2][0]=='C'){	//character value
						for(int i=2;i<len-1;++i){
							b_chr=tmp->inp[2][i];
							tmp->object_code[op_idx++]=hexatable[(int)(b_chr/16)];
							tmp->object_code[op_idx++]=hexatable[(int)(b_chr%16)];
						}
						tmp->object_code[op_idx]=0;
					}else{
						return tmp->line;
					}
				}else{
					return tmp->line;
				}
			}else{
				return tmp->line;
			}
		}
		tmp=tmp->nxt;
	}
	return 0;
}
int make_listing(char *file){ //write listing file
	char lst_file[100];
	strncpy(lst_file, file, strlen(file)+1);
	strncat(lst_file, ".lst", 5);
	FILE *fp=fopen(lst_file, "w");
	if(fp==NULL){
		printf("File open error\n");
		return -1;
	}
	struct lst *lst_tmp=list_head;
	while(lst_tmp!=NULL){	//for all lists
		fprintf(fp, "%-10d", lst_tmp->line);
		if(lst_tmp->tp==3){
			fprintf(fp, "          %s\n", lst_tmp->buf);
			lst_tmp=lst_tmp->nxt;
			continue;
		}
		if(strncmp(lst_tmp->inp[1], "START", 6)==0){	//start
			fprintf(fp, "%04X      %-10s%-10s%s\n", lst_tmp->loc, lst_tmp->inp[0], lst_tmp->inp[1], lst_tmp->inp[2]);
			lst_tmp=lst_tmp->nxt;
			continue;
		}
		if(strncmp(lst_tmp->inp[0], "END", 4)==0){	//end
			fprintf(fp, "%-10s%-10s%-10s%-20s\n", " ", " ", lst_tmp->inp[0], lst_tmp->inp[1]);
			lst_tmp=lst_tmp->nxt;
			continue;
		}
		if(lst_tmp->tp>3){	//base or nobase
			fprintf(fp, "%-10s", " ");
			if(strncmp(lst_tmp->inp[0], "BASE", 5)==0 || strncmp(lst_tmp->inp[0], "NOBASE", 7)==0){
				fprintf(fp, "%-10s%-10s%-20s\n", " ", lst_tmp->inp[0], lst_tmp->inp[1]);
			}else{	//if label exists
				fprintf(fp, "%-10s%-10s%-20s\n", lst_tmp->inp[0], lst_tmp->inp[1], lst_tmp->inp[2]);
			}
		}else{
			fprintf(fp, "%04X      ", lst_tmp->loc);
			int idx=0;
			if(lst_tmp->tp==1 || lst_tmp->tp==2){	//print label
				fprintf(fp, "%-10s", lst_tmp->inp[0]);
				idx++;
			}else{
				fprintf(fp, "%-10s", " ");
			}
			char operands[30]="";
			int op_len=0;
			for(int i=idx+1;i<lst_tmp->inp_num;++i){	//store operands
				op_len+=strlen(lst_tmp->inp[i]);
				strncat(operands, lst_tmp->inp[i], strlen(lst_tmp->inp[i]));
				operands[op_len++]=' ';
				operands[op_len]=0;
			}
			fprintf(fp, "%-10s%-20s", lst_tmp->inp[idx], operands);
			if(lst_tmp->obj_exist){	//object code exists
				fprintf(fp, "%s", lst_tmp->object_code);
			}
			fprintf(fp, "\n");
		}
		lst_tmp=lst_tmp->nxt;
	}
	fclose(fp);
	return 0;
}
int make_obj(char *file){	//write object file
	char obj_file[100];
	strncpy(obj_file, file, strlen(file)+1);
	strncat(obj_file, ".obj", 5);
	FILE *fp=fopen(obj_file, "w");
	if(fp==NULL){
		printf("File open error\n");
		return -1;
	}
	struct lst *lst_tmp=list_head, *last;
	//print head record
	fprintf(fp, "H%-6s%06X%06X\n", object_title, lst_tmp->loc, program_length);
	lst_tmp=lst_tmp;
	int length=0;
	while(lst_tmp!=NULL){	//print text record
		while(lst_tmp!=NULL && !(lst_tmp->obj_exist))	lst_tmp=lst_tmp->nxt;
		if(lst_tmp==NULL)	break;
		fprintf(fp, "T%06X", lst_tmp->loc);	//text record, start address
		length=strlen(lst_tmp->object_code);
		last=lst_tmp->nxt;
		while(last!=NULL){	//find the first list to be written in the next line
			if(last->obj_exist){	//
				if(last->loc-lst_tmp->loc>=29)	break;
				length+=strlen(last->object_code);
			}
			last=last->nxt;
		}
		fprintf(fp, "%02X", length/2);	//record size
		while(lst_tmp!=last){	//for lists to be written in the current line
			if(lst_tmp->obj_exist){	//object code exists
				fprintf(fp, "%s", lst_tmp->object_code);
			}
			lst_tmp=lst_tmp->nxt;
		}
		fprintf(fp, "\n");
	}
	struct modi *modi_tmp=modi_head;	//modification record
	while(modi_tmp!=NULL){	//for all the modification lists
		fprintf(fp, "M%06X%02X\n", modi_tmp->addr, modi_tmp->num);	//address, the number
		modi_tmp=modi_tmp->nxt;
	}
	fprintf(fp, "E%06X\n", return_address);	//end record
	fclose(fp);
	return 0;
}
int assemble_f(char *file, struct sym **symbol_top){
	//pass 1
	int ret=pass_1(file);
	if(ret==-2) return -1;
	if(ret!=0){
		printf("Assemble Error In Line %d\n", ret);
		free_symtab();
		free_lists();
		printf("pass 1 error\n");
		return -1;
	}
	//pass 2
	ret=pass_2();
	if(ret!=0){
		printf("Assemble Error In Line %d\n", ret);
		free_symtab();
		free_lists();
		printf("pass 2 error\n");
		return -1;
	}
	//make new symbol table
	struct sym *sym_tmp;
	while(*symbol_top!=NULL){
		sym_tmp=*symbol_top;
		*symbol_top=(*symbol_top)->nxt;
		free(sym_tmp);
	}
	*symbol_top=symtab;
	symtab=NULL;
	//make obj & listing files
	file[strlen(file)-4]=0;
	ret=make_listing(file);	//write lst file
	if(ret==-1){
		printf("Listing error\n");
		free_symtab();
		free_lists();
		return -1;
	}
	ret=make_obj(file);	//write obj file
	if(ret==-1){
		printf("Object error\n");
		free_symtab();
		free_lists();
		return -1;
	}
	free_lists();
	printf("[%s.lst], [%s.obj]\n", file, file);
	return 1;
}
void symbol_f(struct sym *symbol_top){	//print the symbol table
	if(symbol_top==NULL){	//no symbol table
		printf("There's no symbol table\n");
		return;
	}
	struct sym *tmp=symbol_top;
	while(tmp!=NULL){
		printf("\t%s\t%04X\n", tmp->symbol, tmp->addr);
		tmp=tmp->nxt;
	}
}
