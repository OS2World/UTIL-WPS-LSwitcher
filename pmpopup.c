/*
 *      Copyright (C) 1997-2006 Andrei Los.
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

#include "lswitch.h"
#include "settings.h"
#include "taskbar.h"
#include "common.h"
#include "fspopup.h"
#include "prmdlg.h"
#include "pmpopup.h"
#include "lswres.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <process.h>
#include <ctype.h>
#include <unikbd.h>


/* we subclass the window procedure of the popup window frame */
MRESULT EXPENTRY FrameWndProc(HWND hwnd,ULONG msg,MPARAM mp1,MPARAM mp2)
{ LSWDATA *plswData;

  if (msg!=WM_CREATE)
#ifndef XWORKPLACE
    plswData = WinQueryWindowPtr(WinWindowFromID(hwnd,FID_CLIENT),0);
#else
    plswData = ((PXCENTERWIDGET)WinQueryWindowPtr(WinWindowFromID(hwnd,FID_CLIENT),0))->pUser;
#endif

  switch (msg) {
  case WM_WINDOWPOSCHANGED: {
    PSWP pswp = ((PSWP)PVOIDFROMMP(mp1));
    SHORT x,y,oldx=(plswData->lCurrDesktop&0xFF00)>>8,oldy=plswData->lCurrDesktop&0xFF;
    USHORT usCXScreen=WinQuerySysValue(HWND_DESKTOP,SV_CXSCREEN),
           usCYScreen=WinQuerySysValue(HWND_DESKTOP,SV_CYSCREEN);

    if (plswData->bNowActive || !(pswp->fl & (SWP_SIZE|SWP_MOVE)))
      break;

    x=(POPUPWINHIDEPOSX-pswp->x)/usCXScreen;  y=(POPUPWINHIDEPOSY-pswp->y)/usCYScreen;
    plswData->lCurrDesktop = ((x<<8)+y);

    if (x!=oldx || y!=oldy) {
      SWP swp;
      HWND hwndNext;
      USHORT k;
      BOOL bMinMax;
      HENUM henum = WinBeginEnumWindows(HWND_DESKTOP);

      while ((hwndNext = WinGetNextWindow(henum)) != NULLHANDLE) {
        WinQueryWindowPos(hwndNext,&swp);
        if (swp.fl & SWP_MINIMIZE) {
          for (k=0,bMinMax=FALSE;k<MINMAXITEMS && !bMinMax;k++)
            if (plswData->MinMaxWnd[2*k]==hwndNext) bMinMax=TRUE;
          if (!bMinMax) {
            swp.x=WinQueryWindowUShort(hwndNext, QWS_XRESTORE);
            swp.y=WinQueryWindowUShort(hwndNext, QWS_YRESTORE);
            swp.x+=(oldx-x)*(usCXScreen+8); swp.y+=(oldy-y)*(usCYScreen+8);
            WinSetWindowUShort(hwndNext, QWS_XRESTORE,swp.x);
            WinSetWindowUShort(hwndNext, QWS_YRESTORE,swp.y);
          }
        } else if (swp.fl & SWP_HIDE) {
          swp.x+=(oldx-x)*(usCXScreen+8); swp.y+=(oldy-y)*(usCYScreen+8);
          WinSetWindowPos(hwndNext,0,swp.x,swp.y,0,0,SWP_MOVE);
        }
      }
      WinEndEnumWindows (henum);
    }

    break;
  }

  case WM_ADJUSTWINDOWPOS: {
    PSWP pswp = ((PSWP)PVOIDFROMMP(mp1));

    if (!plswData->bNowActive) {
      /* make sure we are not brought to top or activated as a result of other
       windows repositioning */
      if (pswp->fl & SWP_ZORDER) pswp->fl &= ~SWP_ZORDER;
      if (pswp->fl & SWP_ACTIVATE) pswp->fl &= ~SWP_ACTIVATE;

      if (pswp->fl & SWP_SHOW)
        WinPostMsg(plswData->hwndPopup,WM_COMMAND,MPFROMSHORT(CMD_SHOWSETTINGS),MPFROM2SHORT(CMDSRC_OTHER,FALSE));
    }
    break;
  }

  }
  return ((PFNWP)WinQueryWindowPtr(hwnd,0))(hwnd,msg,mp1,mp2);
}


/* This function returns the bottom left corner coords of the usNum number
  icon counting from the top left corner in the popup window */
VOID IconPosFromNumber(LSWDATA *plswData,USHORT usNum,USHORT *usPosX, USHORT *usPosY)
{ USHORT usLine,usLines,usGapX,usGapY,usIconsPerLine;
  RECTL rcl;
  ULONG cxIcon,cyIcon;

  usIconsPerLine = min(plswData->usItems,MAXICONSINLINE);
  usLine = usNum/MAXICONSINLINE+1;
  usLines = min(ICONLINES,(plswData->usItems-1)/MAXICONSINLINE+1);
  cxIcon = WinQuerySysValue(HWND_DESKTOP, SV_CXICON);
  cyIcon = WinQuerySysValue(HWND_DESKTOP, SV_CYICON);
  usGapX = cxIcon*ICONSPACINGX;
  usGapY = cyIcon*ICONSPACINGY;

  WinQueryWindowRect(plswData->hwndPopClient,&rcl);

  *usPosX = rcl.xRight/2-
    cxIcon*(usIconsPerLine-(usIconsPerLine%2==0?1:0))/2-
    (usIconsPerLine-1)/2*usGapX+(usNum-(usLine-1)*MAXICONSINLINE)*(cxIcon+usGapX);

  *usPosY = rcl.yTop-2*cyIcon*SELBOXOFFSET-SELBOXLINEWIDTH-
    usLine*cyIcon-(usLine-1)*usGapY-(rcl.xRight-rcl.xLeft)/(POPUPBORDERSIZER+1);
}


