/* NetHack may be freely redistributed.  See license for details. */
/* Copyright (c) Jaakko Peltonen, 2000				  */
/* Copyright (c) Daniel Thaler, 2006				  */

#ifndef _vultures_sound_h_
#define _vultures_sound_h_

#include "SDL_audio.h"
#include "SDL_mixer.h"


#define V_MAX_CACHED_SOUNDS 40


/* Event sound types */
enum vultures_event_sound_enum {
	V_EVENT_SOUND_TYPE_NONE = 0,
	V_EVENT_SOUND_TYPE_SND,
	V_EVENT_SOUND_TYPE_MUS,
	V_EVENT_SOUND_TYPE_RANDOM_SONG,
	V_EVENT_SOUND_TYPE_CD_AUDIO,
};


typedef struct {
	char * searchpattern;
	int soundtype;
	char * filename;
} vultures_event_sound;


typedef struct {
	Mix_Chunk *chunk;
	char * filename;
} vultures_cached_sound;


extern void vultures_init_sound(void);
extern void vultures_play_ambient_sound(int force_play);
extern void vultures_play_event_sound(const char *);
extern void vultures_stop_music(void);


extern vultures_cached_sound * vultures_cached_sounds;
extern SDL_CD *vultures_cdrom;
extern vultures_event_sound ** vultures_event_sounds;
extern int vultures_n_event_sounds;
extern int vultures_n_background_songs;


#endif
