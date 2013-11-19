/***************************************************************************
**    tndSocket.c
**    Termnetd Socket Handling code
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
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>                                                                                                             
#include <netdb.h>
#include <sys/socket.h>
#include <signal.h>
#include <stdlib.h>
#if defined(FreeBSD)
#include <string.h>
#endif
#include <syslog.h>
#include "termnetd.h"

#if defined(AIX)
#include <sys/select.h>
#endif

/*
// Seems newer SuSE linux does not define timeval
// outside of linux/time.h
#if defined(LINUX)
#include <linux/time.h>
#endif
*/

static int dbg = 0;

void closeSockets()
{
   portConf *pEntry;
   devcLock *pLock;

   if (ControlSock >= 0) 
   {
      close(ControlSock);
      ControlSock = -1;
   }

   for (pLock = pLockList; pLock != NULL; pLock = pLock->pNext)
   {
      closeModem(pLock->pDevc);
      kill(pLock->pDevc->pid, SIGINT);
   }

   for (pEntry = pPortList; pEntry != NULL; pEntry = pEntry->pNext)
   {
      if (dbg) syslog(LOG_DEBUG, "closeSockets():Next Entry");
      /*
      ** We only need to close the socket for the first one of a port
      */
      if (pEntry->sock >= 0 && findPortByName(NULL , pEntry->srvc) == pEntry)
      {
         close(pEntry->sock);
         pEntry->sock = -1;
      }
      pEntry->sock = -1;
   }
}

