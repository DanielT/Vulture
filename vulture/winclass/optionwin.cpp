/* NetHack may be freely redistributed.  See license for details. */

#include "optionwin.h"
#include "menuwin.h"

#include "vulture_win.h"
#include "vulture_gra.h"
#include "vulture_sdl.h"
#include "vulture_txt.h"


optionwin::optionwin(window *p, menuitem* mi, std::string cap, char accel, char group_accel,
                     int glyph, bool selected, bool is_checkbox) : window(p),
                     glyph(glyph), item(mi), is_checkbox(is_checkbox)
{
	v_type = V_WINTYPE_OPTION;
	accelerator = accel;
	group_accelerator = group_accel;
	
	caption = cap;
	menu_id_v = (void*)mi->identifier;
}


bool optionwin::draw()
{
	if (!is_checkbox) {
		if (item->selected)
			vulture_put_img(abs_x, abs_y, vulture_winelem.radiobutton_on);
		else
			vulture_put_img(abs_x, abs_y, vulture_winelem.radiobutton_off);
	}
	/* otherwise we want checkboxes */
	else {
		if (item->selected) {
			/* selected items can be drawn with either an 'x' (count <= 0) or an '#' otherwise */
			if (item->count <= 0)
				vulture_put_img(abs_x, abs_y, vulture_winelem.checkbox_on);
			else
				vulture_put_img(abs_x, abs_y, vulture_winelem.checkbox_count);
		} else
			vulture_put_img(abs_x, abs_y, vulture_winelem.checkbox_off);
	}

	/* draw the option description */
	vulture_put_text_shadow(V_FONT_MENU, caption, vulture_screen,
							abs_x + vulture_winelem.radiobutton_off->w + 4,
							abs_y + 2, V_COLOR_TEXT, V_COLOR_BACKGROUND);

	vulture_invalidate_region(abs_x, abs_y, w, h);

	return false;
}
