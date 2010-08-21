/* Vulture specific config options */

/* SUSPEND is defined on unix, but we don't want it, because it makes no sense
 * for a graphical interface which one can simply tab out of. Also, it doesn't
 * work anyway if we were started from the K-menu or some other graphical means */
#undef SUSPEND

/* PCMUSIC allows us to play notes when the player uses an instrument ingame
 * nethack only defines this if __BORLANDC__ is also defined, but we can use it 
 * anyway */ 
#define PCMUSIC

/* RELOCATEABLE will make Vulture try to determine where the binary is located
 * and chdir into that directory. This is supposed to make installations of Vulture
 * relocateable.
 *
 * On linux and macosx OS-specific means of determining the path will be used;
 * all other unixes must have a full path in argv[0] for this code to work.
 *
 * On unix it is considered to be Bad Taste(tm) to put the executable and data
 * into the same directory, so if you aren't building a redistributable binary package
 * do yourself a favour and set up HACKDIR and CHDIR correctly instead of messing
 * with this.
 *
 * RELOCATEABLE has no effect on Windows or if CHDIR is defined in include/config.h 
 * RELOCATEABLE is *EXPERIMENTAL*; report problems on the mailing list */
#undef RELOCATEABLE


/* Data librarian.  Defining DLB places most of the support files into
 * a tar-like file, thus making a neater installation.  See *conf.h
 * or detailed configuration. 
 * We enable this here, regardless of the setting in include/config.h
 * because DLB _is_ supported on all platforms capable of running vulture*/
#define DLB
