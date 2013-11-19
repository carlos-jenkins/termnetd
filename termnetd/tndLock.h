/***************************************************************************
**    tndLock.h
**    Termnetd Device Locking Functions
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
#ifndef _TNDLOCK_H
#define _TNDLOCK_H

typedef struct devcLock
{
   portConf          *pDevc;
   struct devcLock   *pNext;
   struct devcLock   *pPrev;
   int               delete;
   char              lock[1024];
} devcLock;

int   lockDevc(portConf *pEntry);
void  chekDevc();
int   isLocked(const char *pDevc);

#endif
