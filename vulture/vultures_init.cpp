/* NetHack may be freely redistributed.  See license for details. */

#include <vector>
#include <string>


#include <errno.h>

#include "vultures_win.h"
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

#include "winclass/introwin.h"

#include "date.h" /* this is in <variant>/include it's needed for VERSION_ID */

static void vultures_show_intro(std::string introscript_name);


/*----------------------------
* function implementaions
*---------------------------- */


void vultures_show_logo_screen(void)
{
	SDL_Event event;
	SDL_Surface *logo;

	vultures_play_event_sound("nhfe_music_main_title");

	if (iflags.wc_splash_screen)
	{
		logo = vultures_load_graphic(V_FILENAME_NETHACK_LOGO);
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
  std::string filename;

	SDL_FillRect(vultures_screen, NULL, CLR32_BLACK);
	logo = vultures_load_graphic(V_FILENAME_CHARACTER_GENERATION);
	if (logo != NULL) {
		vultures_put_img((vultures_screen->w - logo->w) / 2, (vultures_screen->h - logo->h) / 2, logo);
		SDL_FreeSurface(logo);
	}

	vultures_fade_in(0.2);

	vultures_player_selection_internal();

	/* Success! Show introduction. */
	vultures_fade_out(0.2);

	filename = vultures_make_filename(V_CONFIG_DIRECTORY, "", V_FILENAME_INTRO_SCRIPT);

	if (flags.legacy)
		vultures_show_intro(filename);
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


static void vultures_show_intro(std::string introscript_name)
{
	FILE *f;
	char buffer[1024];
	unsigned int nr_scenes, lineno;
  std::string line;
  std::vector<std::string> imagenames;
  std::vector< std::vector<std::string> > subtitles;
	introwin *iw;
    int dummy;

	/* read intro script */
	f = fopen(introscript_name.c_str(), "rb");
	if (f == NULL) {
		vultures_write_log(V_LOG_NOTE, NULL, 0, "intro script %s not found\n",
		                   introscript_name.c_str());
		return;
	}
	
	lineno = nr_scenes = 0;
	while (fgets(buffer, 1024, f)) {
		lineno++;
		
		if (buffer[0] == '%') {
			/* new scene */
			line = std::string(&buffer[1]);
			trim(line);
			
			if (line.length() == 0)
				continue;
			
			nr_scenes++;
			
			if (line.substr(line.length() - 4, 4) != ".png")
				vultures_write_log(V_LOG_NOTE, NULL, 0, "scene image %s not png?", line.c_str());
			
			/* trim off the file extension */
			line = line.substr(0, line.length() - 4);
			
			imagenames.push_back(line);
			subtitles.resize(nr_scenes);
			
		} else {
			/* text lines for current scene */
			if (nr_scenes == 0) {
				vultures_write_log(V_LOG_NOTE, NULL, 0, "subtitle without a preceding scene in line %d of intro script %s\n", lineno, introscript_name.c_str());
				continue;
			}
			
			line = std::string(buffer);
			trim(line);
			
			subtitles[nr_scenes - 1].push_back(line);
		}
	}

	if (nr_scenes == 0) {
		vultures_write_log(V_LOG_NOTE, NULL, 0, "no scenes found in intro script %s\n",
		                   introscript_name.c_str());
		return;
	}
	
	/* display intro */
	iw = new introwin(NULL, imagenames, subtitles);
	vultures_event_dispatcher(&dummy, V_RESPOND_INT, iw);
	delete iw;
}


static void vultures_init_colors()
{
	/* set up the colors used in the game
	* the only good way to do this without needing graphics to have been loaded first
	* is to create a surface here which we then put into display format + alpha ourselves */
	SDL_Surface * pixel = SDL_CreateRGBSurface(SDL_SWSURFACE | SDL_SRCALPHA, 1, 1, 32,
								0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000);
	SDL_Surface * reformatted = SDL_DisplayFormatAlpha(pixel);

	vultures_px_format = new SDL_PixelFormat;
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
  std::string fullname;
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
	if (iflags.wc_font_text) {
		if (access(iflags.wc_font_text, R_OK) == 0) {
			font_loaded = 1;
			font_loaded &= vultures_load_font(V_FONT_SMALL, iflags.wc_font_text, 0, 12);
			font_loaded &= vultures_load_font(V_FONT_LARGE, iflags.wc_font_text, 0, 14);
		}
		else
			printf("Could not access %s: %s\n", iflags.wc_font_text, strerror(errno));
	}

	if (!font_loaded) {/* fallback to default font */
		font_loaded = 1;
		/* add the path to the filename */
		fullname = vultures_make_filename(V_FONTS_DIRECTORY, "", V_FILENAME_FONT);

		font_loaded &= vultures_load_font(V_FONT_SMALL, fullname.c_str(), 0, 12);
		font_loaded &= vultures_load_font(V_FONT_LARGE, fullname.c_str(), 0, 14);
	}

	all_ok &= font_loaded;

	/* Load window style graphics */
	image = vultures_load_graphic(V_FILENAME_WINDOW_STYLE);
	if (image == NULL)
		all_ok = FALSE;
	else {
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

	/* Initialize tile bitmaps */
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
	unsigned int i;

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
	for (i = 0; i < vultures_event_sounds.size(); i++)
		free (vultures_event_sounds[i].searchpattern);

	/* misc small stuff */
	delete vultures_px_format;
}
