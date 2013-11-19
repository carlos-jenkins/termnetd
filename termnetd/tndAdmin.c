/***************************************************************************
**    tndAdmin.c
**    Termnetd Administration Functions
**
** Copyright (C) 1996      Joseph Croft <jcroft@unicomp.net>  
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
#include <../sysdefs.h>
#include <fcntl.h>
#ifdef FreeBSD
#include <sys/ioctl.h>
#endif
#include <errno.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <netinet/in.h>
#include <pwd.h>
#include <signal.h>
#include <syslog.h>
#include <termios.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <regex.h>
#include "termnetd.h"

static int dbg = 0;

extern char PrivilegedUser[];

typedef struct ctl_s
{
   int   sock;
   int   verbose;
   FILE  *fp;
   char  buf[2048];
   char  *cp;
   regex_t portRE;
   regex_t devcRE;
   int   passwdMode;
   int   privileged;
   struct termios tios;
   struct ctl_s *next; 
} ControlStruct;

int baudList[] = {
   0, 50, 75, 110, 134, 150, 200, 300, 600, 1200, 1800,
   2400, 4800, 9600, 19200, 38400, 57600, 115200, 230400
};

static char ttydev_x[] = "edcbazyxwvutsrqp";
static char ttydev_y[] = "fedcba9876543210";
static char ttydev_z[] = "/dev/pty$$";

static ControlStruct *pList = NULL;

void doAdmin(int sock, ControlStruct *p);
void doHelp(ControlStruct *p, char *pStr);
void doEnable(ControlStruct *p, char *pStr);
void doDisable(ControlStruct *p, char *pStr);
void doDisconnect(ControlStruct *p, char *pStr);
void doPrivilege(ControlStruct *p, char *pStr, int sock);
void doShow(ControlStruct *p, char *pStr);
void doSet(ControlStruct *p, char *pStr);
char *parseRE(ControlStruct *p, const char *pStr);


int openPtyDevice(char *pName)
{
   int x, fd = -1;
   char *px, *py;
   struct stat statbuf;

   if (pName == NULL || strlen(pName) == 0) 
   {
      for (px = ttydev_x; *px != '\0'; px++) 
      {
         for (py = ttydev_y; *py != '\0'; py++) 
         {
            ttydev_z[8] = *px;
            ttydev_z[9] = *py;
            if (dbg) syslog(LOG_DEBUG, "openPtyDevice():Attempting |%s|", ttydev_z);
            if ((x = stat(ttydev_z, &statbuf)) == 0 && S_ISCHR(statbuf.st_mode))
            {
               if (dbg) syslog(LOG_DEBUG, "openPtyDevice():Device exists");
               if ((fd = open(ttydev_z, O_RDWR)) >= 0)
               {
                  strcpy(pName, ttydev_z);
                  break;
               }
               else if (dbg)
               {
                  syslog(LOG_DEBUG, "Error opening device:%m");
               }
            }
            else 
            {
               if (dbg) syslog(LOG_DEBUG, "openPtyDevice():x = %d", x);
               if (x != 0) 
               {
                  if (dbg) syslog(LOG_DEBUG, "openPtyDevice():Device does not exists");
               }
               else
                  if (dbg) syslog(LOG_DEBUG, "openPtyDevice():Device type 0x%x, is wrong",
                                          statbuf.st_mode);
            }
         }
      }
   }
   else
   {
      if (dbg) syslog(LOG_DEBUG, "openPtyDevice():Attempting |%s|", pName);
      if ((x = stat(pName, &statbuf)) == 0 && S_ISCHR(statbuf.st_mode)) 
      {
         if (dbg) syslog(LOG_DEBUG, "openPtyDevice():Device exists");
         fd = open(pName, O_RDWR);
      }
      else 
      {
         if (x != 0) 
         {
            if (dbg) syslog(LOG_DEBUG, "openPtyDevice():Device does not exists");
         }
         else
            if (dbg) syslog(LOG_DEBUG, "openPtyDevice():Device type 0x%x, is wrong",
                                    statbuf.st_mode);
      }
   }
   return(fd);
}


int acceptControl()
{
   struct sockaddr_in addr;
   ControlStruct *pEntry;
#ifdef SCO
   int x;
#else
   size_t x;
#endif

   x = sizeof(addr);
   if ((pEntry = (ControlStruct*)malloc(sizeof(ControlStruct))) == NULL)
   {
      syslog(LOG_ERR, "Out of memory for control port structure!");
      close(ControlSock);
      return(-1);
   }

   pEntry->next = pList;
   pList = pEntry;
   pList->cp = pList->buf;
   pEntry->verbose = 0;
   Permiscuous = 1;

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
   if (dbg) syslog(LOG_DEBUG, "acceptControl():Control port opened! fp = %d",
                              (int)pEntry->fp);
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
            if (*cp == '\x08' || *cp == '\x7f')
            {
               if (pEntry->cp != pEntry->buf)
               {
                  *pEntry->cp = '\x0';
                  pEntry->cp--;
               }
               if (pEntry->verbose != 0)
                  fputs("\x08 \x08", pEntry->fp);
            }
            else if (*cp != '\r' && *cp != '\n') 
               *(pEntry->cp++) = *cp;
            *pEntry->cp = '\0';

            if (*cp == '\r' || *cp == '\n' || 
                strlen(pEntry->buf) >= sizeof(pEntry->buf) - 5) 
            {
               if (strlen(pEntry->buf) > 2) 
               {
                  if (dbg) syslog(LOG_DEBUG, "readControl():Have complete line |%s|", pEntry->buf);
                  doAdmin(sock, pEntry);
                  pEntry->cp = pEntry->buf;
               }
               else
               {
                  memset(pEntry->buf, '\0', sizeof(pEntry->buf));
                  pEntry->cp = pEntry->buf;
               }
               if (pEntry->verbose) 
               {
                  fputs("\n->", pEntry->fp);
                  fflush(pEntry->fp);
               }
               memset(pEntry->buf, 0, sizeof(pEntry->buf));
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

/*
** doAdmin(ControlStruct *p)
**
** This function builds the input string from the user via the port
** specified in the configuration file w/ the device named admin to
** handle basic port enabling/disabling and connection status.
**
** Each command must be terminated w/ a '\r' character.
**
** The output of all commands will be through the buffered file i/o
** routines fputs and fputc. Therefore, and connections will be blocked
** until the current command in process completes.
**
** The name expressions are regular expressions to match against the 
** network port field if "port" is specified or the devices physical
** device name or it's symbolic name if "device" is specified.
**
** * Note:
**    Regular expression parsing is only done on operating systems with 
**    the functions regcomp and regexec available. If they are not available
**    both the port and the device name must be given explicitely or
**    substituted w/ the name 'ALL'. In the latter case, all occurances
**    will test true.
**
** The following commands are handled:
**
**    enable {port | device} <name exp>
**       Enable the specified port(s)/device(s)
**
**    disable {port | device} <name exp>
**       Disable the specified port(s)/device(s)
**
**    disconnect {port | device} <name exp>
**       Disconnect the specified port(s)/device(s)
**
**    privilege <password>
**       Enable the privileged mode
**
**    set baud {port | device} <name exp> <baudrate>
**       Set the default Baudrate of the specified port(s)/device(s)
**
**    set conf [port <name exp>] [device <name exp>] <port configuration>
**       Set the default Port Settings of the specified port(s)/device(s)
**
**    show [port <name exp>] [device <name exp>] <name exp>
**       Show the configuration and status of the specified port(s)/device(s)
**
** The following are the responses that will be returned.
**
**    100:<port>:<device>(<symb>)
**       Device enabled
**    101:<port>:<device>(<symb>)
**       Device disabled
**    102:<port>:<device>(<symb>)
**       Device disconnected
**    103:<port>:<device>(<symb>):<new port settings>
**       New default port settings for a given device
**    104:<port>:<device>(<symb>):<new baudrate>
**       New default baudrate for a given device
**    105:<port>:<device>(<symb>):<port settings>:<baudrate>:\
**       :[<ip of connected user>]
**    106
**       Password excepted
**    199
**       End of list
**
**    900:Password rejected
**    901:Bad regular expression given for port
**    902:Bad regular expression given for device
**    903:Syntax error in command
**    904:Command not recognized
**
*/


