/*
  html.cc

  Copyright (c) 1996 Roland Wunderling, Malte Zoeckler
  Copyright (c) 1998 Michael Meeks
  Copyright (c) 1998-2001 Dragos Acostachioaie

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
#include <ctype.h>
#ifdef __BORLANDC__
#include <dir.h>
#endif
#if defined(__VISUALC__) || defined(__WATCOMC__) || defined(__MINGW32__)
#include <direct.h>
#endif
#include <errno.h>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "McDirectory.h"
#include "McHashTable.h"
#include "McSorter.h"
#include "classgraph.h"
#include "doc.h"
#include "gifs.h"
#include "java.h"

#define max(a, b) ((a) > (b) ? (a) : (b))

#define BGCOLOR "<BODY BGCOLOR=\"#ffffff\">\n"

static McString header, footer;
static char *GENERAL_NAME = "General";
FILE *generalf;

static char *docType = "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0 Transitional//EN\">\n\n";
static McString styleSheet;

static McString indexHeader;
static McString indexFooter;

static McString hierHeader;
static McString hierFooter;

static McString generalHeader;

static McString pageHeader;
static McString pageFooter;
static McString pageFooterJava;

void writeTOCentry(FILE *f, Entry *e, bool memo, bool dup = false);

int makedir(const char *d, int perm)
{
#if defined(__BORLANDC__) || defined(__WATCOMC__)
    return mkdir(d);
#else
#if defined(__VISUALC__) || defined(__MINGW32__)
    return _mkdir(d);
#else
    return mkdir(d, perm);
#endif
#endif
}

#define myisalnum(c) ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_' || c== '.' || c == '-')

// Initializes HTML headers
void buildHeaders()
{
    if(htmlStyleSheet.length() > 0)
	{
	styleSheet =	"   <LINK REL=\"stylesheet\" TYPE=\"text/css\" HREF=\"";
	styleSheet +=	htmlStyleSheet;
	styleSheet +=	"\">\n";
	}

    indexHeader =	docType;
    indexHeader	+=	"<HTML>\n"
			"<HEAD>\n";
    indexHeader +=	_("   <TITLE>Table of Contents</TITLE>\n");
    indexHeader +=	"   <META NAME=\"GENERATOR\" CONTENT=\"DOC++ " DOCXX_VERSION "\">\n";
    indexHeader +=	styleSheet;
    indexHeader +=	"</HEAD>\n"
			BGCOLOR;

    hierHeader =	docType;
    hierHeader +=	"<HTML>\n"
			"<HEAD>\n";
    hierHeader +=	_("   <TITLE>Hierarchy of Classes</TITLE>\n");
    hierHeader +=	"   <META NAME=\"GENERATOR\" CONTENT=\"DOC++ " DOCXX_VERSION "\">\n";
    hierHeader +=	styleSheet;
    hierHeader +=	"</HEAD>\n"
			BGCOLOR;

    generalHeader =	docType;
    generalHeader +=	"<HTML>\n";
    generalHeader +=	_("<HEAD>\n   <TITLE>General Bits</TITLE>\n");
    generalHeader +=	"   <META NAME=\"GENERATOR\" CONTENT=\"DOC++ " DOCXX_VERSION "\">\n";
    generalHeader +=	styleSheet;
    generalHeader +=	"</HEAD>\n"
			BGCOLOR;

    pageHeader +=	BGCOLOR;
}

// Initializes HTML footers
void buildFooters()
{
    bool generateClassGraphs = !noClassGraph && relevantClassGraphs(root);

    if(generateClassGraphs)
	{
	indexFooter =	"<P><I><A HREF=\"HIER";
	indexFooter +=	htmlSuffix;
	indexFooter +=	_("\">Hierarchy of classes</A></I></P>");
	}

    hierFooter =	"<P><I><A HREF=\"index";
    hierFooter +=	htmlSuffix;
    hierFooter +=	_("\">Alphabetic index</A></I></P>");

    pageFooter =	"<P><I><A HREF=\"index";
    pageFooter +=	htmlSuffix;
    pageFooter +=	_("\">Alphabetic index</A></I>");
    if(generateClassGraphs)
	{
	pageFooter +=	" <I><A HREF=\"HIER";
	pageFooter +=	htmlSuffix;
	pageFooter +=	_("\">Hierarchy of classes</A></I>");
	}
    pageFooter +=	"</P>";

    pageFooterJava =	"<P><I><A HREF=\"index";
    pageFooterJava +=	htmlSuffix;
    pageFooterJava +=	_("\">Alphabetic index</A></I>");
    if(generateClassGraphs)
	{
	pageFooterJava +=	" <I><A HREF=\"HIER";
	pageFooterJava +=	htmlSuffix;
	pageFooterJava +=	_("\">HTML hierarchy of classes</A>");
	pageFooterJava +=	_(" or <A HREF=\"HIERjava");
	pageFooterJava +=	htmlSuffix;
	pageFooterJava +=	"\">Java</A></I>";
	}
    pageFooterJava +=	"</P>";
}

McString makeFileName(const McString& str, Entry *e)
{
    static McHashTable<char *,int> files(1);
    McString s, ls;
    char buf[40];
    int i, l = str.length();
    int odd = 0;	// An unusual thing ...

    // the user already decided what filename to use, so exit quickly
    if(e->fileName.length() > 0)
	{
	e->fileName += htmlSuffix;
	return e->fileName;
	}

    for(i = 0; i < l; i++)
	if(myisalnum(str[i]))
	    {
	    char c = str[i];
	    s += c;
	    if(c >= 'A' && c <= 'Z')
		c += (((int)'a') - 'A');
	    ls += c;
	    }
    char *tmp = strdup(ls.c_str());
    int *val = files.insert(tmp);
    if(*val > 1)
	{
	free(tmp);
	s += '.';
	sprintf(buf, "%d", *val);
	s += buf;
	odd = 1;
	}
    (*val)++;

    s += htmlSuffix;

#ifdef DEBUG
    if(verb && odd)
	{
	fprintf(stderr, _("Warning: weird file name `%s'\n"), s.c_str());
	e->dump(stdout);
	}
#endif
    return s;
}

enum _outType
{
    DOC		= 1,
    MEMO	= 2
};

void htmlComment(FILE *f, Entry *e, int t)
{
    bool done = false;
    char start[] = "<BLOCKQUOTE>";
    char end[] = "</BLOCKQUOTE>";

    if((t & DOC) && e->doc.length() > 1)
	{
	fprintf(f, "%s%s%s\n", start, (e->hdoc), end);
	done = true;
	}
    if((t & MEMO) && !done && e->memo.length() > 1)
	fprintf(f, "%s%s%s\n", start, (e->hmemo), end);
}

int subEntryIsToBeDocumented(Entry *entry)
{
    bool toBeDocumented = false;

    if(entry->protection != PRIV || withPrivate)
	{
	if(entry->retrn.size() || entry->field.size() ||
	    entry->param.size() || entry->author.length() ||
	    entry->see.size() || entry->version.length())
	    toBeDocumented = true;
	if(entry->doc.length() > entry->memo.length() || 
	    (entry->doc.length() == entry->memo.length() &&
	     entry->doc == entry->memo))
	    toBeDocumented = true;

	if(alwaysPrintDocSection)
	    toBeDocumented = true;
	}

    return toBeDocumented;
}

void copyright(FILE *f)
{
    fprintf(f, "<HR>\n");
    if(footer.length() <= 0 && ownFooter.length() == 0)
	{
	fprintf(f, "<BR>\n");
	fprintf(f, _("This page was generated with the help of <A HREF=\"http://docpp.sourceforge.net\">DOC++</A>.\n"));
	fprintf(f, "</BODY>\n");
	fprintf(f, "</HTML>\n");
	}
    else
	fprintf(f, "%s", footer.c_str());
}

// Writes classgraphs. Either as text-graphs or as java-graphs.
class ClassGraphWriter
{
    public:
	static void writeJava(FILE *f, Entry *e, bool directOnly = true);
	static void writeText(FILE *f, Entry *e);
	static void write(FILE *f, Entry *e);
	static void writeImplements(FILE *f, Entry *e);
};

struct MemberWriterListEntry {
    Entry *entry;
    int links;
    int withSub;
};

// This class writes members.
class MemberWriter
{
    public:
	MemberWriter()
	    {
	    f = NULL;
	    }
    protected:
	McString heading;
	int first;
	FILE *f;
	McDArray<MemberWriterListEntry> list;

	virtual const char *startString()
	    {
	    return "<P><DL>";
	    };
	virtual const char *endString()
	    {
	    return "</DL></P>";
	    };
	virtual void showSubMembers(Entry *);
	virtual void writeMember(Entry *e, bool links, bool withSub = true);
	class EntryCompare {
	    public:
		int operator()(const MemberWriterListEntry& l1, const MemberWriterListEntry& l2)
		    {
		    return strcmp(l1.entry->fullName.c_str(), l2.entry->fullName.c_str());
		    }
	};
    public:
	virtual void sort()
	    {
	    EntryCompare comp;
	    if(list.size())
		::sort((MemberWriterListEntry *)list, list.size(), comp, 0);
	    }
	virtual void startList(FILE *f, char *heading, bool withLinks);
	virtual void addMember(Entry *e, bool links, bool withSub = true)
	    {
	    MemberWriterListEntry le;
	    le.entry = e;
	    le.links = links;
	    le.withSub = withSub;
	    list.append(le);
	    }
	virtual void endList();
	virtual ~MemberWriter()
	    {};
};

class MemberWriterTable : public MemberWriter
{
    protected:
	virtual const char *startString()
	    {
	    return (withBorders ? "<P><TABLE BORDER>" : "<P><TABLE>");
	    };

	virtual const char *endString()
	    {
	    return "</TABLE></P>";
	    };

	virtual void writeMember(Entry *e, bool links, bool withSub = true);
};

class TOClist;

struct TOCListEntry {
    Entry *e;
    TOClist *tl;
    const char *name;
};

// Renamed from `SortedList' as far too specific now...
class TOClist : public MemberWriter
{
    protected:
	McDArray<TOCListEntry> list;
    public:
	virtual void sort()
	    {
	    EntryCompare comp;
	    int lp;

	    if(list.size() > 1)
		::sort((TOCListEntry *)list, list.size(), comp, 0);

	    // Sort subsections
	    for(lp = 0; lp < list.size(); lp++)
		if(list[lp].tl)
		    list[lp].tl->sort();
	    }
	class EntryCompare {
	    public:
		int operator()(TOCListEntry& l1, TOCListEntry& l2)
		    {
		    return strcmp(l1.name, l2.name);
		    }
	};
	void addEntry(Entry *entry, TOClist *tl);
	void write(FILE *f);
	int size()
	    {
	    return list.size();
	    }
	~TOClist();
};

void TOClist::write(FILE *f)
{
    // Resolve conflicts, get meaningful data from WriteTOCentry
    //    (f, entry, 1), and go for it... merge writeTOCEntry into this ?
    // As a further burden, try and merge duplicate ClassName link references
    // ALSO, do sub lists
    int i, last_collision = 0, size = list.size(), conf;
    bool inUL = false;

    for(i = 0; i < size; i++)
	{
	TOCListEntry *le, *nle;
	le = &list[i];

	if(i < size - 1)
	    nle = &list[i + 1];
	else
	    nle = 0;

	// Skip identical entries (e.g. loads of constructors in a class)
	// ---
	// Global functions that are overloaded get seen as indentical,
	// but they in fact have different documentation, which is why we 
	// check for fileName as well <cpbotha@bigfoot.com>
	if(nle && le->name == nle->name && le->tl == nle->tl &&
	    le->e->fileName == nle->e->fileName)
	    continue;

	// Detect identicaly named entries (e.g. lots of next())
	// ---
	// See comment above about overloaded global functions
	// <cpbotha@bigfoot.com>
	conf = nle && le->e->name == nle->e->name &&
	    le->e->fileName == nle->e->fileName;
	if(conf != last_collision && conf)
	    {
	    fprintf(f, "<LI><B>%s:</B>\n<UL>\n", le->e->name.c_str());
	    inUL = true;
	    }

	fprintf(f, "<LI>");
	writeTOCentry(f, le->e, true, (conf | last_collision));

	if(conf != last_collision && !conf)
	    {
	    fprintf(f, "</UL>\n");
	    inUL = false;
	    }

	if(le->tl && le->tl->size() > 0)	// A dreaded sub-list e.g. package list
	    {
	    fprintf(f, "<UL>");
	    le->tl->write(f);
	    fprintf(f, "</UL>");
	    }
	last_collision = conf;
	}
    if(inUL)		// Some odd thing happened at the end of the list...
	fprintf(f, "</UL>\n");
}

void TOClist::addEntry(Entry *entry, TOClist *tl)
{
    TOCListEntry le;

    // don't show class members in the TOC... due to popular demand
    // do the same with structure and namespace members
    if(!showMembersInTOC)
	if((entry->parent->section & CLASS_SEC) ||
	    entry->parent->section == NAMESPACE_SEC ||
	    (entry->parent->section == INTERFACE_SEC && language == LANG_IDL) ||
	    entry->parent->section == UNION_SEC ||
	    entry->parent->section == TYPEDEF_SEC ||
	    (entry->parent->section == MANUAL_SEC && entry->parent != root))
	    return;

    le.e = entry;
    le.tl = tl;
    le.name = entry->fullName.c_str();
    list.append(le);
}

TOClist::~TOClist()
{
    int i;

    for(i = 0; i < list.size(); i++)
	if(list[i].tl)
	    free(list[i].tl);
}

class HIERlist;

struct HIERListEntry {
    Entry *e;
    HIERlist *hl;
    const char *name;
};

class HIERlist : public MemberWriter
{
    protected:
	McDArray<HIERListEntry> list;
    public:
	virtual void sort()
	    {
	    EntryCompare comp;
	    int i;

	    if(list.size() > 1)
		::sort((HIERListEntry *)list, list.size(), comp, 0);

	    // Sort subentries
	    for(i = 0; i < list.size(); i++)
		if(list[i].hl)
		    list[i].hl->sort();
	    }
	class EntryCompare
	    {
	    public:
		int operator()(HIERListEntry& l1, HIERListEntry& l2)
		    {
		    return strcmp(l1.name, l2.name);
		    }
	    };
	void addEntry(Entry *entry, HIERlist *hl);
	void write(FILE *f);
	int size()
	    {
	    return list.size();
	    }
	~HIERlist();
};

void HIERlist::write(FILE *f)
{
    int i;

    for(i = 0; i < size(); i++)
	{
	fprintf(f, "\n<LI>");
	writeTOCentry(f, list[i].e, false);
	if(list[i].hl)
	    {
	    fprintf(f, "<UL>\n");
	    list[i].hl->write(f);
	    fprintf(f, "</UL>\n");
	    }
	}
}

void HIERlist::addEntry(Entry *entry, HIERlist *hl)
{
    HIERListEntry le;

    le.e = entry;
    le.hl = hl;
    le.name = entry->name.c_str();
    list.append(le);
}

HIERlist::~HIERlist()
{
    int i;

    for(i = 0; i < list.size(); i++)
	if(list[i].hl)
	    delete list[i].hl;
}

void writeHIERentry(HIERlist &list, FILE *f, Entry *entry)
{
    int i;
    HIERlist *sub = 0;

    if(entry->pubChilds.size() > 0 || entry->proChilds.size() > 0)
	{
	sub = new HIERlist();

	for(i = 0; i < entry->pubChilds.size(); i++)
	    writeHIERentry(*sub, f, entry->pubChilds[i]);
	for(i = 0; i < entry->proChilds.size(); i++)
	    writeHIERentry(*sub, f, entry->proChilds[i]);
	}

    list.addEntry(entry, sub);
}

void writeHIERrec(HIERlist &list, FILE *f, Entry *entry)
{
    int i;

    for(i = 0; i < entry->sublist.size(); i++)
	{
	if(entry->sublist[i]->baseclasses.size() == 0)
	    if(entry->sublist[i]->isClass() &&
		entry->sublist[i]->proBaseclasses.size() == 0 &&
		entry->sublist[i]->pubBaseclasses.size() == 0)
		writeHIERentry(list, f, entry->sublist[i]);
	writeHIERrec(list, f, entry->sublist[i]);
	}
}

void writeHIER(FILE *f)
{
    HIERlist list;

    if(header.length() == 0 && ownHeader.length() == 0)
	fprintf(f, "%s", hierHeader.c_str());
    else
	fprintf(f, "%s", header.c_str());

    fprintf(f, _("<H1>Hierarchy of Classes</H1>\n"));
    fprintf(f, "<UL>\n");
    writeHIERrec(list, f, root);
    list.sort();
    list.write(f);
    fprintf(f, "</UL>\n");

    fprintf(f, "%s", hierFooter.c_str());

    copyright(f);
}

void MemberWriter::showSubMembers(Entry *e)
{
    Entry *tmp;

    fprintf(f, "%s\n", startString());
    for(tmp = e->sub; tmp; tmp = tmp->next)
	writeMember(tmp, true);
    fprintf(f, "%s\n", endString());
}

void printRefLabel(FILE *f, Entry *entry)
{
    fprintf(f, "\n<A NAME=\"%s\"></A>\n<A NAME=\"DOC.", entry->name.c_str());
    entry->dumpNumber(f);
    fprintf(f, "\"></A>\n");
}

void MemberWriter::writeMember(Entry *entry, bool link, bool withSub)
{
    if(first)
	{
	fprintf(f, "%s\n%s", startString(), heading.c_str());
	first = 0;
	}

    if(!subEntryIsToBeDocumented(entry) && link)
	printRefLabel(f, entry);

    fprintf(f, "<DT>\n");
    if(entry->ownPage)
	{
	fprintf(f, "%s <B><A HREF=\"%s\">%s</A></B>%s\n",
	    entry->htype, entry->fileName.c_str(), entry->hname, entry->hargs);
	if(strlen(entry->hmemo) > 0)
	    fprintf(f, "<DD><I>%s</I>\n", entry->hmemo);
	}
    else
	{
	if(subEntryIsToBeDocumented(entry) && link)
	    {
	    fprintf(f, "<IMG ALT=\"[more]\" BORDER=0 SRC=icon1.gif>");
	    fprintf(f, "%s <B><A HREF=\"#DOC.", entry->htype);
	    entry->dumpNumber(f);
	    fprintf(f, "\">%s</A></B>%s", entry->hname, entry->hargs);
	    if(entry->pureVirtual)
		fprintf(f, " = 0");
	    fprintf(f, "\n");
	    }
	else
	    {
	    if(entry->section != MANUAL_SEC)
		fprintf(f, "<IMG ALT=\"o\" SRC=icon2.gif>");
	    else 
        	fprintf(f, "<P>");
	    fprintf(f, "%s <B>%s</B>%s\n", entry->htype, entry->hname, entry->hargs);
	    }

	if(link && strlen(entry->hmemo) > 0)
	    fprintf(f, "<DD><I>%s</I>\n", entry->hmemo);
	}

    if(entry->sub && withSub)
	showSubMembers(entry);
}

void MemberWriter::startList(FILE *file, char *head, bool withLinks)
{
    f = file;
    heading = head;
    first = 1;
    list.resize(0);
}

void MemberWriter::endList()
{
    int i;

    if(f == NULL)
	return;

    if(sortEntries)
	sort();

    for(i = 0; i < list.size(); i++)
	writeMember(list[i].entry, list[i].links, list[i].withSub);

    if(!first)
	fprintf(f, "%s\n\n", endString());
}

void MemberWriterTable::writeMember(Entry *entry, bool link, bool withSub)
{
    char *args, *type;

    if(first)
	{
	fprintf(f, "%s\n%s", startString(), heading.c_str());
	first = 0;
	}

    fprintf(f, "<TR>\n");
    fprintf(f, "<TD VALIGN=TOP>");

    if(!subEntryIsToBeDocumented(entry) && link) 
	printRefLabel(f, entry);

    if((entry->section & CLASS_SEC) || entry->section == NAMESPACE_SEC)
	{
	fprintf(f, "<DT><A HREF=\"%s\">%s <B>%s</B></A>\n", entry->fileName.c_str(),
	    entry->htype, entry->hname);
	if(strlen(entry->hmemo) > 0)
	    fprintf(f, "<DD><I>%s</I>\n", entry->hmemo);
	}
    else
	{
	if(subEntryIsToBeDocumented(entry) && link)
	    {
	    fprintf(f, "<A HREF=\"#DOC.");
	    entry->dumpNumber(f);
	    fprintf(f, "\"><IMG ALT=\"[more]\" BORDER=0 SRC=icon1.gif></A>");
	    }
	else
	    if(entry->section != MANUAL_SEC)
		fprintf(f, "<IMG ALT=\"o\" SRC=icon2.gif>");
	    else
		fprintf(f, "<P>");

	args = entry->hargs;
	type = entry->htype;

	fprintf(f, "%s ", type);
	fprintf(f, "</TD><TD>");
	fprintf(f, "<B>%s</B> %s<BR>", entry->hname, args);

	if(link)
	    fprintf(f, "\n<I>%s</I>\n", entry->hmemo);
	}

    if(entry->sub && withSub)
	showSubMembers(entry);

    fprintf(f, "</TD></TR>");
}

void ClassGraphWriter::writeJava(FILE *f, Entry *entry, bool directOnly)
{
    ClassGraph cg(entry, 0);
    ClassGraph *cls = &cg;
    McString classes, before, after, indent;
    char first = 1;
    int numLines = 0, longest = 0;
    char buf[40];

    cg.addBases();

    if(directOnly)
	cg.addDirectChilds();
    else
	cg.addAllChilds();

    for(cls = cg.firstLine; cls; cls = cls->nextLine)
	{
	numLines++;    
	if(first)
	    first = 0;
	else
	    {
	    classes += ",";
	    before += ",";
	    after += ",";
	    indent += ",";
	    }
	if(cls->entry)
	    {
	    if(longest < cls->entry->name.size())
		longest = cls->entry->name.size();

	    if(cls->entry->section & CLASS_SEC)
		classes += "C";
	    else
		classes += "I";
	    classes += cls->entry->name;
	    classes += ",M";
	    classes += cls->entry->fileName;
	    }
	else
	    {
	    if(longest < cls->name.size())
		longest = cls->name.size();
	    classes += "M" + cls->name;
	    classes += ",M";
	    }
	before += "M" + cls->before;
	after += "M" + cls->after;
	sprintf(buf, "%d", cls->indent);
	indent += buf;
	}

    fprintf(f, "<APPLET CODE=\"ClassGraph.class\" WIDTH=600 HEIGHT=%d>\n",
	numLines * 30 + 5);
    fprintf(f, "<param name=classes value=\"%s\">\n", classes.c_str());
    fprintf(f, "<param name=before value=\"%s\">\n", before.c_str());
    fprintf(f, "<param name=after value=\"%s\">\n", after.c_str());
    fprintf(f, "<param name=indent value=\"%s\">\n", indent.c_str());
    fprintf(f, "<param name=arrowdir value=");
    if(upArrows)
	fprintf(f, "\"up\">\n");
    else
	fprintf(f, "\"down\">\n");

    fprintf(f, "</APPLET>\n");    
}

void ClassGraphWriter::writeText(FILE *f, Entry *e)
{
    int i;
    Entry *c;

    for(i = 0; i < max(1, e->baseclasses.size()); i++)
	{
	if(i < e->baseclasses.size() && e->baseclasses[i]->section == INTERFACE_SEC)
	    {
	    if(i == 0)
		fprintf(f, "<H3>%s</H3>\n", e->hname);
	    continue;
	    }
	fprintf(f, "<H3>%s\n", e->hname);
	c = e;
	while(c->baseclasses.size() > 0)
	    {
	    if(c == e)
		c = c->baseclasses[i];
	    else
		c = c->baseclasses[0];
	    if(c)
		fprintf(f, "- <A HREF=\"%s\">%s</A>\n",
		    c->fileName.c_str(),
		    c->hname);
	    else
		fprintf(f, "- %s</A>\n", c->name.c_str());  
	    }
	fprintf(f, "</H3>\n");
	}
}

void ClassGraphWriter::write(FILE *f, Entry *e)
{
    if(javaGraphs)
	writeJava(f, e);
    else 
	writeText(f, e);
}

void ClassGraphWriter::writeImplements(FILE *f, Entry *e)
{
    int i, first = 1;
    Entry *c;

    for(i = 0; i < e->baseclasses.size(); i++)
	{
	if(e->baseclasses[i]->section != INTERFACE_SEC)
	    continue;

	if(first)
	    fprintf(f, "<HR>\n<H2>Implements:</H2>\n");
	first = 0;

	c = e->baseclasses[i];
	if(i > 0)
	    fprintf(f, ", ");
	if(c)
	    fprintf(f, "<A HREF=\"%s\">%s</A>", c->fileName.c_str(),
		(const char*)(c->hname));

	}
}

extern char *strToHtml(McString &in, char *dest = 0, Entry *ct = 0, bool withLinks = false, bool pureCode = false);
extern char *seeToHtml(McString &in, Entry *ct = 0);

void entry2link(McString& u, Entry *ref, const char *linkname)
{
    Entry *globref = ref;

    if(globref != root)
	while(globref->parent && globref->parent != root &&
	     !globref->fileName.length())
	    globref = globref->parent;
    if(ref->fileName.length())
	{
	u += "<!1><A HREF=\"";
	u += ref->fileName;
	if(!ref->ownPage)
	    {
	    u += "#DOC.";
	    ref->dumpNumber(u);
	    }
	u += "\">";
	if(linkname)
	    u += linkname;
	else
	    u += ref->hname;
	u += "</A>";
	}
    else
	if(globref)
	    {
	    u += "<!2><A HREF=\"" + globref->fileName + "#DOC.";
	    ref->dumpNumber(u);
	    u += "\">";
	    if(linkname)
		u += linkname;
	    else
		u += ref->hname;
	    u += "</A>";
	    }
}  

void writeTOCentry(McString& out, Entry *e, bool memo, bool dup = false)
{ 
    McString link = "";

    if(e->fileName.length())
	{
	link += "<A HREF=\"";
	if(e->parent)
	    if(e->ownPage)
		{
		if(e->fileName.length() > 0)
	    	    link += e->fileName;
                else
	    	    {
		    link += e->hname;
		    link += htmlSuffix;
		    }
		}
	    else
		{
		if(e->parent->fileName.length() > 0)
		    link += e->parent->fileName;
		else
		    {
		    link += e->parent->hname;
		    link += htmlSuffix;
		    }
		link += "#";
		link += e->hname;
		}
	else
	    {
	    link += GENERAL_NAME;
	    link += htmlSuffix;
	    link += "#";
	    link += e->hname;
	    }
	link += "\">";
	link += ENTRY_NAME(e);
	link += "</A>";
	}
    else
	entry2link(link, e, (const char *)(e->hname));
    out += link;
    if(memo)
	if(e->memo.length())
	    {
	    out += " <I>";
	    out += (const char *)e->hmemo;
	    out += "</I>\n";
	    }
         else
	    out += '\n';    
}

void writeTOCentry(FILE *f, Entry *e, bool memo, bool dup)
{ 
    McString out;

    writeTOCentry(out, e, memo, dup);
    fprintf(f, "%s", out.c_str());
}

void writeHIERentry(FILE *f, Entry *k, bool memo)
{
    int i;

    fprintf(f, "<LI>");
    writeTOCentry(f, k, memo);
    if(k->pubChilds.size() || k->proChilds.size())
	{
	fprintf(f, "<UL>\n");
	for(i = 0; i < k->pubChilds.size(); i++)
	    writeHIERentry(f, k->pubChilds[i], memo);
	for(i = 0; i < k->proChilds.size(); i++)
	    writeHIERentry(f, k->proChilds[i], memo);
	fprintf(f, "</UL>\n");
	}
}

struct {
    int sec;
    const char *name;
} toc_sections[] = {
    { MANUAL_SEC,	"General stuff" },
    { PACKAGE_SEC,	"Packages" },
    { NAMESPACE_SEC,	"Namespaces" },
    { CLASS_SEC,	"Classes" },
    { INTERFACE_SEC,	"Interfaces" },
    { FUNCTION_SEC,	"Functions" },
    { VARIABLE_SEC,	"Variables" },
    { MACRO_SEC,	"Macros" },
    { UNION_SEC,	"Enums, Unions, Structs" },
    { TYPEDEF_SEC,	"Typedefs" },
    { 0, 0 }
};

void writeTOCRec(TOClist& list, FILE *f, Entry *root, int section, int& first)
{ 
    int i, output = root->section == toc_sections[section].sec &&
	root->name.length();

    // Want to recurse down em all, but subtree only if output root
    TOClist *sub = 0;
    if(root->sublist.size())
	{
	if(output)
	    sub = new TOClist();

	for(i = 0; i < root->sublist.size(); i++)
	    if(output)
		writeTOCRec(*sub, f, root->sublist[i], section, first);
	    else
		writeTOCRec(list, f, root->sublist[i], section, first);
	}
    if(output)
	list.addEntry(root, sub);
}

void writeTOC(FILE *f)
{
    int first = 1;

    if(header.length() == 0 && ownHeader.length() == 0)
        fprintf(f, "%s", indexHeader.c_str());
    else
        fprintf(f, "%s", header.c_str());

    fprintf(f, _("\n<H1>Table of Contents</H1>\n"));

    for(int k = 0; toc_sections[k].name; k++)
	{
	first = 1;
	TOClist list;
	writeTOCRec(list, f, root, k, first);
	if(list.size() > 0)
	    {
	    list.sort();
	    fprintf(f, "<H2>%s</H2>\n", toc_sections[k].name);
	    fprintf(f, "<UL>\n");
	    list.write(f);
	    fprintf(f, "</UL>\n");
	    }
	// Javadoc compatibility: fairly sure this is what it does.
	if(java && toc_sections[k].sec == INTERFACE_SEC)
	    break;
	}
    fprintf(f, "%s", indexFooter.c_str());
    copyright(f);
}

void writeHIERrecJava(FILE *f, Entry *root)
{
    int i;

    for(i = 0; i < root->sublist.size(); i++)
	{
	if(root->sublist[i]->baseclasses.size() == 0)
	    if(root->sublist[i]->isClass() &&
		root->sublist[i]->proBaseclasses.size() == 0 &&
		root->sublist[i]->pubBaseclasses.size() == 0)
		ClassGraphWriter::writeJava(f, root->sublist[i], 0);
	writeHIERrecJava(f, root->sublist[i]);
	}
}

void writeHIERjava(FILE *f)
{
    if(header.length() == 0 && ownHeader.length() == 0)
        fprintf(f, "%s", hierHeader.c_str());
    else
        fprintf(f, "%s", header.c_str());

    fprintf(f, _("<H1>Hierarchy of classes</H1>\n"));
    fprintf(f, "<UL>\n");
    writeHIERrecJava(f, root);
    fprintf(f, "</UL>\n");
    fprintf(f, _("<I><A HREF=\"index%s\"> alphabetic index</A></I><P>"),
	htmlSuffix.c_str());
    copyright(f);
}

/** This class keeps track of overloading relationships. Insert mebers
    using addMember. It returns NULL in case this is a new Member, != 0
    otherwise.
*/
class MemberList {
	McHashTable<char *, Entry *> list;
    public:
	MemberList() : list((Entry *)NULL)
	    {}
	/** Add a new member. Returns NULL, if no compatible member
	    has yet occurred.
	*/
	Entry *addMember(Entry *e, Entry *father)
	    {
	    McString signature = e->name;
	    char *tmp;

	    if(language == LANG_JAVA || language == LANG_PHP)
		signature += e->args;
	    tmp = strdup(signature.c_str());
	    Entry **val = list.insert(tmp);
	    if(*val == 0)
		{
		*val = father;
		return 0;
		}
	    else
		{
#ifdef DEBUG
		if(verb)
		    printf(_("Member `%s' was there\n"), tmp);
#endif
		free(tmp);
		}
	    return *val;
	    }
};

