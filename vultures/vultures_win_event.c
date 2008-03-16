/* Copyright (c) Daniel Thaler, 2006, 2008			  */
/* NetHack may be freely redistributed.  See license for details. */

#include <ctype.h>

#include "SDL.h"

#include "vultures_win.h"
#include "vultures_win_event.h"
#include "vultures_sdl.h"
#include "vultures_map.h"
#include "vultures_mou.h"
#include "vultures_txt.h"
#include "vultures_main.h"
#include "vultures_gfl.h"
#include "vultures_opt.h"
#include "vultures_gen.h"
#include "vultures_tile.h"


#define META(c) (0x80 | (c))
#define CTRL(c) (0x1f & (c))

extern int take_off();
extern int select_off(struct obj *);
extern long takeoff_mask;
extern const char *disrobing;

enum scrolltypes {
    V_SCROLL_LINE_REL,
    V_SCROLL_PAGE_REL,
    V_SCROLL_PAGE_ABS,
    V_SCROLL_PIXEL_ABS
};



enum invactions {
    V_INVACTION_APPLY = 1,
    V_INVACTION_DRINK,
    V_INVACTION_EAT,
    V_INVACTION_READ,
    V_INVACTION_ZAP,
    V_INVACTION_WEAR,
    V_INVACTION_PUT_ON,
    V_INVACTION_WIELD,
    V_INVACTION_REMOVE,
    V_INVACTION_DROP,
    V_INVACTION_NAME
};


static struct window * vultures_find_defbtn(struct window * parent);
static int vultures_scrollto(struct window * win, int scrolldir, int scrollpos);
static void vultures_toggle_map(void);


/* List of the event handler functions (they appear in this order):

** pass-through event handler for the root window **
*  vultures_eventh_null

** main window elements **
*  vultures_eventh_messages
*  vultures_eventh_status
*  vultures_eventh_toobar
*  vultures_eventh_enhance
*  vultures_eventh_level

** map display **
*  vultures_eventh_map
*  vultures_eventh_minimap

** handlers for the dialogs created by vultures_yn_function **
*  vultures_eventh_query_choices
*  vultures_eventh_query_direction
*  vultures_eventh_query_anykey

** more basic dialog types **
*  vultures_eventh_messagebox
*  vultures_eventh_menu
*  vultures_eventh_input
*  vultures_eventh_dropdown

** the button event handler is responsible for the "pressed button" effect **
*  vultures_eventh_button

** last but not least: the inventory **
*  vultures_eventh_inventory
*/


/************************************************************
 * main elements
 ************************************************************/

int vultures_eventh_status(struct window* handler, struct window* target,
                           void* result, SDL_Event* event)
{
    if (event->type == SDL_MOUSEMOTION)
        vultures_set_mcursor(V_CURSOR_NORMAL);
    else if (event->type == SDL_VIDEORESIZE)
        /* x coordinate does not change */
        handler->y = handler->parent->h - (handler->h + 6);

    return V_EVENT_HANDLED_NOREDRAW;
}


/* handles events for either of the two toolbars */
int vultures_eventh_toolbar(struct window* handler, struct window* target,
                           void* result, SDL_Event* event)
{
    switch (event->type)
    {
        /* mousemotion sets the correct cursor */
        case SDL_MOUSEMOTION:
            vultures_set_mcursor(V_CURSOR_NORMAL);
            break;

        /* timer: draw tooltips */
        case SDL_TIMEREVENT:
            if (event->user.code > HOVERTIMEOUT)
                if (target != handler  && target->caption)
                    vultures_mouse_set_tooltip(target->caption);
            break;

        /* click events */
        case SDL_MOUSEBUTTONUP:
            /* throw away uninterseting clicks */
            if (event->button.button != SDL_BUTTON_LEFT ||
                target == handler || !target->menu_id)
                break;

            /* one of the buttons was clicked */
            switch (target->menu_id)
            {
                case V_HOTSPOT_BUTTON_LOOK:
                    vultures_eventstack_add('y', -1, -1, V_RESPOND_CHARACTER);
                    ((vultures_event*)result)->num = '/';
                    return V_EVENT_HANDLED_FINAL;

                case V_HOTSPOT_BUTTON_EXTENDED:
                    ((vultures_event*)result)->num = '#';
                    return V_EVENT_HANDLED_FINAL;

                case V_HOTSPOT_BUTTON_MAP:
                    vultures_toggle_map();
                    return V_EVENT_HANDLED_REDRAW;

                case V_HOTSPOT_BUTTON_SPELLBOOK:
                    ((vultures_event*)result)->num = 'Z';
                    return V_EVENT_HANDLED_FINAL;

                case V_HOTSPOT_BUTTON_INVENTORY:
                    ((vultures_event*)result)->num = 'i';
                    return V_EVENT_HANDLED_FINAL;

                case V_HOTSPOT_BUTTON_DISCOVERIES:
                    ((vultures_event*)result)->num = '\\';
                    return V_EVENT_HANDLED_FINAL;

                case V_HOTSPOT_BUTTON_MESSAGES:
                    vultures_messages_view();
                    break;

                case V_HOTSPOT_BUTTON_OPTIONS:
                    ((vultures_event*)result)->num = 'O';
                    return V_EVENT_HANDLED_FINAL;

                case V_HOTSPOT_BUTTON_IFOPTIONS:
                    vultures_iface_opts();
                    break;

                case V_HOTSPOT_BUTTON_HELP:
                    ((vultures_event*)result)->num = '?';
                    return V_EVENT_HANDLED_FINAL;
            }


        case SDL_VIDEORESIZE:
            handler->x = handler->parent->w - (handler->w + 6);
            if (handler->menu_id == V_WIN_TOOLBAR1)
                handler->y = handler->parent->h - (handler->h*2 + 8);
            else
                handler->y = handler->parent->h - (handler->h + 6);
            break;

        default: break;
    }
    return V_EVENT_HANDLED_NOREDRAW;
}


/* this event handler makes the message window completely "transparent"
 * to all mouse events, by passeing them down to eventh_level */
int vultures_eventh_messages(struct window* handler, struct window* target,
                             void* result, SDL_Event* event)
{
    point mouse;
    struct window *map, *new_target;

    if (event->type == SDL_VIDEORESIZE)
        return V_EVENT_HANDLED_NOREDRAW;

    map = vultures_get_window(0);
    mouse = vultures_get_mouse_pos();
    new_target = vultures_get_window_from_point(map, mouse);

    return vultures_eventh_level(map, new_target, result, event);
}



/* the enhance "window" is the golden + above the character stats */
int vultures_eventh_enhance(struct window* handler, struct window* target,
                             void* result, SDL_Event* event)
{
    struct window *win;

    /* change the mouse cursor to indicate that this window is clickable */
    if (event->type == SDL_MOUSEMOTION)
        vultures_set_mcursor(V_CURSOR_NORMAL);

    /* respond to clicks */
    else if (event->type == SDL_MOUSEBUTTONUP && event->button.button == SDL_BUTTON_LEFT)
    {
        ((vultures_event*)result)->num = META('e');
        return V_EVENT_HANDLED_FINAL;
    }

    else if (event->type == SDL_VIDEORESIZE)
    {
        /* this relies on the fact that the enhance window is created
         * immediately after the status window */
        win = handler->sib_prev;

        /* x coordinate does not change */
        handler->y = win->y - handler->h;
    }

    /* show a tooltip */
    else if (event->type == SDL_TIMEREVENT && event->user.code > HOVERTIMEOUT)
        vultures_mouse_set_tooltip("Enhance a skill");

    return V_EVENT_HANDLED_NOREDRAW;
}


static int vultures_handle_map_click(void* result, int button, point mappos)
{
    int retval, action_id = 0;

    /* if vultures_whatis_active is set, we want a location (for look or teleport) */
    if (vultures_whatis_active)
    {
        ((vultures_event*)result)->num = 0;
        ((vultures_event*)result)->x = mappos.x;
        ((vultures_event*)result)->y = mappos.y;
        return V_EVENT_HANDLED_FINAL;
    }

    /* else  */
    /* right click: try to resolve the click on the map to a default action */
    if (button == SDL_BUTTON_LEFT)
        action_id = vultures_get_map_action(mappos);
    /* left click: allow the user to choose from a context menu */
    else if (button == SDL_BUTTON_RIGHT)
        action_id = vultures_get_map_contextmenu(mappos);

    /* if an action was chosen, return it and leave the event loop */
    if (action_id)
    {
        retval = vultures_perform_map_action(action_id, mappos);
        if (retval)
        {
            ((vultures_event*)result)->num = retval;
            return V_EVENT_HANDLED_FINAL;
        }
    }

    return V_EVENT_HANDLED_NOREDRAW;
}