void doAdmin(int sock, ControlStruct *p)
{
   char *cp, *cp1, tmpBuf[20];
   int x;

   if (dbg) syslog(LOG_DEBUG, "doAdmin():Enter |%s|", p->buf);
   if (p->passwdMode != 0) 
   {
      doPrivilege(p, "", sock);
   }
   else
   {
      for (cp = p->buf; *cp != '\0' && isspace(*cp); cp++);
      memset(tmpBuf, '\0', sizeof(tmpBuf));
      for (x = 0, cp1 = tmpBuf; *cp != '\0' && !isspace(*cp) && (unsigned)x < sizeof(tmpBuf); x++)
      {
         *(cp1++) = tolower(*cp++);
         *cp1 = '\0';
      }
      if (dbg) syslog(LOG_DEBUG, "doAdmin():Command = |%s|", tmpBuf);
      for (; *cp != '\0' && isspace(*cp); cp++);

      if (dbg) syslog(LOG_DEBUG, "doAdmin():Remainder Command = |%s|", cp);

      if (strcmp(tmpBuf, "enable") == 0)
         doEnable(p, cp);

      else if (strcmp(tmpBuf, "disable") == 0)
         doDisable(p, cp);

      else if (strcmp(tmpBuf, "disconnect") == 0)
         doDisconnect(p, cp);

//      else if (strcmp(tmpBuf, "privilege") == 0)
//         doPrivilege(p, cp, sock);

      else if (strcmp(tmpBuf, "set") == 0)
         doSet(p, cp);

      else if (strcmp(tmpBuf, "help") == 0)
         doHelp(p, cp);

      else if (strcmp(tmpBuf, "show") == 0)
         doShow(p, cp);

      else if (strcmp(tmpBuf, "verbose") == 0)
         p->verbose = (p->verbose == 0) ? 1 : 0;

      else 
      {
         if (p->verbose)
            fputs("Error, Command Not recognized\n", p->fp);
         else
            fputs("904:Command Not recognized\n", p->fp);
      }
   }
   fflush(p->fp);
}

