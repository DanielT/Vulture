/*      SCCS Id: @(#)jtp_sdl.c  3.0     2000/11/12      */
/* Copyright (c) Jaakko Peltonen, 2000				  */
/* NetHack may be freely redistributed.  See license for details. */

/*-------------------------------------------------------------------
 jtp_sdl.c : SDL API calls for Vulture's windowing system.
 Requires SDL 1.1 or newer.
-------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#ifdef __GNUC__
#include <unistd.h>
#endif
#include <stdarg.h>
#include <errno.h>
#include <time.h>
#include <math.h>
#include <ctype.h>
#ifdef WIN32
#include "win32api.h"
#endif
#include "SDL.h"
#include "SDL_video.h"
#include "SDL_audio.h"
#include "SDL_error.h"
#include "SDL_mixer.h"
#include "vultures_gra.h"
#include "vultures_gen.h"
#include "vultures_txt.h"
#include "vultures_gfl.h"
#include "vultures_mou.h"
#include "vultures_win.h"
#include "vultures_keys.h"
#include "vultures_sdl.h"
#include "vultures_main.h"
#include "date.h"


/* Definitions */

/* The keybuffer is a power-of-two ringbuffer  */
#define JTP_KEYBUF_POW  7
#define JTP_SDL_MAX_BUFFERED_KEYS  (2 << JTP_KEYBUF_POW)
#define JTP_KEYBUF_TRIM(val)  ((val) & (JTP_SDL_MAX_BUFFERED_KEYS - 1))
/* jtp_sdl_num_buffered_keys can't be just jtp_sdl_keybuf_head-jtp_sdl_keybuf_tail bcause jtp_sdl_keybuf_tail 
 * can be greater than jtp_sdl_keybuf_head. therefore we add JTP_SDL_MAX_BUFFERED_KEYS to ensure we have a
 * positive value and then ensure that all values are smaller than JTP_SDL_MAX_BUFFERED_KEYS */
#define jtp_sdl_n_keys_in_buffer  JTP_KEYBUF_TRIM((jtp_sdl_keybuf_head - jtp_sdl_keybuf_tail + JTP_SDL_MAX_BUFFERED_KEYS))

/* 
 * Sound capability: 
 *  4 simultaneous sounds, 
 *  each with 4 seconds of stereo 44100 Hz 16-bit data, little-endian format
 */
#define JTP_SDL_MAX_SOUNDS 4
#define JTP_SDL_MAX_CACHED_SOUNDS 40
#define JTP_SDL_SOUND_BUFFER_FREQUENCY 44100
#define JTP_SDL_SOUND_BUFFER_BITS 16
#define JTP_SDL_SOUND_BUFFER_CHANNELS 1

typedef struct {
    Mix_Chunk *chunk;
    char * filename;
} jtp_sdl_cached_sound;


/* NetHack interface -related objects */
int jtp_sdl_mousex, jtp_sdl_mousey, jtp_sdl_mouseb;

/* Key buffering */
static int jtp_sdl_key_buffer[JTP_SDL_MAX_BUFFERED_KEYS];
static int jtp_sdl_keybuf_head;
static int jtp_sdl_keybuf_tail;

#ifndef WIN32
static int have_fullscreen = 0;
static int fs_message_printed = 0;
#endif

/* Graphics objects */
SDL_Surface *jtp_sdl_screen;             /* Graphics surface */
static SDL_Color *jtp_sdl_colors = NULL; /* Graphics palette */

/* Sound effects objects */
jtp_sdl_cached_sound * jtp_sdl_cached_sounds;
int                  jtp_sdl_oldest_cached_sound = 0;

/* Music objects */
static SDL_CD *jtp_sdl_cdrom = NULL;

