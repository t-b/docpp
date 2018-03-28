/*
  McString.h

  Copyright (c) 1996 Roland Wunderling, Malte Zoeckler
  Copyright (c) 1998 Michael Meeks
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

#ifndef _MC_STRING_H
#define _MC_STRING_H

#include "McDArray.h"
#include "McHandable.h"
#include "McWildMatch.h"
#include "config.h"

#include <assert.h>
#include <stdlib.h>
#ifdef HAVE_STRING_H
#include <string.h>
#else
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif
#endif

/// Character Strings
class McString : public McDArray<char>, public McHandable
{
public:
    /// clear string
    void clear()	
	{ 
	McDArray<char>::clear();
	append(char(0));
	}

    /// initialize string with #McString#
    McString& operator = (McString mcs)
	{
	McDArray<char>::clear();
	append(mcs.size(), (char *)mcs);
	return *this;
	}

    /// initialize string with #str#
    McString& operator = (char *str)
	{
	McDArray<char>::clear();
	append(strlen(str) + 1, str);
	return *this;
	}

    /// initialize string with #str#
    McString& operator = (const char *str)
	{
	McDArray<char>::clear();
	append(strlen(str) + 1, str);
	return *this;
	}

    /// Return a C string of the text, compatible with the C++ standard.
    const char *c_str() const
	{
        return data;
	}

    /// append #str# to string
    McString& operator += (const char *str)
	{
	remove0();
	append(strlen(str) + 1, str);
	return *this;
	}

    /// append #str# to string
    McString& operator += (const McString& str)
	{
	remove0();
	append(str.length() + 1, str);
	return *this;
	}

    /// append #ch# to string
    McString& operator += (char ch)
	{
	(*this)[length()] = ch;
	append(char(0));
	return *this;
	}

    int operator == (const char *other) const
	{
	return(strcmp(this->c_str(), other) == 0);
	}

    int operator == (const McString &other) const
	{
	return(*this == other.c_str());
	}

    int operator != (const McString &other) const
	{
	return !(*this == other.c_str());
	}

    int operator != (const char *other) const
	{
	return !(*this == other);
	}

    int operator < (const char *other) const
	{
	return(strcmp(this->c_str(), other) < 0);
	}

    int operator <= (const McString& other) const
	{
	return(strcmp(this->c_str(), other.c_str()) <= 0);
	}

    int operator <= (const char *other) const
	{
	return(strcmp(this->c_str(), other) <= 0);
	}

    int operator > (const char *other) const
	{
	return(strcmp(this->c_str(), other) > 0);
	}

    int operator > (const McString& other) const
	{
	return *this > other.c_str();
	}

    int operator >= (const char *other) const
	{
	return(strcmp(this->c_str(), other) >= 0);
	}

    /// Find a substring, returns index or -1 if not found
    int index(const char *needle) const
	{
	return index(needle, 0);
	}

    /// Return last char in string. Not the trailing zero.
    char &last() const
	{
	assert(length() > 0);
	return data[length() - 1];
	}

    /// Insert string #addBefore#-the element.
    int insert(int addBefore, const char *str)
	{
	assert(str);
	return McDArray<char>::insert(addBefore, strlen(str), str);
	}

    /** Find element t, starting at index.

        @return The index of #t#, or -1 if t was not found.
    */
    int index(const char t, int start = 0) const
	{
	if(length() < 1)
	    return -1;
	const char *end = &last();
	for(const char *i = data + start; i <= end; i++)
	    if(*i == t)
		return i - data;
	return -1;
	}

    /** Find element t, starting at #start#, searching {\bf backward}.

	If start is -1, search is started at the end.

	@return The index of #t#, or -1 if t was not found.
    */
    int rindex(const char t, int start = -1) const
	{
	if(length() < 1)
	    return -1;
	const char *i;
	if(start >= 0)
	    i = data + start;
	else
	    i = &last();

	for(; i >= data; i--)
	    if(*i == t)
		return i - data;
	return -1;
	}

    /// find a substring, start searching at ndx, return index or -1 if not found
    int index(const char *needle, int indx) const
	{
	const char *tmp = strstr(&(*this)[indx], needle);
	if(tmp == NULL)
	    return -1;
	return tmp - &(*this)[0];
	}

    /// number of characters in string
    int	length() const
	{
	return size() - 1;
	}

    /// Default constructor
    McString() : McDArray<char>(0, 16, (float)1.5)
	{
	append(char(0));
	}

    /// Copy constructor
    McString(const McString& rhs) : McDArray<char>(rhs)
	{
	}

    McString(const char *str) : McDArray<char>(0, 16, (float)1.5)
	{
	if(str)
	    append(strlen(str) + 1, str);
	}

    /// Copy a part from another string.
    McString(const char *str, int start, int len) : McDArray<char>(0, len + 1)
	{
	append(len, &str[start]);
	append(char(0));
	}

    /// Copy a part from another string.
    McString(const McString& str, int start, int len) : McDArray<char>(0, len + 1)
	{
	append(len, &(str.c_str())[start]);
	append(char(0));
	}
  
    McString& operator + (const char *str2) const
	{
	McString *str = new McString(*this);
	*str += str2;
	return *str;
	}

    friend McString& operator + (const char *str1, const McString& str2)
	{
	McString *str = new McString(str1);
	*str += str2;
	return *str;
	}

    /// Matches string against pattern containing wildcards.
    int matches(const char *pattern) const
	{
	return mcWildMatch((const char*)(*this), pattern);
	}

private:
    /// Remove the trailing 0 from the string
    void remove0()
	{
	resize(size() - 1);
	}
};

#endif	