void doHelp(ControlStruct *p, char *pStr)
{
   fputs("The following commands sre available:\n", p->fp);
   fputs("\tverbose\n", p->fp);
   fputs("\t\tMake command interface more user friendly\n", p->fp);

   fputs("\thelp\n", p->fp);
   fputs("\t\tShow this table\n", p->fp);

   fputs("\tenable {port | device} <name expression>\n", p->fp);
   fputs("\t\tEnable connections to the ports specified\n", p->fp);

   fputs("\tdisable {port | device} <name expression>\n", p->fp);
   fputs("\t\tDisable connections to the ports specified\n", p->fp);

   fputs("\tdisconnect {port | device} <name expression>\n", p->fp);
   fputs("\t\tDisconnect any active connections to the ports specified\n", p->fp);

   fputs("\tset baud {port | device} <name expression>\n", p->fp);
   fputs("\t\tSet the default baud rate of the ports specified\n", p->fp);

   fputs("\tset conf {port | device} <name expression>\n", p->fp);
   fputs("\t\tSet the default port configurations to the ports specified\n", p->fp);

   fputs("\tshow {port | device} <name expression> [status]\n", p->fp);
   fputs("\t\tShow the status or settings of the ports specified\n", p->fp);

}

void doPrivilege(ControlStruct *p, char *pStr, int sock)
{
/*
   if (p->passwdMode == 0) 
   {
      p->passwdMode = 1;
      fputs("\nPassword: ", p->fp);
   }
   else 
   {
      struct passwd *pw;
      char passwd[14], salt[14], *cp;


      if (dbg) syslog(LOG_DEBUG, "doPrivilege():User = |%s|", PrivilegedUser);
      pw = getpwnam(PrivilegedUser);
      strcpy(passwd, pw->pw_passwd);
      if (strcmp(passwd, "x") == 0) 
      {
         FILE *pfp;
         if ((pfp = fopen("/etc/shadow", "r")) != NULL)
         {
            while ((pw = fgetpwent(pfp)) != NULL)
            {
               if (strcmp(pw->pw_name, PrivilegedUser) == 0) 
               {
                  strcpy(passwd, pw->pw_passwd);
                  break;
               }
            }
            fclose(pfp);
         }
         else
         {
            fprintf(p->fp, "Cannot test passwd:%s\n", strerror(errno));
            return;
         }
      }
      strcpy(salt, passwd);
      if (dbg) syslog(LOG_DEBUG, "doPrivilege():Passwd = |%s|", salt);
      salt[2] = '\0';
      if (dbg) syslog(LOG_DEBUG, "doPrivilege():Salt = |%s|", salt);
      cp = crypt(p->buf, salt);
      if (dbg) syslog(LOG_DEBUG, "doPrivilege():buf = |%s|, Encrypted = |%s|",
                                 p->buf, cp);
      if (strcmp(cp, passwd) == 0)
      {
         p->privileged = 1;
         if (p->verbose) 
            fputs("Password Accepted\n", p->fp);
         else
            fputs("106:Password Accepted\n", p->fp);
      }
      else
      {
         if (p->verbose) 
            fputs("Invalid password\n", p->fp);
         else
            fputs("900:Password rejected\n", p->fp);
      }
      p->passwdMode = 0;
   }
*/
   p->privileged = 1;
}

