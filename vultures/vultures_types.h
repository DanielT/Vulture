#ifndef _vultures_types_h_
#define _vultures_types_h_
/*
 * This file is meant to be a place to put structure declarations so that dependnecy problems can be avoided
 * (win.h defined jtp_tile, which was neede in mou.h; mou.h defined jtp_hotspot which was needed in win.h ...)
 */


typedef struct { 
    /* image data (width & height encoded in first 4 bytes) */
    unsigned char *graphic;
    /* hotspot offsets;
     * difference between left/top most non-transparent pixel
     * and hotspot defined in the image
     */
    int xmod,ymod;
} jtp_tile;



typedef struct {
  int x1, x2, y1, y2;
  jtp_tile * mcursor;
  unsigned char * tooltip;
  int accelerator;
} jtp_hotspot;


#endif

