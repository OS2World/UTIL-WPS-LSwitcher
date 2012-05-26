#define INCL_DOSPROCESS
#define INCL_DOSERRORS
#include <os2.h>
#include <stdio.h>
#include <string.h>

void main(VOID)
{
   PTIB   ptib = NULL;          /* Thread information block structure  */
   PPIB   ppib = NULL;          /* Process information block structure */
   UCHAR buf[128];
   
   DosGetInfoBlocks(&ptib, &ppib);
   for (;*ppib->pib_pchenv!='\0';ppib->pib_pchenv+=strlen(buf)+1) {
     strncpy(buf,ppib->pib_pchenv,sizeof(buf)-1);
     printf("%s\n",buf);
   }
   
}