/*
** openSockets()
**
** This function scans pPortList and opens sockets for all of the enabled
** entries. If a device is already opened and retry = 0, it's current socket
** will be closed first.
**
** If retry != 0 on entry, an attempt will be made to open those entries 
** whose socket < 0 (pEntry->sock) without closing any sockets already opened.
** This should solve the problem of "dangling" tcp connections.
**
** If an error occurs while attempting to open or bind a socket, an alarm
** will be set for 5 seconds to retry the port.
*/
void openSockets(int retry)
{
   struct servent *se;
   struct sockaddr_in addr;
   portConf *pEntry, *pEntry1;
   int port = 0, defered = 0;

   if (dbg) syslog(LOG_DEBUG, "openSockets(%d):Enter", retry);
   if (retry != 0)
   {
      if (dbg) syslog(LOG_DEBUG, "openSockets():Retrying Connections");
   }
   else
   {
      if (dbg) syslog(LOG_DEBUG, "openSockets():Phase 1");
   }

   if (ControlSock >= 0) 
   {
      close(ControlSock);
      ControlSock = -1;
   }

   for (pEntry = pPortList;
        retry == 0 && pEntry != NULL;
        pEntry = pEntry->pNext)
   {
      if (dbg) syslog(LOG_DEBUG, "openSockets():Closing Next Entry");
      /*
      ** We only need to close the socket for the first one of a port
      */
      if (pEntry->sock >= 0 && findPortByName(NULL , pEntry->srvc) == pEntry)
      {
         if (dbg) syslog(LOG_DEBUG, "openSockets():Closing Socket %d!",
                                    pEntry->sock);
         close(pEntry->sock);
         pEntry->sock = -1;
      }
      pEntry->sock = -1;
   }

   if (strlen(ControlPort) > 0) 
   {
      syslog(LOG_DEBUG, "openSockets():getting host entry for the control port %s",
             ControlPort);
      if ((se = getservbyname(ControlPort, "tcp")) != NULL)
         port = se->s_port;
      else
         port = htons((unsigned short)atoi(ControlPort));
      syslog(LOG_DEBUG, "openSockets():Control Port = %d", ntohs(port));
      addr.sin_family = AF_INET;
      addr.sin_port = port;
      addr.sin_addr.s_addr = htonl(INADDR_ANY);
      if ((ControlSock = socket(AF_INET, SOCK_STREAM, PF_UNSPEC)) >= 0)
      {
         if (bind(ControlSock,
                  (struct sockaddr *)&addr, sizeof(addr)) >= 0)
         {
            syslog(LOG_DEBUG, "openSockets():Listening on Control Port");
            ControlAddr = addr;
            listen(ControlSock, 5);
         }
         else
         {
            syslog(LOG_WARNING, "Error Binding socket for control port %d:%m",
                            ntohs(port));
            close(ControlSock);
            ControlSock = -1;
         }
      }
   }
   else
   {
      syslog(LOG_DEBUG, "openSockets():No Control Port!!");
      ControlSock = -1;
   }

   /*
   ** Here we open all of the Actual ports to be listened to
   */
   if (dbg) syslog(LOG_DEBUG, "openSockets():Phase 2");
   for (pEntry = pPortList; pEntry != NULL; pEntry = pEntry->pNext)
   {
      if (dbg) syslog(LOG_DEBUG, "openSockets():Next Entry");
      /*
      ** Only open a socket for a device if the device is enabled and
      ** this is the first device for the service port!
      */
      if (pEntry->enabled != 0 && 
          findPortByName(NULL, pEntry->srvc) == pEntry &&
          pEntry->sock < 0)
      {
         if (dbg) syslog(LOG_DEBUG, "openSockets():getting host entry for %s",
                                    pEntry->srvc);
         if ((se = getservbyname(pEntry->srvc, "tcp")) != NULL)
            port = se->s_port;
         else
            port = htons((unsigned short)atoi(pEntry->srvc));
         if (dbg) syslog(LOG_DEBUG, "openSockets():Port = %d", ntohs(port));
         addr.sin_family = AF_INET;
         addr.sin_port = port;
         addr.sin_addr.s_addr = htonl(INADDR_ANY);
         if ((pEntry->sock = socket(AF_INET, SOCK_STREAM, PF_UNSPEC)) >= 0)
         {
            int opt = 1;
            if (dbg) syslog(LOG_DEBUG, "openSockets():Binding name to socket");
            if (setsockopt(pEntry->sock, SOL_SOCKET, SO_REUSEADDR, 
                           &opt, sizeof(opt)) < 0)
               syslog(LOG_ERR, "Error setting socket option:%m");
            if (bind(pEntry->sock,
                     (struct sockaddr *)&addr, sizeof(addr)) >= 0)
            {
               if (dbg) syslog(LOG_DEBUG, "openSockets():Listening");
               pEntry->addr = addr;
               listen(pEntry->sock, 5);
            }
            else
            {
               syslog(LOG_WARNING, "Error Binding socket at port %d, device: %s:%m",
                               ntohs(port), pEntry->devc);
               close(pEntry->sock);
               pEntry->sock = -1;
            }
         }
         else
         {
            syslog(LOG_WARNING, "Error Opening socket for port %d, device: %s:%m",
                            ntohs(port), pEntry->devc);
            close(pEntry->sock);
            pEntry->sock = -1;
         }
         if (pEntry->sock < 0)
         {
            syslog(LOG_WARNING, "Opening defered 5 seconds, port: %d, device: %s:",
                            ntohs(port), pEntry->devc);
            defered = 1;
         }
      }
      else if (pEntry->enabled)
      {
         if (dbg) syslog(LOG_DEBUG, "openSockets():");
         if ((pEntry1 = findPortByName(NULL, pEntry->srvc)) != NULL)
         {
            if ((pEntry->sock = pEntry1->sock) < 0)
               syslog(LOG_ERR, "Assuming Error for port %d, device: %s",
                               port, pEntry->devc);
         }
      }
   }
   if (defered != 0)
   {
      syslog(LOG_INFO, "Some connections defered for 5 seconds!");
      alarm(5);
   }
   else
      alarm(0);

   syslog(LOG_INFO, "Ready to Accept Connections");
   if (dbg) syslog(LOG_DEBUG, "openSockets():Exit");
}

