/* NetHack may be freely redistributed.  See license for details. */

#include "mainwin.h"

#include "vultures_gra.h"
#include "vultures_win.h"
#include "vultures_sdl.h"
#include "vultures_txt.h"

#define min(a, b) (((a) < (b)) ? (a) : (b))
#define max(a, b) (((a) > (b)) ? (a) : (b))


mainwin::mainwin(window *p) : window(p)
{
	border_right = vultures_winelem.border_right->w;
	border_left = vultures_winelem.border_left->w;
	border_top = vultures_winelem.border_top->h;
	border_bottom = vultures_winelem.border_bottom->h;
	autobg = true;
}


bool mainwin::draw()
{
	int x = this->abs_x;
	int y = this->abs_y;
	int pos_x, pos_y;

	/* Draw corners */
	vultures_put_img(x, y, vultures_winelem.corner_tl);
	vultures_put_img(x + w - border_right, y, vultures_winelem.corner_tr);
	vultures_put_img(x, y + h - border_bottom, vultures_winelem.corner_bl);
	vultures_put_img(x + w - border_right, y + h - border_bottom, vultures_winelem.corner_br);

	/* Draw top border */
	vultures_set_draw_region(x + border_left, y, x + w - border_right, y + border_top);
	pos_x = x + border_left;

	while (pos_x <= x + w - border_right) {
		vultures_put_img(pos_x, y, vultures_winelem.border_top);
		pos_x += vultures_winelem.border_top->w;
	}

	/* Draw bottom border */
	vultures_set_draw_region(x + border_left, y + h - border_bottom,
						x + w - border_right, y + h);
	pos_x = x + border_left;

	while (pos_x <= x+this->w-vultures_winelem.border_right->w) {
		vultures_put_img(pos_x, y + h - border_bottom, vultures_winelem.border_bottom);
		pos_x += vultures_winelem.border_bottom->w;
	}

	/* Draw left border */
	vultures_set_draw_region(x, y + border_top, x + border_left,
						y + h - border_bottom);
	pos_y = y + border_top;

	while (pos_y <= y + h - border_bottom) {
		vultures_put_img(x, pos_y, vultures_winelem.border_left);
		pos_y += vultures_winelem.border_left->h;
	}

	/* Draw right border */
	vultures_set_draw_region(x + w - border_right, y + border_top,
						x + w, y + h - border_bottom);
	pos_y = y + border_top;

	while (pos_y <= y + h - border_bottom) {
		vultures_put_img(x + w - border_right, pos_y, vultures_winelem.border_right);
		pos_y += vultures_winelem.border_right->h;
	}

	/* Draw center area */
	vultures_set_draw_region(x + border_left, y + border_top,
						x + w - border_right, y + h - border_bottom);
	pos_y = y + border_top;
	while (pos_y <= y + h - border_bottom) {
		pos_x = x + border_left;

		while (pos_x <= x + w - border_right) {
			vultures_put_img(pos_x, pos_y, vultures_winelem.center);
			pos_x += vultures_winelem.center->w;
		}
		pos_y += vultures_winelem.center->h;
	}
	vultures_set_draw_region(0, 0, vultures_screen->w-1, vultures_screen->h-1);


	/* draw title */
	pos_x = this->abs_x + border_left;
	pos_y = this->abs_y + border_top;

	if (!caption.empty())
		vultures_put_text_shadow(V_FONT_HEADLINE, caption, vultures_screen, pos_x, 
								pos_y, V_COLOR_TEXT, V_COLOR_BACKGROUND);

	vultures_invalidate_region(x, y, w, h);

	return true;
}


int mainwin::get_frameheight()
{
	int capheight = 0;
	if (!caption.empty())
		capheight = vultures_text_height(V_FONT_HEADLINE, caption);
	return border_top + capheight + border_bottom;
}


void mainwin::layout()
{
	window *winelem;
	int btncount, btn_maxwidth, btn_totalwidth, pos_x, capheight;
	int max_y, min_y, offset_top;
	int max_x = vultures_text_length(V_FONT_HEADLINE, caption);
	int buttonheight = vultures_get_lineheight(V_FONT_MENU) + 15;
	
	capheight = 0;
	if (!caption.empty())
		capheight = vultures_text_height(V_FONT_HEADLINE, caption) * 2;
	
	btncount = btn_totalwidth = btn_maxwidth = pos_x = 0;
	max_y = capheight;
	min_y = 9999;
	
	for (winelem = first_child; winelem != NULL; winelem = winelem->sib_next) {
		if (winelem->get_wintype() == V_WINTYPE_BUTTON) {
			btncount++;
			btn_maxwidth = max(btn_maxwidth, winelem->w);
		} else {
			max_x = max(winelem->x + winelem->w, max_x);
			max_y = max(winelem->y + winelem->h, max_y);
			min_y = min(winelem->y, min_y);
		}
	}

	/* if there are any element in the space needed for the headline add extra
	 * space at the top of the window */
	offset_top = 0;
	if (min_y < capheight)
		offset_top = capheight;

	/* make the window wide enough to fit all the buttons in */
	btn_totalwidth = (btn_maxwidth + 10) * btncount - 10;
	max_x = max(max_x, btn_totalwidth);
	max_y += offset_top;
	
	/* assign positions */
	pos_x = max((max_x - btn_totalwidth) / 2, 0);
	for (winelem = first_child; winelem != NULL; winelem = winelem->sib_next) {
		if (winelem->get_wintype() == V_WINTYPE_BUTTON) {
			winelem->w = btn_maxwidth;
			winelem->x = pos_x + border_left;
			winelem->y = max_y + border_top;
			pos_x += winelem->w + 10;
		} else {
			winelem->x += border_left;
			winelem->y += border_top + offset_top;
		}
	}
	
	max_y += buttonheight;
	w = max_x + border_left + border_right;
	h = max_y + border_top + border_bottom;
	
	x = (parent->w - w) / 2;
	y = (parent->h - h) / 2;
	abs_x = parent->abs_x + x;
	abs_y = parent->abs_y + y;
}
