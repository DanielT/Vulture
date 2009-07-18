#ifdef VULTURESEYE
# define VULTURES_PORT_ID_NAME "Eye"
#else
# ifdef VULTURESCLAW
#  define VULTURES_PORT_ID_NAME "Claw"
# else
#  define VULTURES_PORT_ID_NAME "Marula"
# endif
#endif
#include "vultures_port_version.h"
#define VULTURES_SUB_ID "[Vultures " VULTURES_PORT_ID_NAME " - " VULTURES_PORT_VERSION "]"
