/* NetHack may be freely redistributed.  See license for details. */

#include "vulture_sdl.h" /* XXX this must be the first include,
                             no idea why but it won't compile otherwise */

extern "C" {
#include "hack.h"
}

#include "vulture_types.h"
#include "vulture_win.h"
#include "vulture_gra.h"
#include "vulture_gfl.h"
#include "vulture_txt.h"
#include "vulture_sound.h"

#include "endingwin.h"


endingwin::endingwin(window *p, std::list<menuitem> &menuitems, int how) :
                     menuwin(p, menuitems, 0)
{
	ending_type = how;
}


bool endingwin::draw()
{
	int lines;
	int textpos_x, textpos_y;
	SDL_Surface *image;

	if (!flags.tombstone) {
		for (item_iterator i = items.begin(); i != items.end(); ++i)
			printf("%s\n", i->str.c_str());

		printf("\n\n");
		return false;
	}

	/* make sure the screen is cleared */
	SDL_FillRect(vulture_screen, NULL, CLR32_BLACK);

	switch (ending_type-1)
	{
		case QUIT:
			image = vulture_load_graphic(V_FILENAME_ENDING_QUIT);
			vulture_play_event_sound("nhfe_music_end_quit");
			break;

		case ASCENDED:
			image = vulture_load_graphic(V_FILENAME_ENDING_ASCENDED);
			vulture_play_event_sound("nhfe_music_end_ascended");
			break;

		case ESCAPED:
			image = vulture_load_graphic(V_FILENAME_ENDING_ESCAPED);
			vulture_play_event_sound("nhfe_music_end_ascended");
			break;

		case PANICKED:
			image = NULL;
			break;

		default:
			image = vulture_load_graphic(V_FILENAME_ENDING_DIED);
			vulture_play_event_sound("nhfe_music_end_died");
	}

	if (image != NULL) {
		vulture_put_img((vulture_screen->w - image->w) / 2, (vulture_screen->h - image->h) / 2, image);
		SDL_FreeSurface(image);
		vulture_fade_in(0.5);
	}

	/* Count n. of rows to display */
	lines = items.size();

	/* Add prompt line */
	lines++;

	/* Display the rows */
	textpos_y = vulture_screen->h - (lines+1) * vulture_get_lineheight(V_FONT_INTRO);
	for (item_iterator i = items.begin(); i != items.end(); ++i) {
		textpos_x = (vulture_screen->w - vulture_text_length(V_FONT_INTRO, i->str))/2;
		vulture_put_text_shadow(V_FONT_INTRO, i->str, vulture_screen, textpos_x,
								textpos_y, V_COLOR_INTRO_TEXT, V_COLOR_BACKGROUND);
		textpos_y += vulture_get_lineheight(V_FONT_INTRO);
	}

	textpos_x = (vulture_screen->w - vulture_text_length(V_FONT_INTRO, "(press any key)"))/2;
	vulture_put_text_shadow(V_FONT_INTRO, "(press any key)", vulture_screen, textpos_x,
							textpos_y, V_COLOR_INTRO_TEXT, V_COLOR_BACKGROUND);

	vulture_refresh();
	return false;
}


eventresult endingwin::handle_mousebuttonup_event(window* target, void* result,
                                            int mouse_x, int mouse_y, int button, int state)
{
	return V_EVENT_HANDLED_FINAL;
}


eventresult endingwin::handle_keydown_event(window* target, void* result, int sym, int mod, int unicode)
{
	return V_EVENT_HANDLED_FINAL;
}
