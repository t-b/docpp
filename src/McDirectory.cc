/*
  McDirectory.cc

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

#include "McDirectory.h"
#include "McSorter.h"
#include "McString.h"
#include "doc.h"

#include <stdio.h>

#ifdef WIN32
#include <windows.h>
#ifdef __BORLANDC__
#define _strdup strdup
#endif

int McDirectory::scan(const McString& dirname, McDArray<char *> &list,
		      const char *pattern)
{
    WIN32_FIND_DATA findFileData;
    char buf[1024];

    if(!pattern)
	pattern = "*";
    sprintf(buf, "%s\\%s", dirname.c_str(), pattern);

    HANDLE searchHandle = FindFirstFile(buf, &findFileData);
    if(searchHandle == INVALID_HANDLE_VALUE)
	{
	fprintf(stderr, _("No files matching `%s' found\n"), buf);
	return 0;
	}
    int next = 1;
    while(searchHandle != INVALID_HANDLE_VALUE && next)
	{
	if(strcmp(".", findFileData.cFileName) != 0 &&
	    strcmp("..", findFileData.cFileName) != 0)
	list.append(_strdup(findFileData.cFileName));
	next = FindNextFile(searchHandle, &findFileData);
	}
    FindClose(searchHandle);
    StringCompare comp;
    if(list.size())
	sort((char **)list, list.size(), comp, 0);
    return 0;
}

int McDirectory::isDirectory(const McString& dirname)
{
    McString d(dirname);

    if(!d.length())
	return 0;
    while(d.length() && d[d.length() - 1] == pathDelimiter())
	d.remove(d.length() - 1);
    WIN32_FIND_DATA findFileData;
    HANDLE searchHandle = FindFirstFile(d.c_str(), &findFileData);
    if(searchHandle == INVALID_HANDLE_VALUE)
	return 0;
    int ret = ((findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0);
    FindClose(searchHandle);
    return ret;
}
#else
#include <dirent.h>
/* Great. We're doing UNIX, not Windows ! */

int McDirectory::scan(const McString& dirname, McDArray<char *> &list,
		      const char *pattern)
{
    DIR *dir = opendir(dirname.c_str());    
    struct dirent *entry;

    if(!pattern)
	pattern = "*";
    if(!dir)
	{
	fprintf(stderr, _("Can't open `%s' dir\n"), dirname.c_str());
	return 0;
	}

    while((entry = readdir(dir)))
	if(entry->d_name[0] != '.')
	    if(mcWildMatch(entry->d_name, pattern))		
		list.append(strdup(entry->d_name));
    closedir(dir);
    StringCompare comp;
    if(list.size())
	sort((char **)list, list.size(), comp, 0);
    return list.size();
}

int McDirectory::isDirectory(const McString& dirname)
{
    McString d(dirname);

    if(!d.length())
	return 0;
    while(d.length() && d[d.length() - 1] == pathDelimiter())
	d.remove(d.length() - 1);
    DIR *dir = opendir(dirname.c_str());    
    if(!dir)
	return 0;
    closedir(dir);
    return 1;
}
#endif
