/* NetHack may be freely redistributed.  See license for details. */

#include <stdlib.h>
#include <string.h>

#include "SDL.h"
#include "SDL_audio.h"
#include "SDL_mixer.h"


#include "vulture_win.h"
#include "vulture_gen.h"
#include "vulture_sound.h"
#include "vulture_opt.h"

/*******************************************************************/


std::vector<vulture_event_sound> vulture_event_sounds;
int vulture_n_background_songs;
int vulture_sound_inited = 0;


/* Sound effects objects */
vulture_cached_sound *vulture_cached_sounds;
int vulture_oldest_cached_sound = 0;

/* Music objects */
SDL_CD *vulture_cdrom = NULL;


Mix_Music *vulture_current_music=NULL;

/*******************************************************************/

static void vulture_play_song(std::string midifilename);
static void vulture_play_cd_track(std::string cdtrackname);
static void vulture_play_sound(std::string wavefilename);
static int vulture_is_music_playing(void);

/*******************************************************************/


void vulture_init_sound(void)
{
	int i;

	if (vulture_sound_inited)
		return;

	if (SDL_InitSubSystem(SDL_INIT_AUDIO | SDL_INIT_CDROM) == -1) {
		/* init failed */
		vulture_opts.play_effects = 0;
		vulture_opts.play_music = 0;
		vulture_sound_inited = 0;
		return;
	}

	if (Mix_OpenAudio(44100, AUDIO_S16SYS, 2, 4096) < 0) {
		vulture_opts.play_effects = 0;
		vulture_opts.play_music = 0;
		vulture_sound_inited = 0;
		return;
	}

	Mix_AllocateChannels(4);

	/* Create the sound cache */
	vulture_cached_sounds = new vulture_cached_sound[V_MAX_CACHED_SOUNDS];
	for (i = 0; i < V_MAX_CACHED_SOUNDS; i++) {
		vulture_cached_sounds[i].chunk = NULL;
		vulture_cached_sounds[i].filename = "";
	}

	/* Initialize cd playing. */
	vulture_cdrom = NULL;
	if (SDL_CDNumDrives() > 0)
		/* Open default drive */
		vulture_cdrom = SDL_CDOpen(0);

	vulture_sound_inited = 1;
}



/* tries to play a sound matching the given str */
void vulture_play_event_sound(const char * str)
{
	unsigned int i;


	/* search the configured sounds for one that matches str */
	for (i = 0; i < vulture_event_sounds.size(); i++) {
		if (strstr(str, vulture_event_sounds[i].searchpattern)) {
      unsigned int effect_enum( rand()%vulture_event_sounds[i].filenames.size() );
			switch (vulture_event_sounds[i].soundtype)
			{
				case V_EVENT_SOUND_TYPE_SND:
					vulture_play_sound(vulture_event_sounds[i].filenames[effect_enum]);
					break;

				case V_EVENT_SOUND_TYPE_MUS:
					vulture_play_song(vulture_event_sounds[i].filenames[effect_enum]);
					break;

				case V_EVENT_SOUND_TYPE_CD_AUDIO:
					vulture_play_cd_track(vulture_event_sounds[i].filenames[effect_enum]);
					break;

				case V_EVENT_SOUND_TYPE_RANDOM_SONG:
					vulture_play_ambient_sound(1);
					break;
			}

			break;
		}
	}
}


void vulture_play_ambient_sound(int force_play)
{
	int k;
	char tempbuffer[256];

	if ((!force_play) && (vulture_is_music_playing()))
		return;

	if (force_play)
		vulture_stop_music();

	k = (rand() >> 4) % vulture_n_background_songs;

	sprintf(tempbuffer, "nhfe_music_background%03d", k);
	vulture_play_event_sound(tempbuffer);
}


static void vulture_play_song(std::string midifilename)
{
	if (!vulture_opts.play_music)
		return;

	if (vulture_current_music)
		Mix_FreeMusic(vulture_current_music);

	vulture_current_music = Mix_LoadMUS(midifilename.c_str());
	Mix_PlayMusic(vulture_current_music,0);
}


static void vulture_play_cd_track(std::string cdtrackname)
{
	int nTrack;

	if (!vulture_opts.play_music)
		return;

	/* Parse the track number from the given string */
	nTrack = atoi(cdtrackname.c_str());
	if (nTrack < 0) { 
		vulture_write_log(V_LOG_NOTE, __FILE__, __LINE__,
                           "Invalid track number [%s]\n", cdtrackname.c_str());
		return;
	}

	if (!vulture_cdrom)
		return;

	if (!CD_INDRIVE(SDL_CDStatus(vulture_cdrom))) {
		vulture_write_log(V_LOG_DEBUG, __FILE__, __LINE__, "No CD in drive\n");
		return;
	}

	SDL_CDPlayTracks(vulture_cdrom, nTrack, 0, 1, 0);
}


static void vulture_play_sound(std::string wavefilename)
{
	int i;
	int sound_exists;
	Mix_Chunk *chunk = NULL;

	if (!vulture_opts.play_effects) return;

	/* Check if the sound exists in the sound cache */
	sound_exists = 0;
	for (i = 0; i < V_MAX_CACHED_SOUNDS; i++)
		if (wavefilename == vulture_cached_sounds[i].filename) {
			sound_exists = 1;
			chunk = vulture_cached_sounds[i].chunk;
			break;
		}

	if (!sound_exists) {
		i = vulture_oldest_cached_sound;

		if (vulture_cached_sounds[i].chunk)
			Mix_FreeChunk(vulture_cached_sounds[i].chunk);

		vulture_cached_sounds[i].filename = wavefilename;

		vulture_cached_sounds[i].chunk = Mix_LoadWAV(wavefilename.c_str());
		chunk = vulture_cached_sounds[i].chunk;

		vulture_oldest_cached_sound++;
		if (vulture_oldest_cached_sound >= V_MAX_CACHED_SOUNDS)
			vulture_oldest_cached_sound = 0;
	}

	/* Play sound */
	vulture_write_log(V_LOG_DEBUG, __FILE__, __LINE__, "Playing file %s\n", wavefilename.c_str());
	Mix_PlayChannel(-1, chunk, 0);
}


void vulture_stop_music(void)
{
	if (vulture_current_music) Mix_FreeMusic(vulture_current_music);
	vulture_current_music = NULL;

	/* Stop any CD tracks playing */
	if (vulture_cdrom) {
		if (SDL_CDStatus(vulture_cdrom) == CD_PLAYING)
		SDL_CDStop(vulture_cdrom);
	}
}


static int vulture_is_music_playing(void)
{
	/* Check for external music files (MIDI or MP3) playing */
	if (Mix_PlayingMusic() > 0) 
		return 1;

	/* Check for CD tracks playing */
	if (vulture_cdrom) {
		if (SDL_CDStatus(vulture_cdrom) == CD_PLAYING)
		return 1;
	}  

	/* No music playing */
	return 0;
}




