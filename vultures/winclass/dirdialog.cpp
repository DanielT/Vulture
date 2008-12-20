
#include "vultures_win.h"
#include "vultures_gen.h"
#include "vultures_gra.h"
#include "vultures_sdl.h"
#include "vultures_txt.h"
#include "vultures_mou.h"
#include "vultures_tile.h"

#include "dirdialog.h"
#include "hotspot.h"

/* for defs of V_MAP_XMOD and V_MAP_YMOD */
#include "levelwin.h"


dirdialog::dirdialog(window *p, string ques) : mainwin(p)
{
	int arrows_w, arrows_h;

	caption = ques;

	arrows_w = vultures_winelem.direction_arrows->w;
	arrows_h = vultures_winelem.direction_arrows->h;


	/* calculate window layout */
	if (!ques.empty())
		w = vultures_text_length(V_FONT_HEADLINE, ques);
	w = (w > arrows_w) ? w : arrows_w;
	w += border_left + border_right;

	arroffset_y = border_top + vultures_get_lineheight(V_FONT_HEADLINE) * 1.5;
	arroffset_x = (w - arrows_w) / 2;

	h = arroffset_y + arrows_h + border_bottom;

	x = (parent->get_w() - w) / 2;
	y = (parent->get_h() - h) / 2;
	abs_x = parent->abs_x + x;
	abs_y = parent->abs_y + y;

	arr = new hotspot(this, arroffset_x, arroffset_y, arrows_w, arrows_h, 0, "");
}


eventresult dirdialog::event_handler(window* target, void* result, SDL_Event* event)
{
	point mouse;
	int dir_x, dir_y;
	char choice = 0;

	const char dirkeys[3][3] = {{'8', '9', '6'},
								{'7', '.', '3'},
								{'4', '1', '2'}};

	/* mouse motion events are merely used to set the correct cursor */
	if (event->type == SDL_MOUSEMOTION)
		vultures_set_mcursor(V_CURSOR_NORMAL);

	/* any key is accepted as a valid choice */
	else if (event->type == SDL_KEYDOWN)
	{
		if (event->key.keysym.sym == SDLK_ESCAPE)
			choice = -1;
		else
			choice = vultures_convertkey_sdl2nh(&event->key.keysym);
	}

	/* mouse click events: only left clicks on the arrow grid are accepted */
	else if (event->type == SDL_MOUSEBUTTONUP && event->button.button == SDL_BUTTON_LEFT)
	{
		if (target == first_child)
		{
			/* get the click coordinates and normalize them to the center of the arrow grid */
			mouse = vultures_get_mouse_pos();
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
				choice = vultures_numpad_to_hjkl(dirkeys[dir_y + 1][dir_x + 1], 0);

			if (dir_x >= 2 && mouse.x < target->w / 2 && mouse.y < target->h / 2)
				choice = '>';

			if (dir_x <= -2 && mouse.x > -target->w / 2 && mouse.y > -target->h / 2)
				choice = '<';
		}
	}
	else if (event->type == SDL_VIDEORESIZE)
	{
		x = (parent->w - w) / 2;
		y = (parent->h - h) / 2;
		return V_EVENT_HANDLED_NOREDRAW;
	}

	if (choice)
	{
		if (!vultures_winid_map)
			choice = vultures_translate_key(choice);
		*(char*)result = choice;
		return V_EVENT_HANDLED_FINAL;
	}

	return V_EVENT_HANDLED_NOREDRAW;
}


bool dirdialog::draw()
{
	mainwin::draw();
	
	vultures_set_draw_region(arr->abs_x, arr->abs_y, 
	                         arr->abs_x + arr->w - 1, arr->abs_y + arr->h - 1);
	vultures_put_img(arr->abs_x, arr->abs_y, vultures_winelem.direction_arrows);
	vultures_set_draw_region(0, 0, vultures_screen->w - 1, vultures_screen->h - 1);

	vultures_invalidate_region(abs_x, abs_y, w, h);

	return false;
}