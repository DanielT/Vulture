/* Copyright (c) Jaakko Peltonen, 2000				  */
/* NetHack may be freely redistributed.  See license for details. */

#include <stdlib.h>
#include <time.h>
#ifdef WIN32
#include "win32api.h"
#else
#include <sys/time.h>
#endif	/* WIN32 */

#include "hack.h"

#include "vultures_sdl.h"
#include "vultures_gen.h"

/* Remove/undefine this to have all log messages end up in stderr */
#define V_LOG_FILENAME "vultures_log.txt"


extern void append_slash(char *);


char vultures_game_path[1024];

/*--------------------------------------------------------------------------
General functions
--------------------------------------------------------------------------*/

char *vultures_basename(const char *filename)
{
    char *basename,	*basename2;

    basename = strrchr(filename, '/');
    if (basename == NULL)
        basename = (char *) filename;
    else
        basename++;
    basename2 = strrchr(basename, '\\');
    if (basename2 == NULL)
        basename2 = basename;
    else
        basename2++;
    return basename2;
}



char *vultures_make_filename(const char *subdir1, const char *subdir2, const char *name)
{
    char *filename;

    filename = malloc(strlen(vultures_game_path) + 1 +
        (subdir1 ? strlen(subdir1) + 2 : 0) +
        (subdir2 ? strlen(subdir2) + 2 : 0) +
        strlen(name) + 1);
    if (filename == NULL)
        OOM(1);

    strcpy(filename, vultures_game_path);
    append_slash(filename);
    if (subdir1)
    {
        strcat(filename, subdir1);
        append_slash(filename);
    }
    if (subdir2)
    {
        strcat(filename, subdir2);
        append_slash(filename);
    }
    strcat(filename, name);
    return filename;
}



void vultures_init_gamepath(void)
{
  /* Get starting directory, and save it for reference */
#ifdef CHDIR
	char hackdir[1024];
	getcwd(hackdir, sizeof(hackdir));
#endif
	strncpy(vultures_game_path, hackdir, 1024);
}



/*--------------------------------------------------------------------------
Log file writing
--------------------------------------------------------------------------*/

void vultures_write_log_va(int msgtype, const char *file, int line, const char *logmessage, va_list args)
{
    FILE *f = NULL;

    if (logmessage == NULL || *logmessage == '\0')
        return;

#ifdef WIN32
    if (msgtype == V_LOG_ERROR && !iflags.window_inited)
    {
        char buf[1024];

#  ifdef _MSC_VER
#    define vsnprintf _vsnprintf
#  endif
        vsnprintf(buf, sizeof(buf), logmessage, args);
        MessageBox(NULL, buf, NULL, MB_OK);
    }
#endif	/* WIN32 */
    if ((msgtype == V_LOG_NOTE) && (V_LOG_WRITE_NOTE == 0))
        return;
    if ((msgtype == V_LOG_ERROR) && (V_LOG_WRITE_ERROR == 0))
        return;
    if ((msgtype == V_LOG_DEBUG) && (V_LOG_WRITE_DEBUG == 0))
        return;

#ifdef V_LOG_FILENAME
    f = fopen(V_LOG_FILENAME, "a");
    if (!f)
    {
        perror("ERROR: could not open log file " V_LOG_FILENAME
            " for appending");
    }
#endif /* V_LOG_FILENAME */
    if (file != NULL)
    {
        fprintf(f ? : stderr, "[%s: %d] %s: ", vultures_basename(file), line,
                msgtype == V_LOG_NOTE ? "Note" :
                msgtype == V_LOG_DEBUG ? "Debug" :
                "ERROR");
    }
    if (msgtype == V_LOG_NETHACK)
        fprintf(f ? : stderr, "[%s]: ", hname ? hname : "NetHack");
    vfprintf(f ? : stderr, logmessage, args);
    if (f) fclose(f);
}


void vultures_write_log(int msgtype, const char *file, int line, const char *logmessage, ...)
{
    va_list args;

    va_start(args, logmessage);
    vultures_write_log_va(msgtype, file, line, logmessage, args);
    va_end(args);
}


void vultures_oom(int do_exit, const char *file, int line)
{
    vultures_write_log(V_LOG_ERROR, file, line, "Out of memory!\n");
    if (do_exit)
    {
        vultures_exit_graphics_mode();
        exit(1);
    }
}

