/***************************************************************************
**    tnlMisc.c
**    Misc. Support functions
**
** Copyright (C) 1998  Joseph Croft <joe@croftj.net>  
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
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <syslog.h>

static int dbg = 0;

#ifdef AIX
static char ttydev_x[] = "EDCBAzyxwvutsrqp";
#else
static char ttydev_x[] = "edcbazyxwvutsrqp";
#endif

#ifdef LINEAR_PTY
#define MAX_PTY 256
static int ttydev_y;
static char ttydev_mask[] = "/dev/ptyp";
#else
static char ttydev_y[] = "fedcba9876543210";
static char ttydev_mask[] = "/dev/pty$$";
#endif

static char ttydev_z[256];

int openPtyDevice(char *pName)
{
   int x, fd = -1;
   struct stat statbuf;
#ifdef LINEAR_PTY
   int  px;
#else
   char *px, *py;
#endif

   if (pName == NULL || strlen(pName) == 0) 
   {
#ifdef LINEAR_PTY
     for (px = 0; px < MAX_PTY; px++) 
     {
        sprintf(ttydev_z, "%s$d", ttydev_mask, px);
#else
      for (px = ttydev_x; *px != '\0'; px++) 
      {
         strcpy(ttydev_z, ttydev_mask);
         for (py = ttydev_y; *py != '\0'; py++) 
         {
            ttydev_z[8] = *px;
            ttydev_z[9] = *py;
#endif
            if (dbg) syslog(LOG_DEBUG, "openPtyDevice():Attempting |%s|", ttydev_z);
            if ((x = stat(ttydev_z, &statbuf)) == 0 && S_ISCHR(statbuf.st_mode))
            {
               if (dbg) syslog(LOG_DEBUG, "openPtyDevice():Device exists");
               if ((fd = open(ttydev_z, O_RDWR)) >= 0)
               {
                  strcpy(pName, ttydev_z);
                  break;
               }
               else if (dbg)
               {
                  syslog(LOG_DEBUG, "Error opening device:%m");
               }
            }
            else 
            {
               if (dbg) syslog(LOG_DEBUG, "openPtyDevice():x = %d", x);
               if (x != 0) 
               {
                  if (dbg) syslog(LOG_DEBUG, "openPtyDevice():Device does not exists");
               }
               else
                  if (dbg) syslog(LOG_DEBUG, "openPtyDevice():Device type 0x%x, is wrong",
                                          statbuf.st_mode);
            }
#ifndef LINEAR_PTY
         }
#endif
      }
   }
   else
   {
      if (dbg) syslog(LOG_DEBUG, "openPtyDevice():Attempting |%s|", pName);
      if ((x = stat(pName, &statbuf)) == 0 && S_ISCHR(statbuf.st_mode)) 
      {
         if (dbg) syslog(LOG_DEBUG, "openPtyDevice():Device exists");
         fd = open(pName, O_RDWR);
      }
      else 
      {
         if (x != 0) 
         {
            if (dbg) syslog(LOG_DEBUG, "openPtyDevice():Device does not exists");
         }
         else
            if (dbg) syslog(LOG_DEBUG, "openPtyDevice():Device type 0x%x, is wrong",
                                    statbuf.st_mode);
      }
   }
   return(fd);
}


