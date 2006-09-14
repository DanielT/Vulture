/*      SCCS Id: @(#)jtp_sdl.c  3.0     2000/11/12      */
/* Copyright (c) Jaakko Peltonen, 2000				  */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef _jtp_sdl_c_
#define _jtp_sdl_c_

/*-------------------------------------------------------------------
 jtp_sdl.c : SDL API calls for Vulture's Eye windowing system.
 Requires SDL 1.1 or newer.
-------------------------------------------------------------------*/

#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include "SDL.h"
#include "SDL_video.h"
#include "SDL_audio.h"
#include "SDL_error.h"
#include "jtp_def.h"
#include "jtp_gra.h"
#include "jtp_gen.h"
#include "jtp_txt.h"
#include "jtp_gfl.h"
#include "jtp_mou.h"
#include "jtp_win.h"
#include "jtp_sdl.h"


/* Definitions */
#define JTP_SDL_LOG_FILENAME "jtp_log.txt"
#define JTP_SDL_LOG_ERROR 1
#define JTP_SDL_LOG_NOTE 2
#define JTP_SDL_LOG_DEBUG 3
#define JTP_SDL_LOG_WRITE_ERROR 1
#define JTP_SDL_LOG_WRITE_NOTE 0
#define JTP_SDL_LOG_WRITE_DEBUG 0

#define JTP_SDL_MAX_BUFFERED_KEYS 100

#define JTP_SDL_MAX_POLLED_MESSAGES 20
#define JTP_SDL_NO_MESSAGE_POLLING -1
#define JTP_SDL_POLLED_MESSAGE_NOT_ARRIVED 0
#define JTP_SDL_POLLED_MESSAGE_ARRIVED 1

/* 
 * Sound capability: 
 *  4 simultaneous sounds, 
 *  each with 4 seconds of stereo 22050 Hz 8-bit data
 */
#define JTP_SDL_MAX_SOUNDS 4
#define JTP_SDL_MAX_CACHED_SOUNDS 40
#define JTP_SDL_SOUND_BUFFER_SIZE 500000
#define JTP_SDL_SOUND_BUFFER_FREQUENCY 44100
#define JTP_SDL_SOUND_BUFFER_BITS 16
#define JTP_SDL_SOUND_BUFFER_CHANNELS 1

typedef struct {
  char * samples;
  int    length;
  char * filename;
} jtp_sdl_cached_sound;


/* NetHack interface -related objects */
int jtp_sdl_appisactive = 1;
int jtp_sdl_mousex, jtp_sdl_mousey, jtp_sdl_mouseb;

/* Key buffering */
int jtp_sdl_n_keys_in_buffer = 0;
int jtp_sdl_key_buffer[JTP_SDL_MAX_BUFFERED_KEYS];

/* Message polling */
int   jtp_sdl_polled_messages[JTP_SDL_MAX_POLLED_MESSAGES]; /* Messages that are polled for */
int    jtp_sdl_n_polled_messages;      /* N. of messages polled for */
int    jtp_sdl_polled_message_arrived; /* Has any polled message arrived? */
int   jtp_sdl_polled_message;         /* The particular message that ended the polling */


/* Graphics objects */
SDL_Surface * jtp_sdl_screen;          /* Graphics surface */
SDL_Color   * jtp_sdl_colors = NULL;   /* Graphics palette */

/* Sound effects objects */
SDL_AudioSpec jtp_sdl_audio_wanted;    /* Audio specification */
static Uint8 * jtp_sdl_audio_chunk;    /* Audio chunk (currently playing sound) */
static Uint32  jtp_sdl_audio_len;      /* Length of the audio chunk */
static Uint8 * jtp_sdl_audio_pos;      /* Current position  in the audio chunk */
jtp_sdl_cached_sound * jtp_sdl_cached_sounds;
int                  jtp_sdl_oldest_cached_sound = 0;

/* Music objects */
SDL_CD * jtp_sdl_cdrom = NULL;
pid_t jtp_sdl_music_player_pid = -1;
char * jtp_sdl_home_directory = NULL;

/*
MCI_OPEN_PARMS       jtp_dx_mciOpenParms;
MCI_LOAD_PARMS       jtp_dx_mciLoadParms;
MCI_PLAY_PARMS       jtp_dx_mciPlayParms;
MCI_STATUS_PARMS     jtp_dx_mciStatusParms;
MCI_GENERIC_PARMS    jtp_dx_mciGenericParms;
int                  jtp_dx_mci_opened = 0;
int                  jtp_dx_mci_playing = 0;
*/

/*--------------------------------------------------------------------------
 Log file writing
--------------------------------------------------------------------------*/
void jtp_SDLWriteLogMessage(int msgtype, char * logmessage)
{
  FILE * f;

  if ((msgtype == JTP_SDL_LOG_NOTE) && (JTP_SDL_LOG_WRITE_NOTE == 0)) return;
  if ((msgtype == JTP_SDL_LOG_ERROR) && (JTP_SDL_LOG_WRITE_ERROR == 0)) return;
  if ((msgtype == JTP_SDL_LOG_DEBUG) && (JTP_SDL_LOG_WRITE_DEBUG == 0)) return;

  f = fopen(JTP_SDL_LOG_FILENAME, "a"); 
  if (!f) return;
  fprintf(f, "%s", logmessage);
  fclose(f);
}


