/*  SCCS Id: @(#)winjtp.h 1.0 2000/09/10  */
/* Copyright (c) Jaakko Peltonen, 2000         */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef WINJTP_H
#define WINJTP_H

#define E extern

extern struct window_procs jtp_procs;

/* port specific variable declarations */
extern winid BASE_WINDOW;

/* ### winjtp.c ### */
E void NDECL(win_jtp_init);

/* external declarations */
E void FDECL(jtp_init_nhwindows, (int *, char **));
E void NDECL(jtp_player_selection);
E void NDECL(jtp_askname);
E void NDECL(jtp_get_nh_event) ;
E void FDECL(jtp_exit_nhwindows, (const char *));
E void FDECL(jtp_suspend_nhwindows, (const char *));
E void NDECL(jtp_resume_nhwindows);
E winid FDECL(jtp_create_nhwindow, (int));
E void FDECL(jtp_clear_nhwindow, (winid));
E void FDECL(jtp_display_nhwindow, (winid, BOOLEAN_P));
E void FDECL(jtp_dismiss_nhwindow, (winid));
E void FDECL(jtp_destroy_nhwindow, (winid));
E void FDECL(jtp_curs, (winid,int,int));
E void FDECL(jtp_putstr, (winid, int, const char *));
E void FDECL(jtp_display_file, (const char *, BOOLEAN_P));
E void FDECL(jtp_start_menu, (winid));
E void FDECL(jtp_add_menu, (winid,int,const ANY_P *,
			CHAR_P,CHAR_P,int,const char *, BOOLEAN_P));
E void FDECL(jtp_end_menu, (winid, const char *));
E int FDECL(jtp_select_menu, (winid, int, MENU_ITEM_P **));
E char FDECL(jtp_message_menu, (CHAR_P,int,const char *));
E void NDECL(jtp_update_inventory);
E void NDECL(jtp_mark_synch);
E void NDECL(jtp_wait_synch);
#ifdef CLIPPING
E void FDECL(jtp_cliparound, (int, int));
#endif
#ifdef POSITIONBAR
E void FDECL(jtp_update_positionbar, (char *));
#endif
E void FDECL(jtp_print_glyph, (winid,XCHAR_P,XCHAR_P,int));
E void FDECL(jtp_raw_print, (const char *));
E void FDECL(jtp_raw_print_bold, (const char *));
E int NDECL(jtp_nhgetch);
E int FDECL(jtp_nh_poskey, (int *, int *, int *));
E void NDECL(jtp_nhbell);
E int NDECL(jtp_doprev_message);
E char FDECL(jtp_yn_function, (const char *, const char *, CHAR_P));
E void FDECL(jtp_getlin, (const char *,char *));
E int NDECL(jtp_get_ext_cmd);
E void FDECL(jtp_number_pad, (int));
E void NDECL(jtp_delay_output);
E void NDECL(jtp_start_screen);
E void NDECL(jtp_end_screen);
E void FDECL(jtp_outrip, (winid,int));
#ifdef CHANGE_COLOR
E void FDECL(jtp_change_color,(int color,long rgb,int reverse));
#ifdef MAC
E void FDECL(jtp_change_background,(int white_or_black));
E short FDECL(set_jtp_font_name, (winid, char *));
#endif
E char * NDECL(jtp_get_color_string);
#endif


#undef E

/* JTP_DEBUG */
extern int debug_putstr;
/* JTP_DEBUG */
extern int jtp_whatis_active;

#endif /* WINJTP_H */
