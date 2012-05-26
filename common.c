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

#include <process.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include "common.h"
#include "apm.h"
#include "prmdlg.h"
#include "settings.h"
#include "taskbar.h"
#include "fspopup.h"
#include "pmpopup.h"
#include "lswres.h"
#include "eastring.h"
#include "object.h"

#ifdef XWORKPLACE
  #include "dlgids.h"
#endif

LONG SwitcherInit(HAB hab, HMQ hmq, UCHAR *ucErrMsg, USHORT usMsgLen, PVOID *ppData, USHORT usFunc)
{ ULONG flFrameFlags,ulHookVer;
  LONG rc,rc1 = 0;
  LSWDATA *plswData;
  UCHAR ucFName[CCHMAXPATH],ucFound=0,ucLangStr[32];
  USHORT usResLang;
  SWCNTRL swctl;

// usFunc is 0 for app initialization, 1 and for widget phases 1 and 2 initialization

  if (usFunc != 1)
    DosSetPriority(PRTYS_THREAD,PRTYC_TIMECRITICAL,+31,0);

  if (usFunc != 2) {
//need to loop so that search handle is closed
    while (FindResDll(ucFName, sizeof(ucFName), &usResLang, ucLangStr, sizeof(ucLangStr)))
      if (usResLang!=0) ucFound=1;

    if (!ucFound) {
      strncpy(ucErrMsg,"Could not find resource DLL or resource DLL invalid",usMsgLen);
      return 1;
    }

    ulHookVer = lswHookGetVersion();
    if (LOUCHAR(LOUSHORT(ulHookVer)) < LASTHOOKVERMAJOROK ||
                HIUCHAR(LOUSHORT(ulHookVer)) < LASTHOOKVERMINOROK ||
                LOUCHAR(HIUSHORT(ulHookVer)) < LASTHOOKREVISIONOK) {
      strncpy(ucErrMsg,"Wrong hook DLL version",usMsgLen);
      return 1;
    }

    if (!WinRegisterClass(hab,LSWPOPUPCLASS,PopupWndProc,CS_SAVEBITS|CS_SYNCPAINT,sizeof(PLSWDATA))) {
      strncpy(ucErrMsg,"WinRegisterClass(lswPopupClass) error",usMsgLen);
      return WinGetLastError(hab);
    }

    if ((rc = DosAllocSharedMem((VOID*)&plswData,SHAREMEM_NAME,sizeof(LSWDATA),PAG_COMMIT | PAG_READ | PAG_WRITE))!=0) {
      strncpy(ucErrMsg,"DosAllocSharedMem error",usMsgLen);
      return rc;
    }

    memset(plswData,0,sizeof(LSWDATA));
    *ppData = (PVOID)plswData;

#ifdef XWORKPLACE
    plswData->bWidget = TRUE;
#endif

    plswData->hab = hab;
    plswData->hmq = hmq;

    GetIniFileName(ucFName,sizeof(ucFName));
    if (LoadSettings(hab,ucFName,&plswData->Settings)!=0) rc1=LSWERRCANTLOADSETTINGS;
    if (rc1==0 && !CheckSettings(&plswData->Settings)) rc1=LSWERROLDSETTINGS;

    if (rc1 < 0)
      InitSettings(&plswData->Settings);
    else if (plswData->Settings.sTskBarCX>WinQuerySysValue(HWND_DESKTOP,SV_CXSCREEN)+2) {
      plswData->Settings.sTskBarX = -TSKBARFRAMEWIDTH;
      if (plswData->Settings.bTaskBarTopScr)
        plswData->Settings.sTskBarY = WinQuerySysValue(HWND_DESKTOP,SV_CYSCREEN)-plswData->Settings.sTskBarCY+TSKBARFRAMEWIDTH;
      else
        plswData->Settings.sTskBarY = -TSKBARFRAMEWIDTH;
      plswData->Settings.sTskBarCX = WinQuerySysValue(HWND_DESKTOP,SV_CXSCREEN)+2*TSKBARFRAMEWIDTH;
      plswData->Settings.sTskBarCY = WinQuerySysValue(HWND_DESKTOP,SV_CYICON)/2+4+2*BUTTONBORDERWIDTH+2+2*TSKBARFRAMEWIDTH;
    }
  }

  if (usFunc != 1) {
#ifdef XWORKPLACE
    if (usFunc==2) plswData=(PLSWDATA)(*ppData);
#endif

    if ((plswData->hmodRes=LoadResource(plswData->Settings.usLanguage, plswData, TRUE))==0) {
      strncpy(ucErrMsg,"Could not load resources ",usMsgLen);
      return 1;
    }

    if ((rc = DosCreateEventSem(SEMRUNNINGNAME,&plswData->hevRunning,0,0)) ||
        (rc = DosCreateEventSem(SEMCTRLTABNAME,&plswData->hevCtrlTab,0,
                                (plswData->Settings.ucPopupHotKey==1)))
       ) {
      strncpy(ucErrMsg,"DosCreateEventSem",usMsgLen);
      return rc;
    }

    flFrameFlags = 0;
#ifndef XWORKPLACE
    flFrameFlags |= FCF_TASKLIST | FCF_ICON;
#endif
    plswData->hwndPopup = WinCreateStdWindow(HWND_DESKTOP,0,&flFrameFlags,
                          LSWPOPUPCLASS,"",
                          0,0,ID_POPUPWIN,&plswData->hwndPopClient);

    if (plswData->hwndPopup==NULLHANDLE) {
      strncpy(ucErrMsg,"WinCreateStdWin(lswPopupWin)",usMsgLen);
      return WinGetLastError(hab);
    }

    WinSetWindowPos(plswData->hwndPopup,HWND_BOTTOM,POPUPWINHIDEPOSX,POPUPWINHIDEPOSY,
                    0, 0, SWP_MOVE | SWP_SIZE | SWP_SHOW | SWP_DEACTIVATE | SWP_ZORDER);
    //subsclassing must go after setting position so that desktop number which is
    //calculated by the frame wnd proc relative to the hide position is done correctly
    WinSetWindowPtr(plswData->hwndPopup, 0, (VOID *)WinSubclassWindow(plswData->hwndPopup, FrameWndProc));

    plswData->hwndBubble = WinCreateWindow(HWND_DESKTOP,WC_STATIC,"",
                                           SS_TEXT | DT_CENTER | DT_VCENTER,
                                           0,0,0,0,0,HWND_BOTTOM,ID_BUBBLE,NULL,NULL);
    WinSetPresParam(plswData->hwndBubble,PP_FONTNAMESIZE,7,"8.Helv");
    WinSetPresParam(plswData->hwndBubble,PP_BACKGROUNDCOLOR,
                    sizeof(plswData->Settings.lBubbleRGBCol),
                    &plswData->Settings.lBubbleRGBCol);
    WinSetPresParam(plswData->hwndBubble,PP_FOREGROUNDCOLOR,
                    sizeof(plswData->Settings.lBubbleTextRGBCol),
                    &plswData->Settings.lBubbleTextRGBCol);
    WinCreateWindow(plswData->hwndBubble,WC_STATIC,"",SS_HALFTONEFRAME,
                    0,0,0,0,0,HWND_TOP,ID_BUBBLEFRAME,NULL,NULL);


    if ((plswData->itidFSDispat = _beginthread(FSMonDispat,NULL,0x4000,plswData))<0) {
      strncpy(ucErrMsg,"_beginthread",usMsgLen);
      return plswData->itidFSDispat;
    }

    if ((rc = lswHookInit(plswData)) < 0) {
      strncpy(ucErrMsg,"lswHookInit",usMsgLen);
      return (-rc);
    }

#ifndef XWORKPLACE
    plswData->hswitch = WinQuerySwitchHandle(plswData->hwndPopup, 0);
    WinQuerySwitchEntry(plswData->hswitch, &swctl);
    swctl.uchVisibility = plswData->Settings.bShowInWinList ? SWL_VISIBLE : SWL_INVISIBLE;
    WinChangeSwitchEntry(plswData->hswitch, &swctl);

    if (plswData->Settings.bTaskBarOn)
      if ((rc = InitTaskBar(plswData,ucErrMsg,usMsgLen)) != 0)
        return rc;

    if ((rc = DosExitList(EXLST_ADD, (PFNEXITLIST)lswExitProc)) !=0 ) {
      strncpy(ucErrMsg,"DosExitList",usMsgLen);
      return rc;
    }
#else
    if (plswData->pWidget->pGlobals!=NULL) plswData->hswitch = WinQuerySwitchHandle(plswData->pWidget->pGlobals->hwndFrame, 0);
#endif
    WinAddAtom(WinQuerySystemAtomTable(), LSWM_SETPRIORITY);
  }

  return rc1;
}

VOID SwitcherTerm(LSWDATA *plswData,USHORT usFunc)
{
// usFunc is 0 for app termination, 1 and 2 for widget phases 1 and 2 termination
  if (usFunc!=2) {
    HATOMTBL hatomtblAtomTbl=WinQuerySystemAtomTable();
    WinDeleteAtom(hatomtblAtomTbl,WinFindAtom(hatomtblAtomTbl,LSWM_SETPRIORITY));

    lswHookTerm(plswData);

    DosPostEventSem(plswData->hevRunning);
    DosSleep(500);

#ifndef XWORKPLACE
    DoneTaskBar(plswData);
#endif

    DosCloseEventSem(plswData->hevShift);
    DosCloseEventSem(plswData->hevPopup);
    DosCloseEventSem(plswData->hevRunning);
    DosCloseEventSem(plswData->hevCtrlTab);

    WinDestroyWindow(plswData->hwndPopup);
    if (WinIsWindow(plswData->hab,plswData->hwndParamDlg)) WinDestroyWindow(plswData->hwndParamDlg);

    DosFreeModule(plswData->hmodRes);
  }

#ifndef XWORKPLACE
  WinDestroyMsgQueue(plswData->hmq);
  WinTerminate(plswData->hab);
#endif

  if (usFunc!=1)
    DosFreeMem(plswData);
}


VOID APIENTRY lswExitProc()
{ LSWDATA *plswData;

  DosGetNamedSharedMem((PVOID*)&plswData,SHAREMEM_NAME,PAG_READ | PAG_WRITE);

  if (plswData!=NULL) SwitcherTerm(plswData,0);

  DosExitList(EXLST_EXIT, (PFNEXITLIST)lswExitProc);
}


ULONG MapCommand(USHORT cmd)
{
  switch (cmd) {
  case CMD_HIDE:     return MAKEULONG(SC_HIDE,SWP_HIDE);
  case CMD_MAXIMIZE: return MAKEULONG(SC_MAXIMIZE,SWP_MAXIMIZE);
  case CMD_MINIMIZE: return MAKEULONG(SC_MINIMIZE,SWP_MINIMIZE);
  case CMD_RESTORE:  return MAKEULONG(SC_RESTORE,SWP_RESTORE);
  case CMD_SHOW:     return MAKEULONG(0,SWP_SHOW);
  case CMD_MOVE:     return MAKEULONG(SC_MOVE,SWP_MOVE);
  case CMD_CLOSE:    return MAKEULONG(SC_CLOSE,0);
  default: return 0;
  }
}

