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

#include "lswitch.h"
#include "common.h"
#include "fspopup.h"

#include <string.h>
#include <stdio.h>
#include <process.h>

static HMONITOR hmonKbd = (HMONITOR)0;
static MONIN monInBuf = {0};
static MONOUT monOutBuf = {0};



VOID FSMonitor(VOID *plswData)
{ static USHORT usCount;
  static KEYPACKET keybuf;
  ULONG ulPostCount;

  DosSetPrty(PRTYS_THREAD,PRTYC_TIMECRITICAL,+31,0);

  while (DosWaitEventSem(((PLSWDATA)plswData)->hevRunning,0)==ERROR_TIMEOUT) {
    usCount = sizeof(keybuf);
    if (DosMonRead((PBYTE)&monInBuf,DCWW_WAIT,(PBYTE)&keybuf,&usCount)) break;
    DosMonWrite((PBYTE)&monOutBuf,(PBYTE)&keybuf,usCount);

/* keybuf.mnflags>>8 is the scan code; check for the Tab key release*/
    if ((keybuf.cp.fsState>>8)==(((PLSWDATA)plswData)->Settings.ucPopupHotKey==1?1:2) &&
        (keybuf.mnflags>>8)==(SCAN_TAB | 0x80)) {
      if (keybuf.cp.fsState & 3)
        DosPostEventSem(((PLSWDATA)plswData)->hevShift);
      else
        DosResetEventSem(((PLSWDATA)plswData)->hevShift,&ulPostCount);
      DosPostEventSem(((PLSWDATA)plswData)->hevPopup);
      break;
    }
  }

}

VOID FSMonDispat(VOID *pParm)
{ PLSWDATA plswData;
  SEL selGSeg,selLSeg;
  PGINFOSEG GSeg;
  UCHAR ucCurrId=0,ucId;
  ULONG ulPostCount;

  plswData = (PLSWDATA)pParm;
  DosSetPriority(PRTYS_THREAD,PRTYC_TIMECRITICAL,+31,0);

  DosGetInfoSeg(&selGSeg,&selLSeg);
  GSeg=(PGINFOSEG)((selGSeg &~7)<<13);

  if (DosCreateEventSem(SEMFSPOPUPNAME,&plswData->hevPopup,0,0)) return;
  if (DosCreateEventSem(SEMSHIFTNAME,&plswData->hevShift,0,0)) return;

  while(DosWaitEventSem(plswData->hevRunning,10L)==ERROR_TIMEOUT) {
    /* if we popup in PM reset the semaphore and post a triggering message;
     if we popup in FS create a thread which will monitor the keyboard and
     display the popup menu, post the semaphore one more time so that we do
     not accidentally create one more such thread because the first one didn't
     have time to issue VioPopUp and thus change current FS session;
     the semaphore will be reset when a keyboard monitor for this thread is
     created */
    if (DosWaitEventSem(plswData->hevPopup,200L)==0 &&
        (DosQueryEventSem(plswData->hevPopup,&ulPostCount),ulPostCount==1)) {
      if (plswData->Settings.bPMPopupInFS) {
        DosQueryEventSem(plswData->hevShift,&ulPostCount);
        WinPostMsg(plswData->hwndPopup,WM_COMMAND,
                   MPFROMSHORT(ulPostCount>0 ? CMD_SCROLLLEFTFROMFS : CMD_SCROLLRIGHTFROMFS),
                   MPFROM2SHORT(CMDSRC_OTHER,FALSE));
        DosResetEventSem(plswData->hevPopup,&ulPostCount);
      } else {
        DosPostEventSem(plswData->hevPopup);
        plswData->itidFSMon=_beginthread(FSPopUp,NULL,0x4000,plswData);
      }
    }

    if ((ucId=GSeg->sgCurrent)!=ucCurrId) {
      ucCurrId=ucId;
      if (hmonKbd!=0) { DosMonClose(hmonKbd); hmonKbd=0; }

      if (DosMonOpen("KBD$",&hmonKbd)==0) {
        monInBuf.cb=sizeof(MONIN); monOutBuf.cb=sizeof(MONOUT);
        if (DosMonReg(hmonKbd,(PBYTE)&monInBuf,(PBYTE)&monOutBuf,1,ucCurrId)==0) {
          if (DosQueryEventSem(plswData->hevPopup,&ulPostCount),ulPostCount==0)
            plswData->itidFSMon=_beginthread(FSMonitor,NULL,0x4000,plswData);
        } else {
          DosMonClose(hmonKbd); hmonKbd=0;
        }
      } else
        hmonKbd=0;
      DosResetEventSem(plswData->hevPopup,&ulPostCount);
    }

  }

  DosSleep(200);
  if (hmonKbd!=0) DosMonClose(hmonKbd);
}


/* this function returns the number of the item with given line number
 counting from top in the popup menu */
SHORT ItemNumFromLineNum(LSWDATA *plswData,USHORT usLineNum)
{ SHORT item;

  item = usLineNum+plswData->iShift+
    (plswData->Settings.bScrollItems ? plswData->usCurrItem : 0)-
    (min(MAXTEXTITEMS,plswData->usItems)-1)/2;

  AdjustItem(item,plswData->usItems);
  return item;
}

