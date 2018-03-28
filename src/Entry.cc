/*
  Entry.cc

  Copyright (c) 1996 Roland Wunderling, Malte Zoeckler
  Copyright (c) 1998 Michael Meeks
  Copyright (c) 1999-2001 Dragos Acostachioaie

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

#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

#include "doc.h"

#define max(a, b) ((a) > (b) ? (a) : (b))

#define MAX_CACHE	5

// Reserved keywords
char cxxreserved[] = "bool char class const double enum export extern float"
		     "friend inline int long namespace operator private"
                     "protected public register short signed static struct"
		     "throw typedef union unsigned using void volatile virtual";
char javareserved[]= "boolean byte char class double extends final float"
		     "implements import int interface long native package"
		     "private protected public short static synchronized throws"
		     "transient volatile";
char idlreserved[] = "any attribute boolean case char const context default"
		     "double enum exception FALSE fixed float in inout"
		     "interface long module Object octet oneway out raises"
		     "readonly sequence short string struct switch TRUE typedef"
		     "unsigned union void wchar wstring";
char phpreserved[] = "class extends function include require";

static McHashTable<const char *, char *> *cxxtable;
static McHashTable<const char *, char *> *javatable;
static McHashTable<const char *, char *> *idltable;
static McHashTable<const char *, char *> *phptable;

static McHashTable<const char*, Entry*>	namespace_roots(0);

// Simple round robin cache for `fastNotSmall == true'
static int cache_pos = 0;
// So we can remember blanks as well
static char *cache_name[MAX_CACHE];
static Entry *cache_entry[MAX_CACHE];

// Don't want general bits
#define worthDoing(m) !m->general

void makeSubLists(Entry *rt);
Entry *findJavaPackage(McString n, Entry *root, Entry *p = 0);
Entry *findJavaClass(McString n, Entry *root);
bool isIt(const McString& n, const Entry *m);
static McString* get1RefName(McString &str, int start, int end);
unsigned int countCommas(McString const &args);
void separateArguments(char const *args, McDArray<McString *> &arg_list);
bool equalTypes(McString const &a, McString const &b);
bool equalArgs(McString const &a, McString const &b);
bool equalSignature(Entry *a, Entry *b);
Entry *lookupEntryBySignature(Entry *group, Entry *model);
bool hasDocumentation(Entry const *entry);
void mergeDocumentation(Entry *original, Entry *ccentry);
bool mergeEntry(Entry *ccentry);
void mergeEntries(Entry *root);
void getRefNames(McDArray<McString*> strings, const char *names);

void addNamespace(Entry* entry)
{
    Entry**	e = namespace_roots.lookup(entry->fullName.c_str());

    if(e == 0)
        namespace_roots[entry->fullName.c_str()] = entry;
}

Entry::Entry()
{
    static int n = 1;

    number = n++;
    subnumber = 0;

    protection    = PUBL;
    section       = EMPTY_SEC;
    file	  = _("No file");
    parent        = 0;
    next          = 0;
    sub           = 0;
    ownPage       = 0;
    override      = 0;

    currentParent = 0;
    htype         = 0;
    hname         = 0;
    hargs         = 0;
    hmemo         = 0;
    hsee          = 0;
    hdoc          = 0;
    line          = 0;
    general	  = 0;
    pureVirtual	  = false;
    fromUnknownPackage = false;
    docify = !onlyDocs;
}

void Entry::addBaseClass(Entry *base, PROTECTION state)
{
    Entry *tmp2 = this;

    if(state == PUBL)
	{
	pubBaseclasses.append(base);
	base->pubChilds.insert(0, 1, &tmp2);
	}
    else
	if(state == PROT)
	    {
	    proBaseclasses.append(base);
	    base->proChilds.insert(0, 1, &tmp2);
	    }
	else
	    {
	    priBaseclasses.append(base);
	    base->priChilds.insert(0, 1, &tmp2);
	    }
    baseclasses.append(base);
}

void Entry::addBaseClass(const char *classname, PROTECTION state)
{
    Entry *tmp = 0, *p;
    McString t, *str;

    if(language == LANG_JAVA)
	{
	t = classname;
	makeFullName(t);
	tmp = findJavaClass(t, root);
	}
    else
	{
	p = (parent != 0) ? parent : this;
	tmp = getRefEntry(classname, p);
	}

    if(tmp && ((tmp->section & CLASS_SEC) || (tmp->section & INTERFACE_SEC)))
	addBaseClass(tmp, state);
    else 
	{
	str = new McString(classname);

	if(state == PUBL)
	    otherPubBaseclasses.append(str);
	else
	    if(state == PROT)
		otherProBaseclasses.append(str);
	    else 
		otherPriBaseclasses.append(str);
	}
}

void Entry::addSubEntry(Entry *current)
{
    int	i = 1;
    Entry **e = &sub;

    sublist.append(current);
    for(; *e; e = &((*e)->next), ++i)
	;
    *e = current;
    current->parent = this;
    current->next = 0;
    current->subnumber = i;
    current->makeFullName();
}

void Entry::clear()
{
    type.clear();
    name.clear();
    fullName.clear();
    args.clear();
    memo.clear();
    doc.clear();
    program.clear();
}

void Entry::dump(FILE *foo, bool recursive)
{
#ifdef DEBUG
    int i;

    fprintf(foo, "--------------------------Start-%p-----------------------------\n",
	this);
    fprintf(foo, "section %d protection `%c'\n", section, protection);
    fprintf(foo, "parent %p next %p sub %p\n", parent, next, sub);
    fprintf(foo, "type `%s'\n", type.c_str());
    fprintf(foo, "name `%s'\n", name.c_str());
    fprintf(foo, "args `%s'\n", args.c_str());
    fprintf(foo, "memo `%s'\n", memo.c_str());
    fprintf(foo, "doc `%s'\n", doc.c_str());
    fprintf(foo, "file `%s'\n", file.c_str());
    fprintf(foo, "fullName `%s'\n", fullName.c_str());
    fprintf(foo, "author `%s' version `%s'\n", author.c_str(),
	version.c_str());

    if(see.size())
        fprintf(foo, "see:\n");
    for(i = 0; i < see.size(); i++)
	fprintf(foo, "    `%s'\n", see[i]->c_str());

    if(param.size())
        fprintf(foo, "param:\n");
    for(i = 0; i < param.size(); i++)
	fprintf(foo, "    `%s'\n", param[i]->c_str());

    if(exception.size())
	fprintf(foo, "exception:\n");
    for(i = 0; i < exception.size(); i++)
	fprintf(foo, "    `%s'\n", exception[i]->c_str());

    if(htype)
	fprintf(foo, "htype `%s'", htype);
    if(hname)
	fprintf(foo, "hname `%s'", hname);
    if(hargs)
	fprintf(foo, "hargs `%s'\n", hargs);

    if(hmemo)
	fprintf(foo, "hmemo `%s'", hmemo);
    if(hsee)
	fprintf(foo, "hsee `%s'", hsee);
    if(hdoc)
	fprintf(foo, "hdoc `%s'\n", hdoc);

    fprintf(foo, "fileName `%s' line %d general %d ownPage %d\n",
	fileName.c_str(), line, general, ownPage);
    fprintf(foo, "number %d subnumber %d\n", number, subnumber);

    if(sublist.size())
	{
	fprintf(foo, "sublist:\n");
	for(i = 0; i < sublist.size(); i++)
	if(recursive)
	    sublist[i]->dump(foo, recursive);
	else
	    fprintf(foo, "        `%s'\n", sublist[i]->name.c_str());
	}

    fprintf(foo,"----------------------------End-%p-----------------------------\n",
	this);
#endif
}

void Entry::dumpNumber(McString &s)
{
    static char buf[33];

    if(parent)
	{
	parent->dumpNumber(s);
	sprintf(buf, "%d", subnumber);
	if(parent->parent)
	    {
	    s += '.';
	    s += buf;
	    }
	else
	    s += buf;
	}
}

void Entry::dumpNumber(FILE *f)
{
    McString num;

    dumpNumber(num);
    fprintf(f, "%s", num.c_str());
}

// This assumes they are sorted, and binary chop searches ?
/*
Entry *Entry::findSub(const McString& name, int first, int last)
{
    int i;

    if(last - first < 4)	// I assume this is to catch different adjacent duplicates
	{
	for(i = first + 1; i <= last - 1; i++)
	    if(isIt(name, sublist[i]))
		return sublist[i];
        return 0;
	}

    int n = (first + last) / 2;
    int result = strcmp(name.c_str(), sublist[n]->name.c_str());

    if(result == 0)
	return sublist[n];
    if(result > 0)
	return findSub(name, n, last);

    return findSub(name, first, n);
}
*/

