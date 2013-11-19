/***************************************************************************
**    termnet.c
**    Termnet mainline code
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
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <ctype.h>
#include <termios.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <strings.h>
#include <errno.h>
#ifdef AIX
#include <memory.h>
#endif
#include "termnet.h"

#ifndef TELOPT_XDISPLOC
#define TELOPT_XDISPLOC 35 /* X Display Location   */
#endif

extern int optind, opterr;
extern char *optarg;

char *pService = "termnetd";
char *pHost = "localhost";   

static int dbg = 0;

static fd_set ReadFds, ExcpFds;

static int argn;

static char *pResponse, responseBuffer[256];

void doExit(int code)
{
   tcsetattr(fileno(userIn), TCSANOW, &OrgTermios);
   puts("\n\r");
   exit(code);
}

void setResponse(char *pBuf)
{
   char *cp, *cp1, ch;

   if (pBuf == NULL)
      return;

   pResponse = responseBuffer;
   *pResponse = '\0';
   for (cp = pBuf, cp1 = responseBuffer; *cp;)
   {
      if ((ch = *(cp++)) == '\\')
      {
         switch (*cp)
         {
            case '\\':
               ch = '\\';
               break;

            case 'n':
               ch = '\n';
               break;

            case 'r':
               ch = '\r';
               break;

            case 'x':
               cp++;

            case '0':
               ch = strtol(cp, &cp, 0);
               break;

            default:
               ch = *cp;
         }
         cp++;
      }
      *(cp1++) = ch;
      *cp1 = '\0';
   }
   gotResponse = 0;
   if (dbg) fprintf(stderr, "setResponse(): Response set to |%s|\n\r", pResponse);
   if (dbg) fflush(stderr);
}

int testResponse(char *pStr, int cnt)
{
   int rv;

   rv = 0;
   while (cnt)
   {
      if (*pStr == *pResponse)
      {
         pStr++;
         cnt--;
         pResponse++;
         if (*pResponse == '\0')
         {
            rv = 1;
            break;
         }
      }
      else if (pResponse != responseBuffer)
         pResponse = responseBuffer;
      else
      {
         pStr++;
         cnt--;
      }
   }
   return(rv);
}

void handleTelNetErrors(int iVal, void *pArg1, void *pArg2)
{
   char buf[256];

   if (iVal == 1)
   {
      strcpy(buf, strerror((int)pArg1));
      fprintf(stderr, "%s:Connection Closed", buf);
      doExit(1);
   }
   else
   {
      sprintf(buf, "Recieved error reading %s", (iVal == 1) ? "socket" : "internal pipe");
      perror(buf);
   }
}

void handleSubOptData(int iVal, void *pArg1, void *pArg2)
{
   char fmtBuf[80], buf[2], ch;

   strcpy(fmtBuf, "");
   switch (iVal)
   {
      case TELOPT_BAUDRATE:
         if (optCnt)
         {
            optCnt--;
            if (!optCnt)
               alarm(0);
         }
         strcpy(fmtBuf, "Baudrate set to: %s\n\r");
         break;

      case TELOPT_PORTSET:
         if (optCnt)
         {
            optCnt--;
            if (!optCnt)
               alarm(0);
         }
         strcpy(fmtBuf, "Port Settings are: %s\n\r");
         break;

      case TELOPT_DEVICE:
         if (optCnt)
         {
            optCnt--;
            if (!optCnt)
               alarm(0);
         }
         strcpy(fmtBuf, "Port Device set to: %s\n\r");
         if (tnlIsOption(TELOPT_BAUDRATE))
            tnlRequestSubOption(TELOPT_BAUDRATE);
         if (tnlIsOption(TELOPT_PORTSET))
            tnlRequestSubOption(TELOPT_PORTSET);
         break;

      case TELOPT_NDEVICE:
         for (ch = 0; ch != 'Q';)
         {
            printf("\rAccept port %s (y/n/q):", (char *)pArg1);
            getInput(buf, sizeof(buf));
            ch = *buf;
            switch (ch = toupper(ch))
            {
               case 'Y':
                  tnlSendSubOption(TELOPT_DEVICE, pArg1);
                  break;

               case 0:
               case 'N':
                  tnlRequestSubOption(iVal);
                  break;

               case 'Q':
                  continue;

               default:
                  printf("Please answer with Y/N/Q\n\r");
                  continue;
            }
            break;
         }
         break;
   }
   if (strlen(fmtBuf) > 0)
      fprintf(userOut, fmtBuf, (char *)pArg1);
}

