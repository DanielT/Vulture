/* Vultures specific config options */

/* SUSPEND is defined on unix, but we don't want it, because it makes no sense
 * for a graphical interface which one can simply tab out of. Also, it doesn't
 * work anyway if we were started from the K-menu or some other graphical means */
#undef SUSPEND

/* PCMUSIC allows us to play notes when the player uses an instrument ingame
 * nethack only defines this if __BORLANDC__ is also defined, but we can use it 
 * anyway */ 
#define PCMUSIC

