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

#ifndef FSPOPUP_H_INCLUDED
#define FSPOPUP_H_INCLUDED

#include "lswitch.h"

#define TEXTSCRWID (UCHAR)80
#define TEXTSCRHGT (UCHAR)25
#define MENUATTR 0x7F
#define CURSORATTR 0x0F
#define MAXTEXTITEMS (USHORT)23

VOID FSPopUp(VOID *p);
VOID FSPopupMenu(LSWDATA *plswData,SHORT cmd);
VOID FSMonDispat(VOID *p);


#endif