/*
  portability.h

  Copyright (c) 2001 Dragos Acostachioaie

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

#ifndef _PORTABILITY_H
#define _PORTABILITY_H

#ifdef _MSC_VER

#include <direct.h>
#include <io.h>
#include <stdio.h>
#include <string.h>

inline int chdir(const char* const dirname)
{
    return _chdir(dirname);
}

inline int fileno(const FILE* const stream)
{
    return _fileno(stream);
}

inline char* getcwd(char* const buffer, const int maxlen)
{
    return _getcwd(buffer, maxlen);
}

inline int isatty(const int handle)
{
    return _isatty(handle);
}

inline char* strdup(const char* const strSource)
{
    return _strdup(strSource);
}

inline int unlink(const char* const filename)
{
    return _unlink(filename);
}

#elif defined __SVR4

#include <cstdlib.h>

#endif	// _MSC_VER

#endif	// _PORTABILITY_H