/* Music player signal handler */
void jtp_sdl_handle_music_player_signal(int sig_id)
{
  if (sig_id == SIGCHLD)
  {
    if (jtp_sdl_music_player_pid > 0)
      kill(jtp_sdl_music_player_pid, SIGKILL);
    jtp_sdl_music_player_pid = -1;
  }
}


void jtp_SDLFillAudio(void *udata, Uint8 *stream, int len)
{
  /* Only play if we have data left */
  if ( jtp_sdl_audio_len == 0 )
    return;

  /* Mix as much data as possible */
  if (jtp_sdl_audio_len < len)
  {
    SDL_MixAudio(stream, jtp_sdl_audio_pos, jtp_sdl_audio_len, SDL_MIX_MAXVOLUME);
    jtp_sdl_audio_pos += jtp_sdl_audio_len; 
    jtp_sdl_audio_len = 0;
  }
  else
  {
    SDL_MixAudio(stream, jtp_sdl_audio_pos, len, SDL_MIX_MAXVOLUME);
    jtp_sdl_audio_pos += len;
    jtp_sdl_audio_len -= len;
  }
}

void jtp_SDLPlaySound
(
  int nbytes,
  unsigned char * samples
)
{
  int i;
  
  /* Interrupt audio while the copying is done */
  SDL_PauseAudio(1);

  /* Copy the sound into the audio chunk */
  for (i = 0; i < nbytes; i++)
    ((unsigned char *)jtp_sdl_audio_chunk)[i] = samples[i];
  jtp_sdl_audio_pos = jtp_sdl_audio_chunk;
  jtp_sdl_audio_len = nbytes;

  /* Allow SDL to play the sound */  
  SDL_PauseAudio(0);
}

