/*
  McWildMatch.h

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

#ifndef _MC_WILD_MATCH_H
#define _MC_WILD_MATCH_H

/** Matches string against pattern containing wildcards ``?'' and ``*''.

    This routine tests a string against a wild card pattern. The wild 
    characters are ``*'' and ``?''. ``*'' matches an arbitrary sequence of
    characters while ``?'' matches any single character. Returns #true# if the
    match is successful, #false# otherwise.
*/
bool mcWildMatch(const char *str, const char *pattern);

#endif