/* this thread monitors keyboard activity during the popup */
VOID FSPopUp(VOID *pParm)
{ static USHORT usCount;
  static KEYPACKET keybuf;
  UCHAR ucScan;
  USHORT usCounter,usPopOpt;
  ULONG ulPostCount;
  BOOL bDoSwitch, bForward, bNowSticky;
  PLSWDATA plswData;

  DosSetPriority(PRTYS_THREAD,PRTYC_TIMECRITICAL,+31,0);

  plswData = (PLSWDATA)pParm;
  InitTaskArr(plswData,TRUE,FALSE,TRUE);

  if (DosQueryEventSem(plswData->hevShift,&ulPostCount),ulPostCount > 0)
    plswData->usCurrItem = plswData->usItems-1;
  else
    plswData->usCurrItem=1;

  usPopOpt = 1;
  VioPopUp(&usPopOpt,0);

  /* wait until monitor dispatcher registeres a monitor for the popup session and
   resets the semaphore */
  usCounter = 0;
  while (DosQueryEventSem(plswData->hevPopup,&ulPostCount),ulPostCount!=0) {
    DosSleep(5);
    usCounter++;
    if (usCounter>200) {
      VioEndPopUp(0);
      return;
    }
  }

  FSPopupMenu(plswData,0);

  bDoSwitch = FALSE; bNowSticky = FALSE;

  for (;;) {
    usCount = sizeof(keybuf);
    if (DosMonRead((PBYTE)&monInBuf,DCWW_WAIT,(PBYTE)&keybuf,&usCount)) break;
    DosMonWrite((PBYTE)&monOutBuf,(PBYTE)&keybuf,usCount);
//    printf("%x %x %x\n",keybuf.cp.fbStatus,keybuf.cp.fsState,keybuf.mnflags>>8);

    ucScan = keybuf.mnflags>>8;
    if (ucScan == (SCAN_TAB | 0x80) || ucScan == SCAN_UP || ucScan == SCAN_DOWN) {
      bForward = ((ucScan == (SCAN_TAB | 0x80) && !(keybuf.cp.fsState & 3)) || /*check shift status */
                  ucScan == SCAN_DOWN);
      if (bForward) {
        if (plswData->usCurrItem<plswData->usItems-1) plswData->usCurrItem++; else plswData->usCurrItem=0;
      } else {
        if (plswData->usCurrItem>0) plswData->usCurrItem--; else plswData->usCurrItem=plswData->usItems-1;
      }

      if (!plswData->Settings.bScrollItems && plswData->usItems>MAXTEXTITEMS &&
          plswData->usCurrItem==
            ItemNumFromLineNum(plswData, bForward ? MAXTEXTITEMS : -1)) {

        plswData->iShift += (SHORT)(bForward ? MAXTEXTITEMS : -MAXTEXTITEMS)/3;
        plswData->iShift -= plswData->iShift/max((SHORT)plswData->usItems,1)*plswData->usItems;

        FSPopupMenu(plswData,0);
      } else
        FSPopupMenu(plswData,bForward ? 1 : -1);
    } else if (ucScan==(((plswData->Settings.bStickyPopup||bNowSticky)?SCAN_ENTER:
                         plswData->Settings.ucPopupHotKey==0?SCAN_ALT:
                         plswData->Settings.ucPopupHotKey==1?SCAN_CTRL:SCAN_WINLEFT)|0x80)) { /*Enter, Ctrl or Alt or LeftWin release */
      bDoSwitch = TRUE;
      break;
    } else if (ucScan == (SCAN_SPACE | 0x80) ||  /* space bar release */
               ((plswData->Settings.bStickyPopup||bNowSticky) && ucScan == SCAN_ESC))
      break;
    else if ((plswData->Settings.ucPopupHotKey==0 && ucScan == (SCAN_CTRL|0x80)) ||
             (plswData->Settings.ucPopupHotKey==1 && ucScan == (SCAN_ALT|0x80)) ||
             (plswData->Settings.ucPopupHotKey==2 && (ucScan == (SCAN_CTRL|0x80)||
                                                      ucScan == (SCAN_ALT|0x80))))
      bNowSticky = TRUE;
  }
  VioEndPopUp(0);

  if (bDoSwitch)
    WinPostMsg(plswData->hwndPopup,WM_COMMAND,MPFROMSHORT(CMD_SWITCHFROMFS),
               MPFROM2SHORT(CMDSRC_OTHER,FALSE));
}


/* this function displays fullscreen popup menu.
 cmd==0: initialize menu, cmd==1 move cursor to the next item, cmd==-1 to the
 previous item */