int vultures_eventh_level(struct window* handler, struct window* target,
                          void* result, SDL_Event* event)
{
    int translated_key, key;
    int macronum, i;
    point mouse, mappos;
    char * ttext;

    mouse = vultures_get_mouse_pos();
    mappos = vultures_mouse_to_map(mouse);

    switch (event->type)
    {
        case SDL_MOUSEBUTTONUP:
            return vultures_handle_map_click(result, event->button.button, mappos);
            break;

        /* keyboard events */
        case SDL_KEYDOWN:
            switch (event->key.keysym.sym)
            {
                case SDLK_TAB:
                    vultures_toggle_map();
                    return V_EVENT_HANDLED_REDRAW;

                /* F1- F6 trigger macros */
                case SDLK_F1:
                case SDLK_F2:
                case SDLK_F3:
                case SDLK_F4:
                case SDLK_F5:
                case SDLK_F6:
                    macronum = event->key.keysym.sym - SDLK_F1;
                    if (!vultures_opts.macro[macronum][0])
                        break;

                    ((vultures_event*)result)->num = vultures_opts.macro[macronum][0];
                    for (i = strlen(vultures_opts.macro[macronum]) - 1; i > 0; i--)
                        vultures_eventstack_add(vultures_opts.macro[macronum][i], -1, -1, V_RESPOND_ANY);
                    return V_EVENT_HANDLED_FINAL;

#if 0
// #ifdef VULTURESEYE
                case SDLK_ESCAPE:
                    if (vultures_whatis_active) /* FIXME There are most certainly other places where this should be ignored */
                        break;
                    vultures_show_mainmenu();
                    return V_EVENT_HANDLED_REDRAW;
#endif
                /* CTRL+SHIFT+o opens the interface options */
                case SDLK_o:
                    if (!(event->key.keysym.mod & KMOD_SHIFT) || !(event->key.keysym.mod & KMOD_CTRL))
                        break;
                    vultures_iface_opts();
                    return V_EVENT_HANDLED_REDRAW;

                /* CTRL+SHIFT+p shows the message log */
                case SDLK_p:
                    if (!(event->key.keysym.mod & KMOD_SHIFT) || !(event->key.keysym.mod & KMOD_CTRL))
                        break;
                    vultures_messages_view();
                    return V_EVENT_HANDLED_REDRAW;

                default:
                    break;
            }

            /* all other keys are converted and passed to the core */
            key = vultures_convertkey_sdl2nh(&event->key.keysym);

            if (!key)
                return V_EVENT_HANDLED_NOREDRAW;

            if (vultures_winid_map && isdigit(key))
                translated_key = key;
            else
                translated_key = vultures_translate_key(key);

            if (translated_key)
            {
                ((vultures_event*)result)->num = translated_key;
                return V_EVENT_HANDLED_FINAL;
            }
            return V_EVENT_HANDLED_NOREDRAW;


        case SDL_MOUSEMOTION:
            if (target != handler && target->menu_id)
            {
                /* show a map scroll cursor if the mouse is in the edge zone */
                switch (target->menu_id)
                {
                    case V_HOTSPOT_SCROLL_UPLEFT:
                        vultures_set_mcursor(V_CURSOR_SCROLLUPLEFT); break;
                    case V_HOTSPOT_SCROLL_UP:
                        vultures_set_mcursor(V_CURSOR_SCROLLUP); break;
                    case V_HOTSPOT_SCROLL_UPRIGHT:
                        vultures_set_mcursor(V_CURSOR_SCROLLUPRIGHT); break;
                    case V_HOTSPOT_SCROLL_LEFT:
                        vultures_set_mcursor(V_CURSOR_SCROLLLEFT); break;
                    case V_HOTSPOT_SCROLL_RIGHT:
                        vultures_set_mcursor(V_CURSOR_SCROLLRIGHT); break;
                    case V_HOTSPOT_SCROLL_DOWNLEFT:
                        vultures_set_mcursor(V_CURSOR_SCROLLDOWNLEFT); break;
                    case V_HOTSPOT_SCROLL_DOWN:
                        vultures_set_mcursor(V_CURSOR_SCROLLDOWN); break;
                    case V_HOTSPOT_SCROLL_DOWNRIGHT:
                        vultures_set_mcursor(V_CURSOR_SCROLLDOWNRIGHT); break;
                    default:
                        vultures_set_mcursor(vultures_get_map_cursor(mappos));
                }
            }
            else
                /* select a cursor for the current position */
                vultures_set_mcursor(vultures_get_map_cursor(mappos));

            /* if the highlight option is on, store the map position of the mouse
             * and refresh the current and previous positions */
            if (vultures_opts.highlight_cursor_square && 
                (vultures_map_highlight.x != mappos.x || vultures_map_highlight.y != mappos.y))
            {
                mouse = vultures_map_to_mouse(vultures_map_highlight);
                vultures_add_to_clipregion(mouse.x - V_MAP_XMOD, mouse.y - V_MAP_YMOD,
                                           mouse.x + V_MAP_XMOD, mouse.y + V_MAP_YMOD);
                mouse = vultures_map_to_mouse(mappos);
                vultures_add_to_clipregion(mouse.x - V_MAP_XMOD, mouse.y - V_MAP_YMOD,
                                           mouse.x + V_MAP_XMOD, mouse.y + V_MAP_YMOD);

                vultures_map_highlight = mappos;
                handler->need_redraw = 1;
                return V_EVENT_HANDLED_REDRAW;
            }

            break;

        case SDL_TIMEREVENT:
            /* hovering on a border: scroll */
            if (target != handler && target->menu_id)
            {
                int increment = (event->user.code / 500) + 1;
                switch (target->menu_id)
                {
                    case V_HOTSPOT_SCROLL_UPLEFT:
                        vultures_view_x-=increment; break;
                    case V_HOTSPOT_SCROLL_UP:
                        vultures_view_x-=increment; vultures_view_y-=increment; break;
                    case V_HOTSPOT_SCROLL_UPRIGHT:
                        vultures_view_y-=increment; break;
                    case V_HOTSPOT_SCROLL_LEFT:
                        vultures_view_x-=increment; vultures_view_y+=increment; break;
                    case V_HOTSPOT_SCROLL_RIGHT:
                        vultures_view_x+=increment; vultures_view_y-=increment; break;
                    case V_HOTSPOT_SCROLL_DOWNLEFT:
                        vultures_view_y+=increment; break;
                    case V_HOTSPOT_SCROLL_DOWN:
                        vultures_view_x+=increment; vultures_view_y+=increment; break;
                    case V_HOTSPOT_SCROLL_DOWNRIGHT:
                        vultures_view_x+=increment; break;
                }

                vultures_view_x = (vultures_view_x < 0) ? 0 : vultures_view_x;
                vultures_view_y = (vultures_view_y < 0) ? 0 : vultures_view_y;
                vultures_view_x = (vultures_view_x > V_MAP_WIDTH) ? V_MAP_WIDTH : vultures_view_x;
                vultures_view_y = (vultures_view_y > V_MAP_HEIGHT) ? V_MAP_HEIGHT : vultures_view_y;

                handler->need_redraw = 1;
                return V_EVENT_HANDLED_REDRAW;
            }

            /* hovering over the map: display a tooltip */
            if (event->user.code < HOVERTIMEOUT)
                break;

            if (target != handler && target->caption)
            {
                vultures_mouse_set_tooltip(target->caption);
            }
            else
            {
                ttext = vultures_map_square_description(mappos, 1);
                if(ttext && ttext[0])
                    vultures_mouse_set_tooltip(ttext);
                free(ttext);
            }
            break;

        case SDL_VIDEORESIZE:
            if (handler == target)
            {
                handler->w = event->resize.w;
                handler->h = event->resize.h;
            }
            else
            {
                switch (target->menu_id)
                {
                    /* case V_HOTSPOT_SCROLL_UPLEFT: never needs updating on resize*/

                    case V_HOTSPOT_SCROLL_UP:
                        target->w = handler->w - 40;
                        break;

                    case V_HOTSPOT_SCROLL_UPRIGHT:
                        target->x = handler->w - 20;
                        break;

                    case V_HOTSPOT_SCROLL_LEFT:
                        target->h = handler->h - 40;
                        break;

                    case V_HOTSPOT_SCROLL_RIGHT:
                        target->h = handler->h - 40;
                        target->x = handler->w - 20;
                        break;

                    case V_HOTSPOT_SCROLL_DOWNLEFT:
                        target->y = handler->h - 20;
                        break;

                    case V_HOTSPOT_SCROLL_DOWN:
                        target->y = handler->h - 20;
                        target->w = handler->w - 40;
                        break;

                    case V_HOTSPOT_SCROLL_DOWNRIGHT:
                        target->x = handler->w - 20;
                        target->y = handler->h - 20;
                        break;
                }
            }
            break;

        default:
            break;
    }

    return V_EVENT_HANDLED_NOREDRAW;
}



/************************************************************
 * map handlers
 ************************************************************/