void jtp_SDLProcessEvent
(
  SDL_Event *cur_event
)
{
  int i;

  /* Check for polled messages */
  if (jtp_sdl_polled_message_arrived == JTP_SDL_POLLED_MESSAGE_NOT_ARRIVED)
  {
    if (jtp_sdl_n_polled_messages > 0) /* Only certain messages polled for */
    {
      for (i = 0; i < jtp_sdl_n_polled_messages; i++)
        if (cur_event->type == jtp_sdl_polled_messages[i])
        {
          jtp_sdl_polled_message_arrived = JTP_SDL_POLLED_MESSAGE_ARRIVED;
          jtp_sdl_polled_message = cur_event->type;
        }
    }
    else /* Any message is accepted */
    {
      jtp_sdl_polled_message_arrived = JTP_SDL_POLLED_MESSAGE_ARRIVED;
      jtp_sdl_polled_message = cur_event->type;
    }
  }    
    
  /* Process the message */
  switch (cur_event->type)
  {
    case SDL_KEYDOWN:
      switch((cur_event->key).keysym.sym)
      {
        case SDLK_TAB:     /* Allow Alt+Tab task switching */
        case SDLK_LSHIFT:  /* Case shift is not a separate key */
        case SDLK_RSHIFT:  /* Case shift is not a separate key */
        case SDLK_LCTRL:   /* Control key is not a separate key */
        case SDLK_RCTRL:   /* Control key is not a separate key */
        case SDLK_LALT:    /* ALT/Meta key is not a separate key */
        case SDLK_RALT:    /* ALT/Meta key is not a separate key */
        case SDLK_LMETA:   /* ALT/Meta key is not a separate key */
        case SDLK_RMETA:   /* ALT/Meta key is not a separate key */
        case SDLK_MODE:
        case SDLK_LSUPER:
        case SDLK_RSUPER:
          break;
        default:
          switch((cur_event->key).keysym.sym)
          {
            case SDLK_RETURN: i='\r'; break;
            case SDLK_PAUSE: i=JTP_KEY_PAUSE; break;
            case SDLK_ESCAPE: i=27; break;
            case SDLK_SPACE: i=' '; break;
            case SDLK_EXCLAIM: i = (cur_event->key).keysym.unicode; break;
            case SDLK_QUOTEDBL: i = (cur_event->key).keysym.unicode; break;
            case SDLK_HASH: i = (cur_event->key).keysym.unicode; break;
            case SDLK_DOLLAR: i = (cur_event->key).keysym.unicode; break;
	    case SDLK_AMPERSAND: i = (cur_event->key).keysym.unicode; break;
            case SDLK_QUOTE: i = (cur_event->key).keysym.unicode; break;
            case SDLK_LEFTPAREN: i = (cur_event->key).keysym.unicode; break;
            case SDLK_RIGHTPAREN: i = (cur_event->key).keysym.unicode; break;
            case SDLK_ASTERISK: i = (cur_event->key).keysym.unicode; break;
            case SDLK_PLUS: i = (cur_event->key).keysym.unicode; break;
            case SDLK_COMMA: i = (cur_event->key).keysym.unicode; break;
            case SDLK_MINUS: i = (cur_event->key).keysym.unicode; break;
            case SDLK_PERIOD: i = (cur_event->key).keysym.unicode; break;
            case SDLK_SLASH: i = (cur_event->key).keysym.unicode; break;
            case SDLK_0: i = (cur_event->key).keysym.unicode; break;
            case SDLK_1: i = (cur_event->key).keysym.unicode; break;
            case SDLK_2: i = (cur_event->key).keysym.unicode; break; 
            case SDLK_3: i = (cur_event->key).keysym.unicode; break; 
            case SDLK_4: i = (cur_event->key).keysym.unicode; break; 
            case SDLK_5: i = (cur_event->key).keysym.unicode; break; 
            case SDLK_6: i = (cur_event->key).keysym.unicode; break; 
            case SDLK_7: i = (cur_event->key).keysym.unicode; break; 
            case SDLK_8: i = (cur_event->key).keysym.unicode; break; 
            case SDLK_9: i = (cur_event->key).keysym.unicode; break; 
            case SDLK_COLON: i = (cur_event->key).keysym.unicode; break;
            case SDLK_SEMICOLON: i = (cur_event->key).keysym.unicode; break;
            case SDLK_LESS: i = (cur_event->key).keysym.unicode; break;
            case SDLK_EQUALS: i = (cur_event->key).keysym.unicode; break;
            case SDLK_GREATER: i = (cur_event->key).keysym.unicode; break;
            case SDLK_QUESTION: i = (cur_event->key).keysym.unicode; break;
            case SDLK_AT: i = (cur_event->key).keysym.unicode; break;
            case SDLK_LEFTBRACKET: i = (cur_event->key).keysym.unicode; break;
            case SDLK_BACKSLASH: i = (cur_event->key).keysym.unicode; break;
            case SDLK_RIGHTBRACKET: i = (cur_event->key).keysym.unicode; break;
            case SDLK_CARET: i = (cur_event->key).keysym.unicode; break;
            case SDLK_UNDERSCORE: i = (cur_event->key).keysym.unicode; break;
            case SDLK_BACKQUOTE: i = (cur_event->key).keysym.unicode; break;
            case SDLK_a: if ((cur_event->key).keysym.mod & KMOD_SHIFT) i='A'; else i='a'; break; 
            case SDLK_b: if ((cur_event->key).keysym.mod & KMOD_SHIFT) i='B'; else i='b'; break; 
            case SDLK_c: if ((cur_event->key).keysym.mod & KMOD_SHIFT) i='C'; else i='c'; break; 
            case SDLK_d: if ((cur_event->key).keysym.mod & KMOD_SHIFT) i='D'; else i='d'; break; 
            case SDLK_e: if ((cur_event->key).keysym.mod & KMOD_SHIFT) i='E'; else i='e'; break; 
            case SDLK_f: if ((cur_event->key).keysym.mod & KMOD_SHIFT) i='F'; else i='f'; break; 
            case SDLK_g: if ((cur_event->key).keysym.mod & KMOD_SHIFT) i='G'; else i='g'; break; 
            case SDLK_h: if ((cur_event->key).keysym.mod & KMOD_SHIFT) i='H'; else i='h'; break; 
            case SDLK_i: if ((cur_event->key).keysym.mod & KMOD_SHIFT) i='I'; else i='i'; break; 
            case SDLK_j: if ((cur_event->key).keysym.mod & KMOD_SHIFT) i='J'; else i='j'; break; 
            case SDLK_k: if ((cur_event->key).keysym.mod & KMOD_SHIFT) i='K'; else i='k'; break; 
            case SDLK_l: if ((cur_event->key).keysym.mod & KMOD_SHIFT) i='L'; else i='l'; break; 
            case SDLK_m: if ((cur_event->key).keysym.mod & KMOD_SHIFT) i='M'; else i='m'; break; 
            case SDLK_n: if ((cur_event->key).keysym.mod & KMOD_SHIFT) i='N'; else i='n'; break; 
            case SDLK_o: if ((cur_event->key).keysym.mod & KMOD_SHIFT) i='O'; else i='o'; break; 
            case SDLK_p: if ((cur_event->key).keysym.mod & KMOD_SHIFT) i='P'; else i='p'; break; 
            case SDLK_q: if ((cur_event->key).keysym.mod & KMOD_SHIFT) i='Q'; else i='q'; break; 
            case SDLK_r: if ((cur_event->key).keysym.mod & KMOD_SHIFT) i='R'; else i='r'; break; 
            case SDLK_s: if ((cur_event->key).keysym.mod & KMOD_SHIFT) i='S'; else i='s'; break; 
            case SDLK_t: if ((cur_event->key).keysym.mod & KMOD_SHIFT) i='T'; else i='t'; break; 
            case SDLK_u: if ((cur_event->key).keysym.mod & KMOD_SHIFT) i='U'; else i='u'; break; 
            case SDLK_v: if ((cur_event->key).keysym.mod & KMOD_SHIFT) i='V'; else i='v'; break; 
            case SDLK_w: if ((cur_event->key).keysym.mod & KMOD_SHIFT) i='W'; else i='w'; break; 
            case SDLK_x: if ((cur_event->key).keysym.mod & KMOD_SHIFT) i='X'; else i='x'; break; 
            case SDLK_y: if ((cur_event->key).keysym.mod & KMOD_SHIFT) i='Y'; else i='y'; break; 
            case SDLK_z: if ((cur_event->key).keysym.mod & KMOD_SHIFT) i='Z'; else i='z'; break; 
            case SDLK_DELETE: i=127; break;
	    case SDLK_BACKSPACE: i=8; break;
            case SDLK_KP0: i='0'; break;
            case SDLK_KP1: i='1'; break;
            case SDLK_KP2: i='2'; break;
            case SDLK_KP3: i='3'; break;
            case SDLK_KP4: i='4'; break;
            case SDLK_KP5: i='5'; break;
            case SDLK_KP6: i='6'; break;
            case SDLK_KP7: i='7'; break;
            case SDLK_KP8: i='8'; break;
            case SDLK_KP9: i='9'; break;
            case SDLK_KP_PERIOD: i='.'; break;
            case SDLK_KP_DIVIDE: i='/'; break;
            case SDLK_KP_MULTIPLY: i='*'; break;
            case SDLK_KP_MINUS: i='-'; break;
            case SDLK_KP_PLUS: i='+'; break;
            case SDLK_KP_ENTER: i='\r'; break;
            case SDLK_KP_EQUALS: i='='; break;
            case SDLK_UP: i=JTP_KEY_MENU_SCROLLUP; break;
            case SDLK_DOWN: i=JTP_KEY_MENU_SCROLLDOWN; break;
            case SDLK_RIGHT: i=JTP_KEY_MENU_SCROLLRIGHT; break;
            case SDLK_LEFT: i=JTP_KEY_MENU_SCROLLLEFT; break;
            case SDLK_INSERT: i=JTP_KEY_INSERT; break;
            case SDLK_HOME: i=JTP_KEY_HOME; break;
            case SDLK_END: i=JTP_KEY_END; break;
            case SDLK_PAGEUP: i=JTP_KEY_MENU_SCROLLPAGEUP; break;
            case SDLK_PAGEDOWN: i=JTP_KEY_MENU_SCROLLPAGEDOWN; break;
	    default: i='?'; /* printf("Received key %d\n", (cur_event->key).keysym.sym); */ break;
          }
          if (jtp_sdl_n_keys_in_buffer < JTP_SDL_MAX_BUFFERED_KEYS)
          {
            if (i >= 0)
            {
              /* Allow ALT/Meta key */
              if (((cur_event->key).keysym.mod & KMOD_ALT) || ((cur_event->key).keysym.mod & KMOD_META))
                i = i | (1 << 7);
              /* Allow Control key */
              if ((cur_event->key).keysym.mod & KMOD_CTRL)
                i = i & 0x1f;
              /* Store the key */
              jtp_sdl_key_buffer[jtp_sdl_n_keys_in_buffer] = i;
              jtp_sdl_n_keys_in_buffer++;
            }
          }
          break;
      }
      break;

    case SDL_MOUSEBUTTONDOWN:
      switch((cur_event->button).button)
      {
        case SDL_BUTTON_LEFT:
          if (jtp_sdl_mouseb == JTP_MBUTTON_RIGHT)
            jtp_sdl_mouseb = JTP_MBUTTON_LEFT | JTP_MBUTTON_RIGHT;
          else jtp_sdl_mouseb = JTP_MBUTTON_LEFT;
          jtp_sdl_mousex=(cur_event->button).x;
          jtp_sdl_mousey=(cur_event->button).y;
          break;
        case SDL_BUTTON_RIGHT:
          if (jtp_sdl_mouseb == JTP_MBUTTON_LEFT)
            jtp_sdl_mouseb = JTP_MBUTTON_LEFT | JTP_MBUTTON_RIGHT;
          else jtp_sdl_mouseb = JTP_MBUTTON_RIGHT;
          jtp_sdl_mousex=(cur_event->button).x;
          jtp_sdl_mousey=(cur_event->button).y;
          break;
        default: break;
      }
      break;

    case SDL_MOUSEBUTTONUP:
      switch((cur_event->button).button)
      {
        case SDL_BUTTON_LEFT:
          if ((jtp_sdl_mouseb & JTP_MBUTTON_RIGHT))
            jtp_sdl_mouseb = JTP_MBUTTON_RIGHT;
          else jtp_sdl_mouseb = JTP_MBUTTON_NONE;
          jtp_sdl_mousex=(cur_event->button).x;
          jtp_sdl_mousey=(cur_event->button).y;
          break;
        case SDL_BUTTON_RIGHT:
          if ((jtp_sdl_mouseb & JTP_MBUTTON_LEFT))
            jtp_sdl_mouseb = JTP_MBUTTON_LEFT;
          else jtp_sdl_mouseb = JTP_MBUTTON_NONE;
          jtp_sdl_mousex=(cur_event->button).x;
          jtp_sdl_mousey=(cur_event->button).y;
          break;
        default: break;
      }
      break;

    case SDL_MOUSEMOTION:
      jtp_sdl_mousex = (cur_event->motion).x;
      jtp_sdl_mousey = (cur_event->motion).y;
      jtp_sdl_mouseb = JTP_MBUTTON_NONE;
      if ((cur_event->motion).state&SDL_BUTTON(1)) jtp_sdl_mouseb = JTP_MBUTTON_LEFT;
      if ((cur_event->motion).state&SDL_BUTTON(3))
        if (jtp_sdl_mouseb == JTP_MBUTTON_LEFT)
          jtp_sdl_mouseb = JTP_MBUTTON_LEFT | JTP_MBUTTON_RIGHT;
        else jtp_sdl_mouseb = JTP_MBUTTON_RIGHT;
      break;      
  }       
}

