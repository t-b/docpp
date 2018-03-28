/*
  nametable.cc

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
  License along with this library; if not, write to the Free
  Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include <assert.h>
#include <iostream>
#include <stdlib.h>
#include <string.h>

#include "McString.h"
#include "nametable.h"

int hashFunction(const NameTable::Name *n)
{
    unsigned int res = 0;
    const char *sptr = n->name;

    while(*sptr)
	{
	res *= 65;
	res += *sptr++ - int('0');
	res %= 0x0fffffff;
	}
    return res;
}

void NameTable::add(int num, const char *name)
{
    int	n = strlen(name) + 1;
    int	i = names.size();
    char *start = names;

    names.append(n, name);
    int	delta = start - (char*)names;
    if(delta)
	for(table.first(); table.current(); table.next())
	    ((Name*)table.current())->name -= delta;

    Name newName(&names[i]);
    table.add(newName, num);
}

void NameTable::clear()
{
    table.clear();
    names.clear();
}

std::ostream& operator << (std::ostream& out, const NameTable& nt)
{
    for(nt.first(); nt.current(); nt.next())
	{
    out << nt[nt.current()] << ':';
    out << nt.current() << char(6) << std::endl;
	}
    return out;
}

std::istream& operator >> (std::istream& in, NameTable& nt)
{
    int num;
    char c;
    McString string;

    nt.clear();
    while(in)
	{
	in >> num;
	in.get(c);
	if(c != ':')
	    break;
	string.clear();
	do
	    {
	    in.get(c);
	    if(c == char(6))
		{
		nt.add(num, string.c_str());
		break;
		}
	    string += c;
	    } while(in);
	}

    return in;
}

int NameTable::isConsistent() const
{
    return names.isConsistent() && table.isConsistent();
}

NameTable::NameTable() : table(hashFunction), names(1000)
{
}