static int vultures_convertkey_sdl2nh(int sym, int unicode, int mod, int manualfix)
{
    int shift = mod & KMOD_SHIFT;
    int ctrl = mod & KMOD_CTRL;
    int alt = mod & KMOD_ALT;

    int ch;

    ch=unicode;

    if (ch=='!') return 0; // damn shell thing :/

    if ((ctrl&&ch=='z')||(ch==('z'-('a'-1)))) return 0; // lets just ignore this nasty lil bugger...

    if (ctrl&&ch>='a'&&ch<'z') return ch-('a'-1);

    if (alt&&ch>='a'&&ch<='z') {
        return (0x80 | (ch));
    }

    if (ch>='a'&&ch<='z') {
        if (shift) ch+='A'-'a';
        return ch;
    }

    switch (sym) {
    case SDLK_BACKSPACE: return '\b';
    case SDLK_KP_ENTER:
    case SDLK_RETURN:return '\n';
    case SDLK_ESCAPE: return '\033';
    case SDLK_TAB: return '\t';
    case SDLK_KP8:
    case SDLK_UP: return iflags.num_pad ? '8' : (shift) ? 'K' : 'k';
    case SDLK_KP2:
    case SDLK_DOWN: return iflags.num_pad ? '2' : (shift) ? 'J' : 'j';
    case SDLK_KP4:
    case SDLK_LEFT: return iflags.num_pad ? '4' : (shift) ? 'H' : 'h';
    case SDLK_KP6:
    case SDLK_RIGHT: return iflags.num_pad ? '6' : (shift) ? 'L' : 'l';
    case SDLK_KP7: return iflags.num_pad ? '7' : (shift) ? 'Y' : 'y';
    case SDLK_KP9: return iflags.num_pad ? '9' : (shift) ? 'U' : 'u';
    case SDLK_KP1: return iflags.num_pad ? '1' : (shift) ? 'B' : 'b';
    case SDLK_KP3: return iflags.num_pad ? '3' : (shift) ? 'N' : 'n';

    case SDLK_PAGEUP: return MENU_PREVIOUS_PAGE;
    case SDLK_PAGEDOWN: return MENU_NEXT_PAGE;
    case SDLK_HOME: return MENU_FIRST_PAGE;
    case SDLK_END: return MENU_LAST_PAGE;
    }

    /* this should only kick in if unicode didn't find these */

    if (manualfix&&shift) {
        switch(sym) {
            case '0': return ')';
            case '1': return '!';
            case '2': return '@';
            case '3': return '#';
            case '4': return '$';
            case '5': return '%';
            case '6': return '^';
            case '7': return '&';
            case '8': return '*';
            case '9': return '(';
            case '-': return '_';
            case '=': return '+';
            case '[': return '{';
            case ']': return '}';
            case '\\': return '|';
            case ';': return ':';
            case '\'': return '"';
            case ',': return '<';
            case '.': return '>';
            case '/': return '?';
        }
    }


    if ((ch>0)&&(ch<0x7e)) return ch;

    return 0;

}

static void jtp_SDLProcessEvent(SDL_Event *cur_event)
{
  /* Process the message */
	switch (cur_event->type)
	{
	case SDL_KEYDOWN:
		switch (cur_event->key.keysym.sym)
		{
		case SDLK_LSHIFT:				/* Case shift is not a separate key */
		case SDLK_RSHIFT:				/* Case shift is not a separate key */
		case SDLK_LCTRL:				/* Control key is not a separate key */
		case SDLK_RCTRL:				/* Control key is not a separate key */
		case SDLK_LALT:					/* ALT/Meta key is not a separate key */
		case SDLK_RALT:					/* ALT/Meta key is not a separate key */
		case SDLK_LMETA:				/* ALT/Meta key is not a separate key */
		case SDLK_RMETA:				/* ALT/Meta key is not a separate key */
		case SDLK_MODE:
		case SDLK_LSUPER:
		case SDLK_RSUPER:
		case SDLK_COMPOSE:
		case SDLK_NUMLOCK:
		case SDLK_CAPSLOCK:
		case SDLK_SCROLLOCK:
			break;
		default:
#ifndef WIN32
            /* TODO Update this ifndef as SDL library gets updated with more
             * OS support for fullscreen toggling */
			
			if (cur_event->key.keysym.sym == SDLK_TAB && (cur_event->key.keysym.mod & KMOD_ALT) && have_fullscreen)
			{
				/* go out of fullscreen for alt + tab */
				have_fullscreen = !(SDL_WM_ToggleFullScreen(jtp_sdl_screen)); /* disable fullscreen */
				if (!have_fullscreen && !fs_message_printed++)
					/* This gets displayed only once */
					pline("You have left fullscreen mode. Press ALT+RETURN to reenter it.");
			}
			else if(cur_event->key.keysym.sym == SDLK_RETURN && (cur_event->key.keysym.mod & KMOD_ALT))
			{
				/* toggle fullscreen with ctrl + enter */
				have_fullscreen = (SDL_WM_ToggleFullScreen(jtp_sdl_screen)) ? !have_fullscreen : have_fullscreen;
			}
            else
#endif
            if(cur_event->key.keysym.sym == SDLK_SYSREQ)
            {
                jtp_save_screenshot();
            }
            else if (jtp_sdl_n_keys_in_buffer < JTP_SDL_MAX_BUFFERED_KEYS)
                jtp_sdl_keybuf_add(
                    vultures_convertkey_sdl2nh(
                        cur_event->key.keysym.sym,
                        cur_event->key.keysym.unicode,
                        cur_event->key.keysym.mod,
                        FALSE)
                    );
			break;
		}
		break;

	case SDL_MOUSEBUTTONDOWN:
		switch (cur_event->button.button)
		{
		case SDL_BUTTON_LEFT:
			jtp_sdl_mouseb |= JTP_MBUTTON_LEFT;
			jtp_sdl_mousex = cur_event->button.x;
			jtp_sdl_mousey = cur_event->button.y;
			break;
		case SDL_BUTTON_RIGHT:
			jtp_sdl_mouseb |= JTP_MBUTTON_RIGHT;
			jtp_sdl_mousex = cur_event->button.x;
			jtp_sdl_mousey = cur_event->button.y;
			break;
		case SDL_BUTTON_WHEELUP:
			if (cur_event->button.state == SDL_PRESSED)
				if (jtp_sdl_n_keys_in_buffer < JTP_SDL_MAX_BUFFERED_KEYS)
					jtp_sdl_keybuf_add(JTP_MOUSEWHEEL_UP);
			break;
		case SDL_BUTTON_WHEELDOWN:
			if (cur_event->button.state == SDL_PRESSED)
				if (jtp_sdl_n_keys_in_buffer < JTP_SDL_MAX_BUFFERED_KEYS)
					jtp_sdl_keybuf_add(JTP_MOUSEWHEEL_DOWN);
			break;
		default:
			break;
		}
		break;

	case SDL_MOUSEBUTTONUP:
		switch ((cur_event->button).button)
		{
		case SDL_BUTTON_LEFT:
			jtp_sdl_mouseb &= ~JTP_MBUTTON_LEFT;
			jtp_sdl_mousex = cur_event->button.x;
			jtp_sdl_mousey = cur_event->button.y;
			break;
		case SDL_BUTTON_RIGHT:
			jtp_sdl_mouseb &= ~JTP_MBUTTON_RIGHT;
			jtp_sdl_mousex = cur_event->button.x;
			jtp_sdl_mousey = cur_event->button.y;
			break;
		default:
			break;
		}
		break;

	case SDL_MOUSEMOTION:
		jtp_sdl_mousex = cur_event->motion.x;
		jtp_sdl_mousey = cur_event->motion.y;
		jtp_sdl_mouseb = JTP_MBUTTON_NONE;
		if (cur_event->motion.state & SDL_BUTTON(1))
			jtp_sdl_mouseb |= JTP_MBUTTON_LEFT;
		if (cur_event->motion.state & SDL_BUTTON(3))
			jtp_sdl_mouseb |= JTP_MBUTTON_RIGHT;
		break;
	
	case SDL_QUIT:
		{
			/* exit gracefully */
			if (program_state.gameover)
			{
				/* assume the user really meant this, as the game is already over... */
				/* to make sure we still save bones, just set stop printing flag */
				program_state.stopprint++;
				jtp_sdl_keybuf_add('\033'); /* and send keyboard input as if user pressed ESC */
			}
			else if (!program_state.something_worth_saving)
			{
				/* User exited before the game started, e.g. during splash display */
				/* Just get out. */
				bail((char *)0);
			}
			else
			{
				switch (jtp_query_choices("Save?",  "yes\0no\0\033cancel", 3))
				{
				case 0:
					jtp_sdl_keybuf_reset();
					jtp_sdl_keybuf_add('y');
					dosave();
					break;
				case 1:
					jtp_sdl_keybuf_reset();
#if 0
					jtp_sdl_key_buffer[jtp_sdl_n_keys_in_buffer++] = 'q';
					done(QUIT);
#else
					jtp_sdl_keybuf_add(jtp_getkey(JTP_NHCMD_QUIT_GAME));
					/*
					 * this will ask again for "Really quit?"/"Really save?", but the user
					 * might not expect that the answer 'n' to the question above
					 * means "quit", without saving.
					 */
#endif
					break;
				default:
					break;
				}
			}
		}
		break;
	case SDL_ACTIVEEVENT:
	case SDL_VIDEOEXPOSE:
		jtp_refresh(&jtp_screen);
		break;
	}
}


