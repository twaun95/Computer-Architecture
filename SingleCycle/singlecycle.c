#include <stdio.h>
int Memory[0x800000]={0, };
int R[32]={0,};
int pc =0x0;
int result,val,i=0;
unsigned int opcode;
int Rn,In,Jn,cyclen,Minstn,Branchn=0;
int rs,rt,rd,funct,immediate,shamt,address,inst;
int Simmediate,JumpAddr,BranchAddr;

void fetch()
{
    inst = Memory[pc/4];   
    printf("instruction=0x%08X ",inst);
}

void decode()
{
    opcode=(inst & 0xfc000000) >> 26; //opcode

    printf("(opcode=0x%X)\n",opcode);
  if(opcode==0x4||opcode==0x5||opcode==0x8||opcode==0x9||opcode==0x23||opcode==0x24||opcode==0x25||opcode==0x28||opcode==0x29||opcode==0x30||opcode==0x38||opcode==0xa||opcode==0xb||opcode==0xc||opcode==0xd||opcode==0x2b||opcode==0xf)
    {
      printf("I type  ");
      rs = (inst & 0x3e00000) >> 21;
      rt = (inst & 0x1f0000) >> 16;
      immediate = (inst & 0xffff);  

      printf("rs=%d ",rs);
      printf("rt=%d ",rt);
      printf("immediate=%d\n",immediate);
      In++;
      }
 else if(opcode == 0x0)
{
         printf("R type  ");
         rs = (inst & 0x3e00000) >> 21;
         rt = (inst & 0x1f0000) >> 16;
         rd = (inst & 0xf800) >> 11;
         shamt = (inst & 0x7c0) >> 6;
         funct = (inst & 0x3f);

         printf("rs=%d ",rs);
         printf("rt=%d ",rt);
         printf("rd=%d ",rd);
         printf("shamt=%d ",shamt);
         printf("funct=%x\n",funct);
         Rn++;
}
 else if(opcode == 0x2||opcode == 0x3)
{
         printf("J type  "); 
         address = (inst & 0x3ffffff);
         printf("address=%d\n",address);
         Jn++;
}
}

void execute()
{  
 int JumpAddr = pc & 0xf0000000 | address << 2;
 int ZeroExtImm = immediate;//(inst & 0x0000ffff);
  
    if((inst & 0xffff) >= 0x8000)
      { Simmediate = 0xffff0000 | (inst & 0xffff); }
     else
      { Simmediate = (inst & 0xffff);  }
 int BranchAddr = Simmediate << 2; 

 switch(opcode)
{     
  case 0x0://Rtype
          switch(funct)
           {
             case 0x20://add
                  result=R[rs]+R[rt];
                  printf("add\n");
                  break;
             case 0x21: //addu
                  result=R[rs]+R[rt];
                  printf("addu\n");
                  break;           
             case 0x24://and
                  result=R[rs]+R[rt];                 
                  printf("and\n");
                  break;      
             case 0x08://jr
                  pc=R[rs];
                  printf("jr inst\n");
                  break;
             case 0x27://nor
                  result=!(R[rs]|R[rt]);
                  printf("nor\n");
                  break;
             case 0x25://or
                  result=R[rs]|R[rt];               
                  printf("or\n");
                  break;
             case 0x2a://slt
                  result=(R[rs]<R[rt])?1:0;
                  printf("slt\n");
                  break;
             case 0x2b://sltu
                  result=(R[rs]<R[rt])?1:0;                
                  printf("sltu\n");
                  break;
             case 0x00://sll
                    if(inst==0x00000000)
                      { printf("nop inst\n");}
                    else
                   {result=R[rt]<<shamt;
                    printf("sll\n");}
                  break;
             case 0x02://srl
                  result=R[rt]>>shamt;                
                  printf("srl\n");
                  break;
             case 0x22://sub
                  result=R[rs]-R[rt];                
                  printf("sub\n");
                  break;
             case 0x23://subu
                  result=R[rs]-R[rt];                
                  printf("subu\n");
                  break;
             default:
                  break;
           }
       break;
//Itype
  case 0x8://addi
           result=R[rs] + Simmediate;
           printf("addi\n");
           break;
  case 0x9://addiu
           result=R[rs] + Simmediate;         
           printf("addiu\n");
           break;
  case 0xc://andi
           result=R[rs] & ZeroExtImm;         
           printf("andi\n");
           break;
  case 0x4://beq
           if(R[rs]==R[rt])
              { pc=pc+4+BranchAddr;
                Branchn++;}
           else
           { pc=pc+4;}
           printf("beq  \n");
           break;
  case 0x5://bne
//	   printf("%d %d \n",R[rs],R[rt]);
           if(R[rs]!=R[rt])
             { pc=pc+4+BranchAddr;
               Branchn++;}
//		printf("%x\n", BranchAddr);}
           else
           {  pc=pc+4;}
           printf("bne \n");
           break;
  case 0xf://lui
           result=immediate << 16;
           printf("lui\n");
           break;
  case 0xd://ori
           result=R[rs]|ZeroExtImm;         
           printf("ori\n");
           break;
  case 0xa://slti
           result=(R[rs]<Simmediate) ? 1 : 0;         
           printf("slti\n");
           break;
  case 0xb://sltiu
           result=(R[rs]<Simmediate) ? 1 : 0;         
           printf("sltiu\n");
           break;
//Jtype
   case 0x2://jump
             pc=JumpAddr;
             printf("inst jump\n");
             break;
   case 0x3://jal
            R[31]=pc+8;
            pc=JumpAddr;
            printf("inst jal \n");
            break;
   default:
           break;
}
}