/*
** void openConnection(int sock)
**
** This function scans through the config file for the first available
** device for the socket specified by sock that is not busy and spawns
** a connection for it. If All the devices for the socket are busy, the
** connection is closed!
*/
void openConnection(int sock)
{
   portConf *pEntry, *pEntry1;
   int newSock = -1;
   struct sockaddr_in sa;
#ifdef SCO
   int x;
#else
   size_t x;
#endif

   if (dbg) syslog(LOG_DEBUG, "openConnection():Enter");
   /*
   ** Need to accept the connection here so we have a copy
   ** of the addr info from the bind.
   */
   x = sizeof(sa);
   if ((newSock = accept(sock, (struct sockaddr *)&sa, &x)) < 0)
   {
      syslog(LOG_ERR, "Error Accepting connection on port %d",
                      pEntry->addr.sin_port);
      return;
   }
   if (dbg) syslog(LOG_DEBUG, "openConnection():Connection accepted, newSock = %d",
                              newSock);
   for (pEntry1 = NULL;
        (pEntry = findSock(pEntry1, sock)) != NULL;
        pEntry1 = pEntry)
   {
      if (pEntry->enabled > 0 && !lockDevc(pEntry))
      {
         /*
         ** If the entry is enabled and the device not busy, 
         ** spawn the connection!
         */
         if (Verbose)
            syslog(LOG_INFO, "Connection opened on port %d for device %s",
                              pEntry->addr.sin_port, pEntry->devc);
         if (dbg) syslog(LOG_DEBUG, "openConnection():Spawning child!");
         memcpy(&pEntry->addr, &sa, sizeof(pEntry->addr));
         spawnConnection(newSock, pEntry);
         break;
      }
   }
   /*
   ** If all the devices for the port are busy, close the connection!
   */
   if (pEntry == NULL)
   {
      if (dbg) syslog(LOG_DEBUG, "openConnection():No Port available, closing new connection!");
      close(newSock);
      if (Verbose)
         syslog(LOG_INFO, "Closing connection: All devices busy!");
   }
   if (dbg) syslog(LOG_DEBUG, "openConnection():Exit");
}

/*
** socketSelect()
**
** This function waits for client connections on the ports
** enabled in pPortList. If a connection is made, openConnection
** is called to accept the connection and start up a child to handle
** it if a non-busy port for it is found.
*/
void socketSelect()
{
   int maxFD = 0, sock;
   portConf *pEntry;
   fd_set fds;

   if (dbg) syslog(LOG_DEBUG, "socketSlect():Enter");
   for (Restart = 0; Restart == 0;)
   {
      /*
      ** Start by building the list of sockets to wait on
      */
      if (dbg) syslog(LOG_DEBUG, "socketSlect():building select parms!");
      FD_ZERO(&fds);
      if (ControlSock >= 0) 
      {
         syslog(LOG_DEBUG, "socketSlect():Adding control port!");
         FD_SET(ControlSock, &fds);
      }
      for (pEntry = pPortList; pEntry != NULL; pEntry = pEntry->pNext)
      {
         if (pEntry->enabled && pEntry->sock >= 0)
         {
            if (dbg) syslog(LOG_DEBUG, "socketSlect():Enabling select of socket (fd) %d",
                                        pEntry->sock);
            FD_SET(pEntry->sock, &fds);
            maxFD = (maxFD <= pEntry->sock) ? pEntry->sock + 1 : maxFD;
         }
      }

      if (dbg) syslog(LOG_DEBUG, "socketSlect():Entering Select loop");
      for (;;)
      {
         struct timeval tv;
         fd_set rfds;
         
         rfds = fds;
         if (Restart)
            break;

         tv.tv_sec = 60;
         tv.tv_usec = 0;
         if (dbg) syslog(LOG_DEBUG, "socketSlect():Calling select");
         if (select(maxFD, &rfds, NULL, NULL, &tv) >= 0)
         {
            chekDevc();
            if (dbg) syslog(LOG_DEBUG, "socketSlect():Select returned!");
            if (ControlSock >= 0 &&
                FD_ISSET(ControlSock, &fds) &&
                FD_ISSET(ControlSock, &rfds))
            {
               int x;
               syslog(LOG_DEBUG, "socketSlect():Have Control Opened!");
               if ((x = acceptControl()) >= 0)
               {
                  syslog(LOG_DEBUG, "socketSlect():Socket accepted, port %d!", x);
                  FD_SET(x, &fds);
                  maxFD = (maxFD <= x) ? x + 1 : maxFD;
                  syslog(LOG_DEBUG, "socketSlect():ControlDataSock = %d!",
                         ControlDataSock);
                  syslog(LOG_DEBUG, "socketSlect():maxFD = %d!", maxFD);
               }
            }
            for (sock = 0; sock < maxFD; sock++)
            {
               if (sock != ControlSock &&
                   FD_ISSET(sock, &fds) && 
                   FD_ISSET(sock, &rfds))
               {
                  int x;

                  if ((x = readControl(sock)) == 0)
                  {
                     if (dbg) syslog(LOG_DEBUG, "socketSlect():Opening connection!");
                     openConnection(sock);
                  }
                  else if (x < 0)
                  {
                     if (dbg) syslog(LOG_DEBUG, "socketSlect():Control port closed, sock %d",
                                                sock);
                     close(sock);
                     FD_CLR(sock, &fds);
                  }
               }
            }
         }
      }
   }
}
