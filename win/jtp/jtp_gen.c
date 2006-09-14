/*	SCCS Id: @(#)jtp_gen.c	3.0	2000/11/12	*/
/* Copyright (c) Jaakko Peltonen, 2000				  */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef _jtp_gen_c_
#define _jtp_gen_c_

#include <time.h>
#ifdef USE_DIRECTX_SYSCALLS
#include "jtp_dirx.h"
#endif
#ifdef USE_DOS_SYSCALLS
#include "jtp_dos.h"
#endif
#ifdef USE_SDL_SYSCALLS
#include "jtp_sdl.h"
#endif
#include "jtp_gen.h"

#define JTP_LOG_FILENAME "jtp_log.txt"

/*--------------------------------------------------------------------------
 General functions
--------------------------------------------------------------------------*/

#include "jtp_gen.h"


jtp_uint4 jtp_randvar[4]={137269,31415821,100000000,10000};

/*--------------------------------------------------------------------------
 Log file writing
--------------------------------------------------------------------------*/
void jtp_write_log_message(char * logmessage)
{
  FILE * f;
  
  f = fopen(JTP_LOG_FILENAME, "a");
  if (!f) 
  { 
    printf("ERROR: could not open log file for appending.\n");
    printf("Message was: %s\n", logmessage); 
    return; 
  }
  fprintf(f, "%s", logmessage);
  fclose(f);
}

/*--------------------------------------------------------------------------
 Range checking
--------------------------------------------------------------------------*/
unsigned char jtp_inrange(jtp_sint4 _a1, jtp_sint4 _a2, jtp_sint4 _x0)
{
 if ((_x0>=_a1) && (_x0<=_a2)) return(1);
 return(0);
}

char jtp_in_area(int x, int y, int x1,int y1,int x2,int y2)
{
 if ((x>=x1)&&(x<=x2)&&(y>=y1)&&(y<=y2))
   return(1);
 
 return(0);  
}


/*--------------------------------------------------------------------------
 Random number generation
--------------------------------------------------------------------------*/
jtp_uint4 jtp_randmult(jtp_uint4 p,jtp_uint4 q)
{
 jtp_uint4 p1,p0,q1,q0;
 p1=p/jtp_randvar[3];
 p0=p%jtp_randvar[3];
 q1=q/jtp_randvar[3];
 q0=q%jtp_randvar[3];
 return((((p0*q1+p1*q0)%jtp_randvar[3])*jtp_randvar[3]+p0*q0)%jtp_randvar[2]);
}


jtp_uint4 jtp_random(jtp_uint4 range)
{
 return random()%range;
/*
 randvar[0]=(jtp_randmult(jtp_randvar[0],jtp_randvar[1])+1)%jtp_randvar[2];
 return(((jtp_randvar[0]/jtp_randvar[3])*range)/jtp_randvar[3]);
*/
}


void jtp_init_random()
{
 srandom(time(NULL));
// randvar[0]=(jtp_uint4)time((time_t *)NULL)%100;
}


/*---------------------------------------------------------------------------
 File handling
---------------------------------------------------------------------------*/

/*-------------------------------------------------------------------------
 Function jtp_file_size returns the size (in bytes) of a file. 
 It preserves the file offset.                                                        
-------------------------------------------------------------------------*/
jtp_uint4 jtp_file_size(FILE *f)
{
 jtp_uint4 cur_pos,f_size;

 cur_pos=ftell(f);
 fseek(f,0,SEEK_END);
 f_size=ftell(f);
 fseek(f,cur_pos,SEEK_SET);

 return(f_size);
}

/*---------------------------------------------------------------------------
 Keyboard reading
---------------------------------------------------------------------------*/
int jtp_getch()
{
#ifdef USE_DIRECTX_SYSCALLS
  return(jtp_DXGetch());
#endif
#ifdef USE_DOS_SYSCALLS
  return(jtp_DOSGetch());
#endif
#ifdef USE_SDL_SYSCALLS
  return(jtp_SDLGetch());
#endif
}

int jtp_kbhit()
{
#ifdef USE_DIRECTX_SYSCALLS
  return(jtp_DXKbHit());
#endif
#ifdef USE_DOS_SYSCALLS
  return(jtp_DOSKbHit());
#endif
#ifdef USE_SDL_SYSCALLS
  return(jtp_SDLKbHit());
#endif
}

/*----------------------------------------------------------------------------
 Music and sound playing
----------------------------------------------------------------------------*/
void jtp_play_midi_song(char * midifilename)
{
#ifdef USE_DIRECTX_SYSCALLS
  jtp_DXPlayMIDISong(midifilename);
#endif
#ifdef USE_SDL_SYSCALLS
  jtp_SDLPlayMIDISong(midifilename);
#endif
}

void jtp_play_cd_track(char * cdtrackname)
{
#ifdef USE_DIRECTX_SYSCALLS
  jtp_DXPlayCDTrack(cdtrackname);
#endif
#ifdef USE_SDL_SYSCALLS
  jtp_SDLPlayCDTrack(cdtrackname);
#endif
}

void jtp_play_mp3_song(char * mp3filename)
{
#ifdef USE_SDL_SYSCALLS
  jtp_SDLPlayMP3Song(mp3filename);
#endif
}

void jtp_stop_music()
{
#ifdef USE_DIRECTX_SYSCALLS
  jtp_DXStopMusic();
#endif
#ifdef USE_SDL_SYSCALLS
  jtp_SDLStopMusic();
#endif
}

int jtp_is_music_playing()
{
#ifdef USE_DIRECTX_SYSCALLS
  return(jtp_DXIsMusicPlaying());
#endif
#ifdef USE_DOS_SYSCALLS
  return(1);
#endif
#ifdef USE_SDL_SYSCALLS
  return(jtp_SDLIsMusicPlaying());
#endif
}


void jtp_play_wave_sound
(
  char * wavefilename,
  int samples_per_sec,
  int bits_per_sample,
  int nchannels
)
{
#ifdef USE_DIRECTX_SYSCALLS
  jtp_DXPlayWaveSound(wavefilename, samples_per_sec, bits_per_sample, nchannels);
#endif
#ifdef USE_SDL_SYSCALLS
  jtp_SDLPlayWaveSound(wavefilename, samples_per_sec, bits_per_sample, nchannels);
#endif
}


#endif
