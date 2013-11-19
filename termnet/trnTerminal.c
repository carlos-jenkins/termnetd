/***************************************************************************
**    trnTerminal.c
**    Termnet Command/terminal funtions
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
#ifdef AIX
#include <sys/resource.h>
#endif
#include <arpa/telnet.h>
#include <sys/wait.h>
#include <signal.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include "termnet.h"

static int dbg = 0;

typedef struct
{
   char  *Name;
   int   (*Action)(int argc, char *argv[]);
   char  *desc;
} command_entry;

char break_char = BREAK_CHAR;

static char pPrompt[] = "termnet>";
static char escape_char = ESCAPE_CHAR;
static char cmdBuffer[256];
static int margc, pid;
static char *margv[20];

int cmdExit(int argc, char *argv[]);
int cmdChat(int argc, char *argv[]);
int cmdBaud(int argc, char *argv[]);
int cmdPort(int argc, char *argv[]);
int cmdDevice(int argc, char *argv[]);
int cmdEcho(int argc, char *argv[]);
int cmdFile(int argc, char *argv[]);
int cmdPause(int argc, char *argv[]);
int cmdExecio(int argc, char *argv[]);
int cmdExec(int argc, char *argv[]);
int cmdHelp(int argc, char *argv[]);

static command_entry cmdTable[] =
{
   { "HELP",         cmdHelp,          "Display this message"              },
   { "CHAT",         cmdChat,          "Execute chat script"               },
   { "$",            cmdExecio,        "Exectute program"                  },
   { "!",            cmdExec,          "Exectute program"                  },
   { "BAUD",         cmdBaud,          "Set/Display Port Baud Rate"        },
   { "PORT",         cmdPort,          "Set/Display Port Configuration"    },
   { "DEVC",         cmdDevice,        "Set/Display Port Device"           },
   { "PAUSE",        cmdPause,         "Pause x seconds"                   },
   { "ECHO",         cmdEcho,          "Echo parameters to stdout"         },
   { "EXIT",         cmdExit,          "Exit Program"                      },
   { "QUIT",         cmdExit,          "Exit Program"                      },
/*
   { "CAT",          cmdFile,          "Send ASCII File to port"              },
   { "SEND",      cmdSend,    "Send Special Character(s)"            },
   { "CAPTURE",   cmdFile,    "Set/Clear/Toggle Capture File         },
   { "DEVC",      cmdDevc,    "Set/Display Port Device"              },
*/
   { NULL,        NULL,       NULL,                                  },
};

void getInput(char *buf, int len)
{
   char *cp, ch;

   strcpy(buf, "");   
   for (ch = 0, cp = buf; ch != '\n' && ch != '\r';)
   {
      ch = getchar();
      switch (ch)
      {
         case '\x7f':
         case '\b':
            if (cp != buf)
            {
               cp--;
               *cp = '\0';
               fputs("\b \b", userOut);
               len++;
            }
            break;

         case '\r':
         case '\n':
            fputs("\n\r", userOut);
            continue;

         default:
            if (len > 0)
            {
               *(cp++) = ch;
               *cp = '\0';
               len--;
               fputc(ch, userOut);
            }
            break;
      }
   }
   return;
}

static void makeArgs()
{
   register char *cp, *cp2, c;
   register char **argp = margv;

   margc = 0;
   cp = cmdBuffer;
   while ((c = *cp)) 
   {
      register int inquote = 0;
      while (isspace(c))
         c = *++cp;
      if (c == '\0')
         break;
      *argp++ = cp;
      margc += 1;
      for (cp2 = cp; c != '\0'; c = *++cp) 
      {
         if (inquote) 
         {
            if (c == inquote) 
            {
               inquote = 0;
               continue;
            }
         } 
         else 
         {
            if (c == '\\') 
            {
               if ((c = *++cp) == '\0')
                  break;
            } 
            else if (c == '"') 
            {
               inquote = '"';
               continue;
            } 
            else if (c == '\'') 
            {
               inquote = '\'';
               continue;
            } else if (isspace(c))
               break;
         }
         *cp2++ = c;
      }
      *cp2 = '\0';
      if (c == '\0')
         break;
      cp++;
   }
   *argp++ = 0;
}

