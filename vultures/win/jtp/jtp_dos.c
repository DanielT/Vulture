/*	SCCS Id: @(#)jtp_dos.c	3.0	2000/11/12	*/
/* Copyright (c) Jaakko Peltonen, 2000				  */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef _jtp_dos_c_
#define _jtp_dos_c_

#include "jtp_def.h"
#include <stdlib.h>
#include <string.h>
#include <pc.h>
#include <dos.h>
#include <go32.h>         /* DJGPP specific file */
#include <dpmi.h>         /* DJGPP specific file */
#include <sys/nearptr.h>  /* DJGPP specific file */
#include "jtp_gra.h"      /* For jtp_screen_t structure definition */
#include "jtp_dos.h"

#define PACKED __attribute__ ((packed))
#pragma pack(1)

/*------------------------------------
 SuperVGA controller information block
-------------------------------------*/
typedef struct {
  char    VESASignature[4]      PACKED; /* VESA 4-byte signature              */
  jtp_uint2   VESAVersion       PACKED; /* VBE version number                 */
  jtp_uint4   OEMStringPtr      PACKED; /* Pointer to OEM string              */
  jtp_uchar   Capabilities[4]   PACKED; /* Capabilities of video card         */
  jtp_uint4   VideoModePtr      PACKED; /* Pointer to supported modes         */
  jtp_uint2   TotalMemory       PACKED; /* Number of 64kb memory blocks       */
  jtp_uint2   OEMSoftwareRev    PACKED; /* VBE software revision              */
  jtp_uint4   OEMVendorNamePtr  PACKED; /* Pointer to vendor name string      */
  jtp_uint4   OEMProductNamePtr PACKED; /* Pointer to product name string     */
  jtp_uint4   OEMProductRevPtr  PACKED; /* Pointer to product revision string */
  char    Reserved[222]         PACKED; /* Reserved as working space          */
  char    OEMData[256]          PACKED; /* Data area for OEM strings          */
} jtp_dos_vbe_vgainfo;


/*------------------------------------
SuperVGA mode information block
-------------------------------------*/
typedef struct {
  short   ModeAttributes;         /* Mode attributes                  */
  char    WinAAttributes;         /* Window A attributes              */
  char    WinBAttributes;         /* Window B attributes              */
  short   WinGranularity;         /* Window granularity in k          */
  short   WinSize;                /* Window size in k                 */
  short   WinASegment;            /* Window A segment                 */
  short   WinBSegment;            /* Window B segment                 */
  void    *WinFuncPtr;            /* Pointer to window function       */
  short   BytesPerScanLine;       /* Bytes per scanline               */
  short   XResolution;            /* Horizontal resolution            */
  short   YResolution;            /* Vertical resolution              */
  char    XCharSize;              /* Character cell width             */
  char    YCharSize;              /* Character cell height            */
  char    NumberOfPlanes;         /* Number of memory planes          */
  char    BitsPerPixel;           /* Bits per pixel                   */

  char    NumberOfBanks;          /* Number of CGA style banks        */
  char    MemoryModel;            /* Memory model type                */
  char    BankSize;               /* Size of CGA style banks          */
  char    NumberOfImagePages;     /* Number of images pages           */
  char    res1;                   /* Reserved                         */
  char    RedMaskSize;            /* Size of direct color red mask    */
  char    RedFieldPosition;       /* Bit posn of lsb of red mask      */
  char    GreenMaskSize;          /* Size of direct color green mask  */
  char    GreenFieldPosition;     /* Bit posn of lsb of green mask    */
  char    BlueMaskSize;           /* Size of direct color blue mask   */
  char    BlueFieldPosition;      /* Bit posn of lsb of blue mask     */
  char    RsvdMaskSize;           /* Size of direct color res mask    */
  char    RsvdFieldPosition;      /* Bit posn of lsb of res mask      */
  char    DirectColorModeInfo;    /* Direct color mode attributes     */

  /* VESA 2.0 variables */
  long    PhysBasePtr;            /* physical address for flat frame buffer */
  long    OffScreenMemOffset;     /* pointer to start of off screen memory */
  short   OffScreenMemSize;       /* amount of off screen memory in 1k units */
  char    res2[206];              /* Pad to 256 byte block size       */
} jtp_dos_vbe_modeinfo;

#pragma pack()


unsigned char * jtp_dos_console;
short int jtp_dos_mousex = 100, jtp_dos_mousey = 100, jtp_dos_mouseb = 0;
short int jtp_dos_oldmx = 100, jtp_dos_oldmy = 100, jtp_dos_oldmb = 0;

/*--------------------------------------------------------------------------
 DOS-specific Palette handling
----------------------------------------------------------------------------*/
void jtp_DOSSetColor
(
  unsigned char cindex,
  unsigned char redval,
  unsigned char greenval,
  unsigned char blueval
)
{
  outp(0x03c6, 0xFF);
  outp(0x03c8, cindex);
  outp(0x03c9, redval);
  outp(0x03c9, greenval);
  outp(0x03c9, blueval);
}

