#include "vultures_sdl.h" /* XXX this must be the first include,
                             no idea why but it won't compile otherwise */

#include <stdio.h>
#include <stdlib.h>
#ifdef __GNUC__
#include <unistd.h>
#endif

#include <ctype.h>
#ifdef WIN32
#include "win32api.h"
#endif

#include "hack.h"


#include "SDL.h"
#define USE_RWOPS
# include "SDL_mixer.h"
#undef USE_RWOPS

#include "vultures_gen.h"
#include "vultures_sound.h"
#include "vultures_pcmusic.h"
#include "vultures_opt.h"


#ifdef PCMUSIC

#define INSTRUMENT_FILE "music/instruments.xm"
#define PATTERN_OFFSET 0x159 /* XXX This hard coded amount needs to be changed if instruments.xm
							* is resaved.  It's the physical file offset of where the saved
							* blank patterns begin */
#define pcmusic_load_instruments ( pcmusic_load_instrument_file()==INSTRUMENT_LOAD_SUCCESS )

#define XM_PATTERNCOMPRESSION                       0x80
#define XM_PATTERNCOMPRESSION_NOTEFOLLOWS           1<<0
#define XM_PATTERNCOMPRESSION_INSTRUMENTFOLLOWS     1<<1
#define XM_PATTERNCOMPRESSION_VOLUMEFOLLOWS         1<<2
#define XM_PATTERNCOMPRESSION_EFFECTFOLLOWS         1<<3
#define XM_PATTERNCOMPRESSION_EFFECTPARAMFOLLOWS    1<<4

#define XM_EFFECT_PATTERNBREAK  0x0D;

enum
{
	INSTR_BELL = 1,
	INSTR_BUGLE,
	INSTR_DRUM_OF_EARTHQUAKE,
	INSTR_FIRE_HORN,
	INSTR_FROST_HORN,
	INSTR_LEATHER_DRUM,
	INSTR_MAGIC_FLUTE,
	INSTR_MAGIC_HARP,
	INSTR_TOOLED_HORN,
	INSTR_WOODEN_FLUTE,
	INSTR_WOODEN_HARP
};

static int pcmusic_load_instrument_file( void );
static unsigned char *pcmusic_instrument_buffer = NULL;
static unsigned int   pcmusic_instrument_buffer_size = 0;
static Mix_Music *pcmusic_instrument_music = NULL;

static void vultures_pcmusic_done( void )
{
	if ( pcmusic_instrument_buffer != NULL ) free( pcmusic_instrument_buffer );
	if ( pcmusic_instrument_music != NULL ) Mix_FreeMusic( pcmusic_instrument_music );
}

void vultures_pcmusic_init( void )
{
	if (!vultures_opts.play_effects) return;

	atexit( vultures_pcmusic_done );
	if (!pcmusic_load_instruments) vultures_write_log(V_LOG_ERROR, __FILE__, __LINE__, "Unable to load instruments");
}

int pcmusic_load_instrument_file( void )
{
	FILE *instrument_file;

	if ( pcmusic_instrument_buffer != NULL ) return INSTRUMENT_LOAD_SUCCESS;

	instrument_file = fopen( INSTRUMENT_FILE, "rb" );
	if ( instrument_file == NULL ) return INSTRUMENT_LOAD_FAILED;

	fseek( instrument_file, 0, SEEK_END );
	pcmusic_instrument_buffer_size = ftell( instrument_file );
	fseek( instrument_file, 0, SEEK_SET );

	pcmusic_instrument_buffer = (unsigned char *)malloc( pcmusic_instrument_buffer_size );
	if ( pcmusic_instrument_buffer == NULL )
	{
		fclose( instrument_file );
		return INSTRUMENT_LOAD_FAILED;
	}

	if ( fread( pcmusic_instrument_buffer, 1, pcmusic_instrument_buffer_size, instrument_file ) != pcmusic_instrument_buffer_size )
	{
		free( pcmusic_instrument_buffer );
		fclose( instrument_file );
		return INSTRUMENT_LOAD_FAILED;
	};

	return INSTRUMENT_LOAD_SUCCESS;
}

static void music_finished( void )
{
	vultures_play_ambient_sound( 0 );
}

void pc_speaker(struct obj *instr, char *tune)
{
	SDL_RWops *rw;
	unsigned char *patternbuffer;
	char *note;
	int octave = 4;

	if (!vultures_opts.play_effects || !pcmusic_load_instruments) return;

	patternbuffer = &pcmusic_instrument_buffer[PATTERN_OFFSET];
	memset( patternbuffer, XM_PATTERNCOMPRESSION, 2*80 );

	*patternbuffer++ = XM_PATTERNCOMPRESSION_INSTRUMENTFOLLOWS;
	switch (instr->otyp)
	{
		case MAGIC_FLUTE:
			*patternbuffer++ = INSTR_MAGIC_FLUTE;
			break;
		case TOOLED_HORN:
			*patternbuffer++ = INSTR_TOOLED_HORN;
			break;
		case FROST_HORN:
			*patternbuffer++ = INSTR_FROST_HORN;
			break;
		case FIRE_HORN:
			*patternbuffer++ = INSTR_FIRE_HORN;
			break;
		case BUGLE:
			*patternbuffer++ = INSTR_BUGLE;
			break;
		case WOODEN_HARP:
			*patternbuffer++ = INSTR_WOODEN_HARP;
			break;
		case MAGIC_HARP:
			*patternbuffer++ = INSTR_MAGIC_HARP;
			break;
		case WOODEN_FLUTE:
		default:
			*patternbuffer++ = INSTR_WOODEN_FLUTE;
	}
	*patternbuffer++ = XM_PATTERNCOMPRESSION; /* Skip 2nd channel */
	note = tune;
	while ( *note )
	{
		switch (toupper(*note))
		{
			case '<':
				if ( octave > 0 ) octave--;
				break;
			case '>':
				if ( octave < 7 ) octave++;
				break;
			case 'A':
			case 'B':
			case 'C':
			case 'D':
			case 'E':
			case 'F':
			case 'G':
				*patternbuffer++ = XM_PATTERNCOMPRESSION|XM_PATTERNCOMPRESSION_NOTEFOLLOWS;
				*patternbuffer++ = 12*octave+(*note-'A');
				*patternbuffer++ = XM_PATTERNCOMPRESSION; /* Skip 2nd channel */
				break;
			default:
				*patternbuffer++ = XM_PATTERNCOMPRESSION;
				*patternbuffer++ = XM_PATTERNCOMPRESSION; /* Skip 2nd channel */
		}
		note++;
	}

	*patternbuffer++ = XM_PATTERNCOMPRESSION|XM_PATTERNCOMPRESSION_EFFECTFOLLOWS; /* Effect follows */
	*patternbuffer++ = XM_EFFECT_PATTERNBREAK; /* End pattern */

	rw = SDL_RWFromMem(pcmusic_instrument_buffer, pcmusic_instrument_buffer_size);
	if ( pcmusic_instrument_music != NULL ) Mix_FreeMusic( pcmusic_instrument_music );
	pcmusic_instrument_music = Mix_LoadMUS_RW( rw );
	if ( pcmusic_instrument_music != NULL ) Mix_PlayMusic( pcmusic_instrument_music, 0);

	Mix_HookMusicFinished( &music_finished );
}

#endif /* PCMUSIC */