/* This advanced killing Copyright (C)1996 by Holger.Veit@gmd.de */
HFILE OpenXF86(VOID)
{ HFILE hfd;
  ULONG action;

  if (DosOpen((PSZ)"/dev/fastio$", (PHFILE)&hfd, &action, 0, FILE_SYSTEM, FILE_OPEN,
      OPEN_SHARE_DENYNONE|OPEN_FLAGS_NOINHERIT|OPEN_ACCESS_READONLY, 0)!=0)
    return 0;
  else
    return hfd;
}

BOOL XF86Installed(VOID)
{ HFILE hfd;

  if ((hfd = OpenXF86())!=0) DosClose(hfd);
  return (hfd!=0);
}

BOOL Death(PID pid)
{ HFILE hfd;
  ULONG plen;
  USHORT param;


  if ((hfd=OpenXF86())==0) return FALSE;
  param = pid;

  if (DosDevIOCtl(hfd,0x76,0x65,(PULONG*)&param,sizeof(USHORT),&plen,NULL,0,NULL)!=0) {
    DosClose(hfd);
    return FALSE;
  }

  DosClose(hfd);
  return TRUE;
}

BOOL IsWPSPid(PID pid)
{ UCHAR szPidPath[_MAX_PATH]={0};

  DosQueryModuleName(HmteFromPID(pid), sizeof(szPidPath), szPidPath);
  return (strstr(szPidPath,"PMSHELL.EXE")!=NULL);
}

BOOL ChangeWindowPos(LSWDATA *plswData,SHORT iItemNum,USHORT cmd)
{ USHORT cmd1;
  BOOL bDoIt;

  cmd1=LOUSHORT(MapCommand(cmd));

  bDoIt = ((cmd==CMD_KILL || cmd==CMD_DEATH || cmd==CMD_CLOSE || cmd==CMD_CLOSEQUIT) ? TRUE :
           cmd == CMD_SHOW ? (!(plswData->TaskArr[iItemNum].fl & SWP_SHOW)):
           WndHasControl(plswData->hab,plswData->TaskArr[iItemNum].hwnd,cmd1));

  if (bDoIt) {
    if (cmd==CMD_SHOW)
      WinShowWindow(plswData->TaskArr[iItemNum].hwnd,TRUE);
    else if (cmd==CMD_CLOSEQUIT)
      WinPostMsg(plswData->TaskArr[iItemNum].hwnd, WM_QUIT, 0,0);
    else if (cmd==CMD_KILL)
      DosKillProcess(DKP_PROCESS,plswData->TaskArr[iItemNum].pid);
    else if (cmd==CMD_DEATH)
      Death(plswData->TaskArr[iItemNum].pid);
    else
      WinPostMsg(plswData->TaskArr[iItemNum].hwnd,WM_SYSCOMMAND,MPFROMSHORT(cmd1),MPFROM2SHORT(CMDSRC_OTHER,FALSE));
  }

  return bDoIt;
}


/* this procedure returns TRUE if the the popup menu for the usItem item in
 the TaskArr needs the menu item usId. If yes, the title is returned also */
BOOL MenuNeedsItem(LSWDATA *plswData,USHORT usItem,USHORT usId,UCHAR *pszTitle,USHORT usLen,BOOL bGroup)
{ BOOL bNeedsItem=FALSE;
  USHORT k;

  if (bGroup) {
    for (k=0;k<plswData->usItems && bNeedsItem==FALSE;k++) {
      if (HmteFromPID(plswData->TaskArr[k].pid)!=HmteFromPID(plswData->TaskArr[usItem].pid))
        continue;

      if (usId==CMD_SHOW && (plswData->TaskArr[k].fl & SWP_HIDE))
        bNeedsItem=TRUE;
      else if (usId>=CMD_HIDE && usId<=CMD_CLOSE && usId!=CMD_MOVE) {
        bNeedsItem = WndHasControl(plswData->hab,plswData->TaskArr[k].hwnd,LOUSHORT(MapCommand(usId)));
        if (usId == CMD_HIDE)
          bNeedsItem &= (!(plswData->TaskArr[k].fl & SWP_HIDE));

        if ((usId >= CMD_HIDE && usId<=CMD_MAXIMIZE))
          bNeedsItem &= IsInDesktop(plswData->hab,plswData->TaskArr[k].hwnd,plswData->lCurrDesktop);
      }
    }
  } else {
    if (usId == CMD_CLOSEQUIT || usId == CMD_ADDFILTER ||
        (usId == CMD_SHOW && (plswData->TaskArr[usItem].fl & SWP_HIDE)))
      bNeedsItem = TRUE;
    else if (usId == CMD_SWITCHFROMPM)
      bNeedsItem = ((plswData->TaskArr[usItem].fl & (SWP_HIDE|SWP_MINIMIZE)) ||
                    plswData->TaskArr[usItem].hwnd!=WinQueryActiveWindow(HWND_DESKTOP) ||
                    !IsInDesktop(plswData->hab,plswData->TaskArr[usItem].hwnd,plswData->lCurrDesktop));
    else if (usId==CMD_KILL || usId==CMD_DEATH || usId==CMD_PRIORITY) {
      bNeedsItem=(!IsWPSPid(plswData->TaskArr[usItem].pid));
      if (usId==CMD_DEATH) bNeedsItem &= XF86Installed();
    } else if (usId == CMD_MOVE) {
      bNeedsItem =
        (!(plswData->TaskArr[usItem].fl & SWP_HIDE) &&
         !IsMinToViewer(plswData->TaskArr[usItem].hwnd,plswData->TaskArr[usItem].fl) &&
         WndHasControl(plswData->hab,plswData->TaskArr[usItem].hwnd,SC_MOVE));
    } else {
      bNeedsItem = WndHasControl(plswData->hab,plswData->TaskArr[usItem].hwnd,LOUSHORT(MapCommand(usId)));
      if (usId == CMD_HIDE)
        bNeedsItem &= (!(plswData->TaskArr[usItem].fl & SWP_HIDE));
    }

    if (usId >= CMD_HIDE && usId<=CMD_MAXIMIZE)
      bNeedsItem &= IsInDesktop(plswData->hab,plswData->TaskArr[usItem].hwnd,plswData->lCurrDesktop);

  }

  if (bNeedsItem && pszTitle!=NULL)
    WinLoadString(plswData->hab,plswData->hmodRes,usId,usLen,pszTitle);

  return bNeedsItem;
}

BOOL WndHasControl(HAB hab,HWND hwndToCheck,USHORT usControl)
{ HWND hwndSysMenu, hwndMinMax;

  return
    (WinIsWindow(hab,hwndToCheck) && WinIsWindowEnabled(hwndToCheck) &&
     (
      (
       (hwndSysMenu=WinWindowFromID(hwndToCheck,FID_SYSMENU))!=NULLHANDLE &&
       SHORT1FROMMR(WinSendMsg(hwndSysMenu,MM_ISITEMVALID,MPFROM2SHORT(usControl,TRUE),0))==TRUE
      ) ||
      (
       (hwndMinMax=WinWindowFromID(hwndToCheck,FID_MINMAX))!=NULLHANDLE &&
       SHORT1FROMMR(WinSendMsg(hwndMinMax,MM_QUERYITEMCOUNT,0,0))>0 &&
       (SHORT)WinSendMsg(hwndMinMax,MM_ITEMPOSITIONFROMID,MPFROM2SHORT(usControl,0),0)!=MIT_NONE
      )
     )
    );
}

SHORT InsertMenuItem(HWND hwndMenu, HWND hwndSubMenu, SHORT iPosition, SHORT sItemId,
                     char *ItemTitle, SHORT afStyle, SHORT afAttr, ULONG hItem)
{ MENUITEM mi;

  mi.iPosition = iPosition;
  mi.afStyle = afStyle;
  mi.afAttribute = afAttr;
  mi.id = sItemId;
  mi.hwndSubMenu = hwndSubMenu;
  mi.hItem = hItem;
  return SHORT1FROMMR(WinSendMsg(hwndMenu, MM_INSERTITEM, (MPARAM)&mi, (MPARAM)ItemTitle));
}

