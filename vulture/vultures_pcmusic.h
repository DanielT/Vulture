#ifndef VULTURES_PCMUSIC_H
#define VULTURES_PCMUSIC_H

enum
{   
	INSTRUMENT_LOAD_SUCCESS,
	INSTRUMENT_LOAD_FAILED
};

extern void vultures_pcmusic_init( void );

extern "C" void pc_speaker(struct obj *instr, char *tune);

#endif
