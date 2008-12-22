/* Copyright (c) Daniel Thaler, 2008                              */
/* NetHack may be freely redistributed.  See license for details. */

#include "vultures_win.h"
#include "vultures_gra.h"
#include "vultures_sdl.h"
#include "vultures_txt.h"

#include "button.h"


button::button(window *p, string caption, int menuid, char accel) : window(p)
{
	v_type = V_WINTYPE_BUTTON;

	this->caption = caption;

	menu_id = menuid;
	accelerator = accel;
	
	image = NULL;
	selected = false;
	autobg = true;

	w = vultures_text_length(V_FONT_MENU, caption) + 14;
	h = vultures_text_height(V_FONT_MENU, caption) + 10;
}


button::~button()
{
	if (image)
		SDL_FreeSurface(image);
	image = NULL;
}


bool button::draw()
{
	int x = abs_x;
	int y = abs_y;

	if (background)
		/* re-draw background: if the button just became un-pressed
		* we get messed up graphics otherwise */
		vultures_put_img(x, y, background);

	int text_start_x, text_start_y;

	/* Black edge */
	vultures_rect(x+1, y+1, x+w-2, y+h-2, V_COLOR_BACKGROUND);

	/* Outer edge (lowered) */
	vultures_draw_lowered_frame(x, y, x + w - 1, y + h - 1);
	/* Inner edge (raised) */
	vultures_draw_raised_frame(x + 2, y + 2, x + w - 3, y + h - 3);

	if (!caption.empty()) {
		text_start_x = x + (w - vultures_text_length(V_FONT_BUTTON, caption))/2;
		text_start_y = y + 5;

		vultures_put_text_shadow(V_FONT_BUTTON, caption, vultures_screen, text_start_x,
								text_start_y, V_COLOR_TEXT, V_COLOR_BACKGROUND);
	} else if (image) {
		vultures_put_img(x + (w - image->w) / 2, y + (h - image->h)/2, image);
	}

	if (selected) {
		/* shift the *entire* image of the button (including borders)
		* 2px left and down */
		SDL_Surface *buttonimage = vultures_get_img(x, y, x+w-3, y+h-3);

		vultures_fill_rect(x, y, x + w - 1, y + 1, CLR32_BLACK);
		vultures_fill_rect(x, y, x + 1, y + h - 1, CLR32_BLACK);      
		vultures_put_img(x + 2, y + 2, buttonimage);

		SDL_FreeSurface(buttonimage);
	}

	vultures_invalidate_region(x, y, w, h);

	return false;
}


eventresult button::handle_mousebuttonup_event(window* target, void* result,
                                            int mouse_x, int mouse_y, int button, int state)
{
	if (selected)
		need_redraw = 1;
	selected = 0;
	return V_EVENT_UNHANDLED_REDRAW;
}


eventresult button::handle_other_event(window* target, void* result, SDL_Event *event)
{
	if (this == target && event->type == SDL_MOUSEBUTTONDOWN &&
	    event->button.button == SDL_BUTTON_LEFT) {
		selected = 1;
		need_redraw = 1;
	} else if (event->type == SDL_MOUSEMOVEOUT) {
		if (selected)
			need_redraw = 1;
		selected = 0;
	}
	
	if (need_redraw)
		return V_EVENT_UNHANDLED_REDRAW;

	return V_EVENT_UNHANDLED;
}
