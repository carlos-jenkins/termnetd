/***************************************************************************
**    ttyd.c
**    Termnet device forwording daemon mainline code
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
#include "../sysdefs.h"
#include <arpa/telnet.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <syslog.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <ctype.h>
#include <fcntl.h>
#include <termios.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <strings.h>
#include <errno.h>
#ifdef AIX
#include <memory.h>
#endif
#include "termnet.h"

#if defined (AIX) || defined(SunOS)
#define LOG_PERROR 0
#endif

#ifndef TELOPT_XDISPLOC
#define TELOPT_XDISPLOC 35 /* X Display Location   */
#endif

extern int optind, opterr;
extern char *optarg;
extern char break_char;

char *pService = "termnetd";
char *pHost = "localhost";   

static int dbg = 2;

static fd_set ReadFds, ExcpFds;

static int argn, netClosed;

static char *pResponse, responseBuffer[256];

void doExit(int code)
{
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
   if (dbg) syslog(LOG_DEBUG, "setResponse(): Response set to |%s|", pResponse);
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

   syslog(LOG_DEBUG, "Have some form of error!");
   if (iVal == 1)
   {
      sprintf(buf, "Error String: |%d|", (int)pArg1);
      syslog(LOG_DEBUG, "%s:Connection Closed", buf);
      netClosed = 1;
   }
   else
   {
      syslog(LOG_ERR, "Recieved error reading %s:%m", (iVal == 1) ? "socket" : "internal pipe");
   }
}

void handleSubOptData(int iVal, void *pArg1, void *pArg2)
{
   char fmtBuf[80];

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
         strcpy(fmtBuf, "Baudrate set to: %s");
         break;

      case TELOPT_PORTSET:
         if (optCnt)
         {
            optCnt--;
            if (!optCnt)
               alarm(0);
         }
         strcpy(fmtBuf, "Port Settings are: %s");
         break;

      case TELOPT_DEVICE:
         if (optCnt)
         {
            optCnt--;
            if (!optCnt)
               alarm(0);
         }
         strcpy(fmtBuf, "Port Device set to: %s");
         if (tnlIsOption(TELOPT_BAUDRATE))
            tnlRequestSubOption(TELOPT_BAUDRATE);
         if (tnlIsOption(TELOPT_PORTSET))
            tnlRequestSubOption(TELOPT_PORTSET);
         break;
   }
   if (strlen(fmtBuf) > 0)
      syslog(LOG_INFO, fmtBuf, (char *)pArg1);
}