int cmdExit(int argc, char *argv[])
{
   tnlShutdown(2);
   doExit(1);
   return(0);  /* Should never execute!   */
}

int cmdBaud(int argc, char *argv[])
{

   if (tnlIsOption(TELOPT_BAUDRATE))
   {
      if (argc == 2)
         tnlSendSubOption(TELOPT_BAUDRATE, argv[1]);
      else if (argc == 1)
         tnlRequestSubOption(TELOPT_BAUDRATE);
      else
         fprintf(stderr, "\rusage: baud [<baud rate>]\n\r");
   }
   else
      fprintf(stderr, "\rOption BAUDRATE is disabled\n\r");
   return(0);
}

int cmdPort(int argc, char *argv[])
{
   if (tnlIsOption(TELOPT_PORTSET))
   {
      if (argc == 2)
         tnlSendSubOption(TELOPT_PORTSET, argv[1]);
      else if (argc == 1)
         tnlRequestSubOption(TELOPT_PORTSET);
      else
      {
         fprintf(stderr, "\rusage: port [<settings>]\n\r");
         fprintf(stderr, "\t<settings> can be in the form of [8|7|6|5][E|O|N][C0|C1][S0|S1]\n\r");
         fprintf(stderr, "\t\t[E|O|N] Sets the parite\n\r");
         fprintf(stderr, "\t\t[8|7|6|5] Sets the bits / byte\n\r");
         fprintf(stderr, "\t\t[C0|C1] Sets hardware flow control on/off\n\r");
         fprintf(stderr, "\t\t[S0|S1] Sets software flow control on/off\n\r");
         fprintf(stderr, "\n\r");
      }

   }
   else
      fprintf(stderr, "\rOption PORTSET is disabled\n\r");
   return(0);
}

int cmdDevice(int argc, char *argv[])
{
   char buf[256], *cp, *cp1;

   if (tnlIsOption(TELOPT_DEVICE))
   {
      if (argc == 2)
      {
         for (cp = argv[1], cp1 = buf; *cp;)
            *(cp1++) = toupper(*(cp++));
         *cp1 = '\0';
         if (strcmp(buf, "SELECT") == 0)
            tnlRequestSubOption(TELOPT_NDEVICE);
         else
            tnlSendSubOption(TELOPT_DEVICE, argv[1]);
      }
      else if (argc == 1)
         tnlRequestSubOption(TELOPT_DEVICE);
      else
         fprintf(stderr, "\rusage: device [<settings>]\n\r");
   }
   else
      fprintf(stderr, "\rOption DEVICE is disabled\n\r");
   return(0);
}

int cmdEcho(int argc, char *argv[])
{
   int x;
   char buf[1024], *cp, *cp1;

   if (argc == 2)
   {
      for (cp1 = buf, cp = argv[1]; *cp; cp++)
      {
         *(cp1++) = toupper(*cp);
         *cp1 = '\0';
      }
      if (strcmp(buf, "ON") == 0)
      {
         if (echoFlg == 1 && remoteEcho == 1)
         {
            if (dbg) fprintf(stderr, "Remote Echo Requested OFF!\n\r");
            tnlRequestOption(TELOPT_ECHO, 0);
         }
         echoFlg = localEcho = 1;
         return(0);
      }
      else if (strcmp(buf, "OFF") == 0)
      {
         if (echoFlg == 1)
         {
            {
               if (dbg) fprintf(stderr, "Local Echo Disabled!\n\r");
               localEcho = 0;
            }
            echoFlg = 0;
         }
         return(0);
      }
   }
   for (x = 1; x < argc; x++)
   {
      if (x != 1)
         fputc(' ', stdout);
      fputs(argv[x], stdout);
   }
   fputs("\n", stdout);
   return(0);
}

int cmdChat(int argc, char *argv[])
{
   if ((userIn = fopen(argv[1], "r")) == NULL)
   {
      perror("Error opening input file");
      doExit(1);
   }
   autoMode = 1;
   return(0);
}