/* event handler for the map (ie the symbolic representation on the parchment) */
int vultures_eventh_map(struct window* handler, struct window* target,
                        void* result, SDL_Event* event)
{
    point mouse, mappos;
    char * ttext;

    mouse = vultures_get_mouse_pos();
    mappos.x = (mouse.x - handler->abs_x - 39) / VULTURES_MAP_SYMBOL_WIDTH;
    mappos.y = (mouse.y - handler->abs_y - 91) / VULTURES_MAP_SYMBOL_HEIGHT;

    if (mappos.x < 1 || mappos.x >= V_MAP_WIDTH ||
        mappos.y < 0 || mappos.y >= V_MAP_HEIGHT)
    {
        mappos.x = -1;
        mappos.y = -1;
    }

    switch (event->type)
    {
        case SDL_MOUSEBUTTONUP:
            /* handler != target if the user clicked on the X in the upper right corner */
            if (handler != target && target->menu_id == 1)
            {
                vultures_toggle_map();
                vultures_mouse_invalidate_tooltip(1);
                return V_EVENT_HANDLED_REDRAW;
            }

            /* only handle clicks on valid map locations */
            if (mappos.x != -1)
                return vultures_handle_map_click(result, event->button.button, mappos);

            break;

        case SDL_TIMEREVENT:
            if (event->user.code < HOVERTIMEOUT)
                return V_EVENT_HANDLED_NOREDRAW;

            /* draw the tooltip for the close button */
            if (handler != target && target->menu_id == 1)
                vultures_mouse_set_tooltip(target->caption);
            /* draw a tooltip for the map location */
            else if (mappos.x != -1)
            {
                ttext = vultures_map_square_description(mappos, 1);
                if(ttext && ttext[0])
                    vultures_mouse_set_tooltip(ttext);
                free(ttext);
            }

            break;

        case SDL_MOUSEMOTION:
            vultures_set_mcursor(V_CURSOR_NORMAL);
            break;

        case SDL_VIDEORESIZE:
            handler->x = (handler->parent->w - handler->w) / 2;
            handler->y = (handler->parent->h - handler->h) / 2;
            break;
    }

    return V_EVENT_HANDLED_NOREDRAW;
}



int vultures_eventh_minimap(struct window* handler, struct window* target,
                           void* result, SDL_Event* event)
{
    point mouse, mappos;
    int offs_x, offs_y;

    /* translate the mouse position to a map coordinate */
    mouse = vultures_get_mouse_pos();

    offs_x = mouse.x - handler->abs_x - 6 - 40;
    offs_y = mouse.y - handler->abs_y - 6;

    mappos.x = ( offs_x + 2*offs_y)/4;
    mappos.y = (-offs_x + 2*offs_y)/4;
    
    
    if (event->type == SDL_VIDEORESIZE)
    {
        handler->x = handler->parent->w - (handler->w + 6);
        return V_EVENT_HANDLED_NOREDRAW;
    }

    if (mappos.x < 1 || mappos.x > V_MAP_WIDTH ||
        mappos.y < 0 || mappos.y > V_MAP_HEIGHT)
        return V_EVENT_UNHANDLED;


    if (event->type == SDL_MOUSEMOTION)
        vultures_set_mcursor(V_CURSOR_NORMAL);

    /* limited mouse event handling, due to the fact that the minimap
     * is too small for precise targeting */
    else if (event->type == SDL_MOUSEBUTTONUP)
    {
        /* left button: direct selection of a location (for very imprecise teleport) */
        if (event->button.button == SDL_BUTTON_LEFT)
        {
            if (vultures_whatis_active)
            {
                ((vultures_event*)result)->num = 0;
                ((vultures_event*)result)->x = mappos.x;
                ((vultures_event*)result)->y = mappos.y;
                return V_EVENT_HANDLED_FINAL;
            }

            ((vultures_event*)result)->num = vultures_perform_map_action(V_ACTION_TRAVEL, mappos);
            return V_EVENT_HANDLED_FINAL;
        }
        /* right button: travel to location */
        else if(event->button.button == SDL_BUTTON_RIGHT)
        {
            vultures_view_x = mappos.x;
            vultures_view_y = mappos.y;
            vultures_get_window(0)->need_redraw = 1;
            return V_EVENT_HANDLED_REDRAW;
        }
    }

    return V_EVENT_HANDLED_NOREDRAW;
}



/************************************************************
 * handlers for the dialogs created by vultures_yn_function
 ************************************************************/

int vultures_eventh_query_choices(struct window* handler, struct window* target,
                                  void* result, SDL_Event* event)
{
    if (event->type == SDL_MOUSEMOTION)
        vultures_set_mcursor(V_CURSOR_NORMAL);

    else if (event->type == SDL_MOUSEBUTTONUP && event->button.button == SDL_BUTTON_LEFT)
    {
        if (target->accelerator)
        {
            *(char*)result = (char)target->accelerator;
            return V_EVENT_HANDLED_FINAL;
        }
    }

    else if (event->type == SDL_KEYDOWN)
    {
        switch (event->key.keysym.sym)
        {
            case SDLK_KP_ENTER:
            case SDLK_RETURN:
                target = vultures_find_defbtn(handler);
                if (target)
                {
                    *(char*)result = (char)target->accelerator;
                    return V_EVENT_HANDLED_FINAL;
                }
                break;

            case SDLK_ESCAPE:
                if (vultures_accel_to_win(handler, 'q'))
                {
                    *(char*)result = 'q';
                    return V_EVENT_HANDLED_FINAL;
                }
                else if (vultures_accel_to_win(handler, 'n'))
                {
                    *(char*)result = 'n';
                    return V_EVENT_HANDLED_FINAL;
                }
                break;

            default:
                if (vultures_accel_to_win(handler, (char)event->key.keysym.unicode))
                {
                    *(char*)result = (char)event->key.keysym.unicode;
                    return V_EVENT_HANDLED_FINAL;
                }
        }
    }
    else if (event->type == SDL_VIDEORESIZE)
    {
        handler->x = (handler->parent->w - handler->w) / 2;
        handler->y = (handler->parent->h - handler->h) / 2;
        return V_EVENT_HANDLED_NOREDRAW;
    }

    return V_EVENT_HANDLED_NOREDRAW;
}


/* direction selection dialog event handler */
int vultures_eventh_query_direction(struct window* handler, struct window* target,
                                    void* result, SDL_Event* event)
{
    point mouse;
    int dir_x, dir_y;
    char choice = 0;

    const char dirkeys[3][3] = {{'8', '9', '6'},
                                {'7', '.', '3'},
                                {'4', '1', '2'}};

    /* mouse motion events are merely used to set the correct cursor */
    if (event->type == SDL_MOUSEMOTION)
        vultures_set_mcursor(V_CURSOR_NORMAL);

    /* any key is accepted as a valid choice */
    else if (event->type == SDL_KEYDOWN)
    {
        if (event->key.keysym.sym == SDLK_ESCAPE)
            choice = -1;
        else
            choice = vultures_convertkey_sdl2nh(&event->key.keysym);
    }

    /* mouse click events: only left clicks on the arrow grid are accepted */
    else if (event->type == SDL_MOUSEBUTTONUP && event->button.button == SDL_BUTTON_LEFT)
    {
        if (target == handler->first_child)
        {
            /* get the click coordinates and normalize them to the center of the arrow grid */
            mouse = vultures_get_mouse_pos();
            mouse.x -= (target->abs_x + target->w/2);
            mouse.y -= (target->abs_y + target->h/2);

            /* translate the click position to a direction */
            dir_x = V_MAP_YMOD * mouse.x + V_MAP_XMOD * mouse.y + V_MAP_XMOD*V_MAP_YMOD;
            dir_x = dir_x / (2 * V_MAP_XMOD * V_MAP_YMOD) - (dir_x < 0);
            dir_y = -V_MAP_YMOD * mouse.x + V_MAP_XMOD * mouse.y + V_MAP_XMOD * V_MAP_YMOD;
            dir_y = dir_y / (2 * V_MAP_XMOD * V_MAP_YMOD) - (dir_y < 0);

            /* convert the chosen direction to a key */
            choice = 0;
            if (dir_y >= -1 && dir_y <= 1 && dir_x >= -1 && dir_x <= 1)
                choice = vultures_numpad_to_hjkl(dirkeys[dir_y + 1][dir_x + 1], 0);

            if (dir_x >= 2 && mouse.x < target->w / 2 && mouse.y < target->h / 2)
                choice = '>';

            if (dir_x <= -2 && mouse.x > -target->w / 2 && mouse.y > -target->h / 2)
                choice = '<';
        }
    }
    else if (event->type == SDL_VIDEORESIZE)
    {
        handler->x = (handler->parent->w - handler->w) / 2;
        handler->y = (handler->parent->h - handler->h) / 2;
        return V_EVENT_HANDLED_NOREDRAW;
    }

    if (choice)
    {
        if (!vultures_winid_map)
            choice = vultures_translate_key(choice);
        *(char*)result = choice;
        return V_EVENT_HANDLED_FINAL;
    }

    return V_EVENT_HANDLED_NOREDRAW;
}