void handleDoOptions(int iVal, void *pArg1, void *pArg2)
{
   char *cp;

   cp = NULL;   
   if (dbg) syslog(LOG_DEBUG, "handleDoOpts::Do Opt = %d, optCnt = %d",
                            iVal, optCnt);
   switch (iVal)
   {
      case TELOPT_BAUDRATE:
         optOffered = 0;
         if (dbg) syslog(LOG_DEBUG, "handleDoOpts::BaudRate = %d", (int)pArg1);
         if ((int)pArg1)
         {
            optCnt++;
            cp = sBaudRate;
         }
         break;

      case TELOPT_PORTSET:
         if (dbg) syslog(LOG_DEBUG, "handleDoOpts::PortSet = %d", (int)pArg1);
         optOffered = 0;
         if ((int)pArg1)
         {
            optCnt++;
            cp = sPortSettings;
         }
         break;

      case TELOPT_DEVICE:
         if (dbg) syslog(LOG_DEBUG, "handleDoOpts::Device = %d", (int)pArg1);
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
      if (dbg) syslog(LOG_DEBUG, "handleDoOpts::Sending opt data %d, |%s|", iVal, cp);
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

int doTermServ(int fd, char *pHost, char *pService)
{
   int sflg = 0, cnt, rv;
   char commbuf[256];

   if (dbg > 1) syslog(LOG_DEBUG, "Termnet:Initializing telnet library!");
   pResponse = responseBuffer;
   if ((rv = tnlInit(pHost, pService, &tnlin, &tnlout, 15)) != -1)
   {
      if (dbg) syslog(LOG_DEBUG, "Termnet:Initialization complete!");
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
      if (dbg) syslog(LOG_DEBUG, "Termnet:Options offered!");

      for (netClosed = 0; netClosed == 0;)
      {
         FD_ZERO(&ReadFds);
         FD_ZERO(&ExcpFds);
         FD_SET(fileno(tnlin), &ReadFds);
         FD_SET(fd, &ReadFds);

         if ((sflg = tnlSelect(&ReadFds, NULL, &ExcpFds, NULL)) != 0)
         {
            if (dbg > 2) syslog(LOG_DEBUG, "Termnet:returned from tnlSelect- found input");
            if (FD_ISSET(fileno(tnlin), &ReadFds))
            {
               if (dbg > 1) syslog(LOG_DEBUG, "Termnet:Have input from Network");
               if ((cnt = read(fileno(tnlin), commbuf, sizeof(commbuf))) >= 0)
               {
                  if ((gotResponse = testResponse(commbuf, cnt)) != 0)
                     needResponse = 0;

                  if (dbg > 1) syslog(LOG_DEBUG, "ttyd:Read %d chars from Network", cnt);
                  if (dbg > 1)
                  {
                     char *cp;
                     int x;
                     for (cp = commbuf, x = 0; x < cnt; x++, cp++)
                        syslog(LOG_DEBUG, "ttyd: Have net char 0x%x, |%c|", *cp, *cp);
                  }
                  write(fd, commbuf, cnt);
               }
               else if (cnt < 0 && errno != EAGAIN && errno != EINTR)
               {
                  syslog(LOG_ERR, "Received error reading from network:%m");
                  break;
               }
               if (cnt <= 0)
               {
                  if (dbg) syslog(LOG_DEBUG, "Network Closed!");
                  break;
               }
            }
            else if (FD_ISSET(fd, &ReadFds))
            {
               if (dbg > 1) syslog(LOG_DEBUG, "Termnet:Have input from the keyboard");
               if ((cnt = read(fd, commbuf, sizeof(commbuf))) > 0)
               {
                  if (dbg > 1) syslog(LOG_DEBUG, "Termnet:Read %d chars from the keyboard", cnt);
                  if (dbg > 1)
                  {
                     char *cp;
                     int x;
                     for (cp = commbuf, x = 0; x < cnt; x++, cp++)
                        syslog(LOG_DEBUG, "ttyd: Have key char 0x%x, |%c|", *cp, *cp);
                  }
                  inputTerminal(commbuf, cnt);
               }
               else
                  break;
            }
         }
         else
         {
            if (dbg > 1) syslog(LOG_DEBUG, "Termnet:returned from tnlSelect- NO input");
         }
         fflush(userOut);
         fflush(tnlout);
      }
      tnlShutdown(2);
   }
   else
   {
      syslog(LOG_ERR, "Error Connecting:%s %s", pHost, pService);
   }
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
   int opt, opFlg, fd, NoDetach, cpid;
   char *cp;

   NoDetach = 0;
   memset(sDevice, '\0', sizeof(sDevice));
   cp = getenv("TERM");
   if (dbg) syslog(LOG_DEBUG, "$TERM = %s\n", cp);
   echoFlg = localEcho = remoteEcho = 0;
   noHostname = 0;
   needResponse = 0;
   opterr = 0;
   signal(SIGINT, SIG_IGN);
   for (opFlg = 1; opFlg && (opt = getopt(argc, argv, "np:b:d:-")) != -1;)
   {
      switch (opt)
      {
         case 'p':
            strcpy(sPortSettings, optarg);
            break;

         case 'b':
            strcpy(sBaudRate, optarg);
            break;

         case 'd':
            strcpy(sDevice, optarg);
            break;

         case 'n':
            NoDetach = 1;
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

   /*
   ** If they didn't ask us not to, detach from the controlling terminal
   ** and chdir to /etc.
   */
   if (NoDetach == 0)
   {
      openlog("ttyd", LOG_PID | LOG_CONS, LOG_DAEMON);
      if ((cpid = fork()) > 0)
         exit(0);
      else if (cpid < 0)
      {
         syslog(LOG_ERR, "Quiting!! Error Detaching from terminal:%m");
         exit(1);
      }
      else
      {
         chdir("/etc");
         close(0);
         close(1);
         close(2);
      }
   }
   else
      openlog("ttyd", LOG_PID | LOG_CONS | LOG_PERROR, LOG_DAEMON);

   if (dbg) syslog(LOG_DEBUG, "main():argc = %d, argv[1] = %s", argc, argv[0]);
   argn = 0;
   if (argc > 0)
      clFlg = 1;
   else
      clFlg = 0;

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

   if (dbg) syslog(LOG_DEBUG, "main():argc = %d, argn = %d, argv[1] = %s", argc, argn, argv[0]);
   if (dbg) syslog(LOG_DEBUG, "main():pHost = |%s|, pService = |%s|", pHost, pService);
   if (argn == argc)
      clFlg = 0;

   break_char = '\0';
   while ((fd = openPtyDevice(sDevice)) >= 0)
   {

      FD_ZERO(&ReadFds);
      FD_SET(fd, &ReadFds);
      select(fd+1, &ReadFds, NULL, NULL, NULL);
      if (dbg) syslog(LOG_DEBUG, "main():Calling doTermServ()!");
      tcgetattr(fd, &OrgTermios);   /* get current console tty mode  */
      RunTermios = OrgTermios;                  /* copy (structure) to RunTermios  */

      RunTermios.c_oflag = 0;
      RunTermios.c_iflag &= ~(IXON | IXOFF | ICRNL | INLCR) | IGNCR;
      RunTermios.c_lflag &= ~(ICANON | ISIG | ECHO);
      RunTermios.c_cc[VMIN] = 1;
      RunTermios.c_cc[VTIME] = 1;
      tcsetattr(fd, TCSANOW, &RunTermios);
      doTermServ(fd, pHost, pService);
      close(fd);
      sleep(1);
   }
   if (dbg) syslog(LOG_DEBUG, "main():All done!");
   doExit(0);
   return(0);
}
