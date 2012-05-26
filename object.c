/* most of this code is taken from Henk Kelder's Hektools package */

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#define INCL_WIN
#define INCL_DOS
#define INCL_GPI
#define INCL_WPERRORS
#include <os2.h>

#include "object.h"

#define OBJECT_ABSTRACT  0x0001
#define OBJECT_ABSTRACT2 0x0002
#define OBJECT_FSYS      0x0003

USHORT GetObjectData       (PBYTE pObjData, PSZ pszObjName, USHORT usTag, PVOID pOutBuf, USHORT usMaxBuf);
BOOL GetObjectDataString(PBYTE pObjData, PSZ pszObjName, USHORT usTag,
                         USHORT usStringTag, PVOID pOutBuf, USHORT usMax);
BOOL GetActiveHandles(HINI hIniSystem, PSZ pszHandlesAppName, USHORT usMax);

PVOID GetProfileData(PSZ pszApp, PSZ pszKey, HINI hini, PULONG pulProfileSize)
{  PBYTE pBuffer;

   if (!PrfQueryProfileSize(hini, pszApp, pszKey, pulProfileSize))
     return NULL;

   if (*pulProfileSize == 0)
     return NULL;

   pBuffer = malloc(*pulProfileSize);
   if (!pBuffer) {
     printf("Not enough memory for profile data!, need %u bytes\n", pulProfileSize);
     return NULL;
   }

   if (!PrfQueryProfileData(hini,pszApp,pszKey,pBuffer, pulProfileSize)) {
     printf("Error while retrieving profile data!\n");
     free(pBuffer);
     return NULL;
   }
   return pBuffer;
}


/*************************************************************
* Get active handles
*************************************************************/
BOOL GetActiveHandles(HINI hIniSystem, PSZ pszHandlesAppName, USHORT usMax)
{  PBYTE pBuffer;
   ULONG ulProfileSize;

   pBuffer = GetProfileData(ACTIVEHANDLES, HANDLESAPP, hIniSystem,&ulProfileSize);
   if (!pBuffer) {
      strncpy(pszHandlesAppName, HANDLES, usMax - 1);
      return TRUE;
   }
   strncpy(pszHandlesAppName, pBuffer, usMax -1);
   free(pBuffer);
   return TRUE;
}

PVOID GetAllProfileNames(PSZ pszApp, HINI hini, PULONG pulProfileSize)
{  PBYTE pBuffer;

   if (!PrfQueryProfileSize(hini, pszApp, NULL, pulProfileSize))
      return NULL;

   if (*pulProfileSize == 0)
      return NULL;

   pBuffer = malloc(*pulProfileSize);
   if (!pBuffer) {
      printf("Not enough memory for profile data! (need %u bytes)\n", pulProfileSize);
      return NULL;
   }

   if (!PrfQueryProfileData(hini, pszApp, NULL, pBuffer, pulProfileSize)) {
      printf("Error while retrieving profile data!\n");
      free(pBuffer);
      return NULL;
   }

   return pBuffer;
}