int vultures_eventh_query_anykey(struct window* handler, struct window* target,
                                 void* result, SDL_Event* event)
{
    char key;
    int count, i;
    char buffer[16];

    if (event->type == SDL_MOUSEMOTION)
        vultures_set_mcursor(V_CURSOR_NORMAL);

    else if (event->type == SDL_MOUSEBUTTONUP && target->accelerator && event->button.button == SDL_BUTTON_LEFT)
    {
        *(char*)result = target->accelerator;
        return V_EVENT_HANDLED_FINAL;
    }
    else if (event->type == SDL_KEYDOWN)
    {
        count = handler->pd.count;
        switch (event->key.keysym.sym)
        {
            case SDLK_ESCAPE:
                *(char*)result = '\033';
                return V_EVENT_HANDLED_FINAL;

            case SDLK_BACKSPACE:
                count = count / 10;
                handler->pd.count = count;
                if (count > 0)
                    sprintf(handler->first_child->caption, "Count: %d", count);
                else
                    sprintf(handler->first_child->caption, "(press any key)");
                handler->first_child->need_redraw = 1;
                return V_EVENT_HANDLED_REDRAW;

            default:
                /* was it an accelerator for one of the buttons? */
                if (event->key.keysym.unicode && vultures_accel_to_win(handler, event->key.keysym.unicode))
                {
                    *(char*)result = event->key.keysym.unicode;
                    return V_EVENT_HANDLED_FINAL;
                }

                /* it isn't, so lets translate it (Stage 1: function keys etc) */
                key = vultures_convertkey_sdl2nh(&event->key.keysym);

                if (!key)
                    /* no translation, it must have been a function key */
                    return V_EVENT_HANDLED_NOREDRAW;

                if (isdigit(key))
                {
                    /* we got a digit and only modify the count */
                    count = count * 10 + (key - 0x30);
                    handler->pd.count = count;
                    sprintf(handler->first_child->caption, "Count: %d", count);
                    handler->first_child->need_redraw = 1;
                    return V_EVENT_HANDLED_REDRAW;
                }

                /* non-digit, non-function-key, non-accelerator: we have a winner! */
                if (count)
                {
                    /* retrieve the count and push most of it onto the eventstack */
                    memset(buffer, 0, 16);
                    snprintf(buffer, 16, "%d", count);
                    vultures_eventstack_add(key, -1 , -1, V_RESPOND_ANY);
                    for (i=15; i > 0; i--)
                        if (buffer[i])
                            vultures_eventstack_add(buffer[i], -1, -1, V_RESPOND_ANY);

                    /* we return the first digit of the count */
                    key = buffer[0];
                }

                /* return our key */
                *(char*)result = key;
                return V_EVENT_HANDLED_FINAL;
        }
    }
    else if (event->type == SDL_VIDEORESIZE)
    {
        handler->x = (handler->parent->w - handler->w) / 2;
        handler->y = (handler->parent->h - handler->h) / 2;
        return V_EVENT_HANDLED_NOREDRAW;
    }
    
    return V_EVENT_HANDLED_NOREDRAW;
}



/************************************************************
 * basic dialog handlers
 ************************************************************/

int vultures_eventh_messagebox(struct window* handler, struct window* target,
                         void* result, SDL_Event* event)
{
    if (event->type == SDL_MOUSEMOTION)
        vultures_set_mcursor(V_CURSOR_NORMAL);

    else if (event->type == SDL_MOUSEBUTTONUP && 
             target->menu_id == 1 &&
             event->button.button == SDL_BUTTON_LEFT)
        return V_EVENT_HANDLED_FINAL;

    else if(event->type == SDL_KEYDOWN)
    {
        if (event->key.keysym.sym == SDLK_RETURN ||
            event->key.keysym.sym == SDLK_KP_ENTER ||
            event->key.keysym.sym == SDLK_ESCAPE ||
            event->key.keysym.sym == SDLK_SPACE)
            return V_EVENT_HANDLED_FINAL;
    }
    else if (event->type == SDL_VIDEORESIZE)
    {
        handler->x = (handler->parent->w - handler->w) / 2;
        handler->y = (handler->parent->h - handler->h) / 2;
        return V_EVENT_HANDLED_NOREDRAW;
    }
    return V_EVENT_HANDLED_NOREDRAW;
}


static void select_option(struct window * handler, struct window * target, int count)
{
    struct window * winelem;

    if (handler->select_how == PICK_ONE)
    {
        /* unselect everything else */
        winelem = handler->first_child;
        while (winelem)
        {
            winelem->selected = 0;
            winelem->pd.count = -1;
            winelem = winelem->sib_next;
        }
        target->selected = 1;
    }
    else
        target->selected = !target->selected;

    if (target->selected)
        target->pd.count = count;
}


int vultures_menu_mousescroll(struct window* handler, struct window* target, int is_drag)
{
    int scrollind_y, scrollpos;
    point mouse;
    int scrollarea_top = target->abs_y + vultures_winelem.scrollbutton_up->h;
    int scrollarea_bottom = target->abs_y + target->h - vultures_winelem.scrollbutton_down->h;

    scrollind_y = ((scrollarea_bottom - scrollarea_top -
                    vultures_winelem.scroll_indicator->h) *
                    (int)target->pd.scrollpos) / 8192.0;

    mouse = vultures_get_mouse_pos();

    /* click on the scroll-up button */
    if (mouse.y <= scrollarea_top)
    {
        if (!is_drag)
            return vultures_scrollto(handler, V_SCROLL_LINE_REL, -1);
    }

    /* click on the scroll-down button */
    else if (mouse.y >= scrollarea_bottom)
    {
        if (!is_drag)
            return vultures_scrollto(handler, V_SCROLL_LINE_REL, 1);
    }

    /* click on the scrollbar above the indicator */
    else if (mouse.y <= scrollarea_top + scrollind_y)
    {
        scrollpos = ((mouse.y - scrollarea_top) * 8192.0 / 
                        (scrollarea_bottom - scrollarea_top - 
                        vultures_winelem.scroll_indicator->h));
        target->pd.scrollpos = scrollpos;
        handler->need_redraw = 1;
        return V_EVENT_HANDLED_REDRAW;
    }

    /*click on the scrollbar below the indicator */
    else if (mouse.y >= scrollarea_top + vultures_winelem.scroll_indicator->h + scrollind_y)
    {
        scrollpos = ((mouse.y - scrollarea_top - vultures_winelem.scroll_indicator->h) * 
                        8192.0 / (scrollarea_bottom - scrollarea_top -
                        vultures_winelem.scroll_indicator->h));
        target->pd.scrollpos = scrollpos;
        handler->need_redraw = 1;
        return V_EVENT_HANDLED_REDRAW;
    }

    return V_EVENT_HANDLED_NOREDRAW;
}


