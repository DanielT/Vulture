/* NetHack may be freely redistributed.  See license for details. */

#ifndef _vulture_main_h
#define _vulture_main_h

#ifdef __cplusplus
extern "C" {
#endif

#include "hack.h"


extern struct window_procs vulture_procs;
extern int vulture_whatis_active;


extern void win_vulture_init();
extern int vulture_find_menu_accelerator(char *used_accelerators);
extern void vulture_bail(const char *mesg);


/* external declarations */
extern void vulture_init_nhwindows(int *, char **);
extern void vulture_get_nh_event() ;
extern void vulture_exit_nhwindows(const char *);
extern void vulture_suspend_nhwindows(const char *);
extern void vulture_resume_nhwindows();
extern winid vulture_create_nhwindow(int);
extern void vulture_clear_nhwindow(winid);
extern void vulture_display_nhwindow(winid, BOOLEAN_P);
extern void vulture_dismiss_nhwindow(winid);
extern void vulture_destroy_nhwindow(winid);
extern void vulture_curs(winid,int,int);
extern void vulture_putstr(winid, int, const char *);
extern void vulture_display_file(const char *, BOOLEAN_P);
extern void vulture_start_menu(winid);
extern void vulture_add_menu(int,int,const ANY_P *,
			CHAR_P,CHAR_P,int,const char *, BOOLEAN_P);
extern void vulture_end_menu(winid, const char *);
extern int vulture_select_menu(winid, int, MENU_ITEM_P **);
extern char vulture_message_menu(CHAR_P,int,const char *);
extern void vulture_update_inventory();
extern void vulture_mark_synch();
extern void vulture_wait_synch();
#ifdef CLIPPING
extern void vulture_cliparound(int, int);
#endif
#ifdef POSITIONBAR
extern void vulture_update_positionbar(char *);
#endif
extern void vulture_print_glyph(winid, XCHAR_P, XCHAR_P, int);
extern void vulture_raw_print(const char *);
extern void vulture_raw_print_bold(const char *);
extern int vulture_nhgetch();
extern int vulture_nh_poskey(int *, int *, int *);
extern void vulture_nhbell();
extern int vulture_doprev_message();
extern char vulture_yn_function(const char *, const char *, CHAR_P);
extern void vulture_getlin(const char *,char *);
extern int vulture_get_ext_cmd();
extern void vulture_number_pad(int);
extern void vulture_delay_output();
extern void vulture_start_screen();
extern void vulture_end_screen();
extern void vulture_outrip(winid,int);
extern void vulture_preference_update(const char *);
#ifdef CHANGE_COLOR
extern void vulture_change_color(int color,long rgb,int reverse);
#ifdef MAC
extern void vulture_change_background(int white_or_black);
extern short set_vulture_font_name(winid, char *);
#endif
extern char * vulture_get_color_string();
#endif


#ifdef __cplusplus
}
#endif 

#endif /* _vulture_main_h */
