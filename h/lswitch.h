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

#ifndef LSWITCH_H_INCLUDED
#define LSWITCH_H_INCLUDED

#define INCL_DOSDEVICES
#define INCL_DOSMODULEMGR
#define INCL_DOSMISC
#define INCL_DOSNMPIPES
#define INCL_DOSPROCESS
#define INCL_DOSSEMAPHORES
#define INCL_DOSRESOURCES
#define INCL_DOSSESMGR
#define INCL_DOSERRORS
#define INCL_DOSPROFILE
#define INCL_VIO
#define INCL_KBD
#define INCL_WIN
#define INCL_PM

#include <os2.h>
#include <stdlib.h>
#include "api16.h"

//#define XWORKPLACE

#ifdef XWORKPLACE
  #include "center.h"
  #include "xwphook.h"

  #define WIDGETMODNAME "LSWIDGET.DLL"

//the widget DLL module handle
extern HMODULE hmodWidgetDll;

/* ******************************************************************
 *
 *   Function imports from XFLDR.DLL
 *
 ********************************************************************/

/*
 *      To reduce the size of the widget DLL, it is
 *      compiled with the VAC subsystem libraries.
 *      In addition, instead of linking frequently
 *      used helpers against the DLL again, we import
 *      them from XFLDR.DLL, whose module handle is
 *      given to us in the INITMODULE export.
 *
 *      Note that importing functions from XFLDR.DLL
 *      is _not_ a requirement. We only do this to
 *      avoid duplicate code.
 *
 *      For each funtion that you need, add a global
 *      function pointer variable and an entry to
 *      the G_aImports array. These better match.
 */

//these are included from kernel.h, which pulls a number of include files of
//which I could not find several for some reason
    MRESULT krnSendDaemonMsg(ULONG msg, MPARAM mp1, MPARAM mp2);
    typedef MRESULT KRNSENDDAEMONMSG(ULONG msg, MPARAM mp1, MPARAM mp2);
    typedef KRNSENDDAEMONMSG *PKRNSENDDAEMONMSG;

// resolved function pointers from XFLDR.DLL
extern PKRNSENDDAEMONMSG pkrnSendDaemonMsg;

#endif

#define VERSIONMAJOR 2
#define VERSIONMINOR 71
#define REVISION 1

#define LASTINIVERMAJOROK 2
#define LASTINIVERMINOROK 71
#define LASTINIREVISIONOK 1

#define LASTHOOKVERMAJOROK 2
#define LASTHOOKVERMINOROK 71
#define LASTHOOKREVISIONOK 1

#define PGMNAME "lSwitcher"
#define DLL_NAME "LSWHOOK"

#define LSWPOPUPCLASS "lswPopupClass"

#define ID_POPUPWIN 1
#define ID_ICON ID_POPUPWIN

#define ID_ALTACCELTABLE 2
#define ID_CTRLACCELTABLE 3
#define ID_NOALTACCELTABLE 4

#define ID_TITLESTR 4
#define ID_BUBBLE 5
#define ID_BUBBLEFRAME 6
#define ID_HINTSTR 7


/* these commands must have consecutive numbers and be in this order */
#define CMD_SWITCHFROMPM 51
#define CMD_HIDE 52
#define CMD_SHOW 53
#define CMD_RESTORE 54
#define CMD_MINIMIZE 55
#define CMD_MAXIMIZE 56
#define CMD_MOVE 57
#define CMD_CLOSE 58
#define CMD_CLOSEQUIT 59
#define CMD_KILL 60
#define CMD_DEATH 61
#define CMD_PRIORITY 62
/* end consecutive commands */

#define CMD_SCROLLLEFT 70
#define CMD_SCROLLRIGHT 71
#define CMD_SCROLLUP 72
#define CMD_SCROLLDOWN 73
#define CMD_CANCELPOPUP 74
#define CMD_SWITCHFROMFS 75
#define CMD_SCROLLRIGHTFROMFS 76
#define CMD_SCROLLLEFTFROMFS 77
#define CMD_RUN 78
#define CMD_SHOWSETTINGS 79
#define CMD_CANCELPOPUP1 80
#define CMD_QUIT 81
#define CMD_XCENTERSUBMENU 82
#define CMD_ADDFILTER 83
#define CMD_SUSPEND 84