/* This function returns the number of the item in the TaskArr from the number
 of its icon counting from the top left corner in the popup window */
SHORT ItemNumFromIconNum(LSWDATA *plswData,SHORT iNum)
{ SHORT iItemNum;
  USHORT usLines;

  usLines = min(ICONLINES,(plswData->usItems-1)/MAXICONSINLINE+1);

  iItemNum = plswData->iShift+iNum-
    (min(MAXICONSINLINE,plswData->usItems)-1)/2-(usLines-1)/2*MAXICONSINLINE+
    (plswData->Settings.bScrollItems ? plswData->usCurrItem : 0);

  AdjustItem(iItemNum,plswData->usItems);

  return iItemNum;
}


/* This function returns the number of the icon in the popup window
 counting from the top left corner from the corresponding item number in the
 TaskArr. */
SHORT IconNumFromItemNum(LSWDATA *plswData, USHORT usNum)
{ USHORT usLines, usLastIconNum;
  SHORT iIconNum;

  usLastIconNum = min(plswData->usItems,MAXICONSINLINE*ICONLINES)-1;

  usLines = min(ICONLINES,(plswData->usItems-1)/MAXICONSINLINE+1);

  iIconNum = usNum-plswData->iShift+
    (min(MAXICONSINLINE,plswData->usItems)-1)/2+(usLines-1)/2*MAXICONSINLINE-
    (plswData->Settings.bScrollItems ? plswData->usCurrItem : 0);

  while (iIconNum > usLastIconNum || iIconNum < 0)
    if (iIconNum > usLastIconNum)
      iIconNum -= plswData->usItems;
    else if (iIconNum < 0)
      iIconNum += plswData->usItems;

  return iIconNum;
}


/* This function returns the number of the item in the TaskArr
 whose icon in the popup window is currently under the mouse pointer.
 -1 is returned if mouse pointer does not point on any icon*/
SHORT ItemNumFromPos(LSWDATA *plswData, USHORT usX, USHORT usY)
{ USHORT usItemNum;
  USHORT usPosX,usPosY,usGapX,usGapY,usCol,usRow;
  ULONG cxIcon,cyIcon;

  cxIcon = WinQuerySysValue(HWND_DESKTOP, SV_CXICON);
  cyIcon = WinQuerySysValue(HWND_DESKTOP, SV_CYICON);
  usGapX = cxIcon*ICONSPACINGX;
  usGapY = cyIcon*ICONSPACINGY;

  IconPosFromNumber(plswData,0,&usPosX,&usPosY);
  usPosY+=cyIcon; //top left corner instead of the bottom left

  usCol = (usX-usPosX)/(cxIcon+usGapX)+1;
  usRow = (usPosY-usY)/(cyIcon+usGapY)+1;

  usItemNum = ItemNumFromIconNum(plswData,(usRow-1)*MAXICONSINLINE+usCol-1);
  /* check if mouse points on an icon */
  if ((usRow-1)*MAXICONSINLINE+usCol<=plswData->usItems &&
      usX>=usPosX+(usCol-1)*(cxIcon+usGapX) && usX<=usPosX+usCol*(cxIcon+usGapX)-usGapX &&
      usY<=usPosY-(usRow-1)*(cyIcon+usGapY) && usY>=usPosY-usRow*(cyIcon+usGapY)+usGapY)
    return usItemNum;
  else
    return -1;
}



VOID MakeHintStr(LSWDATA *plswData,USHORT usItem,UCHAR *ucStr,USHORT usStrLen)
{ UCHAR ucBuf[32],ucCmd[32],*pucTilde,ucHot;
  USHORT k,l;

  WinLoadString(plswData->hab,plswData->hmodRes,STRID_HINTS,usStrLen,ucStr);

  for (k = CMD_HIDE; k <= CMD_CLOSE; k++)
    if (MenuNeedsItem(plswData,usItem,k,ucBuf,sizeof(ucBuf),FALSE) && k!=CMD_MOVE) {
      if ((pucTilde = strchr(ucBuf,'~'))!=NULL) {
        memmove(pucTilde,pucTilde+1,strlen(pucTilde));
        if (plswData->Settings.usLanguage == RU) {
          if (*pucTilde >= 160 && *pucTilde <= 175)
            ucHot = *pucTilde - 32;
          else if (*pucTilde >= 224 && *pucTilde <= 239)
            ucHot = *pucTilde - 80;
          else
            ucHot = *pucTilde;
        } else
          ucHot = toupper(*pucTilde);
      }

      if (plswData->Settings.usLanguage == RU) {
        for (l=0; l < strlen(ucBuf); l++)
          if (ucBuf[l] >= 128 && ucBuf[l]<= 143)
            ucBuf[l] += 32;
          else if (ucBuf[l] >= 144 && ucBuf[l] <= 159)
            ucBuf[l] += 80;
      } else if (plswData->Settings.usLanguage != DE)
        strlwr(ucBuf);

      sprintf(ucCmd,"  %c:%s",ucHot,ucBuf);
      strncat(ucStr,ucCmd,usStrLen-strlen(ucStr)-1);
    }
}