void doEnable(ControlStruct *p, char *pStr)
{
   char *cp;
   portConf *pDevc;

   if ((!Permiscuous && !p->privileged) || (cp = parseRE(p, pStr)) == NULL)
      return;

   for (pDevc = pPortList; pDevc != NULL; pDevc = pDevc->pNext)
   {
      if (regexec(&p->portRE, pDevc->srvc, 0, NULL, 0) == 0 &&
          (regexec(&p->devcRE, pDevc->dnam, 0, NULL, 0) == 0 ||
           regexec(&p->devcRE, pDevc->devc, 0, NULL, 0) == 0))
      {
         if (p->verbose) 
            fprintf(p->fp, "100:%s:%s:%s:Device Enabled\n",
                             pDevc->srvc,
                             pDevc->dnam,
                             pDevc->devc);
         else
            fprintf(p->fp, "100:%s:%s:%s:Device Enabled\n",
                             pDevc->srvc,
                             pDevc->dnam,
                             pDevc->devc);
         pDevc->enabled = 1;
      }
   }
}

void doDisable(ControlStruct *p, char *pStr)
{
   char *cp;
   portConf *pDevc;

   if ((!Permiscuous && !p->privileged) || (cp = parseRE(p, pStr)) == NULL)
      return;

   for (pDevc = pPortList; pDevc != NULL; pDevc = pDevc->pNext)
   {
      if (regexec(&p->portRE, pDevc->srvc, 0, NULL, 0) == 0 &&
          (regexec(&p->devcRE, pDevc->dnam, 0, NULL, 0) == 0 ||
           regexec(&p->devcRE, pDevc->devc, 0, NULL, 0) == 0))
      {
         if (p->verbose) 
            fprintf(p->fp, "Disabling port: %s:%s(%s)\n",
                             pDevc->srvc,
                             pDevc->dnam,
                             pDevc->devc);
         else
            fprintf(p->fp, "101:%s:%s:%s:Device disabled\n",
                             pDevc->srvc,
                             pDevc->dnam,
                             pDevc->devc);
         pDevc->enabled = 0;
      }
   }
}

void doDisconnect(ControlStruct *p, char *pStr)
{
   int cnt;
   char *cp;
   portConf *pDevc;

   if ((!Permiscuous && !p->privileged) || (cp = parseRE(p, pStr)) == NULL)
      return;

   for (cnt = 0, pDevc = pPortList; pDevc != NULL; pDevc = pDevc->pNext)
   {
      if (pDevc->pid > 0 &&
          pDevc->enabled &&
          regexec(&p->portRE, pDevc->srvc, 0, NULL, 0) == 0 &&
          (regexec(&p->devcRE, pDevc->dnam, 0, NULL, 0) == 0 ||
           regexec(&p->devcRE, pDevc->devc, 0, NULL, 0) == 0))
      {
         cnt++;
         if (p->verbose) 
            fprintf(p->fp, "Disconnecting port: %s:%s(%s)\n",
                             pDevc->srvc,
                             pDevc->dnam,
                             pDevc->devc);
         else
            fprintf(p->fp, "102:%s:%s:%s:Port disconnected\n",
                             pDevc->srvc,
                             pDevc->dnam,
                             pDevc->devc);
         if (dbg) syslog(LOG_DEBUG, "doPrivilege():Sending TERM signal to process %d", 
                                    pDevc->pid);
         kill(pDevc->pid, SIGTERM);
      }
   }
   if (cnt == 0)
      fprintf(p->fp, "No port found\n");
}

void strPut(char *buf, int buflen, const char *p)
{
   if ((strlen(buf) + strlen(p)) < (unsigned)(buflen - 3))
   {
      if (strlen(buf) > 0) 
      {
         if ((strlen(buf) + strlen(p)) > 75)
            strcat(buf, ",\n");
         else
            strcat(buf, ", ");
      }
      strcat(buf, p);
   }
}

