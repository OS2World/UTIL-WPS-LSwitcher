/*
 *      Copyright (C) 1997-2007 Andrei Los.
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
#include <stdlib.h>
#include <ctype.h>
#include "lswitch.h"
#include "common.h"
#include "taskbar.h"
#include "settings.h"
#include "apm.h"
#include "lswres.h"
#include "object.h"


#define PROCITEMHGT 6/5

#ifdef XWORKPLACE
// resolved function pointers from XFLDR.DLL
PKRNSENDDAEMONMSG pkrnSendDaemonMsg = NULL;
#endif

SHORT TaskArrItemFromHsw(LSWDATA *plswData,USHORT hsw,BOOL bUseFilters)
{ SHORT iItem,k;

  if (!plswData->bNowActive) InitTaskArr(plswData,FALSE,TRUE,bUseFilters);

  for(k=0,iItem=-1;k<plswData->usItems && iItem<0;k++)
    if ((plswData->TaskArr[k].hsw & 0xFFFF)==hsw) iItem=k;

  return iItem;
}

// Modified from John Martin Alfredsson's wxTask XCenter widget
void FillTaskMenu(HAB hab, HWND hwndMenu, SHORT TskBarHgt, BOOL bProcesses)
{ ULONG ulItemCount,k,l;
  SHORT sMenuItemNum,sItemId;
  MENUITEM mi;

  sMenuItemNum = MenuItemCount(hwndMenu);
  while (sMenuItemNum > 0) {
    sItemId = SHORT1FROMMR(WinSendMsg(hwndMenu,MM_ITEMIDFROMPOSITION,MPFROMSHORT(sMenuItemNum-1),0));
    WinSendMsg(hwndMenu, MM_QUERYITEM, MPFROM2SHORT(sItemId, FALSE), MPFROMP(&mi));
    if (mi.afStyle&MIS_PROCESS) { free(((PROCDATA*)mi.hItem)->pszTitle); free((PROCDATA*)mi.hItem);}
    sMenuItemNum = SHORT1FROMMR(WinSendMsg(hwndMenu,MM_DELETEITEM,MPFROM2SHORT(sItemId,FALSE),0));
  }

  if (bProcesses) {
    static UCHAR szPidPath[_MAX_PATH], szName[_MAX_FNAME], szExt[_MAX_EXT];
    UCHAR ucIndent[64],*pBuf;
    QSPTRREC *pRecHead;
    QSPREC *pApp;
    PID pidPrev=0,pidParent=0;
    HSWITCH hsw;
    SWCNTRL swctl;
    PROCDATA *pProcData;
    POINTL aptl[TXTBOX_COUNT];
    HPS hps;
    USHORT usItemStyle;

    pBuf = (char*) malloc(0x8000); memset(pBuf,0,0x8000);
    DosQuerySysState(QS_PROCESS, 0, 0, 0, (char *) pBuf, 0x8000); // Get processes
    pRecHead = (QSPTRREC *)pBuf; // Point to first process
    pApp = pRecHead->pProcRec;
    for (k=0,pidPrev=0,ucIndent[0]='\0';pApp!=NULL && pApp->RecType == 1;k++) { // While its a process record
      if (pApp->ppid==pidPrev && pApp->ppid!=0) {
        if (strlen(ucIndent)==0)
          strncat(ucIndent,"юд",sizeof(ucIndent)-strlen(ucIndent));
        else {
          strrev(ucIndent); strncat(ucIndent,"    ",sizeof(ucIndent)-strlen(ucIndent)); strrev(ucIndent);
        }
        pidParent=pidPrev;
      } else if (pApp->ppid!=pidParent) {
        for (l=1;l<k;l++) {
          WinSendMsg(hwndMenu, MM_QUERYITEM, MPFROM2SHORT(101 + k-l, FALSE), MPFROMP(&mi));
          if (HIUSHORT(mi.hItem)==pApp->ppid) {
            pidParent=pApp->ppid;
            WinSendMsg(hwndMenu,MM_QUERYITEMTEXT,MPFROM2SHORT(101 + k-l,sizeof(ucIndent)),MPFROMP(ucIndent));
            ucIndent[strspn(ucIndent,"юд ")]='\0';
            break;
          }
        }
        if (l==k) ucIndent[0]='\0';
      }
      pidPrev=pApp->pid;

      memset(szPidPath, 0, _MAX_PATH); memset(&swctl,0,sizeof(swctl));
      DosQueryModuleName(pApp->hMte, sizeof(szPidPath), szPidPath);
      hsw = WinQuerySwitchHandle(0,pApp->pid);
      if (hsw != NULLHANDLE)
        WinQuerySwitchEntry(hsw,&swctl);
      if (strlen(szPidPath) == 0) // If no hit this is the kernel
        sprintf(szPidPath, "%s*SYSINIT  %s (0x%X)",ucIndent,swctl.idProcess==pApp->pid?swctl.szSwtitle:"",pApp->pid);
      else {
        _splitpath(szPidPath, NULL, NULL, szName, szExt);
        sprintf(szPidPath,"%s%s%s  %s (0x%X)",ucIndent,szName,szExt,swctl.idProcess==pApp->pid?swctl.szSwtitle:"",pApp->pid);
      }
      pProcData=(PROCDATA*)malloc(sizeof(PROCDATA));
      pProcData->pszTitle=strdup(szPidPath);
      pProcData->pid=pApp->pid; pProcData->ppid=pApp->ppid;

      usItemStyle=MIS_OWNERDRAW|MIS_SYSCOMMAND|MIS_PROCESS;

      hps=WinGetPS(hwndMenu);
      GpiQueryTextBox(hps,1,"M",TXTBOX_COUNT,aptl);
      WinReleasePS(hps);

      if (k>0 && k%((WinQuerySysValue(HWND_DESKTOP,SV_CYSCREEN)-TskBarHgt)/
                    max(1,(aptl[TXTBOX_TOPLEFT].y-aptl[TXTBOX_BOTTOMLEFT].y)*PROCITEMHGT))==0)
        usItemStyle|=MIS_BREAKSEPARATOR;

      InsertMenuItem(hwndMenu, NULLHANDLE, MIT_END, 101 + k, "", usItemStyle, 0,(ULONG)pProcData);
      pApp=(QSPREC *)((pApp->pThrdRec) + pApp->cTCB);
    }
    free(pBuf);
  }  else  {
    PSWBLOCK pSwb;

    if ((pSwb=GetSwitchList(hab,TRUE,&ulItemCount))==NULL) return;
    for (k = 0; k < ulItemCount; k++ ) {
      if (pSwb->aswentry[k].swctl.uchVisibility == SWL_VISIBLE &&
          pSwb->aswentry[k].swctl.fbJump==SWL_JUMPABLE)
        InsertMenuItem(hwndMenu, NULLHANDLE, MIT_END, pSwb->aswentry[k].hswitch, "",
                       MIS_OWNERDRAW|MIS_SYSCOMMAND, 0,GetWndIcon(pSwb->aswentry[k].swctl.hwnd));
    }
    GetSwitchList(hab,FALSE,NULL);
  }
}

VOID DrawMenuItem(POWNERITEM pItem, LSWDATA *plswData,BOOL bProcesses)
{ SWCNTRL swctl;
  SWP swp={0};
  UCHAR Title[_MAX_PATH];
  BOOL bHilite;
  LONG lBgndCol, lTextCol, lTskBarCol=plswData->Settings.lTskBarRGBCol;

  if (!bProcesses) {
    WinQuerySwitchEntry(pItem->idItem,&swctl);
    WinQueryWindowPos(swctl.hwnd,&swp);
  }

  GpiCreateLogColorTable(pItem->hps, 0, LCOLF_RGB, 0, 0, NULL);
  bHilite = (pItem->fsAttribute & MIA_HILITED) != 0;

  pItem->rclItem.xLeft+=1; pItem->rclItem.xRight-=1;

  if (!bHilite) {
    if (plswData->Settings.b3DTaskBar)
      lTextCol = (swp.fl&(SWP_HIDE|SWP_MINIMIZE))?
        plswData->Settings.lNormalHiddenBtnTitleRGBCol3D:plswData->Settings.lNormalBtnTitleRGBCol3D;
    else
      lTextCol = (swp.fl&(SWP_HIDE|SWP_MINIMIZE))?
        plswData->Settings.lNormalHiddenBtnTitleRGBCol:plswData->Settings.lNormalBtnTitleRGBCol;

    if (plswData->Settings.b3DTaskBar)
      lBgndCol = ((lTskBarCol&0xFF)+240)/2+(((lTskBarCol&0xFF00)>>8)+240)/2*256+(((lTskBarCol&0xFF0000)>>16)+240)/2*65536;
    else
      lBgndCol = plswData->Settings.lNormalBtnRGBCol;

    WinFillRect(pItem->hps, &pItem->rclItem, lBgndCol);
  } else {
    if (swp.fl&(SWP_HIDE|SWP_MINIMIZE)) {
      lTextCol = plswData->Settings.b3DTaskBar?plswData->Settings.lActiveHiddenBtnTitleRGBCol3D:plswData->Settings.lActiveHiddenBtnTitleRGBCol;
      lBgndCol = plswData->Settings.b3DTaskBar?plswData->Settings.lActiveHiddenBtnRGBCol3D:plswData->Settings.lActiveHiddenBtnRGBCol;
    } else {
      lTextCol = plswData->Settings.b3DTaskBar?plswData->Settings.lActiveBtnTitleRGBCol3D:plswData->Settings.lActiveBtnTitleRGBCol;
      lBgndCol = plswData->Settings.b3DTaskBar?plswData->Settings.lActiveBtnRGBCol3D:plswData->Settings.lActiveBtnRGBCol;
    }

    if (plswData->Settings.b3DTaskBar)
      FillRectGradient(pItem->hps, &pItem->rclItem, NULL, lBgndCol, RGB_WHITE, TRUE);
    else
      WinFillRect(pItem->hps, &pItem->rclItem, lBgndCol);

    WinDrawBorder(pItem->hps,&pItem->rclItem,BUTTONBORDERWIDTH,BUTTONBORDERWIDTH,
                  SUNKENFRAMERGBCOL,RAISEDFRAMERGBCOL,DB_DEPRESSED);
  }

  if (!bProcesses)
    WinDrawPointer(pItem->hps,pItem->rclItem.xLeft+1+BUTTONBORDERWIDTH+bHilite,
                   pItem->rclItem.yBottom+BUTTONBORDERWIDTH+2-bHilite,
                   pItem->hItem, DP_MINIICON);

// adjust the rect for text drawing
// shifting the text and icon of a pressed button by one pixel looks better
// than shifting by button border width
  pItem->rclItem.xLeft+=(bProcesses?2:pItem->rclItem.yTop-pItem->rclItem.yBottom)+bHilite+1;
  pItem->rclItem.xRight-=BUTTONBORDERWIDTH+2;
  pItem->rclItem.yTop-=bHilite;
  pItem->rclItem.yBottom-=bHilite;

  if (bProcesses && pItem->hItem!=0)
    strncpy(Title,((PROCDATA*)pItem->hItem)->pszTitle,sizeof(Title));
  else
    GetItemTitle(pItem->idItem,Title,sizeof(Title),FALSE);
  WinDrawText(pItem->hps,strlen(Title),Title,&pItem->rclItem,lTextCol,lBgndCol, DT_VCENTER);
}

SHORT GetItemIdFromPointerPos(HWND hwndMenu,POINTL ptlPointer)
{ SHORT sId,k;
  HAB hab = WinQueryAnchorBlock(hwndMenu);
  RECTL rectl;

  for(k=0;k<MenuItemCount(hwndMenu);k++) {
    if ((sId = SHORT1FROMMR(WinSendMsg(hwndMenu,MM_ITEMIDFROMPOSITION,MPFROMSHORT(k),0)))==MIT_NONE)
      continue;

    WinSendMsg(hwndMenu,MM_QUERYITEMRECT,MPFROM2SHORT(sId,FALSE),MPFROMP(&rectl));
    if (WinPtInRect(hab,&rectl,&ptlPointer))
      return sId;
  }
  return MIT_NONE;
}

MRESULT EXPENTRY ButtonMenuWndProc(HWND hwnd,ULONG msg,MPARAM mp1,MPARAM mp2)
{ MRESULT mrc=0;
  static SHORT sClickId=0;

  switch (msg) {
  case WM_CHAR:
  case WM_ADJUSTWINDOWPOS:
    if ((msg==WM_ADJUSTWINDOWPOS && !(((PSWP)PVOIDFROMMP(mp1))->fl & SWP_HIDE))||
        (msg==WM_CHAR && !(SHORT1FROMMP(mp1)&KC_VIRTUALKEY &&
                           (SHORT2FROMMP(mp2)==VK_UP || SHORT2FROMMP(mp2)==VK_DOWN)))) {
      mrc = ((PFNWP)WinQueryWindowPtr(hwnd,0))(hwnd,msg,mp1,mp2);
      break;
    }
  case MM_SELECTITEM: {
    SHORT k,sId;

    for(k=0;k<MenuItemCount(hwnd);k++) {
      MENUITEM mi;

      if ((sId = SHORT1FROMMR(WinSendMsg(hwnd,MM_ITEMIDFROMPOSITION,MPFROMSHORT(k),0)))==MIT_NONE ||
          (msg==MM_SELECTITEM && sId==SHORT1FROMMP(mp1))) continue;
      WinSendMsg(hwnd,MM_QUERYITEM,MPFROM2SHORT(sId,FALSE),MPFROMP(&mi));
      if (mi.afStyle & MIS_SUBMENU) {
        WinShowWindow(mi.hwndSubMenu,FALSE);
        WinSetParent(mi.hwndSubMenu,HWND_DESKTOP,FALSE);
        mi.afStyle&=~MIS_SUBMENU;  mi.hwndSubMenu=0;
        WinSendMsg(hwnd,MM_SETITEM,MPFROM2SHORT(0,FALSE),MPFROMP(&mi));
      }
    }
    mrc = ((PFNWP)WinQueryWindowPtr(hwnd,0))(hwnd,msg,mp1,mp2);
    break;
  }

  case WM_CONTEXTMENU: {
    SHORT sId,sSelId;
    POINTL ptl;
    LSWDATA *plswData;
    MENUITEM mi;

    ptl.x=SHORT1FROMMP(mp1); ptl.y=SHORT2FROMMP(mp1);
    if ((sId=GetItemIdFromPointerPos(hwnd,ptl))==MIT_NONE) break;

    sSelId=SHORT1FROMMR(WinSendMsg(hwnd,MM_QUERYSELITEMID,0,0));
    WinSendMsg(hwnd,MM_QUERYITEM,MPFROM2SHORT(sSelId,FALSE),MPFROMP(&mi));

    if (mi.afStyle & MIS_SUBMENU) {
      if (sId==sSelId)
        break;
      else {
        WinShowWindow(mi.hwndSubMenu,FALSE);
        mi.afStyle&=(~MIS_SUBMENU); mi.hwndSubMenu=0;
        WinSendMsg(hwnd,MM_SETITEM,MPFROM2SHORT(0,FALSE),MPFROMP(&mi));
      }
    }

    if (DosGetNamedSharedMem((PVOID*)&plswData,SHAREMEM_NAME,PAG_READ | PAG_WRITE)!=0) break;
    if (!plswData->bNowActive && (plswData->iMenuAtItem=TaskArrItemFromHsw(plswData,sId,FALSE))>=0) {
      if (MenuItemCount(plswData->hwndMenu)==0) {
        //without these commands the task submenu won't show for some reason if it's a first popup;
        //apparently WinPopopMenu does some initialization to the menu window
        WinPopupMenu(HWND_DESKTOP, hwnd,plswData->hwndMenu,0,0,0,0);
        WinShowWindow(plswData->hwndMenu,FALSE);
      }

      InitTaskActionsMenu(plswData->hwndMenu,plswData, plswData->iMenuAtItem,TRUE,FALSE);
      WinSendMsg(hwnd,MM_SELECTITEM,MPFROM2SHORT(sId,TRUE),MPFROM2SHORT(0,FALSE));
      WinSendMsg(hwnd,MM_QUERYITEM,MPFROM2SHORT(sId,FALSE),MPFROMP(&mi));
      mi.afStyle|=MIS_SUBMENU;
      mi.hwndSubMenu=plswData->hwndMenu;
      WinSendMsg(hwnd,MM_SETITEM,MPFROM2SHORT(0,FALSE),MPFROMP(&mi));
      WinPostMsg(hwnd,WM_CHAR,MPFROM2SHORT(KC_VIRTUALKEY,0),MPFROM2SHORT(0,VK_RIGHT));
    }
    DosFreeMem(plswData);
    break;
  }

  case WM_BUTTON1UP:
  case WM_BUTTON2UP:
  case WM_BUTTON3UP:
  case WM_CHORD: {
    POINTL ptl;
    ptl.x=SHORT1FROMMP(mp1); ptl.y=SHORT2FROMMP(mp1);
    sClickId=GetItemIdFromPointerPos(hwnd,ptl);
  }
  case WM_BUTTON1CLICK:
  case WM_BUTTON2CLICK:
  case WM_BUTTON3CLICK:
    WinSendMsg(WinQueryWindow(hwnd,QW_OWNER),(msg/*+0x3a1*/)|0x1000,
               mp1,MPFROM2SHORT(sClickId,SHORT2FROMMP(mp2)));

  default:
    mrc = ((PFNWP)WinQueryWindowPtr(hwnd,0))(hwnd,msg,mp1,mp2);
  }

  return mrc;
}