void showSubMembers(FILE *f, Entry *entry);

void showMembers(Entry *e, FILE *f, int links, MemberList *ignore = 0)
{
    static struct {
	char *heading;
	int protection;
	int secMask;
    } sections[] = {
      { _("<DT><H3>Public Fields</H3><DD>"), PUBL, VARIABLE_SEC },
      { _("<DT><H3>Public Methods</H3><DD>"), PUBL, FUNCTION_SEC},
      { _("<DT><H3>Public Members</H3><DD>"), PUBL, ~(VARIABLE_SEC | FUNCTION_SEC) },
      { _("<DT><H3>Protected Fields</H3><DD>"), PROT, VARIABLE_SEC },
      { _("<DT><H3>Protected Methods</H3><DD>"), PROT, FUNCTION_SEC },
      { _("<DT><H3>Protected Members</H3><DD>"), PROT, ~(VARIABLE_SEC | FUNCTION_SEC) },
      { _("<DT><H3>Private Fields</H3><DD>"), PRIV, VARIABLE_SEC },
      { _("<DT><H3>Private Methods</H3><DD>"), PRIV, FUNCTION_SEC },
      { _("<DT><H3>Private Members</H3><DD>"), PRIV, ~(VARIABLE_SEC | FUNCTION_SEC) },
      { 0, 0, 0 }};

    int i;
    Entry *tmp, *type, *othersFather;
    MemberWriter *memberWriter;
    bool ignoreThisOne;

    fprintf(f, "<DL>\n");
    if(withTables && links)
	memberWriter = new MemberWriterTable();
    else
	memberWriter = new MemberWriter();
    for(i = 0; sections[i].heading; i++)
	if(withPrivate || sections[i].protection != PRIV)
	    {
	    memberWriter->startList(f, sections[i].heading, links);
	    for(tmp = e->sub; tmp; tmp = tmp->next)
		{
                type = tmp;
                if(tmp->section & MANUAL_SEC)
                    {
                    if(tmp->sub) 
                        type=tmp->sub;
                    else
                        continue; // skip empty sections
                    }     
		if(tmp->protection == sections[i].protection &&
		    (type->section & sections[i].secMask))
		    if(links ||
			(tmp->name != e->name && (tmp->name[0] != '~' ||
			    strcmp(tmp->name.c_str() + 1, e->name.c_str()) != 0)))
			{
			ignoreThisOne = false;
			if(ignore)
			    {
			    othersFather = ignore->addMember(tmp, e);
			    if(othersFather != 0 && othersFather != e)
				ignoreThisOne = true;
			    }
			bool withSub = (tmp->section == MANUAL_SEC && showMembersInTOC);
			if(!ignoreThisOne || links)
			    memberWriter->addMember(tmp, links, withSub);
			}
		}
	    memberWriter->endList();
	    }

    delete memberWriter;
    fprintf(f, "</DL>\n");
}

