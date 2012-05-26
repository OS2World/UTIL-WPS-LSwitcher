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

#ifndef LSWRES_H_INCLUDED
#define LSWRES_H_INCLUDED

#include "lswitch.h"

/* defines localization of the program */
#define EN 1
#define CZ 420
#define IT 39
#define DE 49
#define ES 34
#define RU 7
#define JP 81


#if LANGUAGE==EN
  #define LANGUAGESTR "English"
#elif LANGUAGE==RU
  #define LANGUAGESTR "Русский"
#elif LANGUAGE==CZ
  #define LANGUAGESTR "Яeчtina"
#elif LANGUAGE==ES
  #define LANGUAGESTR "Espaдol"
#elif LANGUAGE==JP
  #define LANGUAGESTR "Japanese"
#elif LANGUAGE==DE
  #define LANGUAGESTR "Deutsch"
#endif

#if LANGUAGE==RU
  #define SCAN_PRIORITY 0x23  /*буква р */
#elif LANGUAGE==CZ
  #define SCAN_PRIORITY 0x13 /* letter r */
#else
  #define SCAN_PRIORITY 0x19  /*letter p */
#endif

#if LANGUAGE==RU
  #define SCAN_FILTER 0x1e  /*буква ф */
#else
  #define SCAN_FILTER 0x21  /*letter f */
#endif


#if LANGUAGE==CZ
  #define SCAN_CLOSE 0x2c   /*letter z */
#elif LANGUAGE==RU
  #define SCAN_CLOSE 0x19  /*буква з */
#else
  #define SCAN_CLOSE 0x2e  /*letter c */
#endif


#if LANGUAGE==CZ
  #define SCAN_HIDE 0x25 /*letter k */
#elif LANGUAGE==ES
  #define SCAN_HIDE 0x18 /*letter o */
#elif LANGUAGE==IT
  #define SCAN_HIDE 0x31 /* letter n */
#elif LANGUAGE==RU
  #define SCAN_HIDE 0x31 /* буква т */
#elif LANGUAGE==DE
  #define SCAN_HIDE 0x20 /* letter d */
#else
  #define SCAN_HIDE 0x23 /* letter h */
#endif

#if LANGUAGE==CZ
  #define SCAN_MINIMIZE 0x32 /* letter m */
#elif LANGUAGE==IT
  #define SCAN_MINIMIZE 0x13 /* letter r */
#elif LANGUAGE==RU
  #define SCAN_MINIMIZE 0x15 /* буква н */
#elif LANGUAGE==DE
  #define SCAN_MINIMIZE 0x2c /* letter y on German kbd */
#else
  #define SCAN_MINIMIZE 0x31 /* letter n */
#endif

#if LANGUAGE==CZ
  #define SCAN_RESTORE 0x18 /* letter o */
#elif LANGUAGE==IT
  #define SCAN_RESTORE 0x19 /* letter p */
#elif LANGUAGE==RU
  #define SCAN_RESTORE 0x20 /* буква в */
#elif LANGUAGE==DE
  #define SCAN_RESTORE 0x11 /* letter w */
#else
  #define SCAN_RESTORE 0x13 /* letter r */
#endif

#if LANGUAGE==CZ
  #define SCAN_SHOW 0x30 /* letter b */
#elif LANGUAGE==ES
  #define SCAN_SHOW 0x32 /* letter m */
#elif LANGUAGE==IT
  #define SCAN_SHOW 0x32 /* letter m */
#elif LANGUAGE==RU
  #define SCAN_SHOW 0x24 /* буква о */
#elif LANGUAGE==DE
  #define SCAN_SHOW 0x1e /* letter a */
#else
  #define SCAN_SHOW 0x1f /* letter s */
#endif

#if LANGUAGE==IT
  #define SCAN_MAXIMIZE 0x17 /* letter i */
#elif LANGUAGE==RU
  #define SCAN_MAXIMIZE 0x13 /* буква к */
#elif LANGUAGE==DE
  #define SCAN_MAXIMIZE 0x32 /* letter m */
#else
  #define SCAN_MAXIMIZE 0x2d /* letter x */
#endif

#define STRID_HINTS ID_HINTSTR
#define STRID_SWITCH CMD_SWITCHFROMPM
#define STRID_CLOSE CMD_CLOSE
#define STRID_HIDE CMD_HIDE
#define STRID_MINIMIZE CMD_MINIMIZE
#define STRID_RESTORE CMD_RESTORE
#define STRID_SHOW CMD_SHOW
#define STRID_MAXIMIZE CMD_MAXIMIZE
#define STRID_MOVE CMD_MOVE
#define STRID_SHOWSETTINGS CMD_SHOWSETTINGS
#define STRID_RUN CMD_RUN
#define STRID_KILL CMD_KILL
#define STRID_DEATH CMD_DEATH
#define STRID_PRIORITY CMD_PRIORITY
#define STRID_QUIT CMD_QUIT
#define STRID_XCENTERSUBMENU CMD_XCENTERSUBMENU
#define STRID_CLOSEQUIT CMD_CLOSEQUIT
#define STRID_ADDFILTER CMD_ADDFILTER
#define STRID_SUSPEND CMD_SUSPEND

#define MAXERRMSGLEN 256

#define MSG_LOADED   301
#define MSG_CANTOPEN 302
#define MSG_ERROR    303
#define MSG_CANTSAVE 304
#define MSG_WRONGVER 305
#define MSG_CANTFINDOPEN 306
#define MSG_CANTEXECUTE 307
#define MSG_CANTKILL 308
#define MSG_KILLQUERY 309
#define MSG_CANCEL 310
#define MSG_FOLDERS 311


typedef APIENTRY RESVERPROC(ULONG *Version, USHORT *Language,UCHAR *LangString,USHORT usLangStrLen);
RESVERPROC QueryResourceVersion;

#endif
