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

#ifndef COMMON_H_INCLUDED
#define COMMON_H_INCLUDED

#include "lswitch.h"

#define MAXRUNLISTHST 20
#define HSTFILENAME "lswrun.hst"
#define BUBBLETIMERINTERVAL 100

#define AdjustItem(Item,MaxVal)\
  Item += ((SHORT)Item < 0 ? -((SHORT)Item+1)/(SHORT)MaxVal+1 :\
           -(SHORT)Item/(SHORT)MaxVal)*(SHORT)MaxVal

#define MenuItemCount(hwndMenu) (SHORT1FROMMR(WinSendMsg(hwndMenu,MM_QUERYITEMCOUNT,0,0)))

LONG SwitcherInit(HAB hab, HMQ hmq, UCHAR *ucErrMsg, USHORT usMsgLen, PVOID *ppData, USHORT usFunc);
VOID SwitcherTerm(LSWDATA *plswData,USHORT usFunc);
VOID APIENTRY lswExitProc(VOID);
BOOL ChangeWindowPos(LSWDATA *plswData,SHORT iItemNum,USHORT cmd);
BOOL WndHasControl(HAB hab,HWND hwndToCheck,USHORT usControl);
BOOL MenuNeedsItem(LSWDATA *plswData,USHORT usItem,USHORT usId,UCHAR *pszTitle,USHORT usLen,BOOL bGroup);
BOOL IsMinToViewer(HWND hwnd,ULONG flopt);
VOID MinimizeHideAll(LSWDATA *plswData, BOOL bReset, HWND hwndReset);
VOID GetItemTitle(HSWITCH hsw,UCHAR *ucTitle,USHORT usLen,BOOL bSessNum);
HPOINTER GetWndIcon(HWND hwnd);
//PSZ GetObjectName(USHORT usObjHandle);
VOID ShowBubble(LSWDATA *plswData,SHORT iMouseIsAtItem,USHORT usX,USHORT usY,SHORT iFunc,UCHAR *ucTitle2);
VOID InitTaskActionsMenu(HWND hwndMenu,LSWDATA *plswData,SHORT iMenuAtItem,BOOL bTaskBar,BOOL bGroup);
VOID ShowMenu(LSWDATA *plswData,SHORT iMenuAtItem,BOOL bTaskBar,BOOL bGroup);
BOOL IsDesktop(HWND hwnd);
VOID InitTaskArr(LSWDATA *plswData, BOOL bFullScreen,BOOL bTaskBar,BOOL bUseFilters);
BOOL IsInSkipList(SKIPLIST *pSkipList,UCHAR *ucTitle);
BOOL AddFilter(SKIPLIST *pSkipList,UCHAR *ucName);
BOOL RemoveFilter(SKIPLIST *pSkipList,UCHAR *ucName);
BOOL IsWindowClass(HWND hwnd,UCHAR *pszClassName);
PVOID GetSwitchList(HAB hab,BOOL bInit,ULONG *ulItemCount);
ULONG MapCommand(USHORT cmd);
USHORT RunCommand(LSWDATA *plswData,UCHAR *ucCommand,UCHAR *ucErrMsg,USHORT usErrMsgLen);
VOID SetControlsFont(HWND hwnd,BOOL bDoTitleBar);
VOID GetStartupDir(UCHAR *ucDir,USHORT usLen);
BOOL queryAppInstance(VOID);
BOOL UpdateWinFlags(ULONG *OldFlags,ULONG NewFlags);
VOID MakeFitStr(HPS hps,UCHAR *ucStr,USHORT usStrLen, USHORT usStrWid);
USHORT FindResDll(UCHAR *ucDllName, USHORT usNameLen, USHORT *usLang, UCHAR *ucLangStr, USHORT usLangStrLen);
HMODULE LoadResource(USHORT usLang, LSWDATA *plswData, BOOL bEngOk);
USHORT ProcessAPMOffRequest(USHORT usPowerState,USHORT usDevice);
SHORT InsertMenuItem(HWND hwndMenu, HWND hwndSubMenu, SHORT iPosition, SHORT sItemId,
                     char *ItemTitle, SHORT afStyle, SHORT afAttr, ULONG hItem);
BOOL IsWPSPid(PID pid);
HFILE OpenXF86(VOID);
BOOL XF86Installed(VOID);
BOOL Death(PID pid);
VOID FillRectGradient(HPS hps, RECTL *rcl, RECTL *rclClip, LONG RGBDark, LONG RGBLight, BOOL bReverse);
LONG CalcBrightCol(LONG lColor,UCHAR ucBright);
VOID ProcessCommand(USHORT cmd, USHORT src, LSWDATA *plswData, BOOL bTaskBar, BOOL bAll);
BOOL IsInDesktop(HAB hab,HWND hwnd,LONG lCurrDesktop);
//LONG GetCurrDesktop(LSWDATA *plswData);
LHANDLE HmteFromPID(PID pid);
PSZ GetDataFName(ULONG ObjID);

#endif