void writeInherited(Entry *k, FILE *f, MemberList *list = 0)
{
    int i;

    showMembers(k, f, 0, list);
    for(i = 0; i < k->baseclasses.size(); i++)
	{
	fprintf(f, _("<HR><H3>Inherited from <A HREF=\"%s\">%s</A>:</H3>\n"),
	    (k->baseclasses)[i]->fileName.c_str(),
	    (k->baseclasses)[i]->hname);
	writeInherited((k->baseclasses)[i], f, list);
	}
}

void writeHTMLList(FILE *f, McDArray<McString *> list, char *description)
{
    int i, k;

    if(list.size() > 0)
	{
	fprintf(f, "<DT><B>%s:</B><DD>", description);
	for(i = 0; i < list.size(); i++)
	    {
	    k = 0;
	    fprintf(f, "  ");
	    while(k < list[i]->length())
		fprintf(f, "%c", (*list[i])[k++]);
	    fprintf(f, "<BR>");
	    }
	}
}

/** This function writes the @-fields (except @memo, @name) of the
    specified entry
*/
void writeTags(FILE *f, Entry *entry)
{
    int i, k;

    fprintf(f, "<DL><DT>");

    writeHTMLList(f, entry->invariant, "Invariants");

    if(entry->exception.size())
	{
	fprintf(f, _("<DT><B>Throws:</B><DD>"));
	for(i = 0; i < entry->exception.size(); i++)
	    {
	    k = 0;
	    McString s;
	    while(k < entry->exception[i]->length() &&
		(myisalnum((*entry->exception[i])[k]) ||
		(*entry->exception[i])[k] == '_' ||
		(*entry->exception[i])[k] == '<' ||
		(*entry->exception[i])[k] == '>' ||
		(*entry->exception[i])[k] == ':'))
		s += (*entry->exception[i])[k++];
	    fprintf(f, "<B>%s</B> ", strToHtml(s, 0, entry, true, true));
	    while(k < entry->exception[i]->length())
		fprintf(f, "%c", (*entry->exception[i])[k++]);
	    fprintf(f, "<BR>");
	    }
	}

    if(entry->param.size())
	{
	fprintf(f, _("<DT><B>Parameters:</B><DD>"));
	for(k = 0; k < entry->param.size(); k++)
	    {
	    i = 0;
	    fprintf(f,"<B>");
	    while(i < entry->param[k]->length() &&
		(myisalnum((*entry->param[k])[i]) || 
		(*entry->param[k])[i] == '_'))
		fprintf(f, "%c", (*entry->param[k])[i++]);
	    fprintf(f, "</B> - ");

	    while(i < entry->param[k]->length())
		fprintf(f, "%c", (*entry->param[k])[i++]);
	    fprintf(f, "<BR>");
	    }
	}

    if(entry->field.size())
	{
	fprintf(f, _("<DT><B>Fields:</B><DD>"));
	for(k = 0; k < entry->field.size(); k++)
	    {
	    i = 0;
	    fprintf(f,"<B>");
	    while(i < entry->field[k]->length() &&
		(myisalnum((*entry->field[k])[i]) || 
		(*entry->field[k])[i] == '_'))
		fprintf(f, "%c", (*entry->field[k])[i++]);
	    fprintf(f, "</B> - ");

	    while(i < entry->field[k]->length())
		fprintf(f, "%c", (*entry->field[k])[i++]);
	    fprintf(f, "<BR>");
	    }
	}

    writeHTMLList(f, entry->retrn, _("Returns"));
    writeHTMLList(f, entry->precondition, _("Preconditions"));
    writeHTMLList(f, entry->postcondition, _("Postconditions"));
    writeHTMLList(f, entry->friends, _("Friends"));

    if(entry->author.length())
	fprintf(f, _("<DT><B>Author:</B><DD>%s\n"), entry->author.c_str());

    if(entry->version.length())
	fprintf(f, _("<DT><B>Version:</B><DD>%s\n"), entry->version.c_str());

    if(entry->deprecated.length())
	fprintf(f, _("<DT><B><I>Deprecated:</I></B><DD>%s\n"), entry->deprecated.c_str());

    if(entry->since.length())
	fprintf(f, _("<DT><B>Since:</B><DD>%s\n"), entry->since.c_str());

    if(entry->see.size())
	{
	fprintf(f, _("<DT><B>See Also:</B><DD>"));
	for(k = 0; k < entry->see.size(); k++)
	    if(entry->see[k]->length())
		fprintf(f, "%s<BR>", seeToHtml(*entry->see[k], entry));
	}

    fprintf(f, "<DD></DL><P>");
}

