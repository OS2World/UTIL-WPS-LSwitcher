/* this code is a part of Henk Kelder's Hektools package */
#ifndef WPTOOLS_H
#define WPTOOLS_H


/*
   Several defines to read WPShell data
*/
#define FOLDERCONTENT  "PM_Abstract:FldrContent"
#define OBJECTS        "PM_Abstract:Objects"
#define ICONS          "PM_Abstract:Icons"

#define ACTIVEHANDLES  "PM_Workplace:ActiveHandles"
#define HANDLESAPP     "HandlesAppName"

#define FOLDERPOS      "PM_Workplace:FolderPos"
#define HANDLES        "PM_Workplace:Handles"
#define LOCATION       "PM_Workplace:Location"
#define PALETTEPOS     "PM_Workplace:PalettePos"
#define STATUSPOS      "PM_Workplace:StatusPos"
#define STARTUP        "PM_Workplace:Startup"
#define TEMPLATES      "PM_Workplace:Templates"
#define HANDLEBLOCK    "BLOCK1"
#define ASSOC_FILTER   "PMWP_ASSOC_FILTER"
#define ASSOC_TYPE     "PMWP_ASSOC_TYPE"
#define ASSOC_CHECKSUM "PMWP_ASSOC_CHECKSUM"
#define WORKAREARUNNING "FolderWorkareaRunningObjects"
#define JOBCNRPOS       "PM_PrintObject:JobCnrPos"
#define CLASSTABLE      "ClassTable"
#define PRINTOBJECTS    "PM_PrintObject"
#define PM_OBJECTS      "PM_Objects"


/*
   The high word of a HOBJECT determines it's type.
*/


/*
   Several datatags
*/
#define WPPROGRAM_PROGTYPE  1
#define WPPROGRAM_EXEHANDLE 2
#define WPPROGRAM_PARAMS    3
#define WPPROGRAM_DIRHANDLE 4
#define WPPROGRAM_DOSSET    6
#define WPPROGRAM_STYLE     7
#define WPPROGRAM_EXENAME   9
#define WPPROGRAM_DATA      11
#define WPPROGRAM_STRINGS   10
#define WPPGM_STR_EXENAME   0
#define WPPGM_STR_ARGS      1

#define WPABSTRACT_TITLE    1
#define WPABSTRACT_STYLE     2 /* appears to contain the same as WPOBJECT 7 */

#define WPOBJECT_HELPPANEL   2
#define WPOBJECT_SZID        6
#define WPOBJECT_STYLE       7
#define WPOBJECT_MINWIN      8
#define WPOBJECT_CONCURRENT  9
#define WPOBJECT_VIEWBUTTON 10
#define WPOBJECT_DATA       11
#define WPOBJECT_STRINGS    12
#define WPOBJ_STR_OBJID      1

#define WPSHADOW_LINK     104

#define WPFOLDER_FOLDERFLAG  13
#define WPFOLDER_ICONVIEW  2900
#define WPFOLDER_DATA      2931
#define WPFOLDER_FONTS     2932

#define WPPROGFILE_PROGTYPE  1
#define WPPROGFILE_DOSSET    5
#define WPPROGFILE_STYLE     6
#define WPPROGFILE_DATA     10
#define WPPROGFILE_STRINGS  11
#define WPPRGFIL_STR_ARGS    0

#define WPFSYS_MENUCOUNT     4
#define WPFSYS_MENUARRAY     3

#define IDKEY_PRNQUEUENAME 3
#define IDKEY_PRNCOMPUTER  5
#define IDKEY_PRNJOBDIALOG 9
#define IDKEY_PRNREMQUEUE 13

#define IDKEY_RPRNNETID    1

#define IDKEY_DRIVENUM     1

#define MAX_PATH               512
#define MAX_DIR                256
#define FILTER_SIZE            10
#define EANAME_SIZE            50
#define MAX_ASSOC              5

#define TYPE_DRIVE     0
#define TYPE_DIRECTORY 1
#define TYPE_FILE      2
#define TYPE_ABSTRACT  3

#pragma pack(1)

typedef struct _NodeDev
{
BYTE   chName[4];  /* = 'NODE' */
USHORT usLevel;
USHORT usID;
USHORT usParentID;
BYTE   ch[18];
USHORT fIsDir;
USHORT usNameSize;
BYTE   szName[1];
} NODE, *PNODE;

