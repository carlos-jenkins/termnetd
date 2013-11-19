/***************************************************************************
**    tnlinit.c
**    Telnet library Initialization functions
**
** Copyright (C) 1995, 1996  Joseph Croft <joe@croftj.net>  
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
#include <unistd.h>
#include <termios.h>
#include <arpa/telnet.h>
#include <sys/socket.h>
#include <signal.h>
#include <stdlib.h>
#include "SocketIO.h"
#include "tnlP.h"

static char serviceName[] = "telnet";

static void buildStateTables()
{
   fsmInit(SocketFsm, SocketTrans, TS_NSTATES);
   socketState = TS_DATA;
   fsmInit(SubOptionFsm, SubOptionTrans, SS_NSTATES);
   suboptionState = SS_START;
}

static void iotHandler(int sig)
{
   signal(SIGIOT, iotHandler);
}

/*
** int tnlInit(char *pHost, char *pService, **pInFile, FILE **pOutFile)
** 
** This function does the required initialization and setup required by the
** telnet library.
**
** If pHost is NULL, it will be assumed that the application is a server
** and that the connection to the socket has already be established and that
** the the file handle for the socket is 0.
**
** If pHost is not NULL, the application will be assumed to be a client and
** attempt will be made to establish a connection to the server specified by
** pHost and the optional pService arguments. If NULL is passed for pService,
** the name "telnet" will be used.
**
** This function will also open up to Pipes to be used for passing data from
** the application to the socket, and and also passing data from the socket
** to the application. The pointers to the FILE structures for these files
** will be stored at the locations specified by pInFile and pOutFile 
** respectivly.
*/
int tnlInit(char *pHost, char *pService,
            FILE **pInFile, FILE **pOutFile, int timeout_val)
{
   cb_entry *pEntry;
   int fd1[2], fd2[2], rv, x, on, n;

   rv = 0;
   for (x = 0; x < 256; x++)
   {
      pSubOptions[x] = SubOptions[x];
      *pSubOptions[x] = '\0';
   }

   buildStateTables();

   /*
   ** We have no callbacks yet
   */
   for (x = 0, pEntry = CBList; x < CB_MAX; x++, pEntry++)
   {
      pEntry->event = 0;
      pEntry->callback = NULL;
   }

   /*
   ** Disable all options until they are enabled by the application
   */
   for (x = 0; x < 256; x++)
      Options[x] = -1;

   /*
   ** Create the pipes used to communicate w/ the application
   */
   if (!pipe(fd1) && !pipe(fd2))
   {
      /*
      ** Make it easier for yourself to reference the pipes
      */
      AplPipe[APLIN] = fd1[0];
      AplPipe[APLOUT] = fd2[1];

      if ((n = fcntl(fd2[0], F_GETFL, 0)) < 0)
         return(-1);
      n |= FNDELAY;
      fcntl(fd2[0], F_SETFL, n);

      /*
      ** Create buffered file I/O for the application to use
      */
      if ((aplin     = fdopen(fd1[0], "r")) != NULL &&
          (*pOutFile = fdopen(fd1[1], "w")) != NULL &&
          (*pInFile  = fdopen(fd2[0], "r")) != NULL &&
          (aplout    = fdopen(fd2[1], "w")) != NULL)
      {

         /*
         ** force the input pipe from the application to no Block
         ** Save its original state just incase its needed
         */
         RunTermios = OrgTermios;                  /* copy (structure) to RunTermios  */

         /*
         ** If a host was given, connect to it
         */
         if (pHost != NULL)
         {
            if (pService == NULL)
               pService = serviceName;
            if ((nfd = connectSocket(pHost, pService, "tcp", timeout_val)) > 0)
               setsockopt(nfd, SOL_SOCKET, SO_OOBINLINE, (char *)&on, sizeof(on));
            else
               rv = -1;
         }

         /*
         ** Otherwise assume we have the connection already established
         ** on file handle 0
         */
         else
            nfd = 0;

         /*
         ** Open up a buffer I/O file for the socket
         */
         if (rv == -1 || (nfp = fdopen(nfd, "w")) == NULL)
            rv = -1;
      }
      else
         rv = -1;
   }
   else
      rv = -1;

   /*
   ** Return to the caller w/ and error conditions found
   */
   return(rv);
}

