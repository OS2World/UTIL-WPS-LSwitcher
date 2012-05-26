#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#define INCL_WIN
#define INCL_DOS
#define INCL_WPERRORS
#include <os2.h>

#include "object.h"

void main(int argc, char *argv[])
{ char psz[MAX_PATH],*endptr;
  unsigned objid=0;

  if (argc<2) return;
  objid=strtol(argv[1],&endptr,10);
  if (*endptr!='\0') return;
  GetObjectParameters(objid, psz, sizeof(psz));
  printf("%s -- ",psz);
  GetAbstractName(objid, psz, sizeof(psz));
  printf("%s\n",psz);
  GetObjectName(objid, psz, sizeof(psz));
  printf("%s\n",psz);
  
/*
  objid=ObjIDFromName(argv[1], argc<3?"":argv[2], PROG_WINDOWABLEVIO, TRUE);
  GetObjectTitle(objid, psz, sizeof(psz));
  printf("%x %s ",objid,psz);
  GetObjectParameters(objid, psz, sizeof(psz));
  printf("%s\n",psz);
  printf("%u\n",GetObjectProgType(objid));
  
  printf("%x\n",GetObjectIcon(objid+0x10000,NULL));
 */ 
}