// This routine is called `A HELL OF A LOT' (tm), and needs keeping small
Entry *Entry::findSub(const McString& n)
{
    int i, max;

    if(n == name)
	return this;

// For some reason, the binary search fails, so let's fall down to a simple
// linear search, just to be safe
    for(i = 0; i < sublist.size(); i++)
	if(isIt(n, sublist[i]))
	    return sublist[i];

/*
    if(sublist.size())
	{
	max = sublist.size() - 1;
	if(isIt(name, sublist[0]))
	    return sublist[0];
	if(isIt(name, sublist[max]))
	    return sublist[max];
	Entry *found = findSub(name, 0, max);
	if(found)
	    return found;
	}
*/

    for(i = sublist.size(); i-- > 0; )
	if(sublist[i]->sub)
	    {
	    Entry *found = sublist[i]->findSub(n);
	    if(found)
		return found;
	    }

    for(i = 0; i < sublist.size(); i++)
	if(worthDoing(sublist[i]) && n == sublist[i]->name)
		return sublist[i];

    return 0;
}

void Entry::getPackage(McString &n)
{
    Entry *tmp = this;

    n = "";
    while(tmp)
	{
	if(tmp->section == PACKAGE_SEC)
	    {
	    if(n.length() > 0)
		n.insert(0, ".");
	    n.insert(0, tmp->name.c_str());
	    }
	tmp = tmp->parent;
	}    
}

