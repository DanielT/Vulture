/* Copyright (c) Daniel Thaler, 2006                              */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef _vultures_mou_h_
#define _vultures_mou_h_

#include <string>
using std::string;

extern void vultures_mouse_init(void);
extern void vultures_mouse_destroy(void);

extern void vultures_set_mcursor(int cursornum);
extern point vultures_get_mouse_pos(void);
extern point vultures_get_mouse_prev_pos(void);
extern void vultures_set_mouse_pos(int x, int y);

extern void vultures_mouse_draw(void);
extern void vultures_mouse_refresh(void);
extern void vultures_mouse_restore_bg();

/* tooltip handling functions */
extern void vultures_mouse_invalidate_tooltip(int force);
extern void vultures_mouse_set_tooltip(string str);




#endif
