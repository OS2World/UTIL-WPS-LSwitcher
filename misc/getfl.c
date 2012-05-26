#define INCL_PM
#include<os2.h>

void main()
{ SWCNTRL swctl;
  SWP swp,swpd;

  WinQuerySwitchEntry(0x400,&swctl);
  WinQueryWindowPos(HWND_DESKTOP,&swpd);
  WinQueryWindowPos(0x800001ad,&swp);
  printf("%s %X\ndesk %i %i %i %i\npos %i %i \nmin %i %i\nres %i %i\n",
	swctl.szSwtitle,swp.fl,swpd.x,swpd.y,swpd.cx,swpd.cy,swp.x,swp.y,
	 WinQueryWindowUShort(0x800001b8,QWS_XMINIMIZE),
	 WinQueryWindowUShort(0x800001b8,QWS_YMINIMIZE),
	 WinQueryWindowUShort(0x800001b8,QWS_XRESTORE),
	 WinQueryWindowUShort(0x800001b8,QWS_YRESTORE));
}