/* event handler for scrollable menus */
int vultures_eventh_menu(struct window* handler, struct window* target,
                         void* result, SDL_Event* event)
{
    struct window * winelem;
    point mouse, oldpos;
    int key;
    char * str_to_find;

    /* menus use the normal cursor */
    if (event->type == SDL_MOUSEMOTION)
    {
        vultures_set_mcursor(V_CURSOR_NORMAL);

        if (event->motion.state == SDL_PRESSED && 
            (SDL_GetMouseState(NULL, NULL) & SDL_BUTTON(SDL_BUTTON_LEFT)))
        {
            mouse = vultures_get_mouse_pos();
            oldpos.x = mouse.x + event->motion.xrel;
            oldpos.y = mouse.y + event->motion.yrel;
            winelem = vultures_get_window_from_point(handler, oldpos);

            if (target->v_type == V_WINTYPE_SCROLLBAR && winelem == target)
                return vultures_menu_mousescroll(handler, target, 1);

        }
    }

    /* handle clicks and wheel events */
    else if (event->type == SDL_MOUSEBUTTONUP)
    {
        /* wheel up/down: scroll by one line */
        if (event->button.button == SDL_BUTTON_WHEELUP)
            return vultures_scrollto(handler, V_SCROLL_LINE_REL, -1);

        if (event->button.button == SDL_BUTTON_WHEELDOWN)
            return vultures_scrollto(handler, V_SCROLL_LINE_REL, 1);


        if (target == handler)
            /* clicks on the menu window itself are not interesting */
            return V_EVENT_HANDLED_NOREDRAW;


        /* click on an option / checkbox */
        if (target->v_type == V_WINTYPE_OPTION)
        {
            select_option(handler, target, handler->pd.count ? handler->pd.count : -1);
            handler->pd.count = 0;
            if (handler->select_how == PICK_ONE)
            {
                *(int*)result = V_MENU_ACCEPT;
                return V_EVENT_HANDLED_FINAL;
            }

            handler->need_redraw = 1;
            return V_EVENT_HANDLED_REDRAW;
        }


        /* a click on a button */
        else if (target->v_type == V_WINTYPE_BUTTON && target->menu_id)
        {
            *(int*)result = target->menu_id;
            return V_EVENT_HANDLED_FINAL;
        }


        /* a click on the scrollbar */
        else if (target->v_type == V_WINTYPE_SCROLLBAR)
            return vultures_menu_mousescroll(handler, target, 0);

    }


    /* handle keyboard events */
    else if (event->type == SDL_KEYDOWN)
    {
        handler->need_redraw = 1;
        key = event->key.keysym.unicode;
        switch (event->key.keysym.sym)
        {
            case SDLK_RETURN:
            case SDLK_KP_ENTER:
                *(int*)result = V_MENU_ACCEPT;
                return V_EVENT_HANDLED_FINAL;

            case SDLK_SPACE:
            case SDLK_ESCAPE:
                *(int*)result = (handler->content_is_text) ? V_MENU_ACCEPT : V_MENU_CANCEL;
                return V_EVENT_HANDLED_FINAL;

            /* handle menu control keys */
            case SDLK_PAGEUP:   key = MENU_PREVIOUS_PAGE; /* '<' */ break; 
            case SDLK_PAGEDOWN: key = MENU_NEXT_PAGE;     /* '>' */ break;
            case SDLK_HOME:     key = MENU_FIRST_PAGE;    /* '^' */ break;
            case SDLK_END:      key = MENU_LAST_PAGE;     /* '|' */ break;

            /* scroll via arrow keys */
            case SDLK_KP2:
            case SDLK_DOWN:
                return vultures_scrollto(handler, V_SCROLL_LINE_REL, 1);

            case SDLK_KP8:
            case SDLK_UP:
                return vultures_scrollto(handler, V_SCROLL_LINE_REL, -1);

            case SDLK_BACKSPACE:
                handler->pd.count = handler->pd.count / 10;

            default: break;
        }

        if (!key)
            /* a function key, but not one we recognize, wass pressed */
            return V_EVENT_HANDLED_NOREDRAW;

        switch (key)
        {
            case MENU_PREVIOUS_PAGE:
                return vultures_scrollto(handler, V_SCROLL_PAGE_REL, -1);

            case MENU_NEXT_PAGE:
                return vultures_scrollto(handler, V_SCROLL_PAGE_REL, 1);

            case MENU_FIRST_PAGE:
                return vultures_scrollto(handler, V_SCROLL_PAGE_ABS, 0);

            case MENU_LAST_PAGE:
                return vultures_scrollto(handler, V_SCROLL_PAGE_ABS, 9999);


            case MENU_SELECT_ALL:
            case MENU_UNSELECT_ALL:
                /* invalid for single selection menus */
                if (handler->select_how == PICK_ONE)
                    return V_EVENT_HANDLED_NOREDRAW;

                winelem = handler->first_child;
                while (winelem)
                {
                    if (winelem->v_type == V_WINTYPE_OPTION)
                    {
                        winelem->selected = (key == MENU_SELECT_ALL);
                        winelem->pd.count = -1;
                    }
                    winelem = winelem->sib_next;
                }
                handler->need_redraw = 1;
                return V_EVENT_HANDLED_REDRAW;


            case MENU_INVERT_ALL:
                /* invalid for single selection menus */
                if (handler->select_how == PICK_ONE)
                    return V_EVENT_HANDLED_NOREDRAW;

                winelem = handler->first_child;
                while (winelem)
                {
                    if (winelem->v_type == V_WINTYPE_OPTION)
                    {
                        winelem->selected = !winelem->selected;
                        winelem->pd.count = -1;
                    }
                    winelem = winelem->sib_next;
                }
                handler->need_redraw = 1;
                return V_EVENT_HANDLED_REDRAW;


            case MENU_SELECT_PAGE:
            case MENU_UNSELECT_PAGE:
                /* invalid for single selection menus */
                if (handler->select_how == PICK_ONE)
                    return V_EVENT_HANDLED_NOREDRAW;

                winelem = handler->first_child;
                while (winelem)
                {
                    if (winelem->v_type == V_WINTYPE_OPTION && winelem->visible)
                    {
                        winelem->selected = (key == MENU_SELECT_PAGE);
                        winelem->pd.count = -1;
                    }
                    winelem = winelem->sib_next;
                }
                handler->need_redraw = 1;
                return V_EVENT_HANDLED_REDRAW;


            case MENU_INVERT_PAGE:
                /* invalid for single selection menus */
                if (handler->select_how == PICK_ONE)
                    return V_EVENT_HANDLED_NOREDRAW;

                winelem = handler->first_child;
                while (winelem)
                {
                    if (winelem->v_type == V_WINTYPE_OPTION && winelem->visible)
                    {
                        winelem->selected = !winelem->selected;
                        winelem->pd.count = -1;
                    }
                    winelem = winelem->sib_next;
                }
                handler->need_redraw = 1;
                return V_EVENT_HANDLED_REDRAW;


            case MENU_SEARCH:
                str_to_find = malloc(512);
                str_to_find[0] = '\0';
                if (vultures_get_input(-1, -1, "What are you looking for?", str_to_find) != -1)
                {
                    winelem = handler->first_child;
                    while (winelem)
                    {
                        if (winelem->caption && strstr(winelem->caption, str_to_find) && winelem->scrollable)
                        {
                            vultures_scrollto(handler, V_SCROLL_PIXEL_ABS, winelem->y);
                            break;
                        }
                        winelem = winelem->sib_next;
                    }
                }
                free(str_to_find);
                handler->need_redraw = 1;
                return V_EVENT_HANDLED_REDRAW;

            default:
                /* numbers are part of a count */
                if (key >= '0' && key <= '9') {
                    handler->pd.count = handler->pd.count * 10 + (key - '0');
                    break;
                }
            
                /* try to match the key to an accelerator */
                target = vultures_accel_to_win(handler, key);
                if (target)
                {
                    select_option(handler, target, handler->pd.count ? handler->pd.count : -1);
                    handler->pd.count = 0;
                    if (handler->select_how == PICK_ONE)
                    {
                        *(int*)result = V_MENU_ACCEPT;
                        return V_EVENT_HANDLED_FINAL;
                    }

                    /* if the selected element isn't visible bring it into view */
                    if (!target->visible)
                        vultures_scrollto(handler, V_SCROLL_PIXEL_ABS, target->y);

                    handler->need_redraw = 1;
                    return V_EVENT_HANDLED_REDRAW;
                }
                break;
        }
    }
    else if (event->type == SDL_VIDEORESIZE)
    {
        vultures_layout_menu(handler);
        return V_EVENT_HANDLED_NOREDRAW;
    }

    return V_EVENT_HANDLED_NOREDRAW;
}


/* event handler for input boxes */
int vultures_eventh_input(struct window* handler, struct window* target,
                          void* result, SDL_Event* event)
{
    char * text = handler->first_child->caption; /* input boxes have only one child, so this works */
    int len = strlen(text);

    /* it's a menu, so it uses the normal cursor */
    if (event->type == SDL_MOUSEMOTION)
        vultures_set_mcursor(V_CURSOR_NORMAL);

    /* other than that we only care about keyboard events */
    else if (event->type == SDL_KEYDOWN)
    {
        switch (event->key.keysym.sym)
        {
            case SDLK_KP_ENTER:
            case SDLK_RETURN:
                /* done! */
                *(int*)result = 1;
                return V_EVENT_HANDLED_FINAL;

            case SDLK_ESCAPE:
                /* cancel */
                *(int*)result = 0;
                return V_EVENT_HANDLED_FINAL;

            case SDLK_BACKSPACE:
                if (len > 0)
                {
                    text[len - 1] = '\0';
                    handler->first_child->need_redraw = 1;
                }
                return V_EVENT_HANDLED_REDRAW;

            default:
                /* add characters up to a maximum of 256 */
                if (len < 256 && vultures_text_length(V_FONT_MENU, text) <
                   (handler->first_child->w - 10) && isprint(event->key.keysym.unicode))
                {
                    text[len] = (char)event->key.keysym.unicode;
                    text[len+1] = '\0';
                    handler->first_child->need_redraw = 1;
                }
                /* only the child needs to be redrawn */
                return V_EVENT_HANDLED_REDRAW;
        }
    }
    else if (event->type == SDL_VIDEORESIZE)
    {
        handler->x = (handler->parent->w - handler->w) / 2;
        handler->y = (handler->parent->h - handler->h) / 2;
        return V_EVENT_HANDLED_NOREDRAW;
    }

    return V_EVENT_HANDLED_NOREDRAW;
}



int vultures_eventh_dropdown(struct window* handler, struct window* target,
                             void* result, SDL_Event* event)
{
    /* mousemotion: set the cursor */
    if (event->type == SDL_MOUSEMOTION)
    {
        vultures_set_mcursor(V_CURSOR_NORMAL);
        return V_EVENT_HANDLED_NOREDRAW;
    }

    /* clicks: return id of clicked button */
    else if (event->type == SDL_MOUSEBUTTONUP && event->button.button == SDL_BUTTON_LEFT)
    {
        if (handler != target && target->menu_id)
        {
            *(int*)result = target->menu_id;
            return V_EVENT_HANDLED_FINAL;
        }
    }

    /* keypresses or clicks outside the menu cancel it */
    if (event->type == SDL_KEYDOWN || event->type == SDL_MOUSEBUTTONUP  || event->type == SDL_VIDEORESIZE)
    {
        *(int*)result = 0;
        return V_EVENT_HANDLED_FINAL;
    }

    return V_EVENT_HANDLED_NOREDRAW;
}



/************************************************************
 * button handler
 ************************************************************/