void jtp_sdl_keybuf_add(int key)
{
  if (jtp_sdl_n_keys_in_buffer == (JTP_SDL_MAX_BUFFERED_KEYS - 1))
    jtp_sdl_keybuf_tail = JTP_KEYBUF_TRIM(jtp_sdl_keybuf_tail + 1);
  
  jtp_sdl_key_buffer[jtp_sdl_keybuf_head] = key;
  jtp_sdl_keybuf_head = JTP_KEYBUF_TRIM(jtp_sdl_keybuf_head + 1);
}


void jtp_sdl_keybuf_reset()
{
  jtp_sdl_keybuf_head = jtp_sdl_keybuf_tail;
}

void jtp_SDLSetInstance(void)
{
  /* Set initial values for all the objects (variables) used */

  /* Linux API -related objects */ 
  
  /* NetHack interface -related objects */
  jtp_sdl_mousex = 100; jtp_sdl_mousey = 100; jtp_sdl_mouseb = 0;

  /* Key buffering */
  jtp_sdl_keybuf_head = 0;
  jtp_sdl_keybuf_tail = 0;

  /* Graphics objects */
  jtp_sdl_colors = NULL;

  /* Sound effects objects */
  /* jtp_sdl_oldest_sound = 0; */
  jtp_sdl_cached_sounds = NULL;
  jtp_sdl_oldest_cached_sound = 0;

  /* Music objects */
}


void jtp_get_event(void)
{
	SDL_Event event;
	int old_mouseb;
	int old_mousex, old_mousey;
	
	old_mouseb = jtp_sdl_mouseb;
	old_mousex = jtp_sdl_mousex;
	old_mousey = jtp_sdl_mousey;
	jtp_SDLProcessEvents();
	while (jtp_sdl_n_keys_in_buffer == 0 &&
		old_mouseb == jtp_sdl_mouseb &&
		old_mousex == jtp_sdl_mousex &&
		old_mousey == jtp_sdl_mousey)
	{
		if (SDL_WaitEvent(&event) == 0)
			return;
		jtp_SDLProcessEvent(&event);
	}
}
	

