/*
  McHashTable.h

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

#ifndef _MC_HASH_TABLE_H
#define _MC_HASH_TABLE_H

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/** @name Predefined hash and compare methods. */
//@{

/// Hash function for character strings.
extern int hash(const char *str);

/// Hash function for integers.
extern int hash(int i);

/// Comparison function for character strings.
extern int compare(const char *str1, const char *str2);

/// Comparison function for integers.
extern int compare(int i1, int i2);

//@}

/** Dictionary implemented by hashing with chaining.

    This data type maintains a set of key-value pairs. It provides a
    very simple interface via the index operator. Both key and value
    of a hash entry are copied into the internal data-structure. The
    user has not to bother with an extra hash entry data type. However,
    this also involves dynamic memory allocation on each insertion,
    which might impair performance.

    Any data type or class can be used as a key or as a value as long
    as a public copy constructor and a public assignment operator are
    provided. Hashing and comparsion relies on the existance of global
    functions #hash()# and #compare()#. For character strings and integer
    both type of functions are already provided.

    To reduce the costs implied by copying large data objects you may
    use appropriate pointers as key or value types. Preferable these
    should be smart pointers as provided by \Ref{McHandle}.

    NOTE: `calloc' is used so that initial buckets are all 0. It isn't
    clear this has an advantage over `new' other than zeroing things
    automatically.
*/

template<class Key, class Value> class McHashTable
{
public:
    /// Constructor takes default value for has entries.
    McHashTable(const Value& defaultValue);

    /// Destructor.
    ~McHashTable();

    /// Number of items in hash table.
    int size() const
	{
	return nItems;
	}

    /** Read-write access via index operator.

	If the given key does not already exist, a new table entry is
	created. The value of this entry is initialzed with the default
	value specified in the constructor.
    */
    inline Value& operator[](const Key& key)
	{
	return *insert(key);
	}

    /** Inserts or updates key-value pair.

	If the given key does not already exist, a new table entry is
	created. The value of this entry is initialized with the second
	argument. If the key does already exist its value is overidden.
	The return value indicates if a new entry has been created (1)
	or if an existing one has been used (0).
    */
    int insert(const Key& key, const Value& val);

    /// Inserts default value if key doesn't exist.
    Value* insert(const Key&);

    /// Returns pointer to associated value if key does exist, 0 otherwise.
    Value* lookup(const Key& key)
	{
	Item* item = bucketList[hash(key) & mask];
	while(item)
	    {
	    if(compare(item->key, key) == 0)
		return &item->value;
	    item = item->next;
	    }
	return 0;
	}

    /// Removes key and associated value from hash table.
    int remove(const Key& key);

    /// Resets iterator to first key-value pair.
    void resetIter()
	{
	iterIdx = 0;
	iterItem = bucketList[0];
	}

    /// Successively returns all key-value pairs.
    int next(Key& key, Value& value);

    /// Print out some statistics (for debugging only).
    void printStat();

protected:

    struct Item
	{
	Key key;
	Value value;
	Item *next;
	Item(const Key& k, const Value& v) : key(k), value(v), next(0)
	    {
	    }
	};

    void rebuildTable();   // Adjusts size of bucket array.

    int     mask;	   // Mask value used to truncate hash index
    int     nItems;	   // Number key-value pairs stored
    Item**  bucketList;    // Pointer to bucket array
    Item*   staticList[2]; // Static array initially avoids malloc()
    int     iterIdx;	   // Bucket index used for iteration
    Item*   iterItem;      // Current item in iteration
    Value   defaultValue;  // Should this be static or a pointer ?
};

template<class Key, class Value>
McHashTable<Key,Value>::McHashTable(const Value& val) : defaultValue(val)
{
    bucketList = staticList;
    staticList[0] = staticList[1] = 0;
    mask = 1;
    nItems = 0;
}

