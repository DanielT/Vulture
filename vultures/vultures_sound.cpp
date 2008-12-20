/* Copyright (c) Jaakko Peltonen, 2000				  */
/* NetHack may be freely redistributed.  See license for details. */

#include <stdlib.h>
#include <string.h>

#include "SDL.h"
#include "SDL_audio.h"
#include "SDL_mixer.h"


#include "vultures_win.h"
#include "vultures_gen.h"
#include "vultures_sound.h"
#include "vultures_opt.h"

/*******************************************************************/


vector<vultures_event_sound> vultures_event_sounds;
int vultures_n_background_songs;
int vultures_sound_inited = 0;


/* Sound effects objects */
vultures_cached_sound *vultures_cached_sounds;
int vultures_oldest_cached_sound = 0;

/* Music objects */
SDL_CD *vultures_cdrom = NULL;


Mix_Music *vultures_current_music=NULL;

/*******************************************************************/

static void vultures_play_song(string midifilename);
static void vultures_play_cd_track(string cdtrackname);
static void vultures_play_sound(string wavefilename);
static int vultures_is_music_playing(void);

/*******************************************************************/


void vultures_init_sound(void)
{
	int i;

	if (vultures_sound_inited)
		return;

	if (SDL_InitSubSystem(SDL_INIT_AUDIO | SDL_INIT_CDROM) == -1) {
		/* init failed */
		vultures_opts.play_effects = 0;
		vultures_opts.play_music = 0;
		vultures_sound_inited = 0;
		return;
	}

	if (Mix_OpenAudio(44100, AUDIO_S16SYS, 2, 1024) < 0) {
		vultures_opts.play_effects = 0;
		vultures_opts.play_music = 0;
		vultures_sound_inited = 0;
		return;
	}

	Mix_AllocateChannels(4);

	/* Create the sound cache */
	vultures_cached_sounds = new vultures_cached_sound[V_MAX_CACHED_SOUNDS];
	for (i = 0; i < V_MAX_CACHED_SOUNDS; i++) {
		vultures_cached_sounds[i].chunk = NULL;
		vultures_cached_sounds[i].filename = "";
	}

	/* Initialize cd playing. */
	vultures_cdrom = NULL;
	if (SDL_CDNumDrives() > 0)
		/* Open default drive */
		vultures_cdrom = SDL_CDOpen(0);

	vultures_sound_inited = 1;
}



/* tries to play a sound matching the given str */
void vultures_play_event_sound(const char * str)
{
	unsigned int i;

	/* search the configured sounds for one that matches str */
	for (i = 0; i < vultures_event_sounds.size(); i++) {
		if (strstr(str, vultures_event_sounds[i].searchpattern)) {
			switch (vultures_event_sounds[i].soundtype)
			{
				case V_EVENT_SOUND_TYPE_SND:
					vultures_play_sound(vultures_event_sounds[i].filename);
					break;

				case V_EVENT_SOUND_TYPE_MUS:
					vultures_play_song(vultures_event_sounds[i].filename);
					break;

				case V_EVENT_SOUND_TYPE_CD_AUDIO:
					vultures_play_cd_track(vultures_event_sounds[i].filename);
					break;

				case V_EVENT_SOUND_TYPE_RANDOM_SONG:
					vultures_play_ambient_sound(1);
					break;
			}

			break;
		}
	}
}


void vultures_play_ambient_sound(int force_play)
{
	int k;
	char tempbuffer[256];

	if ((!force_play) && (vultures_is_music_playing()))
		return;

	if (force_play)
		vultures_stop_music();

	k = (rand() >> 4) % vultures_n_background_songs;

	sprintf(tempbuffer, "nhfe_music_background%03d", k);
	vultures_play_event_sound(tempbuffer);
}


static void vultures_play_song(string midifilename)
{
	if (!vultures_opts.play_music)
		return;

	if (vultures_current_music)
		Mix_FreeMusic(vultures_current_music);

	vultures_current_music = Mix_LoadMUS(midifilename.c_str());
	Mix_PlayMusic(vultures_current_music,0);
}


static void vultures_play_cd_track(string cdtrackname)
{
	int nTrack;

	if (!vultures_opts.play_music)
		return;

	/* Parse the track number from the given string */
	nTrack = atoi(cdtrackname.c_str());
	if (nTrack < 0) { 
		vultures_write_log(V_LOG_NOTE, __FILE__, __LINE__,
                           "Invalid track number [%s]\n", cdtrackname.c_str());
		return;
	}

	if (!vultures_cdrom)
		return;

	if (!CD_INDRIVE(SDL_CDStatus(vultures_cdrom))) {
		vultures_write_log(V_LOG_DEBUG, __FILE__, __LINE__, "No CD in drive\n");
		return;
	}

	SDL_CDPlayTracks(vultures_cdrom, nTrack, 0, 1, 0);
}


static void vultures_play_sound(string wavefilename)
{
	int i;
	int sound_exists;
	Mix_Chunk *chunk = NULL;

	if (!vultures_opts.play_effects) return;

	/* Check if the sound exists in the sound cache */
	sound_exists = 0;
	for (i = 0; i < V_MAX_CACHED_SOUNDS; i++)
		if (wavefilename == vultures_cached_sounds[i].filename) {
			sound_exists = 1;
			chunk = vultures_cached_sounds[i].chunk;
			break;
		}

	if (!sound_exists) {
		i = vultures_oldest_cached_sound;

		if (vultures_cached_sounds[i].chunk)
			Mix_FreeChunk(vultures_cached_sounds[i].chunk);

		vultures_cached_sounds[i].filename = wavefilename;

		vultures_cached_sounds[i].chunk = Mix_LoadWAV(wavefilename.c_str());
		chunk = vultures_cached_sounds[i].chunk;

		vultures_oldest_cached_sound++;
		if (vultures_oldest_cached_sound >= V_MAX_CACHED_SOUNDS)
			vultures_oldest_cached_sound = 0;
	}

	/* Play sound */
	vultures_write_log(V_LOG_DEBUG, __FILE__, __LINE__, "Playing file %s\n", wavefilename.c_str());
	Mix_PlayChannel(-1, chunk, 0);
}


void vultures_stop_music(void)
{
	if (vultures_current_music) Mix_FreeMusic(vultures_current_music);
	vultures_current_music = NULL;

	/* Stop any CD tracks playing */
	if (vultures_cdrom) {
		if (SDL_CDStatus(vultures_cdrom) == CD_PLAYING)
		SDL_CDStop(vultures_cdrom);
	}
}


static int vultures_is_music_playing(void)
{
	/* Check for external music files (MIDI or MP3) playing */
	if (Mix_PlayingMusic() > 0) 
		return 1;

	/* Check for CD tracks playing */
	if (vultures_cdrom) {
		if (SDL_CDStatus(vultures_cdrom) == CD_PLAYING)
		return 1;
	}  

	/* No music playing */
	return 0;
}




