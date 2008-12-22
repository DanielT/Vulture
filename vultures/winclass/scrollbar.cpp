/* Copyright (c) Daniel Thaler, 2008                              */
/* NetHack may be freely redistributed.  See license for details. */

#include "vultures_gra.h"
#include "vultures_sdl.h"
#include "vultures_win.h"

#include "scrollbar.h"
#include "hotspot.h"


scrollbar::scrollbar(window *p, int scrolloff) : window(p)
{
	v_type = V_WINTYPE_SCROLLBAR;
	scrollpos = 0;
}

bool scrollbar::draw()
{
	int pos_y, scroll_pos;
	int scrollarea_top = abs_y + vultures_winelem.scrollbutton_up->h;
	int scrollarea_bottom = abs_y + h - vultures_winelem.scrollbutton_down->h;

	/* draw top & bottom buttons */
	vultures_put_img(abs_x, abs_y, vultures_winelem.scrollbutton_up);
	vultures_put_img(abs_x, scrollarea_bottom, vultures_winelem.scrollbutton_down);

	/* draw the scrollbar backgound */
	vultures_set_draw_region(abs_x, scrollarea_top, abs_x +
						vultures_winelem.scrollbar->w - 1, scrollarea_bottom - 1);
	for (pos_y = scrollarea_top; pos_y < scrollarea_bottom; pos_y += vultures_winelem.scrollbar->h)
		vultures_put_img(abs_x, pos_y, vultures_winelem.scrollbar);

	vultures_set_draw_region(0, 0, vultures_screen->w - 1, vultures_screen->h - 1);

	/* draw scroll indicator */

	/* for menus that are only _very_ slightly longer than V_MENU_MAXHEIGHT
	* failing to consider spacing between menu elements will lead to scrollpos values
	* that are substantially larger than 8192 (100%). That's OK for the main
	* menu area (desired even), but not for the scroll indicator */
	scroll_pos = (scrollpos <= 8192) ? scrollpos : 8192;

	pos_y = scrollarea_top + ((scrollarea_bottom - scrollarea_top -
			vultures_winelem.scroll_indicator->h) * scroll_pos) / 8192.0;
	vultures_put_img(abs_x, pos_y, vultures_winelem.scroll_indicator);

	vultures_invalidate_region(abs_x, abs_y, w, h);

	return false;
}
