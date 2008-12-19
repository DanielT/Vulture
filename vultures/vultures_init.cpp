/* NetHack may be freely redistributed.  See license for details. */
/* Copyright (c) Jaakko Peltonen, 2000				  */

#include <unistd.h>
#include <string.h>
#include <errno.h>

#include "vultures_win.h"
#include "vultures_map.h"
#include "vultures_gra.h"
#include "vultures_gen.h"
#include "vultures_sdl.h"
#include "vultures_gfl.h"
#include "vultures_txt.h"
#include "vultures_init.h"
#include "vultures_nhplayerselection.h"
#include "vultures_main.h"
#include "vultures_sound.h"
#include "vultures_mou.h"
#include "vultures_tile.h"
#include "vultures_opt.h"

#include "date.h" /* this is in <variant>/include it's needed for VERSION_ID */

/*----------------------------
* constants
*---------------------------- */

/* why is this here? It should ALWAYS be defined by unistd.h */
#ifndef R_OK
#  define R_OK 4
#endif


/*----------------------------
* pre-declared functions
*---------------------------- */
static void trimright(char *buf);
static void vultures_show_intro(const char *introscript_name);


/*----------------------------
* function implementaions
*---------------------------- */

static void trimright(char *buf)
{
	int i;

	i = strlen(buf) - 1;
	while (i >= 0 && (buf[i] == ' ' || buf[i] == '\n' || buf[i] == '\r'))
		i--;
	buf[i + 1] = '\0';
}


void vultures_show_logo_screen(void)
{
	SDL_Event event;
	SDL_Surface *logo;

	vultures_play_event_sound("nhfe_music_main_title");

	if (iflags.wc_splash_screen)
	{
		logo = vultures_load_graphic(NULL, V_FILENAME_NETHACK_LOGO);
		if (logo != NULL)
		{
			/* TODO Stretch this image fullscreen */
			vultures_put_img((vultures_screen->w - logo->w) / 2, (vultures_screen->h - logo->h) / 2, logo);

			vultures_put_text_shadow( V_FONT_INTRO, VERSION_ID ,vultures_screen,
					(vultures_screen->w - vultures_text_length( V_FONT_INTRO, VERSION_ID )) / 2,
					(vultures_screen->h - logo->h) / 2 + logo->h - ( vultures_text_height( V_FONT_INTRO, VERSION_ID) ),
					V_COLOR_INTRO_TEXT, V_COLOR_BACKGROUND);

			vultures_fade_in(0.5);

			vultures_wait_input(&event, 0);

			vultures_fade_out(0.2);

			SDL_FreeSurface(logo);
		}
	}

	vultures_refresh();
}



void vultures_player_selection(void)
{
	SDL_Surface *logo;
	char *filename;

	SDL_FillRect(vultures_screen, NULL, CLR32_BLACK);
	logo = vultures_load_graphic(NULL, V_FILENAME_CHARACTER_GENERATION);
	if (logo != NULL)
	{
		vultures_put_img((vultures_screen->w - logo->w) / 2, (vultures_screen->h - logo->h) / 2, logo);
		SDL_FreeSurface(logo);
	}

	vultures_fade_in(0.2);

	vultures_player_selection_internal();

	/* Success! Show introduction. */
	vultures_fade_out(0.2);

	filename = vultures_make_filename(V_CONFIG_DIRECTORY, NULL, V_FILENAME_INTRO_SCRIPT);

	if (flags.legacy)
		vultures_show_intro(filename);

	free(filename);
}



void vultures_askname(void)
{
	int done;
	char inputbuffer[256];

	done = vultures_get_input(-1, vultures_screen->h - 170,
						"What is your name?", inputbuffer);
	if (!done)
		/* Player pressed ESC during the name query, so quit the game */
		vultures_bail(NULL);

	strncpy(plname, inputbuffer, 32);

	/* do not fade out if user didn't enter anything, we will come back here */
	if (plname[0])
		vultures_fade_out(0.2);
}