void doShow(ControlStruct *p, char *pStr)
{
   char *cp, *cp1, tmpBuf[2048];
   portConf *pDevc;
   int status = 0, x, flag, mask;


   if (dbg) syslog(LOG_DEBUG, "doShow():Enter");
   if ((cp = parseRE(p, pStr)) == NULL)
      return;

   for (; *cp && isspace(*cp); cp++);
   memset(tmpBuf, 0, sizeof(tmpBuf));
   for (x = 0, cp1 = tmpBuf; *cp != '\0' && !isspace(*cp) && x < 20; x++)
   {
      *(cp1++) = tolower(*cp++);
      *cp1 = '\0';
   }
   if (strcmp(tmpBuf, "status") != 0)
      status = 0;
   else
      status = 1;

   if (dbg) syslog(LOG_DEBUG, "doShow():%sShowing Status",
                     (status == 0) ? "Not " : "");
   for (x = 0, pDevc = pPortList; pDevc != NULL; pDevc = pDevc->pNext)
   {
      if (dbg) syslog(LOG_DEBUG, "doShow():Testing port |%s|, devc |%s|",
                      pDevc->srvc, pDevc->devc);
      if (regexec(&p->portRE, pDevc->srvc, 0, NULL, 0) == 0)
      {
         if (dbg) syslog(LOG_DEBUG, "doShow():Port matched!");
      }
      if (regexec(&p->devcRE, pDevc->devc, 0, NULL, 0) == 0)
      {
         if (dbg) syslog(LOG_DEBUG, "doShow():Devc matched!");
      }
      if (regexec(&p->portRE, pDevc->srvc, 0, NULL, 0) == 0 &&
          (regexec(&p->devcRE, pDevc->dnam, 0, NULL, 0) == 0 ||
           regexec(&p->devcRE, pDevc->devc, 0, NULL, 0) == 0))
      {
         if (dbg) syslog(LOG_DEBUG, "doShow():Have match!");
         if (status == 0) 
         {
            if (dbg) syslog(LOG_DEBUG, "doShow():Here!");
            fprintf(p->fp, "\nName:\t'%s'\tPort:\t'%s'\tDevice:\t'%s'\n", 
                    pDevc->dnam, pDevc->srvc, pDevc->devc);
            fprintf(p->fp, "Baud:\t%d\n", baudList[pDevc->speed]);
            memset(tmpBuf, '\0', sizeof(tmpBuf));
            flag = pDevc->tios.c_iflag;
            if (flag & IGNBRK) 
               strPut(tmpBuf, sizeof(tmpBuf), "IGNBRK");
            if (flag & BRKINT) 
               strPut(tmpBuf, sizeof(tmpBuf), "BRKINT");
            if (flag & PARMRK) 
               strPut(tmpBuf, sizeof(tmpBuf), "PARMRK");
            if (flag & INPCK) 
               strPut(tmpBuf, sizeof(tmpBuf), "INPCK");
            if (flag & ISTRIP) 
               strPut(tmpBuf, sizeof(tmpBuf), "ISTRIP");
            if (flag & INLCR) 
               strPut(tmpBuf, sizeof(tmpBuf), "INLCR");
            if (flag & IGNCR) 
               strPut(tmpBuf, sizeof(tmpBuf), "IGNCR");
            if (flag & ICRNL) 
               strPut(tmpBuf, sizeof(tmpBuf), "ICRNL");
#ifndef FreeBSD
            if (flag & IUCLC) 
               strPut(tmpBuf, sizeof(tmpBuf), "IUCLC");
#endif /* FreeBSD */
            if (flag & IXON) 
               strPut(tmpBuf, sizeof(tmpBuf), "IXON");
            if (flag & IXANY) 
               strPut(tmpBuf, sizeof(tmpBuf), "IXANY");
            if (flag & IXOFF) 
               strPut(tmpBuf, sizeof(tmpBuf), "IMAXBEL");
            if (strlen(tmpBuf))
            {
               fputs("Input flags:\n", p->fp);
               strcat(tmpBuf, "\n");
               fputs(tmpBuf, p->fp);
            }

            memset(tmpBuf, '\0', sizeof(tmpBuf));
            flag = pDevc->tios.c_oflag;
            if (flag & OPOST) 
               strPut(tmpBuf, sizeof(tmpBuf), "OPOST");
            if (flag & ONLCR) 
               strPut(tmpBuf, sizeof(tmpBuf), "ONLCR");

#ifndef FreeBSD
            if (flag & OLCUC) 
               strPut(tmpBuf, sizeof(tmpBuf), "OLCUC");
            if (flag & OCRNL) 
               strPut(tmpBuf, sizeof(tmpBuf), "OCRNL");
            if (flag & ONOCR) 
               strPut(tmpBuf, sizeof(tmpBuf), "ONOCR");
            if (flag & ONLRET) 
               strPut(tmpBuf, sizeof(tmpBuf), "ONLRET");
            if (flag & OFDEL) 
               strPut(tmpBuf, sizeof(tmpBuf), "OFDEL");

            mask = flag & NLDLY;
            if (mask == NL0) 
               strPut(tmpBuf, sizeof(tmpBuf), "NL0");
            else if (mask == NL1)
               strPut(tmpBuf, sizeof(tmpBuf), "NL1");

            mask = flag & CRDLY;
            if (mask == CR0) 
               strPut(tmpBuf, sizeof(tmpBuf), "CR0");
            else if (mask == CR1)
               strPut(tmpBuf, sizeof(tmpBuf), "CR1");
            else if (mask == CR2)
               strPut(tmpBuf, sizeof(tmpBuf), "CR2");
            else if (mask == CR3)
               strPut(tmpBuf, sizeof(tmpBuf), "CR3");

            mask = flag & TABDLY;
            if (mask == TAB0) 
               strPut(tmpBuf, sizeof(tmpBuf), "TAB0");
            else if (mask == TAB1)
               strPut(tmpBuf, sizeof(tmpBuf), "TAB1");
            else if (mask == TAB2)
               strPut(tmpBuf, sizeof(tmpBuf), "TAB2");
            else if (mask == TAB3)
               strPut(tmpBuf, sizeof(tmpBuf), "TAB3");
#if !defined(OSF) && !defined(AIX) && !defined(SCO)
            else if (mask == XTABS)
               strPut(tmpBuf, sizeof(tmpBuf), "XTABS");
#endif

            mask = flag & BSDLY;
            if (mask == BS0) 
               strPut(tmpBuf, sizeof(tmpBuf), "BS0");
            else if (mask == BS1)
               strPut(tmpBuf, sizeof(tmpBuf), "BS1");

            mask = flag & VTDLY;
            if (mask == VT0) 
               strPut(tmpBuf, sizeof(tmpBuf), "VT0");
            else if (mask == VT1)
               strPut(tmpBuf, sizeof(tmpBuf), "VT1");

            mask = flag & FFDLY;
            if (mask == FF0) 
               strPut(tmpBuf, sizeof(tmpBuf), "FF0");
            else if (mask == FF1)
               strPut(tmpBuf, sizeof(tmpBuf), "FF1");
            if (strlen(tmpBuf))
            {
               fputs("Output flags:\n", p->fp);
               strcat(tmpBuf, "\n");
               fputs(tmpBuf, p->fp);
            }
#endif /* FreeBSD */

            memset(tmpBuf, '\0', sizeof(tmpBuf));
            flag = pDevc->tios.c_cflag;
            mask = flag & CSIZE;
            if (mask == CS5) 
               strPut(tmpBuf, sizeof(tmpBuf), "CS5");
            else if (mask == CS6)
               strPut(tmpBuf, sizeof(tmpBuf), "CS6");
            else if (mask == CS7)
               strPut(tmpBuf, sizeof(tmpBuf), "CS7");
            else if (mask == CS8)
               strPut(tmpBuf, sizeof(tmpBuf), "CS8");

            if (flag & CSTOPB) 
               strPut(tmpBuf, sizeof(tmpBuf), "CSTOPB");
            if (flag & PARENB) 
            {
               strPut(tmpBuf, sizeof(tmpBuf), "PARENB");
               if (flag & PARODD) 
                  strPut(tmpBuf, sizeof(tmpBuf), "PARODD");
            }
            if (flag & HUPCL) 
               strPut(tmpBuf, sizeof(tmpBuf), "HUPCL");
            if (flag & CLOCAL) 
               strPut(tmpBuf, sizeof(tmpBuf), "CLOCAL");
#if !defined(AIX) && !defined(SCO)
            if (flag & CRTSCTS) 
               strPut(tmpBuf, sizeof(tmpBuf), "CRTSCTS");
#endif
            if (strlen(tmpBuf))
            {
               fputs("Control flags:\n", p->fp);
               strcat(tmpBuf, "\n");
               fputs(tmpBuf, p->fp);
            }

            memset(tmpBuf, '\0', sizeof(tmpBuf));
            flag = pDevc->tios.c_lflag;
            if (flag & ISIG)
               strPut(tmpBuf, sizeof(tmpBuf), "ISIG");
            if (flag & ICANON)
               strPut(tmpBuf, sizeof(tmpBuf), "ICANON");
#ifndef FreeBSD
            if (flag & XCASE)
               strPut(tmpBuf, sizeof(tmpBuf), "XCASE");
#endif /* FreeBSD */
            if (flag & ECHO)
               strPut(tmpBuf, sizeof(tmpBuf), "ECHO");
            if (flag & ECHOE)
               strPut(tmpBuf, sizeof(tmpBuf), "ECHOE");
            if (flag & ECHOK)
               strPut(tmpBuf, sizeof(tmpBuf), "ECHOK");
            if (flag & ECHONL)
               strPut(tmpBuf, sizeof(tmpBuf), "ECHONL");
#if !defined(SCO)
            if (flag & ECHOCTL)
               strPut(tmpBuf, sizeof(tmpBuf), "ECHOCTL");
            if (flag & ECHOPRT)
               strPut(tmpBuf, sizeof(tmpBuf), "ECHOPRT");
            if (flag & ECHOKE)
               strPut(tmpBuf, sizeof(tmpBuf), "ECHOKE");
            if (flag & FLUSHO)
               strPut(tmpBuf, sizeof(tmpBuf), "FLUSHO");
            if (flag & PENDIN)
               strPut(tmpBuf, sizeof(tmpBuf), "PENDIN");
#endif
            if (flag & TOSTOP)
               strPut(tmpBuf, sizeof(tmpBuf), "TOSTOP");
            if (strlen(tmpBuf))
            {
               fputs("Local flags:\n", p->fp);
               strcat(tmpBuf, "\n");
               fputs(tmpBuf, p->fp);
            }
            fflush(p->fp);
         }
         else 
         {
            if (p->verbose) 
            {
               if ((x % 20) == 0) 
               {
                  fputs("\n\nName            Port     Device          State      IP\n", p->fp);
                  fputs("===================================================================\n", p->fp);
               }
               x++;
               fprintf(p->fp, "%-15s %-8s %-15s %-10s %s\n", pDevc->dnam, pDevc->srvc, 
                                                   pDevc->devc,
                                                   (pDevc->enabled == 0) ?
                                                   "Disabled" : (pDevc->pid > 0) ?
                                                   "In Use" : "Enabled",
                                                   (pDevc->pid == 0) ? "" :
                                                   inet_ntoa(pDevc->addr.sin_addr));
            }
            else
            {
               fprintf(p->fp, "105:%s:%s:%s:%s:%s\n", pDevc->dnam, pDevc->srvc, 
                                                   pDevc->devc,
                                                   (pDevc->enabled == 0) ?
                                                   "Disabled" : (pDevc->pid > 0) ?
                                                   "In Use" : "Enabled",
                                                   (pDevc->pid == 0) ? "" :
                                                   inet_ntoa(pDevc->addr.sin_addr));
            }
         }
      }
   }
}

