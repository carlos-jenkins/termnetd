#**************************************************************************
# makefile for the terminal server daemon
#
# Copyright (C) 1995, 1996  Joseph Croft <jcroft@unicomp.net>  
# 
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 1, or (at your option)
# any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
#
#**************************************************************************/

TNDOBJ = termnetd.o tndCfgParse.o tndCfgParser.o tndConfig.o tndLock.o \
			tndModem.o tndSocket.o tndSpawn.o tndVars.o \
			tndAdmin.o

# LLIBS = -lcrypt
LLIBS =

HEADERS =

all: termnetd

clean:
	rm -f *.bak *.o tndCfgParse.c termnetd core

termnetd: $(TNDOBJ) ../libtn/libtn.a 
	$(CC) -g -o termnetd -L../libtn $(TNDOBJ) -ltn $(LIBS)
