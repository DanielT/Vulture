/* Copyright (c) Daniel Thaler, 2006				  */
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

    return V_EVENT_HANDLED_NOREDRAW;
}


int vultures_eventh_toolbar(struct window* handler, struct window* target,
                           void* result, SDL_Event* event)
{
    switch (event->type)
    {
        case SDL_MOUSEMOTION:
            vultures_set_mcursor(V_CURSOR_NORMAL);
            break;

        case SDL_TIMEREVENT:
            if (event->user.code > HOVERTIMEOUT)
                if (target != handler  && target->caption)
                    vultures_mouse_set_tooltip(target->caption);
            break;

        case SDL_MOUSEBUTTONUP:
            if (event->button.button != SDL_BUTTON_LEFT ||
                target == handler || !target->menu_id)
                break;

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

    map = vultures_get_window(0);
    mouse = vultures_get_mouse_pos();
    new_target = vultures_get_window_from_point(map, mouse);

    return vultures_eventh_level(map, new_target, result, event);
}



int vultures_eventh_enhance(struct window* handler, struct window* target,
                             void* result, SDL_Event* event)
{
    if (event->type == SDL_MOUSEMOTION)
        vultures_set_mcursor(V_CURSOR_NORMAL);
    else if (event->type == SDL_MOUSEBUTTONUP && event->button.button == SDL_BUTTON_LEFT)
    {
        ((vultures_event*)result)->num = META('e');
        return V_EVENT_HANDLED_FINAL;
    }
    else if (event->type == SDL_TIMEREVENT && event->user.code > HOVERTIMEOUT)
        vultures_mouse_set_tooltip("Enhance a skill");
    
    return V_EVENT_HANDLED_NOREDRAW;
}



int vultures_eventh_level(struct window* handler, struct window* target,
                          void* result, SDL_Event* event)
{
    int translated_key, key;
    int action_id, retval, macronum, i;
    point mouse, mappos;
    char * ttext;

    mouse = vultures_get_mouse_pos();
    mappos = vultures_mouse_to_map(mouse);

    switch (event->type)
    {
        case SDL_MOUSEBUTTONUP:
            if (vultures_whatis_active)
            {
                ((vultures_event*)result)->num = 0;
                ((vultures_event*)result)->x = mappos.x;
                ((vultures_event*)result)->y = mappos.y;
                return V_EVENT_HANDLED_FINAL;
            }

            /* else */
            action_id = 0;

            if (event->button.button == SDL_BUTTON_LEFT)
                action_id = vultures_get_map_action(mappos);
            else if (event->button.button == SDL_BUTTON_RIGHT)
                action_id = vultures_get_map_contextmenu(mappos);

            if (action_id)
            {
                retval = vultures_perform_map_action(action_id, mappos);
                if (retval)
                {
                    ((vultures_event*)result)->num = retval;
                    return V_EVENT_HANDLED_FINAL;
                }
            }
            break;

        case SDL_KEYDOWN:
            switch (event->key.keysym.sym)
            {
                case SDLK_TAB:
                    vultures_toggle_map();
                    return V_EVENT_HANDLED_REDRAW;

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
                case SDLK_o:
                    if (!(event->key.keysym.mod & KMOD_SHIFT) || !(event->key.keysym.mod & KMOD_CTRL))
                        break;
                    vultures_iface_opts();
                    return V_EVENT_HANDLED_REDRAW;

                case SDLK_p:
                    if (!(event->key.keysym.mod & KMOD_SHIFT) || !(event->key.keysym.mod & KMOD_CTRL))
                        break;
                    vultures_messages_view();
                    return V_EVENT_HANDLED_REDRAW;

                default:
                    break;
            }

            key = vultures_convertkey_sdl2nh(&event->key.keysym);
            
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
                vultures_set_mcursor(vultures_get_map_cursor(mappos));

            break;

        case SDL_TIMEREVENT:
            /* hovering on a border: scroll */
            if (target != handler && target->menu_id)
            {
                switch (target->menu_id)
                {
                    case V_HOTSPOT_SCROLL_UPLEFT:    vultures_view_x--; break;
                    case V_HOTSPOT_SCROLL_UP:        vultures_view_x--; vultures_view_y--; break;
                    case V_HOTSPOT_SCROLL_UPRIGHT:   vultures_view_y--; break;
                    case V_HOTSPOT_SCROLL_LEFT:      vultures_view_x--; vultures_view_y++; break;
                    case V_HOTSPOT_SCROLL_RIGHT:     vultures_view_x++; vultures_view_y--; break;
                    case V_HOTSPOT_SCROLL_DOWNLEFT:  vultures_view_y++; break;
                    case V_HOTSPOT_SCROLL_DOWN:      vultures_view_x++; vultures_view_y++; break;
                    case V_HOTSPOT_SCROLL_DOWNRIGHT: vultures_view_x++; break;
                }
                event->user.code = 0;

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

        default:
            break;
    }

    return V_EVENT_HANDLED_NOREDRAW;
}



/************************************************************
 * map handlers
 ************************************************************/

int vultures_eventh_map(struct window* handler, struct window* target,
                        void* result, SDL_Event* event)
{
    point mouse, mappos;
    int action_id = 0, retval;
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
            if (handler != target && target->menu_id == 1)
            {
                vultures_toggle_map();
                vultures_mouse_invalidate_tooltip(1);
                return V_EVENT_HANDLED_REDRAW;
            }

            if (mappos.x != -1)
            {
                if (vultures_whatis_active)
                {
                    ((vultures_event*)result)->num = 0;
                    ((vultures_event*)result)->x = mappos.x;
                    ((vultures_event*)result)->y = mappos.y;
                    return V_EVENT_HANDLED_FINAL;
                }

                if (event->button.button == SDL_BUTTON_LEFT)
                    action_id = vultures_get_map_action(mappos);
                else if (event->button.button == SDL_BUTTON_RIGHT)
                    action_id = vultures_get_map_contextmenu(mappos);

                if (action_id)
                {
                    retval = vultures_perform_map_action(action_id, mappos);
                    if (retval)
                    {
                        ((vultures_event*)result)->num = retval;
                        return V_EVENT_HANDLED_FINAL;
                    }
                }
            }
            break;

        case SDL_TIMEREVENT:
            if (event->user.code < HOVERTIMEOUT)
                return V_EVENT_HANDLED_NOREDRAW;
            
            if (handler != target && target->menu_id == 1)
                vultures_mouse_set_tooltip(target->caption);
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
    }

    return V_EVENT_HANDLED_NOREDRAW;
}



int vultures_eventh_minimap(struct window* handler, struct window* target,
                           void* result, SDL_Event* event)
{
    point mouse, mappos;
    int offs_x, offs_y;


    mouse = vultures_get_mouse_pos();

    offs_x = mouse.x - handler->abs_x - 6 - 40;
    offs_y = mouse.y - handler->abs_y - 6;

    mappos.x = ( offs_x + 2*offs_y)/4;
    mappos.y = (-offs_x + 2*offs_y)/4;

    if (mappos.x < 1 || mappos.x > V_MAP_WIDTH ||
        mappos.y < 0 || mappos.y > V_MAP_HEIGHT)
        return V_EVENT_UNHANDLED;


    if (event->type == SDL_MOUSEMOTION)
        vultures_set_mcursor(V_CURSOR_NORMAL);

    else if (event->type == SDL_MOUSEBUTTONUP)
    {
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

    return V_EVENT_HANDLED_NOREDRAW;
}



int vultures_eventh_query_direction(struct window* handler, struct window* target,
                                    void* result, SDL_Event* event)
{
    point mouse;
    int dir_x, dir_y;
    char choice = 0;

    if (event->type == SDL_MOUSEMOTION)
        vultures_set_mcursor(V_CURSOR_NORMAL);

    else if (event->type == SDL_KEYDOWN)
    {
        if (event->key.keysym.sym == SDLK_ESCAPE)
            choice = -1;
        else
            choice = vultures_convertkey_sdl2nh(&event->key.keysym);
    }

    else if (event->type == SDL_MOUSEBUTTONUP && event->button.button == SDL_BUTTON_LEFT)
    {
        if (target == handler->first_child)
        {
            mouse = vultures_get_mouse_pos();
            mouse.x -= (target->abs_x + target->w/2);
            mouse.y -= (target->abs_y + target->h/2);

            dir_x = V_MAP_YMOD * mouse.x + V_MAP_XMOD * mouse.y + V_MAP_XMOD*V_MAP_YMOD;
            dir_x = dir_x / (2 * V_MAP_XMOD * V_MAP_YMOD) - (dir_x < 0);
            dir_y = -V_MAP_YMOD * mouse.x + V_MAP_XMOD * mouse.y + V_MAP_XMOD * V_MAP_YMOD;
            dir_y = dir_y / (2 * V_MAP_XMOD * V_MAP_YMOD) - (dir_y < 0);

            choice = 0;
            if (dir_y == -1)
            {
                if (dir_x == -1) choice = '8';
                else if (dir_x == 0) choice = '9';
                else if (dir_x == 1) choice = '6';
            }
            else if (dir_y == 0)
            {
                if (dir_x == -1) choice = '7';
                else if (dir_x == 0) choice = '.';
                else if (dir_x == 1) choice = '3';
            }
            if (dir_y == 1)
            {
                if (dir_x == -1) choice = '4';
                else if (dir_x == 0) choice = '1';
                else if (dir_x == 1) choice = '2';
            }

            if (dir_x >= 2 && mouse.x < target->w / 2 && mouse.y < target->h / 2)
                choice = '>';

            if (dir_x <= -2 && mouse.x > -target->w / 2 && mouse.y > -target->h / 2)
                choice = '<';
        }
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
    return V_EVENT_HANDLED_NOREDRAW;
}



int vultures_eventh_menu(struct window* handler, struct window* target,
                         void* result, SDL_Event* event)
{
    struct window * winelem;
    point mouse;
    int scrollind_y;
    int key;
    char * str_to_find;

    if (event->type == SDL_MOUSEMOTION)
        vultures_set_mcursor(V_CURSOR_NORMAL);
    else if (event->type == SDL_MOUSEBUTTONUP)
    {
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
            if (handler->select_how == PICK_ONE)
            {
                /* unselect everything else */
                winelem = handler->first_child;
                while (winelem)
                {
                    winelem->selected = 0;
                    winelem = winelem->sib_next;
                }
                target->selected = 1;
                return V_EVENT_HANDLED_FINAL;
            }
            else
                target->selected = !target->selected;

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
        {
            int scrollarea_top = target->abs_y + vultures_winelem.scrollbutton_up->h;
            int scrollarea_bottom = target->abs_y + target->h - vultures_winelem.scrollbutton_down->h;
            int scrollpos;

            scrollind_y = ((scrollarea_bottom - scrollarea_top -
                            vultures_winelem.scroll_indicator->h) *
                            (int)target->pd.scrollpos) / 8192.0;

            mouse = vultures_get_mouse_pos();

            /* click on the scroll-up button */
            if (mouse.y <= scrollarea_top)
                return vultures_scrollto(handler, V_SCROLL_LINE_REL, -1);

            /* click on the scroll-down button */
            else if (mouse.y >= scrollarea_bottom)
                return vultures_scrollto(handler, V_SCROLL_LINE_REL, 1);

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
        }
    }
    else if (event->type == SDL_KEYDOWN)
    {
        handler->need_redraw = 1;
        key = event->key.keysym.unicode;
        switch (event->key.keysym.sym)
        {
            case SDLK_RETURN:
            case SDLK_KP_ENTER:
                *(int*)result = 1;
                return V_EVENT_HANDLED_FINAL;

            case SDLK_SPACE:
            case SDLK_ESCAPE:
                *(int*)result = (handler->content_is_text) ? 1 : -1;
                return V_EVENT_HANDLED_FINAL;

            case SDLK_PAGEUP:   key = MENU_PREVIOUS_PAGE; break;
            case SDLK_PAGEDOWN: key = MENU_NEXT_PAGE;  break;
            case SDLK_HOME:     key = MENU_FIRST_PAGE;  break;
            case SDLK_END:      key = MENU_LAST_PAGE;  break;

            case SDLK_KP2:
            case SDLK_DOWN:
                return vultures_scrollto(handler, V_SCROLL_LINE_REL, 1);

            case SDLK_KP8:
            case SDLK_UP:
                return vultures_scrollto(handler, V_SCROLL_LINE_REL, -1);

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
                if (handler->select_how == PICK_ONE)
                    return V_EVENT_HANDLED_NOREDRAW;
                winelem = handler->first_child;
                while (winelem)
                {
                    if (winelem->v_type == V_WINTYPE_OPTION)
                        winelem->selected = (key == MENU_SELECT_ALL);
                    winelem = winelem->sib_next;
                }
                handler->need_redraw = 1;
                return V_EVENT_HANDLED_REDRAW;

            case MENU_INVERT_ALL:
                if (handler->select_how == PICK_ONE)
                    return V_EVENT_HANDLED_NOREDRAW;
                winelem = handler->first_child;
                while (winelem)
                {
                    if (winelem->v_type == V_WINTYPE_OPTION)
                        winelem->selected = !winelem->selected;
                    winelem = winelem->sib_next;
                }
                handler->need_redraw = 1;
                return V_EVENT_HANDLED_REDRAW;

            case MENU_SELECT_PAGE:
            case MENU_UNSELECT_PAGE:
                if (handler->select_how == PICK_ONE)
                    return V_EVENT_HANDLED_NOREDRAW;
                winelem = handler->first_child;
                while (winelem)
                {
                    if (winelem->v_type == V_WINTYPE_OPTION && winelem->visible)
                        winelem->selected = (key == MENU_SELECT_PAGE);
                    winelem = winelem->sib_next;
                }
                handler->need_redraw = 1;
                return V_EVENT_HANDLED_REDRAW;

            case MENU_INVERT_PAGE:
                if (handler->select_how == PICK_ONE)
                    return V_EVENT_HANDLED_NOREDRAW;
                winelem = handler->first_child;
                while (winelem)
                {
                    if (winelem->v_type == V_WINTYPE_OPTION && winelem->visible)
                        winelem->selected = !winelem->selected;
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
                target = vultures_accel_to_win(handler, key);
                if (target)
                {
                    if (handler->select_how == PICK_ONE)
                    {
                        /* unselect everything else */
                        winelem = handler->first_child;
                        while (winelem)
                        {
                            winelem->selected = 0;
                            winelem = winelem->sib_next;
                        }
                        target->selected = 1;
                        return V_EVENT_HANDLED_FINAL;
                    }
                    else
                        target->selected = !target->selected;

                    /* if the selected element isn't visible bring it into view */
                    if (!target->visible)
                        vultures_scrollto(handler, V_SCROLL_PIXEL_ABS, target->y);

                    handler->need_redraw = 1;
                    return V_EVENT_HANDLED_REDRAW;
                }
                break;
        }
    }

    return V_EVENT_HANDLED_NOREDRAW;
}



int vultures_eventh_input(struct window* handler, struct window* target,
                          void* result, SDL_Event* event)
{
    char * text = handler->first_child->caption; /* input boxes have only one child, so this works */
    int len = strlen(text);

    if (event->type == SDL_MOUSEMOTION)
        vultures_set_mcursor(V_CURSOR_NORMAL);

    else if (event->type == SDL_KEYDOWN)
    {
        switch (event->key.keysym.sym)
        {
            case SDLK_KP_ENTER:
            case SDLK_RETURN:
                *(int*)result = 1;
                return V_EVENT_HANDLED_FINAL;

            case SDLK_ESCAPE:
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
                if (len < 256 && vultures_text_length(V_FONT_MENU, text) <
                   (handler->first_child->w - 10) && isprint(event->key.keysym.unicode))
                {
                    text[len] = (char)event->key.keysym.unicode;
                    text[len+1] = '\0';
                    handler->first_child->need_redraw = 1;
                }
                return V_EVENT_HANDLED_REDRAW;
        }
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
    if (event->type == SDL_KEYDOWN || event->type == SDL_MOUSEBUTTONUP)
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

int vultures_eventh_inventory(struct window* handler, struct window* target,
                              void* result, SDL_Event* event)
{
    int action, key = 0;
    struct window * menu;

    switch (event->type)
    {
        case SDL_MOUSEMOTION:
            vultures_set_mcursor(V_CURSOR_NORMAL);

            if (target != handler && target->accelerator)
                vultures_mouse_set_tooltip(target->caption);
            else
                vultures_mouse_invalidate_tooltip(1);

            break;

        case SDL_MOUSEBUTTONUP:
            if (event->button.button == SDL_BUTTON_WHEELUP)
            {
                if (handler->pd.inv_page > 0)
                {
                    handler->need_redraw = 1;
                    handler->pd.inv_page--;
                    return V_EVENT_HANDLED_REDRAW;
                }
                return V_EVENT_HANDLED_NOREDRAW;
            }

            if (event->button.button == SDL_BUTTON_WHEELDOWN)
            {
                if (handler->last_child->visible)
                { /* handler->last_child is always the "next page" arrow */
                    handler->pd.inv_page++;
                    handler->need_redraw = 1;
                    return V_EVENT_HANDLED_REDRAW;
                }
                return V_EVENT_HANDLED_NOREDRAW;
            }

            if (event->button.button == SDL_BUTTON_RIGHT && target->scrollable)
            {
                action = 0;

                menu = vultures_create_window_internal(0, NULL, V_WINTYPE_DROPDOWN);
                vultures_add_context_action(menu, V_INVACTION_APPLY, "Apply");

                if (!target->pd.obj->owornmask)
                {
                    /* if you can wear it there's no way you can eat or drink it */
                    vultures_add_context_action(menu, V_INVACTION_DRINK, "Drink");
                    vultures_add_context_action(menu, V_INVACTION_EAT, "Eat");
                }

                vultures_add_context_action(menu, V_INVACTION_READ, "Read");

                if (target->pd.obj->oclass == WAND_CLASS)
                    vultures_add_context_action(menu, V_INVACTION_ZAP, "Zap");

                /* you could already be wearing it, then you can't wear it again */
                if (!target->pd.obj->owornmask && target->pd.obj->oclass != WAND_CLASS)
                {
                    if (target->pd.obj->oclass != RING_CLASS && target->pd.obj->oclass != AMULET_CLASS)
                        vultures_add_context_action(menu, V_INVACTION_WEAR, "Wear");

                    if (target->pd.obj->oclass != ARMOR_CLASS)
                        vultures_add_context_action(menu, V_INVACTION_PUT_ON, "Put on");
                }

                vultures_add_context_action(menu, V_INVACTION_WIELD, "Wield");

                if (target->pd.obj->owornmask)
                    vultures_add_context_action(menu, V_INVACTION_REMOVE, "Remove");

                if (!target->pd.obj->owornmask)
                    vultures_add_context_action(menu, V_INVACTION_DROP, "Drop");

                if (!objects[target->pd.obj->otyp].oc_name_known)
                    vultures_add_context_action(menu, V_INVACTION_NAME, "Name");

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
            }
            else if (event->button.button == SDL_BUTTON_LEFT &&
                     target != handler && !target->scrollable && target->visible)
            {
                if (target->menu_id == 1)
                    handler->pd.inv_page--;
                if (target->menu_id == 2)
                    handler->pd.inv_page++;

                handler->need_redraw = 1;
                return V_EVENT_HANDLED_REDRAW;
            }
            else
                return V_EVENT_HANDLED_FINAL;

        case SDL_KEYDOWN:
            if (vultures_convertkey_sdl2nh(&event->key.keysym))
                return V_EVENT_HANDLED_FINAL;
            break;
    }
    return V_EVENT_HANDLED_NOREDRAW;
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
        map->pd_type = 1;
        map->pd.image = vultures_load_graphic(NULL, V_FILENAME_MAP_PARCHMENT);
        
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



/* Rotate numpad keys for movement */
int vultures_translate_key(int cmd_key)
{
    static int vultures_last_translated_key = 0;

    /* Count keys aren't translated */
    if (vultures_last_translated_key == 'n' && isdigit(cmd_key))
        return cmd_key;

    vultures_last_translated_key = cmd_key;
    
    if (vultures_opts.no_key_translation)
        return cmd_key;

    switch (cmd_key)
    {
        case '1': return '2';
        case '2': return '3';
        case '3': return '6';
        case '4': return '1';
        case '6': return '9';
        case '7': return '4';
        case '8': return '7';
        case '9': return '8';
    }

    return cmd_key;
}