/*************************************************************
* Read all BLOCK's
*************************************************************/
BOOL fReadAllBlocks(HINI hiniSystem, PSZ pszActiveHandles, PBYTE * ppBlock, PULONG pulSize)
{  PBYTE pbAllBlocks, pszBlockName, p;
   ULONG ulBlockSize, ulTotalSize;
   BYTE  rgfBlock[100], szAppName[10];
   USHORT usBlockNo;

   pbAllBlocks = GetAllProfileNames(pszActiveHandles, hiniSystem, &ulBlockSize);
   if (!pbAllBlocks)
      return FALSE;

   /* Query size of all individual blocks and calculate total */
   memset(rgfBlock, 0, sizeof rgfBlock);
   ulTotalSize = 0L;
   pszBlockName = pbAllBlocks;
   while (*pszBlockName) {
     if (!memicmp(pszBlockName, "BLOCK", 5)) {
       usBlockNo = atoi(pszBlockName + 5);
       if (usBlockNo < 100) {
         rgfBlock[usBlockNo] = TRUE;

         if (!PrfQueryProfileSize(hiniSystem, pszActiveHandles, pszBlockName, &ulBlockSize)) {
           free(pbAllBlocks);
           return FALSE;
         }
         ulTotalSize += ulBlockSize;
       }
     }
     pszBlockName += strlen(pszBlockName) + 1;
   }

   /* *pulSize contains total size of all blocks */
   *ppBlock = malloc(ulTotalSize);
   if (!(*ppBlock)) {
     printf("Not enough memory for profile data!\n");
     free(pbAllBlocks);
     return FALSE;
   }

   /* Now get all blocks  */
   p = *ppBlock; (*pulSize) = 0;
   for (usBlockNo = 1; usBlockNo < 100; usBlockNo++) {
     if (!rgfBlock[usBlockNo])
       break;

     sprintf(szAppName, "BLOCK%u", (UINT)usBlockNo);
     ulBlockSize = ulTotalSize;
     if (!PrfQueryProfileData(hiniSystem, pszActiveHandles, szAppName, p, &ulBlockSize)) {
       free(pbAllBlocks);
       free(*ppBlock);
       return FALSE;
     }
     p += ulBlockSize;
     (*pulSize) += ulBlockSize;
     ulTotalSize -= ulBlockSize;
   }

   free(pbAllBlocks);
   return TRUE;
}

/*************************************************************
* Get the object partname as specified in usID
*************************************************************/
PNODE GetPartName(PBYTE pHandlesBuffer, ULONG ulBufSize, USHORT usID, PSZ pszFname, USHORT usMax, PBYTE pfHandleUsed)
{  PDRIV pDriv = NULL;
   PNODE pNode;
   PBYTE p, pEnd;
   USHORT usSize;

   pEnd = pHandlesBuffer + ulBufSize;
   p = pHandlesBuffer + 4;
   while (p < pEnd) {
     if (!memicmp(p, "DRIV", 4)) {
       pDriv = (PDRIV)p;
       p += sizeof(DRIV) + strlen(pDriv->szName);
     } else if (!memicmp(p, "NODE", 4)) {
       pNode = (PNODE)p;
       p += sizeof (NODE) + pNode->usNameSize;
       if (pNode->usID == usID) {
         if (pfHandleUsed)
           pfHandleUsed[usID] = TRUE;
         usSize = usMax - strlen(pszFname);
         if (usSize > pNode->usNameSize)
           usSize = pNode->usNameSize;
         if (pNode->usParentID) {
           if (!GetPartName(((PBYTE)pDriv) - 4,
                            ulBufSize - ((PBYTE)pDriv - pHandlesBuffer),
                            pNode->usParentID, pszFname, usMax, pfHandleUsed))
             return FALSE;
           strcat(pszFname, "\\");
           strncat(pszFname, pNode->szName, usSize);
           return pNode;
         } else {
           strncpy(pszFname, pNode->szName, usSize);
           return pNode;
         }
       }
     }
     else
       return NULL;
   }
   return NULL;
}


/*********************************************************************
* Is Object Abstract
*********************************************************************/
BOOL IsObjectAbstract(HOBJECT hObject)
{
  return (HIUSHORT(hObject) == OBJECT_ABSTRACT || HIUSHORT(hObject) == OBJECT_ABSTRACT2);
}

/*********************************************************************
* Is Object Abstract
*********************************************************************/
BOOL IsObjectDisk(HOBJECT hObject)
{
  return (HIUSHORT(hObject) == OBJECT_FSYS);
}

/*************************************************************
* Get the object name as specified in hObject
*************************************************************/
BOOL GetObjectName(HOBJECT hObject, PSZ pszFname, USHORT usMax)
{ USHORT usObjID;
  PBYTE pBuffer;
  HINI hini = HINI_SYSTEMPROFILE;
  ULONG ulProfileSize;
  BYTE  szHandles[100];

  usObjID  = LOUSHORT(hObject);
  if (IsObjectAbstract(hObject))
    return GetAbstractName(hObject, pszFname, usMax);

  if (!IsObjectDisk(hObject))
    return FALSE;

  GetActiveHandles(hini, szHandles, sizeof szHandles);

  if (!fReadAllBlocks(hini, szHandles, &pBuffer, &ulProfileSize))
    return FALSE;

  memset(pszFname, 0, usMax);

  if (!GetPartName(pBuffer, ulProfileSize, usObjID, pszFname, usMax, NULL)) {
    free(pBuffer);
    return FALSE;
  }
  free(pBuffer);
  return TRUE;
}


