/*
 *      Copyright (C) 1997-2005 Andrei Los.
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

#ifndef PMPOPUP_H_INCLUDED
#define PMPOPUP_H_INCLUDED

#include "lswitch.h"

#define MAXICONSINLINE (USHORT)9
#define ICONLINES (USHORT)3
#define ICONSPACINGX 1/2
#define ICONSPACINGY 1/3
#define SELBOXOFFSET 1/8
#define SELBOXLINEWIDTH 2
#define POPUPWINHIDEPOSX 24000
#define POPUPWINHIDEPOSY 24000
#define POPUPBORDERSIZER 75

MRESULT EXPENTRY PopupWndProc(HWND hwnd,ULONG msg,MPARAM mp1,MPARAM mp2);
MRESULT EXPENTRY FrameWndProc(HWND hwnd,ULONG msg,MPARAM mp1,MPARAM mp2);

#endif