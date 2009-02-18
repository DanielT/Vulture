/* NetHack may be freely redistributed.  See license for details. */

extern "C" {
#include "hack.h"
boolean can_advance(int skill, int speedy);
}

#include "vultures_win.h"
#include "vultures_sdl.h"
#include "vultures_gra.h"
#include "vultures_gfl.h"
#include "vultures_mou.h"
#include "vultures_tile.h"

#include "enhancebutton.h"


#define META(c) (0x80 | (c))

enhancebutton *enhancebtn;

enhancebutton::enhancebutton(window *p) : window(p)
{
	image = vultures_load_graphic(V_FILENAME_ENHANCE);
	w = image->w;
	h = image->h;
	
	enhancebtn = this;
	autobg = true;
	visible = false;
}

enhancebutton::~enhancebutton()
{
	SDL_FreeSurface(image);
}

bool enhancebutton::draw()
{
	vultures_set_draw_region(abs_x, abs_y, abs_x + w - 1, abs_y + h - 1);
	vultures_put_img(abs_x, abs_y, image);
	vultures_set_draw_region(0, 0, vultures_screen->w-1, vultures_screen->h-1);

	vultures_invalidate_region(abs_x, abs_y, w, h);

	return true;
}


eventresult enhancebutton::handle_timer_event(window* target, void* result, int time)
{
	if (time > HOVERTIMEOUT)
		vultures_mouse_set_tooltip((char *)"Enhance a skill");
	return V_EVENT_HANDLED_NOREDRAW;
}


eventresult enhancebutton::handle_mousemotion_event(window* target, void* result, int xrel, 
                                             int yrel, int state)
{
	vultures_set_mcursor(V_CURSOR_NORMAL);
	return V_EVENT_HANDLED_NOREDRAW;
}


eventresult enhancebutton::handle_mousebuttonup_event(window* target, void* result,
                                            int mouse_x, int mouse_y, int button, int state)
{
	if (button != SDL_BUTTON_LEFT)
		return V_EVENT_HANDLED_NOREDRAW;
	
	((vultures_event*)result)->num = META('e');
	return V_EVENT_HANDLED_FINAL;
}


eventresult enhancebutton::handle_resize_event(window* target, void* result, int res_w, int res_h)
{
	/* this relies on the fact that the enhance window is created
	 * immediately after the status window */
	y = sib_prev->y - h;
	return V_EVENT_HANDLED_NOREDRAW;
}


/* enable and display the enhance icon if enhance is possible */
void enhancebutton::check_enhance(void)
{
	int to_advance = 0, i;
	bool prev_vis;

	/* check every skill */
	for (i = 0; i < P_NUM_SKILLS; i++)
	{
		if (P_RESTRICTED(i))
			continue;
		if (can_advance(i, FALSE))
			to_advance++;
	}

	prev_vis = visible;

	if (!to_advance && prev_vis)
		hide();
	else if (!visible && to_advance)
		visible = true;
}

