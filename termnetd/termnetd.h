/***************************************************************************
**    telnetd.h
**    telnetd common definitions
**
** Copyright (C) 1996      Joseph Croft <joe@croftj.net>
**
** termnetd general definitions
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
#ifndef _TELNETD_H
#define _TELNETD_H

#include <sys/socket.h>
#include "tndConfig.h"
#include "tndLock.h"

extern portConf *pPortList;
extern devcLock *pLockList;

extern char PrivilegedUser[];
extern char ControlPort[];
extern int ControlSock;
extern int ControlDataSock;
extern struct sockaddr_in ControlAddr;
extern int ControlStatus;
extern int PortEnabled;

extern int Permiscuous;
extern int NoDetach;
extern int Verbose;
extern int Restart;
extern int Debug;

void spawnConnection(int sock, portConf *pDevice);
int  openModem(portConf *pDevice);
void closeModem(portConf *pDevice);
void setModem(int mfd, portConf *pDevice);
void buildDeviceSettingStr(int mfd, portConf *pDevice);
int  SendBreak(int mfd);
void termnetd(portConf *pDevice);
void closeSockets();
void openSockets(int retry);
void socketSelect();
int acceptControl();
int readControl(int sock);

#endif
