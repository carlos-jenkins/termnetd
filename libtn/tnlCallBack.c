/***************************************************************************
**    tnlCallBack.c
**    Telnet library Call back funtions
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
#include "tnlP.h"

int tnlSetCallBack(int event, void  (*pF)(int iVal, void *pArg1, void *pArg2))
{
   cb_entry *pEntry;
   int x, rv;
   
   for (x = 0, pEntry = CBList; x < CB_MAX; x++, pEntry++)
   {
      if (pEntry->event == 0)
      {
         pEntry->event = event;
         pEntry->callback = pF;
         break;
      }
   }
   if (x < CB_MAX)
      rv = 0;
   else
      rv = -1;
   return(rv);
}

int doCallBack(int event, int val, char *pArg1, char *pArg2)
{
   cb_entry *pEntry;
   int rv, x;
   
   for (rv = 0, x = 0, pEntry = CBList; x < CB_MAX; x++, pEntry++)
   {
      if (pEntry->event == event)
      {
         rv++;
         (pEntry->callback)(val, pArg1, pArg2);
      }
   }
   return(rv);
}
