
#include "vultures_win.h"
#include "vultures_txt.h"
#include "vultures_mou.h"
#include "vultures_tile.h"

#include "inputdialog.h"


inputdialog::inputdialog(window *p, const char *ques, int size) : mainwin(p)
{
	caption = strdup(ques);

	destsize = size;

	input = new textwin(this, destsize);
	input->menu_id = 1;
	input->autobg = 1;

	/* calc sizes and positions */
	w = vultures_text_length(V_FONT_HEADLINE, (char *)ques);
	w = (w < 500) ? 500 : w;

	input->w = w;
	w += vultures_winelem.border_left->w + vultures_winelem.border_right->w;
	h = vultures_winelem.border_top->h + vultures_get_lineheight(V_FONT_HEADLINE) +
		3 * vultures_get_lineheight(V_FONT_INPUT) + vultures_winelem.border_bottom->h;
	input->h = vultures_get_lineheight(V_FONT_INPUT) + 1;

	input->x = (w - input->w) / 2;
	input->y = vultures_winelem.border_top->h + vultures_get_lineheight(V_FONT_HEADLINE) +
	           vultures_get_lineheight(V_FONT_INPUT);

}

void inputdialog::copy_input(char *dest)
{
	strncpy(dest, input->caption, destsize);
}


bool inputdialog::draw()
{
	return mainwin::draw();
}

eventresult inputdialog::event_handler(window* target, void* result, SDL_Event* event)
{
	char *text = first_child->caption; /* input boxes have only one child, so this works */
	int len = strlen(text);

	/* it's a menu, so it uses the normal cursor */
	if (event->type == SDL_MOUSEMOTION)
		vultures_set_mcursor(V_CURSOR_NORMAL);

	/* other than that we only care about keyboard events */
	else if (event->type == SDL_KEYDOWN)
	{
		switch (event->key.keysym.sym)
		{
			case SDLK_KP_ENTER:
			case SDLK_RETURN:
				/* done! */
				*(int*)result = 1;
				return V_EVENT_HANDLED_FINAL;

			case SDLK_ESCAPE:
				/* cancel */
				*(int*)result = 0;
				return V_EVENT_HANDLED_FINAL;

			case SDLK_BACKSPACE:
				if (len > 0)
				{
					text[len - 1] = '\0';
					first_child->need_redraw = 1;
				}
				return V_EVENT_HANDLED_REDRAW;

			default:
				/* add characters up to a maximum of 256 */
				if (len < 256 && vultures_text_length(V_FONT_MENU, text) <
				(first_child->w - 10) && isprint(event->key.keysym.unicode))
				{
					text[len] = (char)event->key.keysym.unicode;
					text[len+1] = '\0';
					first_child->need_redraw = 1;
				}
				/* only the child needs to be redrawn */
				return V_EVENT_HANDLED_REDRAW;
		}
	}
	else if (event->type == SDL_VIDEORESIZE)
	{
		x = (parent->w - w) / 2;
		y = (parent->h - h) / 2;
		return V_EVENT_HANDLED_NOREDRAW;
	}

	return V_EVENT_HANDLED_NOREDRAW;
}
