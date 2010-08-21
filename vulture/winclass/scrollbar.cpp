/* NetHack may be freely redistributed.  See license for details. */

#include "vulture_gra.h"
#include "vulture_sdl.h"
#include "vulture_win.h"

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
	int scrollarea_top = abs_y + vulture_winelem.scrollbutton_up->h;
	int scrollarea_bottom = abs_y + h - vulture_winelem.scrollbutton_down->h;

	/* draw top & bottom buttons */
	vulture_put_img(abs_x, abs_y, vulture_winelem.scrollbutton_up);
	vulture_put_img(abs_x, scrollarea_bottom, vulture_winelem.scrollbutton_down);

	/* draw the scrollbar backgound */
	vulture_set_draw_region(abs_x, scrollarea_top, abs_x +
						vulture_winelem.scrollbar->w - 1, scrollarea_bottom - 1);
	for (pos_y = scrollarea_top; pos_y < scrollarea_bottom; pos_y += vulture_winelem.scrollbar->h)
		vulture_put_img(abs_x, pos_y, vulture_winelem.scrollbar);

	vulture_set_draw_region(0, 0, vulture_screen->w - 1, vulture_screen->h - 1);

	/* draw scroll indicator */

	/* for menus that are only _very_ slightly longer than V_MENU_MAXHEIGHT
	* failing to consider spacing between menu elements will lead to scrollpos values
	* that are substantially larger than 8192 (100%). That's OK for the main
	* menu area (desired even), but not for the scroll indicator */
	scroll_pos = (scrollpos <= 8192) ? scrollpos : 8192;

	pos_y = scrollarea_top + ((scrollarea_bottom - scrollarea_top -
			vulture_winelem.scroll_indicator->h) * scroll_pos) / 8192.0;
	vulture_put_img(abs_x, pos_y, vulture_winelem.scroll_indicator);

	vulture_invalidate_region(abs_x, abs_y, w, h);

	return false;
}
