/*	SCCS Id: @(#)jtp_gen.c	3.0	2000/11/12	*/
/* Copyright (c) Jaakko Peltonen, 2000				  */
/* NetHack may be freely redistributed.  See license for details. */

#include <time.h>
#ifdef WIN32
#include "win32api.h"
#else
#include <sys/time.h>
#endif	/* WIN32 */
#include "jtp_sdl.h"
#include "jtp_gen.h"
#include "jtp_win.h"

#define JTP_LOG_FILENAME "jtp_log.txt"

/*--------------------------------------------------------------------------
 General functions
--------------------------------------------------------------------------*/

char *jtp_basename(const char *filename)
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

char *jtp_strdup(const char *str)
{
	char *buf = (char *)malloc((strlen(str) + 1) * sizeof(*buf));
	if (buf == NULL)
		OOM(1);
	return strcpy(buf, str);
}

void jtp_usleep(unsigned long microseconds)
{
	if (microseconds == 0)
		return;
#ifdef WIN32
	Sleep(microseconds / 1000);
#else
# ifdef HAVE_NANOSLEEP
	{
		struct timespec request, remaining;
		request.tv_sec = microseconds / 1000000;
		request.tv_nsec = 1000 * (microseconds % 1000000);
		while (nanosleep(&request, &remaining) == -1 && errno == EINTR)
			request = remaining;
	}
# else
	{
		struct timeval tv;
		tv.tv_sec = microseconds / 1000000;
		tv.tv_usec = microseconds % 1000000;
		select(0, NULL, NULL, NULL, &tv);
	}
# endif /* HAVE_NANOSLEEP */
#endif /* WIN32 */
}

double jtp_clocktick(void)
{
#ifdef WIN32
#ifdef _M_IX86
	FILETIME ft;
	unsigned __int64 *time64;
	
	GetSystemTimeAsFileTime(&ft);
	time64 = (unsigned __int64 *)(&ft.dwLowDateTime);
	
	/* Convert from 100s of nanoseconds since 1601-01-01
	 * to Unix epoch. Yes, this is Y2038 unsafe.
	 */
	*time64 -= 116444736000000000LL;
	*time64 /= 10;
	return *time64 / 1000000.0;
#else
	return GetTickCount() / 1000.0;
#endif
#else
	struct timeval r;

	gettimeofday(&r, NULL);
	return r.tv_sec + r.tv_usec / 1000000.0;
#endif
}

/*--------------------------------------------------------------------------
 Log file writing
--------------------------------------------------------------------------*/

void jtp_write_log_message_va(int msgtype, const char *file, int line, const char *logmessage, va_list args)
{
	FILE *f;

	if (logmessage == NULL || *logmessage == '\0')
		return;

#ifdef WIN32
	if (msgtype == JTP_LOG_ERROR && !iflags.window_inited)
	{
		char buf[1024];

#  ifdef _MSC_VER
#    define vsnprintf _vsnprintf
#  endif
		vsnprintf(buf, sizeof(buf), logmessage, args);
		MessageBox(NULL, buf, NULL, MB_OK);
	}
#endif	/* WIN32 */
	if ((msgtype == JTP_LOG_NOTE) && (JTP_LOG_WRITE_NOTE == 0))
		return;
	if ((msgtype == JTP_LOG_ERROR) && (JTP_LOG_WRITE_ERROR == 0))
		return;
	if ((msgtype == JTP_LOG_DEBUG) && (JTP_LOG_WRITE_DEBUG == 0))
		return;

	f = fopen(JTP_LOG_FILENAME, "a");
	if (!f)
	{
		printf("ERROR: could not open log file for appending.\n");
		printf("Message was: %s\n", logmessage);
	}
	if (file != NULL)
	{
		fprintf(f, "[%s: %d] %s: ", jtp_basename(file), line,
				msgtype == JTP_LOG_NOTE ? "Note" :
				msgtype == JTP_LOG_DEBUG ? "Debug" :
				"ERROR");
	}
	if (msgtype == JTP_LOG_NETHACK)
		fprintf(f, "[%s]: ", hname ? hname : "NetHack");
	vfprintf(f, logmessage, args);
	fclose(f);
}


void jtp_write_log_message(int msgtype, const char *file, int line, const char *logmessage, ...)
{
	va_list args;

	va_start(args, logmessage);
	jtp_write_log_message_va(msgtype, file, line, logmessage, args);
	va_end(args);
}


void jtp_oom(int do_exit, const char *file, int line)
{
	jtp_write_log_message(JTP_LOG_ERROR, file, line, "Out of memory!\n");
	if (do_exit)
	{
		jtp_exit_graphics_mode();
		exit(1);
	}
}

/*--------------------------------------------------------------------------
 Range checking
--------------------------------------------------------------------------*/
char jtp_in_area(int x, int y, int x1,int y1,int x2,int y2)
{
 if ((x>=x1)&&(x<=x2)&&(y>=y1)&&(y<=y2))
   return(1);
 
 return(0);  
}

