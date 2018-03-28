/*
  datahashtable.h

  Copyright (c) 1996 Roland Wunderling, Malte Zoeckler
  Copyright (c) 1998 Michael Meeks
  Copyright (c) 1998-2000 Dragos Acostachioaie

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

#ifndef _DATAHASHTABLE_H
#define _DATAHASHTABLE_H

#include <assert.h>
#include <iostream>
#include <stdlib.h>

#include "McDArray.h"

/* This should be a private subclass of #DataHashTable#. However, since cfront
   is not able to compile this constrution, we had move the class to global
   scope.
*/
template <class HashItem, class Info>
struct DataHashTable_Element
{
    HashItem	item;
    Info	info;
    enum
	{
	FREE,		// element has never been used
	RELEASED,	// element had been used, but released
	USED 		// element is in use
	} status;
};

/** Class #DataHashTable# provides a generic hash table for \Ref{Data Objects},
    i.e. a map that maps arguments called #HashItem#s to values called #Info#s.
    #HashItem# and #Info# types are passed as #template# arguments. #HashItem#s
    must provide a comparision #operator==#.  Further both, the #HashItem# and
    #Info# must be data objects in the sense, that the assignment operator is
    equivalent to a #memcpy()# of the structure and no destructor is required.

    The construction of a #DataHashTable# requires a {\em hash function} that
    assigns every #HashItem# to an integer value.  Provided this, pairs of a
    #HashItem# and a #Info# can be added to the #DataHashTable#. No more
    than one #Info# to the same #HashItem# is possible at a time. The #Info#
    to a #HashItem# can be accessed through the subscript #operator[]# with
    #Info# as subscript.

    A #DataHashTable# can hold up to #max()# entries. This #max()# can be
    specified upon construction and may be reset with #reMax()# later on.
    Further, a value #hashSize# is required. This value must be #< max()# and must
    not have a common dominator with #max()#. If not specified explicitely, it
    is set automatically to a reasonable value. It may be reset with method
    #reHash()#. Note that both, #reMax()#ing and #reHash()#ing renders any
    reference to entries of a #DataHashTable# invalid. This also happens, if the
    #DataHashTable# is #reMax()#ed automatically, if more than #max()# entries
    are added to it.
*/
template <class HashItem, class Info>
class DataHashTable
{
private:
    /* The implementation relies on an array of #DataHashTable::Element#s, from
	now on referred to as elements. Upon construction, all elements are
	marked #FREE# in their member #status#. When an entry is added
	to the #DataHashTable#, the hash value is computed by calling #hashval#
	for its #HashItem#. If this array element is unused, it is
	taken right away. Otherwise, the array index is incremented by
	#hashsize# (modulo the element array #size()#) until an unused element
	is found.

	Removing elements is simply done by marking it as #RELEASED#. Hence,
	when searching for an element, the search loop may not stop, when a
	#RELEASED# element is encountered. However, such an element may be
	reused when adding a new element to the #DataHashTable#.

	Further, memory management with resizing of the element array is
	straight forward.
    */
    typedef DataHashTable_Element<HashItem, Info>	Element;
    McDArray< DataHashTable_Element<HashItem, Info> >	element;

    int		hashsize;
    int		thenum;				// current number of entries
    int		(*hashval)(const HashItem*);	// pointer to hash function
    double	factor;				// memory increment factor
    int		theCurrent;			// index for iterator.

    int		cFrontBug;			// #sizeof(Info)#

    /* Compute a good hashsize as the product of all prime numbers not dividors
       of size() that are <= the maximum dividor of size().
     */
    int autoHashSize() const
	{
	int	i, j;
	int	hashsze = 1;
	int	size    = element.size();
	McDArray<char>	prime(size);

	for(i = 2; i < size; ++i)
	    prime[i] = 1;

	for(i = 2; i < size; ++i)
	    if(prime[i])
		{
		for(j = i; j < size; j += i)
		    prime[j] = 0;
		if(size % i != 0)
		    {
		    hashsze *= i;
		    if(hashsze > size)
			{
			hashsze /= i;
			break;
			}
		    }
		}

	return hashsze;
	}