VOID InitTaskActionsMenu(HWND hwndMenu, LSWDATA *plswData,SHORT iMenuAtItem,BOOL bTaskBar,BOOL bGroup)
{ SHORT sItemId,sItemNum,sLastId,k,sAttr;
  UCHAR ucBuf[64];
  HWND hwndSubmenu;

  sItemNum = SHORT1FROMMR(WinSendMsg(hwndMenu,MM_QUERYITEMCOUNT,0,0));
  while (sItemNum > 0) {
    sItemId = SHORT1FROMMR(WinSendMsg(hwndMenu,MM_ITEMIDFROMPOSITION,
                                      MPFROMSHORT(sItemNum-1),0));
    sItemNum = SHORT1FROMMR(WinSendMsg(hwndMenu,
                                       sItemId==CMD_XCENTERSUBMENU?MM_REMOVEITEM:MM_DELETEITEM,
                                       MPFROM2SHORT(sItemId,FALSE),0));
  }

  if (iMenuAtItem >= 0) {  //neither an empty spot on the taskbar nor the Desktop button, show context menu
    for (sItemId = CMD_SWITCHFROMPM,sLastId=0; sItemId <= CMD_MOVE; sItemId++) {
      if (!bTaskBar && sItemId==CMD_MOVE)
        continue;
      if (MenuNeedsItem(plswData,iMenuAtItem,sItemId,ucBuf,sizeof(ucBuf),bGroup)) {
        if ((sItemId == CMD_CLOSE || sItemId == CMD_KILL || sLastId==CMD_SWITCHFROMPM) &&
            SHORT1FROMMR(WinSendMsg(hwndMenu,MM_QUERYITEMCOUNT,0,0))>0)
          InsertMenuItem(hwndMenu, NULLHANDLE, MIT_END, 0, "", MIS_SEPARATOR, 0,0);
        InsertMenuItem(hwndMenu, NULLHANDLE, MIT_END, sItemId, ucBuf, MIS_TEXT, 0,0);
        sLastId=sItemId;
      }
    }

    if (!IsDesktop(plswData->TaskArr[iMenuAtItem].hwnd)) {
      WinLoadString(plswData->hab,plswData->hmodRes,CMD_CLOSE,sizeof(ucBuf),ucBuf);
      if (SHORT1FROMMR(WinSendMsg(hwndMenu,MM_QUERYITEMCOUNT,0,0))>0)
        InsertMenuItem(hwndMenu, NULLHANDLE, MIT_END, 0, "", MIS_SEPARATOR, 0,0);

      hwndSubmenu = WinCreateMenu(hwndMenu, NULL);

      if (hwndSubmenu != NULLHANDLE) {
        WinSetWindowULong(hwndSubmenu,QWL_STYLE,
                          WinQueryWindowULong(hwndSubmenu,QWL_STYLE) | MS_CONDITIONALCASCADE);
        sItemId=InsertMenuItem(hwndMenu, hwndSubmenu, MIT_END, CMD_CLOSE, ucBuf, MIS_TEXT|MIS_SUBMENU, 0,0);

        if (sItemId != MIT_MEMERROR && sItemId != MIT_ERROR) {
          sAttr = MIA_CHECKED;
          if (MenuNeedsItem(plswData,iMenuAtItem,CMD_CLOSE,ucBuf,sizeof(ucBuf),bGroup)) {
            InsertMenuItem(hwndSubmenu, NULLHANDLE, MIT_END, CMD_CLOSE, "SC_CLOSE", MIS_TEXT, MIA_CHECKED,0);
            sAttr = 0;
          }
          InsertMenuItem(hwndSubmenu, NULLHANDLE, MIT_END, CMD_CLOSEQUIT, "WM_QUIT", MIS_TEXT, sAttr,0);

          if (MenuNeedsItem(plswData,iMenuAtItem,CMD_KILL,ucBuf,sizeof(ucBuf),bGroup))
            InsertMenuItem(hwndSubmenu, NULLHANDLE, MIT_END, CMD_KILL, ucBuf, MIS_TEXT, 0,0);

          if (MenuNeedsItem(plswData,iMenuAtItem,CMD_DEATH,ucBuf,sizeof(ucBuf),bGroup))
            InsertMenuItem(hwndSubmenu, NULLHANDLE, MIT_END, CMD_DEATH, ucBuf, MIS_TEXT, 0,0);
        }
      }
    }

    if (MenuNeedsItem(plswData,iMenuAtItem,CMD_PRIORITY,ucBuf,sizeof(ucBuf),bGroup)) {
      InsertMenuItem(hwndMenu, NULLHANDLE, MIT_END, 0, "", MIS_SEPARATOR, 0,0);
      InsertMenuItem(hwndMenu, NULLHANDLE, MIT_END, CMD_PRIORITY, ucBuf, MIS_TEXT, 0,0);
    }

    if (MenuNeedsItem(plswData,iMenuAtItem,CMD_ADDFILTER,ucBuf,sizeof(ucBuf),bGroup)) {
      InsertMenuItem(hwndMenu, NULLHANDLE, MIT_END, 0, "", MIS_SEPARATOR, 0,0);
      InsertMenuItem(hwndMenu, NULLHANDLE, MIT_END, CMD_ADDFILTER, ucBuf, MIS_TEXT, 0,0);
    }
  }
#ifndef XWORKPLACE
  else { //iMenuAtItem==-1, either an empty spot on the taskbar or the Desktop button, show generic menu
    WinLoadString(plswData->hab,plswData->hmodRes,STRID_RUN,sizeof(ucBuf),ucBuf);
    InsertMenuItem(hwndMenu, NULLHANDLE, MIT_END, CMD_RUN, ucBuf, MIS_TEXT, 0,0);
    InsertMenuItem(hwndMenu, NULLHANDLE, MIT_END, 0, "", MIS_SEPARATOR, 0,0);
    WinLoadString(plswData->hab,plswData->hmodRes,STRID_SUSPEND,sizeof(ucBuf),ucBuf);
    InsertMenuItem(hwndMenu, NULLHANDLE, MIT_END, CMD_SUSPEND, ucBuf, MIS_TEXT, 0,0);
    InsertMenuItem(hwndMenu, NULLHANDLE, MIT_END, 0, "", MIS_SEPARATOR, 0,0);
    WinLoadString(plswData->hab,plswData->hmodRes,STRID_SHOWSETTINGS,sizeof(ucBuf),ucBuf);
    InsertMenuItem(hwndMenu, NULLHANDLE, MIT_END, CMD_SHOWSETTINGS, ucBuf, MIS_TEXT, 0,0);
    WinLoadString(plswData->hab,plswData->hmodRes,STRID_QUIT,sizeof(ucBuf),ucBuf);
    InsertMenuItem(hwndMenu, NULLHANDLE, MIT_END, CMD_QUIT, ucBuf, MIS_TEXT, 0,0);
  }
#else
  WinSetPresParam(plswData->pWidget->hwndContextMenu,PP_FONTNAMESIZE,7,"8.Helv");

  if (bTaskBar && iMenuAtItem >= 0) {
  // now set the old context menu as submenu;
    InsertMenuItem(hwndMenu, NULLHANDLE, MIT_END, 0, "", MIS_SEPARATOR, 0,0);

    WinLoadString(plswData->hab,plswData->hmodRes,STRID_XCENTERSUBMENU,sizeof(ucBuf),ucBuf);
    InsertMenuItem(hwndMenu, plswData->pWidget->hwndContextMenu, MIT_END,
                   CMD_XCENTERSUBMENU, ucBuf, MIS_TEXT | MIS_SUBMENU, 0,0);
  }
#endif
}

VOID ShowMenu(LSWDATA *plswData,SHORT iMenuAtItem,BOOL bTaskBar,BOOL bGroup)
{ POINTL ptl;

  InitTaskActionsMenu(plswData->hwndMenu,plswData,iMenuAtItem,bTaskBar,bGroup);
  WinQueryPointerPos(HWND_DESKTOP,&ptl);

#ifdef XWORKPLACE
  WinPopupMenu(HWND_DESKTOP, bTaskBar?plswData->hwndTaskBarClient:plswData->hwndPopClient,
               (iMenuAtItem<0&&bTaskBar)?plswData->pWidget->hwndContextMenu:plswData->hwndMenu,
               ptl.x,ptl.y,0, PU_HCONSTRAIN | PU_VCONSTRAIN | PU_MOUSEBUTTON1 | PU_MOUSEBUTTON2 | PU_KEYBOARD);
#else
  WinPopupMenu(HWND_DESKTOP, bTaskBar?plswData->hwndTaskBarClient:plswData->hwndPopClient,plswData->hwndMenu,
               ptl.x, ptl.y,0, PU_HCONSTRAIN | PU_VCONSTRAIN | PU_MOUSEBUTTON1 | PU_MOUSEBUTTON2 | PU_KEYBOARD);
#endif
}

VOID ShowBubble(LSWDATA *plswData,SHORT iMouseIsAtItem,USHORT usX,USHORT usY,SHORT iFunc,UCHAR *ucTitle2)
{ static SHORT iMouseWasAtItem=-1,iCounter1=0,iBubbleAtItem=-1,iSource;
  USHORT usSizeX,usSizeY;
  POINTL aptl[TXTBOX_COUNT];
  UCHAR ucTitle[NAMELEN];
  HPS hps;
  LONG cxScreen;
  SWP swp;

  if (iFunc>0) {
    if (iFunc<=10) { //iFunc==11 -- update bubble text
      if (iMouseIsAtItem == iMouseWasAtItem || iMouseWasAtItem < 0)
        iCounter1+=BUBBLETIMERINTERVAL;
      else
        iCounter1=0;
      iMouseWasAtItem = iMouseIsAtItem;
      iSource=iFunc;
    }

    if ((iFunc<=10 && ((iCounter1 >= 800 /*ms*/&& iBubbleAtItem < 0) ||
        (iBubbleAtItem >= 0  && iBubbleAtItem != iMouseIsAtItem))) ||
        (iFunc==11 && iMouseIsAtItem==iBubbleAtItem)) {
      if (ucTitle2==NULL)
        GetItemTitle(plswData->TaskArr[iMouseIsAtItem].hsw,ucTitle,sizeof(ucTitle),FALSE);
      else
        strncpy(ucTitle,ucTitle2,sizeof(ucTitle)-1);
      hps = WinGetPS(plswData->hwndBubble);
      GpiQueryTextBox(hps,strlen(ucTitle),ucTitle,TXTBOX_COUNT,aptl);
      WinReleasePS(hps);
      if (iFunc==11) {
        WinQueryWindowPos(plswData->hwndBubble,&swp);
        usX = swp.x; usY = swp.y;
      }
      usSizeX = aptl[TXTBOX_TOPRIGHT].x-aptl[TXTBOX_TOPLEFT].x+8;
      usSizeY = aptl[TXTBOX_TOPLEFT].y-aptl[TXTBOX_BOTTOMLEFT].y+4;
      cxScreen=WinQuerySysValue(HWND_DESKTOP,SV_CXSCREEN);
      if (usX+usSizeX>cxScreen) usX-=usX+usSizeX-cxScreen;
      WinSetWindowPos(plswData->hwndBubble,HWND_TOP,
                      usX,usY,usSizeX,usSizeY,
                      SWP_MOVE | SWP_SIZE | SWP_SHOW | SWP_ZORDER);
      WinSetWindowPos(WinWindowFromID(plswData->hwndBubble,ID_BUBBLEFRAME),0,0,0,
                      usSizeX,usSizeY, SWP_MOVE | SWP_SIZE | SWP_SHOW);
      WinSetWindowText(plswData->hwndBubble,ucTitle);
      iBubbleAtItem = iMouseIsAtItem;
    } else if (iBubbleAtItem>=0 &&
               (WinQueryWindowPos(plswData->hwndBubble,&swp),
                aptl[0].x=swp.x,aptl[0].y=aptl[1].y=swp.y+swp.cy-1,aptl[1].x=swp.x+swp.cx-1,
                WinWindowFromPoint(HWND_DESKTOP,&aptl[0],FALSE)!=plswData->hwndBubble ||
                WinWindowFromPoint(HWND_DESKTOP,&aptl[1],FALSE)!=plswData->hwndBubble
              )) {
      WinSetWindowPos(plswData->hwndBubble,HWND_TOP,0,0,0,0,SWP_ZORDER);
      WinInvalidateRect(plswData->hwndBubble,NULL,TRUE);
    }
  } else if (abs(iFunc)==iSource) {
    UCHAR ucBuf[32];

    if (WinIsWindowVisible(plswData->hwndBubble))
      WinSetWindowPos(plswData->hwndBubble,HWND_BOTTOM,0,0,0,0,SWP_HIDE | SWP_ZORDER);
    iBubbleAtItem = -1;
    iMouseWasAtItem = -1;
    iCounter1 = 0;

    WinQueryPresParam(plswData->hwndBubble,PP_BACKGROUNDCOLOR,0,NULL,sizeof(ucBuf),ucBuf,QPF_NOINHERIT);
    plswData->Settings.lBubbleRGBCol = ucBuf[0]+ucBuf[1]*256+ucBuf[2]*65536;
    WinQueryPresParam(plswData->hwndBubble,PP_FOREGROUNDCOLOR,0,NULL,sizeof(ucBuf),ucBuf,QPF_NOINHERIT);
    plswData->Settings.lBubbleTextRGBCol = ucBuf[0]+ucBuf[1]*256+ucBuf[2]*65536;
  }
}

BOOL IsWindowClass(HWND hwnd,UCHAR *pszClassName)
{ UCHAR ucBuf[128];

  WinQueryClassName(hwnd,sizeof(ucBuf),ucBuf);
  return (strcmp(ucBuf,pszClassName)==0);
}

