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

#include "lswitch.h"
#include "lswres.h"

APIENTRY QueryResourceVersion(ULONG *Version, USHORT *Language,UCHAR *LangString,USHORT usLangStrLen)
{ USHORT k;

  for (k=0; LANGUAGESTR[k]!='\0' && k<usLangStrLen-1; k++)
    LangString[k]=LANGUAGESTR[k];
  LangString[k] = '\0';

  *Version=MAKEULONG(MAKEUSHORT(VERSIONMAJOR,VERSIONMINOR),MAKEUSHORT(REVISION,0));
  *Language=LANGUAGE;
  return *Version;
}
