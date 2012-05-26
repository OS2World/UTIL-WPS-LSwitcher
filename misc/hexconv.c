#include <stdio.h>
#include <string.h>
#include <stdlib.h>

void main(int argc, char* argv[])
{ long v;

  if (argc<3) return;
  if (stricmp(argv[1],"x")) { 
    v=strtol(argv[2],NULL,10);
    printf("%x\n",v);
  } else { 
    v=strtol(argv[2],NULL,16);
    printf("%u\n",v);
  }  
}