/*************************************************************
* Get Abstract name
*************************************************************/
BOOL GetAbstractName(HOBJECT hObject, PSZ pszExe, USHORT usMax)
{  USHORT  usObjID = LOUSHORT(hObject);
   ULONG   ulProfileSize;
   BYTE    szObjID[10],szHandles[100];
   HOBJECT hObj;
   PBYTE   pObjectData,pBuffer;
   WPPGMDATA wpData;


   memset(pszExe, 0, usMax);
   sprintf(szObjID, "%X", usObjID);
   pObjectData = GetProfileData(OBJECTS, szObjID, HINI_USERPROFILE, &ulProfileSize);
   if (!pObjectData)
     return FALSE;

   if (GetObjectData(pObjectData, "WPProgramRef", WPPROGRAM_DATA, &wpData, sizeof wpData)) {
     if (wpData.hExeHandle)
       hObj = wpData.hExeHandle;
     else {
       if (GetObjectDataString(pObjectData, "WPProgramRef", WPPROGRAM_STRINGS,
                               WPPGM_STR_EXENAME, pszExe, usMax)) {
         free(pObjectData);
         return TRUE;
       }
       free(pObjectData);
       return FALSE;
     }
   } else {
     if (GetObjectData(pObjectData, "WPProgramRef", WPPROGRAM_EXENAME, pszExe, usMax)) {
       free(pObjectData);
       return TRUE;
     }
     if (!GetObjectData(pObjectData, "WPProgramRef", WPPROGRAM_EXEHANDLE, &hObj, sizeof hObj)) {
       free(pObjectData);
       return FALSE;
     }
   }
   /* Special situation, name is cmd or command */
   if (hObj == 0xFFFF) {
     strcpy(pszExe, "*");
     free(pObjectData);
     return TRUE;
   }

   free(pObjectData);

   GetActiveHandles(HINI_SYSTEMPROFILE, szHandles, sizeof szHandles);
   if (!fReadAllBlocks(HINI_SYSTEMPROFILE, szHandles, &pBuffer, &ulProfileSize))
     return FALSE;

   memset(pszExe, 0, usMax);
   if (!GetPartName(pBuffer, ulProfileSize, hObj, pszExe, usMax, NULL)) {
     free(pBuffer);
     return FALSE;
   }

   free(pBuffer);
   return TRUE;
}


/****************************************************
*
*****************************************************/
USHORT GetObjectData(PBYTE pObjData, PSZ pszObjName, USHORT usTag, PVOID pOutBuf, USHORT usMax)
{  PBYTE pb, pBufEnd;
   ULONG ulObjSize;

   memset(pOutBuf, 0, usMax);

   ulObjSize = *(PULONG)pObjData;
   pb = pObjData + sizeof (ULONG);
   pb += strlen(pb) + 1;
   pb += 16;

   pBufEnd = pObjData + ulObjSize;
   while (pb < pBufEnd) {
     POINFO pOinfo;
     PBYTE  pbGroepBufEnd;

     pOinfo = (POINFO)pb;
     pb += sizeof(OINFO) + strlen(pOinfo->szName);
     pbGroepBufEnd = pb + pOinfo->cbData;
     while (pb < pbGroepBufEnd) {
       PTAG pTag   = (PTAG)pb;
       pb += sizeof (TAG);
       if (!stricmp(pOinfo->szName, pszObjName) && pTag->usTag == usTag) {
         if (usMax < pTag->cbTag)
           return 0;
         memcpy(pOutBuf, pb, pTag->cbTag);
         return pTag->usTagFormat;
       }
       pb += pTag->cbTag;
     }
   }
   return 0;
}