static void vultures_show_intro(const char *introscript_name)
{
	FILE   * f;
	int      i, j;
	int      nScenes;
	int    * subtitle_rows;
	char *** subtitles;
	char  ** scene_images;
	char     tempbuffer[1024];
	int      lineno;
	int      lineheight = vultures_get_lineheight(V_FONT_INTRO);
	SDL_Rect textrect = {0, 0, vultures_screen->w, 0};
	SDL_Event event;
	SDL_Surface *image=NULL;

	nScenes = 0;
	scene_images = NULL;
	subtitle_rows = NULL;
	subtitles = NULL;

	vultures_play_event_sound("nhfe_music_introduction");

	f = fopen(introscript_name, "rb");
	if (f == NULL)
	{
		vultures_write_log(V_LOG_NOTE, NULL, 0, "intro script %s not found\n", introscript_name);
	} else
	{
		lineno = 1;
		while (fgets(tempbuffer, 1024, f))
		{
		if (tempbuffer[0] == '%') /* Start of new scene */
		{
			trimright(&tempbuffer[1]);
			i = strlen(&tempbuffer[1]);
			if (i > 0)
			{
			char *dot;
			
			nScenes++;
			scene_images = (char **)realloc(scene_images, nScenes*sizeof(char *));
			scene_images[nScenes-1] = (char *)malloc((i + 1) * sizeof(*tempbuffer));
			if (!scene_images[nScenes - 1])
			{
				OOM(1);
			}
			strcpy(scene_images[nScenes - 1], tempbuffer + 1);
			dot = strrchr(scene_images[nScenes - 1], '.');
			if (dot != NULL)
			{
				if (strcmp(dot, ".png") == 0)
				{
				*dot = '\0';
				} else
				{
				vultures_write_log(V_LOG_NOTE, NULL, 0, "scene image %s not png?", scene_images[nScenes - 1]);
				}
			}
			subtitle_rows = (int *)realloc(subtitle_rows, nScenes * sizeof(int));

			subtitle_rows[nScenes - 1] = 0;
			subtitles = (char ***)realloc(subtitles, nScenes * sizeof(char **));

			subtitles[nScenes - 1] = NULL;
			}
		}
		else /* New subtitle line for latest scene */
		{
			if (nScenes > 0)
			{
			subtitle_rows[nScenes - 1]++;
			subtitles[nScenes - 1] = (char **)realloc(subtitles[nScenes - 1],
				subtitle_rows[nScenes - 1] * sizeof(char *));

			/* Remove extra whitespace from line */
			trimright(tempbuffer);
			i = 0;
			while (tempbuffer[i] == ' ')
				i++;
			trimright(&tempbuffer[i]);
			/* Copy line to subtitle array */
			subtitles[nScenes - 1][subtitle_rows[nScenes - 1] - 1] = (char *)malloc(strlen(tempbuffer + i) + 1);
			if (!subtitles[nScenes - 1][subtitle_rows[nScenes - 1] - 1])
			{
				OOM(1);
			}
			strcpy(subtitles[nScenes - 1][subtitle_rows[nScenes - 1] - 1], tempbuffer + i);
			/* DEBUG printf("%s", subtitles[nScenes-1][subtitle_rows[nScenes-1]-1]); DEBUG */
			} else
			{
			vultures_write_log(V_LOG_NOTE, NULL, 0, "subtitle without a preceding scene in line %d of intro script %s\n",
				lineno, introscript_name);
			}
		}
		}
		fclose(f);

		if (nScenes == 0)
		{
		vultures_write_log(V_LOG_NOTE, NULL, 0, "no scenes found in intro script %s\n", introscript_name);
		} else
		{

		/*
		* Show each scene of the introduction in four steps:
		* - Erase previous image, load and fade in new image
		* - Print the subtitles
		* - Wait out a set delay
		* - Erase subtitles, Fade out 
		*/
		vultures_set_draw_region(0, 0, vultures_screen->w - 1, vultures_screen->h - 1);
		for (i = 0; i < nScenes; i++)
		{
			/* If we are starting, or the previous image was different, fade in the current image */
			if ((i <= 0) || (strcmp(scene_images[i], scene_images[i - 1]) != 0))
			{
			if (image)
				SDL_FreeSurface(image);

			image = vultures_load_graphic(NULL, scene_images[i]);
			SDL_FillRect(vultures_screen, NULL, CLR32_BLACK);
			if (image != NULL)
				vultures_put_img((vultures_screen->w - image->w) / 2, (vultures_screen->h - image->h) / 6, image);
			vultures_fade_in(0.2);
			}
			/* Show subtitles */
			for (j = 0; j < subtitle_rows[i] && image != NULL; j++)
			{

			vultures_put_text(V_FONT_INTRO, subtitles[i][j], vultures_screen,
								(vultures_screen->w - vultures_text_length(V_FONT_INTRO, subtitles[i][j])) / 2,
								2 * (vultures_screen->h - image->h) / 6 + image->h + lineheight * j,
								V_COLOR_INTRO_TEXT);
			}
			vultures_refresh();

			/* Wait until scene is over or player pressed a key */
			/* FIXME: should make the timeout depend on amount of text */
			vultures_wait_input(&event, 5000);
			if (event.type != SDL_TIMEREVENT)
				i = nScenes;

			/* Erase subtitles */

			if (image != NULL)
			{
				textrect.y = (vultures_screen->h - image->h) / 6 + image->h;
				textrect.h = vultures_screen->h - textrect.y;
				SDL_FillRect(vultures_screen, &textrect, CLR32_BLACK);
				vultures_refresh();
			}
		
			/* If we are at the end, or the next image is different, fade out the current image */
			if ((i >= nScenes - 1) || (strcmp(scene_images[i], scene_images[i + 1]) != 0))
			vultures_fade_out(0.2);
		}
		
		/* Clean up */
		for (i = 0; i < nScenes; i++)
		{
			free(scene_images[i]);
			for (j = 0; j < subtitle_rows[i]; j++)
			free(subtitles[i][j]);
			if (subtitles[i] != NULL)
			free(subtitles[i]);
		}
		if (subtitle_rows != NULL)
			free(subtitle_rows);
		if (scene_images != NULL)
			free(scene_images);
		}
	}

	if (image)
		SDL_FreeSurface(image);
}



