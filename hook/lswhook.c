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


#define INCL_DOSMEMMGR
#define INCL_DOSMODULEMGR
#define INCL_WIN

#include "lswitch.h"
#include "taskbar.h"
#include <string.h>
#include <stdio.h>

/*
   Since the hooks may be called in the context of any thead having a message
   queue, we must use shared memory to access the globals of lSwitcher daemon.
   For the same reason the shared mem must be freed upon exiting the hook procs,
   otherwise the caller will retain a reference to it and the shared mem object
   will not be destroyed when lSwitcher exits
   IMPORTANT !!! Don't use DosGetNamedSharedMed/DosFreeMem before it's REALLY
   needed, check all other conditions first. These functions turn out to be
   expensive performance-wise

   With version 2.6 this DLL, if compiled with Watcom C, does not use C RTL
   (since Watcom RTL does not support multiple processes attached to a DLL
   with a single data segment, which we need). Because of this stack overflow
   is not checked for, and we want to reduce stack usage to a minimum
*/

HWND hwndMenuUp=NULLHANDLE,hwndTskBar=NULLHANDLE,hwndPopup=NULLHANDLE;
BOOL bWidget;

//simple and dirty string to integer conversion
ULONG __atol(UCHAR *s)
{ SHORT k;
  ULONG r,base;

  for (k=0;s[k]!='\0';k++);
  for (--k,base=1,r=0;k>=0;k--,base*=10)
    r+=(s[k]-48)*base;

  return r;
}

BOOL IsWindowClass(HWND hwnd,UCHAR *pszClassName)
{ static UCHAR ucBuf[32]; //reduce stack usage to minimum since we don't use stack probes
  USHORT k;
  BOOL bEqual;

  WinQueryClassName(hwnd,sizeof(ucBuf)-1,ucBuf);
/* equivalent of strcmp, but we don't want to use the RTL */
  for (k=0, bEqual=TRUE; ucBuf[k]!='\0' && pszClassName[k]!='\0'; k++)
    if (ucBuf[k]!=pszClassName[k]) {
       bEqual = FALSE;
       break;
    }

  return (bEqual && ucBuf[k]=='\0' && pszClassName[k]=='\0');
}

BOOL IsDesktop(HWND hwnd)
{
  return (hwnd==WinQueryWindow(HWND_DESKTOP,QW_BOTTOM) &&
          (IsWindowClass(hwnd,"wpFolder window")));
}

BOOL GetSharedMem(LSWDATA **plswData)
{
  return
   (DosGetNamedSharedMem((PVOID*)plswData,SHAREMEM_NAME,PAG_READ | PAG_WRITE)==0 &&
    plswData!=NULL);
}

VOID SetPrty(ULONG pid,USHORT usPrty)
{ SHORT sPrtyC, sPrtyD;
  BOOL bDesc;

  bDesc = (usPrty & 0x8000); usPrty &= ~0x8000;
  sPrtyC = (usPrty+PRTYD_MAXIMUM)>>8;
  sPrtyD = usPrty-(sPrtyC<<8);
  DosSetPriority(bDesc?PRTYS_PROCESSTREE:PRTYS_PROCESS,sPrtyC,sPrtyD,pid);
}

