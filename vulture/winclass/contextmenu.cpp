/* NetHack may be freely redistributed.  See license for details. */

#include "vultures_gra.h"
#include "vultures_sdl.h"
#include "vultures_win.h"
#include "vultures_txt.h"
#include "vultures_mou.h"
#include "vultures_tile.h"

#include "contextmenu.h"
#include "button.h"


contextmenu::contextmenu(window *p) : window(p)
{
	v_type = V_WINTYPE_CONTEXTMENU;
	layout_done = false;
	autobg = true;
}


bool contextmenu::draw()
{
	int pos_x, pos_y;
	int x, y;
	
	if (!layout_done)
		layout();

	x = abs_x;
	y = abs_y;

	/* Draw center area */
	vultures_set_draw_region(abs_x, abs_y, abs_x + w - 1, abs_y + h - 1);
	pos_y = y;
	while (pos_y <= y + h) {
		pos_x = x;
		while (pos_x <= x + w) {
			vultures_put_img(pos_x, pos_y, vultures_winelem.center);
			pos_x += vultures_winelem.center->w;
		}
		pos_y += vultures_winelem.center->h;
	}

	/* Draw black border */
	vultures_rect(x, y, x+w-1, y+h-1, 0);

	/* Draw edges (raised) */
	vultures_line(x+1,   y+1,   x+w-3, y+1,   CLR32_GRAY20);
	vultures_line(x+1,   y+h-2, x+w-2, y+h-2, CLR32_GRAY70);
	vultures_line(x+w-2, y+1,   x+w-2, y+h-2, CLR32_GRAY77);
	vultures_line(x+1,   y+1,   x+1,   y+h-3, CLR32_GRAY20);

	vultures_set_draw_region(0, 0, vultures_screen->w-1, vultures_screen->h-1);

	vultures_invalidate_region(x, y, w, h);

	return true;
}


/* arrange the btns on a dropdown (context menu) */
void contextmenu::layout(void)
{
	button *btn;
	point mouse;
	int lineheight;
	int btn_maxwidth = 0;
	
	if (layout_done)
		return;

	mouse = vultures_get_mouse_pos();

	h = 3;
	lineheight = vultures_get_lineheight(V_FONT_BUTTON);

	/* calculate the hight and find the max btn width */
	for (itemlist::iterator i = items.begin(); i != items.end(); i++) {
		btn = new button(this, i->first, i->second, '\0');
		btn->h = lineheight + 10;
		btn->y = h;
		btn->x = 3;
		h += btn->h + 2;

		btn->w = vultures_text_length(V_FONT_BUTTON, btn->caption) + 11;
		btn_maxwidth = (btn_maxwidth > btn->w) ? btn_maxwidth : btn->w;
	}
	h += 2;

	/* set the width of every btn to the max btn width */
	for (window *win = first_child; win; win = win->sib_next)
		win->w = btn_maxwidth;

	w = btn_maxwidth + 6;

	/* set dropdown position */
	if (mouse.x < parent->w - w)
		x = mouse.x;
	else
		x = parent->w - w;

	if (mouse.y < parent->h - h)
		y = mouse.y;
	else
		y = parent->h - h;

	abs_x = x + parent->abs_x;
	abs_y = y + parent->abs_y;
	
	layout_done = true;
}


eventresult contextmenu::handle_mousemotion_event(window* target, void* result, int xrel, 
                                             int yrel, int state)
{
	vultures_set_mcursor(V_CURSOR_NORMAL);
	return V_EVENT_HANDLED_NOREDRAW;
}


eventresult contextmenu::handle_mousebuttonup_event(window* target, void* result,
                                            int mouse_x, int mouse_y, int button, int state)
{
	if (button == SDL_BUTTON_LEFT) {
		if (this != target && target->menu_id) {
			*(int*)result = target->menu_id;
			return V_EVENT_HANDLED_FINAL;
		}
	}

	*(int*)result = 0;
	return V_EVENT_HANDLED_FINAL;
}


eventresult contextmenu::handle_keydown_event(window* target, void* result, SDL_keysym keysym)
{
	*(int*)result = 0;
	return V_EVENT_HANDLED_FINAL;
}


void contextmenu::add_item(const char *label, int menuid)
{
	items.push_back(std::pair<char*, int>(strdup(label), menuid));
}