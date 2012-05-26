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

#include <stdio.h>
#include <string.h>
#include "lswitch.h"
#include "settings.h"
#include "common.h"
#include "lswres.h"



VOID main()
{ LONG lErrCode;
  static UCHAR ucMsg[MAXERRMSGLEN],ucBuf[MAXERRMSGLEN+CCHMAXPATH],ucErrStr[30],ucFName[CCHMAXPATH];
  QMSG qmsg;
  HAB hab;
  HMQ hmq;
  HENUM henum;
  HWND hwndPopup;
  LSWDATA *plswData=NULL;

  hab = WinInitialize(0);
  hmq = WinCreateMsgQueue(hab, 0);

  if (queryAppInstance()) {
    strcpy(ucMsg,"Already loaded");
    if (DosGetNamedSharedMem((PVOID*)&plswData,SHAREMEM_NAME,PAG_READ | PAG_WRITE)==0) {
      HMODULE hmodRes;
      DosQueryModuleName(plswData->hmodRes,sizeof(ucBuf),ucBuf);
      if (DosLoadModule(ucErrStr, sizeof(ucErrStr), ucBuf, &hmodRes)==0) {
        WinLoadString(hab,hmodRes,MSG_LOADED,sizeof(ucMsg),ucMsg);
        DosFreeModule(hmodRes);
      }
      hwndPopup = plswData->hwndPopup;
      DosFreeMem(plswData);
    }

    WinAlarm(HWND_DESKTOP,WA_ERROR);
    WinMessageBox(HWND_DESKTOP,HWND_DESKTOP,ucMsg,PGMNAME,0,MB_ERROR | MB_CANCEL);
    WinPostMsg(hwndPopup,WM_COMMAND,MPFROMSHORT(CMD_SHOWSETTINGS),MPFROM2SHORT(CMDSRC_OTHER,FALSE));

    WinDestroyMsgQueue(hmq);
    WinTerminate(hab);
    return;
  }

  lErrCode=SwitcherInit(hab, hmq, ucMsg, sizeof(ucMsg), (PPVOID)(&plswData),0);
  if (lErrCode > 0) {
    if (plswData!=NULL)
      WinLoadString(hab,plswData->hmodRes,MSG_ERROR,sizeof(ucErrStr),ucErrStr);
    sprintf(ucBuf,"%s %s %X",ucMsg,ucErrStr,lErrCode);
    if (lErrCode == 183)
      strcat(ucBuf,". Check if lSwitcher widget is initialized.");
    WinAlarm(HWND_DESKTOP,WA_ERROR);
    WinMessageBox(HWND_DESKTOP,HWND_DESKTOP,ucBuf,PGMNAME,0,MB_ERROR | MB_CANCEL);
    WinDestroyMsgQueue(hmq);
    WinTerminate(hab);
    return;
  } else if (lErrCode == LSWERROLDSETTINGS) {
    WinAlarm(HWND_DESKTOP,WA_NOTE);
    GetIniFileName(ucFName,sizeof(ucFName));
    WinLoadString(hab,plswData->hmodRes,MSG_WRONGVER,sizeof(ucMsg),ucMsg);
    sprintf(ucBuf,ucMsg,strlen(ucFName)==0 ? "OS2.INI":ucFName);

    if (WinMessageBox(HWND_DESKTOP,HWND_DESKTOP,ucBuf,PGMNAME,
                      0,MB_YESNO | MB_ICONEXCLAMATION | MB_DEFBUTTON1)==MBID_YES) {
      if (DeleteSettings(hab,ucFName)!=0) {
        WinAlarm(HWND_DESKTOP,WA_ERROR);
        WinLoadString(hab,plswData->hmodRes,MSG_CANTOPEN,sizeof(ucMsg),ucMsg);
        sprintf(ucBuf,ucMsg,ucFName);
        WinMessageBox(HWND_DESKTOP,HWND_DESKTOP,ucBuf,PGMNAME,0,MB_ERROR | MB_CANCEL);
      }
    }
  }

  while (WinGetMsg(hab, &qmsg, 0L, 0, 0))
    WinDispatchMsg(hab, &qmsg);
}
