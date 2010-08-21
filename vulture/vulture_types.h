/* NetHack may be freely redistributed.  See license for details. */

#ifndef _vulture_types_h_
#define _vulture_types_h_

#include <SDL.h>
/*
 * This file is meant to be a place to put structure declarations
 * and enums that have global importance.
 * 
 * In the interest of modularization THIS FILE SHOULD NOT EXPORT FUNCTIONS
 */
typedef struct { 
    /* image data (width & height encoded in first 4 bytes) */
    SDL_Surface *graphic;
    /* hotspot offsets;
     * difference between left/top most non-transparent pixel
     * and hotspot defined in the image
     */
    int xmod,ymod;
} vulture_tile;


typedef struct {
    int x;
    int y;
} point;

#endif

