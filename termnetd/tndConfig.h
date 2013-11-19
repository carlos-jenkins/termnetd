/***************************************************************************
**    tndConfig.h
**    Termnetd Configuration File and Structure Functions
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
#ifndef _TNDCONF_H
#define _TNDCONF_H

#include <netinet/in.h>
#include <termios.h>
#include <stdio.h>

#define PC_MAXFIELD  16
#define PC_MAXDEVC   80

struct _portConf
{
   int                  sock;
   int                  mfd;
   pid_t                pid;
   int                  enabled;
   int                  enb_save;
   struct sockaddr_in   addr;
   struct termios       tios;
   speed_t              speed;
   char                 devc[PC_MAXDEVC];
   char                 dnam[PC_MAXFIELD];
   char                 baud[PC_MAXFIELD];
   char                 port[PC_MAXFIELD];
   char                 srvc[PC_MAXFIELD];
   struct _portConf     *pNext;
};

typedef struct _portConf portConf;

int loadConfig(const char *pFile);
void setPortStr(portConf *pDevice, const char *pPortStr);
void setBaudStr(portConf *pDevice, const char *pBaudStr);
portConf *findDevc(const portConf *pPrev, const char *pDevc);
portConf *findSrvc(const portConf *pPrev, const char *pSrvc);
portConf *findSock(const portConf *pPrev, const int sock);
portConf *findPortByName(const portConf *pPrev, const char *pSrvc);

portConf *parseCfgEntry(FILE *fp, int *err);


#endif
