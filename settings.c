/*
 *      Copyright (C) 1997-2004 Andrei Los.
 *      This file is part of the lSwitcher source package.
 *      lSwitcher is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published
 *      by the Free Software Foundation, in version 2 as it comes in the
 *      "COPYING" file of the lSwitcher main distribution.
 *      This program is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 */

#include <string.h>
#include <stdio.h>
#include "prmdlg.h"
#include "settings.h"
#include "lswitch.h"
#include "common.h"
#include "taskbar.h"
#include "lswres.h"

#define BKM_SETNOTEBOOKBUTTONS   0x0375  /* Set common pushbuttons    */
#define STRLISTDIVIDER "}{!@$#%^&*]["


VOID GetIniFileName(UCHAR *ucFName,USHORT usLen)
{ PPIB ppib;
  UCHAR *p;
  SHORT k;

#ifndef XWORKPLACE
  DosGetInfoBlocks(NULL,&ppib);

  if ((p=strstr(&ppib->pib_pchcmd[strlen(ppib->pib_pchcmd)+1],"/i"))!=NULL ||
      (p=strstr(&ppib->pib_pchcmd[strlen(ppib->pib_pchcmd)+1],"/I"))!=NULL) {
    GetStartupDir(ucFName,usLen);
    for (k = 2; k < strlen(p) && p[k]==' '; k++); // get rid of leading spaces
    strncat(ucFName,&p[k],usLen-strlen(ucFName)-1);
  } else
    strncpy(ucFName,"",usLen);
#else
  GetStartupDir(ucFName,usLen);
  strncat(ucFName,DEFINIFNAME,usLen-strlen(ucFName)-1);
#endif
}


BOOL CheckSettings(LSWSETTINGS *pSettings)
{
  return (pSettings->ucVerMajor >= LASTINIVERMAJOROK &&
          pSettings->ucVerMinor >= LASTINIVERMINOROK &&
          pSettings->ucRevision >= LASTINIREVISIONOK);
}


VOID InitSettings(LSWSETTINGS *pSettings)
{
  memset(pSettings, 0, sizeof(LSWSETTINGS));
  pSettings->ucVerMajor = VERSIONMAJOR;
  pSettings->ucVerMinor = VERSIONMINOR;
  pSettings->ucRevision = REVISION;
  pSettings->bShowHidden = pSettings->bShowHiddenTskBar = TRUE;
  pSettings->bShowViewer = pSettings->bShowViewerTskBar = TRUE;
  pSettings->bPMSwitcher = TRUE;
  pSettings->bShowInWinList = TRUE;
  pSettings->bShowHints = TRUE;
  pSettings->lBackColor = DEFBACKRGBCOLOR;
  pSettings->lRaisedFrameColor = DEFRAISEDFRAMECOLOR;
  pSettings->lSunkenFrameColor = DEFSUNKENFRAMECOLOR;
  pSettings->lTitleRGBCol = DEFTITLERGBCOLOR;
  pSettings->lHintsRGBCol = DEFHINTSRGBCOLOR;
  pSettings->lBubbleRGBCol = DEFBUBBLERGBCOLOR;
  pSettings->lBubbleTextRGBCol = RGB_BLUE/2; //dark blue
  pSettings->bTaskBarOn = TRUE;
  pSettings->bTaskBarAlwaysVisible = TRUE;
  pSettings->bReduceDsk = TRUE;
  pSettings->bFlatButtons = TRUE;
  pSettings->bAllowResize = TRUE;
  pSettings->b3DTaskBar = TRUE;

  pSettings->lTskBarRGBCol3D = DEFTSKBARRGBCOL3D;
  pSettings->lNormalBtnRGBCol3D = pSettings->lNormalHiddenBtnRGBCol3D = NORMALBTNRGBCOL3D;
  pSettings->lActiveBtnRGBCol3D = pSettings->lActiveHiddenBtnRGBCol3D = ACTIVEBTNRGBCOL3D;
  pSettings->lNormalBtnTitleRGBCol3D = NORMALBTNTITLERGBCOL3D;
  pSettings->lActiveBtnTitleRGBCol3D = ACTIVEBTNTITLERGBCOL3D;
  pSettings->lNormalHiddenBtnTitleRGBCol3D = pSettings->lActiveHiddenBtnTitleRGBCol3D = HIDDENBTNTITLERGBCOL3D;

  pSettings->lTskBarRGBCol = DEFTSKBARRGBCOL;
  pSettings->lNormalBtnRGBCol = pSettings->lNormalHiddenBtnRGBCol = NORMALBTNRGBCOL;
  pSettings->lActiveBtnRGBCol = pSettings->lActiveHiddenBtnRGBCol = ACTIVEBTNRGBCOL;
  pSettings->lNormalBtnTitleRGBCol = NORMALBTNTITLERGBCOL;
  pSettings->lActiveBtnTitleRGBCol = ACTIVEBTNTITLERGBCOL;
  pSettings->lNormalHiddenBtnTitleRGBCol = pSettings->lActiveHiddenBtnTitleRGBCol = HIDDENBTNTITLERGBCOL;

  pSettings->sTskBarX = pSettings->sTskBarY = -TSKBARFRAMEWIDTH;
  pSettings->sTskBarCX = WinQuerySysValue(HWND_DESKTOP,SV_CXSCREEN)+2*TSKBARFRAMEWIDTH;
  pSettings->sTskBarCY = WinQuerySysValue(HWND_DESKTOP,SV_CYICON)/2+4+2*BUTTONBORDERWIDTH+2+2*TSKBARFRAMEWIDTH;
  pSettings->usMaxBtnWid = MAXBUTTONWIDTH;

  pSettings->ucSettingsKey = 'S';   pSettings->ucSettingsScan = SCAN_SETTINGS;
  pSettings->usSwitchMEvent = WM_BUTTON1CLICK;
  pSettings->usMinMEvent = WM_BUTTON1CLICK;
  pSettings->usCloseMEvent = WM_CHORD;
  pSettings->usScrollDiv = 1;

  pSettings->usLanguage = EN;

  strncpy(pSettings->ucTitleFont,DEFTITLEFONT3,sizeof(pSettings->ucTitleFont));
  strncpy(pSettings->ucHintsFont,DEFHINTSFONT,sizeof(pSettings->ucHintsFont));
  strncpy(pSettings->ucButtonFont,DEFTITLEFONT3,sizeof(pSettings->ucButtonFont));
}

