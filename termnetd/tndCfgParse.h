/***************************************************************************
**    tndCfgParse.h
**    Termnetd Configuration File Parser
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
#ifndef TNDCFGPARSE_H
#define TNDCFGPARSE_H

#define YY_IFLAG           100
#define YY_IGNBRK          100
#define YY_BRKINT          101 
#define YY_IGNPAR          102 
#define YY_PARMRK          103 
#define YY_INPCK           104 
#define YY_ISTRIP          105 
#define YY_INCLCR          106 
#define YY_IGNCR           107 
#define YY_ICRNL           108 
#define YY_IUCLC           109 
#define YY_IXON            110
#define YY_IXANY           111
#define YY_IXOFF           112
#define YY_IMAXBEL         113

#define YY_OFLAG           120
#define YY_OPOST           120
#define YY_OLCUC           121
#define YY_ONLCR           122
#define YY_OCRNL           123
#define YY_ONOCR           124
#define YY_ONLRET          125
#define YY_OFILL           126
#define YY_OFDEL           127
#define YY_NL0             128
#define YY_NL1             129
#define YY_CR0             130
#define YY_CR1             131
#define YY_CR2             132
#define YY_CR3             133
#define YY_TAB0            134
#define YY_TAB1            135
#define YY_TAB2            136
#define YY_TAB3            137
#define YY_XTABS           138
#define YY_BS0             139
#define YY_BS1             140
#define YY_VT0             141
#define YY_VT1             142
#define YY_FF0             143
#define YY_FF1             144

#define YY_CFLAG           160
#define YY_CS5             160
#define YY_CS6             161
#define YY_CS7             162
#define YY_CS8             163
#define YY_CSTOPB          164
#define YY_PARENB          165
#define YY_PARODD          166
#define YY_HUPCL           167
#define YY_CLOCAL          168
#define YY_CIBAUD          169
#define YY_CRTSCTS         170
#define YY_CREAD           171

#define YY_LFLAG           180
#define YY_ISIG            180
#define YY_ICANON          181
#define YY_XCASE           182
#define YY_ECHO            183
#define YY_ECHOE           184
#define YY_ECHOK           185
#define YY_ECHONL          186
#define YY_ECHOCTL         187
#define YY_ECHOPRT         188
#define YY_ECHOKE          189
#define YY_FLUSHO          180
#define YY_NOFLSH          181
#define YY_TOSTOP          182
#define YY_PENDIN          183
#define YY_IEXTEN          184

#define YY_SPEED           200
#define YY_B50             200
#define YY_B75             201
#define YY_B110            202
#define YY_B134            203
#define YY_B150            204
#define YY_B200            205
#define YY_B300            206
#define YY_B600            207
#define YY_B1200           208
#define YY_B1800           209
#define YY_B2400           210
#define YY_B4800           211
#define YY_B9600           212
#define YY_B19200          213
#define YY_B38400          214
#define YY_B57600          215
#define YY_B115200         216
#define YY_B230400         217

#define  YY_INT            300
#define  YY_IDENT          301
#define  YY_DEVICE         302
#define  YY_ON             1
#define  YY_OFF            2

#define ECFG_SYNTAX        1
#define ECFG_MEMORY        2

union yy_lval
{
   int   i;
   char  s[256];
};

#define YYSTYPE union yy_lval

#endif  /* TNDCFGPARSE_H */