void writeDoc(FILE *f, Entry *entry)
{  
    if(entry->ownPage)
	return;
    int toBeDocumented = subEntryIsToBeDocumented(entry);
    Entry *tmp;

    if(toBeDocumented)
	{
	char *args = (entry->hargs);
	char *type = (entry->htype);
	printRefLabel(f, entry);
	fprintf(f, "<DT>");
	fprintf(f, "<IMG ALT=\"o\" BORDER=0 SRC=icon2.gif><TT><B>%s %s",
	    type, entry->hname);
	fprintf(f, "%s", args);	
	if(entry->pureVirtual)
	    fprintf(f, " = 0");
	fprintf(f, "</B></TT>\n");

	if(entry->doc.length() > 0)
	    fprintf(f, "<DD>%s\n", entry->hdoc);
	else
	    if(entry->memo.length() > 0)
		fprintf(f, "<DD>%s\n", entry->hmemo);

	writeTags(f, entry);
	}

    if(entry->sub)
	{
	fprintf(f, "<DL>\n");
	for(tmp = entry->sub; tmp; tmp = tmp->next)
	    writeDoc(f, tmp);
	fprintf(f, "</DL>\n");
	}
}

// Replace all occurrences of "pattern" in "str" with "value"
static McString substitute(const McString& str,
			   const McString& pattern,
			   const McString& value)
{
    McString result(str);
    int ndx;

    while ((ndx = result.index(pattern.c_str(), 0)) >= 0) {
	McString before(result, 0, ndx);
	McString after(result, ndx + pattern.length(),
	    result.length() - (ndx + pattern.length()));
	result = before;
	result += value;
	result += after;
    }

    return result;
}