USHORT LoadSettings(HAB hab,PSZ pszFName,LSWSETTINGS *pSettings)
{ ULONG ulCbBuf,k,ulIndex;
  HINI hini;
  BOOL rc;
  UCHAR *pBuf;
  ENTRYNAME SkipStr;
  SKIPLIST *pSkipList;

  if (strlen(pszFName) != 0) {
    if ((hini=PrfOpenProfile(hab,pszFName))==NULLHANDLE) {
      return 1;
    }
  } else
    hini=HINI_USERPROFILE;

  ulCbBuf = sizeof(LSWSETTINGS)+3*MAXITEMS*sizeof(ENTRYNAME)+2*sizeof(STRLISTDIVIDER);
  if ((pBuf = calloc(1,ulCbBuf))==NULL) return 3;

  rc = PrfQueryProfileData(hini, PRF_APPNAME, PRF_SETTINGS, pBuf, &ulCbBuf);
  if (pszFName != NULL) PrfCloseProfile(hini);
  if (!rc) return 2;

  ulIndex = sizeof(LSWSETTINGS)-3*sizeof(pSettings->SkipListPopup);

  memcpy(pSettings,pBuf,ulIndex);

  k=0; pSkipList = &pSettings->SkipListPopup;

  while (ulIndex < ulCbBuf) {
    if (sscanf(&pBuf[ulIndex],"%[^\0]",SkipStr)!=1 || strlen(SkipStr)==0) break;

    ulIndex += strlen(SkipStr)+1;

    if (strcmp(SkipStr,STRLISTDIVIDER)==0) {
      k = 0;
      if (pSkipList == &pSettings->SkipListPopup)
        pSkipList = &pSettings->SkipListTskBar;
      else
        pSkipList = &pSettings->ListIconsOnly;
      continue;
    }
    (*pSkipList)[k++] = strdup(SkipStr);
  }

  free(pBuf);

  return 0;
}


USHORT SaveSettings(HAB hab,PSZ pszFName,LSWSETTINGS *pSettings)
{ USHORT k;
  HINI hini;
  BOOL rc;
  UCHAR *pBuf;
  ULONG ulCbBuf,ulIndex;

  if (strlen(pszFName) != 0) {
    if ((hini=PrfOpenProfile(hab,pszFName))==NULLHANDLE)
      return 1;
  } else
    hini=HINI_USERPROFILE;

  ulCbBuf = sizeof(LSWSETTINGS)+3*MAXITEMS*sizeof(ENTRYNAME)+2*sizeof(STRLISTDIVIDER);
  if ((pBuf = calloc(1,ulCbBuf))==NULL) return 3;

  ulIndex = sizeof(LSWSETTINGS)-3*sizeof(pSettings->SkipListPopup);
  memcpy(pBuf,pSettings,ulIndex);

  for (k=0; pSettings->SkipListPopup[k]!=NULL && k<MAXITEMS; k++) {
    strcat(&pBuf[ulIndex],pSettings->SkipListPopup[k]);
    ulIndex += strlen(pSettings->SkipListPopup[k])+1;
  }

  if (pSettings->SkipListTskBar[0]!=NULL) {
    strcat(&pBuf[ulIndex],STRLISTDIVIDER);
    ulIndex += strlen(STRLISTDIVIDER)+1;
    for (k=0; pSettings->SkipListTskBar[k]!=NULL && k < MAXITEMS; k++) {
      strcat(&pBuf[ulIndex],pSettings->SkipListTskBar[k]);
      ulIndex += strlen(pSettings->SkipListTskBar[k])+1;
    }
  }

  if (pSettings->ListIconsOnly[0]!=NULL) {
    strcat(&pBuf[ulIndex],STRLISTDIVIDER);
    ulIndex += strlen(STRLISTDIVIDER)+1;
    for (k=0; pSettings->ListIconsOnly[k]!=NULL && k < MAXITEMS; k++) {
      strcat(&pBuf[ulIndex],pSettings->ListIconsOnly[k]);
      ulIndex += strlen(pSettings->ListIconsOnly[k])+1;
    }
  }

  rc = PrfWriteProfileData(hini, PRF_APPNAME, PRF_SETTINGS, pBuf, ulIndex);

  if (pszFName != NULL)
    PrfCloseProfile(hini);

  free(pBuf);

  if (rc)
    return 0;
  else
    return 2;
}


USHORT DeleteSettings(HAB hab,PSZ pszFName)
{ HINI hini;

  if (strlen(pszFName) != 0) {
    if ((hini=PrfOpenProfile(hab,pszFName))==NULLHANDLE)
      return 1;
  } else
    hini = HINI_USERPROFILE;

  PrfWriteProfileData(hini, PRF_APPNAME, NULL, NULL, 0);

  if (pszFName != NULL)
    PrfCloseProfile(hini);

  return 0;
}


VOID EditSettings(LSWDATA *plswData)
{
  if (!WinIsWindow(plswData->hab,plswData->hwndParamDlg)) {
    plswData->hwndParamDlg = WinLoadDlg(HWND_DESKTOP, NULLHANDLE, ParmDlgProc,
                                        plswData->hmodRes, DLG_PARAMS, plswData);
    if (plswData->hwndParamDlg != NULLHANDLE) {
      WinSetWindowPos(plswData->hwndParamDlg,HWND_TOP,0,0,0,0,SWP_SHOW | SWP_ZORDER | SWP_ACTIVATE);
      WinProcessDlg(plswData->hwndParamDlg);
      WinDestroyWindow(plswData->hwndParamDlg);
    }
  } else
    WinSetWindowPos(plswData->hwndParamDlg,HWND_TOP,0,0,0,0,SWP_SHOW | SWP_ZORDER | SWP_ACTIVATE);
}


