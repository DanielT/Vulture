/* Copyright (c) Daniel Thaler, 2006   */
/* NetHack may be freely redistributed under the Nethack General Public License
 * See nethack/dat/license for details */

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "SDL.h"
#include "SDL_ttf.h"
#include "vultures_txt.h"
#include "vultures_gen.h"
#include "vultures_win.h"

#define VULTURES_MAX_FONTS  2

/* the rest of vultures gets these defines indirectly from nethack code.
 * but I'd prefer not to pull all that in here */
#define TRUE 1
#define FALSE 0

static struct vultures_font {
             TTF_Font * fontptr;
             char lineheight;
} * vultures_fonts = NULL;


int vultures_load_font (int font_id, const char * ttf_filename, int fontindex, int pointsize)
{
    char * fullname;
    TTF_Font * newfont;
    if(!TTF_WasInit())
    {
        if(TTF_Init()==-1)
            return FALSE;

        if (!vultures_fonts)
            vultures_fonts = calloc(VULTURES_MAX_FONTS, sizeof(struct vultures_font));
    }

    if (font_id >= VULTURES_MAX_FONTS)
        return FALSE;

    /* add the path to the filename */
    fullname = vultures_make_filename(V_FONTS_DIRECTORY, NULL, ttf_filename);

    newfont = TTF_OpenFontIndex(fullname, pointsize, fontindex);

    free(fullname);

    if (!newfont)
        return FALSE;

    if (vultures_fonts[font_id].fontptr)
        TTF_CloseFont(vultures_fonts[font_id].fontptr);

    vultures_fonts[font_id].fontptr = newfont;
    vultures_fonts[font_id].lineheight = TTF_FontAscent(newfont) + 2;

    return TRUE;
}



int vultures_put_text (int font_id, const char *str, SDL_Surface *dest, int x, int y, Uint32 color)
{
    SDL_Surface * textsurface;
    SDL_Color fontcolor;
    SDL_Rect dstrect;
    char * cleaned_str;
    int len, i;

    if (font_id >= VULTURES_MAX_FONTS || (!vultures_fonts[font_id].fontptr) || !str)
        return FALSE;

    /* sanitize the input string of unprintable characters */
    len = strlen(str);
    cleaned_str = malloc(len+1);
    strcpy(cleaned_str, str);
    for (i = 0; i < len; i++)
        if (!isprint(cleaned_str[i]))
            cleaned_str[i] = ' ';


    /* extract components from the 32-bit color value */
    fontcolor.r = (color & dest->format->Rmask) >> dest->format->Rshift;
    fontcolor.g = (color & dest->format->Gmask) >> dest->format->Gshift;
    fontcolor.b = (color & dest->format->Bmask) >> dest->format->Bshift;
    fontcolor.unused = 0;

    textsurface = TTF_RenderText_Blended(vultures_fonts[font_id].fontptr, cleaned_str, fontcolor);
    if (!textsurface)
    {
        free(cleaned_str);
        return FALSE;
    }

    free(cleaned_str);

    dstrect.x = x;
    dstrect.y = y - 1;
    dstrect.w = textsurface->w;
    dstrect.h = textsurface->h;

    SDL_BlitSurface(textsurface, NULL, dest, &dstrect);
    SDL_FreeSurface(textsurface);
    
    return TRUE;
}



int vultures_put_text_shadow (int font_id, const char *str, SDL_Surface *dest,
                              int x, int y, Uint32 textcolor, Uint32 shadowcolor)
{
    /* draw the shadow first */
    vultures_put_text(font_id, str, dest, x+1, y+1, shadowcolor);
    /* we only truly care that the actual text got drawn, so only retrun that status */
    return vultures_put_text(font_id, str, dest, x, y, textcolor);
}



int vultures_text_length (int font_id, const char *str)
{
    int width = 0;

    if (!vultures_fonts[font_id].fontptr || !str)
        return 0;

    TTF_SizeText(vultures_fonts[font_id].fontptr, str, &width, NULL);

    return width;
}



int vultures_text_height (int font_id, const char *str)
{
    int height = 0;

    if (!vultures_fonts[font_id].fontptr || !str)
        return 0;

    TTF_SizeText(vultures_fonts[font_id].fontptr, str, NULL, &height);

    return height;
}



int vultures_get_lineheight(int font_id)
{
    if (font_id < VULTURES_MAX_FONTS && vultures_fonts[font_id].fontptr)
        return vultures_fonts[font_id].lineheight;
    return 0;
}



void vultures_free_fonts()
{
    int font_id;

    for (font_id = 0; font_id < VULTURES_MAX_FONTS; font_id++)
    {
        if (vultures_fonts[font_id].fontptr)
            TTF_CloseFont(vultures_fonts[font_id].fontptr);
        vultures_fonts[font_id].fontptr = NULL;
    }

    free(vultures_fonts);
    vultures_fonts = NULL;

    TTF_Quit();
}