// Given a template string and an entry, substitute entry-specific
// data (such as its type or name) and return the result.
static McString processTemplate(const McString& tmpl, const Entry *e)
{
    McString result(tmpl);

    result = substitute(result, "%file", e->file);
    result = substitute(result, "%fullname", e->fullName);
    result = substitute(result, "%name", e->name);
    result = substitute(result, "%type", e->type);

    return result;
}

int strlenNoHtml(char *in)
{
    int braces = 0;
    unsigned int i, count = 0;

    for(i = 0; i < strlen(in); i++)
	if((in[i] == '<') && (braces == 0))
	    braces = 1;
	else
	    if((in[i] == '>') && (braces == 1))
		braces = 0;
	    else
		if(braces == 0)
		    count++;

    return count;
}

void writeHeader(Entry *e, FILE *f)
{
    McString tmp;
    int blank_len, arg_len;
    char *args, *arg;

    if(header.length() == 0 && ownHeader.length() == 0)
	{
	fprintf(f, "%s", docType);
	fprintf(f, "<HTML>\n");
	fprintf(f, "<HEAD>\n");
	fprintf(f, "   <TITLE>%s %s</TITLE>\n", e->type.c_str(),
	    ENTRY_NAME(e).c_str());
	fprintf(f, "   <META NAME=\"GENERATOR\" CONTENT=\"DOC++ " DOCXX_VERSION "\">\n");
	fprintf(f, "%s", styleSheet.c_str());
	fprintf(f, "</HEAD>\n");
	tmp = processTemplate(pageHeader, e);
	}
    else
	tmp = processTemplate(header, e);
    fprintf(f, "%s\n", tmp.c_str());

    if(showFilenames && e->section != PACKAGE_SEC && e->section != MANUAL_SEC)
	fprintf(f, _("In file %s:"), e->file.c_str());

    if(e->section == PACKAGE_SEC)
	{
	e->getPackage(tmp);
	fprintf(f, _("<H2>Package %s</H2>"), tmp.c_str());
	}
    else
	{
	if(withTables)
	    {
	    fprintf(f, "<TABLE BORDER=0><TR>\n");
	    fprintf(f, "<TD VALIGN=TOP>");
	    }
	fprintf(f, "<H2>");
	if(language == LANG_JAVA)
	    {
	    e->getPackage(tmp);
	    if(tmp.length() < 1)
		{
		blank_len = strlen(e->htype) + strlen(e->hname) + 2;
		fprintf(f, "%s <A HREF=\"#DOC.DOCU\"> %s", e->htype, e->hname);
		}
	    else
		{
		blank_len = strlen(e->htype) + strlen(tmp.c_str()) + strlen(e->hname) + 2;
		fprintf(f, "%s <A HREF=\"#DOC.DOCU\">%s.%s", e->htype,
		    tmp.c_str(), e->hname);
	        }
	    }
	else
            {
	    blank_len = strlen(e->htype) + strlen(ENTRY_NAME(e).c_str()) + 1;
	    fprintf(f, "%s <A HREF=\"#DOC.DOCU\">", e->htype);
            if(e->section == CLASS_SEC ||
        	e->section == NAMESPACE_SEC ||
        	e->section == TYPEDEF_SEC ||
                e->section == UNION_SEC)
		{
                char	buf0[256], buf1[256];

                Entry*	p = e->parent;
                sprintf(buf0, e->name.c_str());
                while(p != 0 && p->section == NAMESPACE_SEC)
		    {
                    sprintf(buf1, "%s::%s", p->name.c_str(), buf0);
                    strcpy(buf0, buf1);
                    p = p->parent;
        	    }

                fprintf(f, "%s", buf0);
		}
	    else
                fprintf(f, "%s", ENTRY_NAME(e).c_str());
	    }

	fprintf(f, "</A></H2>");
	if(withTables)
	    fprintf(f, "</TD>");

	// wrap function declaration with long argument list
	if(e->hargs[0] == '(')
	    {
	    args = (char *)malloc(strlen(e->hargs) + 1);
	    strcpy(args, &(e->hargs[1]));
	    arg = strtok(args, ",");
	    arg_len = strlenNoHtml(arg);
	    if(withTables)
		fprintf(f, "<TD>");
	    fprintf(f, "<H2>(%s", arg);
	    while((arg = strtok(NULL, ",")) != NULL)
		{
		arg_len += strlenNoHtml(arg);
		if((arg_len + blank_len) < 62)
		    fprintf(f, ", %s", arg);
		else
		    {
		    fprintf(f, ",<BR>&nbsp;%s", arg);
		    arg_len = strlenNoHtml(arg);
		    }
		}
	    }
	fprintf(f, "</H2>");
	if(withTables)
	    fprintf(f, "</TD></TR></TABLE>\n");
	}
    htmlComment(f, e, MEMO);
}