BOOL EXPENTRY lswInputHook(HAB hab, PQMSG pQmsg, ULONG fs)
{ LSWDATA *plswData;
  POINTL ptl;
  static BOOL bOnTop=FALSE;
  static USHORT usOldY=0xFFFF,cyScreen=0,cyTskBar=0; // this and the previous lines need to be static
  static RECTL rcl; //reduce stack usage to minimum since we don't use stack probes
  static HWND hwndBtn=0;
  PPIB ppib;

  DosGetInfoBlocks(NULL,&ppib); if (ppib->pib_flstatus&PS_EXITLIST) return FALSE;

  if (cyScreen==0 || pQmsg->msg==WM_SYSVALUECHANGED)
    cyScreen=WinQuerySysValue(HWND_DESKTOP,SV_CYSCREEN);

// get taskbar hwnd in case it changed
  if (pQmsg->msg==WM_NULL && pQmsg->hwnd==0 && GetSharedMem(&plswData)) {
    hwndTskBar = plswData->hwndTaskBarClient;
    DosFreeMem(plswData);
  }

  if (pQmsg->msg==WM_MOUSEMOVE) {
    if (pQmsg->hwnd!=hwndBtn) {
      if (WinQueryWindow(pQmsg->hwnd,QW_PARENT)==hwndTskBar && IsWindowClass(pQmsg->hwnd,"#3")) {
        hwndBtn = pQmsg->hwnd;
        WinPostMsg(hwndTskBar,LSWM_MOUSEOVERBTN,MPFROMHWND(hwndBtn),0);
      } else if (hwndBtn!=0) {
        hwndBtn = 0;
        WinPostMsg(hwndTskBar,LSWM_MOUSEOVERBTN,MPFROMHWND(hwndBtn),0);
      }
    }

    if (!bWidget && hwndMenuUp==NULLHANDLE && (WinQueryPointerPos(HWND_DESKTOP,&ptl),ptl.y!=usOldY)) {
      usOldY=ptl.y;
      if ((!bOnTop && (ptl.y==0 || ptl.y==cyScreen-1)) ||
          (bOnTop && ptl.y>2*cyTskBar && ptl.y<cyScreen-2*cyTskBar)) {

        if (!GetSharedMem(&plswData)) return FALSE;

        if ((!bOnTop && plswData->Settings.bTaskBarTopScr ^ ptl.y==0) ||
            (bOnTop && !WinIsWindowVisible(plswData->hwndMenu))) {
          if (!bOnTop) {
            WinQueryWindowRect(plswData->hwndTaskBar,&rcl);
            cyTskBar = rcl.yTop;
          }
          WinSetWindowPos(plswData->hwndTaskBar,bOnTop?HWND_BOTTOM:HWND_TOP,0,0,0,0,
                          SWP_ZORDER|(plswData->Settings.bTaskBarAlwaysVisible?0:
                                      (bOnTop?SWP_HIDE:SWP_SHOW)));
          bOnTop^=TRUE;
        }
        DosFreeMem(plswData);
      }
    }
  } else if (pQmsg->msg==WinFindAtom(WinQuerySystemAtomTable(), LSWM_GETPROCCMD) &&
      GetSharedMem(&plswData)) {
    UCHAR *ucHandle;
    USHORT usHandle=0;

    strncpy(plswData->buf,&ppib->pib_pchcmd[strlen(ppib->pib_pchcmd)+1],sizeof plswData->buf);
    if (DosScanEnv("WP_OBJHANDLE",&ucHandle)==0 && ucHandle!=NULL)
//      usHandle=(USHORT)__atol(ucHandle);
      strncpy(&plswData->buf[strlen(plswData->buf)+1],ucHandle,sizeof plswData->buf-strlen(plswData->buf)-2);

    DosFreeMem(plswData);
    WinPostMsg(hwndTskBar,LSWM_SWITCHLISTCHANGED,MPFROMLONG(4),pQmsg->mp2);
    //don't understand why we need to pass this over, but there is some system instability otherwise
  } else if (pQmsg->msg==WinFindAtom(WinQuerySystemAtomTable(),LSWM_SETPRIORITY)) {
    SetPrty(LONGFROMMP(pQmsg->mp2),SHORT1FROMMP(pQmsg->mp1));
    return TRUE;
  }

  /* else if (pQmsg->msg==WM_PAINT && WinIsWindowShowing(hwndPopup)) {
    RECTL rcl1,rcl2,rcl3,rcl4;
    USHORT usShadowSize;

    WinQueryWindowRect(pQmsg->hwnd,&rcl1);
    WinMapWindowPoints(pQmsg->hwnd,HWND_DESKTOP,(PPOINTL)&rcl1,2);
    WinQueryWindowRect(hwndPopup,&rcl2);
    WinMapWindowPoints(hwndPopup,HWND_DESKTOP,(PPOINTL)&rcl2,2);
    WinCopyRect(hab,&rcl3,&rcl2);
    usShadowSize=(rcl2.xRight-rcl2.xLeft)/(SHADOWSIZER+1);
    rcl2.xLeft=rcl2.xRight; rcl2.xRight+=usShadowSize;
    rcl3.yTop=rcl3.yBottom; rcl3.yBottom-=usShadowSize;
    if ((WinIntersectRect(hab,&rcl4,&rcl1,&rcl2),!WinIsRectEmpty(hab,&rcl4)))
      WinPostMsg(hwndPopup,WM_USER+101,MPFROMHWND(pQmsg->hwnd),0);
    else if ((WinIntersectRect(hab,&rcl4,&rcl1,&rcl3),!WinIsRectEmpty(hab,&rcl4)))
      WinPostMsg(hwndPopup,WM_USER+101,MPFROMHWND(pQmsg->hwnd),0);
  } */

  return FALSE;
}