/*
 * The function that requests message polling must fill out the
 * jtp_sdl_polled_messages array.
 */
void jtp_SDLPollForMessage(char waitformessage)
{
  SDL_Event cur_event;

  /* Process any waiting messages */
  jtp_sdl_polled_message_arrived = JTP_SDL_POLLED_MESSAGE_NOT_ARRIVED;
  while (SDL_PollEvent(&cur_event))
    jtp_SDLProcessEvent(&cur_event);  

  /* If requested, process messages until a polled one arrives */
  if (waitformessage)
  {
    while (jtp_sdl_polled_message_arrived == JTP_SDL_POLLED_MESSAGE_NOT_ARRIVED)    
      if (SDL_PollEvent(&cur_event))
        jtp_SDLProcessEvent(&cur_event);    
  }
  
  /* Clean up */  
  jtp_sdl_polled_message_arrived = JTP_SDL_NO_MESSAGE_POLLING;  
}



void jtp_SDLSetInstance
(
)
{
  int i;

  /* Set initial values for all the objects (variables) used */

  /* Linux API -related objects */ 
  
  /* NetHack interface -related objects */
  jtp_sdl_appisactive = 1;
  jtp_sdl_mousex = 100; jtp_sdl_mousey = 100; jtp_sdl_mouseb = 0;

  /* Key buffering */
  jtp_sdl_n_keys_in_buffer = 0;

  /* Message polling */
  jtp_sdl_n_polled_messages = 0;
  jtp_sdl_polled_message_arrived = JTP_SDL_NO_MESSAGE_POLLING;

  /* Graphics objects */
  jtp_sdl_colors = NULL;

  /* Sound effects objects */
  /* jtp_sdl_oldest_sound = 0; */
  jtp_sdl_cached_sounds = NULL;
  jtp_sdl_oldest_cached_sound = 0;

  /* Music objects */
}