#define CMD_FIRST CMD_SWITCHFROMPM
#define CMD_LAST  CMD_ADDFILTER


/*#ifdef XWORKPLACE
#define SHAREMEM_NAME "\\SHAREMEM\\LSWITCH\\LSWIDGET.M01"
#define SEMRUNNINGNAME "\\SEM32\\LSWITCH\\LSWIDGET.S03"
#define SEMFSPOPUPNAME "\\SEM32\\LSWITCH\\LSWIDGET.S04"
#define SEMSHIFTNAME "\\SEM32\\LSWITCH\\LSWIDGET.S06"
#define SEMCTRLTABNAME "\\SEM32\\LSWITCH\\LSWIDGET.S08"
#else*/
#define SHAREMEM_NAME "\\SHAREMEM\\LSWITCH\\LSWITCH.M01"
#define SEMRUNNINGNAME "\\SEM32\\LSWITCH\\LSWITCH.S03"
#define SEMFSPOPUPNAME "\\SEM32\\LSWITCH\\LSWITCH.S04"
#define SEMSHIFTNAME "\\SEM32\\LSWITCH\\LSWITCH.S06"
#define SEMCTRLTABNAME "\\SEM32\\LSWITCH\\LSWITCH.S08"
//#endif

/* pre-accelerator input hook, undocumented, */
#define HK_PREACCEL 17

/* this message is sent to WindowList when a switch entry is added,
   removed or modified; undocumented
   the first param of this msg: 0x1 or 0x10001 -- create switch entry
                                0x2 -- modify, 0x3 -- delete
   the second param is the corresponding switch handle*/
#define SH_SWITCHLIST 0x0080

/*undocumented WinDrawBorder extensions */
#define DB_RAISED 0x0400
#define DB_DEPRESSED 0x0800

#define DP_MINIICON 0x0004


#define DEFRAISEDFRAMECOLOR (ULONG)CLR_WHITE
#define DEFSUNKENFRAMECOLOR (ULONG)CLR_DARKGRAY
#define DEFBACKRGBCOLOR ((ULONG)150*65536+160*256+172)
#define DEFTITLERGBCOLOR ((ULONG)0*65536+0*256+191)
#define DEFHINTSRGBCOLOR ((ULONG)0*65536+0*256+0)
#define DEFBUBBLERGBCOLOR ((ULONG)224*65536+224*256+161)
#define DEFTITLEFONT3 "8.Helv"
#define DEFTITLEFONT4 "9.WarpSans"
#define DEFHINTSFONT "2.System VIO"


#define MAXITEMS 128

#define NAMELEN (MAXNAMEL+4+6)

#define LSWM_SWITCHLISTCHANGED WM_USER+1
#define LSWM_WNDSTATECHANGED WM_USER+2
#define LSWM_ACTIVEWNDCHANGED WM_USER+3
#define LSWM_WNDICONCHANGED WM_USER+4
#define LSWM_DOSEXITKILL WM_USER+5
#define LSWM_SETPRIORITY "LSWM_SETPRIORITY"

/* keyboard scan codes for particular commands */

/* these are the same for all languages */
#define SCAN_TAB 0x0f
#define SCAN_SPACE 0x39
#define SCAN_ALT 0x38
#define SCAN_CTRL 0x1d
#define SCAN_ENTER 0x1c
#define SCAN_RIGHTCTRL 0x5b
#define SCAN_RIGHTALT 0x5e
#define SCAN_ESC 0x1
#define SCAN_SETTINGS 0x1f /* letter s */
#define SCAN_UP 0x48
#define SCAN_DOWN 0x50
#define SCAN_LEFTSHIFT 0x2a
#define SCAN_RIGHTSHIFT 0x36
#define SCAN_WINLEFT 0x7e


#define SHADOWSIZER 55


typedef struct _SWITEM {
  HWND hwnd;
  HSWITCH hsw;
  ULONG fl,ulType;
  HPOINTER hIcon;
  PID pid;
} SWITEM;

typedef UCHAR ENTRYNAME[NAMELEN];

typedef UCHAR *SKIPLIST[MAXITEMS];

