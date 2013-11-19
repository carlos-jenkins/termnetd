/***************************************************************************
**    tnlP.h
**    Telnet library private header file
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
#ifndef _TNLP_H
#define _TNLP_H

#include <termios.h>
#include <fcntl.h>
#include "fsm.h"
#include "tnl.h"

struct cb_info
{
   int   event;
   void  (*callback)(int iVal, void *pArg1, void *pArg2);
};

typedef struct cb_info cb_entry;

extern trans_table SubOptionTrans[];
extern unsigned char SubOptionFsm[][NCHRS];
extern int suboptionState;

extern trans_table SocketTrans[];
extern unsigned char SocketFsm[][NCHRS];
extern int socketState;

extern int nfd;
extern FILE *nfp, *mfp;
extern int nSynch, cOpt;
extern int Options[];
extern char SubOptions[256][128];
extern char *pSubOptions[];
extern struct termios RunTermios, OrgTermios;
extern FILE *aplin, *aplout;
extern int AplPipe[];
extern cb_entry CBList[];

void abortFsm(int ch);
void inputNet(unsigned char *buf, int len);
void outputNet(unsigned char *pBuf, int len);
void no_op(int ch);
void inputSubOpt(int ch);
void endSubOpt(int ch);
void doOption(int opt);
void willOption(int opt);
int doCallBack(int event, int val, char *pArg1, char *pArg2);

#define APLIN     0              /* Offsets into AplPipe             */
#define APLOUT    1

#define CB_MAX    64             /*max number of call backs permited */

#endif /* _TNLP_H */

