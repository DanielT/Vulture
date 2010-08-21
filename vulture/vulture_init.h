/* NetHack may be freely redistributed.  See license for details. */

#ifndef _vulture_init_h_
#define _vulture_init_h_


/* exported functions */
extern void          vulture_show_logo_screen(void);
extern int           vulture_init_graphics(void);
extern void          vulture_destroy_graphics(void);

extern void vulture_askname(void);
extern void vulture_player_selection(void);

#endif