bool Entry::isClass()
{
    return((section & CLASS_SEC) || (section & INTERFACE_SEC));
}

void Entry::makeFullName(McString &n)
{
    int i, p, ri;

#ifdef DEBUG
    if(verb)
        printf(_("Make full name from `%s'\n"), n.c_str());
#endif

    // If `n' is already a fully qualified name, just return
    if((p = n.index('.')) >= 0)
	{
    	McString innerName(n, p + 1, n.length() - p - 1);
	McString className(n, 0, p);

	if(innerName.index('.') >= 0)
	    return;
	else
	    {
	    const char *cstr = className.c_str();

	    if(!isupper(*cstr))
		return;

	    makeFullName(className);

	    n = className;
	    n += ".";
	    n += innerName;

	    return;
	    }
	}

    // Search in the list of import statements
    for(i = 0; i < import.size(); i++)
	{
	ri = import[i]->rindex('.');
	if(ri >= 0)
	    if(strcmp(n.c_str(), (char *)(*(import[i])) + ri + 1) == 0)
		{
		n = *(import[i]);
		return;
		}
	}

    McString thispkg;
    getPackage(thispkg);
    if(thispkg.length())
	thispkg += '.';

    // Search the documented packages
    McString tmp(thispkg);
    tmp += n;
    Entry *found = findEntry(root, tmp.c_str(), CLASS_SEC | INTERFACE_SEC);
    for(i = 0; i < import.size() && !found; i++)
	{
	ri = import[i]->rindex('*');
	if(ri >= 0)
	    {
	    tmp = *(import[i]);
	    tmp.resize(ri);
	    if(tmp.length())
	    	tmp += '.';

	    tmp += n;
	    found = findEntry(root, tmp.c_str(), CLASS_SEC | INTERFACE_SEC);
	    }
	}
    if(found)
	{
	n = found->fullName;
	return;
	}

    // Try to find it in java.lang first
    static McHashTable<const char *, int> *jlang = NULL;
    if(!jlang)
	{
	// We have to initialize a hash that contains java.lang classes.
	jlang = new McHashTable<const char *, int>(CLASS_SEC);
	// The names were taken from Java 1.3. (Feb 2000)
	// Interfaces
	jlang->insert("Cloneable", INTERFACE_SEC);
	jlang->insert("Comparable", INTERFACE_SEC);
	jlang->insert("Runnable", INTERFACE_SEC);
	// Classes
	// Final classes (String) are not required here.
	// Character.Subset and Character.UnicodeBlock omitted.
	jlang->insert("ClassLoader");
	jlang->insert("InheritableThreadLocal");
	jlang->insert("Number");
	jlang->insert("Object");
	jlang->insert("Package");
	jlang->insert("Process");
	jlang->insert("Runtime");
	jlang->insert("SecurityManager");
	jlang->insert("Thread");
	jlang->insert("ThreadGroup");
	jlang->insert("ThreadLocal");
	jlang->insert("Throwable");
	// Exceptions  
	jlang->insert("ArithmeticException");
	jlang->insert("ArrayIndexOutOfBoundsException");
	jlang->insert("ArrayStoreException");
	jlang->insert("ClassCastException");
	jlang->insert("ClassNotFoundException");
	jlang->insert("CloneNotSupportedException");
	jlang->insert("Exception");
	jlang->insert("IllegalAccessException");
	jlang->insert("IllegalArgumentException");
	jlang->insert("IllegalMonitorStateException");
	jlang->insert("IllegalStateException");
	jlang->insert("IllegalThreadStateException");
	jlang->insert("IndexOutOfBoundsException");
	jlang->insert("InstantiationException");
	jlang->insert("InterruptedException");
	jlang->insert("NegativeArraySizeException");
	jlang->insert("NoSuchFieldException");
	jlang->insert("NoSuchMethodException");
	jlang->insert("NullPointerException");
	jlang->insert("NumberFormatException");
	jlang->insert("RuntimeException");
	jlang->insert("SecurityException");
	jlang->insert("StringIndexOutOfBoundsException");
	jlang->insert("UnsupportedOperationExceptio");
	// Errors
	jlang->insert("AbstractMethodError");
	jlang->insert("ClassCircularityError");
	jlang->insert("ClassFormatError");
	jlang->insert("Error");
	jlang->insert("ExceptionInInitializerError");
	jlang->insert("IllegalAccessError");
	jlang->insert("IncompatibleClassChangeError");
	jlang->insert("InstantiationError");
	jlang->insert("InternalError");
	jlang->insert("LinkageError");
	jlang->insert("NoClassDefFoundError");
	jlang->insert("NoSuchFieldError");
	jlang->insert("NoSuchMethodError");
	jlang->insert("OutOfMemoryError");
	jlang->insert("StackOverflowError");
	jlang->insert("ThreadDeath");
	jlang->insert("UnknownError");
	jlang->insert("UnsatisfiedLinkError");
	jlang->insert("UnsupportedClassVersionError");
	jlang->insert("VerifyError");
	jlang->insert("VirtualMachineError");
	}
    if(int *i = jlang->lookup(n.c_str()))
	{
	n.insert(0, "java.lang.");
	Entry *tmp = findJavaClass(n, root);
	if(tmp)
	    tmp->section = *i;
	return;
	}

    // Assume it's in the first completely imported package
    for(i = 0; i < import.size(); i++)
	{
	ri = import[i]->rindex('*');
	if(ri >= 0)
	    {
	    McString tmp(*(import[i]));
	    tmp[ri] = '\0';
	    n.insert(0, tmp.c_str());
	    if(verb)
		printf("  Guessed package: %s\n", n.c_str());
	    return;
	    }
	}

    // OK, assume that its the same package!
    n.insert(0, thispkg.c_str());
#ifdef DEBUG
    if(verb)
        printf("	-> `%s'\n", n.c_str());
#endif
}

