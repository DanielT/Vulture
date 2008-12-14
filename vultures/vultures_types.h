/* Copyright (c) Daniel Thaler, 2006, 2008                        */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef _vultures_types_h_
#define _vultures_types_h_
/*
 * This file is meant to be a place to put structure declarations
 * and enums that have global importance.
 * 
 * In the interest of modularization THIS FILE SHOULD NOT EXPORT FUNCTIONS
 */

#include <SDL.h>



#define V_LISTITEM_WIDTH  300
#define V_LISTITEM_HEIGHT  52



typedef struct { 
    /* image data (width & height encoded in first 4 bytes) */
    SDL_Surface *graphic;
    /* hotspot offsets;
     * difference between left/top most non-transparent pixel
     * and hotspot defined in the image
     */
    int xmod,ymod;
} vultures_tile;


typedef struct {
    int x;
    int y;
} point;

#endif