void doSet(ControlStruct *p, char *pStr)
{
   int baud, x, b;
   portConf *pDevc;
   char tmpBuf[20], *cp, *cp1;

   if (dbg) syslog(LOG_DEBUG, "doSet():Enter with \"%s\"", pStr);
   for (x = 0, cp = pStr, cp1 = tmpBuf; *cp != '\0' && !isspace(*cp) && x < 20; x++)
   {
      *(cp1++) = tolower(*(cp++));
      *cp1 = '\0';
   }

   if (dbg) syslog(LOG_DEBUG, "doSet(): Testing \"%s\" for baud", tmpBuf);
   if (strcmp(tmpBuf, "baud") == 0)
      baud = 1;
/*
   else if (strcmp(tmpBuf, "conf"))
      baud = 0;
*/
   else
      fputs("Err:Expecting \"baud\"\n", p->fp);

   if ((cp = parseRE(p, cp)) == NULL)
      return;

   if (dbg) syslog(LOG_DEBUG, "doSet(): Setting baud rate to \"%s\"", cp);
   b = atoi(cp);
   for (pDevc = pPortList; pDevc != NULL; pDevc = pDevc->pNext)
   {
      if (regexec(&p->portRE, pDevc->srvc, 0, NULL, 0) == 0 &&
          (regexec(&p->devcRE, pDevc->dnam, 0, NULL, 0) == 0 ||
           regexec(&p->devcRE, pDevc->devc, 0, NULL, 0) == 0))
      {
         if (baud)
         {
            for (x = 0; x < 19; x++) 
            {
               if (b == baudList[x] && b != 0) 
                  break;
            }
            if (dbg) syslog(LOG_DEBUG, "doSet(): Setting baud rate index to \"%d\"", x);
            if (x == 19) 
            {
               fprintf(p->fp, "Err:Invalid Baudrate (%d, \"%s\") entered!\n", b, cp);
               return;
            }
            pDevc->speed = x;
         }
/*
         else
            setPortStr(pDevc, cp);
*/
         if (p->verbose)
         {
            char bbuf[128];
            sprintf(bbuf, "%d", baudList[pDevc->speed]);

            fprintf(p->fp, "Setting %s for port: %s:%s(%s) to %s\n",
                             (baud == 1) ? "baud" : "port",
                             pDevc->srvc,
                             pDevc->dnam,
                             pDevc->devc,
                             (baud == 1) ? bbuf : pDevc->srvc);
         }
         else
         {
            char bbuf[128];
            sprintf(bbuf, "%d", baudList[pDevc->speed]);

            fprintf(p->fp, "%s:%s:%s:%s:New default %s for device:%s\n",
                             (baud == 1) ? "104" : "103",
                             pDevc->srvc,
                             pDevc->dnam,
                             pDevc->devc,
                             (baud == 1) ? "baudrate" : "port",
                             (baud == 1) ? bbuf : pDevc->srvc);
         }
      }
   }
}