void handleDoOptions(int iVal, void *pArg1, void *pArg2)
{
   char *cp;

   cp = NULL;   
   if (dbg) fprintf(stderr, "handleDoOpts::Do Opt = %d, optCnt = %d\n\r",
                            iVal, optCnt);
   if (dbg) fflush(stderr);
   switch (iVal)
   {
      case TELOPT_BAUDRATE:
         optOffered = 0;
         if (dbg) fprintf(stderr, "handleDoOpts::BaudRate = %d\n\r", (int)pArg1);
         if (dbg) fflush(stderr);
         if ((int)pArg1)
         {
            optCnt++;
            cp = sBaudRate;
         }
         break;

      case TELOPT_PORTSET:
         if (dbg) fprintf(stderr, "handleDoOpts::PortSet = %d\n\r", (int)pArg1);
         if (dbg) fflush(stderr);
         optOffered = 0;
         if ((int)pArg1)
         {
            optCnt++;
            cp = sPortSettings;
         }
         break;

      case TELOPT_DEVICE:
         if (dbg) fprintf(stderr, "handleDoOpts::Device = %d\n\r", (int)pArg1);
         if (dbg) fflush(stderr);
         optOffered = 0;
         if ((int)pArg1)
         {
            optCnt++;
            cp = sPortDevice;
         }
         break;

      default:
         cp = NULL;
   }
   if (cp != NULL && strlen(cp) > 0)
   {
      if (dbg) fprintf(stderr, "handleDoOpts::Sending opt data %d, |%s|\n\r", iVal, cp);
      if (dbg) fflush(stderr);
      tnlSendSubOption(iVal, cp);
   }
   else if ((int)pArg1 && (iVal == TELOPT_BAUDRATE ||
          iVal == TELOPT_PORTSET ||
          iVal == TELOPT_DEVICE))
      tnlRequestSubOption(iVal);
}

void handleWillOptions(int iVal, void *pArg1, void *pArg2)
{
/*
   if (dbg) fprintf(stderr, "handleWillOptions:Have %s %d!\n\r", (int)pArg1 ? "WILL" : "WONT", iVal);
   if (iVal == TELOPT_ECHO)
   {
      if (dbg) fprintf(stderr, "handleWillOptions:Have %s Echo!\n\r", (int)pArg1 ? "WILL" : "WONT");
      if (!(int)pArg1)
      {
         remoteEcho = 0;
         if (echoFlg)
         {
            localEcho = 1;
            if (dbg) fprintf(stderr, "Local Echo Enabled!\n\r");
         }
         else
            if (dbg) fprintf(stderr, "Remote Echo Disabled!\n\r");
      }
      else if (remoteEcho)
         if (dbg) fprintf(stderr, "Remote Echo Enabled!\n\r");
   }
*/
}

void killOpts(int sig)
{
   optOffered = optCnt = 0;
}