/*
extern "C"
{
*/
int jtp_SDLGetch()
{
  int current_key;
  int i;
  
  /* Poll for a key until there's one in the buffer */
  while (jtp_sdl_n_keys_in_buffer == 0)
  {
    jtp_sdl_polled_messages[0] = SDL_KEYUP;
    jtp_sdl_polled_message_arrived = JTP_SDL_POLLED_MESSAGE_NOT_ARRIVED;
    jtp_sdl_n_polled_messages = 1;
    jtp_SDLPollForMessage(1);
    jtp_sdl_polled_message_arrived = JTP_SDL_NO_MESSAGE_POLLING;
    jtp_sdl_n_polled_messages = 0;
  }
  /* Remove key from buffer */
  current_key = jtp_sdl_key_buffer[0];    
  for (i = 0; i < jtp_sdl_n_keys_in_buffer - 1; i++)
    jtp_sdl_key_buffer[i] = jtp_sdl_key_buffer[i+1];
  jtp_sdl_n_keys_in_buffer--;
      
  return(current_key);
}

int jtp_SDLKbHit()
{
  /* Just process any waiting events, then return */
  jtp_sdl_polled_message_arrived = JTP_SDL_NO_MESSAGE_POLLING;
  jtp_sdl_n_polled_messages = 0;
  jtp_SDLPollForMessage(0); 

  if (jtp_sdl_n_keys_in_buffer > 0) return(1);
  return(0);
}

void jtp_SDLReadMouse()
{
  /* Just process any waiting events, then return */
  jtp_sdl_polled_message_arrived = JTP_SDL_NO_MESSAGE_POLLING;
  jtp_sdl_n_polled_messages = 0;
  jtp_SDLPollForMessage(0); 
}

void jtp_SDLProcessEvents()
{
  SDL_Event event;

  while (SDL_PollEvent(&event))
  {
    jtp_SDLProcessEvent(&event);
  }
}

