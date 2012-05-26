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

#ifndef TASKBAR_H_INCLUDED
#define TASKBAR_H_INCLUDED

#include "lswitch.h"


#define LSWTASKBARCLASS "lswTaskBarClass"

#define BUTTONBORDERWIDTH 1
#define TSKBARFRAMEWIDTH 1
#define MAXBUTTONWIDTH 130

#define BN_SWITCH 0x1001
#define BN_MINIMIZE 0x1002
#define BN_CLOSE 0x1003

#define DEFTSKBARRGBCOL3D      155*65536+170*256+180
#define NORMALBTNRGBCOL3D      170*65536+175*256+190
#define ACTIVEBTNRGBCOL3D      90*65536+100*256+110
#define RAISEDFRAMERGBCOL3D    200*65536+200*256+200
#define SUNKENFRAMERGBCOL3D    50*65536+50*256+50
#define NORMALBTNTITLERGBCOL3D 000*65536+000*256+000
#define ACTIVEBTNTITLERGBCOL3D 000*65536+000*256+180
#define HIDDENBTNTITLERGBCOL3D 100*65536+100*256+100

#define DEFTSKBARRGBCOL      200*65536+200*256+200
#define NORMALBTNRGBCOL      200*65536+200*256+200
#define ACTIVEBTNRGBCOL      255*65536+255*256+240
#define RAISEDFRAMERGBCOL    255*65536+255*256+255
#define SUNKENFRAMERGBCOL    50*65536+50*256+50
#define NORMALBTNTITLERGBCOL 000*65536+000*256+000
#define ACTIVEBTNTITLERGBCOL 000*65536+000*256+180
#define HIDDENBTNTITLERGBCOL 130*65536+130*256+130

#define LSWM_TSKBARMENU WM_USER+10
#define LSWM_INITBUTTONS WM_USER+11
#define LSWM_QUERYACTIVEBTNID WM_USER+12
#define LSWM_MOUSEOVERBTN WM_USER+13

#define LSWM_GETPROCCMD "LSWM_GETPROCCMD"

#define TSKBARUPDATETIMERID 1
#define TSKBARUPDATEINTERVAL 35
#define TSKBARBUBBLETIMERID 2

#define MIS_PROCESS 0x8000

typedef struct _SUBMENUITEMDATA {
  USHORT hsw;
  ULONG ulDataHandle;
  HPOINTER hIcon;
} SUBMENUITEMDATA;

typedef struct _BTNDATA {
  PFNWP pOldWndProc;
  HPOINTER hIcon,hObjIcon;
  ULONG flItemWnd;
  HWND hwndSubWinMenu,hwndFirstItem;
  USHORT usObjHandle;
//  PID pid;
  LHANDLE hMte;
  PCHAR pszObjName, pszDataFName, pszGroupTitle;
  BOOL bIconOnly;
} BTNDATA;

typedef struct _PROCDATA {
  PID pid,ppid;
  PSZ pszTitle;
  HPOINTER hIcon;
} PROCDATA;

VOID DoneTaskBar(LSWDATA *plswData);
SHORT InitTaskBar(LSWDATA* plswData,UCHAR *ucErrMsg,USHORT usMsgLen);
VOID AdjustDesktop(LSWDATA *plswData,BOOL bRestore);
MRESULT EXPENTRY TaskBarWndProc(HWND hwnd,ULONG msg,MPARAM mp1,MPARAM mp2);


#define WinSetDesktopWorkArea WINSETDESKTOPWORKAREA
#define WinQueryDesktopWorkArea WINQUERYDESKTOPWORKAREA

/*
 * WinSetDesktopWorkArea:
 *      undocumented Warp 4 API to change the "desktop work
 *      area", which, among other things, sets the size for
 *      maximized windows.
 *
 *      This is manually imported from PMMERGE.5468.
 */

BOOL APIENTRY WinSetDesktopWorkArea(HWND hwndDesktop, PWRECT pwrcWorkArea);

/*
 * WinQueryDesktopWorkArea:
 *      the reverse to the above.
 *
 *      This is manually imported from PMMERGE.5469.
 */

BOOL APIENTRY WinQueryDesktopWorkArea(HWND hwndDesktop, PWRECT pwrcWorkArea);

#endif
