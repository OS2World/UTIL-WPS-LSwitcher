/*
 *      Copyright (C) 1997-2004 Andrei Los.
 *	Based on a sample XWorkplace widget by Ulrich M”ller
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

/*
 *      Any XCenter widget plugin DLL must export the
 *      following procedures by ordinal:
 *
 *      -- Ordinal 1 (WgtInitModule): this must
 *         return the widgets which this DLL provides.
 *
 *      -- Ordinal 2 (WgtUnInitModule): this must
 *         clean up global DLL data.
 *
 *      Unless you start your own threads in your widget,
 *      you can safely compile the widget with the VAC
 *      subsystem libraries to reduce the DLL's size.
 *      You can also import functions from XFLDR.DLL
 *      to avoid code duplication.
 *
 *      This is all new with V0.9.7.
 *
 *@@added V0.9.7 (2000-12-31) [umoeller]
 *@@header "shared\center.h"
 */

#include <string.h>
#include <stdlib.h>

#include "lswitch.h"
#include "common.h"
#include "taskbar.h"
#include "settings.h"
#include "prmdlg.h"

// XWorkplace implementation headers
#include "center.h"              // public XCenter interfaces


VOID EXPENTRY lswShowSettings(PWIDGETSETTINGSDLGDATA pData);

static XCENTERWIDGETCLASS G_WidgetClasses[] = {
 LSWTASKBARCLASS,     // PM window class name
  0,                          // additional flag, not used here
  "lswWidget",                // internal widget class name
  "lSwitcher",                // widget class name displayed to user
  WGTF_UNIQUEGLOBAL,      // widget class flags
  lswShowSettings
};


//global variable used by lSwitcher code
//the widget DLL module handle
HMODULE hmodWidgetDll;


/* ******************************************************************
 *
 *   Callbacks stored in XCENTERWIDGETCLASS
 *
 ********************************************************************/

/*
 *@@ WwgtShowSettingsDlg:
 *      this displays the winlist widget's settings
 *      dialog.
 *
 *      This procedure's address is stored in
 *      XCENTERWIDGET so that the XCenter knows that
 *      we can do this.
 *
 *      When calling this function, the XCenter expects
 *      it to display a modal dialog and not return
 *      until the dialog is destroyed. While the dialog
 *      is displaying, it would be nice to have the
 *      widget dynamically update itself.
 */

VOID EXPENTRY lswShowSettings(PWIDGETSETTINGSDLGDATA pData)
{ LSWDATA *plswData;

/*    HWND hwnd = WinLoadDlg(HWND_DESKTOP,         // parent
                           pData->hwndOwner,
                           fnwpSettingsDlg,
                           pcmnQueryNLSModuleHandle(FALSE),
                           ID_CRD_WINLISTWGT_SETTINGS,
                           // pass original setup string with WM_INITDLG
                           (PVOID)pData);
    if (hwnd) {
        pcmnSetControlsFont(hwnd,1, 10000);
        WinProcessDlg(hwnd);
        WinDestroyWindow(hwnd);
    }*/
  if (DosGetNamedSharedMem((PVOID*)&plswData,SHAREMEM_NAME,PAG_READ | PAG_WRITE)==0 && plswData!=NULL) {
    WinDlgBox(HWND_DESKTOP, HWND_DESKTOP, ParmDlgProc,
              plswData->hmodRes, DLG_PARAMS, plswData);
    DosFreeMem(plswData);
  }
}

/* ******************************************************************
 *
 *   Exported procedures
 *
 ********************************************************************/

