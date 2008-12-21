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


eventresult choicedialog::event_handler(window* target, void* result, SDL_Event* event)
{
	if (event->type == SDL_MOUSEMOTION)
		vultures_set_mcursor(V_CURSOR_NORMAL);

	else if (event->type == SDL_MOUSEBUTTONUP && event->button.button == SDL_BUTTON_LEFT)
	{
		if (target->accelerator)
		{
			*(char*)result = (char)target->accelerator;
			return V_EVENT_HANDLED_FINAL;
		}
	}

	else if (event->type == SDL_KEYDOWN)
	{
		switch (event->key.keysym.sym)
		{
			case SDLK_KP_ENTER:
			case SDLK_RETURN:
				target = defbutton;
				if (target)
				{
					*(char*)result = (char)target->accelerator;
					return V_EVENT_HANDLED_FINAL;
				}
				break;

			case SDLK_ESCAPE:
				if (find_accel('q'))
				{
					*(char*)result = 'q';
					return V_EVENT_HANDLED_FINAL;
				}
				else if (find_accel('n'))
				{
					*(char*)result = 'n';
					return V_EVENT_HANDLED_FINAL;
				}
				break;

			default:
				if (find_accel((char)event->key.keysym.unicode))
				{
					*(char*)result = (char)event->key.keysym.unicode;
					return V_EVENT_HANDLED_FINAL;
				}
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