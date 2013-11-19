/***************************************************************************
**    SocketIO.c
**    Socket Connection routines
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
#ifdef AIX
#include <sys/stream.h>
#endif
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <netdb.h>
#include <time.h>
#include <signal.h>
#include "tnl.h"

#ifndef INADDR_NONE
#define INADDR_NONE 0xffffffff
#endif

extern int errno;

#if !defined(FreeBSD) && !defined(LINUX)
extern char *sys_errlist[];
#endif

int tn_errno = 0;

#if !defined(AIX)
#if !defined(FreeBSD) && !defined(SunOS)
u_short htons();
#endif
u_short inet_addr();
#endif

static void timerExpired(int sig)
{
   sig = 0;
   tn_errno = TNL_TIMEOUT_ERR;
}

int connectSocket(char *pHost, char *pService, char *pProtocol,
                  int timeout_val)
{
   struct hostent *phe;
   struct servent *pse;
   struct protoent *ppe;
   struct sockaddr_in sin;
   struct itimerval  waitTime;
   int n, s, type;

   tn_errno = 0;
   bzero((char *)&sin, sizeof(sin));
   sin.sin_family = AF_INET;

   signal(SIGALRM, (void (*)(int))timerExpired);
   waitTime.it_interval.tv_sec = waitTime.it_interval.tv_usec = 0;
   waitTime.it_value.tv_sec = timeout_val;
   waitTime.it_value.tv_usec = 0;
   setitimer(ITIMER_REAL, &waitTime, NULL);

   if ((pse = getservbyname(pService, pProtocol)) != NULL)
      sin.sin_port = pse->s_port;
   else if ((sin.sin_port = htons((u_short)atoi(pService))) == 0)
   {
      tn_errno = TNL_NOSERVENTRY_ERR;
      return(-1);
   }

   if ((phe = gethostbyname(pHost)) != NULL)
      bcopy(phe->h_addr, (char *)&sin.sin_addr, phe->h_length);
   else if ((sin.sin_addr.s_addr = inet_addr(pHost)) == INADDR_NONE)
   {
      tn_errno = TNL_NOHOSTENTRY_ERR;
      return(-1);
   }

   if ((ppe = getprotobyname(pProtocol)) == 0)
   {
      tn_errno = TNL_INVPROTOCOL_ERR;
      return(-1);
   }

   if (strcmp(pProtocol, "udp") == 0)
      type = SOCK_DGRAM;
   else
      type = SOCK_STREAM;

   if ((s = socket(PF_INET, type, ppe->p_proto)) < 0)
   {
      tn_errno = TNL_SOCKET_ERR;
      return(-1);
   }

   if (connect(s, (struct sockaddr *)&sin, sizeof(sin)) < 0)
   {
      if (tn_errno == 0)
         tn_errno = TNL_NOCONNECT_ERR;
      return(-1);
   }

   waitTime.it_interval.tv_sec = waitTime.it_interval.tv_usec = 0;
   waitTime.it_value.tv_sec = waitTime.it_value.tv_usec = 0;
   setitimer(ITIMER_REAL, &waitTime, NULL);
   signal(SIGALRM, SIG_DFL);

   if ((n = fcntl(s, F_GETFL, 0)) < 0)
      return(-6);
   return(s);
}
