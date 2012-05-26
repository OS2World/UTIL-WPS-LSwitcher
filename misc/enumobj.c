#define INCL_WINWORKPLACE
#include <os2.h>
#include <stdio.h>
#include <stdlib.h>

ULONG    ulSize;
POBJCLASS pObjClass;
BOOL      rc;
PVOID pmem;

void main() {
  /* Get the buffer size required to hold all registered classes */
  if (WinEnumObjectClasses(NULL, &ulSize) && (pmem = (PVOID) malloc (ulSize)))
  {
    pObjClass = (POBJCLASS)pmem;
    rc = WinEnumObjectClasses(pObjClass, &ulSize);
    if (rc)
    {
      printf("success\n");
      do {
      /* pObjClass contains all classes registered with the system */
        printf("%s %s\n",pObjClass->pszClassName,pObjClass->pszModName);
        pObjClass=pObjClass->pNext;
      } while (pObjClass!=NULL);
    }
    else
    {
      /* WinEnumObjectClasses failed */
    }
  }  
}