/*******************************************************************
* Get object progtype
*******************************************************************/
ULONG GetObjectProgType(HOBJECT hObject)
{  USHORT usObjID;
   PBYTE  pObjectData;
   ULONG  ulProfileSize;
   BYTE  szObjID[10];
   ULONG ulType;
   WPPGMDATA wpData;

   usObjID = LOUSHORT(hObject);
   sprintf(szObjID, "%X", usObjID);
   pObjectData = GetProfileData(OBJECTS, szObjID, HINI_USERPROFILE, &ulProfileSize);
   if (!pObjectData)
     return FALSE;

   if (GetObjectData(pObjectData, "WPProgramRef", WPPROGRAM_DATA, &wpData, sizeof wpData))
     ulType = wpData.ulProgType;
   else {
     if (!GetObjectData(pObjectData, "WPProgramRef", WPPROGRAM_PROGTYPE, &ulType, sizeof ulType))
       ulType = 0L;
   }
   free(pObjectData);
   return ulType;
}

/*******************************************************************
* Get the objects title
*******************************************************************/
BOOL GetObjectTitle(HOBJECT hObject, PSZ pszTitle, USHORT usMax)
{  USHORT usObjID;
   PBYTE  pObjectData;
   ULONG  ulProfileSize;
   BYTE  szObjID[10];

   usObjID = LOUSHORT(hObject);

   sprintf(szObjID, "%X", usObjID);
   pObjectData = GetProfileData(OBJECTS, szObjID, HINI_USERPROFILE, &ulProfileSize);
   if (!pObjectData)
     return FALSE;
   if (!GetObjectData(pObjectData, "WPAbstract", WPABSTRACT_TITLE, pszTitle, usMax)) {
     free(pObjectData);
     return FALSE;
   }
   free(pObjectData);
   return TRUE;
}


/*******************************************************************
* Get the abstract parameters
*******************************************************************/
BOOL GetObjectParameters(HOBJECT hObject, PSZ pszParms, USHORT usMax)
{  USHORT  usObjID = LOUSHORT(hObject);
   ULONG   ulProfileSize;
   BYTE    szObjID[10];
   PBYTE   pObjectData;

   sprintf(szObjID, "%X", usObjID);
   pObjectData = GetProfileData(OBJECTS, szObjID, HINI_USERPROFILE, &ulProfileSize);
   if (!pObjectData)
     return FALSE;

   if (!GetObjectDataString(pObjectData, "WPProgramRef", WPPROGRAM_STRINGS,
                            WPPGM_STR_ARGS, pszParms, usMax)) {
     if (!GetObjectData(pObjectData, "WPProgramRef", WPPROGRAM_PARAMS, pszParms, usMax)) {
       free(pObjectData);
       return FALSE;
     }
   }
   free(pObjectData);
   return TRUE;
}


/*******************************************************************
* Get the DOS Settings
*******************************************************************/
BOOL GetDosSettings(HOBJECT hObject, PSZ pszDosSettings, USHORT usMax)
{  USHORT  usObjID = LOUSHORT(hObject);
   ULONG   ulProfileSize;
   BYTE    szObjID[10];
   PBYTE   pObjectData;

   sprintf(szObjID, "%X", usObjID);
   pObjectData = GetProfileData(OBJECTS, szObjID, HINI_USERPROFILE, &ulProfileSize);
   if (!pObjectData)
     return FALSE;
   if (!GetObjectData(pObjectData, "WPProgramRef", WPPROGRAM_DOSSET, pszDosSettings, usMax)) {
     free(pObjectData);
     return FALSE;
   }
   free(pObjectData);
   return TRUE;
}

