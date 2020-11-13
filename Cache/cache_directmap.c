#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int Memory[0x100000];
int R[32];
int pc;

int cyclen;
int ReadM_count=0;
int WriteM_count=0;
int coldmiss_count=0;
int miss_count=0;
int hit_count=0;
double Miss_rate=0;
double Hit_rate=0;

struct ifid
{
int inst;
int pc;
}ifid;

struct idex
{
int inst;
int pc;
int opcode,rs,rt,rd,shamt,funct,immediate,address,Simmediate;//decode
int v1,v2;
int JumpAddr,BranchAddr,ZeroExtImm;
int RegDst,Branch,MemWrite,MemRead,MemtoReg,RegWrite;//control
}idex;

struct exmem
{
int inst;
int pc;
int opcode,funct,Simmediate;
int v1,v2,reg;
int ALUresult;
int MemWrite,MemRead,MemtoReg,RegWrite;//control
int Index;
}exmem;

struct memwr
{
int inst;
int pc;
int reg;
int MEMresult,ALUresult;
int MemtoReg,RegWrite;//control
int Index;
}memwr;

struct cache_
{
unsigned int tag;
unsigned int valid;
unsigned int dirty;
int data[16];
}cache_;//tag 18, index 8, offset 6 

struct ifid ifid_latch[2];
struct idex idex_latch[2];
struct exmem exmem_latch[2];
struct memwr memwr_latch[2];
struct cache_ cache[1024];//cache size16kb, linesize 64byte 
//=========================================================================

int ReadMem(int addr)
{
int tag = (addr & 0xffff0000) >> 16; //16bit
int index = (addr & 0x0000ffc0) >> 6; //10bit
int offset = addr & 0x0000003f; //6bit
int dest_addr = (addr & 0xffffffc0);
int drain_addr = (cache[index].tag << 16) | (index << 6);


if(cache[index].valid != 1) //cold miss
{
	for(int i=0; i<16; i++)
	{
		cache[index].data[i] = Memory[(dest_addr/4) + i];
		printf("%d \n", cache[index].data[i]);
	}
	
	cache[index].tag = tag;
	cache[index].valid = 1;
	cache[index].dirty = 0;
	printf("coldmiss\n");
	miss_count++;
	coldmiss_count++;
	return cache[index].data[offset/4];
}

else if(cache[index].tag != tag)//miss
{
	if(cache[index].dirty == 1)//drain
	{
		for(int i=0; i<16; i++)//writeback
		{
		Memory[(drain_addr/4) + i] = cache[index].data[i];
		}
	}
	
	for(int i=0; i<16; i++)//fill
	{
		cache[index].data[i] = Memory[((addr & 0xffffffc0)/4) + i];
	}

	cache[index].tag = tag;
	cache[index].valid = 1;
	cache[index].dirty = 0;
	miss_count++;	
	return cache[index].data[offset/4];//read

}

else if(cache[index].tag == tag) //hit
{
	hit_count++;
	return cache[index].data[offset/4];//read
	printf("lw, hit\n");
}
}

int WriteMem(int addr, int value)
{
int tag = (addr & 0xffff0000) >> 16; //16bit
int index = (addr & 0x0000ffc0) >> 6; //10bit
int offset = addr & 0x0000003f; //6bit
int dest_addr = (addr & 0xffffffc0);
int drain_addr = (cache[index].tag >> 16) | (index >> 6);

if(cache[index].tag != tag) //miss
{
	if(cache[index].dirty == 1) //drain
	{	
		for(int i=0; i<16; i++)
		{
		Memory[(drain_addr/4) + i]  = cache[index].data[i];
		}
	}
	
	for(int i=0; i<16; i++) //fill
	{
		cache[index].data[i] = Memory[(dest_addr/4) + i];
	}

	cache[index].data[offset/4] = value;
	cache[index].tag = tag;
	cache[index].dirty = 1;
	cache[index].valid = 1;
	miss_count++;
	return 0; //read
}

else if(cache[index].tag == tag) //hit
{
	cache[index].data[offset/4] = value;
	cache[index].dirty = 1;
	printf("sw, hit\n");
	hit_count++;
	return 0;
}
}


void fetch()
{
	
	ifid_latch[0].inst = ReadMem(pc);
	ifid_latch[0].pc=pc;
	pc=pc+4;
	printf("Fet: instruction=0x%08X\n",ifid_latch[0].inst);
}