int cmdFile(int argc, char *argv[])
{
   int cnt, cnt1, fd;
   char *cp, buf[256];

   if ((fd = open(argv[1], O_RDONLY)) >= 0)
   {
      for (;;)
      {
         if ((cnt = read(fd, buf, sizeof(buf))) > 0)
         {
            fprintf(stderr, "cmdFile():Read %d chars\n\r", cnt);
            for (cp = buf; cnt > 0; cnt -= cnt1)
            {
               cnt1 = write(fileno(tnlout), buf, cnt);
               fprintf(stderr, "cmdFile():Wrote %d chars\n\r", cnt1);
               if (cnt1 == 0)
                  perror("cmdFile()");
               cp += cnt1;
            }
         }
         else
            break;
      }
   }
   else
      perror("Error opening file!!");
   return(0);
}

int cmdPause(int argc, char *argv[])
{
   int x;

   if (argc >= 2)
      x = (atoi(argv[1]) > 0) ? atoi(argv[1]) : 1;
   else
      x = 1;
   sleep(x);
   return(0);
}

void cmdSelectDevice(int argc, char *argv[])
{
   tnlRequestSubOption(TELOPT_NDEVICE);
}

void childHandler(int sig)
{
   if (sig == SIGCHLD)
   {
      execComplete = 1;
      tcsetattr(fileno(userIn), TCSANOW, &RunTermios);
      fprintf(stderr, "\n\rCommand complete\n\r");
      wait3(NULL, 0, NULL);
      execMode = 0;
   }
   else
      kill(pid, sig);
}

int cmdExecio(int argc, char *argv[])
{
   pid = fork();
   if (pid == 0)
   {
      fprintf(stderr, "\n\rExecuting command: %s ...\n\r", argv[1]);
      dup2(fileno(tnlin), 0);
      tcsetattr(0, TCSANOW, &OrgTermios);
      dup2(fileno(tnlout), 1);
      execvp(argv[1], &argv[1]);
      perror("Error executing program");
   }
   else if (pid > 0)
   {
      int err;

      if ((err = waitpid(pid, NULL, WNOHANG)) == 0)
      {
         signal(SIGINT, childHandler); 
         signal(SIGCHLD, childHandler); 
         execMode = 1;
      }
      else if (err < 0)
         perror("Error while waiting command completion");
      else
         fprintf(stderr, "\n\rCommand complete\n\r");
   }
   else
      perror("Forking proccess");
   return(0);
}

int cmdExec(int argc, char *argv[])
{
   pid = fork();
   if (pid == 0)
   {
      tcsetattr(fileno(userIn), TCSANOW, &OrgTermios);
      fprintf(stderr, "\n\rExecuting command: %s ...\n\r", argv[1]);
      execvp(argv[1], &argv[1]);
      perror("Error executing program");
   }
   else if (pid > 0)
   {
      signal(SIGINT, childHandler); 
      signal(SIGCHLD, childHandler); 
   }
   else
      perror("Forking proccess");
   return(0);
}

int cmdHelp(int argc, char *argv[])
{
   command_entry *pCmd;

   for (pCmd = cmdTable; pCmd->Name != NULL; pCmd++)
      printf("\r%s:\t%s\n\r", pCmd->Name, pCmd->desc);
   return(0);
}

static command_entry *findCommand(char *pCmd)
{
   command_entry *pEntry;
   char *cp;
   int x;

   for (cp = pCmd; *cp; cp++)
      *cp = toupper(*cp);

   if (dbg) fprintf(stderr, "findCommand(): enter- command = |%s|\n\r", pCmd);
   for (x = 0, pEntry = &cmdTable[x]; pEntry->Name; pEntry = &cmdTable[++x])
   {  
      if (dbg) fprintf(stderr, "findCommand(): testing |%s|\n\r", pEntry->Name);
      if (strcmp(pEntry->Name, pCmd) == 0)
         break;
   }
   if (pEntry->Name == NULL)
      pEntry = NULL;
   else
      if (dbg) fprintf(stderr, "findCommand(): found |%s|\n\r", pEntry->Name);
   return(pEntry);
}

