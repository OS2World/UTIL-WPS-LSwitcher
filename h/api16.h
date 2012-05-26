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


#ifndef API16_H_INCLUDED
#define API16_H_INCLUDED

#include <os2.h>

#define DCWW_WAIT   0
#define DCWW_NOWAIT 1


typedef struct _GINFOSEG {
   ULONG   time;               /* time in seconds */
   ULONG   msecs;              /* milliseconds    */
   UCHAR   hour;               /* hours */
   UCHAR   minutes;            /* minutes */
   UCHAR   seconds;            /* seconds */
   UCHAR   hundredths;         /* hundredths */
   USHORT  timezone;           /* minutes from UTC */
   USHORT  cusecTimerInterval; /* timer interval (units = 0.0001 seconds) */
   UCHAR   day;                /* day */
   UCHAR   month;              /* month */
   USHORT  year;               /* year */
   UCHAR   weekday;            /* day of week */
   UCHAR   uchMajorVersion;    /* major version number */
   UCHAR   uchMinorVersion;    /* minor version number */
   UCHAR   chRevisionLetter;   /* revision letter */
   UCHAR   sgCurrent;          /* current foreground session */
   UCHAR   sgMax;              /* maximum number of sessions */
   UCHAR   cHugeShift;         /* shift count for huge elements */
   UCHAR   fProtectModeOnly;   /* protect mode only indicator */
   USHORT  pidForeground;      /* pid of last process in foreground session */
   UCHAR   fDynamicSched;      /* dynamic variation flag */
   UCHAR   csecMaxWait;        /* max wait in seconds */
   USHORT  cmsecMinSlice;      /* minimum timeslice (milliseconds) */
   USHORT  cmsecMaxSlice;      /* maximum timeslice (milliseconds) */
   USHORT  bootdrive;          /* drive from which the system was booted */
   UCHAR   amecRAS[32];        /* system trace major code flag bits */
   UCHAR   csgWindowableVioMax;/* maximum number of VIO windowable sessions */
   UCHAR   csgPMMax;           /* maximum number of pres. services sessions */
} GINFOSEG;

typedef GINFOSEG *PGINFOSEG;


typedef struct _LINFOSEG {
   PID     pidCurrent;        /* current process id */
   PID     pidParent;         /* process id of parent */
   USHORT  prtyCurrent;       /* priority of current thread */
   TID     tidCurrent;        /* thread ID of current thread */
   USHORT  sgCurrent;         /* session */
   UCHAR   rfProcStatus;      /* process status */
   UCHAR   dummy1;
   BOOL    fForeground;       /* current process has keyboard focus */
   UCHAR   typeProcess;       /* process type */
   UCHAR   dummy2;
   SEL     selEnvironment;    /* environment selector */
   USHORT  offCmdLine;        /* command line offset */
   USHORT  cbDataSegment;     /* length of data segment */
   USHORT  cbStack;           /* stack size */
   USHORT  cbHeap;            /* heap size */
   HMODULE hmod;              /* module handle of the application */
   SEL     selDS;             /* data segment handle of the application */
} LINFOSEG;

typedef LINFOSEG *PLINFOSEG;


#define DosGetInfoSeg DOS16GETINFOSEG

APIRET16 APIENTRY16 DosGetInfoSeg(PSEL GlobalSeg,PSEL LocalSeg);


typedef SHANDLE HMONITOR;
typedef HMONITOR *PHMONITOR;

#pragma pack(2)
typedef struct _MONIN {
        USHORT cb;
        BYTE abReserved[18];
        BYTE abBuffer[108];
} MONIN;
typedef MONIN *PMONIN;

#pragma pack(2)
typedef struct _MONOUT {
        USHORT cb;
        UCHAR buffer[18];
        BYTE abBuf[108];
} MONOUT;
typedef MONOUT *PMONOUT;


//#pragma pack(2)
//typedef struct _KBDKEYINFO {   /* kbci */
//   UCHAR    chChar;             /* ASCII character code */
//   UCHAR    chScan;             /* Scan Code */
//   UCHAR    fbStatus;           /* State of the character */
//   UCHAR    bNlsShift;          /* Reserved (set to zero) */
//   USHORT   fsState;            /* State of the shift keys */
//   ULONG    time;               /* Time stamp of keystroke (ms since ipl) */
//} KBDKEYINFO;


#pragma pack(2)
typedef struct _keypacket {
        USHORT     mnflags;
        KBDKEYINFO cp;
        USHORT     ddflags;
} KEYPACKET;


#pragma seg16(HKBD)
#pragma seg16(MONIN)
#pragma seg16(MONOUT)
#pragma seg16(KEYPACKET)


#define DosMonOpen    DOS16MONOPEN
#define DosMonReg     DOS16MONREG
#define DosMonClose   DOS16MONCLOSE
#define DosMonRead    DOS16MONREAD
#define DosMonWrite   DOS16MONWRITE


APIRET16 APIENTRY16 DosMonOpen(PSZ DevName,PHMONITOR Handle);
APIRET16 APIENTRY16 DosMonReg(HMONITOR Handle,PBYTE BufferI,PBYTE BufferO,USHORT Posflag,USHORT Index);
APIRET16 APIENTRY16 DosMonClose(HMONITOR Handle);
APIRET16 APIENTRY16 DosMonRead(PBYTE BufferI,USHORT WaitFlag,PBYTE DataBuffer,PUSHORT Bytecnt);
APIRET16 APIENTRY16 DosMonWrite(PBYTE BufferO,PBYTE DataBuffer,USHORT Bytecnt);
/* DosGetPrty = DOSCALLS.9 */
APIRET16 APIENTRY16 DosGetPrty(USHORT usScope, PUSHORT pusPriority, USHORT pid);


#pragma pack()

#endif