/*********************************************************************
* GetObjectSubString
*********************************************************************/
BOOL GetObjectDataString(PBYTE pObjData, PSZ pszObjName, USHORT usTag,
                         USHORT usStringTag, PVOID pOutBuf, USHORT usMax)
{  static BYTE pBuffer[300];
   PBYTE p;

   if (!GetObjectData(pObjData, pszObjName, usTag, pBuffer, sizeof pBuffer))
      return FALSE;

   p = pBuffer;
   while (*(PUSHORT)p != 0xFFFF) {
     if (*(PUSHORT)p == usStringTag) {
       strncpy(pOutBuf, p + 2, usMax);
       return TRUE;
     }
     p+=2;
     p+=strlen(p) + 1;
   }
   return FALSE;
}


USHORT ObjIDFromName(PSZ pszAbstractName, PSZ pszCmdLine, ULONG ulProgType, BOOL bSearchGroup)
{ ULONG k,ulProfileSize,ulBlockSize,ulObjID=0,ulObjID0=0;
  BYTE szObjID[10],*pProfileKeys,*pObjectData=NULL,*pBuffer;
  PCHAR pszHandles,pszExe,pszCmd;
  WPPGMDATA wpData;
  HOBJECT hObj=0;

  strlwr(pszAbstractName);

  pProfileKeys=GetProfileData(OBJECTS, NULL, HINI_USERPROFILE, &ulProfileSize);
  if (pProfileKeys==NULL)
    return 0;

  if ((pszHandles=malloc(100))==NULL) return 0;
  GetActiveHandles(HINI_SYSTEMPROFILE, pszHandles, 100);
  if (!fReadAllBlocks(HINI_SYSTEMPROFILE, pszHandles, &pBuffer, &ulBlockSize)) {
    free(pProfileKeys); free(pszHandles);
    return 0;
  }
  free(pszHandles);

  if ((pszExe=malloc(MAX_PATH+1))==NULL) return FALSE;
  if ((pszCmd=malloc(MAX_PATH+1))==NULL) {free(pszExe);return FALSE;}

  for (k=0; pProfileKeys[k]!='\0'; k+=strlen(szObjID)+1, ulObjID=0,hObj=0) {
    strncpy(szObjID,&pProfileKeys[k],sizeof szObjID-1);
    if (pObjectData!=NULL) free(pObjectData);
    memset(pszExe,0,MAX_PATH+1);

    if (!(pObjectData=GetProfileData(OBJECTS, szObjID, HINI_USERPROFILE, &ulProfileSize)))
      continue;

    if (GetObjectData(pObjectData, "WPProgramRef", WPPROGRAM_DATA, &wpData, sizeof wpData)) {
      if (wpData.hExeHandle)
        hObj = wpData.hExeHandle;
      else if (!GetObjectDataString(pObjectData, "WPProgramRef", WPPROGRAM_STRINGS, WPPGM_STR_EXENAME, pszExe, MAX_PATH))
        continue;
    } else
      if (!GetObjectData(pObjectData, "WPProgramRef", WPPROGRAM_EXENAME, pszExe, MAX_PATH) &&
          !GetObjectData(pObjectData, "WPProgramRef", WPPROGRAM_EXEHANDLE, &hObj, sizeof hObj))
        continue;

    if (hObj==0 ||
        (hObj != 0xFFFF     /* Special situation, name is cmd or command */
         && !GetPartName(pBuffer, ulBlockSize, hObj, pszExe, MAX_PATH, NULL)))
      continue;

    ulObjID=strtol(szObjID,NULL,16); GetObjectParameters(ulObjID, pszCmd, MAX_PATH);

    if ((hObj==0xFFFF && strstr(pszAbstractName,"cmd.exe")!=NULL && ulProgType==GetObjectProgType(ulObjID)) ||
        (hObj!=0xFFFF && stricmp(pszAbstractName,pszExe)==0)) {

      if ((bSearchGroup &&
           ((hObj==0xFFFF && strcmpi(&pszCmd[strspn(pszCmd," \t")],"%")==0) ||
            (hObj!=0xFFFF && strspn(pszCmd," \t")==strlen(pszCmd)))) || //empty parameters
          (!bSearchGroup &&
           strcmpi(&pszCmd[strspn(pszCmd," \t")],&pszCmdLine[strspn(pszCmdLine," \t")])==0)) //matching parameters
        break;

      if (hObj!=0xFFFF && strcmpi(&pszCmd[strspn(pszCmd," \t")],&pszCmdLine[strspn(pszCmdLine," \t")])==0)
        ulObjID0=ulObjID;
    }
  }
  free(pObjectData); free(pProfileKeys); free(pBuffer);
  free(pszExe); free(pszCmd);
  return LOUSHORT(ulObjID!=0?ulObjID:(ulObjID0!=0?ulObjID0:0));
}