VOID AdjustPopupWin(LSWDATA *plswData,BOOL bInit)
{ USHORT cxPopup,cyPopup,cxIcon,cyIcon,usLines,usPosX,usPosY,
    usTxtHgtTitle,usTxtHgtHints, usPopupBorderSize;
  RECTL rcl;
  HWND hwndTitle,hwndHints;
  HPS hps;
  POINTL aptl[TXTBOX_COUNT],ptl;
  UCHAR ucBuf[2*NAMELEN];

  cxIcon = WinQuerySysValue(HWND_DESKTOP, SV_CXICON);
  cyIcon = WinQuerySysValue(HWND_DESKTOP, SV_CYICON);

  hwndTitle = WinWindowFromID(plswData->hwndPopClient,ID_TITLESTR);
  hwndHints = WinWindowFromID(plswData->hwndPopClient,ID_HINTSTR);

  if (bInit) {
    WinSetPresParam(plswData->hwndPopClient,PP_BACKGROUNDCOLORINDEX,
                    sizeof(plswData->Settings.lBackColor),
                    &plswData->Settings.lBackColor);

    WinSetPresParam(hwndTitle,PP_FONTNAMESIZE,
                    strlen(plswData->Settings.ucTitleFont)+1,
                    plswData->Settings.ucTitleFont);
    WinSetPresParam(hwndTitle,PP_BACKGROUNDCOLOR,
                    sizeof(plswData->Settings.lBackColor),
                    &plswData->Settings.lBackColor);
    WinSetPresParam(hwndTitle,PP_FOREGROUNDCOLOR,
                    sizeof(plswData->Settings.lTitleRGBCol),
                    &plswData->Settings.lTitleRGBCol);

    WinSetPresParam(hwndHints,PP_BACKGROUNDCOLOR,
                    sizeof(plswData->Settings.lBackColor),
                    &plswData->Settings.lBackColor);
    WinSetPresParam(hwndHints,PP_FOREGROUNDCOLOR,
                    sizeof(plswData->Settings.lHintsRGBCol),
                    &plswData->Settings.lHintsRGBCol);

    WinSetAccelTable(plswData->hab,plswData->Settings.bStickyPopup?plswData->haccNoAlt:
                     plswData->Settings.ucPopupHotKey==0?plswData->haccAlt:
                     plswData->Settings.ucPopupHotKey==1?plswData->haccCtrl:plswData->haccNoAlt,
                     plswData->hwndPopup);
  }

  hps = WinGetPS(hwndTitle);
  GpiQueryTextBox(hps,1L,"M",TXTBOX_COUNT,aptl);
  WinReleasePS(hps);
  usTxtHgtTitle = aptl[TXTBOX_TOPLEFT].y-aptl[TXTBOX_BOTTOMLEFT].y;

  hps = WinGetPS(hwndHints);
  GpiQueryTextBox(hps,1L,"M",TXTBOX_COUNT,aptl);
  WinReleasePS(hps);
  usTxtHgtHints = aptl[TXTBOX_TOPLEFT].y-aptl[TXTBOX_BOTTOMLEFT].y;

  GetItemTitle(plswData->TaskArr[plswData->usCurrItem].hsw,ucBuf,sizeof(ucBuf),FALSE);
  WinSetWindowText(hwndTitle,ucBuf);

  MakeHintStr(plswData,plswData->usCurrItem,ucBuf,sizeof(ucBuf));
  WinSetWindowText(hwndHints,ucBuf);

  WinSendMsg(plswData->hwndPopup,WM_QUERYBORDERSIZE,MPFROMP(&ptl),0);
  cxPopup = cxIcon*MAXICONSINLINE+cxIcon*(MAXICONSINLINE+2)*ICONSPACINGX+2*ptl.x;
  usPopupBorderSize=cxPopup/POPUPBORDERSIZER;
  cxPopup+=2*usPopupBorderSize;
  usLines = plswData->Settings.bOldPopup ? 1 :
    min(ICONLINES,(plswData->usItems-1)/MAXICONSINLINE+1);
  cyPopup = usLines*cyIcon+(usLines-1)*cyIcon*ICONSPACINGY+
    usTxtHgtTitle*
      ((plswData->Settings.bShowHints || plswData->Settings.bOldPopup) ? 2: 3)/2+
    ((plswData->Settings.bShowHints && !plswData->Settings.bOldPopup) ? usTxtHgtHints : 0)+
    4*cyIcon*SELBOXOFFSET+2*SELBOXLINEWIDTH+
    2*ptl.y+2*usPopupBorderSize;

  WinSetWindowPos(plswData->hwndPopup, HWND_TOP,
                  (WinQuerySysValue(HWND_DESKTOP, SV_CXSCREEN)-cxPopup)/2,
                  (WinQuerySysValue(HWND_DESKTOP, SV_CYSCREEN)-cyPopup)/2,
                  cxPopup, cyPopup,
                  SWP_MOVE | SWP_SIZE |
                  (bInit? (SWP_ACTIVATE | SWP_ZORDER | SWP_SHOW) : 0));

  // not sure if this is needed but sometimes the popup window hides immediately
  // after showing. Maybe it doesn't get focus and WM_TIMER hides it, let's
  // observe if this happens ever if we set the focus explicitly
  if (bInit) {
    WinSetFocus(HWND_DESKTOP,plswData->hwndPopClient);
    WinStartTimer(plswData->hab, plswData->hwndPopClient, 0, BUBBLETIMERINTERVAL);
  }

  WinQueryWindowRect(plswData->hwndPopClient,&rcl);

  if (plswData->Settings.bOldPopup) {
    WinSetWindowPos(hwndTitle, 0, cxIcon+cxIcon*ICONSPACINGX, 1,
                    rcl.xRight-cxIcon-cxIcon*ICONSPACINGX-2, rcl.yTop-2,
                    SWP_SIZE | SWP_MOVE);
    WinSetWindowPos(hwndHints,0,0,0,0,0,SWP_SIZE | SWP_MOVE);
  } else {
    IconPosFromNumber(plswData, (usLines-1)*MAXICONSINLINE, &usPosX,&usPosY);
    WinSetWindowPos(hwndTitle, 0,
                    1+usPopupBorderSize,
                    1+usPopupBorderSize+(plswData->Settings.bShowHints ? usTxtHgtHints : 0),
                    rcl.xRight-2-2*usPopupBorderSize,
                    usTxtHgtTitle*(plswData->Settings.bShowHints ? 2 : 3)/2,
                    SWP_SIZE | SWP_MOVE);
    WinSetWindowPos(hwndHints,0,
                    plswData->Settings.bShowHints ? 1+usPopupBorderSize : 0,
                    plswData->Settings.bShowHints ? 1+usPopupBorderSize : 0,
                    plswData->Settings.bShowHints ? rcl.xRight-2-2*usPopupBorderSize : 0,
                    plswData->Settings.bShowHints ? usTxtHgtHints : 0,
                    SWP_SIZE | SWP_MOVE);
  }

  WinInvalidateRect(plswData->hwndPopClient,NULL,TRUE);
}