/* we install this function as a pre-accelerator hook (undocumented). This hook
 is called even before an application accelerator table is translated, which helps
 to avoid compatibility problems with Ctrl-Tab hotkey */
BOOL EXPENTRY lswPreAccHook(HAB hab, PQMSG pQmsg, ULONG fs)
{ USHORT usFlags;
  UCHAR ucScan;
  LSWDATA *plswData;
  BOOL bRet;
  PPIB ppib;

  DosGetInfoBlocks(NULL,&ppib); if (ppib->pib_flstatus&PS_EXITLIST) return FALSE;

  if (pQmsg->msg!=WM_CHAR || (usFlags=SHORT1FROMMP(pQmsg->mp1), ucScan=CHAR4FROMMP(pQmsg->mp1),(usFlags & KC_SCANCODE))==0)
    return FALSE;

  bRet = FALSE;
/* tab w/ left Ctrl down. Trap the key either it's pressed or released,
 so that it does not reach other applications and trigger something*/
/* Alt-Tab might work in alternative shells */
  if (ucScan==SCAN_TAB && ((usFlags & KC_CTRL)!=0 ^ (usFlags & KC_ALT)!=0) &&
      GetSharedMem(&plswData)) {
    if ((plswData->Settings.ucPopupHotKey==0 && (usFlags & KC_ALT)) ||
        (plswData->Settings.ucPopupHotKey==1 && (usFlags & KC_CTRL))) {
      if (usFlags & KC_KEYUP)
        WinPostMsg(plswData->hwndPopClient,WM_COMMAND,MPFROMSHORT
                   ((usFlags & KC_SHIFT) ? CMD_SCROLLLEFT : CMD_SCROLLRIGHT),
                   MPFROM2SHORT(CMDSRC_OTHER,FALSE));
      bRet = TRUE;
    }
    DosFreeMem(plswData);
  }

/* Show settings */
  if (!(usFlags & KC_KEYUP) && (usFlags & KC_ALT) && (usFlags & KC_CTRL) && GetSharedMem(&plswData)) {
    if (ucScan == plswData->Settings.ucSettingsScan) {
      WinPostMsg(plswData->hwndPopClient,WM_COMMAND,MPFROMSHORT(CMD_SHOWSETTINGS),MPFROM2SHORT(CMDSRC_OTHER,FALSE));
      bRet = TRUE;
    }
    DosFreeMem(plswData);
  }

  return bRet;
}


