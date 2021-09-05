#include "20171264.h"
#include "run.h"
#define lt -1
#define eq 0
#define gt 1

enum registers{	//register 0~9
	rA, rX, rL, rB, rS, rT, rF, rf, rPC, rSW
};
int bp[2000], bpidx=0, maxpc=0;	//break points, maximum pc num
int reg[10]={0, };	//register vlaues

void print_bp(){	//print all the break points
	printf("\t\tbreakpoint\n");
	printf("\t\t----------\n");
	for(int i=0;i<bpidx;++i){
		printf("\t\t%X\n", bp[i]);
	}
}
void clear_bp(){	//clear the break points
	bpidx=0;
	printf("\t\t[ok] clear all breakpoints\n");
}
int find_bp(int bpaddr){	//find bpaddr in bp
	for(int i=0;i<bpidx;++i){
		if(bp[i]==bpaddr)	return 1;
	}
	return 0;
}
void set_bp(int bpaddr){	//set new break point
	if(find_bp(bpaddr)){
		printf("\t\t[ok] create breakpoint %X\n", bpaddr);
		return;
	}
	bp[bpidx++]=bpaddr;
	int tmp;
	for(int i=bpidx-1;i>0;--i){	//bubble sort
		for(int j=0;j<i;++j){
			if(bp[j]>bp[j+1]){
				tmp=bp[j];
				bp[j]=bp[j+1];
				bp[j+1]=tmp;
			}
		}
	}
	printf("\t\t[ok] create breakpoint %X\n", bpaddr);
}
void operate(int opcode, int a1, int a2, int ni){	//operate the code
	int val=0, tmp=0;
	if(ni==1)	val=a1;	//immediate value
	else if(ni==2){	//indirect value
		tmp=(unsigned int)read_memory(a1);
		tmp=tmp*256+(unsigned int)read_memory(a1+1);
		tmp=tmp*256+(unsigned int)read_memory(a1+2);
		a1=tmp;
	}else{	//simple vlaue
		val=(unsigned int)read_memory(a1);
		val=val*256+(unsigned int)read_memory(a1+1);
		val=val*256+(unsigned int)read_memory(a1+2);
	}
	switch(opcode){	//find right opcode
		case 0x14: //stl
			tmp=reg[rL];
			edit_f(a1+2, tmp%256);tmp/=256;
			edit_f(a1+1, tmp%256);tmp/=256;
			edit_f(a1, tmp%256);
			break;
		case 0x68: //ldb
			reg[rB]=val;
			break;
		case 0x48: //jsub
			reg[rL]=reg[rPC];
			reg[rPC]=a1;
			break;
		case 0x00: //lda
			reg[rA]=val;
			break;
		case 0x28: //comp
			if(reg[rA]<val)	reg[rSW]=lt;
			else if(reg[rA]==val)	reg[rSW]=eq;
			else reg[rSW]=gt;
			break;
		case 0x30: //jeq
			if(reg[rSW]==eq)	reg[rPC]=a1;
			break;
		case 0x3C: //j
			reg[rPC]=a1;
			break;
		case 0x0C: //sta
			tmp=reg[rA];
			edit_f(a1+2, tmp%256);tmp/=256;
			edit_f(a1+1, tmp%256);tmp/=256;
			edit_f(a1, tmp%256);
			break;
		case 0xB4: //clear
			reg[a1]=0;
			break;
		case 0x74: //ldt
			reg[rT]=val;
			break;
		case 0xE0: //td
			reg[rSW]=lt;
			break;
		case 0xD8: //rd
			break;
		case 0xA0: //compr
			if(reg[a1]<reg[a2])	reg[rSW]=lt;
			else if(reg[a1]==reg[a2]) reg[rSW]=eq;
			else	reg[rSW]=gt;
			break;
		case 0x54: //stch
			tmp=reg[rA];
			edit_f(a1, tmp%256);
			break;
		case 0xB8: //tixr
			reg[rX]++;
			if(reg[rX]<reg[a1])	reg[rSW]=lt;
			else if(reg[rX]==reg[a1])	reg[rSW]=eq;
			else	reg[rSW]=gt;
			break;
		case 0x38: //jlt
			if(reg[rSW]==lt)	reg[rPC]=a1;
			break;
		case 0x10: //stx
			tmp=reg[rX];
			edit_f(a1+2, tmp%256);tmp/=256;
			edit_f(a1+1, tmp%256);tmp/=256;
			edit_f(a1, tmp%256);
			break;
		case 0x4c: //rsub
			reg[rPC]=reg[rL];
			break;
		case 0x50: //ldch
			reg[rA]=(unsigned int)read_memory(a1);
			break;
		case 0xDC: //wd
			break;
		default:
			printf("\n\nnot defined\n\n");
			break;
	}
}
void init_register(){	//reset registers
	for(int i=0;i<10;++i)	reg[i]=0;
	reg[rL]=4215;
	maxpc=reg[rL];
}
void print_register(int bpoint){	//print registers
	printf("A : %06X  X : %06X\n", reg[rA], reg[rX]);
	printf("L : %06X PC : %06X\n", reg[rL], reg[rPC]);
	printf("B : %06X  S : %06X\n", reg[rB], reg[rS]);
	printf("T : %06X\n", reg[rT]);
	if(bpoint==maxpc)	printf("\t\tEnd Program\n");
	else	printf("\t\tStop at checkpoint[%X]\n", bpoint);
}
void run_f(int *loaded){	//run copy.obj
	if(*loaded)	init_register(), *loaded=0;	//if newly loaded
	int bpoint;
	while(reg[rPC]<maxpc){	//before end of program
		int code, format, a1, a2, ni, tmp, x, b, p, e;
		code=(unsigned int)read_memory(reg[rPC]++);
		ni=code&3;	//set ni values
		code=code^ni;	//read opcode
		format=find_format(code);	//find format of opcode
		if(format==1);	//format 1
		else if(format==2){	//format 2
			tmp=(unsigned int)read_memory(reg[rPC]++);	//read registers
			a2=tmp%16;
			a1=tmp>>4;
		}else if(format==3){	//format 3/4
			tmp=(unsigned int)read_memory(reg[rPC]++);	//read 8bits
			a1=tmp%16;
			tmp>>=4;
			//set xbpe values
			x=tmp&8 ? 1:0;
			b=tmp&4 ? 1:0;
			p=tmp&2 ? 1:0;
			e=tmp&1 ? 1:0;
			//read 8bits more
			a1<<=8;
			a1+=(unsigned int)read_memory(reg[rPC]++);
			if(e){	//read 8bits more
				a1<<=8;
				a1+=(unsigned int)read_memory(reg[rPC]++);
			}else{
				if(a1&0x800)	a1|=0xfffff000;	//check negative
				if(x)	a1+=reg[rX];	//indexed addressing
				if(b==0 && p==1){	//pc relative addressing
					a1+=reg[rPC];
				}else if(b==1 && p==0){	//base relative addressing
					a1+=reg[rB];
				}
			}
		}else{
			continue;	//not in opcode table
		}
		operate(code, a1, a2, ni);	//operate the code
		bpoint=reg[rPC];
		if(find_bp(bpoint))	break;	//check if it is break point
	}
	print_register(bpoint);	//print registers
	return;
}
