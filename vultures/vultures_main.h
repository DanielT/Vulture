/* NetHack may be freely redistributed.  See license for details. */

#ifndef _vultures_main_h
#define _vultures_main_h

#ifdef __cplusplus
extern "C" {
#endif

#include "hack.h"


extern struct window_procs vultures_procs;
extern int vultures_whatis_active;


extern void win_vultures_init();
extern int vultures_find_menu_accelerator(char *used_accelerators);
extern void vultures_bail(const char *mesg);


/* external declarations */
extern void vultures_init_nhwindows(int *, char **);
extern void vultures_get_nh_event() ;
extern void vultures_exit_nhwindows(const char *);
extern void vultures_suspend_nhwindows(const char *);
extern void vultures_resume_nhwindows();
extern winid vultures_create_nhwindow(int);
extern void vultures_clear_nhwindow(winid);
extern void vultures_display_nhwindow(winid, BOOLEAN_P);
extern void vultures_dismiss_nhwindow(winid);
extern void vultures_destroy_nhwindow(winid);
extern void vultures_curs(winid,int,int);
extern void vultures_putstr(winid, int, const char *);
extern void vultures_display_file(const char *, BOOLEAN_P);
extern void vultures_start_menu(winid);
extern void vultures_add_menu(int,int,const ANY_P *,
			CHAR_P,CHAR_P,int,const char *, BOOLEAN_P);
extern void vultures_end_menu(winid, const char *);
extern int vultures_select_menu(winid, int, MENU_ITEM_P **);
extern char vultures_message_menu(CHAR_P,int,const char *);
extern void vultures_update_inventory();
extern void vultures_mark_synch();
extern void vultures_wait_synch();
#ifdef CLIPPING
extern void vultures_cliparound(int, int);
#endif
#ifdef POSITIONBAR
extern void vultures_update_positionbar(char *);
#endif
extern void vultures_print_glyph(winid, XCHAR_P, XCHAR_P, int);
extern void vultures_raw_print(const char *);
extern void vultures_raw_print_bold(const char *);
extern int vultures_nhgetch();
extern int vultures_nh_poskey(int *, int *, int *);
extern void vultures_nhbell();
extern int vultures_doprev_message();
extern char vultures_yn_function(const char *, const char *, CHAR_P);
extern void vultures_getlin(const char *,char *);
extern int vultures_get_ext_cmd();
extern void vultures_number_pad(int);
extern void vultures_delay_output();
extern void vultures_start_screen();
extern void vultures_end_screen();
extern void vultures_outrip(winid,int);
extern void vultures_preference_update(const char *);
#ifdef CHANGE_COLOR
extern void vultures_change_color(int color,long rgb,int reverse);
#ifdef MAC
extern void vultures_change_background(int white_or_black);
extern short set_vultures_font_name(winid, char *);
#endif
extern char * vultures_get_color_string();
#endif


#ifdef __cplusplus
}
#endif 

#endif /* _vultures_main_h */