void writeManPage(Entry *e, FILE *f)
{
    int i, numChilds, numParents = 0;
    Entry *c, *tmp;
    McString buf;
    MemberList list;
    MemberWriter memberWriter;

#ifdef DEBUG
    if(verb)
	printf(_("Writing page for `%s %s%s'\n"), e->type.c_str(),
	    e->fullName.c_str(), e->args.c_str());
#endif

    // the page header
    writeHeader(e, f);

    if(printClassDocBeforeGroup)
	{
	// the documentation
	fprintf(f, _("<HR>\n<H2>Documentation</H2>\n"));
	htmlComment (f, e, (DOC | MEMO));
	fprintf(f, "\n<A NAME=\"DOC.DOCU\"></A>\n");
	}

    if(e->isClass())            // is it really a class?
	{ 
	// the inheritance
	numChilds = e->pubChilds.size() + e->proChilds.size() + e->priChilds.size();
	if(language == LANG_JAVA)
	    numParents = e->pubBaseclasses.size()+ e->proBaseclasses.size() +
		e->otherPubBaseclasses.size() + e->otherProBaseclasses.size();
	else
	    numParents = e->pubBaseclasses.size() + e->priBaseclasses.size() +
		e->proBaseclasses.size() + e->otherPubBaseclasses.size() +
		e->otherPriBaseclasses.size() + e->otherProBaseclasses.size();

	if(numParents > 0 || numChilds > 0 || trivialGraphs)
	    {
	    fprintf(f, _("<HR>\n\n<H2>Inheritance:</H2>\n"));
	    ClassGraphWriter::write(f, e);
	    if(language == LANG_JAVA && e->implements.size() > 0)
		ClassGraphWriter::writeImplements(f, e);
	    }

	// the members 
	if(e->sub)
	    {
	    fprintf(f, "<HR>\n\n");
	    showMembers(e, f, 1, &list);
	    }
	}
    else                       // this is not a class
	{ 
	if(e->sub)
	    {
	    fprintf(f, "\n<HR>\n");
	    memberWriter.startList(f, " ", true);
	    for(tmp = e->sub; tmp; tmp = tmp->next)
		memberWriter.addMember(tmp, true, false);
	    }
	memberWriter.endList();
	}

    // the inherited members
    if(e->isClass())            // is it really a class?
	if(showInherited)
	    for(i = 0; i < e->baseclasses.size(); i++)
		{
		fprintf(f, _("<HR><H3>Inherited from <A HREF=\"%s\">%s</A>:</H3>\n"),
		    (e->baseclasses)[i]->fileName.c_str(),
		    (e->baseclasses)[i]->hname);
		writeInherited((e->baseclasses)[i], f, &list);
		}

    if(!printClassDocBeforeGroup)
	{
	// the documentation
	fprintf(f, "\n<A NAME=\"DOC.DOCU\"></A>\n");
	fprintf(f, _("<HR>\n<H2>Documentation</H2>\n"));
	htmlComment (f, e, (DOC | MEMO));
	}

    if(e->sub)
	{
	fprintf(f, "<DL>\n");
	for(tmp = e->sub; tmp; tmp = tmp->next)
	    writeDoc(f, tmp);
	fprintf(f, "</DL>\n");
	}

    // the childrens
    if(e->isClass())            // is it really a class?
	if(e->pubChilds.size() || e->proChilds.size())
	    {
	    fprintf(f, _("<HR>\n<DL><DT><B>Direct child classes:\n</B><DD>"));
	    c = e;
	    for(i = 0; i < c->pubChilds.size(); i++)
		fprintf(f, "<A HREF=\"%s\">%s</A><BR>\n",
		    c->pubChilds[i]->fileName.c_str(),
		    c->pubChilds[i]->hname);
	    for(i = 0; i < c->proChilds.size(); i++)
		fprintf(f, "<A HREF=\"%s\">%s</A><BR>\n",
		    c->proChilds[i]->fileName.c_str(),
		    c->proChilds[i]->hname);
	    fprintf(f, "</DL>\n\n");
	    }
	else
	    fprintf(f, _("\n<HR><DL><DT><B>This class has no child classes.</B></DL>\n\n"));

    writeTags(f, e);
    if(javaGraphs)
	buf = processTemplate(pageFooterJava, e);
    else
	buf = processTemplate(pageFooter, e);
    fprintf(f, "%s", buf.c_str());

    copyright(f);
}

static FILE *myOpen(const char *dir, const McString& name)
{
    FILE *f;
    McString buf = dir;

    buf += PATH_DELIMITER;
    buf += name;
#ifdef DEBUG
    if(verb)
	printf(_("Opening `%s' to write\n"), buf.c_str());
#endif
    if(!(f = fopen(buf.c_str(), "wb")))
	{
	fprintf(stderr, _("Cannot open `%s'\n"), buf.c_str());
	exit(-1);
	}
    return f;
}