MRESULT EXPENTRY PopupDlgProc(HWND hwnd,ULONG msg,MPARAM mp1,MPARAM mp2)
{ static BOOL bInit = FALSE;
  LSWDATA *plswData;

  plswData = WinQueryWindowPtr(hwnd,0);

  switch (msg) {
  case WM_INITDLG: {
    bInit = TRUE;
    SetControlsFont(hwnd,FALSE);
    DosGetNamedSharedMem((PVOID*)&plswData,SHAREMEM_NAME,PAG_READ | PAG_WRITE);
    WinSetWindowPtr(hwnd,0,plswData);
    DosFreeMem(plswData);

    WinCheckButton(hwnd, plswData->Settings.ucPopupHotKey==0?RAD_ALTTAB:
                   plswData->Settings.ucPopupHotKey==1?RAD_CTRLTAB:RAD_WINTAB, TRUE);
    WinCheckButton(hwnd, CHK_PMSWITCHER, plswData->Settings.bPMSwitcher);
    WinCheckButton(hwnd, CHK_PMPOPUPINFS, plswData->Settings.bPMPopupInFS);
    WinCheckButton(hwnd, CHK_SCROLL, plswData->Settings.bScrollItems);
    WinCheckButton(hwnd, CHK_HINTS, plswData->Settings.bShowHints);
    WinCheckButton(hwnd, CHK_STICKY, plswData->Settings.bStickyPopup);

    WinEnableControl(hwnd, CHK_PMSWITCHER,(plswData->Settings.ucPopupHotKey==0));
    WinEnableControl(hwnd, CHK_SCROLL,!plswData->Settings.bOldPopup);

    WinSendDlgItemMsg(hwnd,SPIN_SCROLLDIV,SPBM_SETLIMITS,MPFROMLONG(9),MPFROMLONG(1));
    WinSendDlgItemMsg(hwnd,SPIN_SCROLLDIV,SPBM_SETTEXTLIMIT,MPFROMSHORT(1),0);
    WinSendDlgItemMsg(hwnd,SPIN_SCROLLDIV,SPBM_SETCURRENTVALUE,MPFROMLONG(plswData->Settings.usScrollDiv),0);

    bInit = FALSE;
    return (MRESULT)TRUE;
  }

  case WM_CONTROL: {
    BOOL bValue;

    if (bInit) break;
    bValue = WinQueryButtonCheckstate(hwnd, SHORT1FROMMP(mp1));

    switch (SHORT1FROMMP(mp1)) {
      case SPIN_SCROLLDIV:
        if (SHORT2FROMMP(mp1)==SPBN_CHANGE) {
          LONG ulScrollDiv;

          WinSendDlgItemMsg(hwnd,SPIN_SCROLLDIV,SPBM_QUERYVALUE,MPFROMP(&ulScrollDiv),MPFROM2SHORT(0,SPBQ_ALWAYSUPDATE));
          plswData->Settings.usScrollDiv=(USHORT)ulScrollDiv;
        }
        break;

      case RAD_ALTTAB:
      case RAD_CTRLTAB:
      case RAD_WINTAB: {
        ULONG ulPostCount;

        if      (WinQueryButtonCheckstate(hwnd, RAD_ALTTAB)) plswData->Settings.ucPopupHotKey=0;
        else if (WinQueryButtonCheckstate(hwnd, RAD_CTRLTAB)) plswData->Settings.ucPopupHotKey=1;
        else    plswData->Settings.ucPopupHotKey=2;
        WinEnableControl(hwnd,CHK_PMSWITCHER,(plswData->Settings.ucPopupHotKey==0));

        if (plswData->Settings.ucPopupHotKey==0) {
          DosResetEventSem(plswData->hevCtrlTab,&ulPostCount);
          DosResetEventSem(plswData->hevCtrlTab,&ulPostCount);
        } else if (plswData->Settings.ucPopupHotKey==1)
          DosPostEventSem(plswData->hevCtrlTab);
        else
          DosResetEventSem(plswData->hevCtrlTab,&ulPostCount);

        break;
      }
      case CHK_PMSWITCHER: plswData->Settings.bPMSwitcher = bValue; break;
      case CHK_SCROLL: plswData->Settings.bScrollItems = bValue; break;
      case CHK_HINTS: plswData->Settings.bShowHints = bValue; break;
      case CHK_SWDESKTOP: plswData->Settings.bDesktopMinimizes = bValue; break;
      case CHK_PMPOPUPINFS: plswData->Settings.bPMPopupInFS = bValue; break;
      case CHK_STICKY: plswData->Settings.bStickyPopup = bValue; break;
    }

    break;
  }
  case WM_COMMAND:
    switch (SHORT1FROMMP(mp1)) {
    case DID_OK:
    case DID_CANCEL:
      WinPostMsg(WinQueryWindow(hwnd,QW_OWNER),msg,mp1,mp2);
      break;
    default:
      return WinDefDlgProc(hwnd, msg, mp1, mp2);
    }
    break;

  default:
    return WinDefDlgProc(hwnd, msg, mp1, mp2);
  }
  return 0;
}

MRESULT EXPENTRY ListDlgProc(HWND hwnd,ULONG msg,MPARAM mp1,MPARAM mp2)
{ LSWDATA *plswData;
  SKIPLIST *pSkipList;

  if (msg==WM_INITDLG)
    pSkipList = (SKIPLIST *)PVOIDFROMMP(mp2);
  else {
    plswData = WinQueryWindowPtr(hwnd,0);
    switch (WinQueryWindowUShort(hwnd,QWS_ID)) {
    case DLG_EXCLUDETSKBAR:pSkipList=&plswData->Settings.SkipListTskBar; break;
    case DLG_EXCLUDEPOPUP:pSkipList=&plswData->Settings.SkipListPopup; break;
    case DLG_ICONSONLY:pSkipList=&plswData->Settings.ListIconsOnly; break;
    }
  }

  switch (msg) {
  case WM_INITDLG: {
    PSWBLOCK pswblk;
    ULONG ulItems,k;

    SetControlsFont(hwnd,FALSE);
    DosGetNamedSharedMem((PVOID*)&plswData,SHAREMEM_NAME,PAG_READ | PAG_WRITE);
    WinSetWindowPtr(hwnd,0,plswData);
    DosFreeMem(plswData);
    WinPostMsg(hwnd,LSWM_UPDATEEXCLUDEDLG,MPFROMSHORT(UPDATEADDEDLIST|UPDATEREMOVEDLIST),0);
    return (MRESULT)TRUE;
  }

  case LSWM_UPDATEEXCLUDEDLG: {
    PSWBLOCK pswblk;
    ULONG ulItems,k;

    if (SHORT1FROMMP(mp1)&UPDATEREMOVEDLIST) {
      WinSendDlgItemMsg(hwnd,LB_REMOVED,LM_DELETEALL,0,0);
      for (k = 0; (*pSkipList)[k] != NULL; k++)
        WinSendDlgItemMsg(hwnd,LB_REMOVED,LM_INSERTITEM,MPFROMSHORT(LIT_END), MPFROMP((*pSkipList)[k]));
    }

    if (SHORT1FROMMP(mp1)&UPDATEADDEDLIST) {
      WinSendDlgItemMsg(hwnd,LB_ADDED,LM_DELETEALL,0,0);
      if ((pswblk=GetSwitchList(WinQueryAnchorBlock(hwnd),TRUE,&ulItems))!=NULL) {
        for (k = 0; k < ulItems; k++)
          if (pswblk->aswentry[k].swctl.uchVisibility==SWL_VISIBLE &&
              !IsInSkipList(pSkipList,pswblk->aswentry[k].swctl.szSwtitle))
            WinSendDlgItemMsg(hwnd,LB_ADDED,LM_INSERTITEM,MPFROMSHORT(LIT_END),
                              MPFROMP(pswblk->aswentry[k].swctl.szSwtitle));
        GetSwitchList(0,FALSE,NULL);
      }
    }
    break;
  }

  case WM_COMMAND: {
    switch (SHORT1FROMMP(mp1)) {
    case PB_REMOVE: {
      UCHAR ucName[NAMELEN];
      SHORT iIndex,k;
      PSWBLOCK pswblk;
      ULONG ulItems;

      if ((pswblk=GetSwitchList(WinQueryAnchorBlock(hwnd),TRUE,&ulItems))==NULL) break;

      while ((iIndex = (SHORT)WinSendDlgItemMsg(hwnd,LB_REMOVED,LM_QUERYSELECTION,MPFROMSHORT(LIT_FIRST),0)) != LIT_NONE) {
        WinSendDlgItemMsg(hwnd,LB_REMOVED,LM_QUERYITEMTEXT,MPFROM2SHORT(iIndex,NAMELEN),MPFROMP(ucName));
        WinSendDlgItemMsg(hwnd,LB_REMOVED,LM_DELETEITEM,MPFROMSHORT(iIndex),0);

        for (k = 0; k < ulItems; k++)
          if (strcmpi(ucName,pswblk->aswentry[k].swctl.szSwtitle)==0 &&
              pswblk->aswentry[k].swctl.uchVisibility==SWL_VISIBLE) {

            WinSendDlgItemMsg(hwnd,LB_ADDED,LM_INSERTITEM,MPFROMSHORT(LIT_END),MPFROMP(pswblk->aswentry[k].swctl.szSwtitle));
            break;
          }
        RemoveFilter(pSkipList,ucName);
      }
      GetSwitchList(0,FALSE,NULL);

      if ((WinQueryWindowUShort(hwnd,QWS_ID)==DLG_EXCLUDETSKBAR ||
           WinQueryWindowUShort(hwnd,QWS_ID)==DLG_ICONSONLY) &&
          plswData->Settings.bTaskBarOn) {

        WinPostMsg(plswData->hwndTaskBarClient,LSWM_INITBUTTONS,0,0);
      }

      break;
    }

    case PB_ADD: {
      UCHAR ucName[NAMELEN];
      SHORT iIndex;

      WinQueryDlgItemText(hwnd,LB_ADDED,sizeof(ucName),MPFROMP(ucName));
      WinSetDlgItemText(hwnd,LB_ADDED,NULL);

      if (!AddFilter(pSkipList,ucName)) break;

      iIndex = (SHORT)WinSendDlgItemMsg(hwnd,LB_ADDED,LM_SEARCHSTRING,MPFROM2SHORT(0,LIT_FIRST),MPFROMP(ucName));
      if (iIndex!=LIT_NONE && iIndex!=LIT_ERROR)
        WinSendDlgItemMsg(hwnd,LB_ADDED,LM_DELETEITEM,MPFROMSHORT(iIndex),0);
      WinSendDlgItemMsg(hwnd,LB_REMOVED,LM_INSERTITEM,MPFROMSHORT(LIT_END),MPFROMP(ucName));

      if ((WinQueryWindowUShort(hwnd,QWS_ID)==DLG_EXCLUDETSKBAR ||
           WinQueryWindowUShort(hwnd,QWS_ID)==DLG_ICONSONLY) &&
          plswData->Settings.bTaskBarOn)
        WinPostMsg(plswData->hwndTaskBarClient,LSWM_INITBUTTONS,0,0);

      break;
    }

    case DID_OK:
    case DID_CANCEL:
      WinPostMsg(WinQueryWindow(hwnd,QW_OWNER),msg,mp1,mp2);
      break;
    default:
      return WinDefDlgProc(hwnd, msg, mp1, mp2);

    }
    break;
  }

  case WM_CONTROL:
    if ((SHORT1FROMMP(mp1)==LB_ADDED && SHORT2FROMMP(mp1)==CBN_ENTER) ||
        (SHORT1FROMMP(mp1)==LB_REMOVED && SHORT2FROMMP(mp1)==LN_ENTER))
      WinPostMsg(hwnd, WM_COMMAND, SHORT1FROMMP(mp1)==LB_ADDED?(MPARAM)PB_ADD:(MPARAM)PB_REMOVE,
                 MPFROM2SHORT(CMDSRC_OTHER, FALSE));
    break;

  default:
    return WinDefDlgProc(hwnd, msg, mp1, mp2);
  }

  return (MRESULT)0;
}


