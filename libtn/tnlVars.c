/***************************************************************************
**    tnlVars.c
**    Telnet library Global Variable allocations
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
#include "tnlP.h"

/* 
** State table structures for Sub Option negotiations
*/
extern trans_table SubOptionTrans[];
unsigned char SubOptionFsm[SS_NSTATES][NCHRS];
int suboptionState;

/*
** State table for Socket negotiation
*/
extern trans_table SocketTrans[];
unsigned char SocketFsm[TS_NSTATES][NCHRS];
int socketState;

int nfd = 0;                     /* Socket file handle               */
FILE *nfp = NULL, *mfp = NULL;   /* Network/Device file handles      */
FILE *sfp = NULL;                /* Inter communication file handle  */
int nSynch = 0, cOpt;            /* Misc flags                       */

int emulate_parity = 0;          /* Emulate 7 bit even parity data   */

int Options[256];                /* Option table                     */
char SubOptions[256][128];      /* Sub Option storage               */
char *pSubOptions[256];          /* Sub Option Pointers              */

FILE *aplin, *aplout;            /* File pointers to application in/out */
int AplPipe[2];                  /* Storage for appl in/out file hanles */

cb_entry CBList[CB_MAX];         /* Call Back list                   */

/*
** Termios structures used for setting up the properties of the
** appication's data pipe
*/
struct termios RunTermios, OrgTermios;