// returns a pointer to PSWBLOCK and the item count; a second call with bInit=FALSE
// is needed to free the memory allocated for PSWBLOCK
PVOID GetSwitchList(HAB hab,BOOL bInit,ULONG *ItemCount)
{ static PSWBLOCK pSwb=NULL;
  static ULONG ulItemCount,ulRecCount=0;

  if (bInit) {
    ulRecCount++;
    if (pSwb!=NULL) {
      *ItemCount=ulItemCount;
      return pSwb;
    }
    if ((ulItemCount = WinQuerySwitchList(hab, NULL, 0))==0 ||
        (pSwb=malloc(sizeof(HSWITCH)+sizeof(SWENTRY)*ulItemCount))==NULL) {
      return NULL;
    }
    /* get all switch entries in one call; calling WinQuerySwitchHandle/SwitchEntry
     for each window turns out to be noticeably slower */
    ulItemCount = WinQuerySwitchList(hab,pSwb,sizeof(HSWITCH)+sizeof(SWENTRY)*ulItemCount);
    *ItemCount=ulItemCount;
    return pSwb;
  } else {
    ulRecCount--;
    if (ulRecCount==0) {
      if (pSwb!=NULL) free(pSwb);
      pSwb=NULL;
    }
    return NULL;
  }
}

BOOL IsMinToViewer(HWND hwnd,ULONG flopt)
{
  return
    ((flopt & SWP_MINIMIZE) && WinQueryWindowUShort(hwnd,QWS_YMINIMIZE)==33536);
}

//returns TRUE if any part of the hwnd is currently within desktop
BOOL IsInDesktop(HAB hab,HWND hwnd,LONG lCurrDesktop)
{ SWP swp;
  RECTL rcl1,rcl2,rcl3;
  USHORT usCXScreen=WinQuerySysValue(HWND_DESKTOP,SV_CXSCREEN),
         usCYScreen=WinQuerySysValue(HWND_DESKTOP,SV_CYSCREEN);

  WinQueryWindowPos(hwnd,&swp);

/*  if (swp.fl & SWP_MINIMIZE) {
    swp.x = WinQueryWindowUShort(hwnd,QWS_XRESTORE); swp.y = WinQueryWindowUShort(hwnd,QWS_YRESTORE);
    swp.cx = WinQueryWindowUShort(hwnd,QWS_CXRESTORE); swp.cy = WinQueryWindowUShort(hwnd,QWS_CYRESTORE);
  }*/
  WinSetRect(hab,&rcl1,swp.x,swp.y,swp.x+swp.cx,swp.y+swp.cy);

// +8 may need to be added to CXScreen and CYScreen as virtual desktop dimensions seem to be 8 points larger than screen size
/*  WinSetRect(hab,&rcl2,
             usCXScreen*((lCurrDesktop&0xFF00)>>8),usCYScreen*(lCurrDesktop&0xFF),
             usCXScreen*(((lCurrDesktop&0xFF00)>>8)+1),usCYScreen*((lCurrDesktop&0xFF)+1));
             */

  WinSetRect(hab,&rcl2,0,0,usCXScreen, usCYScreen);

  WinIntersectRect(hab,&rcl3,&rcl1,&rcl2);

  return (!WinIsRectEmpty(hab,&rcl3));
}

/*
//get the virtual desktop number relative to the one in which the program was started
LONG GetCurrDesktop(LSWDATA *plswData)
{ USHORT usCXScreen=WinQuerySysValue(HWND_DESKTOP,SV_CXSCREEN),
         usCYScreen=WinQuerySysValue(HWND_DESKTOP,SV_CYSCREEN);
  SWP swp;
  SHORT x,y;

  if (plswData->bNowActive) return -1;

  WinQueryWindowPos(plswData->hwndPopup,&swp);
  x=(POPUPWINHIDEPOSX-swp.x)/usCXScreen;
  y=(POPUPWINHIDEPOSY-swp.y)/usCYScreen;
  return ((x<<8)+y);
}
*/

VOID MinimizeHideAll(LSWDATA *plswData, BOOL bReset, HWND hwndReset)
{ SHORT i;
  HSWITCH hswDesktop;
  static USHORT usNumWin;
  static BOOL bRestoreAll=FALSE;
  static LONG lDskNum;
  static HWND *hwndList=NULL;

  if (bReset) {
    if (hwndList!=NULL)
    for (i=0;i<usNumWin;i++)
      if (hwndReset==hwndList[i]) {
        free(hwndList); hwndList=NULL;
        bRestoreAll=FALSE;
        break;
      }
    return;
  }

  if (!bRestoreAll) {
    if (hwndList!=NULL) free(hwndList);
    hswDesktop=plswData->TaskArr[plswData->usCurrItem].hsw;
    InitTaskArr(plswData,FALSE,FALSE,FALSE);
    if ((hwndList=malloc(plswData->usItems*sizeof(HWND)))!=NULL)
      for (i = plswData->usItems-2,usNumWin=0; i >= 0; i--) {
        if ((MenuNeedsItem(plswData,i,CMD_MINIMIZE,NULL,0,FALSE) && ChangeWindowPos(plswData,i,CMD_MINIMIZE)) ||
            (MenuNeedsItem(plswData,i,CMD_HIDE,NULL,0,FALSE) && ChangeWindowPos(plswData,i,CMD_HIDE)))
          hwndList[usNumWin++]=plswData->TaskArr[i].hwnd;
      }
    lDskNum=plswData->lCurrDesktop;//GetCurrDesktop(plswData);
    WinSwitchToProgram(hswDesktop);
  } else {
    if (hwndList==NULL || lDskNum!=plswData->lCurrDesktop)//GetCurrDesktop(plswData)
      return;

    for (i=0;i<usNumWin;i++) {
      WinSetWindowPos(hwndList[i],HWND_TOP,0,0,0,0,SWP_SHOW|SWP_ZORDER);
      WinSwitchToProgram(WinQuerySwitchHandle(hwndList[i],0));
    }

    free(hwndList); hwndList=NULL;
  }

  bRestoreAll ^= TRUE;
}


VOID GetItemTitle(HSWITCH hsw,UCHAR *ucTitle,USHORT usLen,BOOL bSessNum)
{ SWCNTRL swctl;
  USHORT k;

  if (WinQuerySwitchEntry(hsw, &swctl)!=0) {
    strcpy(ucTitle,"");
    return;
  }

  if (bSessNum)
    sprintf(ucTitle,"%.*s (0x%X)",usLen-6,swctl.szSwtitle,swctl.idProcess);
  else
    strncpy(ucTitle,swctl.szSwtitle,usLen-1);

  for (k=0;k<strlen(ucTitle);k++)
    if (ucTitle[k]=='\r' || ucTitle[k]=='\n') ucTitle[k]=' ';
}


HPOINTER GetWndIcon(HWND hwnd)
{ HPOINTER hIcon;
  UCHAR *pszPath;
  HWND hwnd1;

  if ((hIcon=(HPOINTER)WinSendMsg(hwnd,WM_QUERYICON,0,0))!=NULLHANDLE)
    return hIcon;

  if (IsWindowClass((hwnd1=WinWindowFromID(hwnd,FID_CLIENT)),"SeamlessClass")) {
    pszPath = (UCHAR *)WinQueryWindowULong(hwnd1,0L);
    hIcon=WinLoadFileIcon(pszPath,FALSE);
  } /*else {
    ULONG ulItemCount;
    LONG k;
    PSWBLOCK pSwb;
    PID pid;
    TID tid;

    if ((pSwb=GetSwitchList(0,TRUE,&ulItemCount))!=NULL) {
      for (k=ulItemCount-1;k>=0 && hIcon==NULLHANDLE;k--) {
        WinQueryWindowProcess(hwnd,&pid,&tid);
        if (pSwb->aswentry[k].swctl.hwnd!=hwnd && pSwb->aswentry[k].swctl.idProcess==pid)
          hIcon=(HPOINTER)WinSendMsg(pSwb->aswentry[k].swctl.hwnd,WM_QUERYICON,0,0);
      }
      GetSwitchList(0,FALSE,NULL);
    }
  }*/

  if (hIcon == NULLHANDLE)
    hIcon = WinQuerySysPointer(HWND_DESKTOP,SPTR_PROGRAM,FALSE);

  return hIcon;
}
/*
PSZ GetObjectName(USHORT usObjHandle)
{ UCHAR ucKey[16],*pucObjData,*substr=NULL,*pszName=NULL;
  ULONG k,ulDataSize;

  if ((pucObjData=malloc(ulDataSize=1024))==NULL) return NULL;
  sprintf(ucKey,"%X",usObjHandle);
  if (PrfQueryProfileData(HINI_USERPROFILE,"PM_Abstract:Objects",ucKey,pucObjData,&ulDataSize))
    for (k=0,substr=NULL;substr==NULL && k<ulDataSize-strlen("WPAbstract");k++)
      if ((substr=strstr(&pucObjData[k],"WPAbstract"))!=NULL) substr+=17;

  if (substr!=NULL) pszName=strdup(substr);
  free(pucObjData);
  return pszName;
}
*/
BOOL IsDesktop(HWND hwnd)
{
  return (hwnd==WinQueryWindow(HWND_DESKTOP,QW_BOTTOM) &&
          (IsWindowClass(hwnd,"wpFolder window")));
}

//Written by Staffan Ulfberg
char Match(char *string, char *pattern)
{
  for (; '*'^*pattern; ++pattern, ++string) {
    if (!*string)
      return (!*pattern);
    if (toupper(*string)^toupper(*pattern) && '?'^*pattern)
      return FALSE;
  }
  /* two-line patch to prevent *too* much recursiveness: */
  while('*' == pattern[1])
    pattern++;
  do {
    if ( Match(string, pattern + 1) )
      return TRUE;
  } while (*string++);
  return FALSE;
}


BOOL IsInSkipList(SKIPLIST *pSkipList,UCHAR *ucTitle)
{ BOOL bFound;
  USHORT j;

  for (j = 0, bFound = FALSE;
       j < MAXITEMS && (*pSkipList)[j]!=NULL &&
         !(bFound=Match(ucTitle, (*pSkipList)[j]));
       j++);

  return bFound;
}

BOOL AddFilter(SKIPLIST *pSkipList,UCHAR *ucName)
{ SHORT iNum;

  if (strlen(ucName)==0 || IsInSkipList(pSkipList,ucName)) return FALSE;

  for (iNum=0; (*pSkipList)[iNum]!=NULL && iNum<MAXITEMS; iNum++);
  if (iNum==MAXITEMS) return FALSE;

  (*pSkipList)[iNum++]=strdup(ucName);
  return TRUE;
}

BOOL RemoveFilter(SKIPLIST *pSkipList,UCHAR *ucName)
{ SHORT iNum,k;

  for (iNum=0; (*pSkipList)[iNum]!=NULL && iNum<MAXITEMS; iNum++);

  for (k = 0; k < iNum; k++)
    if (strcmp((*pSkipList)[k],ucName)==0) {
      free((*pSkipList)[k]);
      memmove(&(*pSkipList)[k],&(*pSkipList)[k+1],sizeof(*pSkipList[0])*(iNum-k-1));
      (*pSkipList)[--iNum]=NULL;
      return TRUE;
    }

  return FALSE;
}