typedef struct _LSWSETTINGS {
  UCHAR
    ucVerMajor,
    ucVerMinor,
    ucRevision;
  BOOL
    bShowHidden,
    bShowViewer,
    bPMSwitcher,
    bOldPopup,
    bDesktopMinimizes,
    bScrollItems,
    bPMPopupInFS,
    bShowInWinList,
    bShowHints,
    bChangeZOrder,
    bTaskBarOn,
    bTaskBarTopScr,
    bTaskBarAlwaysVisible,
    bTaskBarAlwaysTop,
    bStickyPopup,
    bShowHiddenTskBar,
    bShowViewerTskBar,
    bIconsOnlyInTskBar,
    bReduceDsk,
    bFlatButtons,
    bGroupItems,
    bAllowResize,
    b3DTaskBar,
    bTskBarGrow;

  LONG
    lBackColor,
    lRaisedFrameColor,
    lSunkenFrameColor,
    lTitleRGBCol,
    lHintsRGBCol,
    lTskBarRGBCol, lTskBarRGBCol3D,
    lNormalBtnRGBCol, lNormalBtnRGBCol3D,
    lActiveBtnRGBCol, lActiveBtnRGBCol3D,
    lNormalHiddenBtnRGBCol, lNormalHiddenBtnRGBCol3D,
    lActiveHiddenBtnRGBCol, lActiveHiddenBtnRGBCol3D,
    lNormalBtnTitleRGBCol, lNormalBtnTitleRGBCol3D,
    lActiveBtnTitleRGBCol, lActiveBtnTitleRGBCol3D,
    lNormalHiddenBtnTitleRGBCol, lNormalHiddenBtnTitleRGBCol3D,
    lActiveHiddenBtnTitleRGBCol, lActiveHiddenBtnTitleRGBCol3D,
    lBubbleRGBCol,
    lBubbleTextRGBCol;
  UCHAR
    ucTitleFont[FACESIZE],
    ucHintsFont[FACESIZE],
    ucButtonFont[FACESIZE],
    ucSettingsKey,ucSettingsScan,
    ucPopupHotKey;//0-Alt+Tab, 1-Ctrl+Tab, 2-LeftWin+Tab
  SHORT
    sTskBarX,
    sTskBarY,
    sTskBarCX,
    sTskBarCY;
  USHORT
    usMaxBtnWid,
    usSwitchMEvent,
    usMinMEvent,
    usCloseMEvent,
    usScrollDiv,
    usLanguage;	
  SKIPLIST
    SkipListPopup,
    SkipListTskBar,
    ListIconsOnly;
} LSWSETTINGS;


#define MINMAXITEMS 128

typedef struct _LSWDATA {
  LSWSETTINGS Settings;

  BOOL
    bNowActive,
    bWidget,
    bSettingsDlgOn,
    bReserved1;

  HAB hab;
  HMQ hmq;
  HMODULE hmodHook,hmodRes;
  HWND
    hwndPopup,
    hwndPopClient,
    hwndTaskBar,
    hwndTaskBarClient,
    hwndBubble,
    hwndMenu, hwndTaskMenu,
    hwndParamDlg;
  HSWITCH
    hswitch;
  HACCEL
    haccAlt,
    haccCtrl,
    haccNoAlt;
  HEV
    hevRunning,
    hevCtrlTab,
    hevShift,
    hevPopup;
  HMTX
    htmxLswData;
  SHORT
    itidFSDispat,
    itidFSMon,
    iMenuAtItem;

  SWITEM TaskArr[MAXITEMS];
  USHORT
    usItems,
    usCurrItem;

  SHORT iShift;
//virtual desktop number relative to the one in which the program was started, starts at 0,0
  LONG lCurrDesktop;
//the list of minimized maximzed windows with the corresponding virtual desktop numbers
  LONG MinMaxWnd[MINMAXITEMS*2];

  UCHAR buf[1024];
// the following must be the last field
#ifdef XWORKPLACE
  PXCENTERWIDGET pWidget;
#else
  PVOID pDummy;
#endif
} LSWDATA;

typedef LSWDATA *PLSWDATA;


LONG EXPENTRY lswHookInit(LSWDATA *plswData);
VOID EXPENTRY lswHookTerm(LSWDATA *plswData);
ULONG EXPENTRY lswHookGetVersion(void);

#endif