static void vultures_init_colors()
{
	/* set up the colors used in the game
	* the only good way to do this without needing graphics to have been loaded first
	* is to create a surface here which we then put into display format + alpha ourselves */
	SDL_Surface * pixel = SDL_CreateRGBSurface(SDL_SWSURFACE | SDL_SRCALPHA, 1, 1, 32,
								0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000);
	SDL_Surface * reformatted = SDL_DisplayFormatAlpha(pixel);

	vultures_px_format = (SDL_PixelFormat *)malloc(sizeof(SDL_PixelFormat));
	memcpy(vultures_px_format, reformatted->format, sizeof(SDL_PixelFormat));

	SDL_FreeSurface(reformatted);
	SDL_FreeSurface(pixel);

	CLR32_BLACK      = SDL_MapRGBA(vultures_px_format, 0,0,0, 0xff);
	CLR32_BLACK_A30  = SDL_MapRGBA(vultures_px_format, 0,0,0, 0x50);
	CLR32_BLACK_A50  = SDL_MapRGBA(vultures_px_format, 0,0,0, 0x80);
	CLR32_BLACK_A70  = SDL_MapRGBA(vultures_px_format, 0,0,0, 0xB0);
	CLR32_GREEN      = SDL_MapRGBA(vultures_px_format, 0x57, 0xff, 0x57, 0xff);
	CLR32_YELLOW     = SDL_MapRGBA(vultures_px_format, 0xff, 0xff, 0x57, 0xff);
	CLR32_ORANGE     = SDL_MapRGBA(vultures_px_format, 0xff, 0xc7, 0x3b, 0xff);
	CLR32_RED        = SDL_MapRGBA(vultures_px_format, 0xff, 0x23, 0x07, 0xff);
	CLR32_GRAY20     = SDL_MapRGBA(vultures_px_format, 0xb7, 0xab, 0xab, 0xff);
	CLR32_GRAY70     = SDL_MapRGBA(vultures_px_format, 0x53, 0x53, 0x53, 0xff);
	CLR32_GRAY77     = SDL_MapRGBA(vultures_px_format, 0x43, 0x3b, 0x3b, 0xff);
	CLR32_PURPLE44   = SDL_MapRGBA(vultures_px_format, 0x4f, 0x43, 0x6f, 0xff);
	CLR32_LIGHTPINK  = SDL_MapRGBA(vultures_px_format, 0xcf, 0xbb, 0xd3, 0xff);
	CLR32_LIGHTGREEN = SDL_MapRGBA(vultures_px_format, 0xaa, 0xff, 0xcc, 0xff);
	CLR32_BROWN      = SDL_MapRGBA(vultures_px_format, 0x9b, 0x6f, 0x57, 0xff);
	CLR32_WHITE      = SDL_MapRGBA(vultures_px_format, 0xff, 0xff, 0xff, 0xff);
	CLR32_BLESS_BLUE = SDL_MapRGBA(vultures_px_format, 0x96, 0xdc, 0xfe, 0x60);
	CLR32_CURSE_RED  = SDL_MapRGBA(vultures_px_format, 0x60, 0x00, 0x00, 0x50);
	CLR32_GOLD_SHADE = SDL_MapRGBA(vultures_px_format, 0xf0, 0xe0, 0x57, 0x40);
}