/**********************************************************************
* Get pointer bitmaps based on a icon size
**********************************************************************/
BOOL GetPointerBitmaps(HWND hwnd,
                       PBYTE pchIcon,
                       PBITMAPARRAYFILEHEADER2 pbafh2,
                       HBITMAP *phbmPointer,
                       HBITMAP *phbmColor,
                       USHORT usIconSize)
{
HPS   hps;
USHORT usBitCount, usRGB;
PBITMAPFILEHEADER2 pbfh2;
PBITMAPINFOHEADER2 pbmp2;
USHORT usExtra, usExp;
PBYTE  p;

   *phbmPointer = (HBITMAP)0;
   *phbmColor   = (HBITMAP)0;

   /*
      Is it the correct icon type ?
   */
   switch (pbafh2->usType)
      {
      case BFT_BITMAPARRAY    :
         pbfh2 = &pbafh2->bfh2;
         break;

      case BFT_ICON           :
      case BFT_BMAP           :
      case BFT_POINTER        :
      case BFT_COLORICON      :
      case BFT_COLORPOINTER   :
         pbfh2 = (PBITMAPFILEHEADER2)pbafh2;
         break;
      default :
         return FALSE;
      }
   pbmp2 = &pbfh2->bmp2;

      /*
         Is it a BITMAPINFOHEADER or BITMAPINFOHEADER2 ?
      */
   if (pbmp2->cbFix == sizeof (BITMAPINFOHEADER2))
      {
      usRGB = sizeof (RGB2);
      usBitCount = pbmp2->cBitCount;
      if (usIconSize && pbmp2->cx != usIconSize)
         return FALSE;
      }
   else if (pbmp2->cbFix == sizeof (BITMAPINFOHEADER))
      {
      PBITMAPINFOHEADER pbmp = (PBITMAPINFOHEADER)pbmp2;
      usRGB = sizeof (RGB);
      usBitCount = pbmp->cBitCount;
      if (usIconSize && pbmp->cx != usIconSize)
         return FALSE;
      }
   else  /* Unknown length found */
      return FALSE;

   /*
      Create the first pointer by getting the presentation space first
      and than call GpiCreateBitmap
   */

   hps = WinGetPS(hwnd);
   *phbmPointer = GpiCreateBitmap(hps,
      pbmp2,
      CBM_INIT,
      (PBYTE)pchIcon + pbfh2->offBits,
      (PBITMAPINFO2)pbmp2);

   if (*phbmPointer == GPI_ERROR)
      {
      WinReleasePS(hps);
      return FALSE;
      }
   WinReleasePS(hps);

   /*
      If it is a color icon than another BITMAPFILEHEADER follow after
      the color information. This color information contains of a number
      of RGB or RGB2 structures. The number depends of the number of colors
      in the bitmap. The number of colors is calculated by looking at
      the Number of bits per pel and using this number as an exponent on 2.
   */

   if (pbfh2->usType != BFT_COLORICON &&
       pbfh2->usType != BFT_COLORPOINTER)
      return TRUE;


   /*
      Calculate beginning of BITMAPFILEHEADER structure
      2^Bits_per_pel
   */
   for (usExtra = 1, usExp = 0; usExp < usBitCount; usExp++)
      usExtra *= 2;

   p = (PBYTE)(pbfh2) +
      (pbfh2->cbSize + usExtra * usRGB);
   pbfh2 = (PBITMAPFILEHEADER2)p;
   /*
      Get adress of BITMAPINFOHEADER
   */
   pbmp2 = &pbfh2->bmp2;

   if (pbmp2->cbFix == sizeof (BITMAPINFOHEADER2))
      {
      if (pbmp2->cBitCount == 1)
         return TRUE;
      }
   else if (pbmp2->cbFix == sizeof (BITMAPINFOHEADER))
      {
      PBITMAPINFOHEADER pbmp = (PBITMAPINFOHEADER)pbmp2;
      if (pbmp->cBitCount == 1)
         return TRUE;
      }
   else  /* Unknown length found */
      return TRUE;

   /*
      And create bitmap number 2
   */

   hps = WinGetPS(hwnd);
   *phbmColor = GpiCreateBitmap(hps,
      pbmp2,
      CBM_INIT,
      (PBYTE)pchIcon + pbfh2->offBits,
      (PBITMAPINFO2)pbmp2);
   if (*phbmColor == GPI_ERROR)
      {
      GpiDeleteBitmap(*phbmPointer);
      return FALSE;
      }
   WinReleasePS(hps);
   return TRUE;
}


