/* Copyright (c) Daniel Thaler, 2008                              */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef _introwin_h_
#define _introwin_h_

#include "window.h"

#include <string>
#include <vector>
using std::string;
using std::vector;

#define SCENETIME 5000

class introwin : public window
{
public:
	introwin(window *p, vector<string> &imagenames, vector< vector<string> > &subtitles);
	virtual ~introwin();
	virtual bool draw();
	virtual eventresult handle_timer_event(window* target, void* result, int time);
	virtual eventresult handle_mousemotion_event(window* target, void* result, 
	                                             int xrel, int yrel, int state);
	virtual eventresult handle_mousebuttonup_event(window* target, void* result,
	                                       int mouse_x, int mouse_y, int button, int state);
	virtual eventresult handle_keydown_event(window* target, void* result,
	                                         int sym, int mod, int unicode);
	virtual eventresult handle_resize_event(window* target, void* result, int res_w, int res_h);

private:
	eventresult next_scene(void);
	
	int current_scene;
	SDL_Surface *image;
	vector<string> imagenames;
	vector< vector<string> > subtitles;
	bool image_changed;
	int starttick;
};


#endif
