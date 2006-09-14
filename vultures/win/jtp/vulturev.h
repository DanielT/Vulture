#ifdef VULTURESEYE
# define JTP_PORT_ID_NAME "Eye"
#else
# ifdef VULTURESCLAW
#  define JTP_PORT_ID_NAME "Claw"
# else
#  define JTP_PORT_ID_NAME "Marula"
# endif
#endif
#include "vultureversion.h"
#define JTP_SUB_ID "[Vultures " JTP_PORT_ID_NAME " - " JTP_PORT_VERSION "]"