static void doCommands()
{
   command_entry *pCmd;

   for (;;)
   {
      if (isatty(fileno(userOut)))
         printf("\n\r%s>", pPrompt);
      getInput(cmdBuffer, sizeof(cmdBuffer));
      if (dbg) fprintf(stderr, "doComands():command line length = %d\n\r",
                               strlen(cmdBuffer));
      makeArgs();
      if (margc > 0)
      {
         if ((pCmd = findCommand(margv[0])) != NULL)
         {
            if (dbg) fprintf(stderr, "doCommands(): executing function for command = |%s|\n\r", pCmd->Name);
            if (!(pCmd->Action)(margc, margv))
               break;
         }
         else
            printf("Invalid command\n");
      }
      else
         break;
   }
   if (dbg) fprintf(stderr, "doCommands(): Exiting\n\r");
}

char set_even_parity(const char ch)
{
   int x, p;
   char ch1;

   for (ch1 = ch, p = 0, x = 0; x < 7; x++)
   {
      if (((ch1 >> 1) & 0x01) == 1)
         p++;
   }
   ch1 = ((p & 0x01) == 1) ? ch | 0x80 : ch & 0x7f;
   return(ch1);
}

int inputTerminal(char *pBuf, int len)
{
   unsigned char ch;
   int x;

   if (dbg) fprintf(stderr, "inputTerminal():len = %d\n\r", len);
   for (x = 0; x < len; ++x)
   {
      ch = pBuf[x];
      if (ch == (unsigned char)escape_char)
      {
         if (dbg) fprintf(stderr, "inputTerminal():command mode\n\r");
         doCommands();
      }
      else if (ch == (unsigned char)break_char)
      {
         if (dbg) fprintf(stderr, "inputTerminal():Sending Break to Net\n\r");
         tnlSendSpecial(BREAK);
      }
      else if (ch != '\0')
      {
         if (dbg) fprintf(stderr, "inputTerminal():Sending Net: 0x%x\n\r", ch);
         if (echoFlg && localEcho)
            putc(ch, userOut);

         if (emulate_parity != 0)
            ch = set_even_parity(ch);

         putc(ch, tnlout);
      }
   }
   return(0);
}

static void procCommand()
{
   command_entry *pCmd;

   if (dbg) fprintf(stderr, "procComands():command line length = %d\n\r",
                            strlen(cmdBuffer));
   makeArgs();
   if (margc > 0)
   {
      if ((pCmd = findCommand(margv[0])) != NULL)
      {
         if (dbg) fprintf(stderr, "procCommand(): executing function for command = |%s|\n\r", pCmd->Name);
         (pCmd->Action)(margc, margv);
      }
   }
   if (dbg) fprintf(stderr, "procCommand(): Exiting\n\r");
}

void inputBuffer(char *pBuf)
{
   char *cp, *cp1, ch;

   for (cp = pBuf; *cp;)
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
               ch = 0x0d;
               break;

            case 'x':
               cp++;

            case '0':
               if (dbg) fprintf(stderr, "inputBuffer():found char data: |%s|\n\r", cp);
               ch = strtol(cp, &cp, 0);
               if (dbg) fprintf(stderr, "inputBuffer():ch = 0x%x\n\r", ch);
               if (dbg) fprintf(stderr, "inputBuffer():data remaining: |%s|\n\r", cp);
               break;

            default:
               ch = *cp;
               break;
         }
         if (*cp)
            cp++;
      }
      else if (ch == '`')
      {
         for (cp1 = cmdBuffer; *cp != '`';)
         {
            *(cp1++) = *(cp++);
            *cp1 = '\0';
         }
         cp++;
         if (dbg) fprintf(stderr, "inputBuffer():Processing command: |%s|\n\r", cmdBuffer);
         procCommand();
         continue;
      }
      if (dbg) fprintf(stderr, "inputBuffer():Sending Net: 0x%x\n\r", ch);
      if (echoFlg && localEcho)
         putc(ch, userOut);

      if (emulate_parity != 0)
         ch = set_even_parity(ch);

      write(fileno(tnlout), &ch, 1);
   }
}