MRESULT EXPENTRY ButtonWndProc(HWND hwnd,ULONG msg,MPARAM mp1,MPARAM mp2)
{ LSWDATA *plswData;
  MRESULT mrc=0;
  static BOOL bChord = FALSE;
  static ULONG ulBtnDnTime=0;


  if (msg!=WM_DESTROY)
#ifdef XWORKPLACE
    plswData=((PXCENTERWIDGET)WinQueryWindowPtr(WinQueryWindow(hwnd,QW_PARENT),0))->pUser;
#else
    plswData=WinQueryWindowPtr(WinQueryWindow(hwnd,QW_PARENT),0);
#endif

  switch (LOUSHORT(msg)) {
  case WM_MEASUREITEM: {
    UCHAR Title[_MAX_PATH];
    POINTL aptl[TXTBOX_COUNT];
    POWNERITEM pItem=((POWNERITEM)mp2);
    MENUITEM mi;
    BOOL bMenuProcMode;

    WinSendMsg(plswData->hwndTaskMenu, MM_QUERYITEM, MPFROM2SHORT(pItem->idItem, FALSE), MPFROMP(&mi));
    bMenuProcMode=(mi.afStyle&MIS_PROCESS);

    if (!bMenuProcMode) GetItemTitle(pItem->idItem,Title,sizeof(Title),FALSE);
    GpiQueryTextBox(pItem->hps,strlen(bMenuProcMode?((PROCDATA*)pItem->hItem)->pszTitle:Title),
                    bMenuProcMode?((PROCDATA*)pItem->hItem)->pszTitle:Title,TXTBOX_COUNT,aptl);
    pItem->rclItem.yTop = (bMenuProcMode?(aptl[TXTBOX_TOPLEFT].y-aptl[TXTBOX_BOTTOMLEFT].y)*PROCITEMHGT:
                           WinQuerySysValue(HWND_DESKTOP,SV_CYICON)/2+2*BUTTONBORDERWIDTH+2);
    pItem->rclItem.xRight = aptl[TXTBOX_BOTTOMRIGHT].x-aptl[TXTBOX_BOTTOMLEFT].x+
                           (bMenuProcMode?0:pItem->rclItem.yTop);

    mrc = (MRESULT)pItem->rclItem.yTop;

    break;
  }

  case WM_DRAWITEM: {
    POWNERITEM pItem=((POWNERITEM)mp2);
    MENUITEM mi;
    BOOL bMenuProcMode;

    WinSendMsg(plswData->hwndTaskMenu, MM_QUERYITEM, MPFROM2SHORT(pItem->idItem, FALSE), MPFROMP(&mi));
    bMenuProcMode=(mi.afStyle&MIS_PROCESS);

    if (pItem->fsAttribute == pItem->fsAttributeOld ||
        ((pItem->fsAttribute & MIA_HILITED) != (pItem->fsAttributeOld & MIA_HILITED))) {
      DrawMenuItem(pItem,plswData,bMenuProcMode);
      pItem->fsAttributeOld = (pItem->fsAttribute &= ~MIA_HILITED);
      mrc = (MRESULT)TRUE;
    }

    break;
  }

  case WM_COMMAND:
    if (SHORT1FROMMP(mp1)==CMD_ADDFILTER ||
               (SHORT1FROMMP(mp1)>=CMD_FIRST && SHORT1FROMMP(mp1)<=CMD_LAST))
      ProcessCommand(SHORT1FROMMP(mp1),SHORT1FROMMP(mp2),plswData, TRUE, FALSE);
#ifdef XWORKPLACE
    else
      WinPostMsg(plswData->pWidget->hwndWidget,msg,mp1,mp2);
#endif

    break;

  case WM_SYSCOMMAND: {
    MENUITEM mi;
    BOOL bMenuProcMode;

    if (SHORT1FROMMP(mp2)!=CMDSRC_MENU) break;

    WinSendMsg(plswData->hwndTaskMenu, MM_QUERYITEM, MPFROM2SHORT(SHORT1FROMMP(mp1), FALSE), MPFROMP(&mi));
    bMenuProcMode=(mi.afStyle&MIS_PROCESS);

    if (!bMenuProcMode) {
      SWCNTRL swctl;
      WinQuerySwitchEntry(SHORT1FROMMP(mp1),&swctl);
      WinSetWindowPos(swctl.hwnd,HWND_TOP,0,0,0,0,SWP_SHOW|SWP_ZORDER);
      WinSwitchToProgram(SHORT1FROMMP(mp1));
    } else {
      UCHAR ucBuf[256],ucCancel[32];
      PID pid;
      PSZ pszName;

      pid=((PROCDATA*)mi.hItem)->pid;
      pszName=((PROCDATA*)mi.hItem)->pszTitle;

      if (pid==1 || IsWPSPid(pid)) {
        WinLoadString(plswData->hab,plswData->hmodRes,MSG_CANTKILL,sizeof(ucBuf),ucBuf);
        WinMessageBox(HWND_DESKTOP,HWND_DESKTOP,ucBuf,PGMNAME,0,MB_ERROR | MB_CANCEL);
      } else {
        ULONG ulResult,k,ulInfoSize;
        UCHAR ucCancel[MAX_MBDTEXT];
        MB2INFO *pmbInfo;
        BOOL bDeathBtn=XF86Installed();
        MB2D mb2dBut[3]={{"DosKillProcess",501,BS_PUSHBUTTON},{"SIGKILL",502,BS_PUSHBUTTON},
        {"",503, BS_PUSHBUTTON|BS_DEFAULT}};

        k=strspn(pszName,"юд ");
        memmove(pszName,&pszName[k],strlen(pszName)-k+1);
        WinLoadString(plswData->hab,plswData->hmodRes,MSG_KILLQUERY,sizeof(ucBuf),ucBuf);
        WinLoadString(plswData->hab,plswData->hmodRes,MSG_CANCEL,sizeof(ucCancel),ucCancel);
        strcpy(mb2dBut[2].achText,ucCancel);
        sprintf(&ucBuf[strlen(ucBuf)]," %s?",pszName);
        // Need space for MB2INFO plus 3 additional buttons (one button is defined in the MB2INFO structure).
        ulInfoSize=sizeof(MB2INFO) + sizeof(MB2D) * (1+bDeathBtn);
        pmbInfo = malloc (ulInfoSize);
        pmbInfo->cb         = ulInfoSize;        /* Size of block              */
        pmbInfo->hIcon      = NULLHANDLE;
        pmbInfo->cButtons   = 2+bDeathBtn;           /* Number of buttons for box  */
        pmbInfo->flStyle    = MB_ICONQUESTION|MB_MOVEABLE;
        pmbInfo->hwndNotify = NULLHANDLE;
        for (k = 0; k < 3; k+=(bDeathBtn?1:2))
          memcpy(pmbInfo->mb2d+k-(k==2&&!bDeathBtn), mb2dBut+k , sizeof(MB2D));

        ulResult = WinMessageBox2(HWND_DESKTOP, HWND_DESKTOP, ucBuf,PGMNAME,0,pmbInfo);
        free(pmbInfo);

        if (ulResult==501)
          DosKillProcess(DKP_PROCESS,pid);
        else if (ulResult==502)
          Death(pid);
      }
    }
    break;
  }

  case WM_CHORD|0x1000:
  case WM_CHORD:    //for some reason if either mouse button is down, sending
    bChord = TRUE;  //WM_SYSCOMMAND does not have the effect, so need to do it the stupid way
    if (msg==WM_CHORD)
      mrc = (((BTNDATA*)WinQueryWindowPtr(hwnd,0))->pOldWndProc)(hwnd,msg,mp1,mp2);
    break;

  case WM_BUTTON1DOWN:
    ulBtnDnTime=WinQueryMsgTime(plswData->hab);
    break;
  case WM_BUTTON1DBLCLK:
  case WM_BUTTON1CLICK:
  case WM_BUTTON2CLICK:
  case WM_BUTTON3CLICK:
  case WM_BUTTON1CLICK|0x1000:
  case WM_BUTTON2CLICK|0x1000:
  case WM_BUTTON3CLICK|0x1000:
  case WM_BUTTON1UP:
  case WM_BUTTON2UP:
  case WM_BUTTON1UP|0x1000:
  case WM_BUTTON2UP|0x1000: {
    BOOL bShiftKeyDown,bMenuCmd=(msg&0x1000);
    SHORT sItem;
    USHORT usHswActive,usHsw;

    bShiftKeyDown = ((WinGetKeyState(HWND_DESKTOP,VK_CTRL)&0x8000) ||
                     (WinGetKeyState(HWND_DESKTOP,VK_ALT)&0x8000) ||
                     (WinGetKeyState(HWND_DESKTOP,VK_ALTGRAF)&0x8000) ||
                     (WinGetKeyState(HWND_DESKTOP,VK_SHIFT)&0x8000));

    usHswActive = (USHORT)WinSendMsg(plswData->hwndTaskBarClient,LSWM_QUERYACTIVEBTNID,0,0);
    //    usActiveBtn=(USHORT)WinSendMsg(plswData->hwndTaskBarClient,LSWM_QUERYACTIVEBTN,MPFROMHWND(hwnd),0);
    usHsw = bMenuCmd?SHORT1FROMMP(mp2):WinQueryWindowUShort(hwnd,QWS_ID);
    sItem = TaskArrItemFromHsw(plswData,usHsw,TRUE);
    plswData->usCurrItem = sItem;

    if (!bShiftKeyDown && bChord &&
        (((msg&0xFFF)==WM_BUTTON1UP && !(WinGetKeyState(HWND_DESKTOP,VK_BUTTON2)&0x8000)) ||
         ((msg&0xFFF)==WM_BUTTON2UP && !(WinGetKeyState(HWND_DESKTOP,VK_BUTTON1)&0x8000)))) {
      msg=WM_CHORD;
      bChord=FALSE;
    }

    // Show task menu
#ifdef XWORKPLACE
    plswData->Settings.bTaskBarTopScr=(plswData->pWidget->pGlobals->ulPosition==XCENTER_TOP);
#endif
    if (((msg==WM_BUTTON1UP && WinQueryMsgTime(plswData->hab)-ulBtnDnTime>WinQuerySysValue(HWND_DESKTOP,SV_DBLCLKTIME) && !bShiftKeyDown)||
        (msg==WM_BUTTON1CLICK && ((SHORT2FROMMP(mp2)&KC_SHIFT)^(SHORT2FROMMP(mp2)&KC_CTRL)))) &&
        IsDesktop(plswData->TaskArr[sItem].hwnd)) {
      SWP swp;
      POINTL ptl;
      SHORT sItems;
      USHORT usItemHgt,usFrameWid;
      LONG lMenuCol;
      BOOL bMenuProcMode;

      bMenuProcMode=(SHORT2FROMMP(mp2)&KC_CTRL);
      FillTaskMenu(plswData->hab, plswData->hwndTaskMenu,plswData->Settings.sTskBarCY,bMenuProcMode);
      WinQueryWindowPos(hwnd,&swp);
      ptl.x=swp.x; ptl.y=swp.y;
      if (plswData->Settings.bTaskBarTopScr) {
        if (bMenuProcMode) {
          POINTL aptl[TXTBOX_COUNT];
          HPS hps=WinGetPS(plswData->hwndTaskMenu);
          GpiQueryTextBox(hps,1,"M",TXTBOX_COUNT,aptl);
          usItemHgt = (aptl[TXTBOX_TOPLEFT].y-aptl[TXTBOX_BOTTOMLEFT].y)*6/5;
          WinReleasePS(hps);
        } else
          usItemHgt=WinQuerySysValue(HWND_DESKTOP,SV_CYICON)/2+2*BUTTONBORDERWIDTH+2;

        sItems=MenuItemCount(plswData->hwndTaskMenu);
        usFrameWid=WinQuerySysValue(HWND_DESKTOP,SV_CYDLGFRAME);
      }
      WinMapWindowPoints(WinQueryWindow(hwnd,QW_PARENT),HWND_DESKTOP,&ptl,1);
      if (plswData->Settings.b3DTaskBar) {
        LONG lTskBarCol = plswData->Settings.lTskBarRGBCol;
        lMenuCol = ((lTskBarCol&0xFF)+240)/2+(((lTskBarCol&0xFF00)>>8)+240)/2*256+(((lTskBarCol&0xFF0000)>>16)+240)/2*65536;
      } else
        lMenuCol = plswData->Settings.lNormalBtnRGBCol;
      WinSetPresParam(plswData->hwndTaskMenu,PP_MENUBACKGROUNDCOLOR, sizeof(lMenuCol), &lMenuCol);
      WinPopupMenu(HWND_DESKTOP, hwnd, plswData->hwndTaskMenu, ptl.x,
                   ptl.y+(plswData->Settings.bTaskBarTopScr?-sItems*usItemHgt-2*usFrameWid:swp.cy),
                   usHswActive, PU_HCONSTRAIN | PU_VCONSTRAIN | PU_MOUSEBUTTON1 | PU_KEYBOARD |
                   ((SHORT)WinSendMsg(plswData->hwndTaskMenu,MM_ITEMPOSITIONFROMID,MPFROM2SHORT(usHswActive,0),0)!=MIT_NONE?
                  PU_SELECTITEM:0));
    } else if (!bShiftKeyDown && msg!=WM_BUTTON1UP && msg!=WM_BUTTON2UP && sItem>=0) {
      HWND hwndSubWinMenu = ((BTNDATA*)WinQueryWindowPtr(hwnd,0))->hwndSubWinMenu;
      USHORT usMenuItems=MenuItemCount(hwndSubWinMenu);
      SWP swp;

      if (msg==plswData->Settings.usSwitchMEvent && usMenuItems>0) {
        POINTL ptl;
        USHORT usItemHgt,usFrameWid;

        WinQueryWindowPos(hwnd,&swp);
        ptl.x=swp.x; ptl.y=swp.y;
        if (plswData->Settings.bTaskBarTopScr) {
          usItemHgt=WinQuerySysValue(HWND_DESKTOP,SV_CYICON)/2+2*BUTTONBORDERWIDTH+2;
          usFrameWid=WinQuerySysValue(HWND_DESKTOP,SV_CYDLGFRAME);
        }
        WinMapWindowPoints(WinQueryWindow(hwnd,QW_PARENT),HWND_DESKTOP,&ptl,1);
        WinPopupMenu(HWND_DESKTOP, hwnd, hwndSubWinMenu, ptl.x,
                     ptl.y+(plswData->Settings.bTaskBarTopScr?-usMenuItems*usItemHgt-2*usFrameWid:swp.cy),
                     usHswActive, PU_NONE| PU_HCONSTRAIN | PU_VCONSTRAIN | PU_MOUSEBUTTON1 | PU_KEYBOARD |
                     ((SHORT)WinSendMsg(hwndSubWinMenu,MM_ITEMPOSITIONFROMID,MPFROM2SHORT(usHswActive,0),0)!=MIT_NONE?
                    PU_SELECTITEM:0));
      } else if ((msg&0xFFF)==plswData->Settings.usSwitchMEvent &&
                 (MenuNeedsItem(plswData,sItem,CMD_SWITCHFROMPM,NULL,0,!bMenuCmd && usMenuItems>0) ||
                  IsDesktop(plswData->TaskArr[sItem].hwnd))) {
        ProcessCommand(CMD_SWITCHFROMPM, CMDSRC_OTHER, plswData, TRUE,FALSE);
      } else if (msg==WM_BUTTON1DBLCLK && MenuNeedsItem(plswData,sItem,CMD_SWITCHFROMPM,NULL,0,FALSE)) {
        ProcessCommand(CMD_SWITCHFROMPM, CMDSRC_OTHER, plswData, TRUE,FALSE);
      } else if ((msg&0xFFF)==plswData->Settings.usMinMEvent) {
        if (MenuNeedsItem(plswData,sItem,CMD_HIDE,NULL,0,!bMenuCmd && usMenuItems>0))
          ChangeWindowPos(plswData,sItem,CMD_HIDE);
        else if (MenuNeedsItem(plswData,sItem,CMD_MINIMIZE,NULL,0,!bMenuCmd && usMenuItems>0))
          ChangeWindowPos(plswData,sItem,CMD_MINIMIZE);
      } else if (msg==plswData->Settings.usCloseMEvent) {
        ProcessCommand(CMD_CLOSE, CMDSRC_OTHER, plswData, TRUE,!bMenuCmd && usMenuItems>0);
      }
    }

    mrc = (((BTNDATA*)WinQueryWindowPtr(hwnd,0))->pOldWndProc)(hwnd,msg,mp1,mp2);
    break;
  }

  case WM_CONTEXTMENU:
    if (!plswData->bNowActive &&
        (plswData->iMenuAtItem=
         TaskArrItemFromHsw(plswData,WinQueryWindowUShort(hwnd,QWS_ID),TRUE))>=0) {
      ShowBubble(plswData,0,0,0,-2,NULL);
      ShowMenu(plswData, IsDesktop(plswData->TaskArr[plswData->iMenuAtItem].hwnd) ?
               -1 : plswData->iMenuAtItem,TRUE,
               MenuItemCount(((BTNDATA*)WinQueryWindowPtr(hwnd,0))->hwndSubWinMenu)>0);
    }
    break;

  case WM_PRESPARAMCHANGED: {
    UCHAR ucBuf[64];
    LONG lParam, lValue;
    BOOL bActiveBtn, bHidden;
    BTNDATA *pBtnData;

    lParam = LONGFROMMP(mp1);

    if (!(lParam==PP_FOREGROUNDCOLOR || lParam==PP_BACKGROUNDCOLOR || lParam==PP_FONTNAMESIZE) ||
        (LONG)WinQueryPresParam(hwnd,lParam,0,NULL,sizeof(ucBuf),ucBuf,QPF_NOINHERIT)==0)
      break;

    lValue = ucBuf[0]+ucBuf[1]*256+ucBuf[2]*65536;
    bActiveBtn = ((USHORT)WinSendMsg(WinQueryWindow(hwnd,QW_PARENT),LSWM_QUERYACTIVEBTNID,0,0)==
                  WinQueryWindowUShort(hwnd,QWS_ID));
    pBtnData = WinQueryWindowPtr(hwnd,0);
    bHidden = (pBtnData->flItemWnd&(SWP_HIDE|SWP_MINIMIZE));

    switch(lParam) {
    case PP_FOREGROUNDCOLOR:
      if (bActiveBtn)
        if (bHidden)
          if (plswData->Settings.b3DTaskBar)
            plswData->Settings.lActiveHiddenBtnTitleRGBCol3D = lValue;
          else
            plswData->Settings.lActiveHiddenBtnTitleRGBCol = lValue;
        else
          if (plswData->Settings.b3DTaskBar)
            plswData->Settings.lActiveBtnTitleRGBCol3D = lValue;
          else
            plswData->Settings.lActiveBtnTitleRGBCol = lValue;
      else
        if (bHidden)
          if (plswData->Settings.b3DTaskBar)
            plswData->Settings.lNormalHiddenBtnTitleRGBCol3D = lValue;
          else
            plswData->Settings.lNormalHiddenBtnTitleRGBCol = lValue;
        else
          if (plswData->Settings.b3DTaskBar)
            plswData->Settings.lNormalBtnTitleRGBCol3D = lValue;
          else
            plswData->Settings.lNormalBtnTitleRGBCol = lValue;
      break;

    case PP_BACKGROUNDCOLOR:
      if (bActiveBtn)
        if (bHidden)
          if (plswData->Settings.b3DTaskBar)
            plswData->Settings.lActiveHiddenBtnRGBCol3D = lValue;
          else
            plswData->Settings.lActiveHiddenBtnRGBCol = lValue;
        else
          if (plswData->Settings.b3DTaskBar)
            plswData->Settings.lActiveBtnRGBCol3D = lValue;
          else
            plswData->Settings.lActiveBtnRGBCol = lValue;
      else
        if (bHidden)
          if (plswData->Settings.b3DTaskBar)
            plswData->Settings.lNormalHiddenBtnRGBCol3D = lValue;
          else
            plswData->Settings.lNormalHiddenBtnRGBCol = lValue;
        else
          if (plswData->Settings.b3DTaskBar)
            plswData->Settings.lNormalBtnRGBCol3D = lValue;
          else
            plswData->Settings.lNormalBtnRGBCol = lValue;
      break;

    case PP_FONTNAMESIZE: {
      HENUM henum;
      HWND hwndNext;

      if (strcmp(plswData->Settings.ucButtonFont,ucBuf)==0) break; //we don't want infinite recursion

      strncpy(plswData->Settings.ucButtonFont,ucBuf,sizeof(plswData->Settings.ucButtonFont));

      henum = WinBeginEnumWindows(WinQueryWindow(hwnd,QW_PARENT));
      while ((hwndNext = WinGetNextWindow(henum))!=NULLHANDLE) {
        if (IsWindowClass(hwndNext,"#3") && hwndNext!=hwnd)
          WinSetPresParam(hwndNext,PP_FONTNAMESIZE,strlen(plswData->Settings.ucButtonFont)+1,plswData->Settings.ucButtonFont);
      }
      WinEndEnumWindows(henum);
      break;
    }

    }

    if (bActiveBtn)
      WinInvalidateRect(hwnd,NULL,FALSE);
    else
      WinInvalidateRect(WinQueryWindow(hwnd,QW_PARENT),NULL,TRUE);

    break;
  }

  default:
    if (!(WinGetKeyState(HWND_DESKTOP,VK_BUTTON1)&0x8000) &&
        !(WinGetKeyState(HWND_DESKTOP,VK_BUTTON2)&0x8000))
      bChord = FALSE;

    mrc = (((BTNDATA*)WinQueryWindowPtr(hwnd,0))->pOldWndProc)(hwnd,msg,mp1,mp2);
    break;
  }

  return mrc;
}

