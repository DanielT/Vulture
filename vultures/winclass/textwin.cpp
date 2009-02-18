/* NetHack may be freely redistributed.  See license for details. */

#include "vultures_gra.h"
#include "vultures_txt.h"
#include "vultures_win.h"
#include "vultures_sdl.h"

#include "textwin.h"


textwin::textwin(window *p, string cap) : window(p)
{
	v_type = V_WINTYPE_TEXT;
	caption = cap;
	is_input = false;
	textcolor = V_COLOR_TEXT;
	autobg = true;
	w = vultures_text_length(V_FONT_MENU, caption) + 10;
}

textwin::textwin(window *p, int destsize) : window(p)
{
	v_type = V_WINTYPE_TEXT;
	caption = "";
	
	is_input = true;
	textcolor = V_COLOR_TEXT;
	autobg = true;
}


bool textwin::draw()
{
	int textlen = 0;

	if (background) {
		/* the text might have changed, so redraw the background if there is one */
		vultures_put_img(abs_x, abs_y, background);
		vultures_invalidate_region(abs_x-2, abs_y, background->w+4, background->h);
	}

	vultures_put_text_shadow(V_FONT_MENU, caption, vultures_screen,
	                         abs_x, abs_y, textcolor, V_COLOR_BACKGROUND);

	textlen = vultures_text_length(V_FONT_MENU, caption);


	if (is_input)
		/* draw prompt */
		vultures_rect(abs_x + textlen + 1, abs_y, abs_x + textlen + 1,
				abs_y + h - 2, V_COLOR_TEXT);

	vultures_invalidate_region(abs_x-2, abs_y, w+4, h);

	return false;
}


void textwin::set_caption(string str)
{
	window::set_caption(str);
	w = vultures_text_length(V_FONT_MENU, caption) + 10;
}