void jtp_SDLEnterGraphicsMode(jtp_screen_t *newscreen)
{
  int i;

  if (jtp_play_effects)
  {
    if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO|SDL_INIT_CDROM) == -1)
    {
      jtp_SDLWriteLogMessage(JTP_SDL_LOG_ERROR, "[jtp_sdl.c/jtp_SDLEnterGraphicMode/Check1] ERROR: Could not initialize SDL with video and audio\n");
      jtp_SDLWriteLogMessage(JTP_SDL_LOG_ERROR, "[SDL Error] ");
      jtp_SDLWriteLogMessage(JTP_SDL_LOG_ERROR, SDL_GetError());
      jtp_SDLWriteLogMessage(JTP_SDL_LOG_ERROR, "\n");
      exit(1);
    }
  }
  else
  {
    if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_CDROM) == -1)
    {
      jtp_SDLWriteLogMessage(JTP_SDL_LOG_ERROR, "[jtp_sdl.c/jtp_SDLEnterGraphicMode/Check2] ERROR: Could not initialize SDL with video\n");
      jtp_SDLWriteLogMessage(JTP_SDL_LOG_ERROR, "[SDL Error] ");
      jtp_SDLWriteLogMessage(JTP_SDL_LOG_ERROR, SDL_GetError());
      jtp_SDLWriteLogMessage(JTP_SDL_LOG_ERROR, "\n");
      exit(1);
    }
  }

  /* Initialize the event handlers */
  atexit(SDL_Quit);
  /* Filter key, mouse and quit events */
  /*  SDL_SetEventFilter(FilterEvents); */
  /* Enable key repeat */
  SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);  

  if (jtp_fullscreen)
    jtp_sdl_screen = SDL_SetVideoMode(newscreen->width, newscreen->height, 8, SDL_SWSURFACE | SDL_FULLSCREEN);
  else
    jtp_sdl_screen = SDL_SetVideoMode(newscreen->width, newscreen->height, 8, SDL_SWSURFACE);
  if (!jtp_sdl_screen)
  {
    jtp_SDLWriteLogMessage(JTP_SDL_LOG_ERROR, "[jtp_sdl.c/jtp_SDLEnterGraphicMode/Check3] ERROR: Could not initialize video mode\n");
    jtp_SDLWriteLogMessage(JTP_SDL_LOG_ERROR, "[SDL Error] ");
    jtp_SDLWriteLogMessage(JTP_SDL_LOG_ERROR, SDL_GetError());
    jtp_SDLWriteLogMessage(JTP_SDL_LOG_ERROR, "\n");
    exit(1);
  }

  /* Don't show double cursor */
  SDL_ShowCursor(SDL_DISABLE);

  jtp_sdl_colors = (SDL_Color *)malloc(256*sizeof(SDL_Color));

  /* Enable Unicode translation. Necessary to match keypresses to characters */
  SDL_EnableUNICODE(1);

  if (jtp_play_effects)
  {
    /* Set the audio format */
    jtp_sdl_audio_wanted.freq = 22050;
    jtp_sdl_audio_wanted.format = AUDIO_S16;
    jtp_sdl_audio_wanted.channels = 2;    /* 1 = mono, 2 = stereo */
    jtp_sdl_audio_wanted.samples = 1024;  /* Good low-latency value for callback */
    jtp_sdl_audio_wanted.callback = jtp_SDLFillAudio;
    jtp_sdl_audio_wanted.userdata = NULL;

    /* Open the audio device, forcing the desired format */
    if ( SDL_OpenAudio(&jtp_sdl_audio_wanted, NULL) < 0 ) 
    {
      jtp_SDLWriteLogMessage(JTP_SDL_LOG_ERROR, "[jtp_sdl.c/jtp_SDLEnterGraphicMode/Check1] ERROR: Could not initialize SDL audio device\n");
      jtp_SDLWriteLogMessage(JTP_SDL_LOG_ERROR, "[SDL Error] ");
      jtp_SDLWriteLogMessage(JTP_SDL_LOG_ERROR, SDL_GetError());
      jtp_SDLWriteLogMessage(JTP_SDL_LOG_ERROR, "\n");
      exit(1);
    }
    jtp_sdl_audio_chunk = (Uint8 *)malloc(JTP_SDL_SOUND_BUFFER_SIZE*sizeof(Uint8));
    jtp_sdl_audio_len = 30000;
    jtp_sdl_audio_pos = NULL;

    /* Create the sound cache */
    jtp_sdl_cached_sounds = (jtp_sdl_cached_sound *)malloc(JTP_SDL_MAX_CACHED_SOUNDS*sizeof(jtp_sdl_cached_sound));
    for (i = 0; i < JTP_SDL_MAX_CACHED_SOUNDS; i++)
    {
      jtp_sdl_cached_sounds[i].length = 0;
      jtp_sdl_cached_sounds[i].samples = NULL;
      jtp_sdl_cached_sounds[i].filename = NULL;
    }

    /* SDL_PauseAudio(0); */ /* Start playing sounds */
  }

  if (jtp_play_music)
  {
    /* Initialize internal midi music library. Not implemented yet. */

    /* Initialize cd playing. */
    jtp_sdl_cdrom = NULL;
    if (SDL_CDNumDrives() > 0)
    {
      /* Open default drive */
      jtp_sdl_cdrom = SDL_CDOpen(0);
    }
  }
}

void jtp_SDLExitGraphicsMode()
{
  jtp_SDLStopMusic();
  if (jtp_sdl_cdrom) SDL_CDClose(jtp_sdl_cdrom);
  SDL_Quit();
}

void jtp_SDLRecordColor(int cindex, int r, int g, int b)
{
  jtp_sdl_colors[cindex].r = (r*255)/63;
  jtp_sdl_colors[cindex].g = (g*255)/63;
  jtp_sdl_colors[cindex].b = (b*255)/63;
}

void jtp_SDLSetPalette()
{
  SDL_SetColors(jtp_sdl_screen, jtp_sdl_colors, 0, 256);
}


void jtp_SDLRefreshRegion
(
  int x1, int y1, 
  int x2, int y2, 
  jtp_screen_t *newscreen
)
{
  Uint8 * SDLSurfaceTable;
  int i;

  /* Clip edges */
  if (x1 < 0) x1 = 0;
  if (y1 < 0) y1 = 0;
  if (x2 >= newscreen->width) x2 = newscreen->width-1;
  if (y2 >= newscreen->height) y2 = newscreen->height-1;

  /* Get the rectangle to access as a table pointer */
  SDLSurfaceTable = jtp_sdl_screen->pixels + y1*jtp_sdl_screen->pitch + x1;

  /* Plot the selected region */
  for (i = y1; i <= y2; i++)
  {
    memcpy(SDLSurfaceTable, newscreen->vpage + x1 + i*newscreen->width, x2-x1+1);
    SDLSurfaceTable += jtp_sdl_screen->pitch;
  }

  if (SDL_MUSTLOCK(jtp_sdl_screen))
    SDL_UnlockSurface(jtp_sdl_screen);

  SDL_UpdateRect(jtp_sdl_screen, x1, y1, x2-x1+1, y2-y1+1);
}

void jtp_SDLRefresh(jtp_screen_t * newscreen)
{
  jtp_SDLRefreshRegion(0, 0, newscreen->width-1, newscreen->height-1, newscreen);
}


