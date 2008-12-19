
extern "C" {
#include "hack.h"
}

#include "vultures_types.h"
#include "vultures_win.h"
#include "vultures_gra.h"
#include "vultures_gfl.h"
#include "vultures_txt.h"
#include "vultures_sdl.h"
#include "vultures_sound.h"

#include "endingwin.h"


endingwin::endingwin(int how) : menuwin()
{
	ending_type = how;
}


bool endingwin::draw()
{
	int lines;
	point textpos;
	SDL_Surface *image;

	if (!flags.tombstone) {
		for (item_iterator i = items.begin(); i != items.end(); ++i)
			printf("%s\n", (*i)->str);

		printf("\n\n");
		return false;
	}

	/* make sure the screen is cleared */
	SDL_FillRect(vultures_screen, NULL, CLR32_BLACK);

	switch (ending_type-1)
	{
		case QUIT:
			image = vultures_load_graphic(NULL, V_FILENAME_ENDING_QUIT);
			vultures_play_event_sound("nhfe_music_end_quit");
			break;

		case ASCENDED:
			image = vultures_load_graphic(NULL, V_FILENAME_ENDING_ASCENDED);
			vultures_play_event_sound("nhfe_music_end_ascended");
			break;

		case ESCAPED:
			image = vultures_load_graphic(NULL, V_FILENAME_ENDING_ESCAPED);
			vultures_play_event_sound("nhfe_music_end_ascended");
			break;

		case PANICKED:
			image = NULL;
			break;

		default:
			image = vultures_load_graphic(NULL, V_FILENAME_ENDING_DIED);
			vultures_play_event_sound("nhfe_music_end_died");
	}

	if (image != NULL) {
		vultures_put_img((vultures_screen->w - image->w) / 2, (vultures_screen->h - image->h) / 2, image);
		SDL_FreeSurface(image);
		vultures_fade_in(0.5);
	}

	/* Count n. of rows to display */
	lines = items.size();

	/* Add prompt line */
	lines++;

	/* Display the rows */
	textpos.y = vultures_screen->h - (lines+1) * vultures_get_lineheight(V_FONT_INTRO);
	for (item_iterator i = items.begin(); i != items.end(); ++i) {
		textpos.x = (vultures_screen->w - vultures_text_length(V_FONT_INTRO, (*i)->str))/2;
		vultures_put_text_shadow(V_FONT_INTRO, (*i)->str, vultures_screen, textpos.x,
								textpos.y, V_COLOR_INTRO_TEXT, V_COLOR_BACKGROUND);
		textpos.y += vultures_get_lineheight(V_FONT_INTRO);
	}

	textpos.x = (vultures_screen->w - vultures_text_length(V_FONT_INTRO, "(press any key)"))/2;
	vultures_put_text_shadow(V_FONT_INTRO, "(press any key)", vultures_screen, textpos.x,
							textpos.y, V_COLOR_INTRO_TEXT, V_COLOR_BACKGROUND);

	vultures_refresh();
	return false;
}


eventresult endingwin::event_handler(window* target, void* result, SDL_Event* event)
{
	switch (event->type)
	{
		case SDL_KEYDOWN:
		case SDL_MOUSEBUTTONUP:
			return V_EVENT_HANDLED_FINAL;
	}
	return V_EVENT_HANDLED_NOREDRAW;
}


menuwin* endingwin::replace_win(menuwin *win)
{
	menuwin::replace_win(win);
	v_type = V_WINTYPE_ENDING;
	nh_type = NHW_TEXT;
	
	return this;
}
