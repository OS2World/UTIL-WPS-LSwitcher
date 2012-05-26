#define INCL_DOS
#include <os2.h>
#include <stdio.h>
#include <stdlib.h>

void main()
{ static UCHAR pBuf[1024],szPidPath[512];
  FILE *f;
  QSPTRREC *pRecHead;
  QSPREC *pApp;

  DosQuerySysState(QS_PROCESS, 0, 0xdc, 0, (char *) pBuf, sizeof(pBuf));
  pRecHead = (QSPTRREC *)pBuf;
  pApp = pRecHead->pProcRec;
  printf("%X %X %i\n",pApp->pid,pApp->ppid,pApp->type);
  
  DosQuerySysState(QS_PROCESS, 0, pApp->ppid, 0, (char *) pBuf, sizeof(pBuf));
  pRecHead = (QSPTRREC *)pBuf;
  pApp = pRecHead->pProcRec;
  DosQueryModuleName(pApp->hMte, sizeof(szPidPath), szPidPath);
  printf("%X %X %i %s\n",pApp->pid,pApp->ppid,pApp->type,szPidPath);
  
}