void makeHtmlNames(Entry *entry)
{
    Entry *tmp;
    entry->hname = strToHtml(entry->name, 0, entry, false);
    if(entry->section == EMPTY_SEC)
	entry->section = PACKAGE_SEC;

    if(entry->sub)
	for(tmp = entry->sub; tmp; tmp = tmp->next)
	    makeHtmlNames(tmp);
}

// This creates names for all those with their own pages
void makeFileNames(Entry *entry)
{
    Entry *tmp;
    McString file;

    if(entry->ownPage)
	{
	if(language == LANG_JAVA)
	    {
	    McString pack;
	    if(entry->section == PACKAGE_SEC)
		file = "package-";
	    entry->getPackage(pack);
	    if(pack.length())
		{
		file += pack;
		if(entry->section != PACKAGE_SEC)
		    file += '.'; 
		}
	    }
	if(language != LANG_JAVA || entry->section != PACKAGE_SEC)
	    {
	    if(entry->name.length() > 0)
		file += entry->name;
	    else
		if(entry->section == UNION_SEC) // Struct, union, enum
		    {
		    file += entry->type;
#ifdef DEBUG
		    if(verb)
			{	// This is for unnamed enums / unions etc.
			printf(_("Making name from type for:\n"));
			entry->dump(stdout);
			}
#endif
		    }
	    }
	entry->fileName = makeFileName(file, entry);
	}
    if(entry->sub)
	for(tmp = entry->sub; tmp; tmp = tmp->next)
	    makeFileNames(tmp);
}

// This creates filenames for items from their parents
void inheritFileNames(Entry *entry)
{
    Entry *tmp;

    if(!entry->ownPage)
	{
	tmp = entry;
	while(tmp->parent && tmp->fileName.length() == 0)
	    tmp = tmp->parent;
	if(tmp != entry)
	    {
	    entry->fileName = tmp->fileName.c_str();
#ifdef DEBUG
	    if(verb)
		printf(_("Copied `%s %s%s' into `%s'\n"), entry->type.c_str(),
		    entry->fullName.c_str(), entry->args.c_str(),
		    entry->fileName.c_str());
#endif
	    }
	}

    if(entry->sub)
	for(tmp = entry->sub; tmp; tmp = tmp->next)
	    inheritFileNames(tmp);
}

// This marks those names that can't be attached to a class
// It also junks redundant floating comments as a bad job...
void relocateNames(Entry *root)
{
    Entry *e, *next;
    e = root->sub;
    while(e)
	{
	next = e->next;
	if(!e->ownPage)
	    if(useGeneral)
		{
#ifdef DEBUG
		if(verb)
		    printf(_("Warning: `%s %s%s' hanging from root with no page\n"),
			e->type.c_str(), e->fullName.c_str(), e->args.c_str());
#endif
		e->general = true;
		}
	    else
		{
		root->removeSub(e);
		if(!fastNotSmall)
		    delete(e);
		}
	e = next;
	}	
}

// Decide what entries should have their's own page
void decideAboutOwnPages(Entry *entry)
{
    Entry *tmp;

    if(entry->section == EMPTY_SEC)
	entry->section = MANUAL_SEC;

    // The Quantel extension bit is to stop annoying stuff like the RCS info
    // (often in a FED fold) from turning up in the data...
    if(entry->sub || (entry->doc.length() > 1 && !QuantelExtn) || entry->isClass())
	entry->ownPage = true;

    if(entry->sub && (entry->section == MANUAL_SEC ||
	entry->section == PACKAGE_SEC ||
	entry->section == NAMESPACE_SEC || (entry->section & CLASS_SEC)))
	for(tmp = entry->sub; tmp; tmp = tmp->next)
	    if(!(entry->section & CLASS_SEC) || ((entry->section & CLASS_SEC) &&
		(tmp->section & CLASS_SEC)))
		decideAboutOwnPages(tmp);
}

void makeHtml(Entry *entry)
{
    Entry *tmp;
    int i;

    entry->hmemo = strToHtml(entry->memo, 0, entry, false);
    entry->hdoc = strToHtml(entry->doc, 0, entry, false);
    entry->hargs = strToHtml(entry->args, 0, entry, true, true);
    entry->htype = strToHtml(entry->type, 0, entry, true, true);
    entry->author = strToHtml(entry->author, 0, entry, false);
    entry->version = strToHtml(entry->version, 0, entry, false);
    entry->deprecated = strToHtml(entry->deprecated, 0, entry, false);
    entry->since = strToHtml(entry->since, 0, entry, false);

    for(i = 0; i < entry->retrn.size(); i++)
	*entry->retrn[i] = strToHtml(*entry->retrn[i], 0, entry, false);

    for(i = 0; i < entry->param.size(); i++)
	*entry->param[i] = strToHtml(*entry->param[i], 0, entry, true);

    for(i = 0; i < entry->field.size(); i++)
	*entry->field[i] = strToHtml(*entry->field[i], 0, entry, false);

    for(i = 0; i < entry->exception.size(); i++)
	*entry->exception[i] = strToHtml(*entry->exception[i], 0, entry, false);

    for(i = 0; i < entry->invariant.size(); i++)
	*entry->invariant[i] = strToHtml(*entry->invariant[i], 0, entry, false);

    for(i = 0; i < entry->precondition.size(); i++)
	*entry->precondition[i] = strToHtml(*entry->precondition[i], 0, entry, false);

    for(i = 0; i < entry->postcondition.size(); i++)
	*entry->postcondition[i] = strToHtml(*entry->postcondition[i], 0, entry, false);

    for(i = 0; i < entry->friends.size(); i++)
	*entry->friends[i] = strToHtml(*entry->friends[i], 0, entry, true);

    if(entry->sub)
	for(tmp = entry->sub; tmp; tmp = tmp->next)
	    makeHtml(tmp);
}

void writePageSub(FILE *f, Entry *e)
{
    McString htype, hargs;

#ifdef DEBUG
    if(verb)
	printf(_("Writing sub for `%s %s%s' to file `%s'\n"), e->type.c_str(),
	    e->fullName.c_str(), e->args.c_str(), e->fileName.c_str());
#endif

    if(withTables)
	fprintf(f, "<TR><TD VALIGN=TOP>");
    else
	fprintf(f, "<DT>\n");

    // Show the signature if this is a leaf node (meaning its full
    // documentation will be on the current page)
    if(e->fileName.length() == 0 || e->general)
	{
	htype = e->htype;
	hargs = e->hargs;
	}

    // Show the bullet symbol.
    fprintf(f, "\n<IMG ALT=\"o\" BORDER=0 SRC=icon1.gif>");

    if(htype.length() > 0)
	fprintf(f, "%s", htype.c_str());

    // label the symbol being here
    fprintf(f, "<A NAME=\"%s\"></A>\n", e->name.c_str());

    // create a cross reference to the symbol if elsewhere.
    if(!e->general && e->fileName.length() > 0)
	fprintf(f, "<A HREF=%s><B>%s</B></A>", e->fileName.c_str(),
	    e->hname);
    else
	fprintf(f, "<B>%s</B>", e->hname);

    if(hargs.length() > 0)
	fprintf(f, "%s", hargs.c_str());

    if(withTables)
	{
	fprintf(f, "</TD><TD><BR>\n");
	if(e->hmemo && strlen(e->hmemo) > 0)
	    fprintf(f, "<I>%s</I>\n", e->hmemo);
	fprintf(f, "</TD></TR>");
	}
    else
	if(e->hmemo && strlen(e->hmemo) > 0)
	    fprintf(f, "<DD><I>%s</I>\n", e->hmemo);
}