int jtp_getch(void)
{
  int current_key;
  
  /* Poll for a key until there's one in the buffer */
  jtp_get_event();
  if (jtp_sdl_n_keys_in_buffer == 0)
    return 0;
  
  /* Remove key from buffer */
  current_key = jtp_sdl_key_buffer[jtp_sdl_keybuf_tail];
  jtp_sdl_keybuf_tail = JTP_KEYBUF_TRIM(jtp_sdl_keybuf_tail+1);
      
  return(current_key);
}

int jtp_kbhit(void)
{
  /* Just process any waiting events, then return */
  jtp_SDLProcessEvents();

  if (jtp_sdl_n_keys_in_buffer > 0) return(1);
  return(0);
}

void jtp_SDLProcessEvents(void)
{
  SDL_Event event;

  while (SDL_PollEvent(&event))
  {
    jtp_SDLProcessEvent(&event);
  }
}

static void jtp_sdl_error(const char *file, int line, const char *what)
{
	jtp_write_log_message(JTP_LOG_ERROR, file, line, "%s: %s\n", what, SDL_GetError());
}


void jtp_enter_graphics_mode(jtp_screen_t *newscreen)
{
  int i;

#ifdef __APPLE__
  NSApplicationLoad();
#endif

  if (jtp_play_effects)
  {
    if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO|SDL_INIT_CDROM) == -1)
    {
      jtp_sdl_error(__FILE__, __LINE__, "Could not initialize SDL with video and audio");
      exit(1);
    }
  }
  else
  {
    if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_CDROM) == -1)
    {
      jtp_sdl_error(__FILE__, __LINE__, "Could not initialize SDL with video");
      exit(1);
    }
  }

  /* Initialize the event handlers */
  atexit(SDL_Quit);
  /* Filter key, mouse and quit events */
  /*  SDL_SetEventFilter(FilterEvents); */
  /* Enable key repeat */
  SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);  
  SDL_WM_SetCaption(VERSION_ID,NULL);

  if (iflags.wc2_fullscreen)
  {
    jtp_sdl_screen = SDL_SetVideoMode(newscreen->width, newscreen->height, 8, SDL_SWSURFACE | SDL_FULLSCREEN);
#ifndef WIN32
    have_fullscreen = 1;
#endif
  }
  else
    jtp_sdl_screen = SDL_SetVideoMode(newscreen->width, newscreen->height, 8, SDL_SWSURFACE);
  if (!jtp_sdl_screen)
  {
    jtp_sdl_error(__FILE__, __LINE__, "Could not initialize video mode");
    exit(1);
  }

  /* Don't show double cursor */
  SDL_ShowCursor(SDL_DISABLE);

  jtp_sdl_colors = malloc(256*sizeof(SDL_Color));

  /* Enable Unicode translation. Necessary to match keypresses to characters */
  SDL_EnableUNICODE(1);


  if (jtp_play_effects || jtp_play_music)
  {
    if (Mix_OpenAudio(44100,AUDIO_S16SYS,2,1024) < 0)
    {
      jtp_sdl_error(__FILE__, __LINE__, "Could not initialize SDL audio device\n");
      exit(1);
    }
    if (jtp_play_effects) Mix_AllocateChannels(4);

    /* Create the sound cache */
    jtp_sdl_cached_sounds = malloc(JTP_SDL_MAX_CACHED_SOUNDS*sizeof(jtp_sdl_cached_sound));
    for (i = 0; i < JTP_SDL_MAX_CACHED_SOUNDS; i++)
    {
      jtp_sdl_cached_sounds[i].chunk = NULL;
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

void jtp_exit_graphics_mode(void)
{
  jtp_stop_music();
  if (jtp_sdl_cdrom) SDL_CDClose(jtp_sdl_cdrom);
  jtp_sdl_cdrom = NULL;
  SDL_Quit();
}

void jtp_SDLRecordColor(int cindex, int r, int g, int b)
{
  jtp_sdl_colors[cindex].r = (r*255)/63;
  jtp_sdl_colors[cindex].g = (g*255)/63;
  jtp_sdl_colors[cindex].b = (b*255)/63;
}

void jtp_SDLSetPalette(void)
{
  SDL_SetColors(jtp_sdl_screen, jtp_sdl_colors, 0, 256);
}


void jtp_refresh_region
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
  
  if (SDL_MUSTLOCK(jtp_sdl_screen))
    SDL_LockSurface(jtp_sdl_screen);

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

void jtp_refresh(jtp_screen_t * newscreen)
{
  jtp_refresh_region(0, 0, newscreen->width-1, newscreen->height-1, newscreen);
}


void jtp_msleep(unsigned long milliseconds)
{
    SDL_Delay(milliseconds);
}


double jtp_clocktick(void)
{
	return SDL_GetTicks() / 1000.0;
}


Mix_Music *jtpCurrentMusic=NULL;
void jtp_play_song(char * midifilename)
{
    if (!jtp_play_music) return;
    if (jtpCurrentMusic) Mix_FreeMusic(jtpCurrentMusic);
    jtpCurrentMusic=Mix_LoadMUS(midifilename);
    Mix_PlayMusic(jtpCurrentMusic,0);
}

void jtp_play_cd_track(char * cdtrackname)
{
  int nTrack;

  if (!jtp_play_music) return;

  /* Parse the track number from the given string */
  nTrack = atoi(cdtrackname);
  if (nTrack < 0)
  { 
    jtp_write_log_message(JTP_LOG_NOTE, __FILE__, __LINE__, "Invalid track number [%s]\n", cdtrackname);
    return;
  }
 
  if (!jtp_sdl_cdrom)
  {
    return;
  }

  if (!CD_INDRIVE(SDL_CDStatus(jtp_sdl_cdrom)))
  {
    jtp_write_log_message(JTP_LOG_DEBUG, __FILE__, __LINE__, "No CD in drive\n");
    return;
  }

  SDL_CDPlayTracks(jtp_sdl_cdrom, nTrack, 0, 1, 0);
}


void jtp_stop_music(void)
{
  if (jtpCurrentMusic) Mix_FreeMusic(jtpCurrentMusic);
  jtpCurrentMusic = NULL;
  /* Stop any CD tracks playing */
  if (jtp_sdl_cdrom)
  {
    if (SDL_CDStatus(jtp_sdl_cdrom) == CD_PLAYING)
      SDL_CDStop(jtp_sdl_cdrom);
  }
}


int jtp_is_music_playing(void)
{
    
  /* Check for external music files (MIDI or MP3) playing */
  if (Mix_PlayingMusic() > 0) 
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


void jtp_play_sound(char * wavefilename)
{
  int i;
  int sound_exists;
  Mix_Chunk *chunk = NULL;

  if (!jtp_play_effects) return;

  /* Check if the sound exists in the sound cache */
  sound_exists = 0;
  for (i = 0; i < JTP_SDL_MAX_CACHED_SOUNDS; i++)
    if ((jtp_sdl_cached_sounds[i].filename) &&
        (strcmp(wavefilename, jtp_sdl_cached_sounds[i].filename) == 0))
    {
      sound_exists = 1;
      chunk = jtp_sdl_cached_sounds[i].chunk;
      break;
    }

  if (!sound_exists)
  {
    i = jtp_sdl_oldest_cached_sound;
    if (jtp_sdl_cached_sounds[i].filename) free(jtp_sdl_cached_sounds[i].filename);
    if (jtp_sdl_cached_sounds[i].chunk) Mix_FreeChunk(jtp_sdl_cached_sounds[i].chunk);
    jtp_sdl_cached_sounds[i].filename = malloc(strlen(wavefilename)+1);
    strcpy(jtp_sdl_cached_sounds[i].filename, wavefilename);
    jtp_sdl_cached_sounds[i].chunk = Mix_LoadWAV(wavefilename);
    chunk = jtp_sdl_cached_sounds[i].chunk;

    jtp_sdl_oldest_cached_sound++;
    if (jtp_sdl_oldest_cached_sound >= JTP_SDL_MAX_CACHED_SOUNDS)
      jtp_sdl_oldest_cached_sound = 0;
  }

  /* Play sound */
  jtp_write_log_message(JTP_LOG_DEBUG, __FILE__, __LINE__, "Playing file %s\n", wavefilename);
  Mix_PlayChannel(-1,chunk,0);
}

#ifdef PCMUSIC

/*
 * instrument playing functions
 */
#define put_le_short(p, v) \
	(p)[0] = (unsigned char)((v)     ), \
	(p)[1] = (unsigned char)((v) >> 8)
#define put_le_long(p, v) \
	(p)[0] = (unsigned char)((v)      ), \
	(p)[1] = (unsigned char)((v) >>  8), \
	(p)[2] = (unsigned char)((v) >> 16), \
	(p)[3] = (unsigned char)((v) >> 24)
#define put_be_short(p, v) \
	(p)[1] = (unsigned char)((v)     ), \
	(p)[0] = (unsigned char)((v) >> 8)

#define SINE_WAVE 0
#define WRITE_WAV 0


#if WRITE_WAV
#define WAVOUT "sample.wav"

static unsigned char wav_header[] = {
	'R', 'I', 'F', 'F',
	0x99, 0x99, 0x99, 0x99, /* file size */
	'W', 'A', 'V', 'E',
	'f', 'm', 't', ' ',
	16, 0, 0, 0,
	0x01, 0x00, /* format-tag (PCM) */
	0x01, 0x00, /* channels */
	0x99, 0x99, 0x99, 0x99, /* samples_per_sec */
	0x99, 0x99, 0x99, 0x99, /* avg. bytes_per_sec */
	0x02, 0x00,             /* block align */
	0x10, 0x00,             /* bits_per_sample */
	'd', 'a', 't', 'a',
	0x99, 0x99, 0x99, 0x99  /* datasize */
};
static unsigned long total_datasize;
static FILE *wav_fp;
#endif /* WRITE_WAV */


static void play_note(unsigned int freq, unsigned int duration, unsigned int pause)
{
	int mixfreq;
	Uint16 format;
	int channels;
	int bits_per_sample;
	int bytes_per_sample;
	int datasize;
	int nsamples, nsamples1, nsamples2;
	Uint8 *buf, *buf2;
	int is_unsigned;
	int is_le;
	int intervall;
	Mix_Chunk *chunk;
	int channel;
	unsigned int v;
	int i;
	
	if (freq == 0)
		freq = 523;
#ifdef WIN32
    if (iflags.usepcspeaker)
    {
		if (duration)
			Beep(freq, duration);
		Sleep(pause);
	    return;
	}
#endif
	
	Mix_QuerySpec(&mixfreq, &format, &channels);
	is_unsigned = (format & 0x8000) == 0;
	is_le = (format & 0x1000) == 0;
	bits_per_sample = format & 0xff;
#if 0
	printf("mixer spec: %d AUDIO_%s%d%s %d\n", mixfreq,
		is_unsigned ? "U" : "S",
		bits_per_sample,
		bits_per_sample > 8 ? (is_le ? "LSB" : "MSB") : "",
		channels);
#endif
	if (channels < 1 || channels > 2)
		return;
	if (bits_per_sample != 8 && bits_per_sample != 16)
		return;
	bytes_per_sample = bits_per_sample == 8 ? 1 : 2;
	nsamples = (mixfreq * (duration + pause)) / 1000;
	if (nsamples == 0)
		return;
	
	datasize = bytes_per_sample * channels * nsamples;
	buf = malloc(datasize);
	if (buf == NULL)
		return;
	nsamples1 = (mixfreq * duration) / 1000;
	nsamples2 = nsamples - nsamples1;
	
	intervall = (mixfreq / freq);
	
	if (channels == 1)
	{
		if (bytes_per_sample == 1)
		{
			if (is_unsigned)
			{
				for (i = 0; i < nsamples1; i++)
				{
#if SINE_WAVE
					v = (unsigned int)((sin(i * (2 * M_PI) / intervall) + 1.0) * 127);
#else
					v = (i % intervall) < (intervall / 2) ? 0xff : 0x00;
#endif
					buf[i] = v;
				}
			} else
			{
				for (i = 0; i < nsamples1; i++)
				{
#if SINE_WAVE
					v = (unsigned int)(sin(i * (2 * M_PI) / intervall) * 127);
#else
					v = (i % intervall) < (intervall / 2) ? 0x7f : 0x80;
#endif
					buf[i] = v;
				}
			}
		} else
		{
			if (is_le)
			{
				if (is_unsigned)
				{
					for (i = 0; i < nsamples1; i++)
					{
#if SINE_WAVE
						v = (unsigned int)((sin(i * (2 * M_PI) / intervall) + 1.0) * 32765);
#else
						v = (i % intervall) < (intervall / 2) ? 0xffff : 0x0000;
#endif
						put_le_short(buf + 2 * i, v);
					}
				} else
				{
					for (i = 0; i < nsamples1; i++)
					{
#if SINE_WAVE
						v = (unsigned int)(sin(i * (2 * M_PI) / intervall) * 32765);
#else
						v = (i % intervall) < (intervall / 2) ? 0x7fff : 0x8000;
#endif
						put_le_short(buf + 2 * i, v);
					}
				}
			} else
			{
				if (is_unsigned)
				{
					for (i = 0; i < nsamples1; i++)
					{
#if SINE_WAVE
						v = (unsigned int)((sin(i * (2 * M_PI) / intervall) + 1.0) * 65530);
#else
						v = (i % intervall) < (intervall / 2) ? 0xffff : 0x0000;
#endif
						put_be_short(buf + 2 * i, v);
					}
				} else
				{
					for (i = 0; i < nsamples1; i++)
					{
#if SINE_WAVE
						v = (unsigned int)(sin(i * (2 * M_PI) / intervall) * 32765);
#else
						v = (i % intervall) < (intervall / 2) ? 0x7fff : 0x8000;
#endif
						put_be_short(buf + 2 * i, v);
					}
				}
			}
		}
	} else
	{
		if (bytes_per_sample == 1)
		{
			if (is_unsigned)
			{
				for (i = 0; i < nsamples1; i++)
				{
#if SINE_WAVE
					v = (unsigned int)((sin(i * (2 * M_PI) / intervall) + 1.0) * 127);
#else
					v = (i % intervall) < (intervall / 2) ? 0xff : 0x00;
#endif
					buf[i * 2 + 0] = v;
					buf[i * 2 + 1] = v;
				}
			} else
			{
				for (i = 0; i < nsamples1; i++)
				{
#if SINE_WAVE
					v = (unsigned int)(sin(i * (2 * M_PI) / intervall) * 127);
#else
					v = (i % intervall) < (intervall / 2) ? 0x7f : 0x80;
#endif
					buf[i * 2 + 0] = v;
					buf[i * 2 + 1] = v;
				}
			}
		} else
		{
			if (is_le)
			{
				if (is_unsigned)
				{
					for (i = 0; i < nsamples1; i++)
					{
#if SINE_WAVE
						v = (unsigned int)((sin(i * (2 * M_PI) / intervall) + 1.0) * 32765);
#else
						v = (i % intervall) < (intervall / 2) ? 0xffff : 0x0000;
#endif
						put_le_short(buf + 4 * i + 0, v);
						put_le_short(buf + 4 * i + 2, v);
					}
				} else
				{
					for (i = 0; i < nsamples1; i++)
					{
#if SINE_WAVE
						v = (unsigned int)(sin(i * (2 * M_PI) / intervall) * 32765);
#else
						v = (i % intervall) < (intervall / 2) ? 0x7fff : 0x8000;
#endif
						put_le_short(buf + 4 * i + 0, v);
						put_le_short(buf + 4 * i + 2, v);
					}
				}
			} else
			{
				if (is_unsigned)
				{
					for (i = 0; i < nsamples1; i++)
					{
#if SINE_WAVE
						v = (unsigned int)((sin(i * (2 * M_PI) / intervall) + 1.0) * 65530);
#else
						v = (i % intervall) < (intervall / 2) ? 0xffff : 0x0000;
#endif
						put_be_short(buf + 4 * i + 0, v);
						put_be_short(buf + 4 * i + 2, v);
					}
				} else
				{
					for (i = 0; i < nsamples1; i++)
					{
#if SINE_WAVE
						v = (unsigned int)(sin(i * (2 * M_PI) / intervall) * 32765);
#else
						v = (i % intervall) < (intervall / 2) ? 0x7fff : 0x8000;
#endif
						put_be_short(buf + 4 * i + 0, v);
						put_be_short(buf + 4 * i + 2, v);
					}
				}
			}
		}
	}

	buf2 = buf + bytes_per_sample * channels * nsamples1;
	if (channels == 1)
	{
		if (bytes_per_sample == 1)
		{
			if (is_unsigned)
				v = 0x80;
			else
				v = 0x00;
			for (i = 0; i < nsamples2; i++)
			{
				buf2[i] = v;
			}
		} else
		{
			if (is_unsigned)
				v = 0x8000;
			else
				v = 0x0000;
			if (is_le)
			{
				for (i = 0; i < nsamples2; i++)
				{
					put_le_short(buf2 + 2 * i, v);
				}
			} else
			{
				for (i = 0; i < nsamples2; i++)
				{
					put_be_short(buf2 + 2 * i, v);
				}
			}
		}
	} else
	{
		if (bytes_per_sample == 1)
		{
			if (is_unsigned)
				v = 0x80;
			else
				v = 0x00;
			for (i = 0; i < nsamples2; i++)
			{
				buf2[i * 2 + 0] = v;
				buf2[i * 2 + 1] = v;
			}
		} else
		{
			if (is_unsigned)
				v = 0x8000;
			else
				v = 0x0000;
			if (is_le)
			{
				for (i = 0; i < nsamples2; i++)
				{
					put_le_short(buf2 + 4 * i + 0, v);
					put_le_short(buf2 + 4 * i + 2, v);
				}
			} else
			{
				for (i = 0; i < nsamples2; i++)
				{
					put_be_short(buf2 + 4 * i + 0, v);
					put_be_short(buf2 + 4 * i + 2, v);
				}
			}
		}
	}

#if WRITE_WAV
	if (wav_fp != NULL)
	{
		put_le_short(wav_header + 22, channels);
		put_le_long(wav_header + 24, mixfreq);
		put_le_long(wav_header + 28, bytes_per_sample * channels * mixfreq);
		put_le_short(wav_header + 32, bytes_per_sample * channels);
		put_le_short(wav_header + 34, bits_per_sample);
		fwrite(buf, 1, datasize, wav_fp);
		total_datasize += datasize;
	}
#endif
		
	chunk = Mix_QuickLoad_RAW(buf, datasize);
	if (chunk == NULL)
	{
		jtp_write_log_message(JTP_LOG_NOTE, __FILE__, __LINE__, "error creating mix chunk: %s\n", Mix_GetError());
	} else
	{
		channel = Mix_PlayChannel(-1, chunk, 0);
		if (channel < 0)
		{
			jtp_write_log_message(JTP_LOG_NOTE, __FILE__, __LINE__, "error playing a note: %s\n", Mix_GetError());
		} else
		{
			while (Mix_Playing(channel))
				jtp_msleep(10);
		}
		Mix_FreeChunk(chunk);
	}
	free(buf);
}


/* The important numbers here are 287700UL for the factors and 4050816000UL
 * which gives the 440 Hz for the A below middle C. "middle C" is assumed to
 * be the C at octave 3. The rest are computed by multiplication/division of
 * 2^(1/12) which came out to 1.05946 on my calculator.  It is assumed that
 * no one will request an octave beyond 6 or below 0.  (At octave 7, some
 * notes still come out ok, but by the end of the octave, the "notes" that
 * are produced are just ticks.

 * These numbers were chosen by a process based on the C64 tables (which
 * weren't standardized) and then were 'standardized' by giving them the
 * closest value.  That's why they don't seem to be based on any sensible
 * number.
 */

static unsigned long const notefactors[12] =
{483852, 456695, 431063, 406869, 384033,
 362479, 342135, 322932, 304808, 287700, 271553, 256312};


static unsigned int note(int notenum)
{
	int n;
	int o;
	unsigned int freq;
	
	n = notenum % 12;
	o = notenum / 12;
	freq = (unsigned) ((4050816000UL / notefactors[n]) >> (7 - o));
#if 0
	printf("octave = %d, note = %2d, freq = %4u", o, n, freq);
#endif
	return freq;
}

static int const notes[7] = { 9, 11, 0, 2, 4, 5, 7 };


static const char *startnote(const char *c, unsigned int octave, unsigned int *freq)
{
	int n;

	n = toupper(*c);
	c++;
	if (n == 'H')
		n = 'B';
	n = notes[n - 'A'] + octave * 12;
	if (*c == '#' || *c == '+')
	{
		n++;
		c++;
	} else if (*c == '-')
	{
		if (n)
			n--;
		c++;
	}
	*freq = note(n);

	return c;
}


static unsigned int delaytime(unsigned time, int mtype)
{
	/* time and twait are in units of milliseconds */

	unsigned twait;

	switch (toupper(mtype))
	{
	case 'S':
		twait = time / 4;
		break;
	case 'L':
		twait = 0;
		break;
	default:
		twait = time / 8;
		break;
	}
#if 0
	printf(", time = %u, pause = %u\n", time, twait);
#endif
	return twait;
}


static const char *delaynote(const char *c, int mtype, unsigned int length, unsigned int tempo, unsigned int *duration, unsigned int *tpause)
{
	unsigned time = 0;

	while (isdigit(*c))
		time = time * 10 + (*c++ - '0');

	if (!time)
		time = length;

	time = (unsigned) (240000 / time / tempo);

	while (*c == '.')
	{
		time = time * 3 / 2;
		c++;
	}

	*duration = time;
	*tpause = delaytime(time, mtype);

	return c;
}


static void playtune(const char *tune)
{
	const char *c;
	const char *n;
	unsigned num;
	unsigned int freq;
	unsigned int tpause;
	unsigned int duration;

	int mtype = 'N';
	unsigned int tempo = 120;
	unsigned int length = 4;
	unsigned int octave = 3;
	
#if WRITE_WAV
	wav_fp = fopen(WAVOUT, "wb");
	if (wav_fp == NULL)
	{
		pline("cannot create %s\n", WAVOUT);
	} else
	{
		total_datasize = 0;
		fwrite(wav_header, 1, sizeof(wav_header), wav_fp);
	}
#endif
		
	for (c = tune; *c;)
	{
		num = 0;
		for (n = c + 1; isdigit(*n); n++)
			num = num * 10 + (*n - '0');
		if (isspace(*c))
		{
			c++;
		} else
		{
			switch (toupper(*c))
			{
			case 'A':
			case 'B':
			case 'C':
			case 'D':
			case 'E':
			case 'F':
			case 'G':
			case 'H':
				c = startnote(c, octave, &freq);
				c = delaynote(c, mtype, length, tempo, &duration, &tpause);
				play_note(freq, duration - tpause, tpause);
				break;
			case 'P':
				c = delaynote(++c, mtype, length, tempo, &duration, &tpause);
				play_note(0, 0, duration);
				break;
			case 'M':
				c++;
				mtype = *c;
				c++;
				break;
			case 'T':
				if (num)
					tempo = num;
				else
					pline("Zero Tempo (%s)!\n", c);
				c = n;
				break;
			case 'L':
				if (num)
					length = num;
				else
					pline("Zero Length (%s)!\n", c);
				c = n;
				break;
			case 'O':
				if (num <= 7)
					octave = num;
				c = n;
				break;
			case 'N':
				freq = note(num);
				duration = 240000 / length / tempo;
				tpause = delaytime(duration, mtype);
				play_note(freq, duration - tpause, tpause);
				c = n;
				break;
			case '>':
				if (octave < 7)
					octave++;
				c++;
				break;
			case '<':
				if (octave)
					octave--;
				c++;
				break;
			case ' ':
				c++;
				break;
			default:
				pline("Unrecognized play value (%s)!\n", c);
				c = "";
				break;
			}
		}
	}

#if WRITE_WAV
	if (wav_fp != NULL)
	{
		put_le_long(wav_header + 4, total_datasize + 0x24);
		put_le_long(wav_header + 40, total_datasize);
		fseek(wav_fp, 0l, SEEK_SET);
		fwrite(wav_header, 1, sizeof(wav_header), wav_fp);
		fclose(wav_fp);
		wav_fp = NULL;
	}
#endif /* WRITE_WAV */
}

void FDECL( pc_speaker, ( struct obj *, char * ) );

void pc_speaker(struct obj *instr, char *tune)
{
	char *buf;
	const char *prepend;

	if (!jtp_play_effects)
		return;
	prepend = NULL;
    switch (instr->otyp)
    {
	case WOODEN_FLUTE:
	case MAGIC_FLUTE:
		prepend = ">>"; /* up two octaves */
	    break;
	case TOOLED_HORN:
	case FROST_HORN:
	case FIRE_HORN:
		prepend = "<"; /* drop one octave */
	    break;
	case BUGLE:
	    break;
	case WOODEN_HARP:
	case MAGIC_HARP:
		prepend = "MLT240"; /* fast, legato */
	    break;
    }
	if (prepend != NULL)
	{
		buf = malloc(strlen(prepend) + strlen(tune) + 1);
		strcpy(buf, prepend);
		strcat(buf, tune);
		playtune(buf);
		free(buf);
	} else
	{
		playtune(tune);
	}
}

#endif /* PCMUSIC */