void decode()
{
//opcode
	idex_latch[0].opcode=(ifid_latch[1].inst & 0xfc000000) >> 26; 
//Rtype
if(idex_latch[0].opcode == 0x0)
{
	idex_latch[0].rs = (ifid_latch[1].inst & 0x3e00000) >> 21;
	idex_latch[0].rt = (ifid_latch[1].inst & 0x1f0000) >> 16;
	idex_latch[0].rd = (ifid_latch[1].inst & 0xf800) >> 11;
	idex_latch[0].shamt = (ifid_latch[1].inst & 0x7c0) >> 6;
	idex_latch[0].funct = (ifid_latch[1].inst & 0x3f);
	idex_latch[0].v1 = R[idex_latch[0].rs];
	idex_latch[0].v2 = R[idex_latch[0].rt];

          
	printf("Dec: Rtype rs=%d rt=%d rd=%d shamt=%d funct=%d ",idex_latch[0].rs,idex_latch[0].rt,idex_latch[0].rd,idex_latch[0].shamt,idex_latch[0].funct);
}
//Jtype
else if(idex_latch[0].opcode == 0x2||idex_latch[0].opcode == 0x3)
{
	idex_latch[0].address = (ifid_latch[1].inst & 0x3ffffff);
	printf("Dec: Jtype address=%d ",idex_latch[0].address);
}
//Itype
else
{     
	idex_latch[0].rs = (ifid_latch[1].inst & 0x3e00000) >> 21;
	idex_latch[0].rt = (ifid_latch[1].inst & 0x1f0000) >> 16;
	idex_latch[0].immediate = (ifid_latch[1].inst & 0xffff);
	idex_latch[0].v1 = R[idex_latch[0].rs];
	idex_latch[0].v2 = R[idex_latch[0].rt];
//Simmediate
	if((ifid_latch[1].inst & 0x0000ffff) >= 0x8000)
	{
		idex_latch[0].Simmediate = 0xffff0000 | (ifid_latch[1].inst & 0x0000ffff); 
	}
	else
	{ 
		idex_latch[0].Simmediate = (ifid_latch[1].inst & 0x0000ffff);
	}
        
	printf("Dec: rs=%d rt=%d imm=%X Simm=%X ",idex_latch[0].rs,idex_latch[0].rt,idex_latch[0].immediate,idex_latch[0].Simmediate);
}

printf("(opcode=0x%X)\n",idex_latch[0].opcode);
idex_latch[0].JumpAddr = ifid_latch[1].pc & 0xf0000000 | idex_latch[0].address << 2;

//Jump------------------------------------------------------------ 
if(idex_latch[0].opcode == 0x2)//j
{
	pc = idex_latch[0].JumpAddr;
	printf("Jump / pc = %d",idex_latch[0].JumpAddr);
}
else if(idex_latch[0].opcode == 0x3)//jal
{
	R[31] = idex_latch[0].pc + 8;
	pc = idex_latch[0].JumpAddr;
	printf("Jal / R[31]=pc+8, pc=%d",idex_latch[0].JumpAddr);
}

//-----------------------------------------------------------------

	idex_latch[0].pc = ifid_latch[1].pc;
	idex_latch[0].inst = ifid_latch[1].inst;  

//control
if(idex_latch[0].opcode == 0x0)
{
	idex_latch[0].RegDst = 1;
}
else
{
	idex_latch[0].RegDst = 0;
}
if(idex_latch[0].opcode == 0x23)
{
	idex_latch[0].MemtoReg = 1;
}
else
{
	idex_latch[0].MemtoReg = 0;
}
if((idex_latch[0].opcode != 0x2b)&&(idex_latch[0].opcode != 0x2)&&(idex_latch[0].opcode != 0x3)&&(idex_latch[0].opcode != 0x4)&&(idex_latch[0].opcode != 0x5)&&(idex_latch[0].funct != 0x8))
{
	idex_latch[0].RegWrite = 1;
}
else
{
	idex_latch[0].RegWrite = 0;
}
if(idex_latch[0].opcode == 0x2b)
{
	idex_latch[0].MemWrite = 1;
}
else
{
	idex_latch[0].MemWrite = 0;
}
if(idex_latch[0].opcode == 0x23)
{
	idex_latch[0].MemRead = 1;
}
else
{
	idex_latch[0].MemRead = 0;
}
if((idex_latch[0].opcode ==0x4)||(idex_latch[0].opcode == 0x5))
{
	idex_latch[0].Branch = 1;
}
else
{
	idex_latch[0].Branch = 0;
}
}