int doTermServ(char *pHost, char *pService, int argc, char *argv[])
{
   int sflg = 0, cnt, rv;
   char commbuf[256], autoOutBuf[256], autoRspBuf[256], *cp, *pInput;
   char hostname[80], domainname[128], tmpbuf[256];

   if (dbg > 1) fprintf(stderr, "Termnet:Initializing telnet library!\n\r");
   if (dbg) fflush(stderr);
   pResponse = responseBuffer;
   if ((rv = tnlInit(pHost, pService, &tnlin, &tnlout, 15)) != -1)
   {
      if (dbg) fprintf(stderr, "Termnet:Initialization complete!\n\r");
      if (dbg) fflush(stderr);
      /*
      ** Enable the proccessing of a few options
      */
      tnlSetCallBack(TNL_ISSUBOPTDATA_CB, handleSubOptData);
      tnlSetCallBack(TNL_DOOPTION_CB, handleDoOptions);
      tnlSetCallBack(TNL_WILLOPTION_CB, handleWillOptions);
      tnlSetCallBack(TNL_ERROR_CB, handleTelNetErrors);
      tnlEnableOption(TELOPT_ECHO);
      tnlEnableOption(TELOPT_SGA);
      tnlEnableOption(TELOPT_BINARY);

      optOffered = 1;
      tnlOfferOption(TELOPT_BAUDRATE, 1);
      tnlOfferOption(TELOPT_PORTSET, 1);
      tnlOfferOption(TELOPT_DEVICE, 1);
      signal(SIGALRM, (void (*)(int))killOpts);
      alarm(5);
      if ((getenv("TERM") != NULL  && strlen(getenv("TERM")) > 0) || 
          (getenv("term") != NULL && strlen(getenv("term")) > 0))
      {
         if (getenv("TERM") != NULL  && strlen(getenv("TERM")) > 0)
            strcpy(commbuf, getenv("TERM"));
         else if (getenv("term") != NULL && strlen(getenv("term")) > 0)
            strcpy(commbuf, getenv("term"));
         for (cp = commbuf; *cp; cp++)
            *cp = toupper(*cp);
         tnlSetSubOption(TELOPT_TTYPE, commbuf);
         tnlOfferOption(TELOPT_TTYPE, 1);
      }
      if (getenv("DISPLAY") != NULL  && strlen(getenv("DISPLAY")) > 0)
      {
         strcpy(tmpbuf, getenv("DISPLAY"));
         if (noHostname == 0 && *tmpbuf == ':')
         {
            gethostname(hostname, sizeof(hostname));
            getdomainname(domainname, sizeof(domainname));
            strcpy(commbuf, hostname);
            if (strlen(domainname) > 0)
            {
               strcat(commbuf, ".");
               strcat(commbuf, domainname);
            }
            strcat(commbuf, tmpbuf);
         }
         else
            strcpy(commbuf, tmpbuf);
         tnlSetSubOption(TELOPT_XDISPLOC, commbuf);
         tnlOfferOption(TELOPT_XDISPLOC, 1);
      }
      if (dbg) fprintf(stderr, "Termnet:Do's & Don'ts Set!\n\r");
      if (dbg) fflush(stderr);
      initialPass = 1;  

      for (;;)
      {
         if (dbg) fprintf(stderr, "Termnet:Main loop\n\r");
         if (dbg) fflush(stderr);
         if (execComplete)
         {
            execComplete = 0;
            RunTermios.c_oflag = 0;
            RunTermios.c_iflag &= ~(IXON | IXOFF | ICRNL) | INLCR;
            RunTermios.c_lflag &= ~(ICANON | ISIG | ECHO);
            RunTermios.c_cc[VMIN] = 1;
            RunTermios.c_cc[VTIME] = 1;
            tcsetattr(fileno(userIn), TCSANOW, &RunTermios);
         }
         FD_ZERO(&ReadFds);
         FD_ZERO(&ExcpFds);
         FD_SET(fileno(tnlin), &ReadFds);
         FD_SET(fileno(stdin), &ReadFds);
         if (dbg > 1) fprintf(stderr, "Termnet:Select parms set\n\r");
         if (dbg) fflush(stderr);

         if (dbg) fprintf(stderr, "Termnet:initialPass = %d\n\r", initialPass);
         if (!execMode && 
             ((autoMode && !feof(userIn)) || (clFlg && argn < argc)))
         {
            if (dbg) fprintf(stderr, "Termnet:Expecting parametrs\n\r");
            if (dbg) fflush(stderr);
            if (!optOffered && !optCnt && (!needResponse || gotResponse))
            {
               needResponse = 1;
               if (dbg) fprintf(stderr, "Termnet:Getting input from command line\n\r");
               if (dbg) fflush(stderr);
               if (autoMode)
                  pInput = nextItem(userIn, autoOutBuf);
               else
                  pInput = argv[argn++];
               if (dbg) fprintf(stderr, "Termnet:input = |%s|\n\r", pInput);
               if (dbg) fflush(stderr);
               if (autoMode || argn < argc)
               {
                  if (dbg) fprintf(stderr, "Termnet:resonse = |%s|\n\r", argv[argn]);
                  if (dbg) fflush(stderr);
                  if (autoMode)
                     setResponse(nextItem(userIn, autoRspBuf));
                  else
                     setResponse(argv[argn++]);
               }
               if (dbg) fprintf(stderr, "Termnet:initialPass = %d\n\r", initialPass);
               if (initialPass)
               {
                  sleep(1);
                  initialPass = 0;
               }
               if (dbg) fprintf(stderr, "Termnet:sending data = |%s|\n\r", pInput);
               if (dbg) fflush(stderr);
               if (pInput != NULL)
                  inputBuffer(pInput);
               else
                  continue;
               fflush(tnlout);
            }
         }
         else if (!contFlag && ((autoMode && feof(userIn)) || 
                               (clFlg && (!needResponse || gotResponse))))
         {
            if (dbg) fprintf(stderr, "Termnet:Done w/ parametrs\n\r");
            if (dbg) fflush(stderr);
            tnlShutdown(2);
            doExit(0);
         }

         if (dbg) fprintf(stderr, "Termnet:Waiting input optCnt = %d, optOffered = %d\n\r",
                          optCnt, optOffered);
         if (dbg) fflush(stderr);
         if ((sflg = tnlSelect(&ReadFds, NULL, &ExcpFds, NULL)) != 0)
         {
            if (dbg > 1) fprintf(stderr, "Termnet:returned from tnlSelect- found input\n\r");
            if (dbg) fflush(stderr);
            if (FD_ISSET(fileno(tnlin), &ReadFds))
            {
               if (dbg > 2) fprintf(stderr, "Termnet:Have input from Network\n\r");
               if (dbg) fflush(stderr);
               if ((cnt = read(fileno(tnlin), commbuf, sizeof(commbuf))) >= 0)
               {
                  if (emulate_parity != 0)
                  {
                     char *cp;
                     int x;

                     for (cp = commbuf, x = cnt; x > 0; cp++, x--)
                     {
                           *cp = *cp & 0x7f;
                     }
                  }
                  if ((gotResponse = testResponse(commbuf, cnt)) != 0)
                     needResponse = 0;

                  if (dbg > 1) fprintf(stderr, "Termnet:Read %d chars from Network\n\r", cnt);
                  if (dbg > 1)
                  {
                     int icnt;
                     char *cp;
                     for (cp = commbuf, icnt = cnt; icnt > 0; icnt--, cp++)
                        fprintf(stderr, "Have char: 0x%x, |%c|\n\r", *cp, *cp);
                  }
                  if (dbg) fflush(stderr);
                  write(fileno(userOut), commbuf, cnt);
               }
               else if (cnt < 0 && errno != EAGAIN && errno != EINTR)
               {
                  perror("Reading from network");
                  doExit(1);
               }
            }

            if (!optOffered && !optCnt && 
                FD_ISSET(fileno(stdin), &ReadFds))
            {
               if (dbg > 1) fprintf(stderr, "Termnet:Have input from keyboard\n\r");
               if (dbg) fflush(stderr);
               if ((cnt = read(fileno(stdin), commbuf, sizeof(commbuf))) > 0)
               {
                  if (dbg > 1)
                  {
                     int icnt;
                     char *cp;
                     for (cp = commbuf, icnt = cnt; icnt > 0; icnt--, cp++)
                        fprintf(stderr, "Have char: 0x%x, |%c|\n\r", *cp, *cp);
                  }
                  inputTerminal(commbuf, cnt);
               }
            }
         }
         else
         {
            if (dbg > 1) fprintf(stderr, "Termnet:returned from tnlSelect- NO input\n\r");
            if (dbg) fflush(stderr);
         }
         fflush(userOut);
         fflush(tnlout);
      }
   }
   else
      perror("Error connecting");
   return(rv);
}

