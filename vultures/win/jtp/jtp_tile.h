/*
 * Magic colors used in the tile files
 */
/*
 * This one is only used to draw the border around the
 * tile. It is checked in the conversion utility
 * (and when SANITY_CHECKS is defined)
 * to verify that the correct dimensions have been,
 * specified but isn't used otherwise.
 */
#define JTP_COLOR_BORDER  79
/*
 * This one is used to mark the hotspot for mouse cursors,
 * and also is used by other tiles to mark the center.
 * If you change this, you also have to change all tile files.
 */
#define JTP_COLOR_HOTSPOT 16
/*
 * The transparent background pixel. This should not be changed.
 */
#define JTP_COLOR_BACKGROUND 0
/*
 * colors used to draw text
 */
#define JTP_COLOR_TEXT 15
#define JTP_COLOR_INTRO_TEXT 255
/*
 * colors used to draw the mini-map
 */
#define JTP_COLOR_MINI_CORRIDOR 238
#define JTP_COLOR_MINI_STAIRS   165
#define JTP_COLOR_MINI_DOOR      96
#define JTP_COLOR_MINI_FLOOR    236
#define JTP_COLOR_MINI_YOU       15


/*
 * Default tiles to use, when there is no special tile
 */
#define JTP_TILE_DEFAULT_MONSTER JTP_TILE_KNIGHT
#define JTP_TILE_DEFAULT_OBJECT  JTP_TILE_MISC
#define JTP_DEFAULT_FLOOR_STYLE JTP_FLOOR_STYLE_COBBLESTONE
#define JTP_DEFAULT_FLOOR_EDGE_STYLE JTP_FLOOR_EDGE_STYLE_COBBLESTONE