MRESULT EXPENTRY TaskBarDlgProc(HWND hwnd,ULONG msg,MPARAM mp1,MPARAM mp2)
{ static BOOL bInit = FALSE;
  LSWDATA *plswData;

  plswData = WinQueryWindowPtr(hwnd,0);

  switch (msg) {
  case WM_INITDLG: {
    ULONG ulMenuEvent;
    USHORT usEvent;

    bInit = TRUE;
    SetControlsFont(hwnd,FALSE);

    DosGetNamedSharedMem((PVOID*)&plswData,SHAREMEM_NAME,PAG_READ | PAG_WRITE);
    WinSetWindowPtr(hwnd,0,plswData);
    DosFreeMem(plswData);

#ifndef XWORKPLACE
    WinCheckButton(hwnd, CHK_TASKBARON, plswData->Settings.bTaskBarOn);
    WinCheckButton(hwnd, CHK_TASKBARALWAYSVISIBLE, !plswData->Settings.bTaskBarAlwaysVisible);
    WinCheckButton(hwnd, plswData->Settings.bTaskBarTopScr?RAD_TOP:RAD_BOTTOM, TRUE);
    WinCheckButton(hwnd, CHK_REDUCEDSK, plswData->Settings.bReduceDsk);
    WinEnableControl(hwnd, CHK_REDUCEDSK, plswData->Settings.bTaskBarAlwaysVisible);
    WinCheckButton(hwnd, CHK_FLATBTN, plswData->Settings.bFlatButtons);
    WinCheckButton(hwnd, CHK_ALLOWRESIZE, plswData->Settings.bAllowResize);
    WinCheckButton(hwnd, CHK_TSKBARGROW, plswData->Settings.bTskBarGrow);
#else
    WinEnableControl(hwnd, CHK_TASKBARON,FALSE);
    WinEnableControl(hwnd, CHK_TASKBARALWAYSVISIBLE,FALSE);
    WinEnableControl(hwnd, RAD_TOP,FALSE);
    WinEnableControl(hwnd, RAD_BOTTOM,FALSE);
    WinEnableControl(hwnd, CHK_REDUCEDSK,FALSE);
    WinEnableControl(hwnd, CHK_FLATBTN,FALSE);
    WinEnableControl(hwnd, CHK_ALLOWRESIZE,FALSE);
    WinEnableControl(hwnd, CHK_TSKBARGROW,FALSE);
#endif

    WinEnableControl(hwnd, SPIN_BTNWID,!plswData->Settings.bIconsOnlyInTskBar);

    WinCheckButton(hwnd, CHK_GROUPITEMS, plswData->Settings.bGroupItems);
    WinCheckButton(hwnd, CHK_3DTASKBAR, plswData->Settings.b3DTaskBar);

    WinSendDlgItemMsg(hwnd,SPIN_BTNWID,SPBM_SETLIMITS,MPFROMLONG(WinQuerySysValue(HWND_DESKTOP,SV_CXSCREEN)),MPFROMLONG(WinQuerySysValue(HWND_DESKTOP,SV_CYICON)/2+2*BUTTONBORDERWIDTH+2+2*TSKBARFRAMEWIDTH));
    WinSendDlgItemMsg(hwnd,SPIN_BTNWID,SPBM_SETTEXTLIMIT,MPFROMSHORT(4),0);
    WinSendDlgItemMsg(hwnd,SPIN_BTNWID,SPBM_SETCURRENTVALUE,MPFROMLONG(plswData->Settings.usMaxBtnWid),0);

    bInit = FALSE;

    return (MRESULT)TRUE;
  }
  case WM_CONTROL: {
    BOOL bValue;

    if (bInit) break;
    bValue = WinQueryButtonCheckstate(hwnd,SHORT1FROMMP(mp1));

    switch (SHORT1FROMMP(mp1)) {
      case SPIN_BTNWID:
        if (SHORT2FROMMP(mp1)==SPBN_CHANGE) {
          ULONG ulBtnWid;
          SWP swp;

          WinSendDlgItemMsg(hwnd,SPIN_BTNWID,SPBM_QUERYVALUE,MPFROMP(&ulBtnWid),MPFROM2SHORT(0,SPBQ_ALWAYSUPDATE));
          plswData->Settings.usMaxBtnWid=(USHORT)ulBtnWid;
          WinQueryWindowPos(plswData->hwndTaskBarClient,&swp);
          WinPostMsg(plswData->hwndTaskBarClient,WM_SIZE,MPFROM2SHORT(swp.cx,swp.cy),MPFROM2SHORT(swp.cx,swp.cy));
        }
        break;

      case CHK_TASKBARON:
        plswData->Settings.bTaskBarOn = bValue;
        if (bValue) InitTaskBar(plswData,NULL,0); else DoneTaskBar(plswData);
        break;

      case CHK_TASKBARALWAYSVISIBLE:
        plswData->Settings.bTaskBarAlwaysVisible = !bValue;
        WinEnableControl(hwnd, CHK_REDUCEDSK, plswData->Settings.bTaskBarAlwaysVisible);
        WinShowWindow(plswData->hwndTaskBar,!bValue);
        break;

      case CHK_REDUCEDSK:
        plswData->Settings.bReduceDsk = bValue;
        break;

      case CHK_FLATBTN:
        plswData->Settings.bFlatButtons = bValue;
        WinInvalidateRect(plswData->hwndTaskBar,NULL,TRUE);
        break;

      case CHK_3DTASKBAR:
        plswData->Settings.b3DTaskBar = bValue;
        WinInvalidateRect(plswData->hwndTaskBarClient,NULL,TRUE);
        break;

      case CHK_GROUPITEMS:
        plswData->Settings.bGroupItems = bValue;
        WinPostMsg(plswData->hwndTaskBarClient,LSWM_INITBUTTONS,0,0);
        break;

      case CHK_ALLOWRESIZE: {
        ULONG flFrameFlags=WinQueryWindowULong(plswData->hwndTaskBar,QWL_STYLE);

        plswData->Settings.bAllowResize = bValue;
        if (bValue) {
          flFrameFlags&=~FS_BORDER; flFrameFlags|=FS_SIZEBORDER;
        } else {
          flFrameFlags&=~FS_SIZEBORDER; flFrameFlags|=FS_BORDER;
        }
        WinSetWindowULong(plswData->hwndTaskBar,QWL_STYLE,flFrameFlags);
        break;
      }
      case CHK_TSKBARGROW: {
        plswData->Settings.bTskBarGrow = bValue;
        WinEnableControl(hwnd, CHK_ALLOWRESIZE, !bValue);
        if (bValue) {
          WinPostMsg(plswData->hwndTaskBarClient,WM_SIZE,0,0);
        } else
          WinSetWindowPos(plswData->hwndTaskBar,0,0,0,
                          WinQuerySysValue(HWND_DESKTOP,SV_CXSCREEN)+2*TSKBARFRAMEWIDTH,
                          plswData->Settings.sTskBarCY,SWP_SIZE);
        break;
      }
      case RAD_TOP:
      case RAD_BOTTOM:
        plswData->Settings.bTaskBarTopScr = WinQueryButtonCheckstate(hwnd, RAD_TOP);
        if (plswData->Settings.bTaskBarTopScr)
          plswData->Settings.sTskBarY = WinQuerySysValue(HWND_DESKTOP,SV_CYSCREEN)-plswData->Settings.sTskBarCY+TSKBARFRAMEWIDTH;
        else
          plswData->Settings.sTskBarY = -TSKBARFRAMEWIDTH;

        WinSetWindowPos(plswData->hwndTaskBar,0 ,
                        plswData->Settings.sTskBarX,plswData->Settings.sTskBarY,
                        plswData->Settings.sTskBarCX,plswData->Settings.sTskBarCY,
                        SWP_MOVE);
        break;
    }

    break;
  }
  case WM_COMMAND:
    switch (SHORT1FROMMP(mp1)) {
    case DID_OK:
    case DID_CANCEL:
      WinPostMsg(WinQueryWindow(hwnd,QW_OWNER),msg,mp1,mp2);
      break;
    default:
      return WinDefDlgProc(hwnd, msg, mp1, mp2);
    }
    break;

  default:
    return WinDefDlgProc(hwnd, msg, mp1, mp2);
  }
  return 0;
}

