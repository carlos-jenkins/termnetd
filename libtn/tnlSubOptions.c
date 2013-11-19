/***************************************************************************
**    tnlSubOptions.c
**    Telnet library Sub-option handling functions
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
#include <string.h>
#include <syslog.h>
#include "fsm.h"
#include "tnlP.h"

static int option, optionCnt;
static char *pOption, optionBuffer[256];
static int dbg = 0;

/*
** This function is used for proccessing characters from the socket
** specificaly used in sub option negationation by using the state
** machine for sub option negotiation.
*/
void inputSubOpt(int ch)
{
   trans_table *pFsm;
   int index;

   index = SubOptionFsm[suboptionState][ch];
   pFsm = &SubOptionTrans[index];
   (pFsm->action)(ch);
   suboptionState = pFsm->next;
}

/*
** This function proccess the character with the statemachine then
** forces the next state to the Starting state.
*/
void endSubOpt(int ch)
{
   inputSubOpt(ch);
   suboptionState = SS_START;
}

/*
** This function sets up the pointers etc used for gathering
** the suboptions value.
*/
static void setOption(int ch)
{
   option = ch;
   optionCnt = 0;
   pOption = optionBuffer;
   *pOption = '\0';
}

/*
** This function stores the character passed into the 
** suboption buffer, then increments the pointer in 
** anticipation of the next character.
*/
static void getOption(int ch)
{
   if (strlen(optionBuffer) < sizeof(optionBuffer) - 1)
   {
      *(pOption++) = ch;
      *pOption = '\0';
   }
}      

/*
** This function sends the data for the option specified by the 
** variable option, to the other end. This data must have either 
** been previously recieved through the socket or set up be the
** with the tnlSetSubOption(...) function.
*/ 
static void sendOption(int ch)
{
   doCallBack(TNL_SENDSUBOPTDATA_CB, option, (void *)pSubOptions[option], NULL);
   putc(IAC, nfp);
   putc(SB, nfp);
   putc(option, nfp);
   putc(TELQUAL_IS, nfp);
   if (pSubOptions[option] != NULL)
      fputs(pSubOptions[option], nfp);
   else
      putc(' ', nfp);
   putc(IAC, nfp);
   putc(SE, nfp);
   fflush(nfp);
}

/*
** static void doOption(int ch)
**
** This function stores the data gathered from the socket in optionBuffer
** for the option specified by the variable option into long term storage.
** If a call back function was set up for TNL_SUBOPTION_CB, it will be called
** with the suboption and a pointer to the suboptions data as parameters.
** If there is not enough memory available for the storage of the string,
** the call back for TNL_ERROR_CB will be called if it was set up.
*/
static void doSubOption(int ch)
{
   if (dbg) syslog(LOG_DEBUG, "doSubOption():Enter");   
   if (dbg) syslog(LOG_DEBUG, "doSubOption():Option |%s|", optionBuffer);
   if (dbg) syslog(LOG_DEBUG, "doSubOption():Copying new option |%s| to %lx",
                               optionBuffer, (unsigned long)pSubOptions[option]);
   strcpy(pSubOptions[option], optionBuffer);
   if (dbg) syslog(LOG_DEBUG, "doSubOption():Calling Call Back");
   doCallBack(TNL_ISSUBOPTDATA_CB, option, (void *)pSubOptions[option], NULL);
   if (dbg) syslog(LOG_DEBUG, "doSubOption():Exit");
}     

/*
** This function saves the string passed by pointer w/ the pStr argument
** into the long term sub option storage. If a string was already present
** for the suboption, it will be destroyed.
**
** If there is in sufficient memory availabe to store the string, a -1
** will be returned. Otherwise a 0 will be returned.
*/
int tnlSetSubOption(int opt, char *pStr)
{
   int rv;

   if (dbg) syslog(LOG_DEBUG, "tnlSetSubOption(%d, |%s|):Enter", opt, pStr);   
   if (dbg) syslog(LOG_DEBUG, "tnlSetSubOption():Copying string to %lx",
                              (unsigned long)pSubOptions[opt]);   
   strcpy(pSubOptions[opt], pStr);
   rv = 0;
   if (dbg) syslog(LOG_DEBUG, "tnlSetSubOption():Exit(%d)", rv);   
   return(rv);
}     

/*
** int tnlSendSubOption(int opt, char *pSubOption)
**
** This function sends the data for the suboption specified by the argument
** opt to the other end. if pSubOption is not NULL, the suboptions data
** will be set to it with the tnlSetSubOption function first.
**
** The return value will be 0 if there were no problems, otherwise
** it will be -1 if there was no memory available for storing the supplied
** data for the suboption.
*/
int tnlSendSubOption(int opt, char *pSubOption)
{
   int rv;

   rv = 0;
   if (pSubOption != NULL)
      rv = tnlSetSubOption(opt, pSubOption);
   if (rv == 0)
   {
      option = opt;
      sendOption(0);
   }
   return(rv);
}

/*
** void tnlRequestSubOption(int opt)
**
** This function requests the other end to send the data it has for
** the suboption specified by opt. 
*/
void tnlRequestSubOption(int opt)
{
   putc(IAC, nfp);
   putc(SB, nfp);
   putc(opt, nfp);
   putc(TELQUAL_SEND, nfp);
   putc(IAC, nfp);
   putc(SE, nfp);
   fflush(nfp);
}

trans_table SubOptionTrans[] =
{
  /* State     Input             Next State        Action      */
   { SS_START, TCANY,            SS_OPT,        setOption      },

   { SS_OPT,   TELQUAL_IS,       SS_GET,           no_op       },
   { SS_OPT,   TELQUAL_SEND,     SS_END,           sendOption  },
   { SS_OPT,   TCANY,            SS_END,           no_op       },

   { SS_GET,   SE,               SS_END,           doSubOption },
   { SS_GET,   TCANY,            SS_GET,           getOption   },

   { SS_END,   TCANY,            SS_END,           no_op       },

   { FSINVALID, TCANY,           FSINVALID,        abortFsm    }
};