void jtp_SDLPlayExternalMusic(char * playerstring, char * musicfilename)
{
  char tempbuffer[1024];
  char * temptoken;
  char * player_name;
  int n_player_args;
  char ** player_args;
  /*  char ** player_envs; */

  /* printf("Playing [%s] [%s]\n", playerstring, musicfilename); */

  /* If there's a previous child process playing music, stop it */
  if (jtp_sdl_music_player_pid > 0)
  {
    kill(jtp_sdl_music_player_pid, SIGKILL);
    /* Wait for the signal handler to receive SIGCHLD */
    while (jtp_sdl_music_player_pid > 0) pause();
  }

  /* Create a child process to play the music */
  jtp_sdl_music_player_pid = fork();
  if (jtp_sdl_music_player_pid == -1) /* Failure */
  {
    jtp_SDLWriteLogMessage(JTP_SDL_LOG_ERROR, "[jtp_dirx.c/jtp_SDLPlayMIDISong/Check1] ERROR: Could not create child process for MIDI player. ");
    switch(errno)
    {
      case EAGAIN: jtp_SDLWriteLogMessage(JTP_SDL_LOG_ERROR, "Reason: could not allocate memory for page tables\n"); break;
      case ENOMEM: jtp_SDLWriteLogMessage(JTP_SDL_LOG_ERROR, "Reason: could not allocate kernel structures\n"); break;
      default: jtp_SDLWriteLogMessage(JTP_SDL_LOG_ERROR, "Unknown reason.\n"); break;
    }
  }
  else if (jtp_sdl_music_player_pid > 0) /* Parent thread */
  {
    /* Set up a signal handler for the child's stop signal */
    signal(SIGCHLD, jtp_sdl_handle_music_player_signal);
  }
  else if (jtp_sdl_music_player_pid == 0) /* Child thread */
  {
    SDL_Delay(100); /* XXX Hack: Ensure that daddy goes First */ 
    /* Process the player options */
    sprintf(tempbuffer, playerstring, musicfilename);
    /* Get the path to the binary */
    temptoken = strtok(tempbuffer, " ");
    if ((temptoken) && (strlen(temptoken) > 0))
    {
      /* Copy the player name */
      player_name = (char *)malloc(strlen(temptoken)+1);
      strcpy(player_name, temptoken);

      /* Extract player arguments */
      n_player_args = 1;
      player_args = (char **)malloc(n_player_args*sizeof(char *));
      player_args[0] = player_name;

      while ((temptoken = strtok(NULL, " ")) != NULL)
      {
        if (strlen(temptoken) > 0)
	{
          n_player_args++;
          player_args = (char **)realloc(player_args, n_player_args*sizeof(char *));
          player_args[n_player_args-1] = (char *)malloc(strlen(temptoken)+1);
          strcpy(player_args[n_player_args-1], temptoken);
          /* printf("arg %d is [%s]\n", n_player_args-1, player_args[n_player_args-1]); */
	}
      }

      /* Add terminating NULL argument */
      n_player_args++;
      player_args = (char **)realloc(player_args, n_player_args*sizeof(char *));
      player_args[n_player_args-1] = NULL;


      /* Add the HOME environment variable (some players need it) */
      /*
      if (!jtp_sdl_home_directory)
      {
        jtp_sdl_home_directory = (char *)malloc(1024*sizeof(char));
        sprintf(jtp_sdl_home_directory, "HOME=%s", getenv("HOME"));
      }
      player_envs = (char **)malloc(2*sizeof(char *));
      player_envs[0] = jtp_sdl_home_directory;
      player_envs[1] = NULL;
      */

      /* Execute the player command */
      if (execv(player_name, player_args) == -1)
      {
        /* Log the error */
        jtp_SDLWriteLogMessage(JTP_SDL_LOG_ERROR, "[jtp_sdl.c/jtp_SDLPlayExternalMusic/Check2] ERROR: could not execute player command [");
        sprintf(tempbuffer, playerstring, musicfilename);
        jtp_SDLWriteLogMessage(JTP_SDL_LOG_ERROR, tempbuffer);
        jtp_SDLWriteLogMessage(JTP_SDL_LOG_ERROR, "]\n");
      }
      /* If we got here, something went wrong, so make sure to quit */
      jtp_sdl_music_player_pid = -1;
      _exit(1);      
    }
    else /* Something went wrong, so make sure to quit */
    {
      jtp_sdl_music_player_pid = -1;
      _exit(1);
    }
  }
}


void jtp_SDLPlayMIDISong(char * midifilename)
{
  if (jtp_external_midi_player_command)
    jtp_SDLPlayExternalMusic(jtp_external_midi_player_command, midifilename);
}


void jtp_SDLPlayMP3Song(char * mp3filename)
{
  if (jtp_external_mp3_player_command)
    jtp_SDLPlayExternalMusic(jtp_external_mp3_player_command, mp3filename);
}


void jtp_SDLPlayCDTrack(char * cdtrackname)
{
  int nTrack;

  if (!jtp_play_music) return;

  /* Parse the track number from the given string */
  nTrack = atoi(cdtrackname);
  if (nTrack < 0)
  { 
    jtp_SDLWriteLogMessage(JTP_SDL_LOG_DEBUG, "[jtp_dirx.c/jtp_SDLPlayCDTrack/Debug1]: Invalid track number [");
    jtp_SDLWriteLogMessage(JTP_SDL_LOG_DEBUG, cdtrackname);
    jtp_SDLWriteLogMessage(JTP_SDL_LOG_DEBUG, "]\n");
    return;
  }
 
  if (!jtp_sdl_cdrom)
  {
    jtp_SDLWriteLogMessage(JTP_SDL_LOG_DEBUG, "[jtp_dirx.c/jtp_SDLPlayCDTrack/Debug2]: CDROM not initialized\n");
    return;
  }

  if (!CD_INDRIVE(SDL_CDStatus(jtp_sdl_cdrom)))
  {
    jtp_SDLWriteLogMessage(JTP_SDL_LOG_DEBUG, "[jtp_dirx.c/jtp_SDLPlayCDTrack/Debug3]: No CD in drive\n");
    return;
  }

  SDL_CDPlayTracks(jtp_sdl_cdrom, nTrack, 0, 1, 0);
}