VOID EXPENTRY lswSendHook(HAB hab, PSMHSTRUCT psmh, BOOL fInterTask)
{ LSWDATA *plswData;
  HSWITCH hsw;
  static SWCNTRL swctl; //reduce stack usage to a minimum since we don't use stack probes
  static SWP swp;
  ULONG ulFlags;
  BOOL bPosChange,bActiveChange;
  PPIB ppib;

  if (psmh->msg==SH_SWITCHLIST && IsWindowClass(psmh->hwnd,"WindowList") && //check msg target to avoid multiple messages
      (((LONG)psmh->mp1>=1&&(LONG)psmh->mp1<=3)||(LONG)psmh->mp1==0x10001) &&
      GetSharedMem(&plswData)) {
    if (plswData->bNowActive)          WinPostMsg(plswData->hwndPopClient, LSWM_SWITCHLISTCHANGED, psmh->mp1, psmh->mp2);
    if (plswData->Settings.bTaskBarOn) WinPostMsg(plswData->hwndTaskBarClient, LSWM_SWITCHLISTCHANGED,psmh->mp1, psmh->mp2);
    if (WinIsWindow(hab,plswData->hwndParamDlg)) WinPostMsg(plswData->hwndParamDlg, LSWM_SWITCHLISTCHANGED,psmh->mp1, psmh->mp2);
    DosFreeMem(plswData);
    return;
  }

  DosGetInfoBlocks(NULL,&ppib); if (ppib->pib_flstatus&PS_EXITLIST) return;

  if (psmh->msg==WM_COMMAND && IsWindowClass(psmh->hwnd,"AltTabWindow") &&
      (WinGetPhysKeyState(HWND_DESKTOP,0x5e) & 0x8000)==0 && // right Alt up
      GetSharedMem(&plswData)) {
    if (plswData->Settings.ucPopupHotKey==0 && plswData->Settings.bPMSwitcher) {
      BOOL bShift = (WinGetPhysKeyState(HWND_DESKTOP,0x2a) & 0x8000) || // either Shift down
                    (WinGetPhysKeyState(HWND_DESKTOP,0x36) & 0x8000);
      WinPostMsg(plswData->hwndPopClient,WM_COMMAND,MPFROMSHORT(bShift ? CMD_SCROLLLEFT : CMD_SCROLLRIGHT),MPFROM2SHORT(CMDSRC_OTHER,FALSE));
      psmh->msg = WM_NULL;
    }
    DosFreeMem(plswData);
    return;
  }

// make sure the taskbar does not show up and cover desktop menu
  if (psmh->msg==WM_INITMENU && IsDesktop(psmh->hwnd)) hwndMenuUp=HWNDFROMMP(psmh->mp2);
  if (psmh->msg==WM_MENUEND && HWNDFROMMP(psmh->mp2)==hwndMenuUp) hwndMenuUp=NULLHANDLE;


  if (psmh->msg==WM_SETICON && WinQuerySwitchHandle(psmh->hwnd,0)!=0 &&
      GetSharedMem(&plswData)) {
    if (plswData->bNowActive)          WinPostMsg(plswData->hwndPopClient,LSWM_WNDICONCHANGED,psmh->mp1,MPFROMHWND(psmh->hwnd));
    if (plswData->Settings.bTaskBarOn) WinPostMsg(plswData->hwndTaskBarClient,LSWM_WNDICONCHANGED,psmh->mp1,MPFROMHWND(psmh->hwnd));
    DosFreeMem(plswData);
    return;
  }


  /* make just minimized or hidden window appear after the current one in
   the switch list if the corresponding option is on in settings */
  if (psmh->msg==WM_ADJUSTFRAMEPOS &&
      (((PSWP)PVOIDFROMMP(psmh->mp1))->fl & (SWP_HIDE|SWP_MINIMIZE)) &&
      !(WinGetPhysKeyState(HWND_DESKTOP,0x38) & 0x8000) &&
      WinQuerySwitchHandle(psmh->hwnd,0)!=0 && IsWindowClass(psmh->hwnd,"#1") &&
      GetSharedMem(&plswData)) {

    if (plswData->Settings.bChangeZOrder)
      ((PSWP)PVOIDFROMMP(psmh->mp1))->hwndInsertBehind = HWND_TOP;
    DosFreeMem(plswData);
    return;
  }


  if (psmh->msg==WM_DESTROY && WinQueryWindow(psmh->hwnd,QW_PARENT)==WinQueryDesktopWindow(0,0) &&
      GetSharedMem(&plswData)) {
    USHORT k;
//the minimized window list holds the virtual desktop positions the windows were minimized from
    for (k=0;k<MINMAXITEMS;k++)
      if (psmh->hwnd==plswData->MinMaxWnd[2*k]) {
        plswData->MinMaxWnd[2*k]=0;
        break;
      }
    DosFreeMem(plswData);
    return;
  }

  if (psmh->msg==WM_MINMAXFRAME && WinQueryWindow(psmh->hwnd,QW_PARENT)==WinQueryDesktopWindow(0,0) &&
      GetSharedMem(&plswData)) {

    USHORT k;

    ulFlags=((PSWP)PVOIDFROMMP(psmh->mp1))->fl;
    WinQueryWindowPos(psmh->hwnd, &swp);

//the minimized window list holds the virtual desktop positions the windows were minimized from
    for (k=0;k<MINMAXITEMS;k++)
      if ((ulFlags & SWP_MAXIMIZE) && psmh->hwnd==plswData->MinMaxWnd[2*k]) {
        ((PSWP)PVOIDFROMMP(psmh->mp1))->x+=(WinQuerySysValue(HWND_DESKTOP,SV_CXSCREEN)+8)*
          (((plswData->MinMaxWnd[2*k+1]&0xFF00)>>8)-((plswData->lCurrDesktop&0xFF00)>>8));
        ((PSWP)PVOIDFROMMP(psmh->mp1))->y+=(WinQuerySysValue(HWND_DESKTOP,SV_CYSCREEN)+8)*
          ((plswData->MinMaxWnd[2*k+1]&0xFF)-(plswData->lCurrDesktop&0xFF));
        plswData->MinMaxWnd[2*k]=0;
        break;
      } else if ((ulFlags & SWP_MINIMIZE) && (swp.fl & SWP_MAXIMIZE) && plswData->MinMaxWnd[2*k]==0) {
        plswData->MinMaxWnd[2*k]=psmh->hwnd; plswData->MinMaxWnd[2*k+1]=plswData->lCurrDesktop;
        break;
      }

// reduce maximized window height (desktop work area) if this option is set
    if (!bWidget && (ulFlags & SWP_MAXIMIZE) && plswData->Settings.bTaskBarOn &&
        plswData->Settings.bReduceDsk && plswData->Settings.bTaskBarAlwaysVisible) {
      SHORT sHgt,sY,sCY,sCYScr=WinQuerySysValue(HWND_DESKTOP,SV_CYSCREEN);

      WinQueryWindowPos(plswData->hwndTaskBar, &swp);
      sHgt=(plswData->Settings.bTaskBarTopScr?sCYScr-swp.y:swp.y+swp.cy)-1;
      sY=((PSWP)PVOIDFROMMP(psmh->mp1))->y; sCY=((PSWP)PVOIDFROMMP(psmh->mp1))->cy;
      if ((plswData->Settings.bTaskBarTopScr && sY+sCY > sCYScr-sHgt) ||
          (!plswData->Settings.bTaskBarTopScr && sY < sHgt)) {
        ((PSWP)PVOIDFROMMP(psmh->mp1))->cy-=plswData->Settings.bTaskBarTopScr?sY+sCY-(sCYScr-sHgt):sHgt-sY;
        ((PSWP)PVOIDFROMMP(psmh->mp1))->y=plswData->Settings.bTaskBarTopScr?sY:sHgt;
      }
    }

    DosFreeMem(plswData);
    return;
  }

/* can't call WinPostMsg in WM_ACTIVATE hook because of some random crashes
  if (!bWidget && psmh->msg==WM_ACTIVATE) {
    WinPostMsg(hwndTskBar,LSWM_ACTIVEWNDCHANGED,0,0);
    return;
  }
*/

  if (psmh->msg==WM_WINDOWPOSCHANGED &&
      (ulFlags=((PSWP)PVOIDFROMMP(psmh->mp1))->fl,
       bPosChange=(ulFlags & (SWP_HIDE|SWP_SHOW|SWP_MINIMIZE|SWP_MAXIMIZE|SWP_RESTORE)),
       bActiveChange=(ulFlags & (SWP_ACTIVATE|SWP_DEACTIVATE|SWP_FOCUSACTIVATE|SWP_FOCUSDEACTIVATE)),

       (bPosChange && (hsw=WinQuerySwitchHandle(psmh->hwnd,0))!=0 &&
        (WinQuerySwitchEntry(hsw,&swctl),swctl.hwnd==psmh->hwnd)) || bActiveChange) &&
      GetSharedMem(&plswData)) {

//notify the lswitcher app of window state changes
    if (plswData->Settings.bTaskBarOn && bActiveChange)
      WinPostMsg(plswData->hwndTaskBarClient,LSWM_ACTIVEWNDCHANGED,MPFROMHWND(psmh->hwnd),psmh->mp1);
    if (bPosChange) {
      WinPostMsg(plswData->hwndPopClient, LSWM_WNDSTATECHANGED,
                 MPFROM2SHORT(hsw&0xFFFF,(SHORT)(ulFlags&0xFFFF)), MPFROMHWND(psmh->hwnd));
      if (plswData->Settings.bTaskBarOn)
        WinPostMsg(plswData->hwndTaskBarClient, LSWM_WNDSTATECHANGED,
                   MPFROM2SHORT(hsw&0xFFFF,(SHORT)(ulFlags&0xFFFF)), MPFROMHWND(psmh->hwnd));
    }
    DosFreeMem(plswData);
    return;
  }
}


