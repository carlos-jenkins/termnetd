/***************************************************************************
**    termnetd.c
**    Termnetd mainline code
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
#include <errno.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <arpa/telnet.h>
#include "tnl.h"
#include "termnetd.h"

static int dbg = 0;

extern int optind, opterr;
extern char *optarg;

#ifdef OSF
extern int errno;
#endif

static portConf *pDevc;
static char confFile[1024] = "/etc/termnetd.conf";
static int Retry = 0;

#if defined (AIX) || defined(SunOS)
#define LOG_PERROR 0
#endif

void reloadConfig(int sig)
{
   syslog(LOG_INFO, "Reloading Configuration File %s", confFile);
   Restart = 1;
   Retry = 0;
}

void retryConnections(int sig)
{
   syslog(LOG_INFO, "Retrying Failed Connections");
   Restart = Retry = 1;
}

void ignoreSignal(int sig)
{
   syslog(LOG_INFO, "SIGHUP Received by parent!");
}

void enablePort(int sig)
{
   syslog(LOG_INFO, "SIGUSR2 Received by parent!");
   PortEnabled = 1;
}

void disablePort(int sig)
{
   syslog(LOG_INFO, "SIGUSR2 Received by parent!");
   PortEnabled = 0;
}

/*
** termnetd [options]
**
**    Options:
**
**       -c <file>   Use Alternate Configuration file 
**                   def: /etc/termnetd.conf
**
**       -d <dbg>    Set debug message 
**
**       -p          Permiscuous mode, no password needed to
**                   disable or disconnect ports
**
**       -n          Don't detach from the controlling terminal
**
**       -v          Verbose Message logging
*/