MRESULT EXPENTRY TskBarFrameWndProc(HWND hwnd,ULONG msg,MPARAM mp1,MPARAM mp2)
{ LSWDATA *plswData;

  if (msg!=WM_CREATE)
    plswData = WinQueryWindowPtr(WinWindowFromID(hwnd,FID_CLIENT),0);

  switch (msg) {
  case WM_WINDOWPOSCHANGED: {
    PSWP pswp = ((PSWP)PVOIDFROMMP(mp1));

    if (pswp->fl & (SWP_SIZE|SWP_MOVE)) {
      if (!IsInDesktop(plswData->hab,hwnd,plswData->lCurrDesktop)) {
        WinSetWindowPos(hwnd,0,plswData->Settings.sTskBarX,plswData->Settings.sTskBarY,0,0,SWP_MOVE);
      } else {
        plswData->Settings.sTskBarX=pswp->x; plswData->Settings.sTskBarY=pswp->y;
        plswData->Settings.sTskBarCX=pswp->cx; plswData->Settings.sTskBarCY=pswp->cy;
      }
    }
    break;
  }

  case WM_ADJUSTWINDOWPOS: {
    SWP *pswp=PVOIDFROMMP(mp1);
    if (pswp->fl & SWP_ACTIVATE) pswp->fl &= ~SWP_ACTIVATE;
    break;
  }

  }

  return ((PFNWP)WinQueryWindowPtr(hwnd,0))(hwnd,msg,mp1,mp2);
}

