/***************************************************************************
**    tndLock.c
**    Termnetd Device Locking Functions
**
** Copyright (C) 1996      Joseph Croft <joe@croftj.net>  
** 
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 1, or (at your option)
** any later version.
** 
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
** 
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
 ***************************************************************************/
#include "../sysdefs.h"
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <setjmp.h>
#include <signal.h>
#include <syslog.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include "termnetd.h"

static int dbg = 2;

#ifdef UUCP_LOCKING

static char *get_lock_name (char *lock_name, char *device)
{
#ifdef LOCKS_LOWERCASE
   /* sco locking convention -> change all device names to lowercase */

   char p[MAXLINE+1];
   int i;
   if ( ( i = strlen( device ) ) > sizeof(p) )
   {
     if (dbg) syslog(LOG_DEBUG, "lockDevc():get_lock_name: device name too long" );
     exit(5);
   }

#ifdef LOCKS_ALL_LOWERCASE
   /* convert the full name */
   while ( i >= 0 )
   {
     p[i] = tolower( device[i] ); i--;
   }
#else
   /* convert only the last character */
   strcpy( p, device );
   i--;
   p[i] = tolower( p[i] );
#endif

   device = p;
#endif  /* LOCKS_LOWERCASE */

   /* throw out all directory prefixes */
   if ( strchr( device, '/' ) != NULL )
     device = strrchr( device, '/' ) +1;

   sprintf( lock_name, LOCK_NAME, device);

   if (dbg) syslog(LOG_DEBUG, "lockDevc():get_lock_name: Have lock name: |%s|", lock_name);
   return lock_name;
}

static int readlock (char *name)
{
   int fd, pid;
   char apid[20];
   int  length;

   if ((fd = open(name, O_RDONLY)) == 0)
           return(0);

   length = read(fd, apid, sizeof(apid)-1);
   apid[length]=0;         /* make sscanf() happy */

   pid = 0;
   if ( length == sizeof( pid ) || sscanf(apid, "%d", &pid) != 1 ||
        pid == 0 )
   {
      pid = * ( (int *) apid );
#if LOCKS_BINARY == 0
      if (dbg) syslog(LOG_DEBUG, "lockDevc():compiled with ascii locks, found binary lock file (length=%d, pid=%d)!", length, pid );
#endif
   }
#if LOCKS_BINARY == 1
   else
   {
      if (dbg) syslog(LOG_DEBUG, "compiled with binary locks, found ascii lock file (length=%d, pid=%d)!", length, pid );
   }
#endif

   (void) close(fd);
   return(pid);
}

static int writeLockFile(devcLock *pDevLock)
{
#ifdef LOCKS_BINARY
   int bpid;               /* must be 4 bytes wide! */
#else
   char apid[16];
#endif
   int fd, pid;
   char *temp, buf[1025];

   get_lock_name(pDevLock->lock, pDevLock->pDevc->devc);

   /* first make a temp file */
   (void) sprintf(buf, LOCK_NAME, "TM.XXXXXX");
   if ((fd = creat((temp=mktemp(buf)), 0644)) == -1)
   {
      return(-1);
   }

   /* just in case some "umask" is set (errors are ignored) */
   chmod( temp, 0644 );

   /* put my pid in it */

#ifdef LOCKS_BINARY
   bpid = getpid();
   if ( write(fd, &bpid, sizeof(bpid) ) != sizeof(bpid) )
#else
   sprintf( apid, "%10d\n", (int) getpid() );
   if ( write(fd, apid, strlen(apid)) != strlen(apid) )
#endif
   {
      if (dbg) syslog(LOG_DEBUG, "writeLockFile():cannot write PID to (temp) lock file");
      close(fd);
      unlink(temp);
   }
   else
      close(fd);

   while (link(temp, pDevLock->lock) == -1)
   {
      if (errno != EEXIST )
      {
         if (dbg) syslog(LOG_DEBUG, "writeLockFile():lock not made: link(temp,lock) failed");
      }

      if (errno == EEXIST)         /* lock file already there */
      {
         if ((pid = readlock(pDevLock->lock)) == -1)
         {
            if ( errno == ENOENT )  /* disappeared */
               continue;
            else
            {
               if (dbg) syslog(LOG_DEBUG, "writeLockFile():cannot read lockfile");
               unlink(temp);
               return -1;
            }
         }

         if (pid == getpid())      /* huh? WE locked the line!*/
         {
            if (dbg) syslog(LOG_DEBUG, "writeLockFile():we *have* the line!" );
            unlink(temp);
            return 0;
         }

         if ((kill(pid, 0) == -1) && errno == ESRCH)
         {
            /* pid that created lockfile is gone */
            if (dbg) syslog(LOG_DEBUG, "writeLockFile():stale lockfile, created by process %d, ignoring", pid );
            if ( unlink(pDevLock->lock) < 0 &&
                   errno != EINTR && errno != ENOENT )
            {
               if (dbg) syslog(LOG_DEBUG, "writeLockFile():unlink() failed, giving up" );
               unlink(temp);
               return -1;
            }
            continue;
         }

         if (dbg) syslog(LOG_DEBUG, "writeLockFile():lock not made: lock file exists");
      }

      (void) unlink(temp);
      return(-1);
   }
   if (dbg) syslog(LOG_DEBUG, "writeLockFile():lock made");
   (void) unlink(temp);
   return(0);
}
#endif /* UUCP_LOCKING */