/*
 *@@ WgtInitModule:
 *      required export with ordinal 1, which must tell
 *      the XCenter how many widgets this DLL provides,
 *      and give the XCenter an array of XCENTERWIDGETCLASS
 *      structures describing the widgets.
 *
 *      With this call, you are given the module handle of
 *      XFLDR.DLL. For convenience, you may resolve imports
 *      for some useful functions which are exported thru
 *      src\shared\xwp.def. See the code below.
 *
 *      This function must also register the PM window classes
 *      which are specified in the XCENTERWIDGETCLASS array
 *      entries. For this, you are given a HAB which you
 *      should pass to WinRegisterClass. For the window
 *      class style (4th param to WinRegisterClass),
 *      you should specify
 *
 +          CS_PARENTCLIP | CS_SIZEREDRAW | CS_SYNCPAINT
 *
 *      Your widget window _will_ be resized, even if you're
 *      not planning it to be.
 *
 *      This function only gets called _once_ when the widget
 *      DLL has been successfully loaded by the XCenter. If
 *      there are several instances of a widget running (in
 *      the same or in several XCenters), this function does
 *      not get called again. However, since the XCenter unloads
 *      the widget DLLs again if they are no longer referenced
 *      by any XCenter, this might get called again when the
 *      DLL is re-loaded.
 *
 *      There will ever be only one load occurence of the DLL.
 *      The XCenter manages sharing the DLL between several
 *      XCenters. As a result, it doesn't matter if the DLL
 *      has INITINSTANCE etc. set or not.
 *
 *      If this returns 0, this is considered an error, and the
 *      DLL will be unloaded again immediately.
 *
 *      If this returns any value > 0, *ppaClasses must be
 *      set to a static array (best placed in the DLL's
 *      global data) of XCENTERWIDGETCLASS structures,
 *      which must have as many entries as the return value.
 */

ULONG EXPENTRY WgtInitModule(HAB hab,         // XCenter's anchor block
                             HMODULE hmodPlugin, // module handle of the widget DLL
                             HMODULE hmodXFLDR,    // XFLDR.DLL module handle
                             PXCENTERWIDGETCLASS *ppaClasses,
                             PSZ pszErrorMsg)  // if 0 is returned, 500 bytes of error msg
{ PVOID pData;
  LONG lrc;

  if (queryAppInstance()) {
    strcpy(pszErrorMsg, "lSwitcher is already loaded.");
    return 0;
  }

  if (DosQueryProcAddr(hmodXFLDR,0,"krnSendDaemonMsg",(PFN*)&pkrnSendDaemonMsg)!= NO_ERROR) {
    strcpy(pszErrorMsg, "Import krnSendDaemonMsg failed.");
    return 0;
  }

  hmodWidgetDll = hmodPlugin;
  lrc = SwitcherInit(hab, 0, pszErrorMsg, 500, &pData, 1);

  if (lrc > 0) return 0;

  if (!WinRegisterClass(hab,LSWTASKBARCLASS,TaskBarWndProc,
      CS_PARENTCLIP | CS_SIZEREDRAW | CS_SYNCPAINT | CS_CLIPCHILDREN,sizeof(PXCENTERWIDGET))) {
    strcpy(pszErrorMsg,"WinRegisterClass(lswTaskBarClass) failed.");
    return 0;
  }

  *ppaClasses = G_WidgetClasses;
// return no. of classes in this DLL (one here):
  return sizeof(G_WidgetClasses) / sizeof(G_WidgetClasses[0]);
}

/*
 *@@ WgtUnInitModule:
 *      optional export with ordinal 2, which can clean
 *      up global widget class data.
 *
 *      This gets called by the XCenter right before
 *      a widget DLL gets unloaded. Note that this
 *      gets called even if the "init module" export
 *      returned 0 (meaning an error) and the DLL
 *      gets unloaded right away.
 */

VOID EXPENTRY WgtUnInitModule(VOID)
{ LSWDATA *plswData;

  if (DosGetNamedSharedMem((PVOID*)&plswData,SHAREMEM_NAME,PAG_READ | PAG_WRITE)==0 && plswData!=NULL) {
    SwitcherTerm(plswData,2);
    DosFreeMem(plswData);
  }
}

/*
 *@@ WgtQueryVersion:
 *      this new export with ordinal 3 can return the
 *      XWorkplace version number which is required
 *      for this widget to run. For example, if this
 *      returns 0.9.10, this widget will not run on
 *      earlier XWorkplace versions.
 *
 *      NOTE: This export was mainly added because the
 *      prototype for the "Init" export was changed
 *      with V0.9.9. If this returns 0.9.9, it is
 *      assumed that the INIT export understands
 *      the new FNWGTINITMODULE_099 format (see center.h).
 *
 *@@added V0.9.9 (2001-02-06) [umoeller]
 */

VOID EXPENTRY WgtQueryVersion(PULONG pulMajor,
                              PULONG pulMinor,
                              PULONG pulRevision)
{
    *pulMajor = 0;
    *pulMinor = 9;
    *pulRevision = 19;
}

