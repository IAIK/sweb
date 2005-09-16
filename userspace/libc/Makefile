# Projectname: SWEB
# Simple operating system for educational purposes
#
# Copyright (C) 2005  Andreas Niederl
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.



#
# CVS Log Info for $RCSfile: Makefile,v $
#
# $Id: Makefile,v 1.1 2005/09/16 05:01:40 aniederl Exp $
# $Log$
#

LIBC_PATH = src/
LIBC_STATIC = $(LIBC_PATH)/libc.a


all: libc

libc: libc-static

libc-static:
	 make -C $(LIBC_PATH)

clean:
	 make clean -C $(LIBC_PATH)