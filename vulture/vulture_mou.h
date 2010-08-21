/* NetHack may be freely redistributed.  See license for details. */

#ifndef _vulture_mou_h_
#define _vulture_mou_h_

#include <string>

extern void vulture_mouse_init(void);
extern void vulture_mouse_destroy(void);

extern void vulture_set_mcursor(int cursornum);
extern point vulture_get_mouse_pos(void);
extern point vulture_get_mouse_prev_pos(void);
extern void vulture_set_mouse_pos(int x, int y);

extern void vulture_mouse_draw(void);
extern void vulture_mouse_refresh(void);
extern void vulture_mouse_restore_bg();

/* tooltip handling functions */
extern void vulture_mouse_invalidate_tooltip(int force);
extern void vulture_mouse_set_tooltip(std::string str);




#endif
