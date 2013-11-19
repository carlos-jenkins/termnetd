/***************************************************************************
**    tndConfig.c
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
#include "tndCfgParse.h"
#include "termnetd.h"

static int dbg = 0;


void showConfig()
{
   portConf *pEntry;
   int x;

   for (x = 1, pEntry = pPortList; pEntry != NULL; pEntry = pEntry->pNext, x++)
   {
      if (dbg && NoDetach != 0)
      {
         printf("Entry(0x%lx) %d:\n", (long)pEntry, x);
         printf("\tsock = %d\n", pEntry->sock);
         printf("\tEnbl = %d\n", pEntry->enabled);
         printf("\tpid  = %d\n", pEntry->pid);
         printf("\tmfd  = %d\n", pEntry->mfd);
         printf("\tDevc = %s\n", pEntry->devc);
         printf("\tPort = %s\n", pEntry->port);
         printf("\tBaud = %s\n", pEntry->baud);
         printf("\tSrvc = %s\n", pEntry->srvc);
         printf("\tNext = 0x%lx\n", (long)pEntry->pNext);
      }
   }
}

/*
** int loadConfig(const char *pFile)
** 
** This function loads the configuration file named by pFile and
** builds a list of portConf entries pointed to by pPortList.
**
** This function performs very weak syntax checking!!!
**
** Return Value:
**    0 is returned on success where as -1 is returned on error
*/
int loadConfig(const char *pFile)
{
   FILE *pfd = NULL;
   portConf *pEntry;
   int x, err;

   if (pPortList != NULL)
   {
      for (pEntry = pPortList; pEntry; pEntry = pPortList)
      {
         pPortList = pEntry->pNext;
         free(pEntry);
      }
      pPortList = NULL;
   }

   if ((pfd = fopen(pFile, "r")) != NULL)
   {
      for (x = 1; ; x++)
      {
         if (dbg) syslog(LOG_DEBUG, "loadConfig():Parsing next item");
         if ((pEntry = parseCfgEntry(pfd, &err)) != NULL)
         {
            if (dbg) syslog(LOG_DEBUG, "loadConfig():Adding entry to list");
            if (dbg) syslog(LOG_DEBUG, "loadConfig():Start of list = 0x%x",
                            (unsigned int)pPortList);
            if (dbg) syslog(LOG_DEBUG, "loadConfig():This entry = 0x%x",
                            (unsigned int)pEntry);
            pEntry->pNext = pPortList;
            pPortList = pEntry;
            pEntry->sock = -1;
            pEntry->mfd = 0;
            pEntry->pid = 0;
         }
         else if (err == ECFG_SYNTAX)
            syslog(LOG_WARNING, "Config File syntax error in item %d\n", x);
         else if (err == ECFG_MEMORY)
            syslog(LOG_WARNING, "Out of memory!!!\n");
         else
            break;
      }
      fclose(pfd);
      if (dbg) syslog(LOG_DEBUG, "loadConfig():Parse complete!");
   }
   else
   {
      syslog(LOG_ERR, "Error opening Config file: %s\n", pFile);
      exit(1);
   }
   showConfig();
   return((pfd == NULL) ? -1 : 0);
}

void setPortStr(portConf *pDevice, const char *pPortStr)
{
   if (dbg) syslog(LOG_DEBUG, "setPortStr(%s):Enter", pPortStr);

   strncpy(pDevice->port, pPortStr, PC_MAXFIELD);
   if (strlen(pPortStr) >= PC_MAXFIELD)
      pDevice->port[PC_MAXFIELD -1] = '\0';

   if (dbg) syslog(LOG_DEBUG, "setPortStr():Exit");
}

void setBaudStr(portConf *pDevice, const char *pBaudStr)
{
   if (dbg) syslog(LOG_DEBUG, "setBaudStr(%s):Enter", pBaudStr);

   strncpy(pDevice->baud, pBaudStr, PC_MAXFIELD);
   if (strlen(pBaudStr) >= PC_MAXFIELD)
      pDevice->baud[PC_MAXFIELD -1] = '\0';

   if (dbg) syslog(LOG_DEBUG, "setBaudStr():Exit");
}

portConf *findDevc(const portConf *pPrev, const char *pDevc)
{
   portConf *rv;

   for (rv = (pPrev == NULL) ? pPortList : pPrev->pNext; rv != NULL; 
        rv = rv->pNext)
      if (strcmp(pDevc, rv->devc) == 0)
         break;
   return(rv);
}

portConf *findSock(const portConf *pPrev, const int sock)
{
   portConf *rv;

   if (dbg) syslog(LOG_DEBUG, "findSock():Enter- pPrev = 0x%x", (unsigned int)pPrev);
   if (pPrev == NULL)
   {
      if (dbg) syslog(LOG_DEBUG, "findSock():Starting at the begining");
      rv = pPortList;
   }
   else
   {
      if (dbg) syslog(LOG_DEBUG, "findSock():Starting at next position");
      rv = pPrev->pNext;
   }
   if (dbg) syslog(LOG_DEBUG, "findSock():Starting w/ 0x%x", (unsigned int)rv);
   for (; rv != NULL; rv = rv->pNext)
   {
      if (dbg) syslog(LOG_DEBUG, "findSock():Testing Entry = 0x%x", (unsigned int)rv);
      if (dbg) syslog(LOG_DEBUG, "findSock():Entries are: %d & %d",
                      sock, rv->sock);
      if (sock == rv->sock)
         break;
      if (dbg) syslog(LOG_DEBUG, "findSock():Next Entry = 0x%x", (unsigned int)rv->pNext);
   }
   if (dbg) syslog(LOG_DEBUG, "findSock():Exit(0x%x)", (unsigned int)rv);
   return(rv);
}

portConf *findPortByName(const portConf *pPrev, const char *pSrvc)
{
   portConf *rv;

   for (rv = (pPrev == NULL) ? pPortList : pPrev->pNext; rv != NULL; 
        rv = rv->pNext)
      if (strcmp(pSrvc, rv->srvc) == 0)
         break;
   return(rv);
}