void execute()
{

int v1;
int v2;

idex_latch[1].BranchAddr = idex_latch[1].Simmediate << 2;
idex_latch[1].ZeroExtImm = (idex_latch[1].inst & 0x0000ffff);

//datadependency--------------------------------------------------------------------
if((idex_latch[1].rs!=0)&&(idex_latch[1].rs==exmem_latch[0].reg)&&(exmem_latch[0].RegWrite)) 
{
	idex_latch[1].v1  = exmem_latch[0].ALUresult;
	printf("*Dependency rs dist1*\n");
}

else if((idex_latch[1].rs!=0) && (idex_latch[1].rs==memwr_latch[0].reg) && (memwr_latch[0].RegWrite))
{
	if(memwr_latch[0].MemtoReg==1)
	{
		idex_latch[1].v1 = memwr_latch[0].MEMresult;
		printf("*Dependency rs dist2(lw)*\n");
	}  
        else
	{
		idex_latch[1].v1 = memwr_latch[0].ALUresult;
		printf("*Dependency rs dist2*\n");
	}
}
 
if((idex_latch[1].rt!=0)&&(idex_latch[1].rt==exmem_latch[0].reg)&&(exmem_latch[0].RegWrite))
{
	idex_latch[1].v2 = exmem_latch[0].ALUresult;
	printf("*Dependency rt dist1*\n");
}

else if((idex_latch[1].rt!=0) && (idex_latch[1].rt==memwr_latch[0].reg) && (memwr_latch[0].RegWrite))
{
	if(memwr_latch[0].MemtoReg==1)
	{
		idex_latch[1].v2 = memwr_latch[0].MEMresult;
		printf("*Dependency rt dist2(lw)*\n");
	}
	else
	{
		idex_latch[1].v2 = memwr_latch[0].ALUresult;
		printf("*Dendency rt dists*\n");
	}
}

//------------------------------------------------------------------------
v1 = idex_latch[1].v1;
v2 = idex_latch[1].v2;

 switch(idex_latch[1].opcode)
{
case 0x0://Rtype
	switch(idex_latch[1].funct)
	{
	case 0x20://add
		exmem_latch[0].ALUresult = v1+v2;
    		printf("EXE: add\n");
		break;
	case 0x21: //addu
		exmem_latch[0].ALUresult = v1+v2;
		printf("EXE: addux\n");
		break;
	case 0x24://and 
		exmem_latch[0].ALUresult = v1&v2;
		printf("EXE: and\n");
		break;
	case 0x08://jr
                pc=v1;
                printf("EXE: jr  pc=%d\n",pc);
                break;
	case 0x27://nor
                exmem_latch[0].ALUresult =~(v1|v2);
		printf("EXE: nor\n");
                break;
	case 0x25://or
                exmem_latch[0].ALUresult = v1|v2;
		printf("EXE: or\n");
                break;
	case 0x2a://slt
                exmem_latch[0].ALUresult = (v1<v2)?1:0;
		printf("EXE: slt\n");
                break;
	case 0x2b://sltu
                exmem_latch[0].ALUresult = (v1<v2)?1:0;
		printf("EXE: sltu\n");
                break;
	case 0x00://sll
                  exmem_latch[0].ALUresult = v2<<idex_latch[1].shamt;
		 if(idex_latch[1].inst == 0x00000000)
		  {
		   printf("EXE: nop inst \n");
		  }
		  else
		  printf("EXE: sll\n");
                 break;
	case 0x02://srl
                 exmem_latch[0].ALUresult = v2>>idex_latch[1].shamt;
		 printf("EXE: srl\n");
                 break;
	case 0x22://sub
                 exmem_latch[0].ALUresult = v1-v2;
		 printf("EXE: sub\n");
                 break;
	case 0x23://subu
                 exmem_latch[0].ALUresult = v1-v2;
		 printf("EXE: subu\n");
                 break;
	default:
                 break;
            }
break;
//Itype
case 0x8://addi
	exmem_latch[0].ALUresult = v1 + idex_latch[1].Simmediate;
	printf("EXE: addi\n");
	break;
case 0x9://addiu
	exmem_latch[0].ALUresult = v1 + idex_latch[1].Simmediate;
	printf("EXE: addiu\n");
	break;
case 0xc://andi
	exmem_latch[0].ALUresult = v1 & idex_latch[1].ZeroExtImm;
	printf("EXE: andi\n");
	break;
case 0x4://beq
	if(v1==v2)
	{	
	pc = idex_latch[1].pc + idex_latch[1].BranchAddr + 4;
	memset(&ifid_latch[0], 0, sizeof(ifid));
	memset(&idex_latch[0], 0, sizeof(idex));
	}
	else
	{ }
	printf("EXE: Beq\n");
	break;
case 0x5://bne
	if(v1!=v2)
	{
	pc = idex_latch[1].pc + idex_latch[1].BranchAddr + 4;
	memset(&ifid_latch[0], 0, sizeof(ifid));
	memset(&idex_latch[0], 0, sizeof(idex));
	}
	else
	{
	}
	printf("EXE: Bne\n");
        break;
case 0xf://lui
	exmem_latch[0].ALUresult = idex_latch[1].immediate << 16;
	printf("EXE: lui\n");
	break;
case 0xd://ori
	exmem_latch[0].ALUresult = v1|idex_latch[1].ZeroExtImm;
	printf("EXE: ori\n");
	break;
case 0xa://slti
	exmem_latch[0].ALUresult = (v1 < idex_latch[1].Simmediate) ? 1 : 0;
	printf("EXE: slti\n");
	break;
case 0xb://sltiu
	exmem_latch[0].ALUresult = (v1 < idex_latch[1].Simmediate) ? 1 : 0;
	printf("EXE: sltiu\n");
	break;
case 0x23://lw
	exmem_latch[0].ALUresult = v1 + idex_latch[1].Simmediate;
	printf("EXE: lw\n");
	break;
case 0x2b://sw
	exmem_latch[0].ALUresult = v2;
	exmem_latch[0].Index = v1 + idex_latch[1].Simmediate;
	printf("EXE: sw\n"); 
	break;
default:
	break;
 }
   
//control
 if(idex_latch[1].RegDst == 1)
 {
         exmem_latch[0].reg = idex_latch[1].rd;
 }
 else
 {
         exmem_latch[0].reg = idex_latch[1].rt;
 }

exmem_latch[0].pc = idex_latch[1].pc; 
exmem_latch[0].inst = idex_latch[1].inst;
exmem_latch[0].MemWrite = idex_latch[1].MemWrite;
exmem_latch[0].MemRead = idex_latch[1].MemRead;
exmem_latch[0].MemtoReg = idex_latch[1].MemtoReg;
exmem_latch[0].RegWrite = idex_latch[1].RegWrite;
}

