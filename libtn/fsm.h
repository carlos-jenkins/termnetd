/***************************************************************************
**    fsm.h
**    Finite State Machine Header
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

#ifndef _TSDFSM_H
#define _TSDFSM_H

#include <stdio.h>

/*
** Termsrvd Socket Input FSM States
*/
#define TS_DATA      0
#define TS_IAC       1
#define TS_WOPT      2
#define TS_DOPT      3
#define TS_SUBNEG    4
#define TS_SUBIAC    5
#define TS_NSTATES   6

/*
** Termsrvd Option Subnegotiation FSM States
*/
#define SS_START     0
#define SS_OPT       1
#define SS_GET       2
#define SS_END       3

#define SS_NSTATES   4

#define FSINVALID    255

#define NCHRS        256
#define TCANY        (NCHRS + 1)

typedef struct
{
   unsigned char  state;
   short          chr;
   unsigned char  next;
   void           (*action)(int ch);
} trans_table;

void fsmInit(unsigned char fsm[][NCHRS], trans_table ttab[], int nstates);

#endif

