/***************************************************************************
**    tndCfgParse.c
**    Termnetd Configuration File and Structure Functions
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
#include "../sysdefs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <setjmp.h>
#include <syslog.h>
#include <termios.h>
#include "tndCfgParse.h"
#include "termnetd.h"

extern FILE *yyin;
extern char *yytext;

int yylex();

static int dbg = 0;

YYSTYPE yylval;

int xlate_iflag[] =
{
   IGNBRK, BRKINT, IGNPAR, PARMRK, INPCK, ISTRIP,
   INLCR, IGNCR, ICRNL,
#if !defined(FreeBSD)
   IUCLC,
#else
   0,
#endif
   IXON, IXANY, IXOFF, IMAXBEL
};

int xlate_oflag[] =
{
   OPOST,
#if !defined(FreeBSD)
   OLCUC,
#else
   0,
#endif
   ONLCR,
#if !defined(FreeBSD)
   OCRNL, ONOCR, ONLRET, OFILL, OFDEL,
   NL0, NL1, CR0, CR1, CR2, CR3, TAB0, TAB1, TAB2, TAB3,
#else
   0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
#endif
#if !defined(FreeBSD) && !defined(AIX) && !defined(OSF)
   XTABS,
#else
   0,
#endif
#if !defined(FreeBSD)
   BS0, BS1, VT0, VT1, FF0, FF1
#else
   0, 0, 0, 0, 0, 0,
#endif
};

int xlate_cflag[] =
{
   CS5, CS6, CS7, CS8, CSTOPB, PARENB, PARODD, HUPCL, CLOCAL,
#if !defined(FreeBSD) && !defined(SCO) && !defined(AIX) && !defined(OSF)
   CIBAUD, CRTSCTS,
#else
   0, 0,
#endif
   CREAD
};

int xlate_lflag[] =
{
   ISIG, ICANON,
#if !defined(FreeBSD)
   XCASE,
#else
   0,
#endif
   ECHO, ECHOE, ECHOK, ECHONL, ECHOCTL, ECHOPRT,
   ECHOKE, FLUSHO, NOFLSH, TOSTOP, PENDIN, IEXTEN
};

int xlate_speed[] =
{
   B50, B75, B110, B134, B150, B200, B300, B600, B1200, B1800, B2400,
   B4800, B9600, B19200, B38400,
#if !defined(SCO) && !defined(AIX) && !defined(OSF)
   B57600, B115200, B230400
#else
   B38400, B38400, B38400
#endif
};

portConf *parseCfgEntry(FILE *fp, int *err)
{
   int token, state = 0;
   portConf *rv;

   if (dbg) syslog(LOG_DEBUG, "parseCfgEntry():Enter");
   yyin = fp;
   *err = 0;
   if ((rv = (portConf *)malloc(sizeof(portConf))) != NULL)
   {
      if (dbg) syslog(LOG_DEBUG, "parseCfgEntry():Starting to Parse");
      memset(rv, 0, sizeof(portConf));
      while (rv != NULL && state >= 0)
      {
         if ((token = yylex()) == ';' || token == 0)
         {
            if (dbg) syslog(LOG_DEBUG, "parseCfgEntry():Found end of Entry");
            if (state == 0)
            {
               *err = 0;
               free (rv);
               rv = NULL;
            }
            state = -1;
            continue;
         }
         else if (token == ':')
         {
            if (dbg) syslog(LOG_DEBUG, "parseCfgEntry():Found end of state %d",
                                       state);
            if (++state > 3)
            {
               *err = ECFG_SYNTAX;
               free (rv);
               rv = NULL;
            }
            continue;
         }
         if (dbg) syslog(LOG_DEBUG, "parseCfgEntry():Found token  in state %d- %d:|%s|",
                         state, token, yytext);
         switch (state)
         {
            case  0:
               if (token == YY_INT || token == YY_IDENT)
                  strcpy(rv->srvc, yylval.s);
               else
               {
                  if (dbg) syslog(LOG_DEBUG, "parseCfgEntry():Syntax Error");
                  *err = ECFG_SYNTAX;
                  free (rv);
                  rv = NULL;
                  continue;
               }
               break;

            case 1:
               if (token == YY_ON)
                  rv->enabled = 1;
               else if (token == YY_OFF)
                  rv->enabled = 0;
               else
               {
                  if (dbg) syslog(LOG_DEBUG, "parseCfgEntry():Syntax Error");
                  *err = ECFG_SYNTAX;
                  free (rv);
                  rv = NULL;
                  continue;
               }
               rv->enb_save = rv->enabled;
               break;

            case 2:
               if (token == YY_DEVICE)
                  strcpy(rv->devc, yylval.s);
               else
               {
                  if (dbg) syslog(LOG_DEBUG, "parseCfgEntry():Syntax Error");
                  *err = ECFG_SYNTAX;
                  free (rv);
                  rv = NULL;
                  continue;
               }
               break;

            case 3:
               if (token < 100 || token >= YY_INT)
               {
                  if (dbg) syslog(LOG_DEBUG, "parseCfgEntry():Syntax Error");
                  *err = ECFG_SYNTAX;
                  free (rv);
                  rv = NULL;
                  continue;
               }
               else if (token >= YY_SPEED)
                  rv->speed = xlate_speed[token - YY_SPEED];
               else if (token >= YY_LFLAG) 
                  rv->tios.c_lflag |= xlate_lflag[token - YY_LFLAG]; 
               else if (token >= YY_CFLAG) 
                  rv->tios.c_cflag |= xlate_cflag[token - YY_CFLAG]; 
               else if (token >= YY_OFLAG) 
                  rv->tios.c_oflag |= xlate_oflag[token - YY_OFLAG]; 
               else if (token >= YY_IFLAG) 
                  rv->tios.c_iflag |= xlate_iflag[token - YY_IFLAG]; 
               else
               {
                  if (dbg) syslog(LOG_DEBUG, "parseCfgEntry():Syntax Error");
                  *err = ECFG_SYNTAX;
                  free (rv);
                  rv = NULL;
                  continue;
               }
               break;
         }
      }
   }
   else
      *err = ECFG_MEMORY;
   return(rv);
}
