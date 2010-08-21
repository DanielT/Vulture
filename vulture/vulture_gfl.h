/* NetHack may be freely redistributed under the Nethack General Public License
 * See nethack/dat/license for details */

#ifndef _vulture_gfl_h_
#define _vulture_gfl_h_

#include <SDL.h>

#include <string>

extern SDL_Surface *vulture_load_surface(char *srcbuf, unsigned int buflen);
extern SDL_Surface *vulture_load_graphic(std::string name);

extern void vulture_save_png(SDL_Surface * surface, std::string filename, int with_alpha);

extern void vulture_save_screenshot(void);

#endif