void jtp_SDLStopMusic()
{
  /* Stop any external music files (MIDI or MP3) playing */
  if (jtp_sdl_music_player_pid > 0)
  {
    kill(jtp_sdl_music_player_pid, SIGKILL);
    /* Wait for SIGCHLD */    
    while (jtp_sdl_music_player_pid > 0) pause();
    /* jtp_sdl_music_player_pid = -1; */
  }

  /* Stop any CD tracks playing */
  if (jtp_sdl_cdrom)
  {
    if (SDL_CDStatus(jtp_sdl_cdrom) == CD_PLAYING)
      SDL_CDStop(jtp_sdl_cdrom);
  }
}


int jtp_SDLIsMusicPlaying()
{
  /* Check for external music files (MIDI or MP3) playing */
  if (jtp_sdl_music_player_pid > 0) 
    return(1);

  /* Check for CD tracks playing */
  if (jtp_sdl_cdrom)
  {
    if (SDL_CDStatus(jtp_sdl_cdrom) == CD_PLAYING)
      return(1);
  }  

  /* No music playing */
  return(0);
}


void jtp_SDLPlayWaveSound
(
  char * wavefilename,
  int samples_per_sec,
  int bits_per_sample,
  int nchannels
)
{
  FILE * f;
  int nbytes, nbytes2, nsamples;
  int cur_pos, i;
  int k;
  int sound_exists;
  char * samples;
  char * samples2;

  if (!jtp_play_effects) return;

  /* Check if the sound exists in the sound cache */
  sound_exists = 0;
  for (i = 0; i < JTP_SDL_MAX_CACHED_SOUNDS; i++)
    if ((jtp_sdl_cached_sounds[i].filename) &&
        (strcmp(wavefilename, jtp_sdl_cached_sounds[i].filename) == 0))
    {
      sound_exists = 1;
      samples = jtp_sdl_cached_sounds[i].samples;
      nbytes = jtp_sdl_cached_sounds[i].length;
      break;
    }

  if (!sound_exists)
  {
    /* Open the file and get the file size */
    jtp_SDLWriteLogMessage(JTP_SDL_LOG_DEBUG, "[jtp_sdl.c/jtp_SDLPlayWaveSound/Debug1]: Opening file\n");
    f = fopen(wavefilename, "rb");
    if (f == NULL) 
    {
      jtp_SDLWriteLogMessage(JTP_SDL_LOG_NOTE, "[jtp_sdl.c/jtp_SDLPlayWaveSound/Note1]: Wave file [");
      jtp_SDLWriteLogMessage(JTP_SDL_LOG_NOTE, wavefilename);
      jtp_SDLWriteLogMessage(JTP_SDL_LOG_NOTE, "] not found.\n");
      return;
    }
    cur_pos = ftell(f);
    fseek(f, 0, SEEK_END);
    nbytes = ftell(f);
    fseek(f, cur_pos, SEEK_SET);

    /* Allocate and read data, then close file */
    jtp_SDLWriteLogMessage(JTP_SDL_LOG_DEBUG, "[jtp_sdl.c/jtp_SDLPlayWaveSound/Debug2]: Reading data\n");
    samples = (char *)malloc(nbytes*sizeof(char));
    fread(samples, sizeof(char), nbytes, f);
    fclose(f);

    /* Convert the sound into 44100Hz, 16-bit mono */
    if ((samples_per_sec == 22050) &&
        (bits_per_sample == 8) &&
        (nchannels == 1))
    {
      nbytes2 = nbytes*4;
      samples2 = (char *)malloc(nbytes2*sizeof(char));
      for (i = 0; i < nbytes; i++)
      {
        /*
         * A 16-bit sample is 2 bytes (most significant byte last),
         * and the value is signed
         */
        k = ((unsigned char *)samples)[i]; k -= 128;
        k *= 128;
        if (k < 0) k += 65536;
        ((unsigned char *)samples2)[i*4] = k % 256;
        ((unsigned char *)samples2)[i*4+1] = k / 256;
        samples2[i*4+2] = samples2[i*4];
        samples2[i*4+3] = samples2[i*4+1];
      }
      free(samples);
      samples = samples2; nbytes = nbytes2;
    }

    if (nbytes > JTP_SDL_SOUND_BUFFER_SIZE)
      nbytes = JTP_SDL_SOUND_BUFFER_SIZE;

    /* Place the sound in the sound cache */
    i = jtp_sdl_oldest_cached_sound;
    free(jtp_sdl_cached_sounds[i].filename);
    free(jtp_sdl_cached_sounds[i].samples);
    jtp_sdl_cached_sounds[i].filename = (char *)malloc(strlen(wavefilename)+1);
    strcpy(jtp_sdl_cached_sounds[i].filename, wavefilename);
    jtp_sdl_cached_sounds[i].samples = samples;
    jtp_sdl_cached_sounds[i].length = nbytes;

    jtp_sdl_oldest_cached_sound++;
    if (jtp_sdl_oldest_cached_sound >= JTP_SDL_MAX_CACHED_SOUNDS)
      jtp_sdl_oldest_cached_sound = 0;
  }

  /* Play sound */
  jtp_SDLWriteLogMessage(JTP_SDL_LOG_DEBUG, "[jtp_dirx.c/jtp_SDLPlayWaveSound/Debug3]: Playing file\n");
  jtp_SDLPlaySound(nbytes, samples);
}
/* End of C interface 
} 
*/

#endif