void Entry::makeFullName()
{
    fullName.clear();
    if(parent != 0 && parent->fullName.length() > 0)
	{
	fullName = parent->fullName;
	if(language == LANG_CXX)
	    fullName += "::";
	else
    	    fullName += ".";
	}
    fullName += name;
}

void Entry::makeRefs()
{
    McString tmp;
    int i;

    if(language == LANG_JAVA)
	{
	if((section & CLASS_SEC) || (section & INTERFACE_SEC))
	    {
	    for(i = 0; i < implements.size(); i++)
		{
	    	tmp = *(implements[i]);
		makeFullName(tmp);
		addBaseClass(tmp.c_str(), PUBL);
		}
	    for(i = 0; i < extends.size(); i++)
		{
	        tmp = *(extends[i]);
		makeFullName(tmp);	      
		addBaseClass(tmp.c_str(), PUBL);
		}	  
	    }
	}
    else
	if(language == LANG_PHP)
	    {
	    if((section & CLASS_SEC) || (section & INTERFACE_SEC))
		for(i = 0; i < extends.size(); i++)
		    {
		    tmp = *(extends[i]);
		    addBaseClass(tmp.c_str(), PUBL);
		    }	  
	    }
	else
	    if((section & CLASS_SEC) || (section & INTERFACE_SEC))
		findBases();

    for(i = 0; i < sublist.size(); ++i)
	sublist[i]->makeRefs();
}

void Entry::makeSubList()
{
    Entry *tmp;
    int i = 0;

    sublist.clear();
    for(tmp = sub; tmp; tmp = tmp->next)
	++i;

    if(i)
	{
	sublist.remax(i);
	for(tmp = sub; tmp; tmp = tmp->next)
	    {
	    sublist.append((Entry *)0);
	    for(i = sublist.size() - 1; i > 0; --i)
		{
		if(sublist[i - 1]->name <= tmp->name)
		    break;
		sublist[i] = sublist[i - 1];
		}
	    sublist[i] = tmp;
	    }

	/* the sublist items are sorted, but there seem to be rather
	   lots of accesses just by traversing via tmp->next, so it's
	   a quick fix to sort the tmp-entries themselves... */

	/* now everything is sorted out, so let's comment out the code. This
	   could be useful, for example, to make the `--sort' option usable
	   for any output format, not only for HTML
	*/
/*
	for(i = 0; i < sublist.size() - 1; i++)
	    sublist[i]->next = sublist[i + 1];

	sublist[i]->next = (Entry *)0;
	sub = sublist[0];
*/
	}
}

Entry *Entry::newSubEntry()
{
    Entry *newentry = new Entry;

    addSubEntry(newentry);

    return newentry;
}

void Entry::removeSub(Entry *e)
{
    Entry *tmp;
    int i;

    for(i = 0; i < sublist.size(); i++)
	if(sublist[i] == e)
	    {
	    sublist.remove(i);
	    break;
	    }

    if(sub == e)
	sub = e->next;
    else
	for(tmp = sub; tmp->next; tmp = tmp->next)
	    if(tmp->next == e)
		{
		tmp->next = e->next;
		break;
		}
}

void setupLanguageHash()
{
    char *st, *tmp;
    int lp;

    // C/C++
    st = cxxreserved;
    tmp = st;
    cxxtable = new McHashTable<const char *, char *>("");
    while(*tmp)
	if(*tmp == ' ')
	    {
	    *tmp = 0;
	    cxxtable->insert(st, "1");
	    st = ++tmp;
	    }
	else
	    tmp++;

    // Java
    st = javareserved;
    tmp = st;
    javatable = new McHashTable<const char *, char *>("");
    while(*tmp)
	if(*tmp == ' ')
	    {
	    *tmp = 0;
	    javatable->insert(st, "1");
	    st = ++tmp;
	    }
	else
	    tmp++;

    // IDL
    st = idlreserved;
    tmp = st;
    idltable = new McHashTable<const char *, char *>("");
    while(*tmp)
	if(*tmp == ' ')
	    {
	    *tmp = 0;
	    idltable->insert(st, "1");
	    st = ++tmp;
	    }
	else
	    tmp++;

    // PHP
    st = phpreserved;
    tmp = st;
    phptable = new McHashTable<const char *, char *>("");
    while(*tmp)
	if(*tmp == ' ')
	    {
	    *tmp = 0;
	    phptable->insert(st, "1");
	    st = ++tmp;
	    }
	else
	    tmp++;

    // Setup cache as well
    for(lp = 0; lp < MAX_CACHE; lp++)
	{
	cache_entry[lp] = 0;
	cache_name[lp] = 0;
	}
}