/*******************************************************************
* Buffer to Icon
*******************************************************************/
HPOINTER Buffer2Icon(HWND hwnd, PBYTE pchIcon, USHORT usIconSize)
{
static USHORT usDeviceCX = 0;
static USHORT usDeviceCY = 0;
BOOL fContinue,
     fIconFound;
POINTERINFO PointerInfo;
PBITMAPARRAYFILEHEADER2 pbafh2;
PBYTE p;
HPOINTER hptrIcon = (HPOINTER)0;

   memset(&PointerInfo, 0, sizeof PointerInfo);
           
   if (!usDeviceCX)
      {
      usDeviceCX = WinQuerySysValue(HWND_DESKTOP, SV_CXSCREEN);
      usDeviceCY = WinQuerySysValue(HWND_DESKTOP, SV_CYSCREEN);
      }

   fIconFound = FALSE;
   pbafh2 = (PBITMAPARRAYFILEHEADER2)pchIcon;

   switch (pbafh2->usType)
      {
      case BFT_BITMAPARRAY    :
         break;
      case BFT_ICON           :
      case BFT_BMAP           :
      case BFT_POINTER        :
      case BFT_COLORICON      :
      case BFT_COLORPOINTER   :
         if (GetPointerBitmaps(hwnd,
            pchIcon,
            pbafh2,
            &PointerInfo.hbmPointer,
            &PointerInfo.hbmColor,
            0))
            {
            fIconFound = TRUE;
            }
         else
            return (HPOINTER)0;
         break;
      default :
         return (HPOINTER)0;
      }

   /*
      First see if the icon contains an icon for the current device
      size.
   */
   fContinue = TRUE;
   while (!fIconFound && fContinue)
      {
      if (pbafh2->cxDisplay == usDeviceCX &&
          pbafh2->cyDisplay == usDeviceCY)
         {
         if (GetPointerBitmaps(hwnd,
            pchIcon,
            pbafh2,
            &PointerInfo.hbmPointer,
            &PointerInfo.hbmColor,
            usIconSize))
            {
            fIconFound = TRUE;
            break;
            }
         }
      p = (PBYTE)pchIcon + pbafh2->offNext;
      if (!pbafh2->offNext)
            break;
      pbafh2 = (PBITMAPARRAYFILEHEADER2)p;
      }

   /*
      Now look for the independed icons
   */
   if (!fIconFound)
      {
      pbafh2 = (PBITMAPARRAYFILEHEADER2)pchIcon;
      fContinue = TRUE;
      while (fContinue)
         {
         if (pbafh2->cxDisplay == 0 &&
            pbafh2->cyDisplay == 0)
            {
            if (GetPointerBitmaps(hwnd,
               pchIcon,
               pbafh2,
               &PointerInfo.hbmPointer,
               &PointerInfo.hbmColor,
               usIconSize))
               {
               fIconFound = TRUE;
               break;
               }
            }
         p = (PBYTE)pchIcon + pbafh2->offNext;
         if (!pbafh2->offNext)
            break;
         pbafh2 = (PBITMAPARRAYFILEHEADER2)p;
         }
      }

   /*
      if we still haven't found an icon we take the first icon there is
   */
   if (!fIconFound)
      {
      pbafh2 = (PBITMAPARRAYFILEHEADER2)pchIcon;
      if (GetPointerBitmaps(hwnd,
         pchIcon,
         pbafh2,
         &PointerInfo.hbmPointer,
         &PointerInfo.hbmColor,
         0))
         fIconFound = TRUE;
      }

   if (!fIconFound)
      return (HPOINTER)0L;

   PointerInfo.fPointer  = FALSE;
   PointerInfo.xHotspot   = 0;
   PointerInfo.yHotspot   = 0;
   hptrIcon = WinCreatePointerIndirect(HWND_DESKTOP, &PointerInfo);

   if (PointerInfo.hbmPointer)
      GpiDeleteBitmap(PointerInfo.hbmPointer);
   if (PointerInfo.hbmColor)
      GpiDeleteBitmap(PointerInfo.hbmColor);

   return hptrIcon;

}



