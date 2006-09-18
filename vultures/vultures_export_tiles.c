#include "vultures_conf.h"

#ifdef EXPORT_TILES

#include "vultures_types.h"
#include "vultures_tile.h"
#include "vultures_gametiles.h"
#include "vultures_gfl.h"

#include "hack.h"

#ifdef VULTURESEYE
# include "vultures_tilename_reference_nethack.h"
#endif
#ifdef VULTURESCLAW
# include "vultures_tilename_reference_slashem.h"
#endif

extern short glyph2tile[];

void vultures_export_tiles()
{
    int mon_id;
    int obj_id;

    int max_width = 0;
    int max_height = 0;

    int offset_x = 0;
    int offset_y = 0;

    /* vultures_put_img(x + tile->xmod, y + tile->ymod, tile->graphic); */

    void recalculate_bounds( vultures_tile *tile )
    {
        int x,y;

        x = max_width/2+tile->xmod+offset_x;
        y = max_height/2+tile->ymod+offset_y;

        if (x<0)
        {
            max_width-=x;
            offset_x-=x;
        }
        if (y<0)
        {
            max_height-=y;
            offset_y-=y;
        }

        x = (max_width/2+tile->xmod+offset_x)+tile->graphic->w - max_width;
        y = (max_height/2+tile->ymod+offset_y)+tile->graphic->h - max_height;

        //printf("=> %5d %5d\n",x,y);

        if ( x>0 )
        {
            max_width += x;
            offset_x -= x;
        }
        if ( y>0 )
        {
            max_height += y;
            offset_y -= y;
        }
        //printf("* %5d %5d\n",max_width,max_height);
    }

    void save_tile( int tile_enum, vultures_tile *tile )
    {
        SDL_Surface *surface;
        SDL_Rect toRect;
        char filename[200];

        sprintf( filename, "%s.png", tilename_reference[tile_enum]);
        surface = SDL_CreateRGBSurface( SDL_SWSURFACE,
                max_width, max_height,
                tile->graphic->format->BitsPerPixel,
                tile->graphic->format->Rmask, tile->graphic->format->Gmask, tile->graphic->format->Bmask, tile->graphic->format->Amask);
        toRect.x = max_width/2+tile->xmod+offset_x;
        toRect.y = max_height/2+tile->ymod+offset_y;
        printf("Saving %s (%d,%d)\n", filename, toRect.x, toRect.y);
        toRect.w = tile->graphic->w;
        toRect.h = tile->graphic->h;
        SDL_SetAlpha( tile->graphic, 0, SDL_ALPHA_OPAQUE);
        SDL_BlitSurface( tile->graphic, NULL, surface, &toRect );
        vultures_save_png( surface, filename, TRUE );
    }

    /* Calculate maximum width and height for resulting per-tile dimensions */
    for ( mon_id=0 ; mon_id < NUMMONS ; mon_id++ )
        recalculate_bounds( vultures_get_tile( MONSTER_TO_VTILE(mon_id) ) );

    for ( obj_id=0 ; obj_id < NUM_OBJECTS ; obj_id++ )
        recalculate_bounds( vultures_get_tile( OBJECT_TO_VTILE(obj_id) ) );

    /* Save each monster's tile */
    for ( mon_id=0 ; mon_id < NUMMONS ; mon_id++ )
        if (MONSTER_TO_VTILE(mon_id) != V_TILE_NONE)
            save_tile( glyph2tile[ monnum_to_glyph( mon_id ) ], vultures_get_tile( MONSTER_TO_VTILE(mon_id) ) );

    /* Save each object's tile */
    for ( obj_id=0 ; obj_id < NUM_OBJECTS ; obj_id++ )
        if (OBJECT_TO_VTILE(obj_id) != V_TILE_NONE)
            save_tile( glyph2tile[ objnum_to_glyph( obj_id ) ], vultures_get_tile( OBJECT_TO_VTILE(obj_id) ) );

    printf("\nTiles are placed at on offset of (%d,%d) from the centre within the dimensions (%d,%d)\n",offset_x,offset_y,max_width,max_height);

    exit( -1 );
}

#endif
