/*
  classgraph.cc

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
  License along with this library; if not, write to the Free
  Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include <assert.h>
#include <stdio.h>

#include "classgraph.h"

void ClassGraph::addAllChilds()
{
    addChilds("", 1);
}

void ClassGraph::addBases()
{
    int	i;
    int	first = 1;
    ClassGraph *base = 0;

    assert(entry);

    for(i = 0; i < entry->pubBaseclasses.size(); ++i)
	{
	base = new ClassGraph(entry->pubBaseclasses[i], indent - 1);
	base->nextLine = firstLine;
	if		// determine, wether this is the first one in child graph
	    (i == entry->pubBaseclasses.size() - 1	&&
	    entry->otherPubBaseclasses.size() == 0	&&
	    entry->proBaseclasses.size() == 0		&&
	    entry->otherProBaseclasses.size() == 0	&&
	    (withPrivate == 0 ||
		(entry->priBaseclasses.size() == 0	&&
		entry->otherPriBaseclasses.size() == 0)))
	    base->after += first ? "d" : "l";
	else
	    base->after += first ? "D" : "L";
	base->after += "_";
	base->addBases();
	firstLine = base->firstLine;
	first = 0;
	}

    for(i = 0; i < entry->otherPubBaseclasses.size(); ++i)
	{
	base = new ClassGraph(*(entry->otherPubBaseclasses[i]), indent - 1);
	base->nextLine = firstLine;
	if		// determine, wether this is the first one in child graph
	    (i == entry->otherPubBaseclasses.size() - 1	&&
	    entry->proBaseclasses.size() == 0		&&
	    entry->otherProBaseclasses.size() == 0	&&
	    (withPrivate == 0 ||
		(entry->priBaseclasses.size() == 0	&&
		entry->otherPriBaseclasses.size() == 0)))
	    base->after += first ? "d" : "l";
	else
	    base->after += first ? "D" : "L";
	base->after += "_";
	firstLine = base->firstLine;
	first = 0;
	}

    for(i = 0; i < entry->proBaseclasses.size(); ++i)
	{
	base = new ClassGraph(entry->proBaseclasses[i], indent - 1);
	base->nextLine = firstLine;
	if		// determine, wether this is the first one in child graph
	    (i == entry->proBaseclasses.size() - 1	&&
	    entry->otherProBaseclasses.size() == 0	&&
	    (withPrivate == 0 ||
		(entry->priBaseclasses.size() == 0	&&
		entry->otherPriBaseclasses.size() == 0)))
	    base->after += first ? "d" : "l";
	else
	    base->after += first ? "D" : "L";
	base->after += "-";
	base->addBases();
	firstLine = base->firstLine;
	first = 0;
	}

    for(i = 0; i < entry->otherProBaseclasses.size(); ++i)
	{
	base = new ClassGraph(*(entry->otherProBaseclasses[i]), indent - 1);
	base->nextLine = firstLine;
	if		// determine, wether this is the first one in child graph
	    (i == entry->otherProBaseclasses.size() - 1	&&
	    (withPrivate == 0 ||
		(entry->priBaseclasses.size() == 0	&&
		entry->otherPriBaseclasses.size() == 0)))
	    base->after += first ? "d" : "l";
	else
	    base->after += first ? "D" : "L";
	base->after += "-";
	firstLine = base->firstLine;
	first = 0;
	}

    if(withPrivate)
	{
	for(i = 0; i < entry->priBaseclasses.size(); ++i)
	    {
	    base = new ClassGraph(entry->priBaseclasses[i], indent - 1);
	    base->nextLine = firstLine;
	    if		// determine, wether this is the first one in child graph
		(i == entry->priBaseclasses.size() - 1	&&
		entry->otherPriBaseclasses.size() == 0)
		base->after += first ? "d" : "l";
	    else
		base->after += first ? "D" : "L";
	    base->after += "-";
	    base->addBases();
	    firstLine = base->firstLine;
	    first = 0;
	    }

	for(i = 0 ; i < entry->otherPriBaseclasses.size(); ++i)
	    {
	    base = new ClassGraph(*(entry->otherPriBaseclasses[i]), indent - 1);
	    base->nextLine = firstLine;
	    // determine, wether this is the first one in child graph
	    if(i == entry->otherPriBaseclasses.size() - 1)
		base->after += first ? "d" : "l";
	    else
		base->after += first ? "D" : "L";
	    base->after += "-";
	    firstLine = base->firstLine;
	    first = 0;
	    }
	}

    if(base)
	{
	ClassGraph *cls;

	for(cls = base->firstLine; cls != base; cls = cls->nextLine)
	    cls->after += "SP";
	for(; cls != this; cls = cls->nextLine)
	    if(cls->indent < indent - 1)
		cls->after += "||";
	}

    int	min = indent;
    for(base = firstLine; base; base = base->nextLine)
	if(base->indent < min)
	    min = base->indent;
    for(base = firstLine; base; base = base->nextLine)
	base->indent -= min;
}

void ClassGraph::addChilds(const char *strt, bool recursive)
{
    ClassGraph*	current = this;
    ClassGraph*	child;
    int		i;
    int		first = 1;
    Entry*	last;

    if(withPrivate && entry->priChilds.size())
	last = entry->priChilds[entry->priChilds.size() - 1];
    else
	if(entry->proChilds.size())
	    last = entry->proChilds[entry->proChilds.size() - 1];
	else
	    if(entry->pubChilds.size())
		last = entry->pubChilds[entry->pubChilds.size() - 1];
	    else
		return;

    for(i = 0; i < entry->pubChilds.size(); ++i)
	{
	child = new ClassGraph(entry->pubChilds[i], indent);
	child->nextLine = current->nextLine;
	current->nextLine = child;
	current = child;
	child->before += strt;
	if(first)
	    {
	    if(entry->pubChilds[i] == last)
		child->before += "^";
	    else
		child->before += "|";
	    }
	else
	    {
	    if(entry->pubChilds[i] != last)
		child->before += "R";
	    else
		child->before += "r";
	    }
	child->before += "_";
	first = 0;
	}

    for(i = 0; i < entry->proChilds.size(); ++i)
	{
	child = new ClassGraph(entry->proChilds[i], indent);
	child->nextLine = current->nextLine;
	current->nextLine = child;
	current = child;
	child->before += strt;
	if(first)
	    {
	    if(entry->proChilds[i] == last)
		child->before += "^";
	    else
		child->before += "|";
	    }
	else
	    {
	    if(entry->proChilds[i] != last)
		child->before += "R";
	    else
		child->before += "r";
	    }
	child->before += "-";
	first = 0;
	}

    if(withPrivate)
	{
	for(i = 0; i < entry->priChilds.size(); ++i)
	    {
	    child = new ClassGraph(entry->priChilds[i], indent);
	    child->nextLine = current->nextLine;
	    current->nextLine = child;
	    current = child;
	    child->before += strt;
	    if(first)
		{
		if(entry->priChilds[i] == last)
		    child->before += "^";
		else
		    child->before += "|";
		}
	    else
		{
		if(entry->priChilds[i] != last)
		    child->before += "R";
		else
		    child->before += "r";
		}
	    child->before += ".";
	    first = 0;
	    }
	}

    if(recursive)
	{
	McString start(strt);
	start += "||";
	ClassGraph *next = this->nextLine;
	do
	    {
	    child = next;
	    next  = child->nextLine;
	    if(child == current)
		{
		start = strt;
		start += "SP";
		}
	    child->addChilds(start.c_str(), recursive);
	    }
	while(child != current);
	}
}

void ClassGraph::addDirectChilds()
{
    addChilds("", 0);
}
