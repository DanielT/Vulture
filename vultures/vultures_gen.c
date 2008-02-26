/* Copyright (c) Jaakko Peltonen, 2000				  */
/* NetHack may be freely redistributed.  See license for details. */

#include <stdlib.h>

#ifdef WIN32
#include "win32api.h"
#else
#include <libgen.h>
#endif	/* WIN32 */

#include "hack.h"

#include "vultures_sdl.h"
#include "vultures_gen.h"
#include "vultures_opt.h"

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



/* appends the name of a data file to it's subdir name to get a name relative to the executable */
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


/* locate the data dir and chdir there so that data files can be loaded */
int vultures_chdir_to_datadir(char * argv0)
{
#if !defined(WIN32) && !defined(CHDIR) && defined(RELOCATEABLE)
    int len = 1024;
    char fullname[1024];
    char testfile[1024];
    char * dir_name;
    int dir_found = 0;

    /* first try some OS-dependant ways of finding where the executable is located */
#if defined(__APPLE__)
    if (_NSGetExecutablePath(fullname, &len) == 0) {
#elif defined(LINUX)
    if ((len = readlink("/proc/self/exe", fullname, 1024)) > 0) {
#else
    if (0) {
#endif
        dir_name = dirname(fullname);
        snprintf(testfile, 1024, "%s/%s", dir_name, "graphics/gametiles.bin");

        if (access(testfile, R_OK) == 0)
            dir_found = 1;
    }

    /* if that failed, try if argv[0] contains a path we might be able to use */
    if (!dir_found)
    {
        strncpy(fullname, argv0, 1024);
        dir_name = dirname(fullname);
        snprintf(testfile, 1024, "%s/%s", dir_name, "graphics/gametiles.bin");

        if (access(testfile, R_OK) == 0)
            dir_found = 1;
    }

    if (dir_found)
        chdir(dir_name);


/* we need to duplicate this code from unixmain.c here, because
 * otherwise it only runs if CHDIR is defined */
#if defined(VAR_PLAYGROUND)
    len = strlen(VAR_PLAYGROUND);

    fqn_prefix[SCOREPREFIX] = malloc(len+2);
    strcpy(fqn_prefix[SCOREPREFIX], VAR_PLAYGROUND);
    if (fqn_prefix[SCOREPREFIX][len-1] != '/') {
        fqn_prefix[SCOREPREFIX][len] = '/';
        fqn_prefix[SCOREPREFIX][len+1] = '\0';
    }
    fqn_prefix[LEVELPREFIX]   = fqn_prefix[SCOREPREFIX];
    fqn_prefix[SAVEPREFIX]    = fqn_prefix[SCOREPREFIX];
    fqn_prefix[BONESPREFIX]   = fqn_prefix[SCOREPREFIX];
    fqn_prefix[LOCKPREFIX]    = fqn_prefix[SCOREPREFIX];
    fqn_prefix[TROUBLEPREFIX] = fqn_prefix[SCOREPREFIX];
#endif

    return dir_found;

#else
    return 0;
#endif
}


void vultures_init_gamepath(void)
{
  /* Get starting directory, and save it for reference */
	char hackdir[1024];
	getcwd(hackdir, sizeof(hackdir));
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



/* Rotate numpad keys for movement */
int vultures_translate_key(int cmd_key)
{
    const char num_translation_table[9] = {'2', '3', '6', '1', '5', '9', '4', '7', '8'};
    const char hjkl_translation_table[9] = {'j', 'n', 'l', 'b', '.', 'u', 'h', 'y', 'k'};
    const char * hjkl_untranslated = "bjnh.lyku\0";

    static int vultures_last_translated_key = 0;

    int is_upper, charpos = 0;

    /* Count keys aren't translated */
    if (vultures_last_translated_key == 'n' && isdigit(cmd_key))
        return cmd_key;

    vultures_last_translated_key = cmd_key;

    if (vultures_opts.no_key_translation || cmd_key == '.')
        return cmd_key;

    if (iflags.num_pad) {
        if (isdigit(cmd_key))
            return num_translation_table[cmd_key -'0' - 1];
    }
    else
    {
        /* note: the bit-twiddling converts between upper and lower case chars */
        is_upper = !(cmd_key & (1 << 5));

        /* can't use strchr here: it's function depends on the locale and it thinks ',' is '.' */
        while (hjkl_untranslated[charpos] != (cmd_key | (1 << 5)) && hjkl_untranslated[charpos])
            charpos++;

        if (hjkl_untranslated[charpos])
            return hjkl_translation_table[charpos] & ~(is_upper << 5);
    }

    return cmd_key;
}


int vultures_numpad_to_hjkl(int cmd_key, int shift)
{
    const char translation_table[9] = {'B', 'J', 'N', 'H', '.', 'L', 'Y', 'K', 'U'};

    if (!isdigit(cmd_key) || (iflags.num_pad && cmd_key != '.'))
        return cmd_key;

    /* look up the letter in the translation table, then set bit 6 if shift is true
     * note that bit 6 is the difference between an upper cas and a lower case char */
    return translation_table[cmd_key -'0' - 1] | (!shift << 5);
}