void makeSubLists(Entry *rt)
{
    int i;

    rt->makeSubList();
    for(i = rt->sublist.size(); --i >= 0; )
	makeSubLists(rt->sublist[i]);
}

Entry *findJavaPackage(McString n, Entry *root, Entry *p)
{
    Entry *tmp, *newPackage, *result = 0;
    int i = n.index('.'), j;
    McString *pkgName;

    if(!root || language == LANG_PHP)
	return 0;

    if(root->section == EMPTY_SEC)
	{
	p = root;
	root = root->sub;
	}

    if((root->section != PACKAGE_SEC) && (root->section != EMPTY_SEC) &&
	!(root->section & CLASS_SEC))
	return 0;

    if(i > -1)
	{
	for(tmp = root; tmp && !result; tmp = tmp->next)
	    if((tmp->section == PACKAGE_SEC) || (tmp->section & CLASS_SEC))
		{
		i = tmp->name.length();

		if(i <= n.length())
		    {
		    pkgName = new McString(n, 0, i);

		    if(tmp->name == *pkgName)
			{
		    	n.remove(0, i + 1);

		    	if(n.length() < 1)
			    result = tmp;
		    	else
	    		    result = findJavaPackage(n, tmp->sub, tmp);
			}
		    }
		}
    	}
    else
	for(tmp = root; tmp && !result; tmp = tmp->next)
	    if((tmp->section == PACKAGE_SEC) || (tmp->section & CLASS_SEC))
		if(tmp->name == n)
		    result = tmp;

    if(!result)
	{
	i = -1;

	do
	    {
	    j = n.index('.', i + 1);
	    newPackage = p->newSubEntry();

	    if(j != -1)
		newPackage->name = *(new McString(n, 0, j));
	    else
		newPackage->name = n;

	    newPackage->section = PACKAGE_SEC;
	    newPackage->fromUnknownPackage = true;

	    p = newPackage;
	    n.remove(0, j + 1);
	    }
	while((j != -1) && (n.length() > 0));

	result = newPackage;
	}

    return result;
}

Entry *findJavaClass(McString n, Entry *root)
{
    Entry *pkg;
    Entry *tmp;
    Entry *result = NULL;
    McString *package;
    int i = n.rindex('.');

    if(i != -1)
	package = new McString(n, 0, i);
    else
	package = &n;

    pkg = findJavaPackage(*package, root);

    if(pkg != NULL)
	{
	if(i != -1)
	    n.remove(0, i + 1);

	for(tmp = pkg->sub; tmp && (result == NULL); tmp = tmp->next)
	    if((tmp->section & CLASS_SEC) || (tmp->section & INTERFACE_SEC))
		if(tmp->name == n)
		    result = tmp;

	if(!result)
	    {
	    result = pkg->newSubEntry();
	    result->name = n;
	    result->section = CLASS_SEC;
	    result->makeFullName();
	    result->fromUnknownPackage = true;
	    }
	}

    return result;
}

bool isIt(const McString& n, const Entry* m)
{
    if(worthDoing(m))
	return (n == m->fullName);

    return false;
}

static McString *get1RefName(McString& str, int start, int end)
{
    McString *s = 0;

    while((str[start] == ' ' || str[start] == '\t') && start < end)
	start++;
    while((str[end] == ' ' || str[end] == '\t' || str[end] == ',') && start < end)
	end--;

    if(start < end)
	s = new McString(str, start, end);

    return s;
}

bool equalTypes(McString const &a, McString const &b)
{
    char const *type_a = (char const *)a;
    char const *type_b = (char const *)b;

    if(strncmp(type_a, "inline", 6) == 0)
	type_a += 6;
    if(strncmp(type_a, "static", 6) == 0)
	type_a += 6;
    if(strncmp(type_a, "virtual", 7) == 0)
	type_a += 7;

    if(strncmp(type_b, "inline", 6) == 0)
	type_b += 6;
    if(strncmp(type_b, "static", 6) == 0)
	type_b += 6;
    if(strncmp(type_b, "virtual", 7) == 0)
	type_b += 7;

    while(*type_a == ' ')
	type_a++;
    while(*type_b == ' ')
	type_b++;

    return(strcmp(type_a, type_b) == 0);
}

unsigned int countCommas(McString const &args)
{
    unsigned int count = 0;
    int i;

    for(i = 0; i < args.size() && args[i] != 0; i++)
	if(args[i] == ',')
	    count++;

    return count;
}

