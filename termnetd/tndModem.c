/***************************************************************************
**    tndModem.c
**    Modem Device Funtions
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
#include <stdlib.h>
#include <unistd.h>
#include <syslog.h>
#include <signal.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <termios.h>
#include "termnetd.h"

static int dbg = 0;

#if defined(SCO) || defined(AIX)
#define CRTSCTS 0
#endif

void buildDeviceSettingStr(int mfd, portConf *pDevice)
{
   struct termios tios;
   speed_t mspeed;
   char buf[256];
   int x;

   strcpy(buf, "");
   tcgetattr(mfd, &tios);
   switch (tios.c_cflag & CSIZE)
   {
      case CS8:
         strcat(buf, "8");
         break;
      case CS7:
         strcat(buf, "7");
         break;
      case CS6:
         strcat(buf, "6");
         break;
      case CS5:
         strcat(buf, "5");
         break;
      default:
         strcat(buf, "8");
         break;
   }
   switch (tios.c_cflag & (PARENB | PARODD))
   {
      case PARENB | PARODD:
         strcat(buf, "O");
         break;
      case PARENB:
         strcat(buf, "E");
         break;
      default:
         strcat(buf, "N");
   }

#if !defined(SCO) && !defined(AIX) && !defined(OSF)
   if ((x = tios.c_cflag) & CRTSCTS)
      strcat(buf, "C1");
   else
      strcat(buf, "C0");
#else
      strcat(buf, "C0");
#endif
   if (tios.c_iflag & IXON)
      strcat(buf, "S1");
   else
      strcat(buf, "S0");

   setPortStr(pDevice, buf);

   mspeed = cfgetospeed(&tios);
   switch (mspeed)
   {
      case B300:
         strcpy(buf, "300");
         break;

      case B600:
         strcpy(buf, "600");
         break;

      case B1200:
         strcpy(buf, "1200");
         break;

      case B2400:
         strcpy(buf, "2400");
         break;

      case B4800:
         strcpy(buf, "4800");
         break;

      case B9600:
         strcpy(buf, "9600");
         break;

      case B19200:
         strcpy(buf, "19200");
         break;

      case B38400:
         strcpy(buf, "38400");
         break;

#if !defined(SCO) && !defined(AIX) && !defined(OSF)
      case B57600:
         strcpy(buf, "57600");
         break;

      case B115200:
         strcpy(buf, "115200");
         break;

      case B230400:
         strcpy(buf, "230400");
         break;
#endif

      default:
         strcpy(buf, "38400");
         break;
   }
   setBaudStr(pDevice, buf);
}

/*
** This function parses the strings for both the port settings and
** baudrate and sets the port as specified.
**
** They are parsed and set individualy so any errors in the one
** will not keep the other from being set.
**
** After the ports are set, new current configuration strings are
** built reflecting the current settings
*/
void setModem(int mfd, portConf *pDevice)
{
   int x, err;
   char *cp;
   speed_t mspeed;
   struct termios tios;

   if (dbg) syslog(LOG_DEBUG, "setModem():Enter");
   tcgetattr(mfd, &tios);
   if (dbg) syslog(LOG_DEBUG, "setModem():Setting Port parms to |%s|", pDevice->port);
   for (err = 0, cp = pDevice->port; *cp; cp++)
   {
      *cp = toupper(*cp);
      switch(*cp)
      {
         case '8':
            if (dbg) syslog(LOG_DEBUG, "setModem():Setting Port 8 bits");
            tios.c_cflag = (tios.c_cflag & ~CSIZE) | CS8;
            break; 
         case '7':
            if (dbg) syslog(LOG_DEBUG, "setModem():Setting Port 7 bits");
            tios.c_cflag = (tios.c_cflag & ~CSIZE) | CS7;
            break; 
         case '6':
            if (dbg) syslog(LOG_DEBUG, "setModem():Setting Port 6 bits");
            tios.c_cflag = (tios.c_cflag & ~CSIZE) | CS6;
            break; 
         case '5':
            if (dbg) syslog(LOG_DEBUG, "setModem():Setting Port 5 bits");
            tios.c_cflag = (tios.c_cflag & ~CSIZE) | CS5;
            break; 

         case 'N':
            if (dbg) syslog(LOG_DEBUG, "setModem():Setting Port N Parity");
            tios.c_cflag &= ~PARENB;
            break; 
         case 'E':
            if (dbg) syslog(LOG_DEBUG, "setModem():Setting Port E Parity");
            tios.c_cflag = (tios.c_cflag & ~PARODD) | PARENB;
            break; 
         case 'O':
            if (dbg) syslog(LOG_DEBUG, "setModem():Setting Port O Parity");
            tios.c_cflag |= PARODD | PARENB;
            break;

         case 'S':
            if (*(++cp) == '0')
               tios.c_iflag &= ~IXON;
            else
               tios.c_iflag |= IXON;
            break;

         case 'C':
#if !defined(SCO) && !defined(AIX) && !defined(OSF)
            x = CRTSCTS;
#else
            x = 0;
#endif
            if (*(++cp) == '0')
               tios.c_cflag &= ~x;
            else
               tios.c_cflag |= x;
            break;

         default:
            syslog(LOG_ERR, "Unrecognized port setting: '%c', Settings not changed!",
                            *cp);
            err = 1;
            break;
      }
   }
   if (!err)
   {
      tios.c_lflag = 0;
      tios.c_oflag = 0;
      tios.c_iflag |= IGNBRK;
      tios.c_cc[VMIN] = 0;
      tios.c_cc[VTIME] = 0;
      if (dbg)
      {
         syslog(LOG_DEBUG, "setModem():Setting Devc.c_oflag = 0x%x", (unsigned int)tios.c_oflag);
         syslog(LOG_DEBUG, "setModem():Setting Devc.c_iflag = 0x%x", (unsigned int)tios.c_iflag);
         syslog(LOG_DEBUG, "setModem():Setting Devc.c_lflag = 0x%x", (unsigned int)tios.c_lflag);
         syslog(LOG_DEBUG, "setModem():Setting Devc.c_cflag = 0x%x", (unsigned int)tios.c_cflag);
      }
      if (tcsetattr(mfd, TCSANOW, &tios) < 0)
         syslog(LOG_ERR, "Error Setting Port Parameters:%m");
   }

   err = 0;
   tcgetattr(mfd, &tios);
   if (dbg) syslog(LOG_DEBUG, "setModem():Setting Baudrate to |%s|", pDevice->baud);
   x = atoi(pDevice->baud);
   switch (x)
   {
      case 300:
         if (dbg) syslog(LOG_DEBUG, "setModem():Setting Baud to 300");
         mspeed = B300;
         break;

      case 600:
         if (dbg) syslog(LOG_DEBUG, "setModem():Setting Baud to 600");
         mspeed = B600;
         break;

      case 1200:
         if (dbg) syslog(LOG_DEBUG, "setModem():Setting Baud to 1200");
         mspeed = B1200;
         break;

      case 2400:
         if (dbg) syslog(LOG_DEBUG, "setModem():Setting Baud to 2400");
         mspeed = B2400;
         break;

      case 4800:
         if (dbg) syslog(LOG_DEBUG, "setModem():Setting Baud to 4800");
         mspeed = B4800;
         break;

      case 9600:
         if (dbg) syslog(LOG_DEBUG, "setModem():Setting Baud to 9600");
         mspeed = B9600;
         break;

      case 19200:
         if (dbg) syslog(LOG_DEBUG, "setModem():Setting Baud to 19200");
         mspeed = B19200;
         break;

      case 38400:
#if defined(SCO) || defined(AIX) || defined(OSF)
      case 57600:
      case 115200:
      case 230400:
#endif
         if (dbg) syslog(LOG_DEBUG, "setModem():Setting Baud to 38400");
         mspeed = B38400;
         break;

#if !defined(SCO) && !defined(AIX) && !defined(OSF)
      case 57600:
         if (dbg) syslog(LOG_DEBUG, "setModem():Setting Baud to 38400");
         mspeed = B57600;
         break;

      case 115200:
         if (dbg) syslog(LOG_DEBUG, "setModem():Setting Baud to 38400");
         mspeed = B115200;
         break;

      case 230400:
         if (dbg) syslog(LOG_DEBUG, "setModem():Setting Baud to 38400");
         mspeed = B230400;
         break;
#endif

      default:
         syslog(LOG_ERR, "Unrecognized Baud Rate: '%s', Baud Rate not changed!",
                          pDevice->baud);
         err = 1;
   }
   if (!err)
   {
      cfsetospeed(&tios, mspeed);
      cfsetispeed(&tios, mspeed);
      if (tcsetattr(mfd, TCSANOW, &tios) < 0)
         syslog(LOG_ERR, "Error Setting Baud Rate:%m");
   }
   if (dbg)
   {
      tcgetattr(mfd, &tios);
      syslog(LOG_DEBUG, "setModem():Devc.c_oflag = 0x%x", (unsigned int)tios.c_oflag);
      syslog(LOG_DEBUG, "setModem():Devc.c_iflag = 0x%x", (unsigned int)tios.c_iflag);
      syslog(LOG_DEBUG, "setModem():Devc.c_lflag = 0x%x", (unsigned int)tios.c_lflag);
      syslog(LOG_DEBUG, "setModem():Devc.c_cflag = 0x%x", (unsigned int)tios.c_cflag);
   }
   buildDeviceSettingStr(mfd, pDevice);
   if (dbg) syslog(LOG_DEBUG, "setModem():Exit");
}