int vultures_eventh_button(struct window* handler, struct window* target,
                           void* result, SDL_Event* event)
{
    if (handler == target && event->type == SDL_MOUSEBUTTONDOWN && event->button.button == SDL_BUTTON_LEFT)
    {
        target->selected = 1;
        target->need_redraw = 1;
    }
    else if (event->type == SDL_MOUSEBUTTONUP || event->type == SDL_MOUSEMOVEOUT)
    {
        if (target->selected)
            target->need_redraw = 1;
        target->selected = 0;
    }
    else if (event->type == SDL_TIMEREVENT)
        /* no need to send these to our parent... */
        return V_EVENT_HANDLED_NOREDRAW;

    if (target->need_redraw)
        return V_EVENT_UNHANDLED_REDRAW;

    return V_EVENT_UNHANDLED;
}



/************************************************************
 * inventory handler
 ************************************************************/

void vultures_update_invscroll(struct window * win, int newpos)
{
    struct window * winelem;
    int itemcount = 0;
    int itemcol;
    int leftoffset = vultures_winelem.border_left->w;
    int topoffset = vultures_winelem.border_top->h;

    topoffset += vultures_get_lineheight(V_FONT_HEADLINE)*2 + 2;

    if (newpos+win->pd.ow_vcols > win->pd.ow_ncols)
        newpos = win->pd.ow_ncols - win->pd.ow_vcols;
    else if (newpos < 0)
        newpos = 0;

    win->pd.ow_firstcol = newpos;

    winelem = win->first_child;
    while (winelem)
    {
        if (winelem->v_type == V_WINTYPE_OBJITEM || winelem->v_type == V_WINTYPE_OBJITEMHEADER)
        {
            itemcol = (itemcount / win->pd.ow_vrows);

            winelem->x = (itemcol - newpos) * (V_LISTITEM_WIDTH + 4) + leftoffset;
            winelem->y = (itemcount % win->pd.ow_vrows) * V_LISTITEM_HEIGHT + topoffset;

            winelem->visible = (itemcol >= newpos && itemcol < newpos + win->pd.ow_vcols);

            itemcount++;
        }

        if (winelem->v_type == V_WINTYPE_BUTTON)
        {
            if (winelem->menu_id == V_INV_PREVPAGE)
                winelem->visible = (newpos != 0);
            else if (winelem->menu_id == V_INV_NEXTPAGE)
                winelem->visible = (newpos+win->pd.ow_vcols < win->pd.ow_ncols);
        }

        winelem = winelem->sib_next;
    }
}


int vultures_inventory_context_menu(struct window * target)
{
    int action = 0, key = 0;
    struct window * menu;

    menu = vultures_create_window_internal(0, NULL, V_WINTYPE_DROPDOWN);
    vultures_create_button(menu, "Apply", V_INVACTION_APPLY);

    if (!target->pd.obj->owornmask)
    {
        /* if you can wear it there's no way you can eat or drink it */
        if (target->pd.obj->oclass == POTION_CLASS)
            vultures_create_button(menu, "Drink", V_INVACTION_DRINK);
        vultures_create_button(menu, "Eat", V_INVACTION_EAT);
    }

    vultures_create_button(menu, "Read", V_INVACTION_READ);

    if (target->pd.obj->oclass == WAND_CLASS)
        vultures_create_button(menu, "Zap", V_INVACTION_ZAP);

    /* you could already be wearing it, then you can't wear it again */
    if (!target->pd.obj->owornmask && target->pd.obj->oclass != WAND_CLASS)
    {
        if (target->pd.obj->oclass != RING_CLASS && target->pd.obj->oclass != AMULET_CLASS)
            vultures_create_button(menu, "Wear", V_INVACTION_WEAR);

        if (target->pd.obj->oclass != ARMOR_CLASS)
            vultures_create_button(menu, "Put on", V_INVACTION_PUT_ON);
    }

    vultures_create_button(menu, "Wield", V_INVACTION_WIELD);

    if (target->pd.obj->owornmask)
        vultures_create_button(menu, "Remove", V_INVACTION_REMOVE);

    if (!target->pd.obj->owornmask)
        vultures_create_button(menu, "Drop", V_INVACTION_DROP);

    if (!objects[target->pd.obj->otyp].oc_name_known)
        vultures_create_button(menu, "Name", V_INVACTION_NAME);

    vultures_layout_dropdown(menu);

    vultures_event_dispatcher(&action, V_RESPOND_INT, menu);

    vultures_destroy_window_internal(menu);

    if (action)
    {
        vultures_eventstack_add('i', -1, -1, V_RESPOND_POSKEY);

        switch (action)
        {
            case V_INVACTION_APPLY: key = 'a'; break;
            case V_INVACTION_DRINK: key = 'q'; break;
            case V_INVACTION_EAT:   key = 'e'; break;
            case V_INVACTION_READ:  key = 'r'; break;
            case V_INVACTION_ZAP:   key = 'z'; break;
            case V_INVACTION_WEAR:  key = 'W'; break;
            case V_INVACTION_PUT_ON:key = 'P'; break;
            case V_INVACTION_WIELD: key = 'w'; break;
            case V_INVACTION_REMOVE:
                /* we call a bunch of functions in do_wear.c directly here;
                    * we can do so safely because take_off() directly accounts for
                    * elapsed turns */
                select_off(target->pd.obj); /* sets takoff_mask */
                if (takeoff_mask)
                {
                    /* default activity for armor and/or accessories,
                        * possibly combined with weapons */
                    disrobing = "disrobing";

                    /* specific activity when handling weapons only */
                    if (!(takeoff_mask & ~(W_WEP|W_SWAPWEP|W_QUIVER)))
                        disrobing = "disarming";

                    (void) take_off();
                }
                /* having performed an action we need to return to the main game loop
                    * so that thing like AC and vision (because of helmets & amulets of ESP)
                    * get recalculated.
                    * However we do not want to perform any more actions or cause messages
                    * to be printed. CTRL+r (redraw) is a suitable NOP */
                key = CTRL('r');
                return V_EVENT_HANDLED_FINAL;

            case V_INVACTION_NAME:
                vultures_eventstack_add(target->menu_id, -1, -1, V_RESPOND_ANY);
                vultures_eventstack_add('n', -1,-1, V_RESPOND_ANY);
                vultures_eventstack_add(META('n'), -1, -1, V_RESPOND_POSKEY);
                return V_EVENT_HANDLED_FINAL;

            case V_INVACTION_DROP:  key = 'd'; break;
        }

        vultures_eventstack_add(target->menu_id, -1, -1, V_RESPOND_CHARACTER);
        vultures_eventstack_add(key, -1, -1, V_RESPOND_POSKEY);

        return V_EVENT_HANDLED_FINAL;
    }

    return V_EVENT_HANDLED_NOREDRAW;
}


int vultures_eventh_inventory(struct window* handler, struct window* target,
                              void* result, SDL_Event* event)
{
    point mouse;
    int key;
    int key_ok;

    switch (event->type)
    {
        case SDL_MOUSEMOTION:
            vultures_set_mcursor(V_CURSOR_NORMAL);
            break;

        case SDL_MOUSEBUTTONUP:
            mouse = vultures_get_mouse_pos();
            /* close the window if the user clicks outside it */
            if (handler == target && (mouse.x < handler->abs_x || mouse.y < handler->abs_y ||
                                      mouse.x > handler->abs_x + handler->w ||
                                      mouse.y > handler->abs_y + handler->h))
                return V_EVENT_HANDLED_FINAL;

            /* left clicks on object items do nothing */
            if (event->button.button == SDL_BUTTON_LEFT &&
                     target != handler && target->v_type == V_WINTYPE_OBJITEM)
                return V_EVENT_HANDLED_NOREDRAW;

            /* right clicks on object items open a context menu */
            else if (event->button.button == SDL_BUTTON_RIGHT && target->v_type == V_WINTYPE_OBJITEM)
                return vultures_inventory_context_menu(target);

            /* other functions are outsourced... */
            else
                return vultures_eventh_objwin(handler, target, result, event);


        case SDL_KEYDOWN:
            key_ok = 0;
            key = event->key.keysym.unicode;
            switch (event->key.keysym.sym)
            {
                case SDLK_HOME:
                case SDLK_END:
                case SDLK_PAGEDOWN:
                case SDLK_KP2:
                case SDLK_DOWN:
                case SDLK_PAGEUP:
                case SDLK_KP8:
                case SDLK_UP:
                case SDLK_LEFT:
                case SDLK_RIGHT:
                    key_ok = 1;
                    break;

                default: break;
            }

            switch (key)
            {
                case MENU_PREVIOUS_PAGE:
                case MENU_NEXT_PAGE:
                case MENU_FIRST_PAGE:
                case MENU_LAST_PAGE:
                case MENU_SEARCH:
                    key_ok = 1;

                default: break;
            }

            if (key_ok)
                return vultures_eventh_objwin(handler, target, result, event);

            else if (!key)
                return V_EVENT_HANDLED_NOREDRAW;

            else
                return V_EVENT_HANDLED_FINAL;

        case SDL_VIDEORESIZE:
            return vultures_eventh_objwin(handler, target, result, event);
    }

    return V_EVENT_HANDLED_NOREDRAW;
}