void separateArguments(char const *args, McDArray<McString *> &arg_list)
{
    McString *s;
    unsigned int start, end, i, j;

    for(i = 0;; i++)
	{
	if(args[i] == 0)
	    return;
	if(args[i] == '(')
	    break;
	}

    start = ++i;

    for(;; i++)
	{
	if(args[i] == 0)
	    return;
	if(start != 0 && (args[i] == '=' || args[i] == ',' || args[i] == ')'))
	    {
	    while(args[start] == ' ')
		start++;
	    end = i - 1;

	    // remove trailing spaces
	    while(args[end] == ' ')
		end--;

	    // remove parameter name
	    for(j = end; j >= start; j--)
		if(args[j] == ' ')
		    {
		    end = j - 1;
		    break;
		    }

	    if(start < end)
		{
		s = new McString(args, start, end - start + 1);
		arg_list.append(s);
		}
	    start = 0;
	    }
	if(args[i] == ',')
	    start = i + 1;
	if(args[i] == ')')
	    return;
	}
}

bool equalArgs(McString const &a, McString const &b)
{
    McDArray<McString *> a_list, b_list;
    McString a_tmp = a, b_tmp = b;
    int i;
    bool equal = true;

    if(countCommas(a) != countCommas(b))
	return false;

    if(a_tmp.rindex('(') == -1)
	a_tmp.insert(0, "(");
    if(a_tmp.rindex(')') == -1)
	a_tmp += ')';
    if(b_tmp.rindex('(') == -1)
	b_tmp.insert(0, "(");
    if(b_tmp.rindex(')') == -1)
	b_tmp += ')';

    separateArguments(a_tmp.c_str(), a_list);
    separateArguments(b_tmp.c_str(), b_list);

    if(a_list.size() != b_list.size())
	return false;

    for(i = 0; i < a_list.size(); i++)
	{
	if(*(a_list[i]) != *(b_list[i]) != 0)
	    equal = false;
	delete a_list[i];
	delete b_list[i];
	}

    return equal;
}

bool equalSignature(Entry *a, Entry *b)
{
    if(a->fullName != b->fullName)
	return false; // names not the same

    if(!equalTypes(a->type, b->type))
	return false; // types not the same

    if(!equalArgs(a->args, b->args))
	return false; // args not the same

    return true;
}

Entry *lookupEntryBySignature(Entry *group, Entry *model)
{
    Entry *result, *index = group;
    int i;

    for(i = 0; i < group->sublist.size(); i++)
	if((result = lookupEntryBySignature(group->sublist[i], model)))
	    return result;
    for(; index; index = index->next)
	if(equalSignature(index, model) && index != model)
	    return index;

    return 0;
}

bool hasDocumentation(Entry const *entry)
{
    return(entry->memo.length() > 0 ||
	entry->doc.length() > 0 ||
	entry->param.size() > 0 ||
	entry->exception.size() > 0 ||
	entry->retrn.size() > 0 ||
	entry->see.size() > 0 ||
	entry->field.size() > 0 ||
	entry->precondition.size() > 0 ||
	entry->postcondition.size() > 0 ||
	entry->invariant.size() > 0 ||
	entry->friends.size() > 0);
}

void mergeDocumentation(Entry *original, Entry *ccentry)
{
    int i;

    if(hasDocumentation(original) && hasDocumentation(ccentry))
	fprintf(stderr, _("Warning: `%s %s%s' already have documentation, merging\n"),
	    original->type.c_str(),
	    original->fullName.c_str(),
	    original->args.c_str());

    original->memo += ccentry->memo;
    original->doc += ccentry->doc;

    for(i = 0; i < ccentry->param.size(); i++)
	original->param.append(new McString(*(ccentry->param[i])));

    for(i = 0; i < ccentry->exception.size(); i++)
	original->exception.append(new McString(*(ccentry->exception[i])));

    for(i = 0; i < ccentry->retrn.size(); i++)
	original->retrn.append(new McString(*(ccentry->retrn[i])));

    for(i = 0; i < ccentry->see.size(); i++)
	original->see.append(new McString(*(ccentry->see[i])));

    for(i = 0; i < ccentry->field.size(); i++)
	original->field.append(new McString(*(ccentry->field[i])));

    for(i = 0; i < ccentry->precondition.size(); i++)
	original->precondition.append(new McString(*(ccentry->precondition[i])));

    for(i = 0; i < ccentry->postcondition.size(); i++)
	original->postcondition.append(new McString(*(ccentry->postcondition[i])));

    for(i = 0; i < ccentry->invariant.size(); i++)
	original->invariant.append(new McString(*(ccentry->invariant[i])));

    for(i = 0; i < ccentry->friends.size(); i++)
	original->friends.append(new McString(*(ccentry->friends[i])));

    for(i = 0; i < ccentry->sublist.size(); i++)
	original->addSubEntry(ccentry->sublist[i]);
}

