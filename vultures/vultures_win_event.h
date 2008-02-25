/* Copyright (c) Daniel Thaler, 2006				  */
/* NetHack may be freely redistributed.  See license for details. */


#ifndef _vultures_win_event_h_
#define _vultures_win_event_h_


enum hotspots {
    V_HOTSPOT_NONE = 0,
    /* child windows of the map */
    V_HOTSPOT_SCROLL_UPLEFT,
    V_HOTSPOT_SCROLL_UP,
    V_HOTSPOT_SCROLL_UPRIGHT,
    V_HOTSPOT_SCROLL_LEFT,
    V_HOTSPOT_SCROLL_RIGHT,
    V_HOTSPOT_SCROLL_DOWNLEFT,
    V_HOTSPOT_SCROLL_DOWN,
    V_HOTSPOT_SCROLL_DOWNRIGHT,
    V_WIN_MINIMAP,
    V_WIN_STATUSBAR,
    V_WIN_TOOLBAR1,
    V_WIN_TOOLBAR2,
    V_WIN_ENHANCE,

    /* child windows of the statusbar */
    V_HOTSPOT_BUTTON_LOOK,
    V_HOTSPOT_BUTTON_EXTENDED,
    V_HOTSPOT_BUTTON_MAP,
    V_HOTSPOT_BUTTON_SPELLBOOK,
    V_HOTSPOT_BUTTON_INVENTORY,
    V_HOTSPOT_BUTTON_DISCOVERIES,
    V_HOTSPOT_BUTTON_MESSAGES,
    V_HOTSPOT_BUTTON_OPTIONS,
    V_HOTSPOT_BUTTON_IFOPTIONS,
    V_HOTSPOT_BUTTON_HELP
};


/* pass-through event handler for the root window: passes stuff to _eventh_level */
extern int vultures_eventh_null(struct window* handler, struct window* target,
                                void* result, SDL_Event* event);

/* main window elements */
extern int vultures_eventh_messages(struct window* handler, struct window* target,
                                    void* result, SDL_Event* event);
extern int vultures_eventh_status(struct window* handler, struct window* target,
                                  void* result, SDL_Event* event);
extern int vultures_eventh_toolbar(struct window* handler, struct window* target,
                                   void* result, SDL_Event* event);
extern int vultures_eventh_enhance(struct window* handler, struct window* target,
                                   void* result, SDL_Event* event);
extern int vultures_eventh_level(struct window* handler, struct window* target,
                                 void* result, SDL_Event* event);

/* map display */
extern int vultures_eventh_map(struct window* handler, struct window* target,
                               void* result, SDL_Event* event);
extern int vultures_eventh_minimap(struct window* handler, struct window* target,
                                   void* result, SDL_Event* event);

/* handlers for the dialogs created by vultures_yn_function */
extern int vultures_eventh_query_choices(struct window* handler, struct window* target,
                                         void* result, SDL_Event* event);
extern int vultures_eventh_query_direction(struct window* handler, struct window* target,
                                           void* result, SDL_Event* event);
extern int vultures_eventh_query_anykey(struct window* handler, struct window* target,
                                         void* result, SDL_Event* event);

/* more basic dialog types */
extern int vultures_eventh_messagebox(struct window* handler, struct window* target,
                                      void* result, SDL_Event* event);
extern int vultures_eventh_menu(struct window* handler, struct window* target,
                                void* result, SDL_Event* event);
extern int vultures_eventh_input(struct window* handler, struct window* target,
                                 void* result, SDL_Event* event);
extern int vultures_eventh_dropdown(struct window* handler, struct window* target,
                                    void* result, SDL_Event* event);

/* the button event handler is responsible for the "pressed button" effect */
extern int vultures_eventh_button(struct window* handler, struct window* target,
                                  void* result, SDL_Event* event);

extern int vultures_eventh_inventory(struct window* handler, struct window* target,
                                     void* result, SDL_Event* event);

#endif

