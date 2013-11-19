/***************************************************************************
**    trnVars.c
**    Termnet Global Variable Allocations
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
#include <sys/ioctl.h>
#include <fcntl.h>
#include <stdio.h>
#include <termios.h>
#include "termnet.h"

char sBaudRate[256] = "";
char sPortDevice[256] = "";
char sPortSettings[256] = "";
char sDevice[256] = "";

int execComplete = 0;
int debug, verbose;
FILE *tnlin = NULL, *tnlout = NULL, *userIn, *userOut;

int contFlag = 0, clFlg = 0, needResponse = 0, gotResponse = 0;
int echoFlg = 0, localEcho = 0, remoteEcho = 0;
int optCnt = 0, optOffered = 0;
int initialPass = 1, execMode = 0, autoMode = 0;
int noHostname = 0;

struct termios RunTermios, OrgTermios;

