#**************************************************************************
# makefile for the termnet client program
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

TSDOBJ = termnet.o trnVars.o trnParse.o trnTerminal.o
TTDOBJ = ttyd.o trnVars.o trnParse.o trnTerminal.o
         
HEADERS =
LIBDIR = -L../libtn
all: termnet ttyd

depend:
	@makedepend -v -I. -I../libtn termnet.c trnVars.c trnTerminal.c

clean:
	rm -f *.bak *.o ttyd termnet core

termnet: $(TSDOBJ) ../libtn/libtn.a
	$(CC) -o termnet $(TSDOBJ) $(LIBDIR) -ltn $(LIBS)

ttyd: $(TTDOBJ) ../libtn/libtn.a
	$(CC) -o ttyd $(TTDOBJ) $(LIBDIR) -ltn $(LIBS)

# DO NOT DELETE