int lockDevc(portConf *pDevc)
{
   devcLock *pEntry;
   int err = 0, rv = -1;
   jmp_buf mEnv;

   if (dbg) syslog(LOG_DEBUG, "lockDevc():Enter");
   if (!isLocked(pDevc->devc))
   {
      if (dbg) syslog(LOG_DEBUG, "lockDevc():Device %s not locked",
                      pDevc->devc);
      if ((err = setjmp(mEnv)) == 0)
      {
         if (dbg) syslog(LOG_DEBUG, "lockDevc():Allocating Lock struct");
         if ((pEntry = (devcLock *)malloc(sizeof(devcLock))) != NULL)
         {
            if (dbg) syslog(LOG_DEBUG, "lockDevc():Locking Device %s",
                                       pDevc->devc);
            pEntry->delete = 0;
            pEntry->pDevc = pDevc; 
#ifdef UUCP_LOCKING
            if (!writeLockFile(pEntry))
            {
#endif
               pEntry->pNext = pLockList;
               pEntry->pPrev = NULL;
               if (pLockList)
                  pLockList->pPrev = pEntry;
               pLockList = pEntry;
               rv = 0;
#ifdef UUCP_LOCKING
            }
            else
               free(pEntry);
#endif
         }
         else
            err = -1;
      }
      else if (err != 0)
      {
         syslog(LOG_ERR, "Quiting: Out of memory!!!!\n");
         exit(1);
      }
   }
   else
      rv = -1;
   if (dbg) syslog(LOG_DEBUG, "lockDevc(%d):Exit", rv);
   return(rv);
}

void chekDevc()
{
   devcLock *pEntry, *pTmp;

   if (dbg > 2) syslog(LOG_DEBUG, "chekDevc():Enter");
   for (pEntry = pLockList; pEntry != NULL;)
   {
      if (dbg > 1) syslog(LOG_DEBUG, "chekDevc():Testing Entries %d, %d",
                      pEntry->pDevc->pid, pEntry->delete);
      if (waitpid(pEntry->pDevc->pid, NULL, WNOHANG) != 0)
      {
         if (dbg > 1) syslog(LOG_DEBUG, "chekDevc():Pid %d dead!", pEntry->pDevc->pid);
         pEntry->pDevc->pid = 0;
#ifdef UUCP_LOCKING
         if (unlink(pEntry->lock) < 0)
            if (dbg) syslog(LOG_DEBUG, "error removing lock file:%m");
#endif
         pTmp = pEntry->pNext;
         if (pEntry->pNext != NULL)
         {
            pEntry->pNext->pPrev = pEntry->pPrev;
         }
         if (pEntry->pPrev != NULL)
         {
            pEntry->pPrev->pNext = pEntry->pNext;
         }
         if (pEntry == pLockList)
            pLockList = pEntry->pNext;
         free(pEntry);
         pEntry = pTmp;
      }
      else
      {
         pEntry = pEntry->pNext;
      }
   }
   if (dbg > 2) syslog(LOG_DEBUG, "chekDevc():Exit");
}

int isLocked(const char *pDevc)
{
   devcLock *pEntry;
   int rv = 0;

   if (dbg) syslog(LOG_DEBUG, "isLocked():Enter");
   for (pEntry = pLockList; pEntry != NULL; pEntry = pEntry->pNext)
   {
      if (dbg) syslog(LOG_DEBUG, "isLocked():Testing entries |%s| & |%s|",
                       pEntry->pDevc->devc, pDevc);
      if (strcmp(pEntry->pDevc->devc, pDevc) == 0)
      {
         if (dbg) syslog(LOG_DEBUG, "isLocked():Have Match!");
         rv = 1;
         break;
      }
   }
   if (dbg) syslog(LOG_DEBUG, "isLocked():Exit(%d)", rv);
   return(rv);
}
