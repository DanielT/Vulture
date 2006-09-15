/*	SCCS Id: @(#)jtp_gen.h	3.0	2000/11/12	*/
/* Copyright (c) Jaakko Peltonen, 2000				  */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef _jtp_gen_h_
#define _jtp_gen_h_

/*--------------------------------------------------------------------------
 General functions
--------------------------------------------------------------------------*/

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#ifndef __attribute__
#ifndef __GNUC__
#  define __attribute__(x)
#endif
#endif

/* Definitions */
#define JTP_LOG_ERROR 1
#define JTP_LOG_NOTE 2
#define JTP_LOG_DEBUG 3
#define JTP_LOG_NETHACK 4
#define JTP_LOG_WRITE_ERROR 1
#define JTP_LOG_WRITE_NOTE 1
#define JTP_LOG_WRITE_DEBUG 0


char *jtp_basename(const char *filename);
void jtp_usleep(unsigned long microseconds);
char *jtp_strdup(const char *str);
double jtp_clocktick(void);

void jtp_write_log_message(int msgtype, const char *file, int line, const char * logmessage, ...) __attribute__((format(printf, 4, 5)));
void jtp_write_log_message_va(int msgtype, const char *file, int line, const char * logmessage, va_list args);
void jtp_oom(int do_exit, const char *file, int line);
#define OOM(do_exit) jtp_oom(do_exit, __FILE__, __LINE__)

char      jtp_in_area(int x, int y, int x1,int y1,int x2,int y2);

void jtp_play_wave_sound(char *);
void jtp_play_midi_song(char *);
void jtp_play_mp3_song(char *);

#endif