void FSPopupMenu(LSWDATA *plswData,SHORT cmd)
{ static UCHAR ucAttr,ucLineBuf[TEXTSCRWID+1],ucTitle[NAMELEN],
    ucCell[2]={32,MENUATTR}, ucMenuHgt, ucLineLen, ucMenuXLeft, ucMenuXRight,
    ucMenuYTop, ucMenuYBottom;
  USHORT k,usItem;
  SHORT iLinesAboveSel;
  VIOCURSORINFO CursorInfo={0};

  if (cmd==0) {
    CursorInfo.attr=-1;
    VioSetCurType(&CursorInfo,0);
  }

  if (plswData->Settings.bOldPopup) {
    if (cmd!=0) {
      memset(ucLineBuf,0,sizeof(ucLineBuf));
      GetItemTitle(plswData->TaskArr[plswData->usCurrItem].hsw,ucLineBuf,sizeof(ucLineBuf),TRUE);
      ucLineBuf[TEXTSCRWID-1]=0; k=0; while (ucLineBuf[k]) k++;
      memmove(&ucLineBuf[(TEXTSCRWID-k)>>1],ucLineBuf,k);
      memset(ucLineBuf,0,(TEXTSCRWID-k)>>1);
      ucAttr=MENUATTR;
      VioWrtCharStrAtt(ucLineBuf,TEXTSCRWID,0,0,&ucAttr,0);
    }
    return;
  }

  iLinesAboveSel = (min(MAXTEXTITEMS,plswData->usItems)-1)/2-plswData->iShift+
    (plswData->Settings.bScrollItems ? 0 : plswData->usCurrItem);

  while (iLinesAboveSel > min(MAXTEXTITEMS,plswData->usItems)-1 || iLinesAboveSel < 0)
    iLinesAboveSel -= (iLinesAboveSel<0?-1:1)*plswData->usItems;

  if (cmd==0) {
    ucLineLen = 0;
    for (k = 0; k < plswData->usItems; k++) {
      GetItemTitle(plswData->TaskArr[k].hsw,ucTitle,sizeof(ucTitle),TRUE);
      if (strlen(ucTitle)>ucLineLen) ucLineLen=strlen(ucTitle);
    }
    ucLineLen += 2;
    ucMenuHgt = min(MAXTEXTITEMS,plswData->usItems)+2;
    ucMenuXLeft = TEXTSCRWID/2-ucLineLen/2;
    ucMenuYTop = TEXTSCRHGT/2-ucMenuHgt/2;
    ucMenuXRight = ucMenuXLeft+ucLineLen;
    ucMenuYBottom = ucMenuYTop+ucMenuHgt;

    for (k = 0; k < ucMenuHgt; k++) {
      if (k==0) {
        memset(ucLineBuf,205,ucLineLen); ucLineBuf[0]=201; ucLineBuf[ucLineLen-1]=187;
      } else if (k==ucMenuHgt-1) {
        memset(ucLineBuf,205,ucLineLen); ucLineBuf[0]=200; ucLineBuf[ucLineLen-1]=188;
      } else {
        usItem = ItemNumFromLineNum(plswData,k-1); //-1 takes care of the frame
        GetItemTitle(plswData->TaskArr[usItem].hsw,ucTitle,sizeof(ucTitle),TRUE);
        sprintf(ucLineBuf,"%c%-*s%c",186,ucLineLen-2,ucTitle,186);
      }
      ucAttr = MENUATTR;
      VioWrtCharStrAtt(ucLineBuf,ucLineLen,ucMenuYTop+k,ucMenuXLeft,&ucAttr,0);
    }
  } else {
    if (plswData->Settings.bScrollItems) {
      if (cmd==1)
        VioScrollUp(ucMenuYTop+1,ucMenuXLeft+1,ucMenuYBottom-2,ucMenuXRight-2,1,ucCell,0);
      else
        VioScrollDn(ucMenuYTop+1,ucMenuXLeft+1,ucMenuYBottom-2,ucMenuXRight-2,1,ucCell,0);

      usItem = ItemNumFromLineNum(plswData,cmd==1 ? ucMenuHgt-3 : 0);
      GetItemTitle(plswData->TaskArr[usItem].hsw,ucTitle,sizeof(ucTitle),TRUE);
      sprintf(ucLineBuf,"%c%-*s%c",186,ucLineLen-2,ucTitle,186);
      ucAttr=MENUATTR;
      VioWrtCharStrAtt(ucLineBuf,ucLineLen,cmd==1 ? ucMenuYBottom-2 : ucMenuYTop+1,
                       ucMenuXLeft,&ucAttr,0);
      VioWrtNAttr(&ucAttr,ucLineLen-2,ucMenuYTop+iLinesAboveSel+(cmd==1 ? 0 : 2),
                  ucMenuXLeft+1,0);
    } else {
      ucAttr=MENUATTR;
      VioWrtNAttr(&ucAttr,ucLineLen-2,ucMenuYTop+iLinesAboveSel+
                  (cmd==1
                   ? (iLinesAboveSel==0 ? ucMenuHgt-2 : 0)
                   : (iLinesAboveSel==ucMenuHgt-3 ? 1-iLinesAboveSel : 2)),
                  ucMenuXLeft+1,0);
    }
  }
  ucAttr=CURSORATTR;
  VioWrtNAttr(&ucAttr,ucLineLen-2,ucMenuYTop+iLinesAboveSel+1,ucMenuXLeft+1,0);
}
