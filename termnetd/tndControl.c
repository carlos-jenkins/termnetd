/***************************************************************************
**    tndControl.c
**    Termnetd Port Control
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
#include <arpa/inet.h> 
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <setjmp.h>
#include <syslog.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <termios.h>
#include <unistd.h>
#include "termnetd.h"

#define  ERROR_STRING "Error!\n"
#define  PROMPT_STRING "\n->"
#define  ERASE_STRING "\x08 \x08"

static int dbg = 0;

typedef struct ctl_s
{
   int   sock;
   int   mode;
   FILE  *fp;
   char  buf[2048];
   char  *cp;
   struct ctl_s *next; 
} ControlStruct;

typedef struct ctl_c
{
   char  name[32];
   char  help[256];
   void  (*funct)(char*, char*, ControlStruct*);
} CommandStruct;

static void doLine(ControlStruct *pEntry);

static ControlStruct *pList = NULL;

void show_cmd(char *arg1, char *arg2, ControlStruct *pEntry)
{
   portConf *pConf;
   char *cp, tbuf[1024];
   int cnt;
   int devc = 0;

   strcpy(tbuf, arg1);
   for (cp = tbuf; *cp; cp++) 
      *cp = toupper(*cp);
   if (strcmp(tbuf, "DEVICE") == 0)
   {
      strcpy(arg1, arg2);
      devc = 1;
   }
   else if (strcmp(tbuf, "ALL") == 0)
      devc = 2;
   if (strlen(arg1) > 0) 
   {
      if (dbg) syslog(LOG_DEBUG, "show_cmd():Have SHOW command");
      pConf = (devc == 1) ? findDevc(NULL, arg1) :
               (devc == 0) ? findPortByName(NULL, arg1) :
               pPortList;
      for (cnt = 0; pConf != NULL; pConf = (devc == 1) ?
                                       findDevc(pConf, arg1) :
                                       (devc == 2) ?
                                        pConf->pNext :
                                       findPortByName(pConf, arg1)) 
      {
         if (cnt == 0 || (cnt % 20) == 0) 
         {
            fprintf(pEntry->fp, "\n%10s |%30s |%9s |%16s\n", "Port", "Device", "Status", "Connection");
            fputs("-----------|-------------------------------|----------|----------------\n", pEntry->fp);
         }
         cnt++;
         fprintf(pEntry->fp, "%10s |%30s |", pConf->srvc,
                                        pConf->devc);
         fprintf(pEntry->fp, "%9s |", (pConf->enabled == 1) ? "Enabled" :
                                                              "Disabled");
         fprintf(pEntry->fp, "%16s\n", (pConf->pid == 0) ?
                                    "No Connection" : inet_ntoa(pConf->addr.sin_addr));
      }
   }
}

void allow_cmd(char *arg1, char *arg2, ControlStruct *pEntry)
{
   portConf *pConf;
   char *cp, tbuf[1024];
   int cnt;
   int devc = 0;

   strcpy(tbuf, arg1);
   for (cp = tbuf; *cp; cp++) 
      *cp = toupper(*cp);
   if (strcmp(tbuf, "DEVICE") == 0)
   {
      strcpy(arg1, arg2);
      devc = 1;
   }
   if (strlen(arg1) > 0) 
   {
      if (dbg) syslog(LOG_DEBUG, "doLine():Have ALLOW command");
      pConf = (devc == 1) ? findDevc(NULL, arg1) :
                            findPortByName(NULL, arg1);
      for (cnt = 0; pConf != NULL; pConf = (devc == 1) ?
                                       findDevc(pConf, arg1) :
                                       findPortByName(pConf, arg1)) 
      {
            cnt++;
            if (pEntry->mode > 0) 
               fprintf(pEntry->fp, "Allowing connections to %s, device %s\n",
                       arg1, pConf->devc);
            if (dbg) syslog(LOG_DEBUG, "doLine():Allowing |%s|",
                   pConf->devc);
         if (pConf->enabled != 0)
            pConf->enabled = 1;
      }
      if (cnt == 0) 
      {
         fputs("No ports found!\n", pEntry->fp);
      }
   }
}

void disconnect_cmd(char *arg1, char *arg2, ControlStruct *pEntry)
{
   portConf *pConf;
   char *cp, tbuf[1024];
   int cnt;
   int devc = 0;

   strcpy(tbuf, arg1);
   for (cp = tbuf; *cp; cp++) 
      *cp = toupper(*cp);
   if (strcmp(tbuf, "DEVICE") == 0)
   {
      strcpy(arg1, arg2);
      devc = 1;
   }
   if (strlen(arg1) > 0) 
   {
      if (dbg) syslog(LOG_DEBUG, "doLine():Have DENY command");
      pConf = (devc == 1) ? findDevc(NULL, arg1) :
                            findPortByName(NULL, arg1);
      for (cnt = 0; pConf != NULL; pConf = (devc == 1) ?
                                       findDevc(pConf, arg1) :
                                       findPortByName(pConf, arg1)) 
      {
         if (pConf->pid != 0) 
         {
            cnt++;
            fprintf(pEntry->fp, "Disconnecting socket %s, device %s\n",
                    arg1, pConf->devc);
            if (dbg) syslog(LOG_DEBUG, "doLine():Disconnecting |%s|",
                   pConf->devc);
            kill(pConf->pid, SIGINT);
         }
      }
      if (cnt == 0) 
      {
         fputs("No ports found!\n", pEntry->fp);
      }
   }
}

void deny_cmd(char *arg1, char *arg2, ControlStruct *pEntry)
{
   portConf *pConf;
   char *cp, tbuf[1024];
   int cnt;
   int devc = 0;

   strcpy(tbuf, arg1);
   for (cp = tbuf; *cp; cp++) 
      *cp = toupper(*cp);
   if (strcmp(tbuf, "DEVICE") == 0)
   {
      strcpy(arg1, arg2);
      devc = 1;
   }
   if (strlen(arg1) > 0) 
   {
      if (dbg) syslog(LOG_DEBUG, "doLine():Have DENY command");
      pConf = (devc == 1) ? findDevc(NULL, arg1) :
                            findPortByName(NULL, arg1);
      for (cnt = 0; pConf != NULL; pConf = (devc == 1) ?
                                       findDevc(pConf, arg1) :
                                       findPortByName(pConf, arg1)) 
      {
         if (pConf->enabled != 0)
         {
            cnt++;
            if (pEntry->mode > 0) 
               fprintf(pEntry->fp, "Denying connections to %s, device %s\n",
                       arg1, pConf->devc);
            if (dbg) syslog(LOG_DEBUG, "doLine():Denying |%s|",
                   pConf->devc);
            if (pConf->pid != 0) 
               kill(pConf->pid, SIGINT);
            pConf->enabled = -1;
         }
      }
      if (cnt == 0) 
      {
         fputs("No ports found!\n", pEntry->fp);
      }
   }
}

void enable_cmd(char *arg1, char *arg2, ControlStruct *pEntry)
{
   portConf *pConf;
   char *cp, tbuf[1024];
   int cnt;
   int devc = 0;

   strcpy(tbuf, arg1);
   for (cp = tbuf; *cp; cp++) 
      *cp = toupper(*cp);
   if (strcmp(tbuf, "DEVICE") == 0)
   {
      strcpy(arg1, arg2);
      devc = 1;
   }
   if (strlen(arg1) > 0) 
   {
      if (dbg) syslog(LOG_DEBUG, "doLine():Have ENABLE command");
      pConf = (devc == 1) ? findDevc(NULL, arg1) :
                            findPortByName(NULL, arg1);
      for (cnt = 0; pConf != NULL; pConf = (devc == 1) ?
                                       findDevc(pConf, arg1) :
                                       findPortByName(pConf, arg1)) 
      {
         if (pConf->pid != 0) 
         {
            cnt++;
            if (pEntry->mode > 0) 
               fprintf(pEntry->fp, "Enabling traffic for %s, device %s\n",
                       arg1, pConf->devc);
            if (dbg) syslog(LOG_DEBUG, "doLine():Enabling |%s|",
                   pConf->devc);
            kill(pConf->pid, SIGUSR2);
         }
      }
      if (cnt == 0) 
      {
         fputs("No ports found!\n", pEntry->fp);
      }
   }
}

void disable_cmd(char *arg1, char *arg2, ControlStruct *pEntry)
{
   portConf *pConf;
   char *cp, tbuf[1024];
   int cnt;
   int devc = 0;

   strcpy(tbuf, arg1);
   for (cp = tbuf; *cp; cp++) 
      *cp = toupper(*cp);
   if (strcmp(tbuf, "DEVICE") == 0)
   {
      strcpy(arg1, arg2);
      devc = 1;
   }
   if (strlen(arg1) > 0) 
   {
      if (dbg) syslog(LOG_DEBUG, "doLine():Have DISABLE command");
      pConf = (devc == 1) ? findDevc(NULL, arg1) :
                            findPortByName(NULL, arg1);
      for (cnt = 0; pConf != NULL; pConf = (devc == 1) ?
                                       findDevc(pConf, arg1) :
                                       findPortByName(pConf, arg1)) 
      {
         if (pConf->pid != 0) 
         {
            cnt++;
            if (pEntry->mode > 0) 
               fprintf(pEntry->fp, "Disabling traffic for %s, device %s\n",
                       arg1, pConf->devc);
            if (dbg) syslog(LOG_DEBUG, "doLine():Disabling |%s|",
                   pConf->devc);
            kill(pConf->pid, SIGUSR1);
         }
      }
      if (cnt == 0) 
      {
         fputs("No ports found!\n", pEntry->fp);
      }
   }
}

void prompt_cmd(char *arg1, char *arg2, ControlStruct *pEntry)
{
   if ((pEntry->mode = (pEntry->mode != 0) ? 0 : 1) != 0)
      fputs(PROMPT_STRING, pEntry->fp);
}

void help_cmd(char *arg1, char *arg2, ControlStruct *pEntry);

CommandStruct cmdTable[] =
{
   { "HELP", "help\n\tShow this message", help_cmd },
   { "SHOW", "show all|<port name>\n\tShow port status", show_cmd },
   { "ALLOW", "allow [device] <port name>|<device name>\n\tAllow port to accept connections", allow_cmd },
   { "DENY", "deny  [device] <port name>|<device name>\n\tDeny a port's ability to accept connections", deny_cmd },
   { "ENABLE", "enable  [device] <port name>|<device name>\n\tEnable data flow to the port (Only valid for connected ports)", enable_cmd },
   { "DISABLE", "disable  [device] <port name>|<device name>\n\tDisable data flow to the port (Only valid for connected ports)", disable_cmd },
   { "PROMPT", "prompt\n\tToggle the display of the prompt", prompt_cmd },
   { "", "", NULL }
};

void help_cmd(char *arg1, char *arg2, ControlStruct *pEntry)
{
   int x;

   fputs("\n", pEntry->fp);
   for (x = 0; cmdTable[x].funct != NULL; x++)
   {
      fputs(cmdTable[x].help, pEntry->fp);
      fputs("\n\n", pEntry->fp);
   }
}

int acceptControl()
{
   struct sockaddr_in addr;
   ControlStruct *pEntry;
   int x = sizeof(addr);

   if ((pEntry = (ControlStruct*)malloc(sizeof(ControlStruct))) == NULL)
   {
      syslog(LOG_ERR, "Out of memory for control port structure!");
      close(ControlSock);
      return(-1);
   }

   pEntry->next = pList;
   pList = pEntry;
   pList->cp = pList->buf;
   pEntry->mode = 0;

   if ((pEntry->sock = accept(ControlSock, (struct sockaddr *)&addr, 
        &x)) < 0)
   {
      pList = pEntry->next;
      free(pEntry);
      syslog(LOG_ERR, "Error Accepting Control connection on port %d",
                      ControlAddr.sin_port);
      return(-1);
   }
   x = fcntl(pEntry->sock, F_GETFL, 0);
   fcntl(pEntry->sock, F_SETFL, x | O_NONBLOCK);
   pEntry->fp = fdopen(pEntry->sock, "w");
   if (dbg) syslog(LOG_DEBUG, "acceptControl():Control port opened!");
   return(pEntry->sock);
}

int readControl(int sock)
{
   int bc, rv = 1;
   char *cp, tbuf[256];
   ControlStruct *pEntry;

   if (dbg) syslog(LOG_DEBUG, "readControl():Enter");
   for (pEntry = pList; pEntry != NULL; pEntry = pEntry->next) 
   {
      if (pEntry->sock == sock) 
      {
         if (dbg) syslog(LOG_DEBUG, "readControl():Socket found in list!");
         break;
      }
   }
   if (pEntry != NULL)
   { 
      if (dbg) syslog(LOG_DEBUG, "readControl():Have Control port data!");

      if ((bc = read(sock, tbuf, sizeof(tbuf))) > 0) 
      {
         if (dbg) syslog(LOG_DEBUG, "readControl():read %d bytes", bc);
         for (cp = tbuf; bc > 0; bc--, cp++) 
         {
            if (dbg > 2) syslog(LOG_DEBUG, "readControl():have char %2x:|%c|", *cp, *cp);
            if (*cp != '\r' || *cp != '\n') 
               *(pEntry->cp++) = *cp;
            *pEntry->cp = '\0';

            if (*cp == '\r' || *cp == '\n' || 
                strlen(pEntry->buf) >= sizeof(pEntry->buf) - 5) 
            {
               doLine(pEntry);
               pEntry->cp = pEntry->buf;
            }
         }
      }
      else
         rv = -1;
   }
   else
      rv = 0;
   if (dbg) syslog(LOG_DEBUG, "readControl(%d):Exit", rv);
   return(rv);
}

static void doLine(ControlStruct *pEntry)
{
   char command[256], arg1[1024], arg2[1024];
   char *cp;
   int x;

   strcpy(arg1, "");
   strcpy(arg2, "");
   if (dbg) syslog(LOG_DEBUG, "doLine():Have command |%s|", pEntry->buf);
   if ((cp = strtok(pEntry->buf, " \t\r\n")) != NULL)
   {
      strcpy(command, cp);
      if ((cp = strtok(NULL, " \t\r\n")) != NULL)
      {
         strcpy(arg1, cp);
         if ((cp = strtok(NULL, " \t\r\n")) != NULL)
            strcpy(arg2, cp);
      }
      if (dbg) syslog(LOG_DEBUG, "doLine():arg1 |%s|",
             arg1);
      if (dbg) syslog(LOG_DEBUG, "doLine():arg1 |%s|",
             arg2);
      for (cp = command; *cp; cp++) 
         *cp = toupper(*cp);

      if (dbg) syslog(LOG_DEBUG, "doLine():Command converted to |%s|", command);
      for (x = 0; cmdTable[x].funct != NULL; x++) 
      {
         if (dbg) syslog(LOG_DEBUG, "doLine():Testing against: |%s|", cmdTable[x].name);
         if (strcmp(command, cmdTable[x].name) == 0) 
         {
            if (dbg) syslog(LOG_DEBUG, "doLine():Have match!");
            (cmdTable[x].funct)(arg1, arg2, pEntry);
            break;
         }
      }
      if (cmdTable[x].funct == NULL && pEntry->mode != 0) 
      {
         fputs("No such command... Try \"help\"\n", pEntry->fp);
      }
   }
   if (pEntry->mode > 0)
      fputs(PROMPT_STRING, pEntry->fp);
   fflush(pEntry->fp);
}

