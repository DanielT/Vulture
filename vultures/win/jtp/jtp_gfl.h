#ifndef _jtp_gfl_h_
#define _jtp_gfl_h_



/* The following return value constants are provided: */
#define JTP_PCX_SUCCESS        1
#define JTP_PCX_FAILURE        0


/*--------------------------------------------------------------------------
 256-color PCX loader (into screen buffer) 
--------------------------------------------------------------------------*/
int jtp_load_PCX
(
  int xbegin,          /* buffer x-coordinate of upper left image corner */
  int ybegin,          /* buffer y-coordinate of upper left image corner */
  const char *pcxname, /* filename to load image from */
  char loadpalette     /* load image palette into screen palette? (1=yes, 0=no) */
);


/*--------------------------------------------------------------------------
 256-color PCX loader (into new image buffer)
--------------------------------------------------------------------------*/
int jtp_load_PCX_buf
(
  int xbegin,                  /* buffer x-coordinate of upper left image corner */
  int ybegin,                  /* buffer y-coordinate of upper left image corner */
  const char *pcxname,         /* filename to load image from */
  unsigned char **img_destin,  /* address to buffer pointer (function sets this pointer) */
  char loadpalette             /* load image palette into screen palette? (1=yes, 0=no) */
);

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
 256-color PCX palette loader
--------------------------------------------------------------------------*/
int jtp_load_palette
(
  const char *pcxname         /* filename to load palette from */
);


/*--------------------------------------------------------------------------
 256-color PCX saver (from image buffer)
--------------------------------------------------------------------------*/
int jtp_save_PCX
(
  const char *pcxname,         /* filename to save image into */
  unsigned char *image        /* pointer to image buffer (palette is screen palette) */
);

/*-------------------------------------
 Save the contents of the screen (ie.
 back buffer) into a PCX file.
---------------------------------------*/
void jtp_save_screenshot(void);

#endif