bool mergeEntry(Entry *ccentry)
{
    int scope;
    bool noScope = false, parentFound = false;

    scope = ccentry->fullName.rindex(':');
    scope--;
    if(scope < 1 || ccentry->fullName[scope] != ':')
	noScope = true;

    if(language == LANG_PHP)
	{
	scope = ccentry->fullName.rindex('.');
	if(scope < 1 || ccentry->fullName[scope] != '.')
	    noScope = true;
	}

    McString unqualified_name(ccentry->fullName, scope + 2,
	ccentry->fullName.size() - scope - 3);
    McString parent_name(ccentry->fullName, 0, scope);

    Entry *father = getRefEntry(parent_name, ccentry->parent);
    if(father)
	parentFound = true;

    if(!noScope && !parentFound)
	{
	if(verb)
	    fprintf(stderr, _("Warning: no parent `%s' for `%s %s%s' found\n"),
		parent_name.c_str(), ccentry->type.c_str(),
		ccentry->fullName.c_str(), ccentry->args.c_str());
	return false;
	}
    else
	{
	if(!parentFound)
	    father = ccentry->parent;

	Entry *original = lookupEntryBySignature(father, ccentry);
	if(!parentFound && original == 0)
	    return false;

	if(original == 0 && father == ccentry->parent)
	    return false;

	Entry *tmp = ccentry->parent;
	ccentry->parent->removeSub(ccentry);
	ccentry->name = unqualified_name;

	if(original == 0)
	    {
	    father->addSubEntry(ccentry);
	    if(father != tmp)
		return true;
	    }
	else
	    {
    	    mergeDocumentation(original, ccentry);
    	    delete ccentry;
	    return true;
	    }
	}

    return false;
}

void mergeEntries(Entry *root)
{
    int i;

    for(i = 0; i < root->sublist.size(); i++)
	{
	if(root->sublist[i]->sub)
	    mergeEntries(root->sublist[i]);
	if(mergeEntry(root->sublist[i]))
	    i--;
	}
}

void getRefNames(McDArray<McString*> strings, const char *names)
{
    McString str = names, *s;
    int	i = 0, j;

    while((j = str.index(",", i)) >= 0)
	{
	s = get1RefName(str, i, j);
	if(s)
	    strings.append(s);
	i = j + 1;
	}
    s = get1RefName(str, i, str.length());
    if(s)
	strings.append(s);
}

/* We are looking for an entry named `name'. We start on the level of entry.
   `name' can be a simple identifier or it can contain C++ scopes (`::')

   This routine is called `A HELL OF A LOT' (tm), and needs keeping quick
*/
Entry *searchRefEntry(McString &name, Entry *entry)
{    
    Entry *rot = entry, *result;
    static McHashTable<const char *, char *> *table;
    int i, lp;

    // General entries have a) no page, b) no references
    // This saves a _huge_ amount of time!
    if(entry == NULL || entry->general)
	return 0;

    if(language == LANG_JAVA && name.index('.') >= 0)
	{
	result = findJavaClass(name, root);
	return result;
	}

    switch(language)
	{
	case LANG_CXX:
	    table = cxxtable;
	    break;
	case LANG_JAVA:
	    table = javatable;
	    break;
	case LANG_IDL:
	    table = idltable;
	    break;
	case LANG_PHP:
	    table = phptable;
	    break;
	default:
	    table = 0;
	}

    if(table && table->lookup((char *)name))	// It's a keyword silly !
	return 0;

    // Don't bother caching keywords
    if(cache_pos >= MAX_CACHE)
	cache_pos = 0;
    if(fastNotSmall)				// Actualy use the cache :-)
	for(lp = 0; lp < MAX_CACHE; lp++)
	    if(cache_entry[lp] && isIt(name, cache_entry[lp]))
		return cache_entry[lp];		// Don't re-enter it (probably best)
    	    else
		if(cache_name[lp])
		    if(strcmp(name.c_str(), cache_name[lp]) == 0)
			return 0;
    if(cache_name[cache_pos])
	free(cache_name[cache_pos]);
    cache_name[cache_pos] = 0;

    while(rot->section == MANUAL_SEC && rot->parent)
	rot = rot->parent;

    Entry *tmp = rot->findSub(name);
    if(tmp)
	return(cache_entry[cache_pos++] = tmp);

    // Otherwise search in parent classes
    for(i = 0; i < rot->pubBaseclasses.size(); i++)
	{
	entry = searchRefEntry(name, rot->pubBaseclasses[i]);
	if(cache_pos >= MAX_CACHE)
	    cache_pos = 0;
	if(entry)
	    return(cache_entry[cache_pos++] = entry);
	}

    for(i = 0; i < rot->proBaseclasses.size(); i++)
	{
	entry = searchRefEntry(name, rot->proBaseclasses[i]);
	if(cache_pos >= MAX_CACHE)
	    cache_pos = 0;
	if(entry)
	    return(cache_entry[cache_pos++] = entry);
	}

    // Last chance, search on a higher doc++-level
    if(rot->parent)
	return searchRefEntry(name, rot->parent);

    // Cache a name with no corresponding entry
    assert(cache_pos < MAX_CACHE);

    // Whilst entries are static, strings get realloced lots!
    cache_name[cache_pos] = strdup(name.c_str());
    cache_entry[cache_pos++] = 0;

    return 0;
}

Entry *getRefEntry(const char *name, Entry *entry)
{
    Entry *result;
    McString tmp(name);

    result = getRefEntry(tmp, entry);
    return result;
}

