/* Copyright (c) Daniel Thaler, 2008                              */
/* NetHack may be freely redistributed.  See license for details. */

extern "C" {
	#include "hack.h"
}

#include "vultures_main.h"
#include "vultures_win.h"
#include "vultures_txt.h"
#include "vultures_gra.h"
#include "vultures_opt.h"
#include "vultures_sdl.h"
#include "vultures_mou.h"

#include "messagewin.h"
#include "levelwin.h"


messagewin *msgwin;
static Uint32 message_colors[V_MAX_MESSAGE_COLORS];


messagewin::messagewin(window *p) : window(p)
{
	int i;
	Uint32 color;

	v_type = V_WINTYPE_CUSTOM;
	
	bg_img = SDL_CreateRGBSurface(SDL_SWSURFACE, 40, 20, 32,
								vultures_px_format->Rmask,
								vultures_px_format->Gmask,
								vultures_px_format->Bmask, 0);
	SDL_FillRect(bg_img, NULL, CLR32_BLACK);
	SDL_SetAlpha(bg_img, SDL_SRCALPHA, 128);
	
	/* init message ring buffer */
	memset(message_ages, 0, V_MESSAGEBUF_SIZE * sizeof(int));
	message_top = -1;
	message_cur = -1;
	
	msgwin = this;
	
	/* Set message shading */
	for (i = 0; i < V_MAX_MESSAGE_COLORS; i++) {
		color = 255 - (i * 11);
		message_colors[i] = SDL_MapRGB(vultures_px_format, color, color, color);
	}
}



messagewin::~messagewin(void)
{
	SDL_FreeSurface(bg_img);
}



bool messagewin::draw()
{
	int age, textlen, num_messages;
	int pos_x, pos_y, i;
	string message;
	int refresh_x, refresh_y, refresh_h, refresh_w;
	int time_cur = moves;

	if (getshown() != 0)
		/* set time_cur to the age of the first shown message */
		get_message(0, &time_cur);

	refresh_x = refresh_y = 99999;
	refresh_h = refresh_w = 0;

	/* repaint background and free it */
	if (background) {
		vultures_put_img(abs_x, abs_y, background);
		SDL_FreeSurface(background);
		background = NULL;

		/* we need these values, so that we can refresh the larger
		* even if the window shrinks during the redraw */
		refresh_x = abs_x;
		refresh_y = abs_y;
		refresh_w = w;
		refresh_h = h;
	}

	num_messages = 0;
	h = 0;
	w = 0;

	/* calculate height & width of new message area */
	while(!(message = get_message(num_messages, &age)).empty() &&
		(time_cur-age) < V_MAX_MESSAGE_COLORS && num_messages < vultures_opts.messagelines)
	{
		h += (vultures_get_lineheight(V_FONT_MESSAGE) + 1);
		textlen = vultures_text_length(V_FONT_MESSAGE, message);
		w = (w < textlen) ? textlen : w;
		num_messages++;
	}

	/* add a bit of padding around the text */
	h += 2;
	w += 4;

	x = (parent->w - w) / 2;
	abs_x = parent->abs_x + x;
	abs_y = parent->abs_y;

	/* save new background */
	background = vultures_get_img(abs_x, abs_y,
						abs_x + w-1, abs_y + h-1);


	/* shade the message area */
	vultures_set_draw_region(abs_x, abs_y,
						abs_x + w-1, abs_y + h-1);
	pos_y = abs_y;
	while (pos_y <= abs_y + h) {
		pos_x = abs_x;

		while (pos_x <= abs_x+w) {
			vultures_put_img(pos_x, pos_y, bg_img);
			pos_x += bg_img->w;
		}
		pos_y += bg_img->h;
	}
	vultures_set_draw_region(0, 0, vultures_screen->w-1, vultures_screen->h-1);


	/* draw the messages */
	for (i = 0; i < num_messages; i++) {
		message = get_message(num_messages - i - 1, &age);

		pos_x = abs_x + (w - vultures_text_length(V_FONT_MESSAGE, message)) / 2;
		pos_y = abs_y + i * (vultures_get_lineheight(V_FONT_MESSAGE) + 1);
		vultures_put_text(V_FONT_MESSAGE, message, vultures_screen,
						pos_x, pos_y, message_colors[time_cur-age]);
	}

	refresh_w = (refresh_w > w) ? refresh_w : w;
	refresh_h = (refresh_h > h) ? refresh_h : h;

	if (refresh_x > abs_x)
	{
		refresh_w += refresh_x - abs_x;
		refresh_x = abs_x;
	}
	refresh_y = (refresh_y < abs_y) ? refresh_y : abs_y;

	vultures_invalidate_region(refresh_x, refresh_y, refresh_w, refresh_h);

	return false;
}


eventresult messagewin::handle_mousemotion_event(window* target, void* result,
                                            int xrel, int yrel, int state)
{
	point mouse = vultures_get_mouse_pos();
	window *new_target = levwin->get_window_from_point(mouse);

	return levwin->handle_mousemotion_event(new_target, result, xrel, yrel, state);
}


eventresult messagewin::handle_mousebuttonup_event(window* target, void* result,
                                            int mouse_x, int mouse_y, int button, int state)
{
	point mouse = vultures_get_mouse_pos();
	window *new_target = levwin->get_window_from_point(mouse);

	return levwin->handle_mousebuttonup_event(new_target, result, mouse_x, mouse_y, button, state);
}


void messagewin::add_message(string str)
{
	message_cur = message_top = (message_top + 1) % V_MESSAGEBUF_SIZE;
	message_buf[message_top] = str;
	message_ages[message_top] = moves;
}


void messagewin::setshown(int first)
{
	message_cur = (message_top - first + V_MESSAGEBUF_SIZE) % V_MESSAGEBUF_SIZE;
}


int messagewin::getshown()
{
	return (message_top - message_cur + V_MESSAGEBUF_SIZE) % V_MESSAGEBUF_SIZE;
}


/* retrieve a message from the message buffer, offset messages before the current one */
string messagewin::get_message(int offset, int *age)
{
	if (offset < V_MESSAGEBUF_SIZE)
	{
    int msg_id = (message_cur - offset + V_MESSAGEBUF_SIZE) % V_MESSAGEBUF_SIZE;

    if (message_ages[msg_id])
    {
      *age = message_ages[msg_id];
      return message_buf[msg_id];
    }
	}

	*age = 0;
	return "";
}


void messagewin::view_all(void)
{
	int offset, time, winid;
	string message;
	char menuline[256];

	winid = vultures_create_nhwindow(NHW_MENU);

	offset = -getshown();
	while ( !(message = get_message(offset, &time)).empty() )
	{
		sprintf(menuline, "T:%d %s", time, message.c_str());
		vultures_putstr(winid, ATR_NONE, menuline);
		offset++;
	}

	/* Display the messages */
	vultures_display_nhwindow(winid, TRUE);

	/* Clean up */
	vultures_destroy_nhwindow(winid);
}

