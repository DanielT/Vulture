#ifndef _jtp_gfl_h_
#define _jtp_gfl_h_


#include "vultures_win.h"



/* The following return value constants are provided: */
#define JTP_IMAGE_SUCCESS        1
#define JTP_IMAGE_FAILURE        0


/*--------------------------------------------------------------------------
 256-color Image loader (into screen buffer) 
--------------------------------------------------------------------------*/
int jtp_load_image
(
  int xbegin,          /* buffer x-coordinate of upper left image corner */
  int ybegin,          /* buffer y-coordinate of upper left image corner */
  const char *imagename, /* filename to load image from */
  char loadpalette     /* load image palette into screen palette? (1=yes, 0=no) */
);


/*--------------------------------------------------------------------------
 256-color Image loader (into new image buffer)
--------------------------------------------------------------------------*/
int jtp_load_image_buf
(
  const char *imagename,         /* filename to load image from */
  unsigned char **img_destin,  /* address to buffer pointer (function sets this pointer) */
  char loadpalette             /* load image palette into screen palette? (1=yes, 0=no) */
);

/*--------------------------------------------------------------------------
 Tile loader
--------------------------------------------------------------------------*/
char * vultures_load_image(const char *srcbuf, unsigned int buflen, char loadpalette);

#define jtp_get_img_height(image) \
	((image)[0] * 256 + (image)[1])

#define jtp_get_img_width(image) \
	((image)[2] * 256 + (image)[3])

#define jtp_get_dimensions(image, w, h) \
	(*(w) = jtp_get_img_width(image), \
	 *(h) = jtp_get_img_height(image)) 

#define jtp_put_dimensions(image, w, h) \
	((image)[0] = (h) >> 8, \
	 (image)[1] = (h) & 255, \
	 (image)[2] = (w) >> 8, \
	 (image)[3] = (w) & 255)

/*--------------------------------------------------------------------------
 256-color Image palette loader
--------------------------------------------------------------------------*/
int jtp_load_palette
(
  const char *imagename         /* filename to load palette from */
);


/*-------------------------------------
 Save the contents of the screen (ie.
 back buffer) into a BMP file.
---------------------------------------*/
void jtp_save_screenshot(void);

#endif