template<class Key, class Value>
McHashTable<Key,Value>::~McHashTable()
{
    for(int i = 0; i <= mask; i++)
	{
	Item* item = bucketList[i];
	while(item)
	    {
	    Item *next = item->next;
	    delete item;
	    item = next;
	    }
	}
    if(bucketList != staticList)
	free(bucketList);
}

template<class Key, class Value>
int McHashTable<Key,Value>::insert(const Key& key, const Value& value)
{
    int i = hash(key) & mask;
    Item* item = bucketList[i];

    while(item)
	{
	if(compare(item->key, key) == 0)
	    {
	    item->value = value;
	    return 0;
	    }
	item = item->next;
	}

    item = new Item(key, value);
    item->next = bucketList[i];
    bucketList[i] = item;
    nItems++;

    if(nItems > 2 * mask + 1)
	rebuildTable();
    return 1;
}

template<class Key, class Value>
Value* McHashTable<Key,Value>::insert(const Key& key)
{
    int i = hash(key) & mask;
    Item *item = bucketList[i];

    while(item)
	{
	if(compare(item->key, key) == 0)
	    return &item->value;
	item = item->next;
	}

    item = new Item(key, defaultValue);
    item->next = bucketList[i];
    bucketList[i] = item;
    nItems++;

    if(nItems > 2 * mask + 1)
	rebuildTable();
    return &item->value;
}

template<class Key, class Value>
int McHashTable<Key,Value>::remove(const Key& key)
{
    int i = hash(key) & mask;
    Item* item = bucketList[i];

    if(item == 0)
	return 0;

    if(compare(item->key, key) == 0)
	{
	Item *next = item->next;
	delete item;
	bucketList[i] = next;
	nItems--;
	return 1;
	}

    while(item->next)
	{
	if(compare(item->next->key, key) == 0)
	    {
	    Item* next = item->next->next;
	    delete item->next;
	    item->next = next;
	    nItems--;
	    return 1;
	    }
	item = item->next;
	}
    return 0;
}

template<class Key, class Value>
void McHashTable<Key,Value>::rebuildTable()
{
    Item **oldList = bucketList;
    int size = mask + 1;
    mask = (mask << 1) + 1;
    bucketList = (Item **)calloc(2 * size, sizeof(Item));

    for(int i = 0; i < size; i++)
	{
	Item *item = oldList[i];
	while(item)
	    {
	    int k = hash(item->key) & mask;
	    Item* next = bucketList[k];
	    Item* oldNext = item->next;
	    bucketList[k] = item;
	    item->next = next;
	    item = oldNext;
	    }
	}

    if(oldList != staticList)
	free(oldList);
}

template<class Key, class Value>
int McHashTable<Key,Value>::next(Key& key, Value& value)
{
    while(1)
	{
	while(iterItem)
	    {
	    key = iterItem->key;
	    value = iterItem->value;
	    iterItem = iterItem->next;
	    return 1;
	    }
	if(++iterIdx > mask)
	    {
	    iterItem = bucketList[iterIdx = 0];
	    return 0;
	    }
	iterItem = bucketList[iterIdx];
	}
}

template<class Key, class Value>
void McHashTable<Key,Value>::printStat()
{
    int i, total = 0, noZero = 0, count[10];

    for(i = 0; i < 10; i++)
	count[i] = 0;

    for(i = 0; i <= mask; i++)
	{
	int n = 0;
	Item *item = bucketList[i];
	while(item)
	    {
	    n++;
	    item = item->next;
	    }
	if(n < 10)
	    count[n]++;
	if(n)
	    {
	    total += n;
	    noZero++;
	    }
	}

    printf("Hash table contains %d buckets and %d items\n", mask + 1, nItems);
    for(i = 0; i < 10; i++)
	printf("%3d buckets with %d entries\n", count[i], i);
    if(noZero)
	printf("Average search distance is %.1f\n", (float)total / noZero);
}
#endif