void writeManPageRec(const char *dir, Entry *e)
{
    Entry *tmp;
    McString buf;
    FILE *f;

#ifdef DEBUG
    if(verb)
	printf(_("Writing `%s %s%s' to file `%s'\n"), e->type.c_str(),
	    e->fullName.c_str(), e->args.c_str(), e->fileName.c_str());
#endif

    if(e->general)
	writePageSub(generalf, e);

    if(e->ownPage)
	{
#ifdef DEBUG
	if((e->fileName == htmlSuffix ||
	    e->fileName == (const char *)".2.html") && verb)
	    {
	    fprintf(stderr, _("Warning: weird filename `%s' for `%s %s%s'\n"),
		e->fileName.c_str(), e->type.c_str(), e->fullName.c_str(),
		e->args.c_str());
	    e->dump(stdout);
	    }
#endif
	if(!(f = myOpen(dir, e->fileName)))
	    {
	    fprintf(stderr, _("Cannot open `%s' for writing\n"), e->fileName.c_str());
	    return;
	    }
	if(e->section != MANUAL_SEC && e->section != PACKAGE_SEC)
	    writeManPage(e, f);
	else
	    {
	    writeHeader(e, f);

	    if(printGroupDocBeforeGroup)
		{
		fprintf(f, "<A NAME=\"DOC.DOCU\"></A>\n");
		htmlComment(f, e, (DOC | MEMO));
		}

	    if(e->sub)
		{
		if(withTables)
		    fprintf(f, "\n<TABLE>\n");
		else
		    fprintf(f, "\n<HR>\n<DL>\n");
		for(tmp = e->sub; tmp; tmp = tmp->next)
		    writePageSub(f, tmp);
		if(withTables)
		    fprintf(f, "\n</TABLE>\n");
		else
		    fprintf(f, "</DL>\n");
		}

	    if(!printGroupDocBeforeGroup)
		{
		fprintf(f, "<A NAME=\"DOC.DOCU\"></A>\n");
		htmlComment(f, e, (DOC | MEMO));
		}

	    writeTags(f, e);
	    buf = processTemplate(pageFooter, e);
	    fprintf(f, "%s", buf.c_str());
	    copyright(f);
	    }
	fclose(f);
	}
    if(e->sub && (e->section == MANUAL_SEC || e->section == PACKAGE_SEC ||
	e->section == NAMESPACE_SEC || (e->section & CLASS_SEC)))
	for(tmp = e->sub; tmp; tmp = tmp->next)
	    if(!(e->section & CLASS_SEC) || ((e->section & CLASS_SEC) &&
		(tmp->section & CLASS_SEC)))
		writeManPageRec(dir, tmp);
}

static void dumpFile(const char *dir, const char *name, const unsigned char *data, int size)
{
    FILE *f = myOpen(dir, name);
    fwrite(data, 1, size, f);
    fclose(f);
}

void readTemplates()
{
    FILE *f;
    int c;

    if((f = fopen("indexHeader.inc", "r")))
	{
	indexHeader = " ";
	while((c = fgetc(f)) != EOF)
	    indexHeader += c;
	fclose(f);
	}
    if((f = fopen("indexFooter.inc", "r")))
	{
	indexFooter = " ";
	while((c = fgetc(f)) != EOF)
	    indexFooter += c;
	fclose(f);
	}
    if((f = fopen("hierHeader.inc", "r")))
	{
	hierHeader = " ";
	while((c = fgetc(f)) != EOF)
	    hierHeader += c;
	fclose(f);
	}
    if((f = fopen("hierFooter.inc", "r")))
	{
	hierFooter = " ";
	while((c = fgetc(f)) != EOF)
	    hierFooter += c;
	fclose(f);
	}
    if((f = fopen("classHeader.inc", "r")))
	{
	pageHeader = " ";
	while((c = fgetc(f)) != EOF)
	    pageHeader += c;
	fclose(f);
	}
    if((f = fopen("classFooter.inc", "r")))
	{
	pageFooter = " ";
	while((c = fgetc(f)) != EOF)
	    pageFooter += c;
	fclose(f);
	}
}

void doHTML(const char *dir, Entry *root)
{
    FILE *f;
    Entry *tmp;
    McString buf;
    int c;
    bool haveManualRootSec = false;

    buildHeaders();
    buildFooters();

    if(makedir(dir, 0755) != 0)
	if(errno == EEXIST)
	    {
	    FILE *exist = fopen(dir, "a");
	    if(exist)
		{
		fprintf(stderr, _("`%s' directory already exists\n"), dir);
		fclose(exist);
		}
	    }
	else
	    fprintf(stderr, _("Could not create `%s' directory\n"), dir);

    decideAboutOwnPages(root);
    makeFileNames(root);

    if(root->name.length() == 0 || root->section != MANUAL_SEC)
	if(root->sublist.size() == 1 && root->sublist[0]->section == MANUAL_SEC)
	    {
	    root->sublist[0]->fileName = "index";
	    root->sublist[0]->fileName += htmlSuffix;
	    haveManualRootSec = true;
	    }

    // This is as items under root with no page inherit this name
    root->fileName = GENERAL_NAME;
    root->fileName += htmlSuffix;
    inheritFileNames(root);
    relocateNames(root);

    if(root->name.length() > 0 && root->section == MANUAL_SEC)
	{
	root->fileName = "index";
	root->fileName += htmlSuffix;
	haveManualRootSec = true;
	}
#if 0
    if(verb)
	root->dump(stdout, true);
#endif

    if(verb)
	printf(_("Converting DOC++ to HTML...\n"));

    if(root->section == EMPTY_SEC)
	root->section = MANUAL_SEC;
    for(tmp = root; tmp; tmp = tmp->next)
	makeHtmlNames(tmp);
    for(tmp = root; tmp; tmp = tmp->next)	// This takes the bulk of the time
	makeHtml(tmp);

    readTemplates();

    if(ownFooter.length() > 0)
	{
	FILE *in = fopen(ownFooter.c_str(), "r");
	if(!in && ownFooter != "none" && ownFooter.length() > 1)
	    fprintf(stderr, _("Warning: Can't open `%s', producing no footer.\n"),
		ownFooter.c_str());
	else
	    {
	    while((c = fgetc(in)) != EOF)
		footer += (char)c;
	    fclose(in);
	    }
	}

    if(ownHeader.length() > 0)
	{
	FILE *in = fopen(ownHeader.c_str(), "r");
	if(!in && ownHeader != "none" && ownHeader.length() > 1)
	    fprintf(stderr, _("Warning: Can't open `%s', producing no header.\n"),
		ownHeader.c_str());
	else
	    {
	    while((c = fgetc(in)) != EOF)
		header += (char)c;
	    fclose(in);
	    }
	}

    if(verb)
	printf(_("Writing files...\n"));

    dumpFile(dir, "icon1.gif", blueBall, sizeof(blueBall));
    dumpFile(dir, "icon2.gif", greyBall, sizeof(greyBall));

    if(javaGraphs && !noClassGraph && relevantClassGraphs(root))
	{
	dumpFile(dir, "ClassGraph.class", ClassGraph_class,
	    sizeof(ClassGraph_class));
	dumpFile(dir, "ClassGraphPanel.class", ClassGraphPanel_class,
	    sizeof(ClassGraphPanel_class));
	dumpFile(dir, "ClassLayout.class", ClassLayout_class,
	    sizeof(ClassLayout_class));
	dumpFile(dir, "NavigatorButton.class", NavigatorButton_class,
	    sizeof(NavigatorButton_class));
	}

    if(verb)
	printf(_("Writing TOC...\n"));

    // Table of contents
    if(haveManualRootSec)
	buf = "toc";
    else
	buf = "index";
    buf += htmlSuffix;
    f = myOpen(dir, buf);
    writeTOC(f);
    fclose(f);

    if(!noClassGraph && relevantClassGraphs(root))
	// Class heirarchy
	{
	if(verb)
	    printf(_("Writing Class Hierarchy...\n"));

	buf = "HIER";
	buf += htmlSuffix;
	f = myOpen(dir, buf);
	writeHIER(f);
	fclose(f);

	// Java class heirarchy
	if(javaGraphs)
	    {
	    buf = "HIERjava";
	    buf += htmlSuffix;
	    f = myOpen(dir, buf);
	    writeHIERjava(f);
	    fclose(f);  
	    }
	}

    /* Render all the rest of the pages to HTML, and output a 'General' file
       of floating #defines, global variables / functions etc.
    */

    // We don't want root page coming out as .html !
    root->ownPage = false;
    root->general = false;

    buf = GENERAL_NAME;
    buf += htmlSuffix;
    generalf = myOpen(dir, buf);
    if(header.length() == 0 && ownHeader.length() == 0)
        fprintf(generalf, "%s", generalHeader.c_str());
    else
        fprintf(generalf, "%s", header.c_str());
    if(withTables)
	fprintf(generalf, "\n<TABLE>\n");
    else
	fprintf(generalf, "\n<DL>\n");

    // Recursively write all pages
    writeManPageRec(dir, root);

    if(withTables)
	fprintf(generalf, "\n</TABLE>\n");
    else
	fprintf(generalf, "</DL>\n");

    fprintf(generalf, "%s", pageFooter.c_str());
    copyright(generalf);

    fclose(generalf);
}      
