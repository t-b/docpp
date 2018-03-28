/*
  McDirectory.h

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

#ifndef _MCDIRECTORY_H
#define _MCDIRECTORY_H

#include "McDArray.h"
#include "McString.h"

#if defined(WIN32) || defined(__BORLANDC__) || defined(__VISUALC__) || defined(__WATCOMC__) || defined(__MINGW32__)
#define PATH_DELIMITER '\\'
#else
#define PATH_DELIMITER '/'
#endif

class McDirectory
{
public:

  /// Character used to separate directories in a path.
    static inline char pathDelimiter()
	{
	return PATH_DELIMITER;
	}

  /** Scan a directory for files.

    dirname contains path of directory, pattern specifies which files to use
    (may contain wildcards), the result is returned in list.

    @returns 0 if averything went o.k., !=0 otherwise.
  */
  static int scan(const McString& dirname, McDArray<char *> &list, const char *pattern);

  /// Check whether a given name is directory.
  static int isDirectory(const McString& dirname);

protected:
    class StringCompare
	{
	public:
	    int operator()(const char *t1, const char *t2)
		{
		return strcmp(t1, t2);
		}
	};
};
#endif
