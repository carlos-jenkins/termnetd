%{
/*
****************************************************************************
**    tndCfgParse.h
**    Termnetd Configuration File Parser Lex definition
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
****************************************************************************
*/
#include "../sysdefs.h"
#include <stdlib.h>
#include <string.h>
#include "tndCfgParse.h"

extern YYSTYPE yylval;

void parseStr(const char *pStr);
void yyerror(const char *errStr);

%}

%%

[\t ]+   {/* Ignore Whitespace */ }
[\n\r]   {/* Ignore Newlines and Carriage returns */}
#.*      { /* ignore Comments   */ }

IGNBRK         { return(YY_IGNBRK); }
BRKINT         { return(YY_BRKINT); }
IGNPAR         { return(YY_IGNPAR); }
PARMRK         { return(YY_PARMRK); }
INPCK          { return(YY_INPCK); }
ISTRIP         { return(YY_ISTRIP); }
INCLCR         { return(YY_INCLCR); }
IGNCR          { return(YY_IGNCR); }
ICRNL          { return(YY_ICRNL); }
IUCLC          { return(YY_IUCLC); }
IXON           { return(YY_IXON); }
IXANY          { return(YY_IXANY); }
IXOFF          { return(YY_IXOFF); }
IMAXBEL        { return(YY_IMAXBEL); }

OPOST          { return(YY_OPOST); }
OLCUC          { return(YY_OLCUC); }
ONLCR          { return(YY_ONLCR); }
OCRNL          { return(YY_OCRNL); }
ONOCR          { return(YY_ONOCR); }
ONLRET         { return(YY_ONLRET); }
OFILL          { return(YY_OFILL); }
OFDEL          { return(YY_OFDEL); }
NL0            { return(YY_NL0); }
NL1            { return(YY_NL1); }
CR0            { return(YY_CR0); }
CR1            { return(YY_CR1); }
CR2            { return(YY_CR2); }
CR3            { return(YY_CR3); }
TAB0           { return(YY_TAB0); }
TAB1           { return(YY_TAB1); }
TAB2           { return(YY_TAB2); }
TAB3           { return(YY_TAB3); }
XTABS          { return(YY_XTABS); }
BS0            { return(YY_BS0); }
BS1            { return(YY_BS1); }
VT0            { return(YY_VT0); }
VT1            { return(YY_VT1); }
FF0            { return(YY_FF0); }
FF1            { return(YY_FF1); }

CS5            { return(YY_CS5); }
CS6            { return(YY_CS6); }
CS7            { return(YY_CS7); }
CS8            { return(YY_CS8); }
CSTOPB         { return(YY_CSTOPB); }
PARENB         { return(YY_PARENB); }
PARODD         { return(YY_PARODD); }
HUPCL          { return(YY_HUPCL); }
CREAD          { return(YY_CREAD); }
CLOCAL         { return(YY_CLOCAL); }
CIBAUD         { return(YY_CIBAUD); }
CRTSCTS        { return(YY_CRTSCTS); }

ISIG           { return(YY_ISIG); }
ICANON         { return(YY_ICANON); }
XCASE          { return(YY_XCASE); }
ECHO           { return(YY_ECHO); }
ECHOE          { return(YY_ECHOE); }
ECHOK          { return(YY_ECHOK); }
ECHONL         { return(YY_ECHONL); }
ECHOCTL        { return(YY_ECHOCTL); }
ECHOPRT        { return(YY_ECHOPRT); }
ECHOKE         { return(YY_ECHOKE); }
FLUSHO         { return(YY_FLUSHO); }
NOFLSH         { return(YY_NOFLSH); }
TOSTOP         { return(YY_TOSTOP); }
PENDIN         { return(YY_PENDIN); }
IEXTEN         { return(YY_IEXTEN); }

B50            { return(YY_B50); }
B75            { return(YY_B75); }
B110           { return(YY_B110); }
B134           { return(YY_B134); }
B150           { return(YY_B150); }
B200           { return(YY_B200); }
B300           { return(YY_B300); }
B600           { return(YY_B600); }
B1200          { return(YY_B1200); }
B1800          { return(YY_B1800); }
B2400          { return(YY_B2400); }
B4800          { return(YY_B4800); }
B9600          { return(YY_B9600); }
B19200         { return(YY_B19200); }
B38400         { return(YY_B38400); }
B57600         { return(YY_B57600); }
B115200        { return(YY_B115200); }
B230400        { return(YY_B230400); }

[Oo][Nn]      { return(YY_ON); }
[Oo][Ff][Ff]  { return(YY_OFF); }

"/dev/"[.A-Za-z][.A-Za-z0-9_-]* { strcpy(yylval.s, yytext); return(YY_DEVICE); }
[0-9A-Za-z_-]+ { strcpy(yylval.s, yytext); return(YY_IDENT); }
.              { return(yytext[0]); }

%%

int yywrap()
{
   return(1);
}

