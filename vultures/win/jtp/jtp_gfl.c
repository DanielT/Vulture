/*  SCCS Id: @(#)jtp_gfl.c  3.0 2000/9/10 */
/* Copyright (c) Jaakko Peltonen, 2000				  */
/* NetHack may be freely redistributed.  See license for details. */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#if defined(__GNUC__)
#include <unistd.h>
#endif
#include "jtp_gra.h"
#include "jtp_gfl.h"
#include "jtp_gen.h"
#include "jtp_win.h"

/* PCX file format constants */
#define JTP_PCX_LOADBUFFERSIZE 10000
#define JTP_PCX_HEADERSIZE     128
#define JTP_PCX_MANUFACTURER   10
#define JTP_PCX_VERSION        5
#define JTP_PCX_ENCODING       1
#define JTP_PCX_BITS_PER_PIXEL 8
#define JTP_PCX_RESERVED       0
#define JTP_PCX_COLORPLANES    1
#define JTP_PCX_PALETTE_INFO   1
#define JTP_PCX_FILLER         0
#define JTP_PCX_256COL_PALETTE 12

/*--------------------------------------------------------------------------
 256-color PCX loader (into screen buffer)
 xalku       : buffer x-coordinate of upper left image corner
 yalku       : buffer y-coordinate of upper left image corner
 pcxname     : filename to load image from
 loadpalette : load image palette into screen palette? (1=yes, 0=no)
--------------------------------------------------------------------------*/
int jtp_load_PCX
(
  int xbegin,int ybegin,
  const char *pcxname,
  char loadpalette
)
{
  FILE *f;
  int x_size,y_size;
  int linewidth;
  int pixels_read=0;
  int i=xbegin,j=ybegin,k,l,amount;
  unsigned char col;
  unsigned char *buffer;
  unsigned char numero, header[JTP_PCX_HEADERSIZE];
  
  buffer = (unsigned char *)malloc(JTP_PCX_LOADBUFFERSIZE*sizeof(unsigned char));
  f = fopen(pcxname,"rb");
  if (f == NULL)
  {
    jtp_write_log_message(JTP_LOG_NOTE, __FILE__, __LINE__, "Could not open file [%s] for reading: %s\n", pcxname, strerror(errno));
    return(JTP_PCX_FAILURE);
  }
  
  fread(header,1,JTP_PCX_HEADERSIZE,f);
  x_size =   ((int)header[ 9]*256+(int)header[ 8])
            -((int)header[ 5]*256+(int)header[ 4])+1;
  y_size =   ((int)header[11]*256+(int)header[10])
            -((int)header[ 7]*256+(int)header[ 6])+1;
  linewidth = (int)header[67]*256 + (int)header[66];
 
  k = JTP_PCX_LOADBUFFERSIZE-1;
    
  while(pixels_read < x_size * y_size)
  {
    k++; if (k >= JTP_PCX_LOADBUFFERSIZE) { fread(buffer,1,JTP_PCX_LOADBUFFERSIZE,f); k = 0; }
    numero = buffer[k];
    if((numero & (128+64)) == 128+64)     
    {                                 
      amount = numero & (32+16+8+4+2+1); 
      k++; if (k >= JTP_PCX_LOADBUFFERSIZE) { fread(buffer,1,JTP_PCX_LOADBUFFERSIZE,f); k = 0; }
      col = buffer[k];
    }
    else { amount = 1; col = numero; }
     
    for(l = 0; l < amount; l++)
    {
      jtp_pixelput(i,j,col);
      i++;
      if (i >= x_size + xbegin)
      {
        while (i < linewidth)
        {
          i++;
          k++; if (k >= JTP_PCX_LOADBUFFERSIZE) { fread(buffer,1,JTP_PCX_LOADBUFFERSIZE,f); k = 0; }
        }
        i = xbegin; j++;
      }
    }
    pixels_read += amount;
  }
 
  /* If there is a 256-color palette, load it */
  fseek(f,-769,SEEK_END);
  fread(&numero,1,1,f);
  if((numero == JTP_PCX_256COL_PALETTE)&&(loadpalette))
  {
    fread(jtp_colors,3,256,f);
    for(l = 0; l < 256; l++)
    {
      jtp_colors[l][0]>>=2;
      jtp_colors[l][1]>>=2;
      jtp_colors[l][2]>>=2;
      jtp_correct_gamma(&jtp_colors[l][0], 
                        &jtp_colors[l][1], 
                        &jtp_colors[l][2],
                        jtp_gamma_correction);
    }
  }
  fclose(f);
  free(buffer);
  return(JTP_PCX_SUCCESS);
}


