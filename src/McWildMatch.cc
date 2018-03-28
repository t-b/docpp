/*
  McWildMatch.cc

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

bool mcWildMatch(const char *str, const char *pat)
{
    register int i = 0, j = 0;
    int	k = 0;
    bool star = false;

    while(pat[i] != '\0' && str[j] != '\0')
	{
	if(pat[i] == '*')
	    {
	    star = true;
	    while(pat[i] == '*')
		i++;
	    k = i;
	    }
	else
	    {
	    if(pat[i] == '?' || pat[i] == str[j])
		{
	        j++;
		i++;
		}
	    else
		{
		if(!star)
		    return false;
	        i = k;
		j++;
		}
	    }
	}

    while(pat[i] == '*')
	i++;

    if(pat[i] == str[j])
	return true;
    else
	if(pat[i] == '\0' && (pat[i - 1] == '*' || pat[i - 1] == '?'))
	    return true;
	else
	    return false;
}
