#ifdef VULTURE_NETHACK
# define VULTURE_PORT_ID_NAME "NetHack"
#else
# ifdef VULTURE_SLASHEM
#  define VULTURE_PORT_ID_NAME "SlashEM"
# else
#  ifdef VULTURE_UNNETHACK
#   define VULTURE_PORT_ID_NAME "UnNetHack"
#  else
#   define VULTURE_PORT_ID_NAME "Something"
#  endif
# endif
#endif
#include "vulture_port_version.h"
#define VULTURE_SUB_ID "[Vulture for " VULTURE_PORT_ID_NAME " - " VULTURE_PORT_VERSION "]"