/*--------------------------------------------------------------------------
 256-color PCX palette loader (just the palette, not the actual image)
 pcxname     : filename to load the palette from
--------------------------------------------------------------------------*/
int jtp_load_palette(const char *pcxname)
{
  FILE *f;
  int l;
  unsigned char numero;
  
  f = fopen(pcxname,"rb");
  if (f == NULL)
  {
    jtp_write_log_message(JTP_LOG_NOTE, __FILE__, __LINE__, "Could not open file [%s] for reading: %s\n", pcxname, strerror(errno));
    return(JTP_PCX_FAILURE);
  }
   
  /* If there is a 256-color palette, load it */
  fseek(f,-769,SEEK_END);
  fread(&numero,sizeof(char),1,f);
  if (numero == JTP_PCX_256COL_PALETTE)
  {
    fread(jtp_colors,3,256,f);
    for(l = 0; l < 256; l++)
    {
      jtp_colors[l][0]>>=2;
      jtp_colors[l][1]>>=2;
      jtp_colors[l][2]>>=2;
      jtp_correct_gamma(&jtp_colors[l][0], 
                        &jtp_colors[l][1], 
                        &jtp_colors[l][2],
                        jtp_gamma_correction);
    }
  }
  fclose(f);
  return(JTP_PCX_SUCCESS);
}


/*--------------------------------------------------------------------------
 256-color PCX loader (into new image buffer)
 xbegin       : buffer x-coordinate of upper left image corner
 ybegin       : buffer y-coordinate of upper left image corner
 img_destin   : address to buffer pointer (function sets this pointer)
 pcxname      : filename to load image from
 loadpalette  : load image palette into screen palette? (1=yes, 0=no)
--------------------------------------------------------------------------*/
int jtp_load_PCX_buf
(
  int xbegin,int ybegin,
  const char *pcxname,
  unsigned char **img_destin,
  char loadpalette
)
{
  FILE *f;
  int x_size,y_size;
  int linewidth;
  int pixels_read=0;
  int i=xbegin,j=ybegin,k,l,amount;
  unsigned char col;
  unsigned char *buffer;
  unsigned char numero,header[JTP_PCX_HEADERSIZE];
 
  buffer = (unsigned char *)malloc(JTP_PCX_LOADBUFFERSIZE*sizeof(unsigned char));
  f = fopen(pcxname,"rb");
  if (f == NULL) {*img_destin = NULL; return(JTP_PCX_FAILURE);}
 
  fread(header,1,JTP_PCX_HEADERSIZE,f);
  x_size=    ((int)header[ 9]*256+(int)header[ 8])
            -((int)header[ 5]*256+(int)header[ 4])+1;
  y_size=    ((int)header[11]*256+(int)header[10])
            -((int)header[ 7]*256+(int)header[ 6])+1;
  linewidth = (int)header[67]*256 + (int)header[66];

  *img_destin=(unsigned char *)malloc((x_size*y_size+4)*sizeof(unsigned char));
  (*img_destin)[0]=y_size/256;
  (*img_destin)[1]=y_size%256;
  (*img_destin)[2]=x_size/256;
  (*img_destin)[3]=x_size%256;
 
  k = JTP_PCX_LOADBUFFERSIZE - 1;
   
  while(pixels_read<x_size*y_size)
  {
    k++; if (k >= JTP_PCX_LOADBUFFERSIZE) { fread(buffer,1,JTP_PCX_LOADBUFFERSIZE,f); k = 0; }
    numero=buffer[k];
    if((numero & (128+64))==128+64)     
    {                                 
      amount=numero & (32+16+8+4+2+1); 
      k++; if (k >= JTP_PCX_LOADBUFFERSIZE) { fread(buffer,1,JTP_PCX_LOADBUFFERSIZE,f); k = 0; }
      col=buffer[k];
    }
    else { amount=1; col=numero; }
    
    for(l=0;l<amount;l++)
    {
      (*img_destin)[j*x_size+i+4]=col;
      i++;
      if (i>=x_size+xbegin)
      {
        while (i < linewidth)
        {
          i++;
          k++; if (k >= JTP_PCX_LOADBUFFERSIZE) { fread(buffer,1,JTP_PCX_LOADBUFFERSIZE,f); k = 0; }
        }
        i = xbegin; j++;
      }
    }
    pixels_read+=amount;
  }

  /* If there is a 256-color palette, load it */
  fseek(f,-769,SEEK_END);
  fread(&numero,1,1,f);
  if((numero==JTP_PCX_256COL_PALETTE)&&(loadpalette))
  {
    fread(jtp_colors,3,256,f);
    for(l=0;l<256;l++)
    {
      jtp_colors[l][0]>>=2;
      jtp_colors[l][1]>>=2;
      jtp_colors[l][2]>>=2;
    }
  }
  fclose(f);
  free(buffer);
  return(JTP_PCX_SUCCESS);
}


