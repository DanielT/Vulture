/* Copyright (c) Daniel Thaler, 2008                              */
/* NetHack may be freely redistributed.  See license for details. */

#include "optionwin.h"
#include "menuwin.h"

#include "vultures_win.h"
#include "vultures_gra.h"
#include "vultures_sdl.h"
#include "vultures_txt.h"


optionwin::optionwin(window *p, menuitem* mi, string cap, char accel, 
                     int glyph, bool selected, bool is_checkbox) : window(p),
                     glyph(glyph), item(mi), is_checkbox(is_checkbox)
{
	v_type = V_WINTYPE_OPTION;
	accelerator = accel;
	
	caption = cap;
	menu_id_v = (void*)mi->identifier;
}


bool optionwin::draw()
{
	if (!is_checkbox) {
		if (item->selected)
			vultures_put_img(abs_x, abs_y, vultures_winelem.radiobutton_on);
		else
			vultures_put_img(abs_x, abs_y, vultures_winelem.radiobutton_off);
	}
	/* otherwise we want checkboxes */
	else {
		if (item->selected) {
			/* selected items can be drawn with either an 'x' (count <= 0) or an '#' otherwise */
			if (item->count <= 0)
				vultures_put_img(abs_x, abs_y, vultures_winelem.checkbox_on);
			else
				vultures_put_img(abs_x, abs_y, vultures_winelem.checkbox_count);
		} else
			vultures_put_img(abs_x, abs_y, vultures_winelem.checkbox_off);
	}

	/* draw the option description */
	vultures_put_text_shadow(V_FONT_MENU, caption, vultures_screen,
							abs_x + vultures_winelem.radiobutton_off->w + 4,
							abs_y + 2, V_COLOR_TEXT, V_COLOR_BACKGROUND);

	vultures_invalidate_region(abs_x, abs_y, w, h);

	return false;
}
