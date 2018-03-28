/*
  McHashTable.cc

  Copyright (c) 1996 Roland Wunderling, Malte Zoeckler
  Copyright (c) 1999-2000 Dragos Acostachioaie

  This file is part of DOC++.

  DOC++ is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public
  License as published by the Free Software Foundation; either
  version 2 of the license, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public
  License along with this program; if not, write to the Free
  Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include "McHashTable.h"

// One word summary of a string. This is taken from Tcl.
int hash(const char *str)
{
    int result = 0;
    while(1)
	{
	char c = *str++;
	if(c == 0)
	    break;
	result += (result << 3) + c;
	}
    return result;
}

int hash(int i)
{
    return i;
}

int compare(const char *str1, const char *str2)
{
    return strcmp(str1, str2);
}

int compare(int i1, int i2)
{
    return i1 - i2;
}
