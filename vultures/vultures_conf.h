/* Vultures specific config options */

/* SUSPEND is defined on unix, but we don't want it, because it makes no sense
 * for a graphical interface which one can simply tab out of. Also, it doesn't
 * work anyway if we were started from the K-menu or some other graphical means */
#undef SUSPEND