char *getStr(char *pBuf, const char *cp)
{
   int escape = 0;
   char quote = '\0', ch, *tp;

   tp = pBuf;
   if (dbg) syslog(LOG_DEBUG, "getStr():Enter");
   while (isspace(*cp)) cp++;
   while (*cp) 
   {
      if (dbg) syslog(LOG_DEBUG, "getStr():Have char %2x, escape = %d", *cp, escape);
      if (!escape) 
      {
         if (*cp == '\\')
         {
            escape = 1;
         }
         else if (*cp == '"' || *cp == '\'')
         {
            if (quote != '\0') 
            {
               quote = *cp;
            }
            else
            {
               quote = '\0';
            }
         }
         else if (quote == '\0')
         {
            if (*cp == ' ' || *cp == '\t')
               break;
         }
         if (*cp == '\r' || *cp == '\n' || *cp == '\0')
         {
            break;
         }
         if (*cp == '\t')
         {
            ch = ' ';
         }
         else 
            ch = *cp;
         if (dbg) syslog(LOG_DEBUG, "getStr():Adding char %2x", ch);
         *(pBuf++) = ch;
         *pBuf = '\0';
         cp++;
      }
      else
      {
         if (*cp == '\r' || *cp == '\n' || *cp == '\0') 
         {
            break;
         }
         if (dbg) syslog(LOG_DEBUG, "getStr():Adding char %2x", ch);
         *(pBuf++) = *(cp++);
         *pBuf = '\0';
      }
   }
   if (dbg) syslog(LOG_DEBUG, "getStr():Returning string |%s| and |%s|", tp, cp);
   return((char*)cp);
}