void memoryaccess()
{
if(exmem_latch[1].MemWrite == 1)//sw
{ 
	WriteMem(exmem_latch[1].Index,exmem_latch[1].ALUresult);
	WriteM_count++;
	printf("MEM: MemWrite %d\n", exmem_latch[1].MemWrite);
}

if(exmem_latch[1].MemRead == 1)//lw
{ 
	memwr_latch[0].MEMresult = ReadMem(exmem_latch[1].ALUresult);
	ReadM_count++;
	printf("MEM: MemRead %d\n", exmem_latch[1].MemRead);
}

//---------------------------------------------------------------------------
memwr_latch[0].ALUresult = exmem_latch[1].ALUresult;
memwr_latch[0].inst = exmem_latch[1].inst;
memwr_latch[0].reg = exmem_latch[1].reg;
memwr_latch[0].MemtoReg = exmem_latch[1].MemtoReg;
memwr_latch[0].RegWrite = exmem_latch[1].RegWrite;
}


void writeback()
{
//-------------------------------------------------------------------------
if(memwr_latch[1].RegWrite ==1)
{
	if(memwr_latch[1].MemtoReg == 1)
	{
		R[memwr_latch[1].reg] = memwr_latch[1].MEMresult;
		printf(" WB: MemtoReg %d\n", memwr_latch[1].MemtoReg);
	}
	else
	{
		R[memwr_latch[1].reg] = memwr_latch[1].ALUresult;
		printf(" WB: RegWrite %d\n", memwr_latch[1].RegWrite);
	}
}
else
{
	printf(" WB: No Writeback\n");
}
}

void main(int argc,char *argv[])
{
FILE *fp = NULL;
char *path = "input4.bin";
int res;
int val,i=0;
R[31]=-1;
R[29]=0x400000;

if(argc == 2)
{
	path = argv[1];
}

fp = fopen(path, "rb");

if(fp == NULL)
{
	printf("invalid input file : %s\n", path);
	return ;
}

while (1)
{
int inst;
res = fread(&val, sizeof(val), 1, fp);
 
//swap
inst = (val & 0xFF) << 24
|(val & 0xFF00) << 8
|(val & 0xFF0000) >>8
|(val & 0xFF000000) >>24;

Memory[i] = inst;
i++;
if(res == 0)
break;
}

while(pc!=-1)
{
	printf("\n---------------------------------------------\n");
	cyclen++;
	printf("cycle=%d / pc=%X\n",cyclen,pc); 
	fetch();
	writeback();
	decode();
	execute();
	memoryaccess();

//latch update 
	ifid_latch[1] = ifid_latch[0];
	idex_latch[1] = idex_latch[0];
	exmem_latch[1] = exmem_latch[0];
	memwr_latch[1] = memwr_latch[0];
}

Hit_rate = (double)(hit_count)/(hit_count+miss_count) * 100;
Miss_rate = (double)(miss_count)/(hit_count+miss_count) * 100;

printf("\n===========================================\n");
printf("\nFinal R[2]=%d\n",R[2]);
printf("Number of ReadMemory = %d\n",ReadM_count);
printf("Number of WriteMemory = %d\n",WriteM_count);
printf("Number of coldmiss = %d\n",coldmiss_count);
printf("Number of Hit = %d\n",hit_count);
printf("Number of Miss = %d\n",miss_count);
printf("Hit rate = %f\n", Hit_rate);
printf("Miss rate = %f\n", Miss_rate);
fclose(fp);
}
