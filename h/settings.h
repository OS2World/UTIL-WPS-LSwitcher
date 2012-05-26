/*
 *      Copyright (C) 1997-2001 Andrei Los.
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

#ifndef SETTINGS_H_INCLUDED
#define SETTINGS_H_INCLUDED

#include "lswitch.h"
#include <stdlib.h>

#define PRF_APPNAME "lSwitcher"
#define PRF_SETTINGS "Settings"
#define DEFINIFNAME "lSwitch.ini"

#define LSWERRCANTLOADSETTINGS -1
#define LSWERROLDSETTINGS -2

#define LSWM_UPDATEEXCLUDEDLG WM_USER+21
#define UPDATEREMOVEDLIST 1
#define UPDATEADDEDLIST 2
#define UPDATETSKBARLIST 1
#define UPDATEPOPUPLIST 2
#define UPDATEICONSONLYLIST 2


#define EN_SCANCODE 0x8000

MRESULT EXPENTRY ParmDlgProc(HWND hwnd,ULONG msg,MPARAM mp1,MPARAM mp2);

VOID lswSetSettings(LSWSETTINGS *pSettings);
VOID lswGetSettings(LSWSETTINGS *pSettings);

BOOL CheckSettings(LSWSETTINGS *pSettings);
VOID InitSettings(LSWSETTINGS *pSettings);
VOID EditSettings(LSWDATA *plswData);
USHORT LoadSettings(HAB hab,PSZ pszFName,LSWSETTINGS *pSettings);
USHORT SaveSettings(HAB hab,PSZ pszFName,LSWSETTINGS *pSettings);
USHORT DeleteSettings(HAB hab,PSZ pszFName);

VOID GetIniFileName(UCHAR *ucFName,USHORT usLen);


#endif