MRESULT EXPENTRY MouseDlgProc(HWND hwnd,ULONG msg,MPARAM mp1,MPARAM mp2)
{ static BOOL bInit = FALSE;
  LSWDATA *plswData;

  plswData = WinQueryWindowPtr(hwnd,0);

  switch (msg) {
  case WM_INITDLG: {
    ULONG ulMenuEvent;
    USHORT usEvent;

    bInit = TRUE;
    SetControlsFont(hwnd,FALSE);

    DosGetNamedSharedMem((PVOID*)&plswData,SHAREMEM_NAME,PAG_READ | PAG_WRITE);
    WinSetWindowPtr(hwnd,0,plswData);
    DosFreeMem(plswData);

    usEvent=plswData->Settings.usSwitchMEvent;
    WinCheckButton(hwnd, usEvent==WM_BUTTON1CLICK?RAD_SWMBTN1:
                         usEvent==WM_BUTTON2CLICK?RAD_SWMBTN2:
                         usEvent==WM_BUTTON3CLICK?RAD_SWMBTN3:RAD_SWMBTN12, TRUE);

    usEvent=plswData->Settings.usMinMEvent;
    WinCheckButton(hwnd, usEvent==WM_BUTTON1CLICK?RAD_MINMBTN1:
                         usEvent==WM_BUTTON2CLICK?RAD_MINMBTN2:
                         usEvent==WM_BUTTON3CLICK?RAD_MINMBTN3:RAD_MINMBTN12, TRUE);

    usEvent=plswData->Settings.usCloseMEvent;
    WinCheckButton(hwnd, usEvent==WM_BUTTON1CLICK?RAD_CLOSEMBTN1:
                         usEvent==WM_BUTTON2CLICK?RAD_CLOSEMBTN2:
                         usEvent==WM_BUTTON3CLICK?RAD_CLOSEMBTN3:RAD_CLOSEMBTN12, TRUE);

    if (WinQuerySysValue(HWND_DESKTOP,SV_CMOUSEBUTTONS)<3) {
      WinEnableControl(hwnd, RAD_SWMBTN3,FALSE);
      WinEnableControl(hwnd, RAD_MINMBTN3,FALSE);
      WinEnableControl(hwnd, RAD_CLOSEMBTN3,FALSE);
    }
    ulMenuEvent = WinQuerySysValue(HWND_DESKTOP,SV_CONTEXTMENU);
    if (LOUSHORT(ulMenuEvent)==WM_BUTTON1CLICK && HIUSHORT(ulMenuEvent)==0) {
      WinEnableControl(hwnd, RAD_SWMBTN1,FALSE);
      WinEnableControl(hwnd, RAD_MINMBTN1,FALSE);
      WinEnableControl(hwnd, RAD_CLOSEMBTN1,FALSE);
    } else if (LOUSHORT(ulMenuEvent)==WM_BUTTON2CLICK && HIUSHORT(ulMenuEvent)==0) {
      WinEnableControl(hwnd, RAD_SWMBTN2,FALSE);
      WinEnableControl(hwnd, RAD_MINMBTN2,FALSE);
      WinEnableControl(hwnd, RAD_CLOSEMBTN2,FALSE);
    }

    bInit = FALSE;

    return (MRESULT)TRUE;
  }
  case WM_CONTROL: {

    if (bInit) break;
    switch (SHORT1FROMMP(mp1)) {
      case RAD_SWMBTN1:
      case RAD_SWMBTN2:
      case RAD_SWMBTN3:
      case RAD_SWMBTN12:
        plswData->Settings.usSwitchMEvent =
          (WinQueryButtonCheckstate(hwnd,RAD_SWMBTN1)?WM_BUTTON1CLICK:
           WinQueryButtonCheckstate(hwnd,RAD_SWMBTN2)?WM_BUTTON2CLICK:
           WinQueryButtonCheckstate(hwnd,RAD_SWMBTN3)?WM_BUTTON3CLICK:WM_CHORD);
        break;

      case RAD_MINMBTN1:
      case RAD_MINMBTN2:
      case RAD_MINMBTN3:
      case RAD_MINMBTN12:
        plswData->Settings.usMinMEvent =
          (WinQueryButtonCheckstate(hwnd,RAD_MINMBTN1)?WM_BUTTON1CLICK:
           WinQueryButtonCheckstate(hwnd,RAD_MINMBTN2)?WM_BUTTON2CLICK:
           WinQueryButtonCheckstate(hwnd,RAD_MINMBTN3)?WM_BUTTON3CLICK:WM_CHORD);
        break;

      case RAD_CLOSEMBTN1:
      case RAD_CLOSEMBTN2:
      case RAD_CLOSEMBTN3:
      case RAD_CLOSEMBTN12:
        plswData->Settings.usCloseMEvent =
          (WinQueryButtonCheckstate(hwnd,RAD_CLOSEMBTN1)?WM_BUTTON1CLICK:
           WinQueryButtonCheckstate(hwnd,RAD_CLOSEMBTN2)?WM_BUTTON2CLICK:
           WinQueryButtonCheckstate(hwnd,RAD_CLOSEMBTN3)?WM_BUTTON3CLICK:WM_CHORD);
        break;
    }

    break;
  }
  case WM_COMMAND:
    switch (SHORT1FROMMP(mp1)) {
    case DID_OK:
    case DID_CANCEL:
      WinPostMsg(WinQueryWindow(hwnd,QW_OWNER),msg,mp1,mp2);
      break;
    default:
      return WinDefDlgProc(hwnd, msg, mp1, mp2);
    }
    break;

  default:
    return WinDefDlgProc(hwnd, msg, mp1, mp2);
  }
  return 0;
}

