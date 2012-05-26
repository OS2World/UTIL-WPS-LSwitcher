#define INCL_PM
#include <os2.h>
#include <stdio.h>
#include <string.h>


void main()
{ static UCHAR ucKey[16],ucObjData[65000],ucKeyData[1024],*substr;
  ULONG k,l,ulObjDataSize=sizeof(ucObjData),ulKeyDataSize;
  USHORT w;
  FILE *f;

  if (PrfQueryProfileData(HINI_USERPROFILE,"PM_Abstract:Objects",NULL,ucObjData,&ulObjDataSize)) {
    f=fopen("abstract.txt","w");
    fwrite(ucObjData, ulObjDataSize,1,f); 
    fclose(f);
  
   for (k=0;ucObjData[k]!='\0';k+=strlen(ucKey)+1) {
     strncpy(ucKey,&ucObjData[k],sizeof(ucKey)-1);
     printf("%s",ucKey);
     ulKeyDataSize=sizeof(ucKeyData);
     if (PrfQueryProfileData(HINI_USERPROFILE,"PM_Abstract:Objects",ucKey,ucKeyData,&ulKeyDataSize)) {
       for (l=0,substr=NULL;substr==NULL && l<ulKeyDataSize-strlen("WPAbstract");l++) 
         if ((substr=strstr(&ucKeyData[l],"WPAbstract"))!=NULL) substr+=17;
       printf(" %s %s cmd=%s",&ucKeyData[4],substr,&ucKeyData[0xb9]);
       
       for (l=0,substr=NULL;substr==NULL && l<ulKeyDataSize-strlen("WPProgramRef");l++) 
         if ((substr=strstr(&ucKeyData[l],"WPProgramRef"))!=NULL) {
           substr+=19; 
           memcpy(&w,substr,2);
           printf(" %X",w);
         }
     }  
     printf("\n");
   }
  } 
  
  ulObjDataSize=sizeof(ucObjData);
  if (PrfQueryProfileData(HINI_SYSTEMPROFILE,"PM_Workplace:Handles0","BLOCK1",ucObjData,&ulObjDataSize)) {
    f=fopen("block1.txt","w");
    fwrite(ucObjData, ulObjDataSize,1,f); 
    fclose(f);
  }  
printf("%X\n",153764);
}