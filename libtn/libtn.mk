#**************************************************************************
# makefile for the TelNet Library
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

TNLOBJ = tnlInit.o tnlSelect.o tnlSocket.o tnlOptions.o tnlSubOptions.o \
         tnlVars.o SocketIO.o tnlCallBack.o tnlMisc.o fsminit.o

all: libtn.a

clean:
	rm -f *.bak *.o *.a core

libtn.a: $(TNLOBJ) tnl.h
	ar -rc libtn.a $(TNLOBJ)
