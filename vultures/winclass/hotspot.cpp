/* NetHack may be freely redistributed.  See license for details. */

#include "hotspot.h"

hotspot::hotspot(window *parent, int x, int y, int w, int h, int menu_id, std::string name) : window(parent)
{
	this->x = x;
	this->y = y;
	this->w = w;
	this->h = h;
	this->abs_x = parent->abs_x + this->x;
	this->abs_y = parent->abs_y + this->y;
	this->menu_id = menu_id;
	
	caption = name;
}


bool hotspot::draw()
{
	return false;
}