/* popup client window procedure */
MRESULT EXPENTRY PopupWndProc(HWND hwnd,ULONG msg,MPARAM mp1,MPARAM mp2)
{ static ULONG cxIcon,cyIcon,cyPointer;
  static HWND hwndTitle,hwndHints;
  static BOOL bNowSticky=FALSE;
  static USHORT usScrollCnt=0;
  LSWDATA *plswData;
  MRESULT mrc=0;

  if (msg!=WM_CREATE)
#ifndef XWORKPLACE
    plswData = WinQueryWindowPtr(hwnd,0);
#else
    plswData = ((PXCENTERWIDGET)WinQueryWindowPtr(hwnd,0))->pUser;
#endif

  switch (msg) {
  case WM_HSCROLL:
  case WM_VSCROLL:
    usScrollCnt=++usScrollCnt%plswData->Settings.usScrollDiv;

    if (usScrollCnt==0 && (SHORT2FROMMP(mp2)==SB_LINELEFT || SHORT2FROMMP(mp2)==SB_LINERIGHT))
      WinPostMsg(hwnd,WM_COMMAND,MPFROMSHORT(SHORT2FROMMP(mp2)==SB_LINELEFT?CMD_SCROLLLEFT:CMD_SCROLLRIGHT),MPFROM2SHORT(CMDSRC_OTHER,FALSE));
    break;

  case WM_BUTTON1CLICK: {
    SHORT iItemNum;

    if (!plswData->Settings.bOldPopup &&
        (iItemNum=ItemNumFromPos(plswData,SHORT1FROMMP(mp1),SHORT2FROMMP(mp1)))>=0) {
      plswData->usCurrItem=iItemNum;
      WinInvalidateRect(hwnd,NULL,FALSE);
      DosSleep(50);
      ProcessCommand(CMD_SWITCHFROMPM,CMDSRC_OTHER,plswData, FALSE,FALSE);
      mrc=(MRESULT)TRUE;
    }
    break;
  }

  case WM_BUTTON2CLICK:
    if (!plswData->Settings.bOldPopup &&
        (plswData->iMenuAtItem=
         ItemNumFromPos(plswData,SHORT1FROMMP(mp1),SHORT2FROMMP(mp1)))>=0) {
      ShowMenu(plswData, plswData->iMenuAtItem,FALSE,FALSE);
      ShowBubble(plswData,0,0,0,-1,NULL);

      mrc=(MRESULT)TRUE;
    }
    break;

  case WM_CHAR: {
    USHORT usFlags;
    UCHAR ucScan;

    usFlags = SHORT1FROMMP(mp1); ucScan = CHAR4FROMMP(mp1);
    if (plswData->bNowActive && (usFlags & KC_SCANCODE) && (usFlags & KC_KEYUP)) {

      if ((plswData->Settings.ucPopupHotKey==0 && (ucScan==SCAN_CTRL || ucScan==SCAN_RIGHTCTRL)) ||
          (plswData->Settings.ucPopupHotKey==1 && (ucScan==SCAN_ALT || ucScan==SCAN_RIGHTALT)) ||
          (plswData->Settings.ucPopupHotKey==2 && (ucScan==SCAN_CTRL || ucScan==SCAN_RIGHTCTRL ||
                                                   ucScan==SCAN_ALT || ucScan==SCAN_RIGHTALT))) {
        bNowSticky=TRUE;
        WinSetAccelTable(plswData->hab,plswData->haccNoAlt, plswData->hwndPopup);
        break;
      }

      // left alt, ctrl or winkey release, switch to the selected window
      if (ucScan==((plswData->Settings.bStickyPopup||bNowSticky)?SCAN_ENTER:
                   plswData->Settings.ucPopupHotKey==0?SCAN_ALT:
                   plswData->Settings.ucPopupHotKey==1?SCAN_CTRL:SCAN_WINLEFT))
        ProcessCommand(CMD_SWITCHFROMPM,CMDSRC_OTHER,plswData, FALSE,FALSE);
    }
    break;
  }

  case WM_COMMAND:
    switch (SHORT1FROMMP(mp1)) {
    case CMD_SCROLLLEFT:
    case CMD_SCROLLRIGHT:
    case CMD_SCROLLUP:
    case CMD_SCROLLDOWN:
    case CMD_SCROLLLEFTFROMFS:
    case CMD_SCROLLRIGHTFROMFS: {
      USHORT usPrevSel, usMaxIcons, usPosX, usPosY;
      RECTL rcl,rcl1,rcl2;
      BOOL bUpdateAll,bFullScreenInit,bForward,bNeedScroll;
      UCHAR ucBuf[2*NAMELEN];
      SHORT iCurrItem;

      bFullScreenInit = SHORT1FROMMP(mp1)==CMD_SCROLLLEFTFROMFS ||
                        SHORT1FROMMP(mp1)==CMD_SCROLLRIGHTFROMFS;

      if (!plswData->bNowActive) {
        InitTaskArr(plswData,bFullScreenInit,FALSE,TRUE);
        if (plswData->usItems == 0) break;
      }

      usMaxIcons = MAXICONSINLINE*ICONLINES;

      usPrevSel = plswData->usCurrItem;
      bForward = SHORT1FROMMP(mp1)==CMD_SCROLLRIGHT ||
                 SHORT1FROMMP(mp1)==CMD_SCROLLRIGHTFROMFS ||
                 SHORT1FROMMP(mp1)==CMD_SCROLLDOWN;
      bNeedScroll = FALSE;

      switch SHORT1FROMMP(mp1) {
      case CMD_SCROLLLEFT:
      case CMD_SCROLLLEFTFROMFS:  iCurrItem = usPrevSel - 1; break;
      case CMD_SCROLLRIGHT:
      case CMD_SCROLLRIGHTFROMFS: iCurrItem = usPrevSel + 1; break;
      case CMD_SCROLLUP:
      case CMD_SCROLLDOWN: {
        SHORT iRow;
        USHORT usRows,usCol,usIconNum;

        usIconNum = IconNumFromItemNum(plswData,usPrevSel);
        usRows = min(ICONLINES,(plswData->usItems-1)/MAXICONSINLINE+1);
        if (usRows == 1) { iCurrItem = usPrevSel; break; }
        iRow = usIconNum/MAXICONSINLINE;
        usCol = usIconNum-iRow*MAXICONSINLINE;

        if (plswData->usItems <= usMaxIcons) {
          if (SHORT1FROMMP(mp1) == CMD_SCROLLUP)
            if (iRow == 0) iRow = usRows-1; else iRow--;
          else
            if (iRow == usRows-1) iRow = 0; else iRow++;
          iCurrItem = usPrevSel+min(iRow*MAXICONSINLINE+usCol,plswData->usItems-1)-
                      usIconNum;
        } else {
          iCurrItem = usPrevSel +
            (SHORT1FROMMP(mp1) == CMD_SCROLLUP ? -MAXICONSINLINE : MAXICONSINLINE);
          bNeedScroll = (iRow==ICONLINES-1 && bForward) || (iRow==0 && !bForward);
        }
        break;
      }
      }

      AdjustItem(iCurrItem,plswData->usItems);
      plswData->usCurrItem = iCurrItem;

      /* check if the number of items is larger than fits in the window and we are
         at a window end so we need to scroll */
      if (!plswData->Settings.bScrollItems && plswData->usItems>usMaxIcons &&
          (plswData->usCurrItem ==
           ItemNumFromIconNum(plswData,bForward ? usMaxIcons : -1) ||
           bNeedScroll)) {
        plswData->iShift += (SHORT)(bForward ? MAXICONSINLINE : -MAXICONSINLINE);
        plswData->iShift -= plswData->iShift/max((SHORT)plswData->usItems,1)*plswData->usItems;
        bUpdateAll = TRUE;
      } else
        bUpdateAll = FALSE;

      if (!plswData->bNowActive) {
        // this must be just before adjusting the window, otherwise if it's after,
        // the subclassed frame window proc won't let us do repositioning;
        // if it's much before, the WM_TIMER might have time to hide the window
        plswData->bNowActive = TRUE; bNowSticky = FALSE;

        if (bFullScreenInit)  WinSwitchToProgram(plswData->hswitch);

        AdjustPopupWin(plswData,TRUE);

      } else {
        if (plswData->Settings.bScrollItems || plswData->Settings.bOldPopup ||
            bUpdateAll) {
          WinQueryWindowRect(hwnd,&rcl);
          if (plswData->Settings.bOldPopup) {
            WinSetRect(plswData->hab,&rcl,1,1,rcl.xRight-2,rcl.yTop-2);
          } else {
            WinQueryWindowRect(hwndHints,&rcl1);
            WinSetRect(plswData->hab,&rcl,1,1+rcl1.yTop,rcl.xRight-2,rcl.yTop-2);
          }
        } else {
          IconPosFromNumber(plswData, IconNumFromItemNum(plswData,usPrevSel),
                            &usPosX,&usPosY);
          WinSetRect(plswData->hab,&rcl1, usPosX-cxIcon*SELBOXOFFSET-SELBOXLINEWIDTH,
                     usPosY-cyIcon*SELBOXOFFSET-SELBOXLINEWIDTH,
                     usPosX+cxIcon+cxIcon*SELBOXOFFSET+SELBOXLINEWIDTH,
                     usPosY+cyIcon+cyIcon*SELBOXOFFSET+SELBOXLINEWIDTH);
          IconPosFromNumber(plswData, IconNumFromItemNum(plswData,plswData->usCurrItem),
                            &usPosX,&usPosY);
          WinSetRect(plswData->hab,&rcl2, usPosX-cxIcon*SELBOXOFFSET-SELBOXLINEWIDTH,
                     usPosY-cyIcon*SELBOXOFFSET-SELBOXLINEWIDTH,
                     usPosX+cxIcon+cxIcon*SELBOXOFFSET+SELBOXLINEWIDTH,
                     usPosY+cyIcon+cyIcon*SELBOXOFFSET+SELBOXLINEWIDTH);
          WinUnionRect(plswData->hab,&rcl,&rcl1,&rcl2);
        }

        WinInvalidateRect(hwnd,&rcl,FALSE);

        GetItemTitle(plswData->TaskArr[plswData->usCurrItem].hsw,ucBuf,sizeof(ucBuf),FALSE);
        WinSetWindowText(hwndTitle,ucBuf);

        MakeHintStr(plswData,plswData->usCurrItem,ucBuf,sizeof(ucBuf));
        WinSetWindowText(hwndHints,ucBuf);
      }
      break;
    }

    default:
      if (plswData->bNowActive &&
          (SHORT1FROMMP(mp1)==CMD_SWITCHFROMPM || SHORT1FROMMP(mp1)==CMD_CANCELPOPUP || SHORT1FROMMP(mp1)==CMD_CANCELPOPUP1)) {
        UCHAR ucBuf[128];

        usScrollCnt=0;
        WinQueryPresParam(hwndTitle,PP_FONTNAMESIZE,0,NULL,sizeof(ucBuf),ucBuf,QPF_NOINHERIT);
        strncpy(plswData->Settings.ucTitleFont,ucBuf,sizeof(plswData->Settings.ucTitleFont));
        WinQueryPresParam(hwndHints,PP_FONTNAMESIZE,0,NULL,sizeof(ucBuf),ucBuf,QPF_NOINHERIT);
        strncpy(plswData->Settings.ucHintsFont,ucBuf,sizeof(plswData->Settings.ucHintsFont));
        WinQueryPresParam(hwndTitle,PP_FOREGROUNDCOLOR,0,NULL,sizeof(ucBuf),ucBuf,QPF_NOINHERIT);
        plswData->Settings.lTitleRGBCol = ucBuf[0]+ucBuf[1]*256+ucBuf[2]*65536;
        WinQueryPresParam(hwndHints,PP_FOREGROUNDCOLOR,0,NULL,sizeof(ucBuf),ucBuf,QPF_NOINHERIT);
        plswData->Settings.lHintsRGBCol = ucBuf[0]+ucBuf[1]*256+ucBuf[2]*65536;
      }
      ProcessCommand(SHORT1FROMMP(mp1),SHORT1FROMMP(mp2),plswData, FALSE,FALSE);
    }

    break;

  case WM_CREATE: {
    DosGetNamedSharedMem((PVOID*)&plswData,SHAREMEM_NAME,PAG_READ | PAG_WRITE);

    plswData->hwndMenu = WinCreateMenu(HWND_DESKTOP, NULL);
    WinSetPresParam(plswData->hwndMenu,PP_FONTNAMESIZE,7,"8.Helv");
    hwndTitle = WinCreateWindow(hwnd,WC_STATIC,"",
                                WS_VISIBLE | SS_TEXT | DT_CENTER | DT_VCENTER,
                                0,0,0,0,hwnd,HWND_TOP,ID_TITLESTR,NULL,NULL);
    hwndHints = WinCreateWindow(hwnd,WC_STATIC,"",
                                WS_VISIBLE | SS_TEXT | DT_CENTER | DT_VCENTER,
                                0,0,0,0,hwnd,HWND_TOP,ID_HINTSTR,NULL,NULL);
    WinSetPresParam(hwndHints,PP_FONTNAMESIZE,strlen(plswData->Settings.ucHintsFont)+1,plswData->Settings.ucHintsFont);

#ifndef XWORKPLACE
    WinSetWindowPtr(hwnd, 0, plswData);
#else
    WinSetWindowPtr(hwnd, 0, plswData->pWidget);
#endif
    DosFreeMem(plswData);
  }

  case WM_SYSVALUECHANGED:
    cxIcon = WinQuerySysValue(HWND_DESKTOP, SV_CXICON);
    cyIcon = WinQuerySysValue(HWND_DESKTOP, SV_CYICON);
    cyPointer=WinQuerySysValue(HWND_DESKTOP, SV_CYPOINTER);
    break;

  case WM_MENUEND:
    WinSetWindowPos(plswData->hwndMenu,HWND_BOTTOM,0,0,0,0,SWP_ZORDER);
    if (plswData->bNowActive) WinSetFocus(HWND_DESKTOP,hwnd);
    break;

  case WM_ERASEBACKGROUND:
    mrc = (MRESULT)FALSE;
    break;

  case WM_PAINT: {
    HPS hps;
    RECTL rclUpdate, rclWnd, rclSelBox;
    USHORT k,usPosX,usPosY,usItemNum,usBorderSize,usR,usB,usG;

    hps = WinBeginPaint(hwnd,NULLHANDLE,&rclUpdate);
    WinQueryWindowRect(hwnd, &rclWnd);
    usBorderSize=max(1,(rclWnd.xRight-rclWnd.xLeft)/(POPUPBORDERSIZER+1));

    GpiCreateLogColorTable(hps, 0, LCOLF_RGB, 0, 0, NULL);

    WinFillRect(hps,&rclUpdate,plswData->Settings.lBackColor);

    usR=(plswData->Settings.lBackColor&0xFF0000)>>16,
    usG=(plswData->Settings.lBackColor&0xFF00)>>8,
    usB=(plswData->Settings.lBackColor&0xFF);

    for (k=1;k<=usBorderSize;k++) {
      ULONG ulColor;

      ulColor=(230-usB)/usBorderSize*(/*usBorderSize-*/k)+usB+
        ((230-usG)/usBorderSize*(/*usBorderSize-*/k)+usG)*256+
        ((230-usR)/usBorderSize*(/*usBorderSize-*/k)+usR)*65536;
      WinDrawBorder(hps,&rclWnd,1,1,ulColor,ulColor,0);
      rclWnd.xLeft+=1; rclWnd.xRight-=1; rclWnd.yBottom+=1; rclWnd.yTop-=1;
    }

    if (plswData->Settings.bOldPopup) {
      WinDrawPointer(hps,rclWnd.xLeft+cxIcon*ICONSPACINGX,
                     (rclWnd.yTop+rclWnd.yBottom-cyIcon)/2,
                     plswData->TaskArr[plswData->usCurrItem].hIcon,DP_NORMAL);
    } else {
      /* Draw icons */
      for (k = 0; k < min(MAXICONSINLINE*ICONLINES,plswData->usItems); k++) {

        IconPosFromNumber(plswData,k,&usPosX,&usPosY);
        usItemNum = ItemNumFromIconNum(plswData,k);

        WinDrawPointer(hps, usPosX, usPosY, plswData->TaskArr[usItemNum].hIcon,DP_NORMAL|
                       (plswData->TaskArr[usItemNum].fl&(SWP_HIDE|SWP_MINIMIZE))?DP_HALFTONED:0);

/* Draw selection box */
        if (usItemNum == plswData->usCurrItem) {
          WinSetRect(plswData->hab,&rclSelBox,
            usPosX-cxIcon*SELBOXOFFSET-1, usPosY-cyIcon*SELBOXOFFSET-1,
            usPosX+cxIcon*SELBOXOFFSET+cxIcon+1,usPosY+cyIcon*SELBOXOFFSET+cyIcon+1);
          WinDrawBorder(hps,&rclSelBox,SELBOXLINEWIDTH,SELBOXLINEWIDTH,
                        plswData->Settings.lSunkenFrameColor,
                        plswData->Settings.lRaisedFrameColor,DB_DEPRESSED);
        }
      }

    }
    WinEndPaint(hps);

    break;
  }
/*
  case WM_USER+101: {
    HPS hpsScreen;
    USHORT usShadowSize;
    POINTL ptl;
    SWP swp;

    if (msg==WM_USER+101)
      WinSendMsg(HWNDFROMMP(mp1),WM_NULL,0,0);
    WinQueryWindowPos(plswData->hwndPopup,&swp);
    hpsScreen=WinGetScreenPS(HWND_DESKTOP);
    GpiCreateLogColorTable(hpsScreen, 0, LCOLF_RGB, 0, 0, NULL);
    usShadowSize=swp.cx/(SHADOWSIZER+1);
    GpiSetColor(hpsScreen,0x00555555);
    GpiSetMix(hpsScreen,FM_AND);
    ptl.x=swp.x+swp.cx; ptl.y=swp.y;
    GpiSetCurrentPosition(hpsScreen,&ptl);
    ptl.x+=usShadowSize; ptl.y+=swp.cy-usShadowSize;
    GpiBox(hpsScreen,DRO_FILL,&ptl,0,0);
    WinReleasePS(hpsScreen);
    break;
  }
*/
  case LSWM_WNDSTATECHANGED:
//clear the list of minimized/hidden windows (reset the function)
    if (SHORT2FROMMP(mp1) & (SWP_RESTORE|SWP_MAXIMIZE|SWP_SHOW))
      MinimizeHideAll(plswData, TRUE, HWNDFROMMP(mp2));

  case LSWM_WNDICONCHANGED: {
    USHORT k,usPosX,usPosY;
    RECTL rcl;
    UCHAR ucBuf[150];
    BOOL bNeedUpdate;

    if (!plswData->bNowActive) break;

    for (k=0;k<plswData->usItems;k++)
      if (plswData->TaskArr[k].hwnd==HWNDFROMMP(mp2)) {
        if (msg==LSWM_WNDICONCHANGED) {

          plswData->TaskArr[k].hIcon=LONGFROMMP(mp1);
          WinSendMsg(HWNDFROMMP(mp2),WM_NULL,0,0); // to make sure the icon handle has become valid
          bNeedUpdate = TRUE;
        } else {
          bNeedUpdate = UpdateWinFlags(&plswData->TaskArr[k].fl, SHORT2FROMMP(mp1));

          if (plswData->TaskArr[MAXITEMS-1].hwnd == plswData->TaskArr[k].hwnd)
            plswData->TaskArr[MAXITEMS-1].fl = plswData->TaskArr[k].fl;

          if (k==plswData->usCurrItem && bNeedUpdate) {
            MakeHintStr(plswData,k,ucBuf,sizeof(ucBuf)); //change hints to reflect the changed task state
            WinSetWindowText(hwndHints,ucBuf);
          }

          if (IsWindowClass(WinWindowFromID(HWNDFROMMP(mp2),FID_CLIENT),"SeamlessClass")) {//dynamic WIn-OS2 icon handles change
            DosSleep(20); //there is some delay in icon change
            plswData->TaskArr[k].hIcon=GetWndIcon(HWNDFROMMP(mp2));
            bNeedUpdate = TRUE;
          }
        }

        if (bNeedUpdate) {
          IconPosFromNumber(plswData, IconNumFromItemNum(plswData,k), &usPosX,&usPosY);
          WinSetRect(plswData->hab,&rcl,usPosX,usPosY,usPosX+cxIcon,usPosY+cyIcon);
          WinInvalidateRect(hwnd,&rcl,FALSE);
        }

        break;
      }
    break;
  }
  case LSWM_SWITCHLISTCHANGED: {
    USHORT k;
    UCHAR ucBuf[NAMELEN];

    if (!plswData->bNowActive) break;

    for (k=0;k<plswData->usItems;k++) {
      if (plswData->TaskArr[k].hsw!=LONGFROMMP(mp2)) continue;

      if (LONGFROMMP(mp1)==2) ShowBubble(plswData,k,0,0,11,NULL); //update bubble

      if (LONGFROMMP(mp1)==2 && k==plswData->usCurrItem) {
        GetItemTitle(plswData->TaskArr[k].hsw,ucBuf,sizeof(ucBuf),FALSE);
        WinSetWindowText(hwndTitle,ucBuf);
      }
      if ((LONGFROMMP(mp1)==2 &&
           (GetItemTitle(plswData->TaskArr[k].hsw,ucBuf,sizeof(ucBuf),FALSE),
            IsInSkipList(&plswData->Settings.SkipListPopup,ucBuf))) ||
          LONGFROMMP(mp1)==3) {
        memmove(&plswData->TaskArr[k],&plswData->TaskArr[k+1],sizeof(SWITEM)*(plswData->usItems-k-1));
        plswData->usItems--;
        if (plswData->usItems==0)
          ProcessCommand(CMD_CANCELPOPUP1,CMDSRC_OTHER,plswData, FALSE,FALSE);//cancel popup and don't return to original session
        else if (WinQueryFocus(HWND_DESKTOP)==plswData->hwndPopClient && !plswData->Settings.bOldPopup)
          AdjustPopupWin(plswData,FALSE);

        break;
      }
    }
    break;

  }

  case WM_PRESPARAMCHANGED: {
    LONG lParam, lValue;
    UCHAR ucBuf[64];
    lParam = LONGFROMMP(mp1);

    if (!(lParam==PP_FOREGROUNDCOLOR || lParam==PP_BACKGROUNDCOLOR) ||
        (LONG)WinQueryPresParam(hwnd,lParam,0,NULL,sizeof(ucBuf),ucBuf,QPF_NOINHERIT)==0)
      break;

    lValue = ucBuf[0]+ucBuf[1]*256+ucBuf[2]*65536;

    switch(lParam) {
    case PP_BACKGROUNDCOLOR:
      plswData->Settings.lBackColor = plswData->Settings.lTitleRGBCol =
        plswData->Settings.lHintsRGBCol = lValue;
      WinSetPresParam(hwndTitle,PP_BACKGROUNDCOLOR,4,&lValue);
      WinSetPresParam(hwndHints,PP_BACKGROUNDCOLOR,4,&lValue);
      WinInvalidateRect(hwnd,NULL,TRUE);
      break;
    }
    break;
  }

  case WM_TIMER: {
    SHORT iMouseIsAtItem;
    POINTL ptl;
    SWP swp1,swp2;
    HWND hwndFocus;

    if (plswData->bNowActive) {
      if (WinQueryFocus(HWND_DESKTOP) == plswData->hwndPopClient &&
          WinQueryWindow(HWND_DESKTOP,QW_TOP) != plswData->hwndPopup)
        WinSetWindowPos(plswData->hwndPopup,WinIsWindowVisible(plswData->hwndBubble)?
                        plswData->hwndBubble:HWND_TOP,0,0,0,0,SWP_ZORDER);

      hwndFocus = WinQueryFocus(HWND_DESKTOP);
      if (hwndFocus != plswData->hwndPopClient && hwndFocus != plswData->hwndMenu &&
          WinQueryWindow(hwndFocus,QW_OWNER) != plswData->hwndMenu)
        ProcessCommand(CMD_CANCELPOPUP1,CMDSRC_OTHER,plswData, FALSE,FALSE);//cancel popup and don't return to original session
      else if (!(plswData->Settings.bStickyPopup||bNowSticky) &&
               (WinGetPhysKeyState(HWND_DESKTOP,(plswData->Settings.ucPopupHotKey==0?SCAN_ALT:
                                             plswData->Settings.ucPopupHotKey==1?SCAN_CTRL:
                                             SCAN_WINLEFT)) & 0x8000)==0)
        ProcessCommand(CMD_SWITCHFROMPM,CMDSRC_OTHER,plswData, FALSE,FALSE);
    }

/* check if mouse pointer is inside the popup window and above an icon */
    if (plswData->bNowActive && (WinQueryPointerPos(HWND_DESKTOP,&ptl),
        WinQueryWindowPos(plswData->hwndPopup,&swp1),WinQueryWindowPos(hwnd,&swp2),
        (ptl.x>=swp1.x+swp2.x && ptl.y>=swp1.y+swp2.y &&
        ptl.x<=swp1.x+swp2.x+swp2.cx && ptl.y<=swp1.y+swp2.y+swp2.cy)) &&
        (iMouseIsAtItem=
        ItemNumFromPos(plswData,ptl.x-swp1.x-swp2.x,ptl.y-swp1.y-swp2.y))>=0 &&
        !WinIsWindowVisible(plswData->hwndMenu))

      ShowBubble(plswData,iMouseIsAtItem,ptl.x,ptl.y-cyPointer-1,1,NULL);
    else
      ShowBubble(plswData,0,0,0,-1,NULL);

    break;
  }
  default:
    mrc = WinDefWindowProc(hwnd, msg, mp1, mp2);
  }
  return mrc;
}

