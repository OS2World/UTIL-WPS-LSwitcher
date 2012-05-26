#define INCL_DOS
#define INCL_DOSERRORS
#include <os2.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>


void getinfo()
{ char buf[1024];
  char *pbuf,*pbuf1,*pbuf2;
  ULONG rc,ulEnv,ulPTDA,ulSeg,ulErrorClass,ulErrorAction,ulErrorLoc;
  PID pid;
  USHORT cb,usEnvHob,usEnvHar;
  FILE *f;
  char buf1[1000];
  
  for (rc=0,pid=0;rc==0 && pid!=0x3a;) {
    rc=DosPerfSysCall(0xC5,(ULONG)buf,sizeof(buf),0); //get thread info
    pid=(PID)(*((USHORT*)&buf[18]));
  }  
  ulPTDA=*(ULONG*)&buf[12];
  printf("%i PID %X PTDA %X\n",rc,pid,ulPTDA);

  rc=DosPerfSysCall(0xC1,(ULONG)buf,sizeof(buf),ulPTDA); //get PTDA
  usEnvHob=*((USHORT*)&buf[272]);
  printf("%i ENVHOB %04X\n",rc,usEnvHob);
  
  rc=DosPerfSysCall(0xD0,(ULONG)buf,0x10,(ULONG)usEnvHob); //get env hob info  
  usEnvHar=*((USHORT*)&buf[0]);
  printf("%i ENVHAR %04X\n",rc,usEnvHar);
  
  rc=DosPerfSysCall(0xCD,(ULONG)buf,0x16,(ULONG)usEnvHar); //get env arena record
  ulEnv=(ULONG)(*((USHORT*)&buf[6]))<<16;
  printf("%i ENV ADDR %08X\n",rc,ulEnv);
  

  DosAllocMem((void**)&pbuf2,0x1000,0x13);
  rc=DosPerfSysCall(0xDB,(ULONG)pbuf2,ulPTDA,0);  
  printf("0xDB %i\n",rc);


  DosAllocMem((void**)&pbuf,0x1000,0x13);
//get linear memory info
  rc=DosPerfSysCall(0xDC,(ULONG)pbuf,ulEnv,ulPTDA);
  printf("0xDC %i\n",rc);
  f=fopen("DC.TXT","w");
  fwrite(pbuf,0x1000,1,f);
  fclose(f);


  DosAllocMem((void**)&pbuf1,0x114,0x13);
  memset(pbuf1,0,0x114);

  rc=DosPerfSysCall(0xD9,(ULONG)&pbuf1,0,0);
  printf("%i %p %p %s\n",rc,pbuf1,&pbuf1,pbuf1);

  f=fopen("D9.TXT","w");
  fwrite(pbuf1,0x114,1,f);
  fclose(f);

  DosFreeMem(pbuf);
  DosFreeMem(pbuf1);
  DosFreeMem(pbuf2);
}  


void main()
{
  getinfo();
}  