int main(int argc, char *argv[])
{
   int opt, cpid;
   struct sigaction sigact;

   while ((opt = getopt(argc, argv, "c:d:ps:nu:v")) != -1)
   {
      switch (opt)
      {
         case 'c':
            strcpy(confFile, optarg);
            break;

         case 'd':
            Debug = atoi(optarg);
            break;

         case 'p':
            Permiscuous = 1;
            break;

         case 'n':
            NoDetach = 1;
            break;

         case 'u':
            strcpy(PrivilegedUser, optarg);
            break;

         case 'v':
            Verbose = 1;
            break;

         case 's':
            strcpy(ControlPort, optarg);
            break;

         break;
      }
   }



   /*
   ** If they didn't ask us not to, detach from the controlling terminal
   ** and chdir to /etc.
   */
   if (NoDetach == 0)
   {
      openlog("termnetd", LOG_PID | LOG_CONS, LOG_DAEMON);
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
      openlog("termnetd", LOG_PID | LOG_CONS | LOG_PERROR, LOG_DAEMON);

   if (loadConfig(confFile) < 0)
   {
      syslog(LOG_ERR, "Quiting!! Error opening config file %s:%m", confFile);
      exit(1);
   }
   memset(&sigact, 0, sizeof(sigact));
   sigact.sa_flags = SA_RESTART;

   sigact.sa_handler = reloadConfig;
   sigaction(SIGHUP, &sigact, NULL);
   sigact.sa_handler = retryConnections;
   sigaction(SIGALRM, &sigact, NULL);
   sigact.sa_handler = SIG_IGN;
   sigaction(SIGCHLD, &sigact, NULL);
   sigact.sa_handler = ignoreSignal;
   sigaction(SIGHUP, &sigact, NULL);
   for (;;)
   {
      Retry = 0;
      if (dbg) syslog(LOG_DEBUG, "main():Opening Sockets!");
      openSockets(Retry);
      if (dbg) syslog(LOG_DEBUG, "main():Calling Socket Select!");
      socketSelect();
      if (Retry == 0)
      {
         if (dbg) syslog(LOG_DEBUG, "main():Closing Sockets!");
         closeSockets();
      }
      if (dbg) syslog(LOG_DEBUG, "main():Loading Config file!");
      if (loadConfig(confFile) < 0)
      {
         syslog(LOG_ERR, "Quiting!! Error opening config file %s:%m", confFile);
         exit(1);
      }
      if (dbg) syslog(LOG_DEBUG, "main():Grabbing 60 winks!");
   }
#ifndef SCO
   exit(0);
#endif
}

void handleSpecialChars(int iVal, void *pArg1, void *pArg2)
{
   if (iVal == BREAK)
      SendBreak(pDevc->mfd);
}

void handleTelNetErrors(int iVal, void *pArg1, void *pArg2)
{
   if (iVal != 1)
      syslog(LOG_ERR, "Recieved Telnet error %d, rv = %d", iVal, (int)pArg1); 
   else
   {
      syslog(LOG_ERR, "Quit: Socket Closed");
      close(0);
      exit(1);
   }
}

void handleNewSubOptData(int iVal, void *pArg1, void *pArg2)
{
   char buff[80];

   if (dbg) syslog(LOG_DEBUG, "handleNewSubOptData():New SubOpt = %d", iVal);
   strcpy(buff, "");
   switch (iVal)
   {
      case TELOPT_BAUDRATE:
         if (dbg) syslog(LOG_DEBUG, "handleNewSubOptData():Setting baudrate to %s",
                                     (const char *)pArg1);
         setBaudStr(pDevc, pArg1);
         setModem(pDevc->mfd, pDevc);
         strcpy(buff, pDevc->baud);
         if (dbg) syslog(LOG_DEBUG, "handleNewSubOptData():Baudrate set to %s",
                                     pDevc->baud);
         break;

      case TELOPT_PORTSET:
         if (dbg) syslog(LOG_DEBUG, "handleNewSubOptData():Setting port settings to %s",
                                     (const char *)pArg1);
         setPortStr(pDevc, pArg1);
         setModem(pDevc->mfd, pDevc);
         strcpy(buff, pDevc->port);
         if (dbg) syslog(LOG_DEBUG, "handleNewSubOptData():Port Settings set to %s",
                                     pDevc->port);
         break;

      case TELOPT_DEVICE:
         if (dbg) syslog(LOG_DEBUG, "handleNewSubOptData():Have Device request for %s",
                                     (const char *)pArg1);
         strcpy(buff, pDevc->devc);
         if (dbg) syslog(LOG_DEBUG, "handleNewSubOptData():Port Settings set to %s",
                                     pDevc->port);
         break;
   }
   if (strlen(buff) > 0)
   {
      if (dbg) syslog(LOG_DEBUG, "handleNewSubOptData():Sending %s back", buff);
      tnlSendSubOption(iVal, buff);
   }
}

void hangup(int sig)
{
   syslog(LOG_INFO, "TTY Closed!, killing self");
   exit(0);
}

void termnetd(portConf *pDevice)
{
   int mfd, mError = 0, sflg = 0, cnt, rv;
   FILE *tnlin = NULL, *tnlout = NULL, *mfp = NULL;
   char commbuf[256];
   fd_set readFds, excpFds;
   int nSynch = 0;
   struct sigaction sigact;

   if (dbg) syslog(LOG_DEBUG, "telnetd():Enter");
   memset(&sigact, 0, sizeof(sigact));
   sigact.sa_flags = SA_RESTART;

   sigact.sa_handler = disablePort;
   sigaction(SIGUSR1, &sigact, NULL);
   sigact.sa_handler = enablePort;
   sigaction(SIGUSR2, &sigact, NULL);
   sigact.sa_handler = SIG_IGN;
   sigaction(SIGALRM, &sigact, NULL);
   sigact.sa_handler = SIG_IGN;
   sigaction(SIGCHLD, &sigact, NULL);

   pDevc = pDevice;
   mfd = openModem(pDevc);
   pDevc->mfd = mfd;

   PortEnabled = 1;
   if ((rv = tnlInit(NULL, NULL, &tnlin, &tnlout, 15)) != -1)
   {
      if (dbg) syslog(LOG_DEBUG, "telnetd():libtn initialized");

      /*
      ** Enable the proccessing of a few options & set the 
      ** Baud rate and port settings of the device
      */
      tnlSetCallBack(TNL_ISSUBOPTDATA_CB, handleNewSubOptData);
      tnlSetCallBack(TNL_SPECIALCHAR_CB, handleSpecialChars);
      tnlSetCallBack(TNL_ERROR_CB, handleTelNetErrors);
      tnlEnableOption(TELOPT_BAUDRATE);
      tnlEnableOption(TELOPT_PORTSET);
      tnlEnableOption(TELOPT_DEVICE);
      tnlEnableOption(TELOPT_ECHO);

      if (dbg) syslog(LOG_DEBUG, "telnetd():Initial Baudrate of |%s|",
                                 pDevice->baud);
      tnlSetSubOption(TELOPT_BAUDRATE, pDevice->baud);
      if (dbg) syslog(LOG_DEBUG, "telnetd():Initial Port parms of |%s|",
                                 pDevice->port);
      tnlSetSubOption(TELOPT_PORTSET, pDevice->port);
      tnlSetSubOption(TELOPT_DEVICE, pDevice->devc);

      tnlOfferOption(TELOPT_SGA, 1);
      tnlRequestOption(TELOPT_SGA, 1);
      if (dbg) syslog(LOG_DEBUG, "telnetd():Options offered");

      for (;;)
      {
         if (mfp == NULL)
         {
            if (dbg) syslog(LOG_DEBUG, "telnetd():Opening buffered I/O for device");
            if ((mfp = fdopen(mfd, "w")) == NULL)
            {
               syslog(LOG_ERR, "Error opening buffer I/O for device %s:%m",
                               pDevice->devc);
               closeModem(pDevc);
               close(0);
               exit(1);
            }
            if (dbg) syslog(LOG_DEBUG, "telnetd():Done opening buffered I/O for device");
         }

         FD_ZERO(&readFds);
         FD_ZERO(&excpFds);
         FD_SET(fileno(tnlin), &readFds);
         FD_SET(mfd, &readFds);

         if (dbg) syslog(LOG_DEBUG, "telnetd():Calling tnlSelect");
         if ((sflg = tnlSelect(&readFds, NULL, &excpFds, NULL)) >= 0)
         {
            if (dbg > 2) syslog(LOG_DEBUG, "telnetd():Returned from  tnlSelect");
            if (mfd && FD_ISSET(mfd, &readFds))
            {
               if (dbg > 2) syslog(LOG_DEBUG, "telnetd():Have data from Modem");
               if ((cnt = read(mfd, commbuf, sizeof(commbuf) - 1)) > 0)
               {
                  commbuf[cnt] = '\0';
                  if (dbg > 2) syslog(LOG_DEBUG, "telnetd():Read %d bytes from modem: |%s|",
                                              cnt, commbuf);
                  if (PortEnabled) 
                  {
                     if ((cnt = write(fileno(tnlout), commbuf, cnt)) < 0)
                     {
                           syslog(LOG_ERR, "telnetd():Error Writing to network:%m");
                        syslog(LOG_INFO, "telnetd():Shutting down proccess");
                        fclose(mfp);
                        close(mfd);
                        close(0);
                        exit(0);
                     }
                     if (dbg > 2) syslog(LOG_DEBUG, "telnetd():Wrote %d bytes to Network",
                                         cnt);
                  }
                  else
                     if (dbg > 2) syslog(LOG_DEBUG, "telnetd():Chucked %d bytes from modem",
                                         cnt);
               }
               else if (cnt < 0 && errno != EAGAIN && errno != EINTR)
               {
                  syslog(LOG_ERR, "telnetd():Error Reading from modem:%m");
                  mError = 1;
                  mfd = 0;
                  mfp = NULL;
                  exit(1);
               }
            }

            if (FD_ISSET(fileno(tnlin), &readFds))
            {
               if (dbg > 2) syslog(LOG_DEBUG, "telnetd():Have data from Network!");
               if ((cnt = read(fileno(tnlin), commbuf, sizeof(commbuf))) > 0)
               {
                  commbuf[cnt] = '\0';
                  if (dbg) syslog(LOG_DEBUG, "telnetd():Read %d bytes from Network: |%s|",
                                              cnt, commbuf);
                  if (PortEnabled) 
                  {
                     if ((cnt = write(mfd, commbuf, cnt)) < 0)
                     {
                           syslog(LOG_ERR, "telnetd():Error Writing to modem:%m");
                           syslog(LOG_INFO, "Shutting down proccess");
                           closeModem(pDevc);
                           fclose(mfp);
                           pDevc->mfd = -1;
                           close(0);
                           exit(0);
                     }
                     if (dbg) syslog(LOG_DEBUG, "telnetd():Wrote %d bytes to modem",
                                         cnt);
                  }
                  else
                     if (dbg > 2) syslog(LOG_DEBUG, "telnetd():Chucked %d bytes from network",
                                         cnt);
               }
               else
               {
                  if (dbg > 2) syslog(LOG_ERR, "telnetd():Error reading network:%m");
                  if (dbg > 2) syslog(LOG_DEBUG, "telnetd():Error reading network:%m");
                  if (dbg > 2) syslog(LOG_INFO, "telnetd():Shutting down proccess");
                  closeModem(pDevc);
                  fclose(mfp);
                  pDevc->mfd = -1;
                  close(0);
                  exit(0);
               }
            }

            if (FD_ISSET(fileno(tnlin), &excpFds))
            {
               nSynch = 1;
            }
         }
         else
            syslog(LOG_ERR, "telnetd():Quiting!! Error from tnlSelect:%m");
      }
   }
   else
      syslog(LOG_ERR, "telnetd():Error connecting:m");
   closeModem(pDevc);
   fclose(mfp);
   pDevc->mfd = -1;
   close(0);
}     