int vultures_init_graphics(void)
{
	int all_ok = TRUE;
	SDL_Surface *image;
	char * fullname;
	int font_loaded = 0;


	vultures_write_log(V_LOG_DEBUG, __FILE__, __LINE__, "Initializing filenames\n");
	vultures_init_gamepath();


	/* Read options file */
	vultures_write_log(V_LOG_DEBUG, __FILE__, __LINE__, "Reading Vulture's options\n");
	vultures_read_options();


	/* Enter graphics mode */
	vultures_write_log(V_LOG_DEBUG, __FILE__, __LINE__, "Enter graphics mode\n");
	vultures_enter_graphics_mode();


	vultures_write_log(V_LOG_DEBUG, __FILE__, __LINE__, "Initializing screen buffer\n");


	vultures_init_colors();

	/* Load fonts */
	vultures_write_log(V_LOG_DEBUG, __FILE__, __LINE__, "Initializing fonts\n");

	/* try custom font first */
	if (iflags.wc_font_text)
	{
		if (access(iflags.wc_font_text, R_OK) == 0)
		{
			font_loaded = 1;
			font_loaded &= vultures_load_font(V_FONT_SMALL, iflags.wc_font_text, 0, 12);
			font_loaded &= vultures_load_font(V_FONT_LARGE, iflags.wc_font_text, 0, 14);
		}
		else
			printf("Could not access %s: %s\n", iflags.wc_font_text, strerror(errno));
	}

	if (!font_loaded) /* fallback to default font */
	{
		font_loaded = 1;
		/* add the path to the filename */
		fullname = vultures_make_filename(V_FONTS_DIRECTORY, NULL, V_FILENAME_FONT);

		font_loaded &= vultures_load_font(V_FONT_SMALL, fullname, 0, 12);
		font_loaded &= vultures_load_font(V_FONT_LARGE, fullname, 0, 14);

		free(fullname);
	}

	all_ok &= font_loaded;

	/* Load window style graphics */
	image = vultures_load_graphic(NULL, V_FILENAME_WINDOW_STYLE);
	if (image == NULL)
		all_ok = FALSE;
	else
	{
		vultures_winelem.corner_tl = vultures_get_img_src(1, 1, 23, 23, image);
		vultures_winelem.border_top = vultures_get_img_src(27, 1, 57, 23, image);
		vultures_winelem.corner_tr = vultures_get_img_src(61, 1, 84, 23, image);
		vultures_winelem.border_left = vultures_get_img_src(1, 27, 23, 54, image);
		vultures_winelem.center = vultures_get_img_src(141, 1, 238, 168, image);
		vultures_winelem.border_right = vultures_get_img_src(61, 27, 84, 54, image);
		vultures_winelem.corner_bl = vultures_get_img_src(1, 58, 23, 82, image);
		vultures_winelem.border_bottom = vultures_get_img_src(27, 58, 57, 82, image);
		vultures_winelem.corner_br = vultures_get_img_src(61, 58, 84, 82, image);
		vultures_winelem.checkbox_off = vultures_get_img_src(1, 107, 17, 123, image);
		vultures_winelem.checkbox_on = vultures_get_img_src(21, 107, 37, 123, image);
		vultures_winelem.checkbox_count = vultures_get_img_src(21, 127, 37, 143, image);
		vultures_winelem.radiobutton_off = vultures_get_img_src(41, 107, 57, 123, image);
		vultures_winelem.radiobutton_on = vultures_get_img_src(61, 107, 77, 123, image);
		vultures_winelem.scrollbar = vultures_get_img_src(81, 107, 97, 123, image);
		vultures_winelem.scrollbutton_down = vultures_get_img_src(101, 107, 117, 123, image);
		vultures_winelem.scrollbutton_up = vultures_get_img_src(121, 107, 137, 123, image);
		vultures_winelem.scroll_indicator = vultures_get_img_src(1, 127, 17, 154, image);
		vultures_winelem.direction_arrows = vultures_get_img_src(242, 1, 576, 134, image);
		vultures_winelem.invarrow_left = vultures_get_img_src(1, 158, 67, 174, image);
		vultures_winelem.invarrow_right = vultures_get_img_src(1, 178, 67, 194, image);
		vultures_winelem.closebutton = vultures_get_img_src(41, 127, 59, 145, image);
		SDL_FreeSurface(image);
	}

	/* Initialize the isometric tiles */
	all_ok &= vultures_init_map();  /* Initialize tile bitmaps */

	vultures_load_gametiles();


	/* make sure the cursor is not NULL when we try to draw for the first time */
	vultures_mouse_init();

	vultures_write_log(V_LOG_DEBUG, __FILE__, __LINE__, "Vulture's window system ready.\n");

	vultures_set_draw_region(0, 0, vultures_screen->w, vultures_screen->h);
	SDL_FillRect(vultures_screen, NULL, CLR32_BLACK);

	return all_ok;
}



