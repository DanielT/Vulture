/* NetHack may be freely redistributed.  See license for details. */

#ifndef _vultures_gen_h_
#define _vultures_gen_h_

/*--------------------------------------------------------------------------
 General functions
--------------------------------------------------------------------------*/
#include <string>

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

/*
* Subdirectories used by Vulture's.
* These should be under the main directory.
*/
#define V_CONFIG_DIRECTORY   "config"
#define V_GRAPHICS_DIRECTORY "graphics"
#define V_SOUND_DIRECTORY    "sound"
#define V_MUSIC_DIRECTORY    "music"
#define V_MANUAL_DIRECTORY   "manual"
#define V_FONTS_DIRECTORY    "fonts"

#define OOM(do_exit) vultures_oom(do_exit, __FILE__, __LINE__)

extern std::string& trim(std::string &str);
extern char *vultures_basename(char *filename);
extern std::string vultures_make_filename(std::string subdir1, std::string subdir2, std::string name);
extern void vultures_init_gamepath(void);

extern void vultures_write_log(int msgtype, char *file,
                    int line, const char * logmessage, ...) __attribute__ ((format(printf, 4, 5)));

extern void vultures_write_log_va(int msgtype, char *file, int line,
                                  const char * logmessage, va_list args);
extern void vultures_oom(int do_exit, char *file, int line);

extern int vultures_chdir_to_datadir(char * argv0);

extern int vultures_translate_key(int cmd_key);
extern int vultures_numpad_to_hjkl(int cmd_key, int shift);
extern int vultures_make_nh_key(int sym, int mod, int ch);


#endif
