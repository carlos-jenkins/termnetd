/***************************************************************************
**    tnlSocket.c
**    Telnet library Socket I/O funtions
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
#include <stdlib.h>
#include "tnlP.h"

static int dbg = 0;

/*
** tnlSendSpecial(unsignned char *ch)
**
** This function will send a Telnet Special character by preceding the
** character passed in ch with an IAC character
*/
void tnlSendSpecial(unsigned char ch)
{
   putc(IAC, nfp);
   putc(ch, nfp);
   fflush(nfp);
}

/*
** int abortFsm(int ch)
**
** This function is used for dealing with invalid states encountered by the 
** state machine. 
**
** If a call back function was set up for dealing with errors (TNL_ERROR_CB)
** it will be called with TNL_INVLDSTATE_ERR as its argument. Otherwise,
** the exit will be called with an argument of 1.
*/
void abortFsm(int ch)
{
   if (!doCallBack(TNL_ERROR_CB, TNL_INVLDSTATE_ERR, NULL, NULL))
   {
      fprintf(stderr, "Aborting State Machine!");
      exit(1);
   }
}

/*
** This function proccesses character coming in from the socket
** with the use of the main state machine for the socket.
*/
void inputNet(unsigned char *buf, int len)
{
   trans_table *pFsm;
   int x, index;
   unsigned char ch;

   for (x = 0; x < len; ++x)
   {
      ch = buf[x];

      index = SocketFsm[socketState][ch];
      pFsm = &SocketTrans[index];
      (pFsm->action)(ch);
      socketState = pFsm->next;
   }
}

/*
** This function outputs a buffer of characters to the socket.
** if an IAC character is encountered in the buffer, it will
** be sent twice.
*/
void outputNet(unsigned char *pBuf, int len)
{
   for (; len > 0; len--)
   {
      if (*pBuf == IAC)
      {
         if (dbg) fprintf(stderr, "outputNet(): sending char to net 0x%x, |%c|\n", *pBuf, *pBuf);
         write(fileno(nfp), pBuf, 1);
      }
      if (dbg) fprintf(stderr, "outputNet(): sending char to net 0x%x, |%c|\n", *pBuf, *pBuf);
      write(fileno(nfp), pBuf, 1);
      pBuf++;
   }
}

static void doSpecial(int ch)
{
   doCallBack(TNL_SPECIALCHAR_CB, ch, NULL, NULL);
}

/*
** This function passes data from the socket to the application
** through pipe for the application.
*/
static void spputc(int ch)
{
    unsigned char c;

   if (nSynch)
      return;

   c = ch;
   write(fileno(aplout), &c, 1);
   if (dbg) fprintf(stderr, "spputc(): sending char to pipe 0x%x, |%c|\n", c, c);
}

/*
** This function turns off the data synching operation
*/
static void tcdm(int ch)
{
   nSynch = 0;
}

/*
** This function saves the character passed in ch as the 
** current option command (DO/DONT, WILL/WONT).
*/
static void recopt(int ch)
{
   cOpt = ch;
}

void no_op(int ch)
{
   return;
}

trans_table SocketTrans[] =
{
  /* State     Input             Next State        Action      */
   { TS_DATA,  IAC,              TS_IAC,           no_op       },
   { TS_DATA,  TCANY,            TS_DATA,          spputc      },
   { TS_IAC,   IAC,              TS_DATA,          spputc      },
   { TS_IAC,   SB,               TS_SUBNEG,        no_op       },

   /* Telnet commands         */
   { TS_IAC,   NOP,              TS_DATA,          no_op       },
   { TS_IAC,   DM,               TS_DATA,          tcdm        },

   /* Option Negotiation      */
   { TS_IAC,   WILL,             TS_WOPT,          recopt      },
   { TS_IAC,   WONT,             TS_WOPT,          recopt      },
   { TS_IAC,   DO,               TS_DOPT,          recopt      },
   { TS_IAC,   DONT,             TS_DOPT,          recopt      },
   { TS_IAC,   TCANY,            TS_DATA,          doSpecial   },

   { TS_WOPT,   TCANY,           TS_DATA,          doOption    },
   { TS_DOPT,   TCANY,           TS_DATA,          willOption  },

   /* Option Subnegotiation   */
   { TS_SUBNEG, IAC,             TS_SUBIAC,        no_op       },
   { TS_SUBNEG, TCANY,           TS_SUBNEG,        inputSubOpt },
   { TS_SUBIAC, SE,              TS_DATA,          endSubOpt   },
   { TS_SUBIAC, TCANY,           TS_SUBNEG,        inputSubOpt },

   { FSINVALID, TCANY,           FSINVALID,        abortFsm    }
};
