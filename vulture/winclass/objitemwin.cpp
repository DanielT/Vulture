/* NetHack may be freely redistributed.  See license for details. */

#include "vulture_win.h"
#include "vulture_gra.h"
#include "vulture_sdl.h"
#include "vulture_mou.h"
#include "vulture_txt.h"
#include "vulture_tile.h"

#include "menuwin.h"
#include "objitemwin.h"


objitemwin::objitemwin(window *p, menuitem* mi, std::string cap, char accel, char group_accel,
                       int glyph, bool selected, bool multiselect) : 
                       optionwin(p, mi, cap, accel, group_accel, glyph, selected, multiselect)
{
	v_type = V_WINTYPE_OBJITEM;
	last_toggled = false;
	hover = false;
	autobg = true;
	w = V_LISTITEM_WIDTH;
	h = V_LISTITEM_HEIGHT;
	obj = NULL;
}


bool objitemwin::draw()
{
	char tmpstr[32];
	int text_start_x, text_start_y, txt_height;
	int x = abs_x;
	int y = abs_y;
	int weight = 0;
	Uint32 textcolor;

	vulture_set_draw_region(x, y, x + w - 1, y + h - 1);

	/* re-set the background to prevent shadings from stacking repatedly until they become solid */
	if (background)
		vulture_put_img(x, y, background);


	/* hovering gives an item a light blue frame */
	if (hover)
		vulture_rect(x+1, y+1, x+w-2, y+h-2, CLR32_BLESS_BLUE);

	/* otherwise, if it is selected, the item has an orange frame */
	else if (item->selected)
		vulture_rect(x+1, y+1, x+w-2, y+h-2, CLR32_ORANGE);

	/* all other items appear etched */
	else
	{
		/* draw the outer edge of the frame */
		vulture_draw_lowered_frame(x, y, x+w-1, y+h-1);
		/* Inner edge */
		vulture_draw_raised_frame(x+1, y+1, x+w-2, y+h-2);
	}

	/* the item that was toggled last has a white outer frame to indicate it's special status */
	if (last_toggled)
		vulture_rect(x, y, x+w-1, y+h-1, CLR32_WHITE);


	/* selected items also have yellow background shading */
	if (item->selected)
		vulture_fill_rect(x+h-1, y+2, x+w-3, y+h-3, CLR32_GOLD_SHADE);


	/* use a different text color for worn objects */
	if (obj && obj->owornmask)
		textcolor = CLR32_LIGHTGREEN;
	else
		textcolor = CLR32_WHITE;


	/* draw text, leaving a h by h square on the left free for the object tile */
	/* line 1 and if necessary line 2 contain the item description */
	vulture_put_text_multiline(V_FONT_MENU, caption, vulture_screen, x + h,
								y + 3, textcolor, CLR32_BLACK, w - h - 6);

  if ( obj ) {
    weight = obj->owt;
  }

	/* weight is in line 3 */
	txt_height = vulture_text_height(V_FONT_MENU, caption);
	text_start_y = y + txt_height*2 + 4;

	/* draw the object weight */
	tmpstr[0] = '\0';
	if (weight)
		snprintf(tmpstr, 32, "w: %d", weight);
	text_start_x = x + (w - vulture_text_length(V_FONT_MENU, tmpstr))/2;
	vulture_put_text_shadow(V_FONT_MENU, tmpstr, vulture_screen, text_start_x,
								text_start_y, textcolor, CLR32_BLACK);

	if (item->selected) {
		tmpstr[0] = '\0';
		if (item->count <= 0 || (obj && item->count > obj->quan))
			snprintf(tmpstr, 32, "selected (all)");
		else
			snprintf(tmpstr, 32, "selected (%d)", item->count);
		text_start_x = x + w - vulture_text_length(V_FONT_MENU, tmpstr) - 6;
		vulture_put_text_shadow(V_FONT_MENU, tmpstr, vulture_screen, text_start_x,
									text_start_y, textcolor, CLR32_BLACK);
	}

	/* draw the tile itself */
	/* constrain the drawing region to the box for the object tile, so that large
	* tiles don't overlap */
	vulture_set_draw_region(x + 2, y + 2, x + h - 3, y + h - 3);

	/* darken the background */
	vulture_fill_rect(x + 2, y + 2, x + h - 3, y + h - 3, CLR32_BLACK_A30);

	/* indicate blessed/cursed visually */
	if (obj && obj->bknown && obj->blessed)
		vulture_fill_rect(x + 2, y + 2, x + h - 3, y + h - 3, CLR32_BLESS_BLUE);

	if (obj && obj->bknown && obj->cursed)
		vulture_fill_rect(x + 2, y + 2, x + h - 3, y + h - 3, CLR32_CURSE_RED);

	/* draw the object tile */
	if (obj) {
    int tile_x, tile_y;
    int tile = 0;

		tile = vulture_object_to_tile(obj->otyp, -1, -1, obj);

		tile_x = x + h/2;
		tile_y = y + h * 3 / 4;

		if (TILE_IS_OBJECT(tile))
		{
			tile = tile - OBJTILEOFFSET + ICOTILEOFFSET;
			tile_x = x + 2;
			tile_y = y + 2;
		}

    vulture_put_tile(tile_x, tile_y, tile);
	}

	/* draw the item letter on the top left corner of the object tile */
	snprintf(tmpstr, 11, "%c", accelerator);
	vulture_put_text_shadow(V_FONT_MENU, tmpstr, vulture_screen, x + 2,
								y + 2, textcolor, CLR32_BLACK);

	/* draw the quantity on the tile */
	if (obj && obj->quan > 1)
	{
		snprintf(tmpstr, 11, "%ld", obj->quan);
		txt_height = vulture_text_height(V_FONT_MENU, tmpstr);
		text_start_x = x + h - vulture_text_length(V_FONT_MENU, tmpstr) - 2;
		vulture_put_text_shadow(V_FONT_MENU, tmpstr, vulture_screen, text_start_x,
									y + h - txt_height, CLR32_WHITE, CLR32_BLACK);
	}

	/* restore the drawing region */
	vulture_set_draw_region(0, 0, vulture_screen->w - 1, vulture_screen->h - 1);

	vulture_invalidate_region(x, y, w, h);

	return 0;
}


eventresult objitemwin::handle_mousemotion_event(window* target, void* result,
                                                  int xrel, int yrel, int state)
{
	bool prevhover;
	
	vulture_set_mcursor(V_CURSOR_NORMAL);
	prevhover = hover;
	hover = true;

	if (hover != prevhover) {
		need_redraw = true;
		return V_EVENT_HANDLED_REDRAW;
	}
	return V_EVENT_HANDLED_NOREDRAW;
}


eventresult objitemwin::handle_other_event(window* target, void* result, SDL_Event *event)
{
	bool prevhover;

	if (event->type == SDL_MOUSEMOVEOUT) {
		prevhover = hover;
		hover = false;

		if (hover != prevhover) {
			need_redraw = true;
			return V_EVENT_HANDLED_REDRAW;
		}
		return V_EVENT_HANDLED_NOREDRAW;
	}
	
	return V_EVENT_UNHANDLED;
}


eventresult objitemwin::handle_mousebuttonup_event(window* target, void* result,
                                int mouse_x, int mouse_y, int button, int state)
{
	if (button == SDL_BUTTON_WHEELUP || button == SDL_BUTTON_WHEELDOWN)
		hover = false;
	return V_EVENT_UNHANDLED;
}