void memaddr()
{
switch(opcode)
{
    case 0x24://ibu m
             result=Memory[R[rs]+Simmediate] & 0xff;           
             printf("ibu\n");
             Minstn++;
             break;
    case 0x25://ihu m
             result=Memory[R[rs]+Simmediate] & 0xffff;           
             printf("ihu\n");
             Minstn++;
             break;
    case 0x30://ll m 
              result=Memory[R[rs]+Simmediate];           
              printf("ll\n");
              Minstn++;
              break;
    case 0x23://lw m
              result=Memory[R[rs]+Simmediate];           
              printf("lw\n");
              Minstn++;
              break;
    case 0x28://sb m
              Memory[R[rs]+Simmediate] = R[rt] & 0xff;           
              printf("sb\n");
              Minstn++;
              break;
//    case 0x38://sc m              
    case 0x29://sh m
              Memory[R[rs]+Simmediate] = R[rt] & 0xffff;           
              printf("sh\n");
              Minstn++;
              break;
    case 0x2b://sw  m
             Memory[R[rs]+Simmediate]=R[rt]; 
	     pc=pc+4;
             Minstn++;		
             printf("sw\n");
             break;

  default:
           break;
}
}

void writeback()
{
  switch(opcode)
  {
    case 0x0://Rtype 
          if(rd!=0)
           R[rd]=result;
//           printf("R[%d]<-%X   R[2]=%X\n",rd,result,R[2]);        
           break;     
    case 0x2://j
//            printf("R[2]=%X\n",R[2]);        
            break;
    case 0x3://jal
//            printf("R[2]=%X\n",R[2]);           
            break;
    default://Itype
           if(rt!=0)
            R[rt]=result;
//            printf("R[%d]<-%X   R[2]=%X\n",rt,R[rt],R[2]);
            break;
  }
//pc value  
if(opcode!=0x4&&opcode!=0x5&&opcode!=0x2&&opcode!=0x3&&funct!=0x8)
 pc=pc+4;
}

void main(int argc,char *argv[])
{
   FILE *fp = NULL;
   char *path = "gcd.bin";
   int res;

   R[31]=-1;
   R[29]=0x800000;

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
  while(pc/4<=i&&pc!=-1)
{  
printf("---------------------------------------------\n"); 
printf("cycle number=%d / pc=%X\n",cyclen,pc);
   fetch();
   decode();
   execute();
   memaddr(); 
  if(opcode!=0x2b)  
   {writeback();}
  
  cyclen++;
}

  printf("\nFinal R[2]=%d\n",R[2]);
  printf("R_inst_num=%d I_inst_num=%d J_inst_num=%d\nmemoryaccessnum=%d Branchnum=%d  cycle number=%d\n\n",Rn,In,Jn,Minstn,Branchn,cyclen);

 fclose(fp);
}

