/***************************************************************************
**    tnlSelect.c
**    Telnet library File Select funtions
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
#include <arpa/telnet.h>
#include <sys/socket.h>
#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include "tnlP.h"

void tnlShutdown(int how)
{
   shutdown(nfd, how);
}

/*
**
** void tnlSelect(fd_set *pReadFds, fd_set *pWriteFds, 
**                 fd_set *pExcpFds, sturct timeval *pTimer)
**
** This functon is for the handling of the telnet I/O used by the application
** It has the same caller parameters is the system call select. The basic
** difference is that it will add to the file sets the handle used to 
** send data to the telnet socket and also the file handle for the pipe 
** that is used to feed data to the telnet socket.
** 
** It is assumed that the caller will have already added to the file sets 
** those files it is interested in. These normaly would include the file
** handles for the Pipe used by the library to send and socket data to the 
** application and stdin.
**
** The various call back functions set up by the caller may be called BEFORE
** this function returns. For the overall performance of the application, it
** would be best if those call backs did minimal work and returned as quickly
** as possible, to be processed further after the subsequent return of this
** function.
**
** The caller MUST supply the file sets for Reads, and exceptions. The file
** set for writing the passing of the timeval structure follows the same
** rules as for the system call select.
**
** The only translations done by this function of the data to the socket
** from the application is that any IAC characters WILL be duplicated so
** they will will be passed to the other end as a character.
*/
int tnlSelect(fd_set *pReadFds, fd_set *pWriteFds, 
                 fd_set *pExcpFds, struct timeval *pTimer)
{
   int maxFd, cnt, rv;
   unsigned char commbuf[256];
   
   maxFd = getdtablesize();
   
   FD_SET(nfd, pReadFds);
   FD_SET(nfd, pExcpFds);
   FD_SET(AplPipe[APLIN], pReadFds);

   if ((rv = select(maxFd, pReadFds, pWriteFds, pExcpFds, pTimer)) >= 0)
   {
      if (FD_ISSET(nfd, pReadFds))
      {
         if ((cnt = read(nfd, commbuf, sizeof(commbuf))) > 0)
            inputNet(commbuf, cnt);
         else
            doCallBack(TNL_ERROR_CB, TNL_SOCKETREAD_ERR, (void *)errno, NULL);
      }
      
      if (FD_ISSET(AplPipe[APLIN], pReadFds))
      {
         if ((cnt = read(AplPipe[APLIN], commbuf, sizeof(commbuf))) > 0)
            outputNet(commbuf, cnt);
         else
            doCallBack(TNL_ERROR_CB, TNL_PIPEREAD_ERR, (void *)errno, NULL);
      }
         
      if (FD_ISSET(nfd, pExcpFds))
         nSynch = 1;
   }
   fflush(aplout);
   fflush(nfp);
   return(rv);
}
