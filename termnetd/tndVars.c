/***************************************************************************
**    tndVars.h
**    Termnetd Global Variables
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
#include <unistd.h>
#include <sys/socket.h>
#include "termnetd.h"

portConf *pPortList = NULL;
devcLock *pLockList = NULL;
int Permiscuous = 0;
int NoDetach = 0;
int Restart = 0;
int Verbose = 0;
int Debug = 0;
char PrivilegedUser[1024] = "root";
char ControlPort[1024] = "";
int ControlSock = -1;
int ControlDataSock = -1;
struct sockaddr_in ControlAddr;
int ControlStatus = 0;
int PortEnabled = 1;