BOOL IsDesktopBtn(HWND hwndBtn)
{ SWCNTRL swctl;

  WinQuerySwitchEntry(WinQueryWindowUShort(hwndBtn,QWS_ID),&swctl);
  return IsDesktop(swctl.hwnd);
}

HWND GetBtnFromHsw(HWND hwndTaskBar,HSWITCH hsw)
{ HWND hwndNext,hwndFound=0;
  HENUM henum = WinBeginEnumWindows(hwndTaskBar);
  BTNDATA *pbData;

//look in submenus first
  while ((hwndNext=WinGetNextWindow(henum))!=NULLHANDLE && hwndFound==0)
    if (IsWindowClass(hwndNext,"#3") && (pbData = WinQueryWindowPtr(hwndNext,0))!=NULL &&
        (SHORT)WinSendMsg(pbData->hwndSubWinMenu,MM_ITEMPOSITIONFROMID,MPFROM2SHORT(hsw&0xFFFF,FALSE),0)!=MIT_NONE)
      hwndFound = pbData->hwndSubWinMenu;
  WinEndEnumWindows(henum);
  if (!hwndFound) hwndFound = WinWindowFromID(hwndTaskBar,hsw&0xFFFF);
  return hwndFound;
}

HWND GetBtnFromHmte(HWND hwndTaskBar, SWCNTRL swctl)
{ SWCNTRL swctl1stItem;
  HWND hwndNext,hwndBtnSameHmte=0;
  HENUM henum = WinBeginEnumWindows(hwndTaskBar);
  BTNDATA *pbData;

  while ((hwndNext=WinGetNextWindow(henum))!=NULLHANDLE && hwndBtnSameHmte==0)
    if (IsWindowClass(hwndNext,"#3") && (pbData = WinQueryWindowPtr(hwndNext,0))!=NULL &&
        pbData->hMte==HmteFromPID(swctl.idProcess) && (!IsWPSPid(swctl.idProcess) || (!IsDesktopBtn(hwndNext))) &&
        (WinQuerySwitchEntry(WinQuerySwitchHandle(pbData->hwndFirstItem,0),&swctl1stItem),
         swctl.bProgType==swctl1stItem.bProgType) )
      hwndBtnSameHmte=hwndNext;
  WinEndEnumWindows(henum);

  return hwndBtnSameHmte;
}

VOID UpdateTaskMenu(LSWDATA *plswData, HSWITCH hsw, SWCNTRL swctl, ULONG ulFunc)
{ MENUITEM mi;
  ENTRYNAME Title;
  BOOL bExists, bHidden;

  if (!WinIsWindowVisible(plswData->hwndTaskMenu) || MenuItemCount(plswData->hwndTaskMenu)==0 ||
      (WinSendMsg(plswData->hwndTaskMenu,MM_QUERYITEM,
                  MPFROM2SHORT((SHORT)WinSendMsg(plswData->hwndTaskMenu,MM_ITEMIDFROMPOSITION,MPFROMSHORT(0),0),0),
                  MPFROMP(&mi)),
       mi.afStyle&MIS_PROCESS))
    return;

  bExists=((SHORT)WinSendMsg(plswData->hwndTaskMenu,MM_ITEMPOSITIONFROMID,MPFROM2SHORT(hsw,0),0)!=MIT_NONE);
  bHidden=(swctl.uchVisibility != SWL_VISIBLE || swctl.fbJump!=SWL_JUMPABLE);

  if (((ulFunc&1) || ulFunc==2) && !bExists && !bHidden) {
    InsertMenuItem(plswData->hwndTaskMenu, NULLHANDLE, 0, hsw, "",
                   MIS_OWNERDRAW|MIS_SYSCOMMAND, 0,GetWndIcon(swctl.hwnd));
  } else if (ulFunc==3 || (ulFunc==2 && bHidden)) {
    WinSendMsg(plswData->hwndTaskMenu,MM_DELETEITEM,MPFROM2SHORT(hsw,FALSE),0);
  } else if (ulFunc==2 && bExists) {
    GetItemTitle(hsw,Title,sizeof(Title),FALSE);
    WinSendMsg(plswData->hwndTaskMenu,MM_SETITEMTEXT,MPFROMSHORT(hsw),MPFROMP(Title));
  }
}

VOID UpdateTaskbarItem(LSWDATA *plswData, USHORT usMinBtnWid, HSWITCH hswItem, HWND hwndItem)
{ RECTL rcl;
  SHORT sItem;
  ENTRYNAME Title;

  if (IsWindowClass(hwndItem,"#3")) { //the item is the button itself
// update button text
    WinQueryWindowRect(hwndItem,&rcl);
    rcl.xLeft = usMinBtnWid;
    WinInvalidateRect(hwndItem,&rcl,FALSE);
// update bubble text
    if (!plswData->bNowActive && (sItem=TaskArrItemFromHsw(plswData,hswItem,TRUE))>=0)
      ShowBubble(plswData,sItem,0,0,11,NULL);
  } else { //the item is in the button submenu
    GetItemTitle(hswItem,Title,sizeof(Title),FALSE);
    WinSendMsg(hwndItem,MM_SETITEMTEXT,MPFROMSHORT(hswItem),MPFROMP(Title));
  }
}

VOID RemoveTaskbarItem(LSWDATA *plswData, HSWITCH hswItem, HWND hwndItem)
{ BTNDATA *pbData;
  ENTRYNAME Title;

  if (IsWindowClass(hwndItem,"#3")) {
    pbData=WinQueryWindowPtr(hwndItem,0);
    WinDestroyWindow(hwndItem);
    WinDestroyWindow(pbData->hwndSubWinMenu);
    if (pbData->pszObjName!=NULL) free(pbData->pszObjName);
    if (pbData->pszDataFName!=NULL) free(pbData->pszDataFName);
    if (pbData->hObjIcon!=0) WinDestroyPointer(pbData->hObjIcon);
    free(pbData);
    //          WinInvalidateRect(hwnd,NULL,FALSE);
    WinPostMsg(plswData->hwndTaskBarClient,WM_SIZE,0,0);
  } else {
    HWND hwndBtn=WinQueryWindow(hwndItem,QW_OWNER);
    SHORT sItemCount;

    pbData=WinQueryWindowPtr(hwndBtn,0);
    WinSendMsg(pbData->hwndSubWinMenu,MM_DELETEITEM,MPFROM2SHORT(hswItem,FALSE),0);
    sItemCount=MenuItemCount(hwndItem);
    if (sItemCount==1 || (hswItem & 0xFFFF)==WinQueryWindowUShort(hwndBtn,QWS_ID)) {// last menu item or button item
      SWCNTRL swctlItem;
      SWP swpItem;
      BOOL bIconOnly;
      SHORT sId=(SHORT)WinSendMsg(pbData->hwndSubWinMenu,MM_ITEMIDFROMPOSITION,0,0);

      WinSetWindowUShort(hwndBtn,QWS_ID,sId);
      WinQuerySwitchEntry(sId,&swctlItem);
      WinQueryWindowPos(swctlItem.hwnd,&swpItem);
      pbData->flItemWnd = swpItem.fl;
      pbData->hwndFirstItem=swctlItem.hwnd;
      pbData->hMte=HmteFromPID(swctlItem.idProcess);
      if (sItemCount==1) {
        pbData->hIcon = GetWndIcon(swctlItem.hwnd);
        WinSendMsg(pbData->hwndSubWinMenu,MM_DELETEITEM,MPFROM2SHORT(sId,FALSE),0);
        if (GetItemTitle(sId,Title,sizeof(Title),FALSE),
            bIconOnly=IsInSkipList(&plswData->Settings.ListIconsOnly,Title),
            pbData->bIconOnly!=bIconOnly) {
          pbData->bIconOnly=bIconOnly;
          WinPostMsg(plswData->hwndTaskBarClient,WM_SIZE,0,0);
          return;
        }
      }
    }
    WinInvalidateRect(hwndBtn,NULL,FALSE);
  }
}

VOID InitButtonGroup(BTNDATA *pbData, SWCNTRL swctl, PSZ pszCmdLine)
{
  if (pbData->hMte==0 && (swctl.bProgType==PROG_VDM || swctl.bProgType==PROG_WINDOWEDVDM))
    pbData->pszObjName=strdup("VDM");
  else if (IsWPSPid(swctl.idProcess))
    pbData->pszObjName=strdup("WPS");
  else {
    PCHAR pszModuleName=malloc(MAX_PATH+1), pszObjName=malloc(MAX_PATH+1);

    if (DosQueryModuleName(pbData->hMte,MAX_PATH,pszModuleName)!=0)
      pbData->usObjHandle=0xFFFF;
    else { //no errors
      USHORT k;
      UCHAR fname[_MAX_FNAME];

      if ((pbData->usObjHandle=ObjIDFromName(pszModuleName,pszCmdLine,swctl.bProgType,TRUE))==0 ||
          !GetObjectTitle(pbData->usObjHandle, pszObjName,MAX_PATH)) { //could not get object ID or title -- use filename
        _splitpath(pszModuleName,NULL,NULL,fname,NULL);
        strupr(fname);strlwr(&fname[1]);
        pbData->pszObjName=strdup(fname);
      } else {
        for (k=0;pszModuleName[k]!='\0';k++) //get rid of tildas in object title
          if (pszModuleName[k]=='~')
            memmove(&pszObjName[k],&pszObjName[k+1],strlen(&pszObjName[k+1])+1);
        pbData->pszObjName=strdup(pszObjName);
        pbData->hObjIcon=GetObjectIcon(pbData->usObjHandle|0x10000,NULL);
      }
    }
    free(pszModuleName); free(pszObjName);
  }
}

