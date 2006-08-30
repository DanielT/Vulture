/* Copyright (c) Jaakko Peltonen, 2000				  */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef _vultures_gen_h_
#define _vultures_gen_h_

/*--------------------------------------------------------------------------
 General functions
--------------------------------------------------------------------------*/

#include <stdarg.h>

#ifndef __attribute__
#ifndef __GNUC__
#  define __attribute__(x)
#endif
#endif

/* Definitions */
#define V_LOG_ERROR 1
#define V_LOG_NOTE 2
#define V_LOG_DEBUG 3
#define V_LOG_NETHACK 4
#define V_LOG_WRITE_ERROR 1
#define V_LOG_WRITE_NOTE 1
#define V_LOG_WRITE_DEBUG 0

#define OOM(do_exit) vultures_oom(do_exit, __FILE__, __LINE__)

extern char *vultures_basename(const char *filename);
extern char *vultures_make_filename(const char *subdir1, const char *subdir2, const char *name);
extern void vultures_init_gamepath(void);

extern void vultures_write_log(int msgtype, const char *file,
                    int line, const char * logmessage, ...) __attribute__ ((format(printf, 4, 5)));

extern void vultures_write_log_va(int msgtype, const char *file, int line,
                                  const char * logmessage, va_list args);
extern void vultures_oom(int do_exit, const char *file, int line);

extern int vultures_chdir_to_datadir(char * argv0);

#endif
