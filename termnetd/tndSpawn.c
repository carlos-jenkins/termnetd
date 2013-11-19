/***************************************************************************
**    tndSpawn.c
**    Child Proccess Spawning Funtions
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
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <syslog.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#if defined(AIX)
#include <sys/select.h>
#endif
#include "termnetd.h"

static int dbg = 0;

void spawnConnection(int sock, portConf *pDevice)
{
   portConf *pEntry, devc;

   if (dbg) syslog(LOG_DEBUG, "spawnConnection():Opening device: %s",
                              pDevice->devc);
   /*
   ** Spawn the child process, the parent just needs to
   ** return at this point
   */
   if ((pDevice->pid = fork()) < 0)
   /*
   ** If child could not be spawned, log the error and return
   */
   {
      syslog(LOG_ERR, "Error Spawning child for port %d, device %s:%m",
                      sock, pDevice->devc);
      closeModem(pDevice);
   }
   else if (pDevice->pid == 0)
   {
      openlog("termnetd", LOG_PID | LOG_CONS, LOG_DAEMON);
      /*
      ** Close all sockets opened for listening so the child only
      ** has the socket for the client and the modem device opened
      */
      for (pEntry = pPortList; pEntry != NULL; pEntry = pEntry->pNext)
         if (pEntry->enabled &&
             findDevc(pPortList, pEntry->devc) == pEntry)
            close(pDevice->sock);

      /*
      ** Duplicate the file handle for the socket connection on 0
      ** If the device happens to be using that handle, movit it out
      ** of the way
      */
      if (dup2(sock, 0) < 0)
      {
         syslog(LOG_ERR, "Error juggling file descripters for port %d device %s:%m",
                         sock, pDevice->devc);
         exit(1);
      }
      else
         close(sock);
      /*
      ** Go do it!!!
      */
      devc = *pDevice;
      if (dbg) syslog(LOG_DEBUG, "Calling termnetd!");
      sleep(1);
      termnetd(&devc);
      if (dbg) syslog(LOG_DEBUG, "returned from termnetd!");
      exit(1);
   }
   else
      close(sock);
}
