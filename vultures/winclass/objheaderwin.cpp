/* NetHack may be freely redistributed.  See license for details. */

#include "vultures_sdl.h"
#include "vultures_gra.h"
#include "vultures_txt.h"
#include "vultures_win.h"

#include "objheaderwin.h"


objheaderwin::objheaderwin(window *parent, std::string cap) : window(parent)
{
	v_type = V_WINTYPE_OBJITEMHEADER;
	w = V_LISTITEM_WIDTH;
	h = V_LISTITEM_HEIGHT;
	autobg = true;
	caption = cap;
}


bool objheaderwin::draw()
{
	/* constrain drawing to this window */
	vultures_set_draw_region(abs_x, abs_y, abs_x + w - 1, abs_y + h - 1);

	vultures_fill_rect(abs_x + 2, abs_y + 2, abs_x + w - 3, abs_y + h - 3, CLR32_BLACK_A50);

	/* Outer edge */
	vultures_draw_lowered_frame(abs_x, abs_y, abs_x+w-1, abs_y+h-1);
	/* Inner edge */
	vultures_draw_raised_frame(abs_x+1, abs_y+1, abs_x+w-2, abs_y+h-2);

	/* draw the text centered in the window */
	int txt_width = vultures_text_length(V_FONT_MENU, caption);
	int txt_height = vultures_text_height(V_FONT_MENU, caption);
	int text_start_x = abs_x + (w - txt_width)/2;
	int text_start_y = abs_y + (h - txt_height)/2;

	vultures_put_text_shadow(V_FONT_MENU, caption, vultures_screen, text_start_x,
							text_start_y, CLR32_WHITE, CLR32_BLACK);

	vultures_invalidate_region(abs_x, abs_y, w, h);

	/* lift drawing region restriction */
	vultures_set_draw_region(0, 0, vultures_screen->w - 1, vultures_screen->h - 1);

	return 0;
}
