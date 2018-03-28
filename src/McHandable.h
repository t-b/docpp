/*
  McHandable.h

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

#ifndef _MC_HANDABLE_H
#define _MC_HANDABLE_H

#include <assert.h>
#include <stdio.h>

/// Base class for ref-counted objects.
class McHandable
{
  public:
    /// Constructor.
    McHandable() : refcount(0)
	{
	}

    /// Adds a reference to an instance.
    void ref()
	{
	refcount++;
	}

    /** Removes a reference from an instance. Calls delete this, if
        this was the last ref.
    */
    void unref()
	{
	assert(refcount > 0);
	if(--refcount == 0)
	    delete this;
	}

    /// Removes a reference but doesn't destroy object.
    void unrefNoDelete()
	{
	assert(refcount > 0);
	--refcount;
	}

protected:

    /// Destructor is protected. Use unref() instead.
    virtual ~McHandable()
	{
	assert(refcount == 0);
	}

    /// Reference count.
    int refcount;
};
#endif