int openModem(portConf *pDevice)
{
   int mflgs, x, rv = -1;

   if (dbg) syslog(LOG_DEBUG, "openModem():Opening device |%s|", pDevice->devc);
   if ((rv = open(pDevice->devc, O_RDWR | O_NDELAY)) < 0)
   {
      syslog(LOG_ERR, "Error opening Device %s:%m", pDevice->devc);
      rv = -1;
   }
   else
   {
      if (((mflgs = fcntl(rv, F_GETFL, 0)) == -1) ||
         (fcntl(rv, F_SETFL, mflgs & ~O_NDELAY) == -1))
      {
         syslog(LOG_ERR, "Error Resetting O_NODELAY on device %s:%m",
                         pDevice->devc);
         close(rv);
         rv = -1;
      }
      else
      {
         /*
         ** Get the current termios structure for the device
         ** and set up the initial defaults. Then configure
         ** it as was specified in the configuration file.
         */
         struct termios tios;
         x = CREAD | CLOCAL | CS8 | B38400;
#if !defined(SCO) && !defined(AIX) && !defined(OSF)
         x |= CRTSCTS;
#endif
         tcgetattr(rv, &tios);
         tios.c_cflag = x;
         tios.c_lflag = 0;
         tios.c_oflag = 0;
         tios.c_iflag |= IGNBRK;
         tios.c_cc[VMIN] = 0;
         tios.c_cc[VTIME] = 0;
         tcsetattr(rv, TCSANOW, &tios);
         cfsetispeed(&pDevice->tios, pDevice->speed);
         cfsetospeed(&pDevice->tios, pDevice->speed);
         tcsetattr(rv, TCSANOW, &pDevice->tios);
         buildDeviceSettingStr(rv, pDevice);
      }
   }
   return(rv);
}

void closeModem(portConf *pDevice)
{
   syslog(LOG_DEBUG, "closeModem():Closing device |%s|", pDevice->devc);
   close (pDevice->mfd);
}

int SendBreak(int mfd)
{
   return(tcsendbreak(mfd, 0));
}