VOID InitTaskArr(LSWDATA *plswData,BOOL bFullScreen,BOOL bTaskBar,BOOL bUseFilters)
{ ULONG sidCurr,ulItemCount,ulItem;
  PSWBLOCK pSwb;
  HENUM henum;
  HWND hwndNext,hwndActive;
  SWP swp;
  UCHAR ucIndex;
  SWITEM ditem;
  BOOL bFound,bIsCurrent;
  SKIPLIST *pSkipList=bTaskBar?&plswData->Settings.SkipListTskBar:&plswData->Settings.SkipListPopup;

  sidCurr = 0;
  hwndActive = NULLHANDLE;

  plswData->usItems = plswData->usCurrItem = plswData->iShift = 0;
  memset(plswData->TaskArr,0,sizeof(plswData->TaskArr));

  if ((pSwb=GetSwitchList(plswData->hab,TRUE,&ulItemCount))==NULL) return;
/* get SID of the current fullscreen process */
  if (bFullScreen) {
    if (DosQuerySysInfo(24,24,&sidCurr,sizeof(sidCurr))!=0) sidCurr=0;
  } else {
    hwndActive = WinQueryActiveWindow(WinQueryDesktopWindow(plswData->hab,NULLHANDLE));
  }

  henum = WinBeginEnumWindows(WinQueryDesktopWindow(plswData->hab,NULLHANDLE));

  while ((hwndNext = WinGetNextWindow(henum))!=NULLHANDLE && plswData->usItems < MAXITEMS-1) {
    for (ulItem = 0, bFound = FALSE; ulItem < ulItemCount; ulItem++)
      if ((bFound=(hwndNext == pSwb->aswentry[ulItem].swctl.hwnd))==TRUE) break;

    if (!bFound || !WinIsWindow(plswData->hab,hwndNext) ||
        pSwb->aswentry[ulItem].swctl.uchVisibility != SWL_VISIBLE)
      continue;

    WinQueryWindowPos(hwndNext,&swp);

    bFound = (!bUseFilters || (sidCurr == pSwb->aswentry[ulItem].swctl.idSession ||
               (((bTaskBar?plswData->Settings.bShowHiddenTskBar:plswData->Settings.bShowHidden) || !(swp.fl & SWP_HIDE)) &&
                ((bTaskBar?plswData->Settings.bShowViewerTskBar:plswData->Settings.bShowViewer) || !IsMinToViewer(hwndNext,swp.fl)))) &&
              !IsInSkipList(pSkipList, pSwb->aswentry[ulItem].swctl.szSwtitle));

    /* save the current entry in the last element of the array even if it's
     not supposed to be switched to. It will be used if switching is cancelled */
    bIsCurrent = ((!bFullScreen &&
                  (hwndNext == hwndActive ||
                   hwndNext == WinQueryWindow(hwndActive,QW_OWNER))) ||
                  (bFullScreen && pSwb->aswentry[ulItem].swctl.idSession == sidCurr));

    if (bFound) {
      ucIndex = plswData->usItems;
      plswData->usItems++;
    } else if (bIsCurrent)
      ucIndex = MAXITEMS-1;
    else
      continue;

    plswData->TaskArr[ucIndex].ulType = pSwb->aswentry[ulItem].swctl.bProgType;
    plswData->TaskArr[ucIndex].fl = swp.fl;
    plswData->TaskArr[ucIndex].hsw = pSwb->aswentry[ulItem].hswitch;
    plswData->TaskArr[ucIndex].hwnd = hwndNext;
    plswData->TaskArr[ucIndex].hIcon = GetWndIcon(hwndNext);
    plswData->TaskArr[ucIndex].pid = pSwb->aswentry[ulItem].swctl.idProcess;

    if (bFound && bIsCurrent) {
      memcpy(&plswData->TaskArr[MAXITEMS-1],&plswData->TaskArr[ucIndex],sizeof(SWITEM));
      /* make sure current session or window is in the 0th element of the TaskArr */
      if (ucIndex != 0) {
        memcpy(&ditem,&plswData->TaskArr[ucIndex],sizeof(ditem));
        memmove(&plswData->TaskArr[1],&plswData->TaskArr[0],sizeof(ditem)*ucIndex);
        memcpy(&plswData->TaskArr[0],&ditem,sizeof(ditem));
      }
    }
  }

  WinEndEnumWindows(henum);

  GetSwitchList(0,FALSE,NULL);

  if (plswData->TaskArr[MAXITEMS-1].hsw == 0)
    plswData->TaskArr[MAXITEMS-1].hwnd = hwndActive;
}

USHORT RunCommand(LSWDATA *plswData,UCHAR *ucCommand,UCHAR *ucErrMsg,USHORT usErrMsgLen)
{ STARTDATA SData;
  static UCHAR k,ucCmdLine[_MAX_PATH]="",ucPath[_MAX_PATH],ucPgmInp[_MAX_PATH],
    drive[_MAX_DRIVE],dir[_MAX_DIR],name[_MAX_FNAME],ext[_MAX_EXT],*args,
    ucExtStr[4][4]={"bat\0","cmd\0","com\0","exe\0"};
  APIRET rc;
  PID pid;
  ULONG ulSessID,ulAppType;

  if ((args=strchr(ucCommand,' '))!=NULL) {
    strcpy(ucCmdLine,args);
    *args='\0';
  }
  strcpy(ucPath,ucCommand);

  _splitpath(ucPath,drive,dir,name,ext);

  for (k=0,rc=0;k<(strlen(ext)==0?4:1);k++) { //add an extension if needed and find if file exists
    if (strlen(ext)==0) _makepath(ucPath,drive,dir,name,ucExtStr[k]);
    ulAppType=0;
    rc=DosQueryAppType(ucPath,&ulAppType);
    if (rc==0 || rc==191 || rc==193)
      break;
    else
      strncpy(ucPath,name,sizeof(ucPath));
  }

  memset(&SData,0,sizeof(SData));
  SData.Length  = sizeof(SData);
  SData.Related = SSF_RELATED_INDEPENDENT;
  SData.FgBg    = SSF_FGBG_FORE;
  SData.TraceOpt = SSF_TRACEOPT_NONE;
  SData.InheritOpt = SSF_INHERTOPT_SHELL;
  SData.PgmControl = SSF_CONTROL_VISIBLE;
  SData.SessionType = SSF_TYPE_DEFAULT;

  if (ulAppType==0x20 || strcmpi(ucPath + strlen(ucPath) - 4, ".BAT") == 0)
    SData.SessionType = SSF_TYPE_WINDOWEDVDM;

  if (ulAppType&(FAPPTYP_WINDOWSREAL|FAPPTYP_WINDOWSPROT|FAPPTYP_WINDOWSPROT31)) {
    SData.PgmName="WINOS2.COM";
    sprintf(ucPgmInp, "/3 %s %s", ucPath, ucCmdLine);
    SData.SessionType = PROG_31_ENHSEAMLESSCOMMON;
  } else if (rc!=0 || strcmpi(ucPath + strlen(ucPath) - 4, ".CMD") == 0) {
    sprintf(ucPgmInp, "/C%s %s", ucPath, ucCmdLine);
  } else {
    SData.PgmName = ucPath;
    strcpy(ucPgmInp, ucCmdLine);
  }
  SData.PgmInputs = ucPgmInp;

  if ((rc=DosStartSession(&SData, &ulSessID, &pid))!=0) {
    WinLoadString(plswData->hab,plswData->hmodRes,MSG_CANTEXECUTE,usErrMsgLen,ucErrMsg);
    strncat(ucErrMsg,SData.PgmName,usErrMsgLen-strlen(ucErrMsg)-1);
    return rc;
  }

  return 0;
}

VOID SetControlsFont(HWND hwnd,BOOL bDoTitleBar)
{ ULONG aulSysInfo[2]={0};
  UCHAR ucFont[FACESIZE];
  HWND hwndCtl;
  HENUM henum;

  DosQuerySysInfo(QSV_VERSION_MAJOR, QSV_VERSION_MINOR,aulSysInfo,sizeof(aulSysInfo));
  strcpy(ucFont,(aulSysInfo[0]==20 && aulSysInfo[1]>=40)?DEFTITLEFONT4:DEFTITLEFONT3);

  henum = WinBeginEnumWindows(hwnd);
  while ((hwndCtl=WinGetNextWindow(henum))!=NULLHANDLE) {
    if (!bDoTitleBar && IsWindowClass(hwndCtl,"#9")) continue;
    WinSetPresParam(hwndCtl,PP_FONTNAMESIZE,strlen(ucFont)+1,ucFont);
  }
  WinEndEnumWindows(henum);
}

VOID GetStartupDir(UCHAR *ucDir,USHORT usLen)
{ PPIB ppib;
  SHORT k;

#ifndef XWORKPLACE
  DosGetInfoBlocks(NULL,&ppib);
  DosQueryModuleName(ppib->pib_hmte, usLen, ucDir);
#else
  DosQueryModuleName(hmodWidgetDll, usLen, ucDir);
#endif

  for (k = strlen(ucDir)-1; ucDir[k] != '\\' && k >= 0; k--);
  ucDir[k+1] = '\0';
}

BOOL queryAppInstance(VOID)
{ HEV hev;

  if (hev=0, DosOpenEventSem(SEMRUNNINGNAME,&hev))
    return FALSE;
  else {
    DosCloseEventSem(hev);
    return TRUE;
  }
}

BOOL UpdateWinFlags(ULONG *OldFlags,ULONG NewFlags)
{ BOOL bNeedUpdate;

  NewFlags &= (SWP_MINIMIZE|SWP_MAXIMIZE|SWP_RESTORE|SWP_SHOW|SWP_HIDE|
               SWP_ACTIVATE|SWP_DEACTIVATE);

  bNeedUpdate = (((NewFlags & SWP_MINIMIZE) && !(*OldFlags & SWP_MINIMIZE)) ||
                 ((NewFlags & SWP_ACTIVATE) && !(*OldFlags & SWP_ACTIVATE)) ||
                 ((NewFlags & SWP_DEACTIVATE) && !(*OldFlags & SWP_DEACTIVATE)) ||
                 ((NewFlags & SWP_SHOW) && !(*OldFlags & SWP_SHOW)) ||
                 ((NewFlags & SWP_HIDE) && !(*OldFlags & SWP_HIDE)) ||
                 ((NewFlags & SWP_RESTORE) && (*OldFlags & SWP_MINIMIZE)) ||
                 ((NewFlags & SWP_MAXIMIZE) && (*OldFlags & SWP_MINIMIZE))
                );

  *OldFlags |= NewFlags;

  if (NewFlags & SWP_MINIMIZE) *OldFlags &= (~(SWP_RESTORE|SWP_MAXIMIZE));
  if (NewFlags & SWP_RESTORE)  *OldFlags &= (~(SWP_MINIMIZE|SWP_MAXIMIZE));
  if (NewFlags & SWP_MAXIMIZE) *OldFlags &= (~(SWP_MINIMIZE|SWP_RESTORE));

  if (NewFlags & SWP_HIDE) *OldFlags &= (~SWP_SHOW);
  if (NewFlags & SWP_SHOW) *OldFlags &= (~SWP_HIDE);

  if (NewFlags & SWP_ACTIVATE)   *OldFlags &= (~SWP_DEACTIVATE);
  if (NewFlags & SWP_DEACTIVATE) *OldFlags &= (~SWP_ACTIVATE);

  return bNeedUpdate;
}

