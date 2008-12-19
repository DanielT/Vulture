
#include "vultures_win.h"
#include "vultures_sdl.h"
#include "vultures_mou.h"
#include "vultures_txt.h"
#include "vultures_tile.h"


#include "anykeydialog.h"
#include "textwin.h"
#include "button.h"


anykeydialog::anykeydialog(window *p, string ques) : mainwin(p)
{
	caption = ques;
	count = 0;

	/* add it as a menuitem so that menuwin::layout will dtrt */
	txt = new textwin(this, "(type any key)");
	txt->y = 0;
	txt->h = vultures_text_height(V_FONT_MENU, txt->caption) + 10;

	/* create buttons */
	new button(this, "Show choices", 2, '?');
	new button(this, "Show inventory", 3, '*');
	new button(this, "Cancel", 4, '\033');
	
	mainwin::layout();
}


eventresult anykeydialog::event_handler(window* target, void* result, SDL_Event* event)
{
	char key;
	int i;
	char buffer[32];

	if (event->type == SDL_MOUSEMOTION)
		vultures_set_mcursor(V_CURSOR_NORMAL);

	else if (event->type == SDL_MOUSEBUTTONUP && target->accelerator &&
	    event->button.button == SDL_BUTTON_LEFT) {
		*(char*)result = target->accelerator;
		return V_EVENT_HANDLED_FINAL;
	} else if (event->type == SDL_KEYDOWN) {
		switch (event->key.keysym.sym)
		{
			case SDLK_ESCAPE:
				*(char*)result = '\033';
				return V_EVENT_HANDLED_FINAL;

			case SDLK_BACKSPACE:
				count = count / 10;
				if (count > 0)
					sprintf(buffer, "Count: %d", count);
				else
					sprintf(buffer, "(press any key)");
				txt->set_caption(buffer);
				txt->need_redraw = 1;
				return V_EVENT_HANDLED_REDRAW;

			default:
				/* was it an accelerator for one of the buttons? */
				if (event->key.keysym.unicode && find_accel(event->key.keysym.unicode)) {
					*(char*)result = event->key.keysym.unicode;
					return V_EVENT_HANDLED_FINAL;
				}

				/* it isn't, so lets translate it (Stage 1: function keys etc) */
				key = vultures_convertkey_sdl2nh(&event->key.keysym);

				if (!key)
					/* no translation, it must have been a function key */
					return V_EVENT_HANDLED_NOREDRAW;

				if (isdigit(key)) {
					/* we got a digit and only modify the count */
					if (count < 10000000)
						count = count * 10 + (key - 0x30);
					sprintf(buffer, "Count: %d", count);
					txt->set_caption(buffer);
					txt->need_redraw = 1;
					return V_EVENT_HANDLED_REDRAW;
				}

				/* non-digit, non-function-key, non-accelerator: we have a winner! */
				if (count) {
					/* retrieve the count and push most of it onto the eventstack */
					memset(buffer, 0, 16);
					snprintf(buffer, 16, "%d", count);
					vultures_eventstack_add(key, -1 , -1, V_RESPOND_ANY);
					for (i=15; i > 0; i--)
						if (buffer[i])
							vultures_eventstack_add(buffer[i], -1, -1, V_RESPOND_ANY);

					/* we return the first digit of the count */
					key = buffer[0];
				}

				/* return our key */
				*(char*)result = key;
				return V_EVENT_HANDLED_FINAL;
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

bool anykeydialog::draw()
{
	return mainwin::draw();
}
