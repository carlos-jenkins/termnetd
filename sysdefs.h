#ifndef _SYSDEFS_H
#define _SYSDEFS_H

#if defined(FreeBSD) || defined(AIX)
#include <sys/types.h>
#endif

#ifdef SCO
#define _SVID3
#include <sys/types.h>
#include <sys/file.h>
#endif

#ifdef SunOS
#define _SVID3
#include <sys/types.h>
#include <sys/file.h>
#include <strings.h>
#endif

#define TELOPT_BAUDRATE 128
#define TELOPT_PORTSET  129
#define TELOPT_DEVICE   130
#define TELOPT_NDEVICE  131


#if defined(OSF)
#define LOCK_NAME       "/var/spool/locks/LCK..%s"

#elif defined(AIX)
#define LOCK_NAME       "/etc/locks/LCK..%s"

#elif defined(SCO)
#define LOCK_NAME       "/usr/spool/uucp/LCK..%s"

#elif defined(LINUX)
#define LOCK_NAME       "/var/lock/LCK..%s"
#define _GNU_SOURCE
#define NO_ASM

#elif defined(SunOS)
#define LOCK_NAME       "/var/lock/LCK..%s"
#define _GNU_SOURCE
#define INADDR_NONE     ((uint32_t) 0xffffffff)
#define NO_ASM

#elif defined(FreeBSD)
#define LOCK_NAME       "/var/lock/LCK..%s"
#define _GNU_SOURCE
#define _BSD_SOURCE
#define NO_ASM

#endif

/*
** Comment out this define if you do not want to use uucp style device locking
** There is truely no need to change this parameter here. It is now part
** of the configure script.
*/
/* #define UUCP_LOCKING */

/*
** These parameters are here if needed fopr any given OS.
**
** LOCKS_LOWERCASE sets the lock functions to use lowercase
** device names for the lock file even if the device name has uppercase
** letters in it.
**
** LOCKS_BINARY places the pid in the lock file as a binary number
** instead of an ascii string.
*/
/*
#define LOCKS_LOWERCASE
#define LOCKS_BINARY
*/

#endif
