/* NetHack may be freely redistributed.  See license for details. */

#ifndef _vulture_sound_h_
#define _vulture_sound_h_

#include <SDL.h>
#include <SDL_audio.h>
#include <SDL_mixer.h>

#include <string>
#include <vector>

#define V_MAX_CACHED_SOUNDS 40


/* Event sound types */
enum vulture_event_sound_enum {
	V_EVENT_SOUND_TYPE_NONE = 0,
	V_EVENT_SOUND_TYPE_SND,
	V_EVENT_SOUND_TYPE_MUS,
	V_EVENT_SOUND_TYPE_RANDOM_SONG,
	V_EVENT_SOUND_TYPE_CD_AUDIO,
};


typedef struct {
	char * searchpattern;
	int soundtype;
  std::string filename;
} vulture_event_sound;


typedef struct {
	Mix_Chunk *chunk;
  std::string filename;
} vulture_cached_sound;


extern void vulture_init_sound(void);
extern void vulture_play_ambient_sound(int force_play);
extern void vulture_play_event_sound(const char *);
extern void vulture_stop_music(void);


extern vulture_cached_sound * vulture_cached_sounds;
extern SDL_CD *vulture_cdrom;
extern std::vector<vulture_event_sound> vulture_event_sounds;
extern int vulture_n_background_songs;


#endif
