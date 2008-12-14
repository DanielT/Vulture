/* Copyright (c) Daniel Thaler, 2006, 2008			  */
/* NetHack may be freely redistributed.  See license for details. */



// #define META(c) (0x80 | (c))
// #define CTRL(c) (0x1f & (c))

// extern "C" int take_off();
// extern "C" int select_off(struct obj *);
// extern "C" long takeoff_mask;
// extern "C" const char *disrobing;




#if 0
/************************************************************
* basic dialog handlers
************************************************************/

int vultures_eventh_messagebox(struct window* handler, struct window* target,
						void* result, SDL_Event* event)
{
	if (event->type == SDL_MOUSEMOTION)
		vultures_set_mcursor(V_CURSOR_NORMAL);

	else if (event->type == SDL_MOUSEBUTTONUP && 
			target->menu_id == 1 &&
			event->button.button == SDL_BUTTON_LEFT)
		return V_EVENT_HANDLED_FINAL;

	else if(event->type == SDL_KEYDOWN)
	{
		if (event->key.keysym.sym == SDLK_RETURN ||
			event->key.keysym.sym == SDLK_KP_ENTER ||
			event->key.keysym.sym == SDLK_ESCAPE ||
			event->key.keysym.sym == SDLK_SPACE)
			return V_EVENT_HANDLED_FINAL;
	}
	else if (event->type == SDL_VIDEORESIZE)
	{
		handler->x = (handler->parent->w - handler->w) / 2;
		handler->y = (handler->parent->h - handler->h) / 2;
		return V_EVENT_HANDLED_NOREDRAW;
	}
	return V_EVENT_HANDLED_NOREDRAW;
}

/************************************************************
* inventory handler
************************************************************/

int vultures_eventh_objitem(struct window* handler, struct window* target,
							void* result, SDL_Event* event)
{
	int prevhover = 0;

	switch (event->type)
	{
		case SDL_MOUSEMOTION:
			vultures_set_mcursor(V_CURSOR_NORMAL);

			prevhover = handler->hover;
			handler->hover = 1;

			if (handler->hover != prevhover)
			{
				handler->need_redraw = 1;
				return V_EVENT_HANDLED_REDRAW;
			}
			return V_EVENT_HANDLED_NOREDRAW;

		case SDL_MOUSEMOVEOUT:
			prevhover = handler->hover;
			handler->hover = 0;

			if (handler->hover != prevhover)
			{
				handler->need_redraw = 1;
				return V_EVENT_HANDLED_REDRAW;
			}
			return V_EVENT_HANDLED_NOREDRAW;

		case SDL_MOUSEBUTTONUP:
			if ((event->button.button == SDL_BUTTON_WHEELUP)||
				(event->button.button == SDL_BUTTON_WHEELDOWN))
				handler->hover = 0;
			return V_EVENT_UNHANDLED;
	}

	return V_EVENT_UNHANDLED;
}


#endif