int vultures_eventh_objwin(struct window* handler, struct window* target,
                              void* result, SDL_Event* event)
{
    struct window * winelem;
    char * str_to_find;
    int key, itemcount, colno;

    switch (event->type)
    {
        case SDL_MOUSEMOTION:
            vultures_set_mcursor(V_CURSOR_NORMAL);
            break;

        case SDL_MOUSEBUTTONUP:
            if (event->button.button == SDL_BUTTON_WHEELUP)
            {
                if (handler->pd.ow_firstcol > 0)
                {
                    /* scroll inventory backwards */
                    vultures_update_invscroll(handler, handler->pd.ow_firstcol - 1);
                    handler->need_redraw = 1;
                    return V_EVENT_HANDLED_REDRAW;
                }
                return V_EVENT_HANDLED_NOREDRAW;
            }

            else if (event->button.button == SDL_BUTTON_WHEELDOWN)
            {
                if (handler->pd.ow_firstcol + handler->pd.ow_vcols < handler->pd.ow_ncols)
                {
                    /* scroll inventory forwards */
                    vultures_update_invscroll(handler, handler->pd.ow_firstcol + 1);
                    handler->need_redraw = 1;
                    return V_EVENT_HANDLED_REDRAW;
                }
                return V_EVENT_HANDLED_NOREDRAW;
            }

            else if (event->button.button == SDL_BUTTON_LEFT)
            {
                if (handler == target)
                    break;

                if (target->v_type == V_WINTYPE_BUTTON)
                {
                    switch (target->menu_id)
                    {
                        case V_MENU_ACCEPT:
                        case V_MENU_CANCEL:
                        case V_INV_CLOSE:
                            *(int*)result = target->menu_id;
                            return V_EVENT_HANDLED_FINAL;

                        case V_INV_PREVPAGE:
                            vultures_update_invscroll(handler, handler->pd.ow_firstcol - 1);
                            handler->need_redraw = 1;
                            return V_EVENT_HANDLED_REDRAW;

                        case V_INV_NEXTPAGE:
                            vultures_update_invscroll(handler, handler->pd.ow_firstcol + 1);
                            handler->need_redraw = 1;
                            return V_EVENT_HANDLED_REDRAW;

                    }
                }

                /* select a range of items from target (clicked item) to handler->pd.ow_lasttoggled (previously clicked item) */
                if (target->v_type == V_WINTYPE_OBJITEM && (SDL_GetModState() & KMOD_LSHIFT) && 
                    handler->select_how != PICK_ONE && handler->pd.ow_lasttoggled)
                {
                    int selectme = 0;
                    for (winelem = handler->first_child; winelem; winelem = winelem->sib_next)
                    {
                        if (winelem == target || winelem == handler->pd.ow_lasttoggled)
                        {
                            selectme = !selectme;
                            winelem->selected = 1;
                            winelem->pd.count = -1;
                        }

                        if (selectme && winelem->v_type == V_WINTYPE_OBJITEM)
                        {
                            winelem->selected = 1;
                            winelem->pd.count = -1;
                        }
                    }

                    handler->pd.ow_lasttoggled->last_toggled = 0;
                    handler->pd.ow_lasttoggled = target;
                    target->last_toggled = 1;

                    handler->need_redraw = 1;
                    return V_EVENT_HANDLED_REDRAW;
                }
                else if (target->v_type == V_WINTYPE_OBJITEM)
                {
                    select_option(handler, target, -1);

                    if (handler->pd.ow_lasttoggled)
                        handler->pd.ow_lasttoggled->last_toggled = 0;
                    handler->pd.ow_lasttoggled = target;
                    target->last_toggled = 1;

                    if (handler->select_how == PICK_ONE)
                    {
                        *(int*)result = V_MENU_ACCEPT;
                        return V_EVENT_HANDLED_FINAL;
                    }

                    handler->need_redraw = 1;
                    return V_EVENT_HANDLED_REDRAW;
                }
            }
            break;

        case SDL_KEYDOWN:
            handler->need_redraw = 1;
            key = event->key.keysym.unicode;
            switch (event->key.keysym.sym)
            {
                case SDLK_RETURN:
                case SDLK_KP_ENTER:
                    *(int*)result = V_MENU_ACCEPT;
                    return V_EVENT_HANDLED_FINAL;

                case SDLK_SPACE:
                case SDLK_ESCAPE:
                    *(int*)result = (handler->content_is_text) ? V_MENU_ACCEPT : V_MENU_CANCEL;
                    return V_EVENT_HANDLED_FINAL;

                /* handle menu control keys */
                case SDLK_HOME:     key = MENU_FIRST_PAGE;    /* '^' */ break;
                case SDLK_END:      key = MENU_LAST_PAGE;     /* '|' */ break;

                /* scroll via arrow keys */
                case SDLK_PAGEDOWN:
                case SDLK_KP2:
                case SDLK_DOWN:
                case SDLK_RIGHT:
                    vultures_update_invscroll(handler, handler->pd.ow_firstcol + 1);
                    return V_EVENT_HANDLED_REDRAW;

                case SDLK_PAGEUP:
                case SDLK_KP8:
                case SDLK_UP:
                case SDLK_LEFT:
                    vultures_update_invscroll(handler, handler->pd.ow_firstcol - 1);
                    return V_EVENT_HANDLED_REDRAW;

                case SDLK_BACKSPACE:
                    if (handler->pd.ow_lasttoggled)
                        handler->pd.ow_lasttoggled->pd.count = handler->pd.ow_lasttoggled->pd.count / 10;
                    return V_EVENT_HANDLED_REDRAW;

                default: break;
            }

            if (!key)
                /* a function or modifier key, but not one we recognize, was pressed */
                return V_EVENT_HANDLED_NOREDRAW;

            switch (key)
            {
                case MENU_PREVIOUS_PAGE:
                    vultures_update_invscroll(handler, handler->pd.ow_firstcol - 1);
                    return V_EVENT_HANDLED_REDRAW;

                case MENU_NEXT_PAGE:
                    vultures_update_invscroll(handler, handler->pd.ow_firstcol + 1);
                    return V_EVENT_HANDLED_REDRAW;

                case MENU_FIRST_PAGE:
                    vultures_update_invscroll(handler, 0);
                    return V_EVENT_HANDLED_REDRAW;

                case MENU_LAST_PAGE:
                    vultures_update_invscroll(handler, 999999);
                    return V_EVENT_HANDLED_REDRAW;


                case MENU_SELECT_ALL:
                case MENU_UNSELECT_ALL:
                    /* invalid for single selection menus */
                    if (handler->select_how == PICK_ONE)
                        return V_EVENT_HANDLED_NOREDRAW;

                    winelem = handler->first_child;
                    while (winelem)
                    {
                        if (winelem->v_type == V_WINTYPE_OBJITEM)
                        {
                            winelem->selected = (key == MENU_SELECT_ALL);
                            winelem->pd.count = -1;
                        }
                        winelem = winelem->sib_next;
                    }
                    return V_EVENT_HANDLED_REDRAW;


                case MENU_INVERT_ALL:
                    /* invalid for single selection menus */
                    if (handler->select_how == PICK_ONE)
                        return V_EVENT_HANDLED_NOREDRAW;

                    winelem = handler->first_child;
                    while (winelem)
                    {
                        if (winelem->v_type == V_WINTYPE_OBJITEM)
                        {
                            winelem->selected = !winelem->selected;
                            winelem->pd.count = -1;
                        }
                        winelem = winelem->sib_next;
                    }
                    return V_EVENT_HANDLED_REDRAW;


                case MENU_SELECT_PAGE:
                case MENU_UNSELECT_PAGE:
                    /* invalid for single selection menus */
                    if (handler->select_how == PICK_ONE)
                        return V_EVENT_HANDLED_NOREDRAW;

                    winelem = handler->first_child;
                    while (winelem)
                    {
                        if (winelem->v_type == V_WINTYPE_OBJITEM && winelem->visible)
                        {
                            winelem->selected = (key == MENU_SELECT_PAGE);
                            winelem->pd.count = -1;
                        }
                        winelem = winelem->sib_next;
                    }
                    return V_EVENT_HANDLED_REDRAW;


                case MENU_INVERT_PAGE:
                    /* invalid for single selection menus */
                    if (handler->select_how == PICK_ONE)
                        return V_EVENT_HANDLED_NOREDRAW;

                    winelem = handler->first_child;
                    while (winelem)
                    {
                        if (winelem->v_type == V_WINTYPE_OBJITEM && winelem->visible)
                        {
                            winelem->selected = !winelem->selected;
                            winelem->pd.count = -1;
                        }
                        winelem = winelem->sib_next;
                    }
                    return V_EVENT_HANDLED_REDRAW;


                case MENU_SEARCH:
                    str_to_find = malloc(512);
                    str_to_find[0] = '\0';
                    if (vultures_get_input(-1, -1, "What are you looking for?", str_to_find) != -1)
                    {
                        itemcount = 0;
                        winelem = handler->first_child;
                        while (winelem)
                        {
                            itemcount++;
                            if (winelem->caption && strstr(winelem->caption, str_to_find))
                            {
                                colno = itemcount / handler->pd.ow_vrows;
                                vultures_update_invscroll(handler, colno);
                                break;
                            }

                            winelem = winelem->sib_next;
                        }

                        if (handler->pd.ow_lasttoggled)
                            handler->pd.ow_lasttoggled->last_toggled = 0;
                        handler->pd.ow_lasttoggled = winelem;
                        if (winelem)
                            winelem->last_toggled = 1;
                    }
                    free(str_to_find);
                    return V_EVENT_HANDLED_REDRAW;

                default:
                    /* numbers are part of a count */
                    if (key >= '0' && key <= '9' && handler->pd.ow_lasttoggled && 
                        handler->pd.ow_lasttoggled->pd.count < 1000000)
                    {
                        if (handler->pd.ow_lasttoggled->pd.count == -1)
                            handler->pd.ow_lasttoggled->pd.count = 0;
                        handler->pd.ow_lasttoggled->pd.count = handler->pd.ow_lasttoggled->pd.count * 10 + (key - '0');

                        return V_EVENT_HANDLED_REDRAW;
                    }

                    /* try to match the key to an accelerator */
                    target = vultures_accel_to_win(handler, key);
                    if (target)
                    {
                        select_option(handler, target, -1);
                        handler->pd.count = 0;
                        if (handler->select_how == PICK_ONE)
                        {
                            *(int*)result = V_MENU_ACCEPT;
                            return V_EVENT_HANDLED_FINAL;
                        }

                        if (handler->pd.ow_lasttoggled)
                            handler->pd.ow_lasttoggled->last_toggled = 0;
                        handler->pd.ow_lasttoggled = target;
                        target->last_toggled = 1;


                        /* if the selected element isn't visible bring it into view */
                        if (!target->visible)
                        {
                            itemcount = 0;
                            winelem = handler->first_child;
                            while (winelem)
                            {
                                itemcount++;
                                winelem = winelem->sib_next;
                                if (winelem == target)
                                    break;
                            }
                            colno = itemcount / handler->pd.ow_vrows;
                            vultures_update_invscroll(handler, colno);
                        }
                        return V_EVENT_HANDLED_REDRAW;
                    }
                    break;
            }
            break;

        case SDL_VIDEORESIZE:
            if (handler->visible)
            {
                /* hide_window takes care of the background */
                vultures_hide_window(handler);

                /* resize */
                vultures_layout_itemwin(handler);

                /* redraw */
                handler->visible = 1;
                handler->need_redraw = 1;
            }
            return V_EVENT_HANDLED_NOREDRAW;
    }

    return V_EVENT_HANDLED_NOREDRAW;
}



