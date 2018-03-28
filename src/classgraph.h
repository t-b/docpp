/*
  classgraph.h

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
  License along with this library; if not, write to the Free
  Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#ifndef _CLASS_GRAPH_H
#define _CLASS_GRAPH_H

#include <assert.h>
#include <stdlib.h>

#include "McString.h"
#include "doc.h"

/// Class graph class
class ClassGraph
{
public:
    /// constructor for entry
    ClassGraph(Entry *cls, int ind)
	: nextLine(0), indent(ind), entry(cls)
    {
	firstLine = this;
	assert(cls->isClass());
    }

    /// constructor for name
    ClassGraph(const McString& nm, int ind)
	: nextLine(0), indent(ind), entry(0), name(nm)
    {
	firstLine = this;
    }

    void addAllChilds();

    void addBases();

    void addDirectChilds();

    /// pointer to first line of this class graph
    ClassGraph*	firstLine;

    /// pointer to next line in this class graph
    ClassGraph*	nextLine;

    /// number of spaces to indent
    int	indent;

    /** String coding of graphs.  These strings encode the graph to
      draw using single characters to represent lines and arrow.
      The following characters are used:
      \begin{description}
      \item[<blank>]	a Space
      \item[l]		top left arrow
      \item[L]		in between left arrow
      \item[^]		first bottom right line
      \item[|]		first in between right line
      \item[r]		bottom right line
      \item[R]		in between right line
      \item[.]		private inheritance
      \item[-]		protected inheritance
      \item[_]		public inheritance
      \end{description}
      */
    //@{
    /// string coding arrows to be typeset before class box
    McString before;

    	/// string coding arrows to be typeset after class box
    McString after;
    //@}

    /// pointer to entry defining class to be set into box
    Entry *entry;

    /// if no entry to this box is known, take this name instead
    McString name;

private:
    void addChilds(const char *strt, bool recursive);
};

#endif	
