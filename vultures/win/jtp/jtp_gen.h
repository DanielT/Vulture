/*	SCCS Id: @(#)jtp_gen.h	3.0	2000/11/12	*/
/* Copyright (c) Jaakko Peltonen, 2000				  */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef _jtp_gen_h_
#define _jtp_gen_h_

/*--------------------------------------------------------------------------
 General functions
--------------------------------------------------------------------------*/

#include "jtp_def.h"
#include <stdio.h>
#include <string.h>

void jtp_write_log_message(char * logmessage);
int  jtp_getch();
int  jtp_kbhit();

jtp_uint4 jtp_file_size(FILE *f);
unsigned char jtp_inrange(jtp_sint4 _a1, jtp_sint4 _a2, jtp_sint4 _x0);
char      jtp_in_area(int x, int y, int x1,int y1,int x2,int y2);

void jtp_play_wave_sound(char *);
void jtp_play_midi_song(char *);
void jtp_play_mp3_song(char *);
void jtp_play_cd_track(char *);
void jtp_stop_music();
int  jtp_is_music_playing();


#endif