VOID MakeFitStr(HPS hps,UCHAR *ucStr,USHORT usStrSize, USHORT usStrWid)
{ USHORT usLen,usEllWid;
  POINTL aptl[TXTBOX_COUNT];

  usLen=strlen(ucStr);
  GpiQueryTextBox(hps,usLen,ucStr,TXTBOX_COUNT,aptl);
  if (aptl[TXTBOX_BOTTOMRIGHT].x-aptl[TXTBOX_BOTTOMLEFT].x<=usStrWid) return;

  GpiQueryTextBox(hps,2,"..",TXTBOX_COUNT,aptl);
  usEllWid = aptl[TXTBOX_BOTTOMRIGHT].x-aptl[TXTBOX_BOTTOMLEFT].x;

  do {
    GpiQueryTextBox(hps,--usLen,ucStr,TXTBOX_COUNT,aptl);
  } while (usLen>0 && aptl[TXTBOX_BOTTOMRIGHT].x-aptl[TXTBOX_BOTTOMLEFT].x + usEllWid > usStrWid);

  ucStr[usLen]='\0';
  strncat(ucStr,"..",usStrSize-usLen);
}

USHORT FindResDll(UCHAR *ucDllName, USHORT usNameLen, USHORT *usLang, UCHAR *ucLangStr, USHORT usLangStrLen)
{ static UCHAR ucDir[_MAX_PATH],ucName[_MAX_PATH],ucErr[32];
  static HDIR hdir = HDIR_CREATE;
  HMODULE hmodRes;
  FILEFINDBUF3 FindBuffer = {0};
  ULONG ulResultBufLen, ulFindCount;
  APIRET rc;
  RESVERPROC *ResVerProc;
  ULONG ulVer;

  ulResultBufLen = sizeof(FILEFINDBUF3);
  ulFindCount = 1;

  if (hdir == HDIR_CREATE) {
    GetStartupDir(ucDir,sizeof(ucDir));
    sprintf(ucName,"%s*.dll",ucDir);
    rc = DosFindFirst(ucName, &hdir, FILE_NORMAL, &FindBuffer, ulResultBufLen, &ulFindCount, FIL_STANDARD);
  } else
    rc = DosFindNext(hdir, &FindBuffer, ulResultBufLen, &ulFindCount);

  if (rc == 0) {
    sprintf(ucName,"%s%s",ucDir,FindBuffer.achName);
    if (DosLoadModule(ucErr, sizeof(ucErr), ucName, &hmodRes)==0) {
      if (DosQueryProcAddr(hmodRes, 0, "QueryResourceVersion", &ResVerProc)==0 &&
          (ResVerProc(&ulVer,usLang,ucLangStr,usLangStrLen),
            (LOUCHAR(LOUSHORT(ulVer)) >= LASTINIVERMAJOROK &&
             HIUCHAR(LOUSHORT(ulVer)) >= LASTINIVERMINOROK &&
             LOUCHAR(HIUSHORT(ulVer)) >= LASTINIREVISIONOK))) {
        strncpy(ucDllName,ucName,usNameLen);
      } else
        *usLang = 0;
      DosFreeModule(hmodRes);
    }
  } else {
    DosFindClose(hdir);
    hdir = HDIR_CREATE;
    return 0;
  }
  return 1;
}

HMODULE LoadResource(USHORT usLang, LSWDATA *plswData, BOOL bEngOk)
{ static UCHAR ucDllName[_MAX_PATH], ucLangStr[32], ucErr[32], ucFoundDllName[_MAX_PATH];
  HMODULE hmodRes;
  USHORT usFound = 0, usResLang;

  while (FindResDll(ucDllName, sizeof(ucDllName), &usResLang, ucLangStr, sizeof(ucLangStr)))
    if (usResLang!=0) {
      if (usResLang==usLang)
        usFound = 1;
      else if (bEngOk && usResLang==EN && usFound==0)
        usFound = 2;
      else
        continue;
      strcpy(ucFoundDllName,ucDllName);
    }

  if (usFound && DosLoadModule(ucErr, sizeof(ucErr), ucFoundDllName, &hmodRes)==0) {
    if ((plswData->haccAlt = WinLoadAccelTable(plswData->hab,hmodRes,ID_ALTACCELTABLE))==0 ||
        (plswData->haccCtrl = WinLoadAccelTable(plswData->hab,hmodRes,ID_CTRLACCELTABLE))==0 ||
        (plswData->haccNoAlt = WinLoadAccelTable(plswData->hab,hmodRes,ID_NOALTACCELTABLE))==0) {
      DosFreeModule(hmodRes);
      return 0;
    } else {
      plswData->Settings.usLanguage = (usFound==1?usLang:EN);
      return hmodRes;
    }
  } else
    return 0;
}


/* this function is adapted from Roman Stangle's APM/2 package */
USHORT ProcessAPMOffRequest(USHORT usPowerState,USHORT usDevice)
{  APIRET rc=NO_ERROR;
   ULONG ulPacketSize, ulDataSize, ulVersion[2],ulAction=0;
   struct POWERRETURNCODE powerRC;
   struct SENDPOWEREVENT  sendpowereventAPM;
   HFILE hfileAPM;

   /* For /Shutdown "Request", we need Warp 4 otherwise at least a CHKDSK reliably occurs (if the request
    does work at all) */
   DosQuerySysInfo(QSV_VERSION_MAJOR, QSV_VERSION_MINOR, ulVersion, sizeof(ulVersion));
   if(usPowerState==POWERSTATE_OFF) {
     if((ulVersion[0]<0x14) || (ulVersion[1]<0x28))
       return 1;
   }

   if (DosOpen("\\DEV\\APM$", &hfileAPM, &ulAction, 0, FILE_NORMAL, OPEN_ACTION_OPEN_IF_EXISTS,
       OPEN_FLAGS_FAIL_ON_ERROR | OPEN_SHARE_DENYNONE | OPEN_ACCESS_READWRITE, NULL)!=NO_ERROR)
     return 2;

   memset(&sendpowereventAPM, 0, sizeof(sendpowereventAPM));
   powerRC.usReturnCode=0;
                                    /* Enable PWR MGMT function */
   sendpowereventAPM.usSubID=SUBID_ENABLE_POWER_MANAGEMENT;
   ulPacketSize=sizeof(sendpowereventAPM);
   ulDataSize=sizeof(powerRC);
   rc=DosDevIOCtl(hfileAPM, IOCTL_POWER, POWER_SENDPOWEREVENT, &sendpowereventAPM, ulPacketSize,
                  &ulPacketSize, &powerRC, ulDataSize, &ulDataSize);
   if(rc!=NO_ERROR || powerRC.usReturnCode!=POWER_NOERROR) {
     DosClose(hfileAPM);
     return 3;
   }

   DosSleep(1000);    /* need a delay before the set state call for some reason */
   /* Invoke APM request */
   memset(&sendpowereventAPM, 0, sizeof(sendpowereventAPM));
   powerRC.usReturnCode=0;
   sendpowereventAPM.usSubID=SUBID_SET_POWER_STATE;
   sendpowereventAPM.usData1=usDevice;
   sendpowereventAPM.usData2=usPowerState;
   ulPacketSize=sizeof(sendpowereventAPM);
   ulDataSize=sizeof(powerRC);
   rc=DosDevIOCtl(hfileAPM, IOCTL_POWER, POWER_SENDPOWEREVENT,&sendpowereventAPM, ulPacketSize,
                  &ulPacketSize, &powerRC, ulDataSize, &ulDataSize);
   DosClose(hfileAPM);

   if(rc!=NO_ERROR || powerRC.usReturnCode!=POWER_NOERROR)
     return 4;
   else
     return 0;
}

VOID FillRectGradient(HPS hps, RECTL *rcl, RECTL *rclClip, LONG RGBDark, LONG RGBLight, BOOL bReverse)
{ USHORT k,usBorderHgt,usRDark,usGDark,usBDark,usRLight,usGLight,usBLight,usYDiff;
  float fRGrad,fGGrad,fBGrad;
  POINTL ptl;

  usRDark=(RGBDark&0xFF0000)>>16,
  usGDark=(RGBDark&0xFF00)>>8,
  usBDark=(RGBDark&0xFF);
  usRLight=(RGBLight&0xFF0000)>>16,
  usGLight=(RGBLight&0xFF00)>>8,
  usBLight=(RGBLight&0xFF);
  usYDiff=max(1,rcl->yTop-rcl->yBottom);
  fRGrad=(float)(usRLight-usRDark)/(float)usYDiff;
  fGGrad=(float)(usGLight-usGDark)/(float)usYDiff;
  fBGrad=(float)(usBLight-usBDark)/(float)usYDiff;

  for (k=0;k<rcl->yTop-rcl->yBottom;k++) {
    ptl.x=rcl->xLeft;
    ptl.y=bReverse?rcl->yTop-k-1:rcl->yBottom+k;
    if (rclClip!=NULL && (ptl.y<rclClip->yBottom || ptl.y>rclClip->yTop)) continue;
    GpiMove(hps,&ptl);
    ptl.x=rcl->xRight-1;
    GpiSetColor(hps,(int)(usRDark+fRGrad*k)*65536+(int)(usGDark+fGGrad*k)*256+(int)(usBDark+fBGrad*k));
    GpiLine(hps,&ptl);
  }
}

LONG CalcBrightCol(LONG lColor,UCHAR ucBright)
{ UCHAR ucR,ucG,ucB,ucMax;
  LONG lBrightCol;

  ucB=lColor&0xFF;  ucG=(lColor&0xFF00)>>8;  ucR=(lColor&0xFF0000)>>16;
  ucMax=max(1,max(ucR,max(ucG,ucB)));
  return (ucBright*ucR/ucMax*65536+ucBright*ucG/ucMax*256+ucBright*ucB/ucMax);
}

