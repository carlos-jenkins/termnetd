Termnetd: Network serial port daemon
====================================

Termnetd (aka termpkg apparently) is a cool little application that lets you
stream serial port data over the network. It's a terminal server daemon that
exposes the serial port and tty devices directly to a network port.


Installation
------------

First Get the repository, execute the configure script, change directory to the
platform detected and run the ``make`` command:

    git clone git@github.com:carlos-jenkins/termnetd.git
    cd termnetd
    ./configure
    cd linux
    make


Configuration
-------------

Once installed, the configuration file, ``/etc/termnetd.conf``, can be edited
to configure which serial port maps to which network port. It can be used to
set serial port connection settings as well. The format is as follows:

    <IP port>:<state>:<device>:<termios options>;

**Example:**

    3000:on:/dev/ttyS0:B115200 CLOCAL IGNBRK CRTSCTS CS8 CREAD;


Usage
-----

Start termnetd by running from the command line:

    termnetd

From another terminal use telnet or netcat to receive the data

    nc localhost 3000


License
-------

Copyright (C) 1995, 1996  Joseph Croft <joe@croftj.net>

This program is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free Software
Foundation; either version 2 of the License, or (at your option) any later
version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see http://www.gnu.org/licenses/.

