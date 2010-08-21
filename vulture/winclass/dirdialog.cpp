/* NetHack may be freely redistributed.  See license for details. */

#include "vulture_win.h"
#include "vulture_gen.h"
#include "vulture_gra.h"
#include "vulture_sdl.h"
#include "vulture_txt.h"
#include "vulture_mou.h"
#include "vulture_tile.h"

#include "dirdialog.h"
#include "hotspot.h"
#include "map.h"

/* for defs of V_MAP_XMOD and V_MAP_YMOD */
#include "levelwin.h"


dirdialog::dirdialog(window *p, std::string ques) : mainwin(p)
{
	int arrows_w, arrows_h;

	caption = ques;

	arrows_w = vulture_winelem.direction_arrows->w;
	arrows_h = vulture_winelem.direction_arrows->h;


	/* calculate window layout */
	if (!ques.empty())
		w = vulture_text_length(V_FONT_HEADLINE, ques);
	w = (w > arrows_w) ? w : arrows_w;
	w += border_left + border_right;

	arroffset_y = border_top + vulture_get_lineheight(V_FONT_HEADLINE) * 1.5;
	arroffset_x = (w - arrows_w) / 2;

	h = arroffset_y + arrows_h + border_bottom;

	x = (parent->get_w() - w) / 2;
	y = (parent->get_h() - h) / 2;
	abs_x = parent->abs_x + x;
	abs_y = parent->abs_y + y;

	arr = new hotspot(this, arroffset_x, arroffset_y, arrows_w, arrows_h, 0, "");
}


eventresult dirdialog::handle_mousemotion_event(window* target, void* result, 
                                                int xrel, int yrel, int state)
{
	vulture_set_mcursor(V_CURSOR_NORMAL);
	return V_EVENT_HANDLED_NOREDRAW;
}


eventresult dirdialog::handle_mousebuttonup_event(window* target, void* result,
                                            int mouse_x, int mouse_y, int button, int state)
{
	point mouse;
	int dir_x, dir_y;
	char choice = 0;
	const char dirkeys[3][3] = {{'8', '9', '6'},
								{'7', '.', '3'},
								{'4', '1', '2'}};
	
	if (button != SDL_BUTTON_LEFT || target != first_child)
		return V_EVENT_HANDLED_NOREDRAW;

	/* get the click coordinates and normalize them to the center of the arrow grid */
	mouse = vulture_get_mouse_pos();
	mouse.x -= (target->abs_x + target->w/2);
	mouse.y -= (target->abs_y + target->h/2);

	/* translate the click position to a direction */
	dir_x = V_MAP_YMOD * mouse.x + V_MAP_XMOD * mouse.y + V_MAP_XMOD*V_MAP_YMOD;
	dir_x = dir_x / (2 * V_MAP_XMOD * V_MAP_YMOD) - (dir_x < 0);
	dir_y = -V_MAP_YMOD * mouse.x + V_MAP_XMOD * mouse.y + V_MAP_XMOD * V_MAP_YMOD;
	dir_y = dir_y / (2 * V_MAP_XMOD * V_MAP_YMOD) - (dir_y < 0);

	/* convert the chosen direction to a key */
	choice = 0;
	if (dir_y >= -1 && dir_y <= 1 && dir_x >= -1 && dir_x <= 1)
		choice = vulture_numpad_to_hjkl(dirkeys[dir_y + 1][dir_x + 1], 0);

	if (dir_x >= 2 && mouse.x < target->w / 2 && mouse.y < target->h / 2)
		choice = '>';

	if (dir_x <= -2 && mouse.x > -target->w / 2 && mouse.y > -target->h / 2)
		choice = '<';

	if (choice) {
		if (!mapwin)
			choice = vulture_translate_key(choice);
		*(char*)result = choice;
		return V_EVENT_HANDLED_FINAL;
	}
	
	return V_EVENT_HANDLED_NOREDRAW;
}


eventresult dirdialog::handle_keydown_event(window* target, void* result,
                                            int sym, int mod, int unicode)
{
	char choice = 0;

	if (sym == SDLK_ESCAPE)
		choice = -1;
	else
		choice = vulture_make_nh_key(sym, mod, unicode);

	if (!mapwin)
		choice = vulture_translate_key(choice);
	*(char*)result = choice;
	return V_EVENT_HANDLED_FINAL;
}


eventresult dirdialog::handle_resize_event(window* target, void* result, int res_w, int res_h)
{
	x = (parent->w - w) / 2;
	y = (parent->h - h) / 2;
	return V_EVENT_HANDLED_NOREDRAW;
}


bool dirdialog::draw()
{
	mainwin::draw();
	
	vulture_set_draw_region(arr->abs_x, arr->abs_y, 
	                         arr->abs_x + arr->w - 1, arr->abs_y + arr->h - 1);
	vulture_put_img(arr->abs_x, arr->abs_y, vulture_winelem.direction_arrows);
	vulture_set_draw_region(0, 0, vulture_screen->w - 1, vulture_screen->h - 1);

	vulture_invalidate_region(abs_x, abs_y, w, h);

	return false;
}