/*------------------------------------------------------------------------
 DOS-specific graphics initialization and closing (using VESA SVGA modes)
--------------------------------------------------------------------------*/

void jtp_DOSGoBackToTextMode()
{
  union REGS regs;
  regs.w.ax=3;
  int86(0x10,&regs,&regs);
}

void jtp_DOSSetGDisplayMode(jtp_uint4 disp_mode)
{
  union REGS regs;
  regs.x.eax=0x4f02;
  regs.x.ebx=disp_mode;
  int86(0x10,&regs,&regs);
}

void jtp_DOSSetWindow(jtp_uint4 w_num)
{
  union REGS regs;
  regs.x.eax=0x4f05;
  regs.x.ebx=0;
  regs.x.edx=w_num;
  int86(0x10,&regs,&regs);
}

/*-------------------------------------------------
Detects precence of VESA controller.
Returns 0 on success, an error code on failure.
--------------------------------------------------*/
int jtp_DOSDetectVBE
(
  jtp_dos_vbe_vgainfo *vbeinfo
)
{
   __dpmi_regs regs;

   strncpy(vbeinfo->VESASignature, "VBE2", 4);
   regs.x.ax = 0x4F00;
   regs.x.di = __tb & 0x0F;
   regs.x.es = (__tb >> 4) & 0xFFFF;
   dosmemput(vbeinfo, sizeof(jtp_dos_vbe_vgainfo), __tb);
   __dpmi_int(0x10, &regs);
   dosmemget(__tb, sizeof(jtp_dos_vbe_vgainfo), vbeinfo);

   return(regs.h.ah);
}

/*-------------------------------------------------
Retrieves info on a VESA videomode into a pointer.
Returns 0 on success, an error code on failure.
--------------------------------------------------*/
int jtp_DOSGetVBEModeInfo
(
  unsigned short mode, 
  jtp_dos_vbe_modeinfo *modeinfo
)
{
   __dpmi_regs regs;

   regs.x.ax=0x4F01;
   regs.x.cx=mode;
   regs.x.di=__tb & 0x0F;
   regs.x.es=(__tb >> 4) & 0xFFFF;
   __dpmi_int(0x10, &regs);
   dosmemget(__tb, sizeof(jtp_dos_vbe_modeinfo), modeinfo);

   return(regs.h.ah);
}

/*---------------------------------------------------
Finds the VESA mode number corresponding to the given
resolution and color settings. Color settings are:
8  = 256 colors (8 bit)
16 = highcolor (16 bit)
24 = truecolor (24 bit)
32 = truecolor (32 bit)
-----------------------------------------------------*/
#define JTP_MAX_VBEMODES 1000
int jtp_DOSGetVBEModeNumber(int xres, int yres, int bitsperpixel)
{
  jtp_dos_vbe_vgainfo  vbeinfo;
  jtp_dos_vbe_modeinfo modeinfo;
  jtp_uint2 vbemodes[JTP_MAX_VBEMODES];
  int       i = 0;
  char      oembuffer[200];
 
  if (jtp_DOSDetectVBE(&vbeinfo) != 0) return(0);
 
  while (i < 0xFFFF)
  {
    jtp_DOSGetVBEModeInfo(i, &modeinfo);
    if ((modeinfo.XResolution == xres)&&
        (modeinfo.YResolution == yres)&&
        (modeinfo.BitsPerPixel == bitsperpixel))
      { return(i); }
    i++;
  }                
  return(0);
}


void jtp_DOSSetMode(int screen_width, int screen_height, int screen_bitdepth)
{
  int modenumber;
  
  /* Find requested mode and activate it */
  modenumber = jtp_DOSGetVBEModeNumber(screen_width, screen_height, screen_bitdepth);
  if (!modenumber)
  {
    printf("Could not find graphics mode [%dx%d, %d-bit].\n", 
           screen_width, screen_height, screen_bitdepth);
    printf("Please check your settings. If necessary, use a VESA driver.\n");
    exit(1);
  }
  
  jtp_DOSSetGDisplayMode(modenumber);

  /* Set pointer to start of video memory area */
  jtp_dos_console = (unsigned char *)(0xA0000 + __djgpp_conventional_base);  
}



/*-------------------------------------------------------------------------
 DOS-specific display updating
---------------------------------------------------------------------------*/

void jtp_DOSRefresh(jtp_screen_t *newscreen)
{
 int i, npixels, tempwin;
 unsigned char * isource;
 
 tempwin = 0; 
 npixels = newscreen->width*newscreen->height;
 isource = newscreen->vpage;
 while (npixels > 0)
 {
   jtp_DOSSetWindow(tempwin);
   if (npixels > 65536)
     memcpy(jtp_dos_console,isource,65536);
   else  
     memcpy(jtp_dos_console,isource,npixels);
   npixels -= 65536;
   isource += 65536;
   tempwin++;
 }  
}