    // Return index in #element# to #h# or -1, if not present
    int	index(const HashItem& h) const
	{
	int	i, j;
	for(i = j = (*hashval)(&h) % element.size();
	    element[i].status != DataHashTable_Element<HashItem,Info>::FREE; )
	    {
	    if(element[i].item == h)
		return i;
	    i = (i + hashsize) % element.size();
	    if(i == j)
		break;
	    }
	return -1;
	}

public:
    /**@name Inquiry Methods */
    //@{
    /// number of elements that would fit
    int max() const
	{
	return element.size();
	}

    /// number of hashed elements
    int num() const
	{
	return thenum;
	}

    /// return hash size, i.e. ~the increment when searching for elements.
    int hashSize () const
	{
	return hashsize;
	}
    //@}

    /**@name Access Methods */
    //@{
    /// Is item #h# is present in #DataHashTable#?
    int	has(const HashItem& h) const
	{
	return index(h) >= 0 ? 1 : 0;
	}

    /// return pointer to #Info# to #h# or 0
    Info* get(const HashItem& h)
	{
	int i = index(h);
	return i >= 0 ? &element[i].info : 0;
	}

    /** return pointer to #Info# to #h# or 0
	@return pointer to #Info# component of #hash# element or zero pointer if
		element not in table.
    */
    const Info*	get(const HashItem& h) const
	{
	int i = index(h);
	return i >= 0 ? &element[i].info : 0;
	}

    /// reference #Info# of #h#
    Info& operator[](const HashItem& h)
	{
	return element[index(h)].info;
	}

    /** reference #Info# of #h#. Index operator for accessing the #Info#
	associated to #HashItem item#. It is required, that #h# belongs to the
	#DataHashTable#, otherwise it core dumps. Methods #has()# or #get()# can
	be used for inquiring wheater #h# belongs to the #DataHashTable# or not.
    */
    const Info& operator[](const HashItem& h) const
	{
	return element[index(h)].info;
	}
    //@}

    /**@name Iteration

       Often it is desired to loop though all elements in a #DataHashTable#.
       This is provided by means of the following 5 methods. They imply an
       arbitray order to all elements currently in the #DataHashTable#. This
       order may change after any non#const# member function invocation. When
       calling one of these methods, a maker is set that serves as reference
       point for the next call.
    */
    //@{

    /// return first #Item# in hash table and set marker to it
    const HashItem* first() const
	{
	*(int*)&theCurrent = -1;
	return next();
	}

    /// return last #Item# in hash table and set marker to it
    const HashItem* last() const
    	{
	*(int*)&theCurrent = element.size();
	return prev();
	}

    /// return #Item# following current marker thereby increasing marker
    const HashItem* next() const
    	{
	if(theCurrent < 0)
	*(int*)&theCurrent = -1;
	while(++*(int*)&theCurrent < element.size())
	    if(element[theCurrent].status ==
		DataHashTable_Element<HashItem,Info>::USED)
		return &element[theCurrent].item;
	*(int*)&theCurrent = -1;
	return 0;
	}

    /// return #Item# referenced by current marker
    const HashItem* current() const
	{
	return (theCurrent < 0) ? 0 : &element[theCurrent].item;
	}

    /// return #Item# preceding current marker thereby decreasing marker
    const HashItem* prev() const
    	{
	if(theCurrent > element.size())
	    *(int*)&theCurrent = element.size();
	while(--*(int*)&theCurrent >= 0)
	    if(element[theCurrent].status ==
		DataHashTable_Element<HashItem,Info>::USED )
		return &element[theCurrent].item;
	return 0;
	}
    //@}

    /**@name Manipulation Methods */
    //@{

