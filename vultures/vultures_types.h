#ifndef _vultures_types_h_
#define _vultures_types_h_
/*
 * This file is meant to be a place to put structure declarations
 * and enums that have global importance.
 * 
 * In the interest of modularization THIS FILE SHOULD NOT EXPORT FUNCTIONS
 */

#include <SDL.h>


enum wintypes {
    V_WINTYPE_NONE, /* only the root window has this type */
    V_WINTYPE_MAIN,
    V_WINTYPE_BUTTON,
    V_WINTYPE_OPTION,
    V_WINTYPE_SCROLLBAR,
    V_WINTYPE_TEXT,
    V_WINTYPE_DROPDOWN,
    V_WINTYPE_CUSTOM
};

struct window;

typedef int (*event_handler_func)(struct window* handler, struct window* target,
                                    void* result, SDL_Event* event);
typedef int (*draw_func)(struct window* win);


typedef struct window {
    int id;
    int nh_type;                   /* type assigned by nethack */
    enum wintypes v_type;
    char * caption;

    /* absolute coords; calculated befor drawing */
    int abs_x, abs_y;

    /* coords relative to parent */
    int x, y;
    int w, h;

    struct window * parent;
    struct window * sib_next;
    struct window * sib_prev;
    struct window * first_child;
    struct window * last_child;

    SDL_Surface * background;

    unsigned need_redraw : 1;
    unsigned selected : 1;  /* is the option selected?  for buttons selected == pressed */
    unsigned autobg : 1;  /* the generic code should handle background saving & restoring for the window */
    unsigned content_is_text : 1; /* does the menu window cntain only text? */
    unsigned is_input : 1; /* is the text writable? */
    unsigned visible : 1;
    unsigned is_default : 1;
    unsigned scrollable : 1;
    unsigned pd_type : 1; /* 0 == the pd union is storing a value, 1 == it's storing an SDL_Surface */
    unsigned select_how : 2;

    int count;
    int accelerator;

    union {
        int menu_id;
        void * menu_id_v;
    };

    union {
        int scrollpos;  /* for scrollbars */
        int ending_type;/* how you died */
        struct obj * obj;
        int inv_page;   /* the currently displayed inventory page */
        int count;      /* the count in query_anykey dialogs */
        Uint32 textcolor;
        SDL_Surface * image; /* various windows want to keep image data around */
    } pd;

    /* Function pointers */
    draw_func draw;
    event_handler_func event_handler;
} vultures_win;



typedef struct { 
    /* image data (width & height encoded in first 4 bytes) */
    SDL_Surface *graphic;
    /* hotspot offsets;
     * difference between left/top most non-transparent pixel
     * and hotspot defined in the image
     */
    int xmod,ymod;
} vultures_tile;


typedef struct {
    int x;
    int y;
} point;

#endif