char *parseRE(ControlStruct *p, const char *pStr)
{
   int x, err;
   char tmpBuf[20], *cp, *cp1, *rv;
   char portStr[80], devcStr[80], errBuf[80];

   strcpy(portStr, ".*");
   strcpy(devcStr, ".*");
   cp = (char*)pStr;
   for (;;)
   {
      strcpy(tmpBuf, "");
      for (rv = cp; *cp && isspace(*cp); cp++);
      rv = cp;
      for (x = 0, cp1 = tmpBuf; *cp != '\0' && !isspace(*cp) && x < 20; x++)
      {
         *(cp1++) = tolower(*cp++);
         *cp1 = '\0';
      }
      if (strcmp(tmpBuf, "port") == 0)
         cp = getStr(portStr, cp);
      else if (strcmp(tmpBuf, "device") == 0)
         cp = getStr(devcStr, cp);
      else
      {
         break;
      }
   }
   if (dbg) syslog(LOG_DEBUG, "parseRE():have portStr of |%s|, |%s|", portStr, rv);
   if (dbg) syslog(LOG_DEBUG, "parseRE():have devcStr of |%s|, |%s|", devcStr, rv);
   if ((err = regcomp(&p->portRE, portStr, REG_NOSUB)) != 0)
   {
      regerror(err, &p->portRE, errBuf, sizeof(errBuf));
      fprintf(p->fp, "Err:%s in port RE\n", errBuf);
      rv = NULL;
   }
   if ((err = regcomp(&p->devcRE, devcStr, REG_NOSUB)) != 0)
   {
      regerror(err, &p->devcRE, errBuf, sizeof(errBuf));
      fprintf(p->fp, "Err:%s in Device RE\n", errBuf);
      rv = NULL;
   }
   if (dbg) syslog(LOG_DEBUG, "parseRE():returning with |%s|", rv);
   return(rv);
}