void vultures_destroy_graphics(void)
{
	int i;

	vultures_destroy_map();

	/* free tilearrays, related data */
	vultures_unload_gametiles();

	/* clean up fonts */
	vultures_free_fonts();

	/* unload mouse & tooltip backgrounds, tooltip text and tootltip surface */
	vultures_mouse_destroy();
	
	vultures_eventstack_destroy();

	/* free window elements */
	SDL_FreeSurface(vultures_winelem.corner_tl);
	SDL_FreeSurface(vultures_winelem.border_top);
	SDL_FreeSurface(vultures_winelem.corner_tr);
	SDL_FreeSurface(vultures_winelem.border_left);
	SDL_FreeSurface(vultures_winelem.center);
	SDL_FreeSurface(vultures_winelem.border_right);
	SDL_FreeSurface(vultures_winelem.corner_bl);
	SDL_FreeSurface(vultures_winelem.border_bottom);
	SDL_FreeSurface(vultures_winelem.corner_br);
	SDL_FreeSurface(vultures_winelem.checkbox_off);
	SDL_FreeSurface(vultures_winelem.checkbox_on);
	SDL_FreeSurface(vultures_winelem.checkbox_count);
	SDL_FreeSurface(vultures_winelem.radiobutton_off);
	SDL_FreeSurface(vultures_winelem.radiobutton_on);
	SDL_FreeSurface(vultures_winelem.scrollbar);
	SDL_FreeSurface(vultures_winelem.scrollbutton_down);
	SDL_FreeSurface(vultures_winelem.scrollbutton_up);
	SDL_FreeSurface(vultures_winelem.scroll_indicator);
	SDL_FreeSurface(vultures_winelem.direction_arrows);
	SDL_FreeSurface(vultures_winelem.invarrow_left);
	SDL_FreeSurface(vultures_winelem.invarrow_right);
	SDL_FreeSurface(vultures_winelem.closebutton);

	/* free sound descriptions */
	for (i = 0; i < vultures_n_event_sounds; i++)
	{
		free (vultures_event_sounds[i]->searchpattern);
		free (vultures_event_sounds[i]->filename);
		free (vultures_event_sounds[i]);
	}
	free (vultures_event_sounds);

	/* misc small stuff */
	free (vultures_px_format);
}


