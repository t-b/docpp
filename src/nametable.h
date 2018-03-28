/*
  nametable.h

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

#ifndef	_NAME_TABLE_H
#define	_NAME_TABLE_H

#include <assert.h>
#include <iostream>

#include "datahashtable.h"

/** Maps names to names.

    Class #NameTable# implements a map from names to names. It allows to store
    or remove names (i.e. #char*#), but does not provide means for manipulating
    stored names.

    All names (i.e. the actual #char# strings) in a #NameTable# are stored in one
    continuous memory block of size #memMax()#. At one time #memSize()# bytes of
    it are used for actually saving names; the remaining memory is free to hold
    additional names. #memRemax()# can be used to reset #memMax()# but not lower
    than to #memSize()#. Method #memPack()# performs a garbage collection to
    gain free memory resulting from removed names.
*/
class NameTable
{
public:
    class Name
    {
    public:
	const char *name;

	friend int operator == (const Name& n1, const Name& n2)
	    {
	    return (strcmp(n1.name, n2.name) == 0);
	    }

	friend ostream& operator << (ostream& out, const Name& n)
	    {
	    return out << n.name;
	    }

	friend int hashFunction(const Name *);

	int isConsistent() const
	    {
	    return (name != 0);
	    }

	Name(const char *str)
	    {
	    name = str;
	    }
    };

    DataHashTable<Name,int> table;	// hashtable for names
    McDArray<char> names;		// memory where to store names

    ///	return nr. of names in #NameTable#
    int	num() const
	{
	return table.num();
	}

    ///	return maximum nr. of names that fit into #NameTable#
    int	max() const
	{
	return table.max();
	}

    ///	does #NameTable# have name #str#?
    int	has(const char *str) const
	{
	const Name nam(str);
	return table.has(nam);
	}

    ///	return number for #name#
    int	operator [] (const char *str) const
	{
	const Name nam(str);
	return table[nam];
	}

    /** @name Iteration */
    //@{
    const char*	first() const
    	{
	if(table.first())
	    return table.current()->name;
	else
	    return 0;
	}

    const char *last() const
    	{
	if(table.last())
	    return table.current()->name;
	else
	    return 0;
	}

    const char *next() const
    	{
	if(table.next())
	    return table.current()->name;
	else
	    return 0;
	}

    const char *current() const
    	{
	if(table.current())
	    return table.current()->name;
	else
	    return 0;
	}

    const char *prev() const
    	{
	if(table.prev())
	    return table.current()->name;
	else
	    return 0;
	}
    //@}

    void add(int num, const char *name);

    void clear();

    friend ostream& operator << (ostream& out, const NameTable& nt);

    friend istream& operator >> (istream& out, NameTable& nt);

    int isConsistent() const;

    NameTable();
};

#endif