VOID CreateTaskbarItem(LSWDATA *plswData, HWND hwndBtnGroup, BTNDATA *pbDataGroup,
                       HSWITCH hswItem, SWCNTRL swctlItem, SWP swpItem)
{
  if (hwndBtnGroup && pbDataGroup->usObjHandle!=0xFFFF) {//0xFFFF errors in getting object title
    BOOL bIconOnly=IsInSkipList(&plswData->Settings.ListIconsOnly,pbDataGroup->pszObjName);
    if (MenuItemCount(pbDataGroup->hwndSubWinMenu)==0) {
      InsertMenuItem(pbDataGroup->hwndSubWinMenu,NULL,MIT_END,
                     WinQueryWindowUShort(hwndBtnGroup,QWS_ID),"",MIS_OWNERDRAW,0,
                     pbDataGroup->hIcon);
      if (IsWPSPid(swctlItem.idProcess)) {
        HLIB hlib;
        if ((hlib = WinLoadLibrary(plswData->hab, "PMWP.DLL"))!=NULLHANDLE) {
          pbDataGroup->hIcon = WinLoadPointer(HWND_DESKTOP, hlib, 26);
          WinDeleteLibrary(plswData->hab, hlib);
        }
      } else if (pbDataGroup->hObjIcon!=0)
        pbDataGroup->hIcon=pbDataGroup->hObjIcon;
    }
    InsertMenuItem(pbDataGroup->hwndSubWinMenu,NULL,MIT_END,hswItem&0xFFFF,"",MIS_OWNERDRAW,0,GetWndIcon(swctlItem.hwnd));
    if (pbDataGroup->bIconOnly!=bIconOnly) {
      pbDataGroup->bIconOnly=bIconOnly;
      WinPostMsg(plswData->hwndTaskBarClient,WM_SIZE,0,0);
    } else
      WinInvalidateRect(hwndBtnGroup,NULL,FALSE);
  } else {
    BTNDATA *pbData1;
    ENTRYNAME Title;
    HWND hwndBtn;

    hwndBtn=WinCreateWindow(plswData->hwndTaskBarClient,WC_BUTTON,"",
                            WS_VISIBLE|BS_USERBUTTON|BS_NOPOINTERFOCUS,0,0,0,0,
                            plswData->hwndTaskBarClient,
                            IsDesktop(swctlItem.hwnd)?HWND_TOP:HWND_BOTTOM,
                            hswItem&0xFFFF, NULL,NULL);
    if (hwndBtn==NULLHANDLE) return;
    if ((pbData1=calloc(1,sizeof(BTNDATA)))!=NULL) {
      GetItemTitle(hswItem,Title,sizeof(Title),FALSE);
      pbData1->bIconOnly=IsInSkipList(&plswData->Settings.ListIconsOnly,Title);
      pbData1->hIcon = GetWndIcon(swctlItem.hwnd);
      pbData1->flItemWnd = swpItem.fl;
      pbData1->hwndFirstItem=swctlItem.hwnd;
      pbData1->hwndSubWinMenu=WinCreateMenu(HWND_DESKTOP, NULL);
      WinSetWindowPtr(pbData1->hwndSubWinMenu,0,(VOID*)WinSubclassWindow(pbData1->hwndSubWinMenu,ButtonMenuWndProc));
      WinSetOwner(pbData1->hwndSubWinMenu,hwndBtn);
      pbData1->hMte=HmteFromPID(swctlItem.idProcess);
      WinSetWindowPtr(hwndBtn,0,(VOID *)pbData1);
      pbData1->pOldWndProc=WinSubclassWindow(hwndBtn,ButtonWndProc);
      WinSetPresParam(hwndBtn,PP_FONTNAMESIZE,strlen(plswData->Settings.ucButtonFont)+1,plswData->Settings.ucButtonFont);
      WinPostMsg(plswData->hwndTaskBarClient,WM_SIZE,0,0);
    } else
      WinDestroyWindow(hwndBtn);
  }
}