/***********************************************************************
*  Get Object icon
***********************************************************************/
HPOINTER GetObjectIcon(HOBJECT hObject, PBYTE pObjectData)
{ PBYTE pBuffer;
  USHORT usObjID;
  HPOINTER hptr = NULL;
  ULONG ulProfileSize;
  BYTE  szPath[CCHMAXPATH],szFileName[CCHMAXPATH];

  usObjID = LOUSHORT(hObject);
  if (IsObjectAbstract(hObject)) {
    BYTE  szObjID[10];
    BOOL  fObjectDataRead = FALSE;

    sprintf(szObjID, "%X", usObjID);

    pBuffer = GetProfileData(ICONS, szObjID, HINI_USERPROFILE, &ulProfileSize);
    if (pBuffer) {
      hptr = Buffer2Icon(HWND_DESKTOP, pBuffer, WinQuerySysValue(HWND_DESKTOP, SV_CXICON));
      free(pBuffer);
      return hptr;
    }

    if (!pObjectData) {
      pObjectData = GetProfileData(OBJECTS, szObjID, HINI_USERPROFILE, &ulProfileSize);
      if (!pObjectData)
        return NULL;
      fObjectDataRead = TRUE;
    }
    if (!memicmp(pObjectData  + 6, "Shadow", 5)) {
      HOBJECT hObjShadow;
      if (GetObjectData(pObjectData, "WPShadow", WPSHADOW_LINK, &hObjShadow, sizeof hObjShadow))
        hptr = GetObjectIcon(hObjShadow, NULL);
    }
    if (!memicmp(pObjectData + 4, "WPProgram", 9)) {
      HOBJECT hObjProgram;
      WPPGMDATA wpData;

      if (GetObjectData(pObjectData, "WPProgramRef", WPPROGRAM_DATA, &wpData, sizeof wpData)) {
        if (wpData.hExeHandle) {
          hObjProgram = wpData.hExeHandle  + 0x30000;
          hptr = GetObjectIcon(hObjProgram, NULL);
        } else {
          if (GetObjectDataString(pObjectData, "WPProgramRef", WPPROGRAM_STRINGS,
                                  WPPGM_STR_EXENAME, szFileName, sizeof szFileName)) {
            _searchenv(szFileName, "PATH", szPath);
            hptr = WinLoadFileIcon(szPath, FALSE);
          }
        }
      } else if (GetObjectData(pObjectData, "WPProgramRef", WPPROGRAM_EXEHANDLE, &hObjProgram, sizeof hObjProgram))
        hptr = GetObjectIcon(hObjProgram, NULL);
      else if (GetObjectData(pObjectData, "WPProgramRef", WPPROGRAM_EXENAME, szFileName, sizeof szFileName))
        hptr = WinLoadFileIcon(szFileName, FALSE);
    }

    if (fObjectDataRead)
      free(pObjectData);
    return hptr;
  }

  if (!GetObjectName(hObject, szFileName, sizeof szFileName))
    return NULL;

  if (szFileName[0] == '*')
    return NULL;

  return WinLoadFileIcon(szFileName, FALSE);
}


