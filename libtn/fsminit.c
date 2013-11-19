/***************************************************************************
**    fsminit.c
**    Finite State Machine initialization
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
#include <stdio.h>
#include "fsm.h"

#define TINVALID 0xff

static int dbg = 0;

void fsmInit(unsigned char fsm[][NCHRS], trans_table ttab[], int nstates)
{
   trans_table *pTbl;
   int ch, state, index;

   if (dbg) fprintf(stderr, "Filling table w/ TINVALID\r\n");   
   for (ch = 0; ch < NCHRS; ++ch)
      for (index = 0; index < nstates; ++index)
         fsm[index][ch] = TINVALID;
   
   if (dbg) fprintf(stderr, "Scanning Table...\r\n");   
   for (index = 0; ttab[index].state != FSINVALID; ++index)
   {
      pTbl = &ttab[index];
      state = pTbl->state;
      if (dbg) fprintf(stderr, "index = %d, state = %d, chr = 0x%x\r\n", index, state, pTbl->chr);
      if (pTbl->chr == TCANY)
      {
         for (ch = 0; ch < NCHRS; ++ch)
         {
            if (fsm[state][ch] == TINVALID)
               fsm[state][ch] = index;
         }
      }
      else
         fsm[state][pTbl->chr] = index;
   }
   
   if (dbg) fprintf(stderr, "Scanning Array...\r\n");   
   for (ch = 0; ch < NCHRS; ++ch)
      for (index = 0; index < nstates; ++index)
         if (fsm[index][ch] == TINVALID)
            fsm[index][ch] = index;
}