typedef struct _DrivDev
{
BYTE  chName[4];  /* = 'DRIV' */
BYTE  ch1[8];
ULONG ulSerialNr;
BYTE  ch2[4];
BYTE  szName[1];
} DRIV, *PDRIV;
                               

typedef struct _FileInfo
{
HPOINTER  hptrIcon;
HPOINTER  hptrMiniIcon;
BOOL      bIconCreated;
BOOL      bMiniIconCreated;
BYTE      szFileName[MAX_PATH];
BYTE      szTitle[MAX_PATH];
BYTE      chType;
HAPP      happ;
HOBJECT   hObject;
HOBJECT   hObjAssoc[MAX_ASSOC];
} FILEINFO, *PFILEINFO;

/*
   Two structures needed for reading WPAbstract's object information
*/
typedef struct _ObjectInfo
{
USHORT cbName;       /* Size of szName */
USHORT cbData;       /* Size of variable length data, starting after szName */
BYTE   szName[1];    /* The name of the datagroup */
} OINFO, *POINFO;

typedef struct _TagInfo
{
USHORT usTagFormat;  /* Format of data       */
USHORT usTag;        /* The data-identifier  */
USHORT cbTag;        /* The size of the data */
} TAG, *PTAG;

typedef struct _WPProgramRefData
{
HOBJECT hExeHandle;
HOBJECT hCurDirHandle;
ULONG   ulFiller1;
ULONG   ulProgType;
BYTE    bFiller[12];
} WPPGMDATA;

typedef struct _WPProgramFileData
{
HOBJECT hCurDirHandle;
ULONG   ulProgType;
ULONG   ulFiller1;
ULONG   ulFiller2;
ULONG   ulFiller3;
} WPPGMFILEDATA;

typedef struct _WPObjectData
{
LONG  lDefaultView;
ULONG ulHelpPanel;
ULONG ulUnknown3;
ULONG ulObjectStyle;
ULONG ulMinWin;
ULONG ulConcurrent;
ULONG ulViewButton;
ULONG ulMenuFlag;
} WPOBJDATA;

typedef struct _WPFolderData
{
ULONG ulIconView;
ULONG ulTreeView;
ULONG ulDetailsView;
ULONG ulFolderFlag;
ULONG ulTreeStyle;
ULONG ulDetailsStyle;
BYTE  rgbIconTextBkgnd[4];
BYTE  Filler1[4];
BYTE  rgbIconTextColor[4];
BYTE  Filler2[8]; 
BYTE  rgbTreeTextColor[4];
BYTE  rgbDetailsTextColor[4];
BYTE  Filler3[4]; 
USHORT fIconTextVisible;
USHORT fIconViewFlags;
USHORT fTreeTextVisible;
USHORT fTreeViewFlags;
BYTE  Filler4[4];
ULONG ulMenuFlag;
BYTE  rgbIconShadowColor[4];
BYTE  rgbTreeShadowColor[4];
BYTE  rgbDetailsShadowColor[4];
} WPFOLDATA;


typedef struct _FsysMenu
{
   USHORT  usIDMenu   ;
   USHORT  usIDParent ;
   USHORT  usMenuType; /*  1 = Cascade, 2 = condcascade, 3 = choice */
   HOBJECT hObject;
   BYTE    szTitle[32];
} FSYSMENU, *PFSYSMENU;

typedef struct _FolderSort
{
LONG lDefaultSortIndex;
BOOL fAlwaysSort;
LONG lCurrentSort;
BYTE bFiller[12];
} FOLDERSORT, *PFOLDERSORT;


#define BASECLASS_TRANSIENT 0
#define BASECLASS_ABSTRACT  1
#define BASECLASS_FILESYS   2
#define BASECLASS_OTHER    99

#pragma pack()

BOOL  GetAbstractName     (HOBJECT hObject, PSZ pszExe, USHORT usMax);
ULONG GetObjectProgType(HOBJECT hObject);
BOOL GetObjectTitle(HOBJECT hObject, PSZ pszTitle, USHORT usMax);
BOOL GetObjectParameters(HOBJECT hObject, PSZ pszParms, USHORT usMax);
BOOL GetDosSettings(HOBJECT hObject, PSZ pszDosSettings, USHORT usMax);
USHORT ObjIDFromName(PSZ pszAbstractName, PSZ pszCmdLine, ULONG ulProgType, BOOL bSearchGroup);
HPOINTER GetObjectIcon(HOBJECT hObject, PBYTE pObjectData);
BOOL GetObjectName(HOBJECT hObject, PSZ pszFname, USHORT usMax);


#endif


