/* NetHack may be freely redistributed under the Nethack General Public License
 * See nethack/dat/license for details */

#ifndef _vultures_gfl_h_
#define _vultures_gfl_h_

#include <SDL.h>

#include <string>
using std::string;

extern SDL_Surface *vultures_load_surface(char *srcbuf, unsigned int buflen);
extern SDL_Surface *vultures_load_graphic(string name);

extern void vultures_save_png(SDL_Surface * surface, string filename, int with_alpha);

extern void vultures_save_screenshot(void);

#endif
