/***************************************************************************
**    tnl.h
**    telnet library public header file
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
#ifndef _TNL_H
#define _TNL_H

#ifdef AIX
#include <sys/select.h>
#endif
#include <sys/time.h>
#include <unistd.h>
#include <stdio.h>

/*
** Error codes
*/

/*
** Callback types
*/
#define TNL_DOOPTION_CB       1
#define TNL_WILLOPTION_CB     2
#define TNL_SENDSUBOPTDATA_CB 3
#define TNL_ISSUBOPTDATA_CB   4
#define TNL_SPECIALCHAR_CB    5
#define TNL_ERROR_CB          255

/*
** Error definitions
*/
#define TNL_SOCKETREAD_ERR    1
#define TNL_PIPEREAD_ERR      2
#define TNL_INVLDSTATE_ERR    3
#define TNL_OUTOFMEM_ERR      4
#define TNL_NOSERVENTRY_ERR   5
#define TNL_NOHOSTENTRY_ERR   6
#define TNL_INVPROTOCOL_ERR   7
#define TNL_SOCKET_ERR        8
#define TNL_NOCONNECT_ERR     9
#define TNL_TIMEOUT_ERR       10

extern int tn_errno;

int tnlSelect(fd_set *pReadFds, fd_set *pWriteFds, 
               fd_set *pExcpFds, struct timeval *pTimer);
int tnlInit(char *pHost, char *pService,
            FILE **pInFile, FILE **pOutFile, int timeout_val);
void tnlShutdown(int how);
void tnlSendSpecial(unsigned char ch);
void tnlOfferOption(int opt, int will);
void tnlRequestOption(int opt, int _do);
void tnlDisableOption(int opt);
void tnlEnableOption(int opt);
int tnlIsOption(int opt);
int tnlSetSubOption(int opt, char *pStr);
int tnlSendSubOption(int opt, char *pSubOption);
void tnlRequestSubOption(int opt);
int tnlSetCallBack(int event, void  (*pF)(int iVal, void *pArg1, void *pArg2));
int openPtyDevice(char *pName);

#endif /* _TNL_H */

