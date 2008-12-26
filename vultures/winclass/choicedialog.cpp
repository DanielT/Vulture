/* Copyright (c) Daniel Thaler, 2008                              */
/* NetHack may be freely redistributed.  See license for details. */

#include <cstring>

#include "vultures_win.h"
#include "vultures_mou.h"
#include "vultures_txt.h"
#include "vultures_tile.h"

#include "choicedialog.h"
#include "button.h"


choicedialog::choicedialog(window *p, string question, const char *choices, char defchoice) : mainwin(p)
{
	button *btn;
	int nbuttons = 0, longdesc = 0;
	int i, len;
	const char * str = (char*)choices;
	char *btncaption;

	defbutton = NULL;
	caption = question;
	nbuttons = strlen(choices);

	/* a very common case is "yn" queries. Improve that to a yes/no query*/
	if (strncmp(choices, "yn", 3) == 0)
	{
		str = "yes\0no";
		longdesc = 1;
	}
	else if (strncmp(choices, "ynq", 4) == 0)
	{
		str = "yes\0no\0quit";
		longdesc = 1;
	}

	for(i = 0; i < nbuttons; i++)
	{
		len = 1;
		if (longdesc)
			len = strlen(str);

		btncaption = new char[len+1];
		strncpy(btncaption, str, len);
		btncaption[len] = '\0';
		
		btn = new button(this, btncaption, i, str[0]);
		if (str[0] == defchoice)
			defbutton = btn;

		delete btncaption;

		if (longdesc)
			str += len;

		str++;
	}

	this->defchoice = defchoice;

	mainwin::layout();
}

bool choicedialog::draw()
{
	return mainwin::draw();
}


eventresult choicedialog::handle_mousemotion_event(window* target, void* result, int xrel, 
                                             int yrel, int state)
{
	vultures_set_mcursor(V_CURSOR_NORMAL);
	return V_EVENT_HANDLED_NOREDRAW;
}


eventresult choicedialog::handle_mousebuttonup_event(window* target, void* result,
                                            int mouse_x, int mouse_y, int button, int state)
{
	if (button == SDL_BUTTON_LEFT && target->accelerator) {
		*(char*)result = (char)target->accelerator;
		return V_EVENT_HANDLED_FINAL;
	}
	return V_EVENT_HANDLED_NOREDRAW;
}


eventresult choicedialog::handle_keydown_event(window* target, void* result, int sym, int mod, int unicode)
{
	switch (sym) {
		case SDLK_KP_ENTER:
		case SDLK_RETURN:
			target = defbutton;
			if (target) {
				*(char*)result = (char)target->accelerator;
				return V_EVENT_HANDLED_FINAL;
			}
			break;

		case SDLK_ESCAPE:
			if (find_accel('q')) {
				*(char*)result = 'q';
				return V_EVENT_HANDLED_FINAL;
			}
			else if (find_accel('n')) {
				*(char*)result = 'n';
				return V_EVENT_HANDLED_FINAL;
			}
			break;

		default:
			if (find_accel((char)unicode)) {
				*(char*)result = (char)unicode;
				return V_EVENT_HANDLED_FINAL;
			}
	}
	return V_EVENT_HANDLED_NOREDRAW;
}


eventresult choicedialog::handle_resize_event(window* target, void* result, int res_w, int res_h)
{
	x = (parent->w - w) / 2;
	y = (parent->h - h) / 2;
	return V_EVENT_HANDLED_NOREDRAW;
}

