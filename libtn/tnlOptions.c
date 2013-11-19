/***************************************************************************
**    tnlOptions.c
**    Telnet library Option handling funtions
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
#include <arpa/telnet.h>
#include <stdio.h>
#include <stdlib.h>
#include "tnlP.h"

static int dbg = 0;

/*
** This function responds appropriatly to WILLs and WONTs depending of the
** state of the option flag in Options
*/
void doOption(int opt)
{
   if (dbg) fprintf(stderr, "doOption: opt:%d = %d, cOpt = %d\r\n", opt, Options[opt], cOpt);
   if ((Options[opt] == 0 && cOpt == WILL) || 
      (Options[opt] > 0 && cOpt == WONT))
   {
      if (dbg) fprintf(stderr, "doOption:Changing Option state\r\n");   
      Options[opt] = !Options[opt];
      putc(IAC, nfp);
      if (Options[opt] > 0)
      {
         if (dbg) fprintf(stderr, "doOption:Sending Do\r\n");   
         putc(DO, nfp);
      }
      else
      {
         if (dbg) fprintf(stderr, "doOption:Sending Dont\r\n");   
         putc(DONT, nfp);
      }
      putc(opt, nfp);
   }
   else if (Options[opt] < 0)
   {
      if (dbg) fprintf(stderr, "doOption:DONT\r\n");   
      putc(IAC, nfp);
      putc(DONT, nfp);
      putc(opt, nfp);
   }
   if (dbg) fprintf(stderr, "doOption:Calling Callback!!\r\n");   
   doCallBack(TNL_WILLOPTION_CB, opt, (void *)Options[opt], NULL);
}
      
/*
** This function responds appropriatly to DOs and DONTs depending of the
** state of the option flag in Options
*/
void willOption(int opt)
{
   if (dbg) fprintf(stderr, "willOption: opt:%d = %d, cOpt = %d\r\n", opt, Options[opt], cOpt);
   if ((Options[opt] == 0 && cOpt == DO) || 
      (Options[opt] > 0 && cOpt == DONT))
   {
      if (dbg) fprintf(stderr, "willOption:Changing Option state\r\n");   
      Options[opt] = !Options[opt];
      putc(IAC, nfp);
      if (Options[opt] > 0)
      {
         if (dbg) fprintf(stderr, "willOption:Sending WILL\r\n");   
         putc(WILL, nfp);
      }
      else
      {
         if (dbg) fprintf(stderr, "willOption:Sending WONT\r\n");   
         putc(WONT, nfp);
      }
      putc(opt, nfp);
   }
   else if (Options[opt] < 0)
   {
      if (dbg) fprintf(stderr, "willOption:WONT\r\n");   
      putc(IAC, nfp);
      putc(WONT, nfp);
      putc(opt, nfp);
   }
   if (dbg) fprintf(stderr, "willOption:Calling Callback!!\r\n");   
   doCallBack(TNL_DOOPTION_CB, opt, (void *)Options[opt], NULL);
}

/*
** void tnlRequestOption(int opt, int do)
**
** The function requests the other end DOES/DOESNT do the option opt. If 'do'
** is TRUE, the function requests DO, otherwise DONT
**
** As a side effect, if an option's value in the Option table is < 0 
** indicating it is 'forced' dis-abled, it will be set to either a 1 or 0
** to allow it to be settable
*/
void tnlRequestOption(int opt, int _do)
{
   putc(IAC, nfp);
   if (_do)
   {
      if (dbg) fprintf(stderr, "requestOption: DO opt:%d\r\n", opt);
      Options[opt] = 1;
      putc(DO, nfp);
   }
   else
   {
      if (dbg) fprintf(stderr, "requestOption: DONT opt:%d\r\n", opt);
      Options[opt] = 0;
      putc(DONT, nfp);
   }
   putc(opt, nfp);
   fflush(nfp);
}

/*
** void tnlOfferOption(int opt, int will)
**
** The function offers to DO/DONT the option opt to the other end. If 'will'
** is TRUE, the function requests WILL, otherwise WONT
**
** As a side effect, if an option's value in the Option table is < 0 
** indicating it is 'forced' dis-abled, it will be set to either a 1 or 0
** to allow it to be settable
*/
void tnlOfferOption(int opt, int will)
{
   putc(IAC, nfp);
   if (will)
   {
      if (dbg) fprintf(stderr, "offerOption: WILL opt:%d\r\n", opt);
      Options[opt] = 1;
      putc(WILL, nfp);
   }
   else
   {
      if (dbg) fprintf(stderr, "offerOption: WONT opt:%d\r\n", opt);
      Options[opt] = 0;
      putc(WONT, nfp);
   }
   putc(opt, nfp);
   fflush(nfp);
}

/*
** void tnlDisableOption(int opt, int will)
**
** This function sends a WONT to the other side then forces the option to be 
** disabled by setting its value in the option table to -1.
*/
void tnlDisableOption(int opt)
{
   tnlOfferOption(opt, 0);
   Options[opt] = -1;
}

/*
** void tnlEnableOption(int opt, int will)
**
** This function silently removes a forced disable of an option
** by setting it value in the option table to 0
*/
void tnlEnableOption(int opt)
{
   Options[opt] = 0;
}

/*
** int tnlIsOption(int opt)
**
** This function tests the state of an option, if it is enabled, a TRUE will
** be returned, Otherwise a FALSE will be returned.
*/
int tnlIsOption(int opt)
{
   if (dbg) fprintf(stderr, "isOption:Opt: %d = %d!!\r\n", opt, Options[opt]);   
   if (Options[opt] <= 0)
      return(0);
   else
      return(1);
}
