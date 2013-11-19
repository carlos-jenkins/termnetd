/***************************************************************************
**    trnParse.c
**    Termnet Chat Parser
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
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include "termnet.h"

static int dbg = 0;

int ctoi(char ch, int base, char *rslt)
{
   int rv = 1;

   if (base == 16)
   {
      if (dbg) printf("ctoi():base 16, ch = 0x%x, %c\n\r", ch, ch);
      if (isxdigit(ch))
      {
         *rslt *= 16;
         if ((ch = toupper(ch) - '0') > 9)
            *rslt += ch - 7;
         else
            *rslt += ch;
         if (dbg) printf("ctoi():base 16, rslt = 0x%x\n\r", *rslt);
      }
      else
         rv = 0;
   }
   else if (base == 8)
   {
      if (dbg) printf("ctoi():base 8, ch = 0x%x, %c\n\r", ch, ch);
      if (ch >= '0' && ch <= '7')
      {
         *rslt *= 8;
         *rslt += ch - '0';
         if (dbg) printf("ctoi():base 8, rslt = 0%x\n\r", *rslt);
      }
      else
         rv = 0;
   }
   else
   {
      if (dbg) printf("ctoi():base 10, ch = 0x%x, %c\n\r", ch, ch);
      if (isdigit(ch))
      {
         *rslt *= 10;
         *rslt += ch - '0';
         if (dbg) printf("ctoi():base 10, rslt = 0x%x\n\r", *rslt);
      }
      else
         rv = 0;
   }
   if (dbg) printf("ctoi():exit(%d)\n", rv);
   return(rv);
}

char *nextItem(FILE *fp, char *buff)
{
   char lastChar = '\0', lastQuote = '\0', ch, *cp, x;
   int base;

   
   /*
   **    Skip any leading white space
   */
   while (!feof(fp))
   {
      ch = fgetc(fp);
      if (!isspace(ch))
         break;
   }

   /*
   **    If we're at the end of file, do nothing
   */
   if (feof(fp))
      return(NULL);

   /*
   **    Otherwise, collect charecters untill the next white space or
   **    eof is encountered.
   */
   cp = buff;
   *cp = '\0';
   while (ch != 0)
   {
      if (lastChar != '\\')
      {
         if (!lastQuote)
            if (isspace(ch))
               break;

         if (ch == '\'' || ch == '\"' || ch == '`')
         {
            if (ch == '`')
            {
               *(cp++) = ch;
               *cp = '\0';
               if (lastQuote == ch)
               {
                  inputBuffer(buff);
                  nextItem(fp, buff);
                  break;
               }
            }
            if (lastQuote == ch)
               break;
            else if (lastQuote == '\0')
               lastQuote = ch;
         }
         else if (ch != '\\')
         {
               *(cp++) = ch;
               *cp = '\0';
         }
         lastChar = ch;
      }
      else
      {
         lastChar = '\0';
         switch (toupper(ch))
         {
            case 'X':
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
               base = (ch == 'X' || ch == 'x') ? 16 : ((ch == '0') ? 8 : 10);
               if (dbg) printf("nextItem():base = %d\n\r", base);
               if (base == 16)
                  ch = '0';
               x = 0;
               while (!feof(fp))
               {
                  if (!ctoi(ch, base, &x))
                     break;
                  lastChar = ch;
                  ch = fgetc(fp);
               }
               if (dbg) printf("nextItem():rslt = 0x%x\n\r", x);
               *(cp++) = x;
               *cp = '\0';
               if (!feof(fp))
                  continue;
               else
                  break;

            case 'R':
               *(cp++) = '\r';
               *cp = '\0';
               break;

            case 'N':
               *(cp++) = '\n';
               *cp = '\0';
               break;

            case 'T':
               *(cp++) = '\t';
               *cp = '\0';
               break;

            case 'E':
               *(cp++) = 0x1b;
               *cp = '\0';
               break;

            default:
               *(cp++) = ch;
               *cp = '\0';
               break;
         }
      }
      if (feof(fp))  
         ch = 0;     
      else           
         ch = fgetc( fp);
   }                 
   return(buff);     
}                    
                     