/*
** telnet [options] [<host> [<service>]] [<string> ... ]
**
** Where options can be a combination of the following:
**
**    -b <Baudrate>
**    -p <PortSettings>
**    -d <PortDevice>
**    --                Marks the end of the options (optional)
*/
int main(int argc, char *argv[])
{
   int opt, opFlg;
   char *cp;

   userIn = stdin;
   userOut = stdout;

   cp = getenv("TERM");
   if (dbg) fprintf(stderr, "$TERM = %s\n", cp);
   echoFlg = localEcho = remoteEcho = 0;
   noHostname = 0;
   needResponse = 0;
   opterr = 0;
   emulate_parity = 0;

   signal(SIGINT, SIG_IGN);
   for (opFlg = 1; opFlg && (opt = getopt(argc, argv, "hec7f:o:p:b:d:-")) != -1;)
   {
      switch (opt)
      {
         case '7':
            emulate_parity = 1;
            break;

         case 'b':
            strcpy(sBaudRate, optarg);
            break;

         case 'c':
            contFlag = 1;
            break;

         case 'e':
            echoFlg = localEcho = 1;
            break;

         case 'f':
            if ((userIn = fopen(optarg, "r")) == NULL)
            {
               perror("Error opening input file");
               doExit(1);
            }
            autoMode = 1;
            break;

         case 'h':
            noHostname = 1;
            break;

         case 'o':
            if ((userOut = fopen(optarg, "w")) == NULL)
            {
               perror("Error opening output file");
               doExit(1);
            }
            break;

         case 'd':
            strcpy(sPortDevice, optarg);
            break;

         case 'p':
            strcpy(sPortSettings, optarg);
            break;

         case '?':
         default:
            optind--;
            opFlg = 0;
            break;
      }
   }
   argv += optind;
   argc -= optind;

   if (dbg) fprintf(stderr, "main():argc = %d, argv[1] = %s\n\r", argc, argv[0]);
   argn = 0;
   if (argc > 0)
      clFlg = 1;
   else
      clFlg = 0;

   if (autoMode == 0)
      contFlag = 1;

   if (argc >= 2)
   {
      if (getservbyname(argv[1], "tcp") != NULL ||
          htons((u_short)atoi(argv[1])) != 0)
      {
         argn++;
         pService = argv[1];
      }
   }

   if (argc >= 1)
   {
      if (gethostbyname(argv[0]) != NULL ||
          inet_addr(argv[0]) != INADDR_NONE)
      {
         argn++;
         pHost = argv[0];
      }
   }

   if (dbg) fprintf(stderr, "main():argc = %d, argn = %d, argv[1] = %s\n\r", argc, argn, argv[0]);
   if (dbg) fprintf(stderr, "main():pHost = |%s|, pService = |%s|\n\r", pHost, pService);
   if (argn == argc)
      clFlg = 0;

   tcgetattr(fileno(userIn), &OrgTermios);   /* get current console tty mode  */
   RunTermios = OrgTermios;                  /* copy (structure) to RunTermios  */

   RunTermios.c_oflag = 0;
   RunTermios.c_iflag &= ~(IXON | IXOFF | ICRNL | INLCR);
   RunTermios.c_lflag &= ~(ICANON | ISIG | ECHO);
   RunTermios.c_cc[VMIN] = 1;
   RunTermios.c_cc[VTIME] = 1;
   tcsetattr(fileno(userIn), TCSANOW, &RunTermios);

   doTermServ(pHost, pService, argc, argv);
   doExit(0);
   return(0); /* keep the compiler warnings to a minimum */
}