void jtp_DOSRefreshRegion
(
  int x1, int y1,
  int x2, int y2,
  jtp_screen_t *newscreen
)
{
  int curwindow = 0;
  int i,j;
  int osplus;
  int con_ofs;
  int buf_ofs;
  
  if ((x2 < 0) || (y2 < 0) || (x1 > jtp_screen.width-1) || (y1 > jtp_screen.height-1)) 
    return;
  
  if (x2 > newscreen->width-1) x2 = newscreen->width-1;
  if (y2 > newscreen->height-1) y2 = newscreen->height-1;
  if (x1 < 0) x1 = 0;
  if (y1 < 0) y1 = 0;

  con_ofs = newscreen->width*y1+x1; /* Start offset in console window */
  buf_ofs = newscreen->width*y1+x1; /* Start offset in buffer */
  x2 = x2-x1+1;        /* Width of refresh area */
  osplus = newscreen->width-x2;     /* Difference between end of refresh row and start of next */
 
  while (con_ofs > 65535)
  {
    curwindow++;
    con_ofs -= 65536;
  }
  jtp_DOSSetWindow(curwindow);

  for (i = y1; i <= y2; i++)
  {
    if (con_ofs > 65535)
    {
      con_ofs -= 65536;
      curwindow++;
      jtp_DOSSetWindow(curwindow);
    }

    if (x2+con_ofs > 65536) /* Row straddles a window boundary */
    {
      x1 = x2 + con_ofs - 65536;
      /* Show part of row in old window */
      memcpy(jtp_dos_console + con_ofs, newscreen->vpage + buf_ofs, 65536-con_ofs);
      buf_ofs += 65536-con_ofs;

      /* Change to next window */
      curwindow++;
      jtp_DOSSetWindow(curwindow);

      /* Show part of row in new window */
      memcpy(jtp_dos_console, newscreen->vpage + buf_ofs, x1);
      buf_ofs += x1;
      con_ofs = x1;
    }
    else
    {
      memcpy(jtp_dos_console + con_ofs, newscreen->vpage + buf_ofs, x2);
      buf_ofs += x2;
      con_ofs += x2;
    }

    con_ofs += osplus;
    buf_ofs += osplus;
  }
}

/*---------------------------------------------------
 DOS-specific Mouse functions
---------------------------------------------------*/

void jtp_DOSResetMouse()
{
  union REGS regs;
  regs.w.ax=0;
  int86(0x33,&regs,&regs);
}

void jtp_DOSActivateMouse()
{
  union REGS regs;
  regs.w.ax=32;
  int86(0x33,&regs,&regs);
}

void jtp_DOSShowMouse()
{
  union REGS regs;
  regs.w.ax=1;
  int86(0x33,&regs,&regs);
}

void jtp_DOSInvisMouse()
{
  union REGS regs;
  regs.w.ax=2;
  int86(0x33,&regs,&regs);
}

void jtp_DOSReadMouseButtons()
{
  union REGS regs;
  regs.w.ax=3;
  int86(0x33,&regs,&regs);
  jtp_dos_mouseb=regs.w.bx;
}

void jtp_DOSReadMouseDelta()
{
  union REGS regs;
  regs.w.ax=0x000B;
  int86(0x33,&regs,&regs);
  jtp_dos_mousex=regs.w.cx;
  jtp_dos_mousey=regs.w.dx;
}


void jtp_DOSSetMouse(int new_mx,int new_my)
{
  union REGS regs;
  regs.x.eax=4;
  regs.x.ecx=new_mx;
  regs.x.edx=new_my;
  int86(0x33,&regs,&regs);
}

void jtp_DOSReadMouse(jtp_screen_t *newscreen)
{
  jtp_dos_oldmx = jtp_dos_mousex; 
  jtp_dos_oldmy = jtp_dos_mousey; 
  jtp_dos_oldmb = jtp_dos_mouseb;
  jtp_DOSReadMouseButtons();
  jtp_DOSReadMouseDelta();
  jtp_dos_mousex = jtp_dos_oldmx+jtp_dos_mousex/2;
  jtp_dos_mousey = jtp_dos_oldmy+jtp_dos_mousey/2;

  if (jtp_dos_mousex < 0) jtp_dos_mousex = 0;
  if (jtp_dos_mousey < 0) jtp_dos_mousey = 0;
  if (jtp_dos_mousex >= newscreen->width) jtp_dos_mousex = newscreen->width-1;
  if (jtp_dos_mousey >= newscreen->height) jtp_dos_mousey = newscreen->height-1;
  jtp_DOSSetMouse(100,100);
}

/*---------------------------------------------------
 DOS-specific keyboard functions
---------------------------------------------------*/
int jtp_DOSGetch()
{
  return(getch());
}

int jtp_DOSKbHit()
{
  return(kbhit());
}


/*----------------------------------------------------------
 Interface to graphics initialization
-----------------------------------------------------------*/
jtp_DOSEnterGraphicsMode(jtp_screen_t *newscreen)
{
  jtp_DOSSetMode(newscreen->width, newscreen->height, 8);
  __djgpp_nearptr_enable();
  jtp_DOSSetMouse(100,100); /* Just to make sure */
}

jtp_DOSExitGraphicsMode()
{
  __djgpp_nearptr_disable();
  jtp_DOSGoBackToTextMode();
}

#endif
