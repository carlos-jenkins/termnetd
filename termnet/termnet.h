/***************************************************************************
**    termnet.h
**    termnet header file
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
#include <stdio.h>
#include "tnl.h"

extern char sBaudRate[256];
extern char sPortDevice[256];
extern char sPortSettings[256];
extern char sDevice[256];

extern int debug, verbose;
extern FILE *tnlin, *tnlout, *userOut, *userIn;

extern int contFlag, clFlg, needResponse, gotResponse, optCnt, optOffered;
extern int echoFlg, localEcho, remoteEcho;
extern int initialPass, execMode, autoMode;
extern int noHostname;
extern int execComplete;
extern int emulate_parity;

extern struct termios RunTermios, OrgTermios;

#define ESCAPE_CHAR (']' - 0x40)
#define BREAK_CHAR ('B' - 0x40)

int inputNet(char *buf, int len);
void getInput(char *buf, int len);
void inputBuffer(char *pBuf);
int inputTerminal(char *pBuf, int len);
char *nextItem(FILE *fp, char *buff);
void doExit(int code);