static MRESULT EXPENTRY PrtyDlgProc(HWND hwnd,ULONG msg,MPARAM mp1,MPARAM mp2)
{ static HWND hwndPrty;
  static PID pidPrty;

  switch (msg) {
  case WM_INITDLG: {
    USHORT usPrty;
    SHORT sPrtyC, sPrtyD;
    UCHAR ucBuf[128],ucTitle[NAMELEN];
    HPS hps;
    HWND hwndTBar;
    RECTL rcl;
    POINTL aptl[TXTBOX_COUNT];

    LSWDATA *plswData;

    SetControlsFont(hwnd,TRUE);

    plswData = (LSWDATA*)PVOIDFROMMP(mp2);
    hwndPrty = plswData->TaskArr[plswData->iMenuAtItem].hwnd;
    pidPrty = plswData->TaskArr[plswData->iMenuAtItem].pid;

    WinQueryDlgItemText(hwnd,FID_TITLEBAR,sizeof(ucBuf),ucBuf);
    GetItemTitle(plswData->TaskArr[plswData->iMenuAtItem].hsw,ucTitle,sizeof(ucTitle),TRUE);
    strncat(ucBuf,": ",sizeof(ucBuf)-strlen(ucTitle)-1);
    strncat(ucBuf,ucTitle,sizeof(ucBuf)-strlen(ucTitle)-1);

    hwndTBar=WinWindowFromID(hwnd,FID_TITLEBAR);
    hps=WinGetPS(hwndTBar);
    WinQueryWindowRect(hwndTBar,&rcl);
    GpiQueryTextBox(hps,1,"W",TXTBOX_COUNT,aptl);

    MakeFitStr(hps,ucBuf,sizeof(ucBuf), rcl.xRight-rcl.xLeft-aptl[TXTBOX_BOTTOMRIGHT].x-aptl[TXTBOX_BOTTOMLEFT].x);
    WinReleasePS(hps);

    WinSetDlgItemText(hwnd,FID_TITLEBAR,ucBuf);

    DosGetPrty(PRTYS_PROCESS, &usPrty, pidPrty);
    sPrtyC = (usPrty+PRTYD_MAXIMUM)>>8;
    sPrtyD = usPrty-(sPrtyC<<8);

    WinCheckButton(hwnd, sPrtyC==PRTYC_IDLETIME?RAD_IDLETIME:
                         sPrtyC==PRTYC_REGULAR?RAD_REGULAR:
                         sPrtyC==PRTYC_TIMECRITICAL?RAD_CRITICAL:RAD_FGNDSERVER,TRUE);

    WinSendDlgItemMsg(hwnd,SPIN_DELTA,SPBM_SETLIMITS,MPFROMLONG(PRTYD_MAXIMUM),MPFROMLONG(PRTYD_MINIMUM));
    WinSendDlgItemMsg(hwnd,SPIN_DELTA,SPBM_SETTEXTLIMIT,MPFROMSHORT(3),0);
    WinSendDlgItemMsg(hwnd,SPIN_DELTA,SPBM_SETCURRENTVALUE,MPFROMLONG(sPrtyD),0);

    break;
  }
  case WM_COMMAND:
    if (SHORT1FROMMP(mp1)==DID_OK) {
      SHORT sPrtyC, sPrtyD;
      BOOL bDesc;

      WinSendDlgItemMsg(hwnd,SPIN_DELTA,SPBM_QUERYVALUE,MPFROMP(&sPrtyD),MPFROM2SHORT(0,SPBQ_ALWAYSUPDATE));
      if (WinQueryButtonCheckstate(hwnd,RAD_IDLETIME)) sPrtyC=PRTYC_IDLETIME;
      else if (WinQueryButtonCheckstate(hwnd,RAD_REGULAR)) sPrtyC=PRTYC_REGULAR;
      else if (WinQueryButtonCheckstate(hwnd,RAD_CRITICAL)) sPrtyC = PRTYC_TIMECRITICAL;
      else sPrtyC = PRTYC_FOREGROUNDSERVER;

      bDesc = (WinQueryButtonCheckstate(hwnd,CHK_DESCENDANTS));

      WinPostMsg(hwndPrty,WinFindAtom(WinQuerySystemAtomTable(),LSWM_SETPRIORITY),
                 MPFROM2SHORT(((sPrtyC<<8)+sPrtyD)|(bDesc?0x8000:0),0),
                 MPFROMLONG(pidPrty));
    }
  default:
    return WinDefDlgProc(hwnd, msg, mp1, mp2);
  }
  return 0;
}

static MRESULT EXPENTRY RunDlgProc(HWND hwnd,ULONG msg,MPARAM mp1,MPARAM mp2)
{ static HWND hwndList;
  static UCHAR ucFName[_MAX_PATH];

  switch (msg) {
  case WM_INITDLG: {
    UCHAR *ucBuf;
    HFILE hFile;
    ULONG ulAction,ulcbRead,k,pos;
    APIRET rc;

    SetControlsFont(hwnd,FALSE);
    hwndList=WinWindowFromID(hwnd,CB_RUNCOMMAND);
    WinSendMsg(hwndList,EM_SETTEXTLIMIT,MPFROMSHORT(_MAX_PATH),0);
    GetStartupDir(ucFName,sizeof(ucFName));
    strncat(ucFName,HSTFILENAME,sizeof(ucFName)-strlen(ucFName)-1);
    rc=DosOpen(ucFName,&hFile,&ulAction,0,FILE_ARCHIVED|FILE_NORMAL,
               OPEN_ACTION_OPEN_IF_EXISTS|OPEN_ACTION_FAIL_IF_NEW,
               OPEN_FLAGS_FAIL_ON_ERROR|OPEN_SHARE_DENYWRITE|OPEN_ACCESS_READWRITE,0);
    if (rc==0) {
      if ((ucBuf=malloc((_MAX_PATH+2)*MAXRUNLISTHST))==NULL) {
        DosClose(hFile);
        break;
      }
      DosRead(hFile,ucBuf,(_MAX_PATH+2)*MAXRUNLISTHST,&ulcbRead);
      DosClose(hFile);
      for (k=0,pos=0;k<ulcbRead;k++)
        if (ucBuf[k]=='\n') {
          ucBuf[k]=ucBuf[k-1]='\0';
          WinSendMsg(hwndList,LM_INSERTITEM,MPFROMSHORT(LIT_END),MPFROMP(&ucBuf[pos]));
          pos=k+1;
        }
      free(ucBuf);
      WinSendMsg(hwndList,LM_SELECTITEM,MPFROMSHORT(0),MPFROMSHORT(TRUE));
    }

    break;
  }

  case WM_COMMAND:
    switch (SHORT1FROMMP(mp1)) {
    case DID_OK: {
      UCHAR ucCmd[_MAX_PATH+2],ucErrMsg[128],ucItemText[_MAX_PATH];
      USHORT k,usItemCount;
      SHORT sIndex;
      ULONG ulcbWrite,ulAction;
      APIRET rc;
      HFILE hFile;
      LSWDATA *plswData;

      WinQueryWindowText(hwndList,sizeof(ucCmd), ucCmd);
      strcpy(ucItemText,ucCmd); //RunCommand changes ucCmd

      if (DosGetNamedSharedMem((PVOID*)&plswData,SHAREMEM_NAME,PAG_READ | PAG_WRITE)==0 &&
          (rc=RunCommand(plswData,ucCmd,ucErrMsg,sizeof(ucErrMsg)))!=0) {
        WinMessageBox(HWND_DESKTOP,HWND_DESKTOP,ucErrMsg,PGMNAME,0,MB_ERROR | MB_CANCEL);
        DosFreeMem(plswData);
        break;
      }

      sIndex=(SHORT)WinSendMsg(hwndList,LM_SEARCHSTRING,MPFROM2SHORT(0,LIT_FIRST),MPFROMP(ucItemText));
      if (sIndex!=LIT_NONE)
        WinSendMsg(hwndList,LM_DELETEITEM,MPFROMSHORT(sIndex),0);
      WinSendMsg(hwndList,LM_INSERTITEM,MPFROMSHORT(0),MPFROMP(ucItemText));

      rc=DosOpen(ucFName,&hFile,&ulAction,0,FILE_ARCHIVED|FILE_NORMAL,
                 OPEN_ACTION_REPLACE_IF_EXISTS|OPEN_ACTION_CREATE_IF_NEW,
                 OPEN_FLAGS_FAIL_ON_ERROR|OPEN_SHARE_DENYWRITE|OPEN_ACCESS_READWRITE,0);
      if (rc==0) {
        usItemCount=LONGFROMMR(WinSendMsg(hwndList,LM_QUERYITEMCOUNT,0,0));
        for (k=0;k<usItemCount && k<MAXRUNLISTHST;k++) {
          WinSendMsg(hwndList,LM_QUERYITEMTEXT,MPFROM2SHORT(k,sizeof(ucItemText)),MPFROMP(ucItemText));
          sprintf(ucCmd,"%s\r\n",ucItemText);
          DosWrite(hFile,ucCmd,strlen(ucCmd),&ulcbWrite);
        }
        DosClose(hFile);
      }

      return WinDefDlgProc(hwnd, msg, mp1, mp2);
    }
    case PB_BROWSE: {
      static FILEDLG fild={0};
      HWND hwndFDlg;
      SHORT k;

      fild.cbSize = sizeof(FILEDLG);
      fild.fl = FDS_CENTER | FDS_OPEN_DIALOG ;
      hwndFDlg = WinFileDlg(HWND_DESKTOP, hwnd, &fild);
      if (hwndFDlg!=NULLHANDLE && (fild.lReturn == DID_OK))
        WinSetWindowText(hwndList,(PCHAR)PVOIDFROMMP(fild.szFullFile));
      for (k=strlen(fild.szFullFile);k>=0;k--)
        if (fild.szFullFile[k]=='\\') {
          fild.szFullFile[k+1]='\0';
          strcat(fild.szFullFile,"*.EXE");
          break;
        }

      break;
    }
    case DID_CANCEL:
      WinSendDlgItemMsg(hwnd,CB_RUNCOMMAND,CBM_SHOWLIST,MPFROMSHORT(FALSE),0);
      return WinDefDlgProc(hwnd, msg, mp1, mp2);

    default:
      return WinDefDlgProc(hwnd, msg, mp1, mp2);
    }
    break;
  default:
    return WinDefDlgProc(hwnd, msg, mp1, mp2);
  }
  return 0;
}

