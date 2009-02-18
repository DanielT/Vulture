/* NetHack may be freely redistributed.  See license for details. */

#ifndef _vultures_init_h_
#define _vultures_init_h_


/* exported functions */
extern void          vultures_show_logo_screen(void);
extern int           vultures_init_graphics(void);
extern void          vultures_destroy_graphics(void);

extern void vultures_askname(void);
extern void vultures_player_selection(void);

#endif