/*--------------------------------------------------------------------------
 256-color PCX saver (from image buffer)
 pcxname     : filename to save image into
 image       : pointer to image buffer (palette is screen palette)
--------------------------------------------------------------------------*/
int jtp_save_PCX
(
  const char *pcxname,
  unsigned char *image
)
{
  FILE *f;
  int x_size, y_size;
  int i, j;
  int runwidth;
  unsigned char col, header[128];

  y_size = image[0]; y_size = y_size*256 + (int)image[1];
  x_size = image[2]; x_size = x_size*256 + (int)image[3];
  image += 4;

  f = fopen(pcxname,"wb"); if (f == NULL) return(JTP_PCX_FAILURE);
  /* Write PCX header */

  header[0] = JTP_PCX_MANUFACTURER;                   /* manufacturer */
  header[1] = JTP_PCX_VERSION;                        /* version */
  header[2] = JTP_PCX_ENCODING;                       /* encoding */
  header[3] = JTP_PCX_BITS_PER_PIXEL;                 /* bits per pixel */
  header[4] = 0; header[5] = 0;                   /* picture xmin */
  header[6] = 0; header[7] = 0;                   /* picture ymin */
  i = x_size - 1; j = y_size - 1;
  header[8] = i%256; header[9] = i/256;           /* picture xmax */
  header[10] = j%256; header[11] = j/256;         /* picture ymax */
  i = x_size; j = y_size;
  header[12] = i%256; header[13] = i/256;         /* horiz. resolution */
  header[14] = j%256; header[15] = j/256;         /* vert. resolution */
  for (i = 0; i < 16; i++)                        /* EGA palette */
    for (j = 0; j < 3; j++)
      header[16 + 3*i + j] = jtp_colors[i][j]*4;
  header[64] = JTP_PCX_RESERVED;                      /* reserved */
  header[65] = JTP_PCX_COLORPLANES;                   /* color planes */
  i = x_size + x_size%2;
  header[66] = i%256; header[67] = i/256;         /* bytes per line */
  i = JTP_PCX_PALETTE_INFO;
  header[68] = i%256; header[69] = i/256;         /* palette info */
  for (i = 70; i < 128; i++)                      /* filler to 128 bytes */
    header[i] = JTP_PCX_FILLER;  
  fwrite(header, sizeof(unsigned char), 128, f);
 
  /* Encode image with run-length encoding and save into file */

  for (i = 0; i < y_size; i++)
  {
    col = image[0]; runwidth = 0;
    for (j = 0; j < x_size; j++)
    {
      if (image[j] == col) runwidth++;
      else
      {
        if ((runwidth == 1)&&(col < 192)) fputc(col, f);
        else
        {
          while(runwidth >= 63) { fputc(192 + 63, f); fputc(col, f); runwidth -= 63; }
          if (runwidth > 0) { fputc(192 + runwidth, f); fputc(col, f); }
        }
        col = image[j]; runwidth = 1;    
      }
    }
    image += x_size;

    /* write leftover scans at end of line */

    while(runwidth >= 63) { fputc(192 + 63, f); fputc(col, f); runwidth -= 63; }
    if (runwidth > 0) { fputc(192 + runwidth, f); fputc(col, f); }
    if ((x_size%2) == 1) fputc(JTP_PCX_FILLER, f);
  }

  /* Write VGA 256-color palette */
  fputc(JTP_PCX_256COL_PALETTE, f);
  for (i = 0; i < 256; i++)
    for (j = 0; j < 3; j++)
      fputc(jtp_colors[i][j]*4, f);

  /* Complete! Close file and exit function */
  fclose(f);
  return(JTP_PCX_SUCCESS);
}


/*-------------------------------------
 Save the contents of the screen (ie.
 back buffer) into a PCX file.
---------------------------------------*/
void jtp_save_screenshot(void)
{
	unsigned char *tempimg;
	char *filename;
	int i;
	char *msg;
	char namebuf[20];
	
	for (i = 0; i < 1000; i++)
	{
		sprintf(namebuf, "scree%03d.pcx", i);
		filename = jtp_make_filename(NULL, NULL, namebuf);
		if (access(filename, R_OK) != 0)
		{
			tempimg = jtp_get_img(0, 0, jtp_screen.width - 1, jtp_screen.height - 1);
			jtp_save_PCX(filename, tempimg);
			free(tempimg);
			msg = malloc(sizeof("Screenshot saved as ") + strlen(filename) + 2);
			strcpy(msg, "Screenshot saved as ");
			strcat(msg, namebuf);
			strcat(msg, ".");
			jtp_messagebox(msg);
			free(msg);
			free(filename);
			return;
		}
		free(filename);
	}
	jtp_messagebox("Too many screenshots already saved.");
}