MRESULT EXPENTRY TaskBarWndProc(HWND hwnd,ULONG msg,MPARAM mp1,MPARAM mp2)
{ LSWDATA *plswData;
  static USHORT usMinBtnWid,usBorderWid,usHswActive=0;
  static LONG cyScreen;
  static HWND hwndMouseOver=0;
  static ATOM atomGetProcCmd;
  MRESULT mrc=0;

  if (msg!=WM_CREATE)
#ifdef XWORKPLACE
    plswData = ((PXCENTERWIDGET)WinQueryWindowPtr(hwnd,0))->pUser;
#else
    plswData = WinQueryWindowPtr(hwnd,0);
#endif

  switch (msg) {
  case LSWM_QUERYACTIVEBTNID: {
    mrc = (MPARAM)usHswActive;
    break;
  }

  case LSWM_WNDSTATECHANGED: {
    HWND hwndFound;
    BTNDATA *pbtnData;

    if (WinIsWindow(plswData->hab,HWNDFROMMP(mp2)) &&
        (hwndFound = GetBtnFromHsw(hwnd,SHORT1FROMMP(mp1)))!=NULLHANDLE) {
      if (IsWindowClass(hwndFound,"#3") && (pbtnData = WinQueryWindowPtr(hwndFound,0))!=NULL &&
         UpdateWinFlags(&pbtnData->flItemWnd,SHORT2FROMMP(mp1)))
        WinInvalidateRect(hwndFound,NULL,FALSE);
      else if (!IsWindowClass(hwndFound,"#3"))
        WinInvalidateRect(WinQueryWindow(hwndFound,QW_OWNER),NULL,TRUE);
      if (IsWindowClass(WinWindowFromID(HWNDFROMMP(mp2),FID_CLIENT),"SeamlessClass")) //dynamic WIn-OS2 icon handles change
        WinPostMsg(hwnd,LSWM_WNDICONCHANGED,NULL,mp2);
    }
    break;
  }

  case LSWM_WNDICONCHANGED: {
    RECTL rcl;
    BTNDATA *pbtnData;
    HWND hwndFound,hwndBtn;
    USHORT usHsw=WinQuerySwitchHandle(HWNDFROMMP(mp2),0);
    BOOL bTaskMenu=WinIsWindowVisible(plswData->hwndTaskMenu);

    if ((hwndFound=GetBtnFromHsw(hwnd,usHsw))==NULLHANDLE) break;
    hwndBtn=IsWindowClass(hwndFound,"#3")?hwndFound:WinQueryWindow(hwndFound,QW_OWNER);

    if ((hwndFound==hwndBtn || WinQueryWindowUShort(hwndBtn,QWS_ID)==usHsw) &&
        (pbtnData = WinQueryWindowPtr(hwndBtn,0))!=NULL) {
      if (IsWindowClass(WinWindowFromID(HWNDFROMMP(mp2),FID_CLIENT),"SeamlessClass")) {//dynamic WIn-OS2 icon handles change
        DosSleep(20); //there is some delay in icon change
        pbtnData->hIcon=GetWndIcon(HWNDFROMMP(mp2));
      } else {
        WinSendMsg(HWNDFROMMP(mp2),WM_NULL,0,0); //make sure the icon handle has become valid
        pbtnData->hIcon=LONGFROMMP(mp1);
      }
      WinQueryWindowRect(hwndBtn,&rcl);
      rcl.xRight = usMinBtnWid;
      WinInvalidateRect(hwndBtn,&rcl,FALSE);
    }
    if (bTaskMenu || hwndFound!=hwndBtn) {
      WinSendMsg(bTaskMenu?plswData->hwndTaskMenu:hwndFound,MM_SETITEMHANDLE,MPFROM2SHORT(usHsw,FALSE),mp1);
      WinSendMsg(bTaskMenu?plswData->hwndTaskMenu:hwndFound,MM_QUERYITEMRECT,MPFROM2SHORT(usHsw,FALSE),MPFROMP(&rcl));
      WinInvalidateRect(bTaskMenu?plswData->hwndTaskMenu:hwndFound,&rcl,FALSE);
    }

    break;
  }

  case LSWM_SWITCHLISTCHANGED: {
    ULONG ulFunc;
    HSWITCH hsw;
    BOOL bHiddenItem;
    SWCNTRL swctl;
    SWP swp;
    HWND hwndFound=0;

//0x1 or 0x10001 -- create switch entry, 0x2 -- modify, 0x3 -- delete
//these are sent by task manager to the switch list and intersepted by our hook
//0x4 means the "second loop" of processing after checking the object name

    ulFunc=LONGFROMMP(mp1);
    hsw=LONGFROMMP(mp2);

    if (hsw==0 || (ulFunc!=3 && (WinQuerySwitchEntry(hsw,&swctl)!=0 ||
                                 !WinIsWindow(plswData->hab,swctl.hwnd) ||
                                 !WinQueryWindowPos(swctl.hwnd,&swp))))
      break;

    UpdateTaskMenu(plswData, hsw, swctl, ulFunc);

    if (ulFunc != 0x3)
      bHiddenItem = (
        swctl.uchVisibility != SWL_VISIBLE ||
        (!plswData->Settings.bShowHiddenTskBar && (swp.fl&SWP_HIDE)) ||
        (!plswData->Settings.bShowViewerTskBar && IsMinToViewer(swctl.hwnd,swp.fl)) ||
        IsInSkipList(&plswData->Settings.SkipListTskBar, swctl.szSwtitle));

    if ((hwndFound=GetBtnFromHsw(hwnd,hsw))!=0) {
      if (ulFunc == 0x2 && !bHiddenItem)  // item text has changed
        UpdateTaskbarItem(plswData, usMinBtnWid, hsw, hwndFound);
      else if ((ulFunc==0x2 && bHiddenItem) || ulFunc==0x3) //item became hidden or has been deleted
        RemoveTaskbarItem(plswData, hsw, hwndFound);
    } else {
      if (ulFunc!=0x3 && !bHiddenItem) { //an item became visible or a new item has been added
        BTNDATA *pbData;
        HWND hwndBtnSameHmte=0;

        if (plswData->Settings.bGroupItems && (hwndBtnSameHmte=GetBtnFromHmte(hwnd, swctl))!=0)
          pbData = WinQueryWindowPtr(hwndBtnSameHmte,0);

// 0xFFFF means errors in getting object title, don't make further attempts
        if (hwndBtnSameHmte!=0 && pbData->pszObjName==NULL && pbData->usObjHandle!=0xFFFF) {
          if (ulFunc!=4 && swctl.bProgType==PROG_PM && !IsWPSPid(swctl.idProcess)) {
            WinPostMsg(pbData->hwndFirstItem,atomGetProcCmd,0,mp2);
            break;
          }
          InitButtonGroup(pbData, swctl, plswData->buf);

      //extract first number from WP_OBJHANDLE env var, this may be the datafile objectID
      //      for (k=0;pbData->usDataHandles[k]!=0 && k<128;k++);
      //      if (k<128) pbData->usDataHandles[k]=strtoul(plswData->buf,NULL,10)&0xFFFF;

        }
        CreateTaskbarItem(plswData, hwndBtnSameHmte, pbData, hsw, swctl, swp);
      }
    }
    break;
  }

  case WM_SIZE: {
    USHORT usButtons,usIconBtnHgt,k,usScrWid,usBtnWid,usBtnRows,usBtnHgt,
           usBtnPerRow,usRowWid,usMinBtnHgt,usMaxBtnWid,usEdgeGap,
           usIconBtns,usRegBtns,usIconBtn=0,usRegBtn=0,usMaxBtnX=0;
    HENUM henum;
    HWND hwndNext;
    RECTL rcl;
    SWP *aswp;
    BTNDATA *pbtnData;

    henum = WinBeginEnumWindows(hwnd); usButtons = usIconBtns =0;// = hwndTop = 0;
    while ((hwndNext=WinGetNextWindow(henum))!=NULLHANDLE) {
      if (IsWindowClass(hwndNext,"#3")) {
        usButtons++;
        if ((pbtnData=WinQueryWindowPtr(hwndNext,0))!=NULL && pbtnData->bIconOnly)
          usIconBtns++;
      }
    }
    WinEndEnumWindows(henum);
    usRegBtns = usButtons-usIconBtns;
    if (usButtons==0) break;

    WinQueryWindowRect(hwnd,&rcl); // may not be real WM_SIZE, can't use mp2 to get size

#ifdef XWORKPLACE
    usBorderWid=((plswData->pWidget->pGlobals->flDisplayStyle & XCS_FLATBUTTONS)?1:2);// didn't find any constants for this
#else
    usBorderWid = BUTTONBORDERWIDTH;
#endif
    usEdgeGap = plswData->Settings.b3DTaskBar?min(cyScreen/100,rcl.yTop/6)/2:0; //half the gradient border width

    usMinBtnWid = WinQuerySysValue(HWND_DESKTOP,SV_CXICON)/2+2*usBorderWid+2;
    usMaxBtnWid = plswData->Settings.usMaxBtnWid;
#ifdef XWORKPLACE
    usMinBtnHgt = usMinBtnWid+((plswData->pWidget->pGlobals->flDisplayStyle & XCS_FLATBUTTONS)?2:0);
#else
    usMinBtnHgt = usMinBtnWid;
#endif
    usBtnHgt = (rcl.yTop-2*usEdgeGap)/max(rcl.yTop/usMinBtnHgt,1);
    usIconBtnHgt = rcl.yTop-2*usEdgeGap;

    usBtnRows = max(1,rcl.yTop/max(usBtnHgt,1));

#ifndef XWORKPLACE
    if (plswData->Settings.bTskBarGrow)
      usRowWid = min(WinQuerySysValue(HWND_DESKTOP,SV_CXSCREEN),
                     usIconBtns*usMinBtnWid+usRegBtns*usMaxBtnWid)+2*TSKBARFRAMEWIDTH;
    else
#endif
      usRowWid = rcl.xRight-usIconBtns*usMinBtnWid;
    usScrWid = usRowWid*usBtnRows;

    if (usRegBtns<=usRowWid/usMaxBtnWid*usBtnRows)
      usBtnPerRow = usRowWid/usMaxBtnWid;
    else
      usBtnPerRow = usRegBtns/usBtnRows+(usRegBtns%usBtnRows>0);

    usBtnWid = max(usRowWid/max(usBtnPerRow,1),usMinBtnWid);

    if ((aswp=malloc(sizeof(SWP)*usButtons))==NULL) break;
    henum=WinBeginEnumWindows(hwnd); k=0;
    while ((hwndNext=WinGetNextWindow(henum))!=NULLHANDLE) {
      if (!IsWindowClass(hwndNext,"#3")||(pbtnData=WinQueryWindowPtr(hwndNext,0))==NULL) continue;

      if (pbtnData->bIconOnly) {
        usIconBtn++;
        aswp[k].x = (usIconBtn-1)*usMinBtnWid; aswp[k].y = usEdgeGap;
        aswp[k].cx = usMinBtnWid; aswp[k].cy = usIconBtnHgt;
      } else {
        USHORT usRow, usCol;

        usRegBtn++;
        usRow = (usRegBtn-1)/max(usBtnPerRow,1); usCol = (usRegBtn-1)%max(usBtnPerRow,1);

        aswp[k].x = usCol*usBtnWid+usIconBtns*usMinBtnWid;
        aswp[k].cx = (usCol==usBtnPerRow-1 && usBtnWid!=usMinBtnWid) ? //last btn in a row
                      usRowWid-usBtnWid*(usBtnPerRow-1) : usBtnWid;
        aswp[k].y = usRow*usBtnHgt+usEdgeGap;
        aswp[k].cy = usBtnHgt;
      }
#ifdef XWORKPLACE
      if (plswData->pWidget->pGlobals->flDisplayStyle & XCS_FLATBUTTONS &&
          !plswData->Settings.b3DTaskBar) {
        aswp[k].y += 1; aswp[k].cy-=2;
      }
#endif
      aswp[k].hwnd = hwndNext;
      aswp[k].fl = SWP_SIZE | SWP_MOVE;
      usMaxBtnX=max(usMaxBtnX,aswp[k].x+aswp[k].cx);
      k++;
    }
    WinEndEnumWindows(henum);
    WinSetMultWindowPos(plswData->hab,aswp,usButtons);
    free(aswp);
#ifndef XWORKPLACE
    if (plswData->Settings.bTskBarGrow)
      WinSetWindowPos(plswData->hwndTaskBar,0,0,0,usMaxBtnX+2*TSKBARFRAMEWIDTH,plswData->Settings.sTskBarCY,SWP_SIZE);
#endif

    break;
  }

  case WM_CREATE: {
#ifdef XWORKPLACE
    VOID *pData;
    PXCENTERWIDGET pWidget;
    LONG lrc;
#endif
    if (DosGetNamedSharedMem((PVOID*)&plswData,SHAREMEM_NAME,PAG_READ | PAG_WRITE)!=0 || plswData==NULL) {
      mrc=(MRESULT)TRUE;
      break;
    }
#ifndef XWORKPLACE
    WinSetWindowPtr(hwnd, 0, plswData);
#else
    pWidget = (PXCENTERWIDGET)mp1;
    if (pWidget==NULL || pWidget->pfnwpDefWidgetProc==NULL) {
      mrc = (MRESULT)TRUE;
      break;
    }

    pWidget->pUser=plswData;
    plswData->pWidget = pWidget; plswData->hwndTaskBarClient = hwnd;

    pData = (PVOID)plswData;
    lrc = SwitcherInit(plswData->hab, 0, NULL, 0, &pData, 2);
    if (lrc > 0) {
      if (pData != NULL) SwitcherTerm(plswData,1);
      mrc = (MRESULT)TRUE;
      break;
    } else if (lrc == LSWERROLDSETTINGS) {
    }

    WinSetWindowPtr(hwnd, 0, pWidget);
#endif

    plswData->hwndTaskMenu=WinCreateMenu(HWND_DESKTOP, NULL);
    WinSetWindowPtr(plswData->hwndTaskMenu,0,(VOID*)WinSubclassWindow(plswData->hwndTaskMenu,ButtonMenuWndProc));

    atomGetProcCmd = WinAddAtom(WinQuerySystemAtomTable(), LSWM_GETPROCCMD);

    DosFreeMem(plswData);
    WinPostMsg(hwnd,LSWM_INITBUTTONS,0,0);
#ifdef XWORKPLACE
//    pkrnSendDaemonMsg(XDM_ADDWINLISTWATCH,MPFROMHWND(hwnd),MPFROMLONG(LSWM_ACTIVEWNDCHANGED));
#endif
  }

  case WM_SYSVALUECHANGED:
    cyScreen=WinQuerySysValue(HWND_DESKTOP,SV_CYSCREEN);
    break;

  case LSWM_INITBUTTONS: {
    ULONG ulItemCount;
    LONG k;
    PSWBLOCK pSwb;

    if ((pSwb=GetSwitchList(plswData->hab,TRUE,&ulItemCount))==NULL)
      break;

    for (k=0;k<min(MAXITEMS,ulItemCount)-1;k++)
      WinSendMsg(hwnd,LSWM_SWITCHLISTCHANGED,MPFROMLONG(3),MPFROMLONG(pSwb->aswentry[k].hswitch));

    for (k=min(MAXITEMS,ulItemCount)-1;k>=0;k--)
      WinSendMsg(hwnd,LSWM_SWITCHLISTCHANGED,
                 MPFROMLONG(IsInSkipList(&plswData->Settings.SkipListTskBar,pSwb->aswentry[k].swctl.szSwtitle)?2:1),
                 MPFROMLONG(pSwb->aswentry[k].hswitch));

    GetSwitchList(0,FALSE,NULL);
    WinSendMsg(hwnd,LSWM_ACTIVEWNDCHANGED,MPFROMHWND(WinQueryActiveWindow(HWND_DESKTOP)),MPFROMLONG(WM_ACTIVATE));
    break;
  }

  case WM_DESTROY:
    WinDeleteAtom(WinQuerySystemAtomTable(),atomGetProcCmd);

    WinDestroyWindow(plswData->hwndTaskMenu);
#ifdef XWORKPLACE
//    pkrnSendDaemonMsg(XDM_REMOVEWINLISTWATCH,(MPARAM)hwnd,NULL);

    SwitcherTerm(plswData,1);
    mrc=plswData->pWidget->pfnwpDefWidgetProc(hwnd, msg, mp1, mp2);
#endif
    break;

  case WM_PRESPARAMCHANGED: {
    LONG lParam, lValue;
    UCHAR ucBuf[4];

    lParam = LONGFROMMP(mp1);

    if (lParam==PP_BACKGROUNDCOLOR &&
        WinQueryPresParam(hwnd,lParam,0,NULL,sizeof(ucBuf),ucBuf,QPF_NOINHERIT)!=0) {
      lValue = ucBuf[0]+ucBuf[1]*256+ucBuf[2]*65536;
      if (plswData->Settings.b3DTaskBar)
        plswData->Settings.lTskBarRGBCol3D = lValue;
      else
        plswData->Settings.lTskBarRGBCol = lValue;

      WinInvalidateRect(hwnd,NULL,FALSE);
    }
    break;
  }

  case WM_PAINT: {
    HPS hps;
    RECTL rcl,rcl1;

    hps = WinBeginPaint(hwnd,NULLHANDLE,&rcl);
    WinQueryWindowRect(hwnd, &rcl);
    GpiCreateLogColorTable(hps, 0, LCOLF_RGB, 0, 0, NULL);

    if (plswData->Settings.b3DTaskBar) {
      SWP swp;
      POINTL ptl;
      HWND hwndBottom;
      USHORT usEdgeWid;

      if ((hwndBottom = WinQueryWindow(hwnd,QW_BOTTOM))!=NULLHANDLE)
        WinQueryWindowPos(hwndBottom,&swp);
      else
        swp.x = swp.cx = 0;

      rcl1.xLeft=(swp.y+swp.cy==rcl.yTop?swp.x+swp.cx:0); rcl1.xRight=rcl.xRight;
      usEdgeWid=min(cyScreen/100,rcl.yTop/6);
      rcl1.yBottom=rcl.yBottom;
      rcl1.yTop=rcl.yTop-usEdgeWid;
      FillRectGradient(hps, &rcl1, NULL, plswData->Settings.lTskBarRGBCol3D, RGB_WHITE, TRUE);

      rcl1.yBottom=rcl1.yTop; rcl1.yTop=rcl.yTop;
      FillRectGradient(hps, &rcl1, NULL, plswData->Settings.lTskBarRGBCol3D, 240*65536+240*256+240, FALSE);
    } else
      WinFillRect(hps,&rcl,plswData->Settings.lTskBarRGBCol);

#ifndef XWORKPLACE
    if (!plswData->Settings.b3DTaskBar)
      WinDrawBorder(hps,&rcl,0,TSKBARFRAMEWIDTH,SUNKENFRAMERGBCOL,RAISEDFRAMERGBCOL,DB_RAISED);
#endif

    WinEndPaint(hps);
    break;
  }

  case LSWM_MOUSEOVERBTN: {
    BTNDATA *pbtnData;
    HWND hwndOldOver;

    hwndOldOver = hwndMouseOver;
    hwndMouseOver = HWNDFROMMP(mp1);

#ifdef XWORKPLACE
    if (plswData->pWidget->pGlobals->flDisplayStyle & XCS_FLATBUTTONS) {
#else
    if (plswData->Settings.bFlatButtons) {
#endif
      USHORT k; HWND hwndBtn,hwndActive;
      for (k=0,hwndBtn=hwndOldOver;k<2;k++,hwndBtn=hwndMouseOver)
        if (hwndBtn!=0 && (hwndActive=GetBtnFromHsw(hwnd,usHswActive),
                           (IsWindowClass(hwndActive,"#3")?hwndActive!=hwndBtn:
                            WinQueryWindow(hwndActive,QW_OWNER)!=hwndBtn)))
          WinInvalidateRect(hwndBtn,NULL,FALSE);
    }

    if (hwndMouseOver!=0 && !WinIsWindowVisible(plswData->hwndMenu) &&
        !WinIsWindowVisible(plswData->hwndTaskMenu) && WinIsWindowShowing(hwndMouseOver) &&
        (pbtnData = WinQueryWindowPtr(hwndMouseOver,0))!=NULL && !WinIsWindowVisible(pbtnData->hwndSubWinMenu)) {
      WinStartTimer(plswData->hab, hwnd, TSKBARBUBBLETIMERID,BUBBLETIMERINTERVAL);
    } else if (!(WinGetKeyState(HWND_DESKTOP,VK_SHIFT)&0x8000)) {
      WinStopTimer(plswData->hab,hwnd,TSKBARBUBBLETIMERID);
      ShowBubble(plswData,0,0,0,-2,NULL); //should be after WinStopTimer
    }

    break;
  }
  case WM_CONTROL:
#ifdef XWORKPLACE
    if (SHORT1FROMMP(mp1) == ID_XCENTER_CLIENT) {
      if (SHORT2FROMMP(mp1)==XN_QUERYSIZE) {//XCenter wants to know our size.
        PSIZEL pszl;

        pszl = (PSIZEL)mp2;
        pszl->cx = -1;      // desired width
// desired minimum height, 2 pixels more than the min button hgt, used for the dynamic border
        pszl->cy = WinQuerySysValue(HWND_DESKTOP,SV_CYICON)/2+2*BUTTONBORDERWIDTH+2+2;
        mrc = (MRESULT)TRUE;
      }
    } else
#endif
    {
      USHORT hsw;
      SHORT sItem;

      hsw = SHORT1FROMMP(mp1);

      switch (SHORT2FROMMP(mp1)) {
      case BN_PAINT: {
        USERBUTTON *pubButton=(USERBUTTON *)mp2;
        RECTL rcl;
        BOOL bHilite,bAllHidden;
        BTNDATA *pbtnData;
        LONG lBgndCol, lTextCol;
        USHORT usRclHgt;
        HWND hwndBtnActive, hwndSubWinMenu = ((BTNDATA*)WinQueryWindowPtr(pubButton->hwnd,0))->hwndSubWinMenu;
        USHORT usMenuItems=MenuItemCount(hwndSubWinMenu);

        if (pubButton == NULL || !WinIsWindow(plswData->hab,pubButton->hwnd) ||
            (pbtnData = WinQueryWindowPtr(pubButton->hwnd,0))==NULL) break;

        WinQueryWindowRect(pubButton->hwnd,&rcl);
        usRclHgt = rcl.yTop;

        GpiCreateLogColorTable(pubButton->hps, 0, LCOLF_RGB, 0, 0, NULL);

        hwndBtnActive=GetBtnFromHsw(hwnd,usHswActive);

        if (usMenuItems>0) {
          SHORT sId,k;
          SWCNTRL swctl;
          SWP swp;

          for (k=0,bAllHidden=TRUE;k<usMenuItems;k++) {
            sId=SHORT1FROMMR(WinSendMsg(hwndSubWinMenu,MM_ITEMIDFROMPOSITION,MPFROMSHORT(k),0));
            WinQuerySwitchEntry(sId,&swctl);
            WinQueryWindowPos(swctl.hwnd,&swp);
            if (!(swp.fl&(SWP_HIDE|SWP_MINIMIZE))) {
              bAllHidden=FALSE;
              break;
            }
          }
        }

        bHilite=((LOUSHORT(pubButton->fsState)&BDS_HILITED)==BDS_HILITED ||
                 hsw==usHswActive || WinQueryWindow(hwndBtnActive,QW_OWNER)==pubButton->hwnd);

        if (bHilite)
          if ((usMenuItems>0 && bAllHidden) || (usMenuItems==0 && pbtnData->flItemWnd&(SWP_HIDE|SWP_MINIMIZE))) {
            lTextCol = plswData->Settings.b3DTaskBar?plswData->Settings.lActiveHiddenBtnTitleRGBCol3D:plswData->Settings.lActiveHiddenBtnTitleRGBCol;
            lBgndCol = plswData->Settings.b3DTaskBar?plswData->Settings.lActiveHiddenBtnRGBCol3D:plswData->Settings.lActiveHiddenBtnRGBCol;
          } else {
            lTextCol = plswData->Settings.b3DTaskBar?plswData->Settings.lActiveBtnTitleRGBCol3D:plswData->Settings.lActiveBtnTitleRGBCol;
            lBgndCol = plswData->Settings.b3DTaskBar?plswData->Settings.lActiveBtnRGBCol3D:plswData->Settings.lActiveBtnRGBCol;
          }
        else
          if ((usMenuItems>0 && bAllHidden) || (usMenuItems==0 && pbtnData->flItemWnd&(SWP_HIDE|SWP_MINIMIZE))) {
            lTextCol = plswData->Settings.b3DTaskBar?plswData->Settings.lNormalHiddenBtnTitleRGBCol3D:plswData->Settings.lNormalHiddenBtnTitleRGBCol;
            lBgndCol = plswData->Settings.b3DTaskBar?plswData->Settings.lNormalHiddenBtnRGBCol3D:plswData->Settings.lNormalHiddenBtnRGBCol;
          } else {
            lTextCol = plswData->Settings.b3DTaskBar?plswData->Settings.lNormalBtnTitleRGBCol3D:plswData->Settings.lNormalBtnTitleRGBCol;
            lBgndCol = plswData->Settings.b3DTaskBar?plswData->Settings.lNormalBtnRGBCol3D:plswData->Settings.lNormalBtnRGBCol;
          }

        if (!plswData->Settings.b3DTaskBar)
          WinFillRect(pubButton->hps, &rcl, lBgndCol);

#ifdef XWORKPLACE
        if (!(plswData->pWidget->pGlobals->flDisplayStyle & XCS_FLATBUTTONS) ||
#else
        if (!plswData->Settings.bFlatButtons ||
#endif
            bHilite || hwndMouseOver==pubButton->hwnd) {

          if (plswData->Settings.b3DTaskBar)
            FillRectGradient(pubButton->hps, &rcl, NULL, lBgndCol, RGB_WHITE, bHilite);

          WinDrawBorder(pubButton->hps,&rcl,usBorderWid,usBorderWid,
                        SUNKENFRAMERGBCOL,RAISEDFRAMERGBCOL,bHilite?DB_DEPRESSED:DB_RAISED);
        } else if (plswData->Settings.b3DTaskBar) {
          RECTL rcl1,rclClient;
          SWP swp;

          WinQueryWindowPos(pubButton->hwnd,&swp);
          WinQueryWindowRect(hwnd,&rclClient);

          rcl1.xLeft=rcl.xLeft; rcl1.xRight=rcl.xRight;
          rcl1.yBottom=rcl.yBottom-swp.y;
          rcl1.yTop=rclClient.yTop-swp.y-min(cyScreen/100,rclClient.yTop/6);
          FillRectGradient(pubButton->hps, &rcl1, &rcl, plswData->Settings.lTskBarRGBCol3D, RGB_WHITE, TRUE);
          rcl1.yBottom=rcl1.yTop; rcl1.yTop=rclClient.yTop-swp.y;
          FillRectGradient(pubButton->hps, &rcl1, &rcl, plswData->Settings.lTskBarRGBCol3D, 240*65536+240*256+240, FALSE);
        }

// adjust the rect for text drawing
// shifting the text and icon of a pressed button by one pixel looks better
// than shifting by button border width
        rcl.xLeft+=usMinBtnWid+bHilite+1; rcl.xRight-=usBorderWid+2;
        rcl.yTop-=bHilite;                rcl.yBottom-=bHilite;

        if (!plswData->Settings.bIconsOnlyInTskBar) {
          USHORT usMenuItems=MenuItemCount(pbtnData->hwndSubWinMenu);
          USHORT usTitleLen=max(usMenuItems==0?0:strlen(pbtnData->pszObjName)+5,sizeof(ENTRYNAME))+2;
          PCHAR pszTitle;

          if ((pszTitle=malloc(usTitleLen))!=NULL) {
            pszTitle[0]='\0';

            if (usMenuItems>0 && pbtnData->pszObjName!=NULL) {
              sprintf(pszTitle,"%i - ",usMenuItems);
              strncat(pszTitle,pbtnData->pszObjName,usTitleLen-strlen(pszTitle));
            } else
              GetItemTitle(hsw,&pszTitle[strlen(pszTitle)],usTitleLen-strlen(pszTitle),FALSE);
            MakeFitStr(pubButton->hps,pszTitle,usTitleLen,rcl.xRight-rcl.xLeft+bHilite-1);
            WinDrawText(pubButton->hps,strlen(pszTitle),pszTitle,&rcl,lTextCol,lBgndCol, DT_VCENTER);
            free(pszTitle);
          }
        }

        WinDrawPointer(pubButton->hps, usBorderWid+bHilite+1,
                       (usRclHgt-usMinBtnWid)/2+usBorderWid-bHilite+1,
                       ((BTNDATA*)WinQueryWindowPtr(pubButton->hwnd,0))->hIcon,
                       DP_MINIICON/*|((pbtnData->flItemWnd&(SWP_HIDE|SWP_MINIMIZE))?DP_HALFTONED:0)*/);
        break;
      }
#ifdef XWORKPLACE
      default:
        mrc=plswData->pWidget->pfnwpDefWidgetProc(hwnd, msg, mp1, mp2);
#endif
      }
    }
    break;

  case WM_ERASEBACKGROUND:
    mrc=(MRESULT)TRUE;
    break;

  case WM_CONTEXTMENU:
    if (!plswData->bNowActive) {
      ShowBubble(plswData,0,0,0,-2,NULL);
      ShowMenu(plswData, -1,TRUE,FALSE);
    }
    break;

  case WM_COMMAND:
    if (SHORT1FROMMP(mp1)==CMD_SUSPEND) {
      ProcessAPMOffRequest(POWERSTATE_SUSPEND,DEVID_ALL_DEVICES);
    } else if (SHORT1FROMMP(mp1)==CMD_ADDFILTER ||
               (SHORT1FROMMP(mp1)>=CMD_FIRST && SHORT1FROMMP(mp1)<=CMD_LAST)) {
      ProcessCommand(SHORT1FROMMP(mp1),SHORT1FROMMP(mp2),plswData, TRUE,
        !IsWindowClass(GetBtnFromHsw(hwnd,plswData->TaskArr[plswData->iMenuAtItem].hsw),"#3"));
    }
#ifdef XWORKPLACE
    else
      mrc=plswData->pWidget->pfnwpDefWidgetProc(hwnd, msg, mp1, mp2);
#endif

    break;

  case LSWM_ACTIVEWNDCHANGED:
//#ifndef XWORKPLACE
//need a small delay as sometimes a window does not become active immediately after a WM_ACTIVATE
    WinStartTimer(plswData->hab,hwnd, TSKBARUPDATETIMERID, TSKBARUPDATEINTERVAL);
    break;
//#endif
  case WM_TIMER: {
//#ifdef XWORKPLACE
//    if (msg==LSWM_ACTIVEWNDCHANGED /*&& (ULONG)mp2==WM_ACTIVATE*/) {
//#else
    if (SHORT1FROMMP(mp1)==TSKBARUPDATETIMERID) {
//#endif
      USHORT usOldActive;
      HWND hwndHsw,hwndActive;

//#ifndef XWORKPLACE
      WinStopTimer(plswData->hab,hwnd,TSKBARUPDATETIMERID);
//#endif

      usOldActive=usHswActive;
      hwndActive=WinQueryActiveWindow(HWND_DESKTOP);
      usHswActive=WinQuerySwitchHandle(hwndActive,0)&0xFFFF;
      if (usHswActive!=usOldActive) {
        hwndHsw=GetBtnFromHsw(hwnd,usOldActive);
        WinInvalidateRect(IsWindowClass(hwndHsw,"#3")?hwndHsw:WinQueryWindow(hwndHsw,QW_OWNER),NULL,FALSE);
        hwndHsw=GetBtnFromHsw(hwnd,usHswActive);
        WinInvalidateRect(IsWindowClass(hwndHsw,"#3")?hwndHsw:WinQueryWindow(hwndHsw,QW_OWNER),NULL,FALSE);
      }
      break;
    }

    if (SHORT1FROMMP(mp1)==TSKBARBUBBLETIMERID) {
      POINTL ptl;
      SHORT iItem;
      USHORT usX,usSizeY;
      RECTL rcl1;
      BTNDATA *pbtnData;
      HPS hps;
      POINTL aptl[TXTBOX_COUNT];

      if (WinIsWindowShowing(hwndMouseOver) && //no activity in FS mode
          (iItem=TaskArrItemFromHsw(plswData,WinQueryWindowUShort(hwndMouseOver,QWS_ID),TRUE))>=0 &&
          (pbtnData = WinQueryWindowPtr(hwndMouseOver,0))!=NULL) {
        WinQueryPointerPos(HWND_DESKTOP,&ptl);
        usX=ptl.x;
        WinMapWindowPoints(HWND_DESKTOP,hwnd,&ptl,1);
        WinQueryWindowRect(hwndMouseOver,&rcl1);
        WinMapWindowPoints(hwndMouseOver,HWND_DESKTOP,(POINTL*)&rcl1,2);
        hps = WinGetPS(plswData->hwndBubble);
        GpiQueryTextBox(hps,1,"M",TXTBOX_COUNT,aptl);
        WinReleasePS(hps);
        usSizeY = aptl[TXTBOX_TOPLEFT].y-aptl[TXTBOX_BOTTOMLEFT].y+4;
        ShowBubble(plswData,iItem,usX,rcl1.yTop+1+usSizeY>cyScreen?
                   rcl1.yBottom-usSizeY:rcl1.yTop+1,2,
                   MenuItemCount(pbtnData->hwndSubWinMenu)>0?pbtnData->pszObjName:NULL);
      }
      break;
    }
  }

  default:
#ifdef XWORKPLACE
    mrc=plswData->pWidget->pfnwpDefWidgetProc(hwnd, msg, mp1, mp2);
#else
    mrc=WinDefWindowProc(hwnd, msg, mp1, mp2);
#endif
  }

  return (mrc);
}


SHORT InitTaskBar(LSWDATA* plswData,UCHAR *ucErrMsg,USHORT usMsgLen)
{ ULONG ulFrameFlags;

  if (!WinRegisterClass(plswData->hab,LSWTASKBARCLASS,TaskBarWndProc,
      CS_PARENTCLIP | CS_SIZEREDRAW | /*CS_SYNCPAINT |*/ CS_CLIPCHILDREN,sizeof(PLSWDATA))) {
    strncpy(ucErrMsg,"WinRegisterClass(lswTaskBarClass)",usMsgLen);
    return WinGetLastError(plswData->hab);
  }

  ulFrameFlags = plswData->Settings.bAllowResize?FCF_SIZEBORDER:FCF_BORDER;
  plswData->hwndTaskBar = WinCreateStdWindow(HWND_DESKTOP,0L,&ulFrameFlags,
                          LSWTASKBARCLASS,"", 0L,NULLHANDLE,0,&plswData->hwndTaskBarClient);

  if (plswData->hwndTaskBar==NULLHANDLE) {
    strncpy(ucErrMsg,"WinCreateStdWin(LSWTaskBar)",usMsgLen);
    return WinGetLastError(plswData->hab);
  }

  WinSendMsg(plswData->hwndTaskBar,WM_SETBORDERSIZE,MPFROMSHORT(TSKBARFRAMEWIDTH),MPFROMSHORT(TSKBARFRAMEWIDTH));

  WinSetWindowPtr(plswData->hwndTaskBar,0,(VOID *)WinSubclassWindow(plswData->hwndTaskBar, TskBarFrameWndProc));

  WinSetWindowPos(plswData->hwndTaskBar,0 ,
                  plswData->Settings.sTskBarX,plswData->Settings.sTskBarY,
                  plswData->Settings.sTskBarCX,plswData->Settings.sTskBarCY,
                  SWP_MOVE | SWP_SIZE |
                  (plswData->Settings.bTaskBarAlwaysVisible?SWP_SHOW:0));

  WinPostMsg(0,WM_NULL,0,0); //reset the hook

  return 0;
}

VOID DoneTaskBar(LSWDATA *plswData)
{
  WinDestroyWindow(plswData->hwndTaskBar);
  plswData->hwndTaskBar = NULLHANDLE;
}

  /*
  case WM_USER+1000:
  case LSWM_SWITCHLISTCHANGED: {
    ULONG ulFunc;
    HSWITCH hsw;
    BOOL bHiddenItem;
    SWCNTRL swctl;
    SWP swp;
    HWND hwndFound=0;
    USHORT usObjHandle;

//    static struct _PRES {ULONG cb; ULONG id1; ULONG cb1; UCHAR szFnt1[sizeof(DEFTITLEFONT3)];} pres=
//      {sizeof(ULONG)*2+sizeof(DEFTITLEFONT3),PP_FONTNAMESIZE,sizeof(DEFTITLEFONT3),DEFTITLEFONT3};

//0x1 or 0x10001 -- create switch entry, 0x2 -- modify, 0x3 -- delete
//these are sent by task manager to the switch list and intersepted by our hook
//0x4 means the "second loop" of processing after checking the object name

    if (msg==LSWM_SWITCHLISTCHANGED)
      ulFunc=LONGFROMMP(mp1);
    else {
      ulFunc=0x4;
      usObjHandle=SHORT1FROMMP(mp1);
    }

    hsw=LONGFROMMP(mp2);

    if (hsw==0 || (ulFunc!=3 && (WinQuerySwitchEntry(hsw,&swctl)!=0 ||
                                 !WinIsWindow(plswData->hab,swctl.hwnd) ||
                                 !WinQueryWindowPos(swctl.hwnd,&swp))))
      break;

    if (ulFunc != 0x3)
      bHiddenItem = (
        swctl.uchVisibility != SWL_VISIBLE ||
        (!plswData->Settings.bShowHiddenTskBar && (swp.fl&SWP_HIDE)) ||
        (!plswData->Settings.bShowViewerTskBar && IsMinToViewer(swctl.hwnd,swp.fl)) ||
        IsInSkipList(&plswData->Settings, swctl.szSwtitle,TRUE));

    if ((hwndFound=GetBtnFromHsw(hwnd,hsw))!=0) {
      if (ulFunc == 0x2 && !bHiddenItem) { // item text has changed
        RECTL rcl;
        SHORT sItem;

        if (IsWindowClass(hwndFound,"#3")) {
          WinQueryWindowRect(hwndFound,&rcl);
          rcl.xLeft = usMinBtnWid;
          WinInvalidateRect(hwndFound,&rcl,FALSE);
          if (!plswData->bNowActive && (sItem=TaskArrItemFromHsw(plswData,hsw,TRUE))>=0)
            ShowBubble(plswData,sItem,0,0,11,NULL);
        } else {
          ENTRYNAME Title;

          GetItemTitle(hsw,Title,sizeof(Title),FALSE);
          WinSendMsg(hwndFound,MM_SETITEMTEXT,MPFROMSHORT(hsw),MPFROMP(Title));
        }

        break;
      } else if ((ulFunc==0x2 && bHiddenItem) || ulFunc==0x3) { //item became hidden or has been deleted
        BTNDATA *pbData;

        if (IsWindowClass(hwndFound,"#3")) {
          pbData=WinQueryWindowPtr(hwndFound,0);
          WinDestroyWindow(hwndFound);
          WinDestroyWindow(pbData->hwndSubWinMenu);
          free(pbData);
          WinInvalidateRect(hwnd,NULL,FALSE);
        } else {
          HWND hwndBtn=WinQueryWindow(hwndFound,QW_OWNER);
          SHORT sItemCount;

          pbData=WinQueryWindowPtr(hwndBtn,0);

          WinSendMsg(pbData->hwndSubWinMenu,MM_DELETEITEM,MPFROM2SHORT(hsw,FALSE),0);
          sItemCount=SHORT1FROMMR(WinSendMsg(hwndFound,MM_QUERYITEMCOUNT,0,0));
//          WinSetWindowUShort(hwndBtn,QWS_ID,(SHORT)WinSendMsg(hwndFound,MM_ITEMIDFROMPOSITION,MPFROMSHORT(1),0));
          if (sItemCount==1 || (hsw & 0xFFFF)==WinQueryWindowUShort(hwndBtn,QWS_ID)) {
            SWCNTRL swctlItem;
            SWP swpItem;
            SHORT sId=(SHORT)WinSendMsg(pbData->hwndSubWinMenu,MM_ITEMIDFROMPOSITION,0,0);

            WinSetWindowUShort(hwndBtn,QWS_ID,sId);
            WinQuerySwitchEntry(sId,&swctlItem);
            WinQueryWindowPos(swctlItem.hwnd,&swpItem);
            pbData->hIcon = GetWndIcon(swctlItem.hwnd);
            pbData->flItemWnd = swpItem.fl;
            if (sItemCount==1) {
              WinSendMsg(pbData->hwndSubWinMenu,MM_DELETEITEM,MPFROM2SHORT(sId,FALSE),0);
              free(pbData->pszObjName); pbData->pszObjName=NULL;
            }
          }
          WinInvalidateRect(hwndBtn,NULL,FALSE);
        }
      } else
        break;
    } else {
      if (ulFunc!=0x3 && !bHiddenItem) { //an item became visible or a new item has been added
        BTNDATA *pbData;
        HWND hwndBtn,hwndBtnSamePid=0,hwndNext;
        HENUM henum;
        RECTL rcl;
        SWCNTRL swNext;

        if (plswData->Settings.bGroupItems) {
          henum = WinBeginEnumWindows(hwnd);
          while ((hwndNext=WinGetNextWindow(henum))!=NULLHANDLE) {
            if (IsWindowClass(hwndNext,"#3") && (pbData = WinQueryWindowPtr(hwndNext,0))!=NULL &&
                pbData->pid==swctl.idProcess &&
                (swctl.idProcess!=GetWPSPid() || (!IsDesktopBtn(hwndNext)))) {
              hwndBtnSamePid=hwndNext;
              break;
            }
          }
          WinEndEnumWindows(henum);
        }

        if (hwndBtnSamePid && (ulFunc!=4 || (ulFunc==4 && usObjHandle!=0))) {
//we get object handle and name only when the first entry is added to the submenu
          if (SHORT1FROMMR(WinSendMsg(pbData->hwndSubWinMenu,MM_QUERYITEMCOUNT,0,0))==0) {
            if (ulFunc!=0x4 && swctl.idProcess!=GetWPSPid()) {
              WinPostMsg(swctl.hwnd,LSWM_WPSGETOBJID,MPFROMLONG(WPSGETOBJIDMAGIC),mp2);
              break;
            } else {
              USHORT usHswBtn=WinQueryWindowUShort(hwndBtnSamePid,QWS_ID);
              if (swctl.idProcess!=GetWPSPid())
                pbData->pszObjName=GetObjectName(usObjHandle);
              else {
                HLIB hlib;
                if ((hlib = WinLoadLibrary(plswData->hab, "PMWP.DLL"))!=NULLHANDLE) {
                  pbData->hIcon = WinLoadPointer(HWND_DESKTOP, hlib, 26);
                  WinDeleteLibrary(plswData->hab, hlib);
                }
                pbData->pszObjName=strdup("WPS");
              }
              InsertMenuItem(pbData->hwndSubWinMenu,NULL,MIT_END,usHswBtn,"",MIS_OWNERDRAW,0,0);
            }
          }
          InsertMenuItem(pbData->hwndSubWinMenu,NULL,MIT_END,hsw&0xFFFF,"",MIS_OWNERDRAW,0,0);
          WinInvalidateRect(hwndBtnSamePid,NULL,FALSE);
          break;
        } else {
          hwndBtn=WinCreateWindow(hwnd,WC_BUTTON,"",WS_VISIBLE | BS_USERBUTTON | BS_NOPOINTERFOCUS,
                                  0,0,0,0,hwnd,IsDesktop(swctl.hwnd)?HWND_TOP:HWND_BOTTOM,hsw & 0xFFFF, NULL,NULL);
          if (hwndBtn==NULLHANDLE) break;
          if ((pbData=malloc(sizeof(BTNDATA)))!=NULL) {
            pbData->hIcon = GetWndIcon(swctl.hwnd);
            pbData->flItemWnd = swp.fl;
            pbData->usObjHandle=usObjHandle;
            pbData->hwndSubWinMenu=WinCreateMenu(HWND_DESKTOP, NULL);
            WinSetWindowPtr(pbData->hwndSubWinMenu,0,(VOID*)WinSubclassWindow(pbData->hwndSubWinMenu,ButtonMenuWndProc));
            WinSetOwner(pbData->hwndSubWinMenu,hwndBtn);
            pbData->pid=swctl.idProcess;
            pbData->pszObjName=NULL;
            WinSetWindowPtr(hwndBtn,0,(VOID *)pbData);
            pbData->pOldWndProc=WinSubclassWindow(hwndBtn,ButtonWndProc);
            WinSetPresParam(hwndBtn,PP_FONTNAMESIZE,strlen(plswData->Settings.ucButtonFont)+1,plswData->Settings.ucButtonFont);
          } else
            WinDestroyWindow(hwndBtn);
        }
      } else
        break;
    }
  }
*/