Entry *getRefEntry(McString &name, Entry *entry)
{
    extern McDArray<namespace_entry *> namespace_table;
    int i;
    McString tmp, fullName;
    Entry *result = searchRefEntry(name, entry);

    if(!result && language == LANG_CXX)
	// try to search inside namespaces, if any
	for(i = 0; i < namespace_table.size(); i++)
	    {
	    tmp = namespace_table[i]->name;
	    fullName = namespace_table[i]->name;
	    fullName += "::";
	    fullName += name;
	    if((result = searchRefEntry(fullName, searchRefEntry(tmp, root))))
		{
#ifdef DEBUG
		if(verb)
		    printf(_("`%s' found in namespace `%s'!\n"),
			name.c_str(), (char *)namespace_table[i]->name);
#endif
		break;
		}
	    }

    if(!result && language == LANG_CXX)
	{
        // Search for fully-qualified identifiers in top-level namespaces
        namespace_roots.resetIter();

        const char*	key;
        Entry*		value;

        while(!result && namespace_roots.next(key, value))
	    {
            fullName = value->fullName;
            fullName += "::";
            fullName += name;

            result = searchRefEntry(fullName, value);
	    }
	}

    return result;
}

Entry *findEntry(Entry *start, const char *fullName, unsigned short section)
{
    Entry *result, *find;

    for(find = start; find; find = find->next)
	{
	if(strcmp(find->fullName.c_str(), fullName) == 0)
	    if(section == 0 || (find->section & section))
		return find;
	if(find->sub)
	    if((result = findEntry(find->sub, fullName, section)))
		return result;
	}

    return 0;
}

int getNumChildren(Entry *tp)
{
    Entry *tmp = tp;
    int num = 0;

    for(tmp = tp->sub; tmp; tmp = tmp->next)
	num++;

    return num;
}

void checkPackages(Entry *tp)
{
    Entry *tmp = tp, *child, *tmp2, *tmp3;
    McString newName;

    for(tmp = tp; tmp; tmp = tmp->next)
	{
	if(tmp->sub)
	    checkPackages(tmp->sub);

	if(MAKE_DOC(tmp))
	    if((tmp->section == PACKAGE_SEC) && (getNumChildren(tmp) == 1))
		{
		child = tmp->sub;

		if(child->section == PACKAGE_SEC)
		    {
		    newName.clear();
		    newName += tmp->name;
		    newName += "." + (tmp->sub)->name;

		    tmp2 = tmp->sub;
		    tmp2->parent = tmp->parent;

		    for(tmp3 = tmp2->sub; tmp3; tmp3 = tmp3->next)
			tmp3->parent = tmp;

		    tmp->section = tmp2->section;
		    tmp->general = tmp2->general;
		    tmp->protection = tmp2->protection;
		    tmp->ownPage = tmp2->ownPage;
		    tmp->sub = tmp2->sub;
		    tmp->type = tmp2->type;
		    tmp->name = newName;
		    tmp->args = tmp2->args;
		    tmp->memo = tmp2->memo;
		    tmp->doc = tmp2->doc;
		    tmp->program = tmp2->program;
		    tmp->author = tmp2->author;
		    tmp->version = tmp2->version;
		    tmp->see = tmp2->see;
		    tmp->param = tmp2->param;
		    tmp->exception = tmp2->exception;
		    tmp->precondition = tmp2->precondition;
        	    tmp->postcondition = tmp2->postcondition;
        	    tmp->invariant = tmp2->invariant;
		    tmp->retrn = tmp2->retrn;
		    tmp->sublist = tmp2->sublist;
		    tmp->pubChilds = tmp2->pubChilds;
		    tmp->proChilds = tmp2->proChilds;
		    tmp->priChilds = tmp2->priChilds;
		    tmp->baseclasses = tmp2->baseclasses;
		    tmp->pubBaseclasses = tmp2->pubBaseclasses;
		    tmp->proBaseclasses = tmp2->proBaseclasses;
		    tmp->priBaseclasses = tmp2->priBaseclasses;
		    tmp->otherPubBaseclasses = tmp2->otherPubBaseclasses;
		    tmp->otherProBaseclasses = tmp2->otherProBaseclasses;
		    tmp->otherPriBaseclasses = tmp2->otherPriBaseclasses;
		    tmp->implements = tmp2->implements;
		    tmp->extends = tmp2->extends;
		    tmp->import = tmp2->import;
		    tmp->override = tmp2->override;
		    }
	    	}
	}
}

void reNumber(Entry *tp)
{
    Entry *tmp = tp;
    int	i = 1;

    for(tmp = tp->sub; tmp; tmp = tmp->next)
	{
	if(MAKE_DOC(tmp))
	    tmp->subnumber = i++;
	reNumber(tmp);
	}
    for(tmp = tp->sub; tmp; tmp = tmp->next)
	if(!MAKE_DOC(tmp))
	    tmp->subnumber = i++;
}

bool relevantClassGraphs(Entry *tmp)
{
    for(; tmp; tmp = tmp->next)
	{
	if((tmp->section & CLASS_SEC) &&
	    tmp->proBaseclasses.size() == 0 &&
	    tmp->pubBaseclasses.size() == 0)
	    return true;
	if(relevantClassGraphs(tmp->sub))
	    return true;
	}

    return false;
}