VOID ProcessCommand(USHORT cmd, USHORT src, LSWDATA *plswData, BOOL bTaskBar, BOOL bAll)
{
  switch (cmd) {
  case CMD_CANCELPOPUP:
  case CMD_CANCELPOPUP1:
    plswData->usCurrItem = MAXITEMS-1;

  case CMD_SWITCHFROMPM:
    if (plswData->bNowActive) {
      WinStopTimer(plswData->hab,plswData->hwndPopClient,0);
      WinSetWindowPos(plswData->hwndPopup,HWND_BOTTOM, POPUPWINHIDEPOSX,
                      POPUPWINHIDEPOSY, 0, 0, SWP_MOVE | SWP_SIZE | SWP_ZORDER);
      ShowBubble(plswData,0,0,0,-1,NULL);
    }

    if (src==CMDSRC_MENU) plswData->usCurrItem = plswData->iMenuAtItem;
//no break here
  case CMD_SWITCHFROMFS:
    /* turn the flag off only after the window has been repositioned or
     our subclassed frame window function won't let do this */
    plswData->bNowActive = FALSE;

    if (cmd != CMD_CANCELPOPUP && cmd != CMD_CANCELPOPUP1) {
      if (plswData->Settings.bDesktopMinimizes && IsDesktop(plswData->TaskArr[plswData->usCurrItem].hwnd)) {
        MinimizeHideAll(plswData,FALSE,0);
        break; //switching to desktop is done in MinimizeHideAll function
      }

      // for some weird reason WinSwitchToProgram does not always bring the window to top
      // showing is needed for hidden windows and won't hurt in other cases
      WinSetWindowPos(plswData->TaskArr[plswData->usCurrItem].hwnd,HWND_TOP,0,0,0,0,SWP_SHOW|SWP_ZORDER);
    }

    /* don't switch if the popup has been cancelled and  current program
     has been minimized or hidden or if a close window query has popped up and
     changed the focus to itself (and the POPUP_CANCEL has been triggered by
     the WM_TIMER) after the close command has been sent to a window */

    if (cmd != CMD_CANCELPOPUP1) {
      if (cmd != CMD_CANCELPOPUP ||
          ((plswData->TaskArr[plswData->usCurrItem].fl & (SWP_HIDE | SWP_MINIMIZE))==0) ||
          ((plswData->TaskArr[plswData->usCurrItem].ulType==PROG_FULLSCREEN ||
            plswData->TaskArr[plswData->usCurrItem].ulType==PROG_VDM) &&
           plswData->Settings.bPMPopupInFS))

// if we want to switch to the currently active window, which may be required if it's in
// another virtual desktop, activate desktop first, otherwise WinSwitchToProgram will not work
        if (WinQueryActiveWindow(HWND_DESKTOP)==plswData->TaskArr[plswData->usCurrItem].hwnd)
          WinSetActiveWindow(HWND_DESKTOP,plswData->TaskArr[plswData->usItems-1].hwnd);

        if (plswData->TaskArr[plswData->usCurrItem].hsw != 0)
          WinSwitchToProgram(plswData->TaskArr[plswData->usCurrItem].hsw);
        else
          WinSetWindowPos(plswData->TaskArr[plswData->usCurrItem].hwnd,
                          HWND_TOP,0,0,0,0,SWP_ACTIVATE | SWP_ZORDER);

      /* this is needed to make a full screen session current */
      if  (plswData->TaskArr[plswData->usCurrItem].ulType==PROG_FULLSCREEN ||
           plswData->TaskArr[plswData->usCurrItem].ulType==PROG_VDM)
        WinSetWindowPos(plswData->TaskArr[plswData->usCurrItem].hwnd,HWND_TOP,
                        0,0,0,0,SWP_ZORDER | SWP_ACTIVATE);
    }
    break;

  case CMD_SHOWSETTINGS:
    if (!plswData->bNowActive) EditSettings(plswData);
    break;

  case CMD_PRIORITY:
    if (src==CMDSRC_ACCELERATOR) {
      if (!MenuNeedsItem(plswData,plswData->usCurrItem,CMD_PRIORITY,NULL,0,FALSE))
        break;

      plswData->iMenuAtItem = plswData->usCurrItem;
    }

    WinDlgBox(HWND_DESKTOP,HWND_DESKTOP,PrtyDlgProc,plswData->hmodRes,DLG_PRTY,plswData);
    break;

  case CMD_ADDFILTER: {
    static ENTRYNAME ucName,ucName1;
    USHORT k;

    if (src==CMDSRC_ACCELERATOR)
      plswData->iMenuAtItem = plswData->usCurrItem;

    GetItemTitle(plswData->TaskArr[plswData->iMenuAtItem].hsw,ucName,sizeof(ucName),FALSE);
    if (AddFilter(bTaskBar?&plswData->Settings.SkipListTskBar:&plswData->Settings.SkipListPopup,ucName)) {
      if (WinIsWindow(plswData->hab,plswData->hwndParamDlg))
        WinPostMsg(plswData->hwndParamDlg,LSWM_UPDATEEXCLUDEDLG,0,MPFROMSHORT(bTaskBar?UPDATETSKBARLIST:UPDATEPOPUPLIST));

      for (k=0;k<plswData->usItems;k++) { //check for entries with the same name and update popup window or taskbar
        GetItemTitle(plswData->TaskArr[k].hsw,ucName1,sizeof(ucName1),FALSE);
        if (strcmp(ucName,ucName1)==0)
          WinPostMsg(bTaskBar?plswData->hwndTaskBarClient:plswData->hwndPopClient,LSWM_SWITCHLISTCHANGED, MPFROMLONG(2), MPFROMLONG(plswData->TaskArr[k].hsw));
      }
    }

    break;
  }

  case CMD_HIDE:
  case CMD_SHOW:
  case CMD_RESTORE:
  case CMD_MINIMIZE:
  case CMD_MAXIMIZE:
  case CMD_MOVE:
  case CMD_CLOSE:
  case CMD_KILL:
  case CMD_DEATH:
  case CMD_CLOSEQUIT:
    if (bAll && cmd>=CMD_HIDE && cmd<=CMD_DEATH) {
      SHORT k,item;
      LHANDLE hMte1;
      SWCNTRL swctl1,swctl2;

      item=src==CMDSRC_MENU?plswData->iMenuAtItem:plswData->usCurrItem;
      hMte1=HmteFromPID(plswData->TaskArr[item].pid);
      WinQuerySwitchEntry(WinQuerySwitchHandle(plswData->TaskArr[item].hwnd,0),&swctl1);

      for (k=plswData->usItems-1;k>=0;k--)
        if (hMte1==HmteFromPID(plswData->TaskArr[k].pid)&&
            !IsDesktop(plswData->TaskArr[k].hwnd) &&
            (WinQuerySwitchEntry(WinQuerySwitchHandle(plswData->TaskArr[k].hwnd,0),&swctl2),
             swctl2.bProgType==swctl1.bProgType) &&
            (cmd!=CMD_CLOSE||(cmd==CMD_CLOSE && WndHasControl(plswData->hab,plswData->TaskArr[k].hwnd,SC_CLOSE))))
          ChangeWindowPos(plswData, k,cmd);
    } else
      /* if this command comes from the keyboard use plswData->usCurrItem */
      ChangeWindowPos(plswData, src==CMDSRC_MENU ? plswData->iMenuAtItem : plswData->usCurrItem,cmd);
    break;

  case CMD_RUN: {
    static HWND hwndRunDlg=NULLHANDLE;

    if (!WinIsWindow(plswData->hab,hwndRunDlg))
      hwndRunDlg=WinLoadDlg(HWND_DESKTOP,NULLHANDLE,RunDlgProc,plswData->hmodRes,DLG_RUN,NULL);
    if (hwndRunDlg != NULLHANDLE) {
      WinSetWindowPos(hwndRunDlg,HWND_TOP,0,0,0,0,SWP_SHOW | SWP_ZORDER | SWP_ACTIVATE);
      WinProcessDlg(hwndRunDlg);
      WinDestroyWindow(hwndRunDlg);
    }
    break;
  }

  case CMD_QUIT:
    WinPostMsg(plswData->hwndPopup, WM_QUIT, 0, 0);
    break;
  }
}

LHANDLE HmteFromPID(PID pid)
{ UCHAR *pBuf;
  QSPTRREC *pRecHead;
  QSPREC *pApp;
  LHANDLE hmte;

  pBuf = (char*) malloc(0x8000); memset(pBuf,0,0x8000);
  DosQuerySysState(QS_PROCESS, 0, 0, 0, (char *) pBuf, 0x8000); // Get processes
  for (pRecHead = (QSPTRREC *)pBuf,pApp = pRecHead->pProcRec;
       pApp!=NULL && pApp->RecType == 1;
       pApp=(QSPREC *)((pApp->pThrdRec) + pApp->cTCB)) { // While its a process record
    if (pApp->pid==pid) {
      hmte=pApp->hMte;
      break;
    }
  }
  free(pBuf);
  return hmte;
}

PSZ GetDataFName(ULONG ObjID)
{ UCHAR fpath[_MAX_PATH], fname[_MAX_FNAME];

  if (!GetObjectName(ObjID, fpath, _MAX_PATH)) return NULL;

  if (EAQueryString(fpath, ".LONGNAME", sizeof(fname), fname)!=0)
    _splitpath(fpath, NULL, NULL, fname, NULL);

  return strdup(fname);
}

/**********************************************************************
* Check the EA's of a file; adapted from Hektools package
**********************************************************************/
/*
PFEA2LIST GetLongFName(PSZ pszName)
{ ULONG ulDenaCnt, ulActionTaken;
  APIRET rc;
  PFEA2LIST pFea2List;
  PDENA2 pDenaBlock, pDena;
  ULONG  ulFEASize, ulGEASize, ulIndex;
  EAOP2  EAop2;
  PGEA2  pGea;


  ulDenaCnt = (ULONG)-1;
  if ((pDenaBlock=calloc(1,2000))=NULL)
    return 1;

  rc = DosEnumAttribute(ENUMEA_REFTYPE_PATH, pszName, 1, pDenaBlock, 2000,
                        &ulDenaCnt, ENUMEA_LEVEL_NO_VALUE);
  if (rc || !ulDenaCnt) {
    free(pDenaBlock);
    return 2;
  }

  // Calculate size of needed buffers
  ulFEASize = sizeof (ULONG); ulGEASize = sizeof (ULONG);
  pDena = (PDENA2)pDenaBlock;
  for (ulIndex = 0; ulIndex < ulDenaCnt; ulIndex++) {
    if (strcmpi(pDena->szName,".LONGNAME")==0) {
      ULONG ulFSize = sizeof (FEA2) + pDena->cbName + pDena->cbValue;
      ULONG ulGSize = sizeof (GEA2) + pDena->cbName;
      ulFSize += (4 - (ulFSize % 4)); ulFEASize += ulFSize;
      ulGSize += (4 - (ulGSize % 4)); ulGEASize += ulGSize;
      break;
    }

    if (!pDena->oNextEntryOffset)
      break;
    pDena = (PDENA2) ((PBYTE)pDena + pDena->oNextEntryOffset);
  }

  // Allocate needed buffers
  if ((EAop2.fpGEA2List = calloc(1,ulGEASize))==NULL) {
    free(pDenaBlock);
    return 2;
  }
  if ((EAop2.fpFEA2List = calloc(1,ulFEASize))==NULL) {
    free(EAop2.fpGEA2List); free(pDenaBlock);
    return 2;
  }

  // Build the PGEALIST
  EAop2.fpGEA2List->cbList = ulGEASize; EAop2.fpFEA2List->cbList = ulFEASize;

  pDena = (PDENA2)pDenaBlock;
  pGea  = EAop2.fpGEA2List->list;
  pGea->cbName = pDena->cbName;
  memcpy(pGea->szName, pDena->szName, pDena->cbName + 1);

  rc = DosQueryPathInfo(pszName, FIL_QUERYEASFROMLIST, (PBYTE) &EAop2, sizeof EAop2);

  if (rc) {
    MessageBox(TITLE, "DosQueryXXXXInfo on %s :\n %s", pszName, GetOS2Error(rc));
    free(EAop2.fpGEA2List); free(EAop2.fpFEA2List); free(bDena);
    return NULL;
  }

   free(EAop2.fpGEA2List);
   return EAop2.fpFEA2List;
}
*/