MRESULT EXPENTRY HotkeyWndProc(HWND hwnd,ULONG msg,MPARAM mp1,MPARAM mp2)
{
  switch (msg) {
  case WM_CHAR:
    if (SHORT1FROMMP(mp1) & KC_CHAR)
    WinSendMsg(WinQueryWindow(hwnd,QW_OWNER),WM_CONTROL,
      MPFROM2SHORT(WinQueryWindowUShort(hwnd,QWS_ID),EN_SCANCODE),MPFROMCHAR(CHAR4FROMMP(mp1)));
  default:
    return (((PFNWP)WinQueryWindowPtr(hwnd,0))(hwnd,msg,mp1,mp2));
  }
}

MRESULT EXPENTRY MiscDlgProc(HWND hwnd,ULONG msg,MPARAM mp1,MPARAM mp2)
{ static BOOL bInit;
  LSWDATA *plswData;
  static USHORT usLangArr[128];

  plswData = WinQueryWindowPtr(hwnd,0);

  switch (msg) {
  case WM_INITDLG: {
    static UCHAR ucDllName[_MAX_PATH],ucLangStr[32];
    USHORT usLang;

    bInit = TRUE;
    SetControlsFont(hwnd,FALSE);
    DosGetNamedSharedMem((PVOID*)&plswData,SHAREMEM_NAME,PAG_READ | PAG_WRITE);
    WinSetWindowPtr(hwnd,0,plswData);
    DosFreeMem(plswData);

    WinCheckButton(hwnd, CHK_SWDESKTOP, plswData->Settings.bDesktopMinimizes);
#ifndef XWORKPLACE
    WinCheckButton(hwnd, CHK_SHOWINWINLIST, plswData->Settings.bShowInWinList);
#else
    WinEnableControl(hwnd,CHK_SHOWINWINLIST,FALSE);
#endif
    WinCheckButton(hwnd, CHK_ZORDER, plswData->Settings.bChangeZOrder);

    WinSendDlgItemMsg(hwnd,EF_HOTKEY,EM_SETTEXTLIMIT,MPFROMSHORT(1),0);
    WinSetDlgItemText(hwnd,EF_HOTKEY,&plswData->Settings.ucSettingsKey);

    WinSetWindowPtr(WinWindowFromID(hwnd,EF_HOTKEY),0,
                   (VOID *)WinSubclassWindow(WinWindowFromID(hwnd,EF_HOTKEY),HotkeyWndProc));

    while (FindResDll(ucDllName, sizeof(ucDllName), &usLang, ucLangStr, sizeof(ucLangStr)))
      if (usLang!=0) {
        SHORT sIndex;

        sIndex = (SHORT)WinSendDlgItemMsg(hwnd,COMBO_LANG,LM_INSERTITEM,MPFROMSHORT(LIT_END), MPFROMP(ucLangStr));
        if (sIndex!=LIT_ERROR) {
          usLangArr[sIndex]=usLang;
          if (usLang == plswData->Settings.usLanguage)
            WinSendDlgItemMsg(hwnd,COMBO_LANG,LM_SELECTITEM,MPFROMSHORT(sIndex), MPFROMSHORT(TRUE));
        }
      }

    bInit = FALSE;
    return (MRESULT)TRUE;
  }

  case WM_CONTROL:
    if (bInit) break;

    switch (SHORT1FROMMP(mp1)) {
      case COMBO_LANG:
        if (SHORT2FROMMP(mp1)==CBN_EFCHANGE) {
          SHORT sIndex;

          sIndex = (SHORT)WinSendDlgItemMsg(hwnd,COMBO_LANG,LM_QUERYSELECTION,MPFROMSHORT(LIT_FIRST), 0);
          if (sIndex>=0 && usLangArr[sIndex]!=plswData->Settings.usLanguage) {
            WinDestroyAccelTable(plswData->haccAlt);
            WinDestroyAccelTable(plswData->haccCtrl);
            WinDestroyAccelTable(plswData->haccNoAlt);
            DosFreeModule(plswData->hmodRes);

            if ((plswData->hmodRes = LoadResource(usLangArr[sIndex],plswData,FALSE))==0) {
              plswData->hmodRes = LoadResource(plswData->Settings.usLanguage,plswData,TRUE);
              WinMessageBox(HWND_DESKTOP,HWND_DESKTOP,"Could not load resource DLL or resource DLL invalid",PGMNAME,0,MB_ERROR | MB_CANCEL);
            }
          }
        }
        break;

      case EF_HOTKEY:
        if (SHORT2FROMMP(mp1)==EN_CHANGE) {
          UCHAR buf[3];

          WinQueryDlgItemText(hwnd,EF_HOTKEY,sizeof(buf),buf);
          plswData->Settings.ucSettingsKey=buf[0];
        } else if (SHORT2FROMMP(mp1)==EN_SCANCODE) {
          plswData->Settings.ucSettingsScan = CHAR1FROMMP(mp2);
        }
        break;

      case CHK_ZORDER:
        plswData->Settings.bChangeZOrder = WinQueryButtonCheckstate(hwnd, CHK_ZORDER);
        break;

      case CHK_SWDESKTOP:
        plswData->Settings.bDesktopMinimizes = WinQueryButtonCheckstate(hwnd, CHK_SWDESKTOP);
        break;

      case CHK_SHOWINWINLIST: {
        SHORT iIndex;
        SWCNTRL swctl;
        PID pidPid; TID tidTid;

        WinQueryWindowProcess(hwnd,&pidPid,&tidTid);
        WinQuerySwitchEntry(WinQuerySwitchHandle(0,pidPid),&swctl);

        plswData->Settings.bShowInWinList = WinQueryButtonCheckstate(hwnd, CHK_SHOWINWINLIST);
        WinQuerySwitchEntry(plswData->hswitch, &swctl);
        swctl.uchVisibility = plswData->Settings.bShowInWinList ? SWL_VISIBLE : SWL_INVISIBLE;
        WinChangeSwitchEntry(plswData->hswitch, &swctl);

        if ((SHORT)(WinSendDlgItemMsg(hwnd,LB_REMOVED,LM_SEARCHSTRING,
                                      MPFROM2SHORT(LSS_CASESENSITIVE,LIT_FIRST),
                                      MPFROMP(swctl.szSwtitle)))==LIT_NONE) {
          iIndex = (SHORT)(WinSendDlgItemMsg(hwnd,LB_ADDED,LM_SEARCHSTRING,
                                           MPFROM2SHORT(LSS_CASESENSITIVE,LIT_FIRST),
                                           MPFROMP(swctl.szSwtitle)));
          if (plswData->Settings.bShowInWinList && iIndex==LIT_NONE)
            WinSendDlgItemMsg(hwnd,LB_ADDED,LM_INSERTITEM,MPFROMSHORT(LIT_END),MPFROMP(swctl.szSwtitle));
          else if (!plswData->Settings.bShowInWinList && iIndex!=LIT_ERROR && iIndex!=LIT_NONE)
            WinSendDlgItemMsg(hwnd,LB_ADDED,LM_DELETEITEM,MPFROMSHORT(iIndex),0);
        }
        break;
      }
    }
    break;

  case WM_COMMAND:
    switch (SHORT1FROMMP(mp1)) {
    case DID_OK:
    case DID_CANCEL:
      WinPostMsg(WinQueryWindow(hwnd,QW_OWNER),msg,mp1,mp2);
      break;
    default:
      return WinDefDlgProc(hwnd, msg, mp1, mp2);
    }
    break;
  default:
    return WinDefDlgProc(hwnd, msg, mp1, mp2);
  }
  return 0;
}

