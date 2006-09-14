#ifndef _jtp_def_h_
#define _jtp_def_h_

#define _BORLAND_DOS_REGS 1  // djgpp specific switch

typedef signed char     jtp_schar;
typedef unsigned char   jtp_uchar;

#ifdef __386__

typedef signed char     jtp_sint1;
typedef unsigned char   jtp_uint1;
typedef signed short    jtp_sint2;
typedef unsigned short  jtp_uint2;
typedef signed int      jtp_sint4;
typedef unsigned int    jtp_uint4;

#else

typedef signed char     jtp_sint1;
typedef unsigned char   jtp_uint1;
typedef signed int      jtp_sint2;
typedef unsigned int    jtp_uint2;
typedef signed long     jtp_sint4;
typedef unsigned long   jtp_uint4;

#endif



#endif
