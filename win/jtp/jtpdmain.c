/*	SCCS Id: @(#)jtpdmain.c	3.0	2000/11/12	*/
/* Copyright (c) Jaakko Peltonen, 2000				  */
/* NetHack may be freely redistributed.  See license for details. */

#include "windows.h"
#include "jtp_dirx.h"
#include "jtpdmain.h"
#include <string.h>

extern "C"
{
#include "hack.h"
#include "jtp_win.h"
  extern int pcmain(int argc, char **argv);
}

int WINAPI WinMain 
(
  HINSTANCE hThisInstance, 
  HINSTANCE hPrevInstance, 
  LPSTR lpszCmdParam, 
  int nCmdShow
)
{
  int argc;
  char **argv;
  char *argscopy;
  char *newarg;

  jtp_DXSetInstance(hThisInstance, hPrevInstance, lpszCmdParam, nCmdShow);

  /* The first command line argument must be the executable name */
  argc = 1;
  argv = (char **)calloc(argc, sizeof(char *));
  argv[0] = (char *)malloc(strlen("nethack.exe")+1);
  strcpy(argv[0], "nethack.exe");

  /* Separate the other command line arguments */
  argscopy = (char *)malloc(strlen(lpszCmdParam)+1);
  strcpy(argscopy, lpszCmdParam);
  if ((newarg = strtok(argscopy, " ")) != NULL)
  {
    do 
    {
      if (strcmp(newarg, "") != 0) /* Whitespace blocks ("  ") produce empty tokens */
      {
        argc++;
        argv = (char **)realloc(argv, argc*sizeof(char *));
        argv[argc-1] = (char *)malloc(strlen(newarg)+1);
        strcpy(argv[argc-1], newarg);
      }
    }
    while ((newarg = strtok(NULL, " ")) != NULL);
  }
  free(argscopy);

  pcmain(argc, argv);
#ifdef LAN_FEATURES
  init_lan_features();
#endif
  moveloop();

  /* 
   * We should free the arguments, but since the memory is 
   * freed anyway when the program exits, we'll skip this.
   */

/*  nethack_exit(EXIT_SUCCESS); */
  return(0);
}