MRESULT EXPENTRY ParmNBWndProc(HWND hwnd,ULONG msg,MPARAM mp1,MPARAM mp2)
{
  switch (msg) {
  case WM_CHAR:
    if ((SHORT1FROMMP(mp1) & KC_VIRTUALKEY) && SHORT2FROMMP(mp2)==VK_ESC)
    WinPostMsg(WinQueryWindow(hwnd,QW_OWNER),WM_COMMAND,
      MPFROMSHORT(DID_CANCEL),MPFROM2SHORT(CMDSRC_PUSHBUTTON,FALSE));
  default:
    return (((PFNWP)WinQueryWindowPtr(hwnd,0))(hwnd,msg,mp1,mp2));
  }
}

MRESULT EXPENTRY ParmDlgProc(HWND hwnd,ULONG msg,MPARAM mp1,MPARAM mp2)
{ LSWDATA *plswData;

  plswData = WinQueryWindowPtr(hwnd,0);

  switch (msg) {
  case WM_INITDLG: {
    HWND  hwndPage, hwndNB;
    ULONG ulPageId;
    BOOKPAGEINFO pi;
    RECTL rcl;
    UCHAR ucBuf1[32],ucBuf2[32],ucBuf3[32],ucBuf4[32],ucBuf5[64];
    BOOL bMajorPage;
    SWP swp;
    USHORT k,usNameID,usTabID;

    plswData = (PLSWDATA)PVOIDFROMMP(mp2);
    WinSetWindowPtr(hwnd,0,plswData);
    SetControlsFont(hwnd,FALSE);

    hwndNB = WinWindowFromID(hwnd,NB_PARAMS);

    WinSendMsg(hwndNB, BKM_SETNOTEBOOKCOLORS, MPFROMLONG(SYSCLR_FIELDBACKGROUND),
      MPFROMSHORT(BKA_BACKGROUNDPAGECOLORINDEX));

    for (k = 0; k < 7; k++) {
      bMajorPage=(k==0||k==2||k==6);
      ulPageId = (LONG)WinSendMsg(hwndNB, BKM_INSERTPAGE, 0,
                                  MPFROM2SHORT(BKA_AUTOPAGESIZE | (bMajorPage?BKA_MAJOR:BKA_MINOR) | BKA_STATUSTEXTON, BKA_LAST));

      memset(&pi,0,sizeof(pi));
      pi.cb = sizeof(pi);
      pi.fl = BFA_PAGEFROMDLGRES|BFA_STATUSLINE|(bMajorPage?BFA_MAJORTABTEXT:0)|(k!=6?BFA_MINORTABTEXT:0);
      pi.bLoadDlg = TRUE; pi.hmodPageDlg = plswData->hmodRes;

      switch (k) {
      case 0: pi.idPageDlg = usTabID = DLG_POPUP; usNameID=DLG_GENERALPOPUP; pi.pfnPageDlgProc = (PFN)PopupDlgProc; break;
      case 1: pi.idPageDlg = DLG_EXCLUDE; usTabID = DLG_POPUP; usNameID=DLG_EXCLUDEPOPUP; pi.pfnPageDlgProc = (PFN)ListDlgProc; pi.pPageDlgCreateParams =&plswData->Settings.SkipListPopup; break;
      case 2: pi.idPageDlg = usTabID = DLG_TASKBAR; usNameID=DLG_GENERALTSKBAR; pi.pfnPageDlgProc = (PFN)TaskBarDlgProc; break;
      case 3: pi.idPageDlg = DLG_MOUSE; usTabID = DLG_TASKBAR; usNameID=DLG_MOUSE; pi.pfnPageDlgProc = (PFN)MouseDlgProc;  break;
      case 4: pi.idPageDlg = DLG_EXCLUDE; usTabID = DLG_TASKBAR; usNameID=DLG_EXCLUDETSKBAR; pi.pfnPageDlgProc = (PFN)ListDlgProc; pi.pPageDlgCreateParams =&plswData->Settings.SkipListTskBar; break;
      case 5: pi.idPageDlg = DLG_EXCLUDE; usTabID = DLG_TASKBAR; usNameID=DLG_ICONSONLY; pi.pfnPageDlgProc = (PFN)ListDlgProc; pi.pPageDlgCreateParams =&plswData->Settings.ListIconsOnly; break;
      case 6: pi.idPageDlg = usTabID = DLG_MISC; usNameID=DLG_MISC; pi.pfnPageDlgProc = (PFN)MiscDlgProc; break;
      }

      WinLoadString(WinQueryAnchorBlock(hwnd), plswData->hmodRes, usTabID,sizeof(ucBuf1),ucBuf1);
      WinLoadString(WinQueryAnchorBlock(hwnd), plswData->hmodRes, usNameID,sizeof(ucBuf2),ucBuf2);
      WinLoadString(WinQueryAnchorBlock(hwnd), plswData->hmodRes, STR_DLGPAGE,sizeof(ucBuf3),ucBuf3);
      pi.cbMajorTab = strlen(ucBuf1); pi.pszMajorTab = ucBuf1;
      sprintf(ucBuf5,"%s: %s", ucBuf1, ucBuf2);
      pi.cbMinorTab = strlen(ucBuf5); pi.pszMinorTab = ucBuf5;
      sprintf(ucBuf4,ucBuf3,k==6?1:k<2?k%2+1:(k-2)%4+1,k==6?1:k<2?2:4);
      pi.cbStatusLine = strlen(ucBuf4); pi.pszStatusLine = ucBuf4;

      WinSendMsg(hwndNB,BKM_SETPAGEINFO,MPFROMLONG(ulPageId),MPFROMP(&pi));

      hwndPage=(HWND)WinSendMsg(hwndNB,BKM_QUERYPAGEWINDOWHWND,MPFROMLONG(ulPageId),0);
      if (k==1) {
        WinQueryWindowRect(hwndPage,&rcl);
        WinSendMsg(hwndNB, BKM_CALCPAGERECT, MPFROMP(&rcl), MPFROMSHORT(FALSE));
        WinQueryWindowPos(WinWindowFromID(hwnd,DID_OK),&swp);
        WinSetWindowPos(hwnd,0,0,0,rcl.xRight-rcl.xLeft+8, rcl.yTop-rcl.yBottom+swp.y+
                        swp.cy+swp.cy/2+2*WinQuerySysValue(HWND_DESKTOP,SV_CYTITLEBAR), SWP_SIZE);
        WinSetWindowUShort(hwndPage,QWS_ID,DLG_EXCLUDEPOPUP);
      } else if (k==4)
        WinSetWindowUShort(hwndPage,QWS_ID,DLG_EXCLUDETSKBAR);
      else if (k==5)
        WinSetWindowUShort(hwndPage,QWS_ID,DLG_ICONSONLY);
    }

    WinSetWindowPtr(hwndNB,0,(VOID *)WinSubclassWindow(hwndNB,ParmNBWndProc));

    WinQueryWindowPos(hwnd, &swp);
    WinSetWindowPos(hwnd, 0, (WinQuerySysValue(HWND_DESKTOP, SV_CXSCREEN)-swp.cx)/2,
                    (WinQuerySysValue(HWND_DESKTOP, SV_CYSCREEN)-swp.cy)/2, 0, 0, SWP_MOVE);
    WinSetFocus(HWND_DESKTOP, hwndNB);

    return (MRESULT)TRUE;
  }

  case WM_ADJUSTWINDOWPOS: {
    PSWP pswp;
    SWP swp;

    pswp=PVOIDFROMMP(mp1);
    if (pswp->fl & SWP_SIZE) {
      WinQueryWindowPos(WinWindowFromID(hwnd,DID_OK),&swp);
      WinSetWindowPos(WinWindowFromID(hwnd,NB_PARAMS),0,4,swp.y+swp.cy+swp.cy/2, pswp->cx-8,
                      pswp->cy-swp.y-swp.cy-swp.cy/2-
                      2*WinQuerySysValue(HWND_DESKTOP,SV_CYTITLEBAR),SWP_SIZE|SWP_MOVE);
    }
    return (WinDefDlgProc(hwnd,msg,mp1,mp2));
  }

  case WM_COMMAND:
    switch (SHORT1FROMMP(mp1)) {
    case DID_OK: {
      USHORT rc;
      static UCHAR ucErrMsg[100],ucFName[CCHMAXPATH],ucBuf[100+CCHMAXPATH];

      GetIniFileName(ucFName,sizeof(ucFName));

      if ((rc=SaveSettings(WinQueryAnchorBlock(hwnd),ucFName,&plswData->Settings))!=0) {
        WinLoadString(WinQueryAnchorBlock(hwnd),NULLHANDLE,rc==1?MSG_CANTOPEN:MSG_CANTSAVE,sizeof(ucErrMsg),ucErrMsg);
        sprintf(ucBuf,ucErrMsg,ucFName);
        WinAlarm(HWND_DESKTOP,WA_ERROR);
        WinMessageBox(HWND_DESKTOP,hwnd,ucBuf,PGMNAME,0,MB_ERROR | MB_CANCEL);
      }
    }
    default:
      return WinDefDlgProc(hwnd, msg, mp1, mp2);
    }

    break;

  case LSWM_UPDATEEXCLUDEDLG:
  case LSWM_SWITCHLISTCHANGED: {
    HWND hwndPage;
    ULONG ulPageId=
      (ULONG)WinSendMsg(WinWindowFromID(hwnd,NB_PARAMS),BKM_QUERYPAGEID,0,MPFROM2SHORT(BKA_FIRST,0));
    USHORT usDlgId;
    BOOL bNeedUpdate;

    while (ulPageId>0) {
      hwndPage=LONGFROMMP(WinSendMsg(WinWindowFromID(hwnd,NB_PARAMS),BKM_QUERYPAGEWINDOWHWND,MPFROMLONG(ulPageId),0));
      usDlgId=WinQueryWindowUShort(hwndPage,QWS_ID);
      if (((usDlgId == DLG_EXCLUDEPOPUP || usDlgId == DLG_EXCLUDETSKBAR || usDlgId == DLG_ICONSONLY)
           && msg==LSWM_SWITCHLISTCHANGED ) ||
          (msg==LSWM_UPDATEEXCLUDEDLG && SHORT1FROMMP(mp2)==UPDATEPOPUPLIST && usDlgId == DLG_EXCLUDEPOPUP) ||
          (msg==LSWM_UPDATEEXCLUDEDLG && SHORT1FROMMP(mp2)==UPDATETSKBARLIST && usDlgId == DLG_EXCLUDETSKBAR) ||
          (msg==LSWM_UPDATEEXCLUDEDLG && SHORT1FROMMP(mp2)==UPDATEICONSONLYLIST && usDlgId == DLG_ICONSONLY)
         )
        WinPostMsg(hwndPage,LSWM_UPDATEEXCLUDEDLG,MPFROMSHORT(UPDATEADDEDLIST|(msg==LSWM_UPDATEEXCLUDEDLG?UPDATEREMOVEDLIST:0)),0);
      ulPageId = (LONG)WinSendMsg(WinWindowFromID(hwnd,NB_PARAMS),BKM_QUERYPAGEID,MPFROMLONG(ulPageId),MPFROM2SHORT(BKA_NEXT,0));
    }
  }

  default:
    return (WinDefDlgProc(hwnd,msg,mp1,mp2));
  }
  return 0;
}