LONG EXPENTRY lswHookInit(LSWDATA *plswData)
{
  bWidget=plswData->bWidget;

hwndTskBar=plswData->hwndTaskBarClient;
hwndPopup=plswData->hwndPopClient;

  if (DosQueryModuleHandle(DLL_NAME,&plswData->hmodHook)) return -1;

  if (!WinSetHook(plswData->hab,NULLHANDLE,HK_SENDMSG,(PFN)lswSendHook,plswData->hmodHook)) return -2;
  if (!WinSetHook(plswData->hab,NULLHANDLE,HK_PREACCEL,(PFN)lswPreAccHook,plswData->hmodHook)) return -3;
  if (!WinSetHook(plswData->hab,NULLHANDLE,HK_INPUT,(PFN)lswInputHook,plswData->hmodHook)) return -4;

  WinPostMsg(0,WM_NULL,0,0); //reset the hook

  return MAKEULONG(MAKEUSHORT(VERSIONMAJOR,VERSIONMINOR),MAKEUSHORT(REVISION,0));
}

VOID EXPENTRY lswHookTerm(LSWDATA *plswData)
{
  WinReleaseHook(plswData->hab,NULLHANDLE,HK_INPUT,(PFN)lswInputHook,plswData->hmodHook);
  WinReleaseHook(plswData->hab,NULLHANDLE,HK_PREACCEL,(PFN)lswPreAccHook,plswData->hmodHook);
  WinReleaseHook(plswData->hab,NULLHANDLE,HK_SENDMSG,(PFN)lswSendHook,plswData->hmodHook);
}

ULONG EXPENTRY lswHookGetVersion(void)
{
  return MAKEULONG(MAKEUSHORT(VERSIONMAJOR,VERSIONMINOR),MAKEUSHORT(REVISION,0));
}