int vultures_eventh_objitem(struct window* handler, struct window* target,
                              void* result, SDL_Event* event)
{
    int prevhover = 0;

    switch (event->type)
    {
        case SDL_MOUSEMOTION:
            vultures_set_mcursor(V_CURSOR_NORMAL);

            prevhover = handler->hover;
            handler->hover = 1;

            if (handler->hover != prevhover)
            {
                handler->need_redraw = 1;
                return V_EVENT_HANDLED_REDRAW;
            }
            return V_EVENT_HANDLED_NOREDRAW;

        case SDL_MOUSEMOVEOUT:
            prevhover = handler->hover;
            handler->hover = 0;

            if (handler->hover != prevhover)
            {
                handler->need_redraw = 1;
                return V_EVENT_HANDLED_REDRAW;
            }
            return V_EVENT_HANDLED_NOREDRAW;

        case SDL_MOUSEBUTTONUP:
            if ((event->button.button == SDL_BUTTON_WHEELUP)||
                (event->button.button == SDL_BUTTON_WHEELDOWN))
                handler->hover = 0;
            return V_EVENT_UNHANDLED;
    }

    return V_EVENT_UNHANDLED;
}


/********************************************************************
 * event handling helper functions
 ********************************************************************/


/* find the default button */
static struct window * vultures_find_defbtn(struct window * parent)
{
    struct window * child = parent->first_child;

    while (child)
    {
        if (child->is_default  && child->v_type == V_WINTYPE_BUTTON)
            return child;

        child = child->sib_next;
    }

    return NULL;
}



/* scroll to a position; depending on scrolltype this is either
 * a direction (+- 1 line), a page number, or a vertical pixel offset */
static int vultures_scrollto(struct window * win, int scrolltype, int scrolldir)
{
    struct window *child, *firstvisible, *scrollbar;
    int height = 0;
    int scroll_top, offset, scrollpos;

    offset = vultures_winelem.border_top->h;
    if (win->caption)
        offset += 2 * vultures_text_height(V_FONT_HEADLINE, win->caption);

    scroll_top = offset;

    firstvisible = NULL;
    scrollbar = NULL;
    child = win->first_child;

    /* find the first visible child (menu item or text item),
     * the maximum menu height and the menu's scrollbar */
    while (child)
    {
        if (child->scrollable)
        {
            if (child->visible && !firstvisible)
                firstvisible = child;

            height = (height > (child->y + child->h)) ? height : (child->y + child->h);
        }

        if (child->v_type == V_WINTYPE_SCROLLBAR)
            scrollbar = child;

        child = child->sib_next;
    }

    /* this function can be called by menus without a scrollbar if a
     * scrolling hotkey is pressed or the mousewhell is used over the window */
    if (!scrollbar)
        return V_EVENT_HANDLED_NOREDRAW;

    height += vultures_get_lineheight(V_FONT_MENU);
    height -= offset;


    switch (scrolltype)
    {
        case V_SCROLL_LINE_REL:
            if (scrolldir < 0)
            {
                if (firstvisible->sib_prev && firstvisible->sib_prev->scrollable)
                    scroll_top = firstvisible->sib_prev->y;
            }
            else
            {
                if (firstvisible->sib_next && firstvisible->sib_next->scrollable &&
                    (firstvisible->y-offset < height - V_MENU_MAXHEIGHT))
                    scroll_top = firstvisible->sib_next->y;
                else
                    scroll_top = firstvisible->y;
            }
            break;

        case V_SCROLL_PAGE_REL:
            child = firstvisible;
            if (scrolldir < 0)
            {
                while (child && child->sib_prev && child->scrollable &&
                       child->y > (firstvisible->y - V_MENU_MAXHEIGHT))
                    child = child->sib_prev;

                if (child->y < (firstvisible->y - V_MENU_MAXHEIGHT))
                    scroll_top = child->sib_next->y;
            }
            else
            {
                while (child && child->sib_next && child->scrollable &&
                       child->y < (firstvisible->y + V_MENU_MAXHEIGHT) &&
                       (child->y-offset < height - V_MENU_MAXHEIGHT))
                    child = child->sib_next;

                if (child->y > (firstvisible->y + V_MENU_MAXHEIGHT))
                    scroll_top = child->sib_prev->y;
                else
                    scroll_top = child->y;
            }
            break;

        case V_SCROLL_PAGE_ABS:
        case V_SCROLL_PIXEL_ABS:
            scroll_top = V_MENU_MAXHEIGHT * scrolldir;

            if (scrolltype == V_SCROLL_PIXEL_ABS)
                scroll_top = scrolldir - offset;

            /* find the first menuitem */
            child = win->first_child;
            while (child->sib_next && !child->scrollable)
                child = child->sib_next;

            while (child->sib_next && ((child->y - offset) < scroll_top) &&
                   child->scrollable && (child->y-offset < height - V_MENU_MAXHEIGHT))
                child = child->sib_next;

            if (child->y - offset > scroll_top)
                child = child->sib_prev;

            scroll_top = child->y;
            break;
    }

    scrollpos = ((scroll_top - offset) * 8192.0 / (height - V_MENU_MAXHEIGHT));
    scrollbar->pd.scrollpos = scrollpos;

    win->need_redraw = 1;

    return V_EVENT_HANDLED_REDRAW;
}



/* show or hide the parchment map */
static void vultures_toggle_map(void)
{
    static struct window * map = NULL;
    struct window * txt;

    if (!map)
    {
        map = vultures_create_window_internal(0, NULL, V_WINTYPE_CUSTOM);

        map->autobg = 1;
        map->need_redraw = 1;
        map->draw = vultures_draw_map;
        map->event_handler = vultures_eventh_map;
        map->w = 640;
        map->h = 480;
        map->x = (map->parent->w - map->w) / 2;
        map->y = (map->parent->h - map->h) / 2;

        /* Load map parchment */
        map->image = vultures_load_graphic(NULL, V_FILENAME_MAP_PARCHMENT);

        vultures_winid_map = map->id;

        txt = vultures_create_window_internal(0, map, V_WINTYPE_TEXT);
        txt->x = map->w/2;
        txt->y = 48;
        txt->caption = malloc(128);
        txt->caption[0] = 0;

        vultures_create_hotspot(598, 11, 28, 24, 1, map, "Close map");
        map->last_child->abs_x = map->last_child->x + map->x;
        map->last_child->abs_y = map->last_child->y + map->y;
    }
    else
    {
        vultures_destroy_window_internal(map);
        map = NULL;
        vultures_winid_map = 0;
    }
}