    /** Add a new entry to hash table. Adds a new entry consisting of #h# and
	#x# to the #DataHashTable#. No entry to with #HashItem h# must yet be in
	the #DataHashTable#. After completion, #x# may be accessed via #get# or
	#operator[]# with #h# as parameter. The #DataHashTable# is #reMax()#ed
	if it becomes neccessary.
    */
    void add(const HashItem& h, const Info& x)
	{
	assert(!has(h));
	int i;
	if(thenum >= element.size())
	    reMax(int(factor * thenum) + 1);
	for(i = (*hashval)(&h) % element.size();
	    element[i].status == DataHashTable_Element<HashItem,Info>::USED;
	    i = (i+hashsize) % element.size())
	    ;
	element[i].status = DataHashTable_Element<HashItem,Info>::USED;
	memcpy(&(element[i].info), &x, cFrontBug);
	memcpy(&(element[i].item), &h, sizeof(HashItem));
	++thenum;
	}

    /// remove entry to #h#
    void remove(const HashItem& h)
	{
	assert(has(h));
	element[index(h)].status =
	    DataHashTable_Element<HashItem,Info>::RELEASED;
	--thenum;
	}

    /// remove all entries from #DataHashTable#
    void clear()
	{
	for(int i = element.size() - 1; i >= 0; --i)
	    element[i].status = DataHashTable_Element<HashItem,Info>::FREE;
	thenum = 0;
	}

    /** Reset the #max()# of a #DataHashTable# to #nel#. However, if
	#nel < num()#, it is resized to #num()# only. If #hashsze < 1# a new
	hash size is computed automatically. Otherwise, the specified value will
	be taken.
    */
    void reMax(int nel = -1, int hashsze = 0)
	{
	McDArray< DataHashTable_Element<HashItem, Info> > cpy(element);
	element.resize(nel < num() ? num() : nel);
	clear();
	if(hashsze < 1)
	    this->hashsize = autoHashSize();
	else
	    this->hashsize = hashsze;
	for(int i = cpy.size() - 1; i >= 0; --i)
	    if(cpy[i].status == DataHashTable_Element<HashItem, Info>::USED)
		add(cpy[i].item, cpy[i].info);
	}
    //@}

    /**@name Miscallaneous */
    //@{

    ///
    int	isConsistent() const
	{
	int	i, tot;

	for(i = element.size() - 1, tot = 0; i >= 0; --i)
	    if(element[i].status == DataHashTable_Element<HashItem, Info>::USED)
		{
		++tot;
		if(!has(element[i].item))
		    {
		    cout << "Inconsistency detected in class DataHashTable\n";
		    return 0;
		    }
		}

	if(tot != thenum)
	    {
	    cout << "Inconsistency detected in class DataHashTable\n"; 
	    return 0;
	    }
	return element.isConsistent();
	}

#ifdef DEFINE_OUTPUT_OPERATOR 
    /// Output operator, displays all elements currently contained in hash table
    friend ostream& operator << (ostream& out,
				 const DataHashTable<HashItem, Info>& h)
	{
	const HashItem*	item;
	for(item = h.first(); item; item = h.next())
	    out << "    " << *item << "\t\t" << h[*item] << endl;
	return out;
        }
#endif

    /** Default constructor. Allocates a #DataHashTable# for #nel# entries using
	#f# as hash function. If #hashsze > 0# #hashSize()# is set to the
	specified value, otherwise a suitable #hashSize()# is computed
	automatically. Parameter #incr# is used for memory management: If more
	than #nel# entries are added to the #DataHashTable#, it will
	automatically be #reMax()#ed by a factor of #incr#.
    */
    DataHashTable(
	/// pointer to hash function
	int	(*f)(const HashItem*),

	/// hash size
	int	nel      = 256,

	/// factor for increasing data block
	int	hashsze = 0,

        /// number of hash elements
	double	incr     = 2.0)
	: element(nel), hashval(f), factor(incr)
	{
	cFrontBug = sizeof(Info);
	clear();
	if(hashsze < 1)
	    this->hashsize = autoHashSize();
	else
	    this->hashsize = hashsze;
	assert(factor > 1);
	}
    //@}
};

#endif
