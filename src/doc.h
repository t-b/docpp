/*
  doc.h

  Copyright (c) 1996 Roland Wunderling, Malte Zoeckler
  Copyright (c) 1998 Michael Meeks
  Copyright (c) 1999-2002 Dragos Acostachioaie

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

#ifndef _DOC_H
#define _DOC_H

#include <stdio.h>

#include "McDArray.h"
#include "McHashTable.h"
#include "McString.h"
#include "config.h"
#include "portability.h"

#include <gettext.h>
#ifdef ENABLE_NLS
#define _(x) gettext(x)
#else
#define _(x) (x)
#endif

#define DOCXX_VERSION	"3.4.10"

extern FILE			*out;			// output FILE
extern char			language;		// language

extern bool			onlyDocs;		// --all
extern bool			commentExtn;		// --c-comments
extern McString			configFile;		// --config FILE
extern bool			HTMLsyntax;		// --html
extern McString			fileList;		// --input FILE
extern bool			java;			// --java
extern bool			php;			// --php
extern bool			trivialGraphs;		// --trivial-graphs
extern bool			noDefines;		// --no-define
extern bool			withPrivate;		// --private
extern bool			fastNotSmall;		// --quick
extern bool			QuantelExtn;		// --quantel
extern bool			internalDoc;		// --internal-doc
extern bool			doTeX;			// --tex
extern bool			upArrows;		// --upwards-arrows
extern bool			verb;			// --verbose
extern bool			idl;
extern bool			doDOCBOOK;
extern bool			doDOCBOOKXML;
extern bool			scanIncludes;		// --scan-includes
extern McDArray<McString *>	inputFiles;

extern bool			withTables;		// --tables
extern bool			withBorders;		// --tables-borders
extern McString			ownFooter;		// --footer FILE
extern McString			outputDir;		// --dir DIR
extern bool			showFilenames;		// --filenames
extern bool			showFilePath;		// --filenames-path
extern bool			noGifs;			// --no-gifs
extern bool			forceGifs;		// --gifs
extern bool			showInherited;		// --no-inherited
extern bool			javaGraphs;		// --no-java-graphs
extern bool			alwaysPrintDocSection;	// --no-members
extern bool			showMembersInTOC;	// --full-toc
extern bool			useGeneral;		// --no-general
extern bool			sortEntries;		// --sort
extern McString			ownHeader;		// --header FILE
extern bool			printClassDocBeforeGroup;	// --before-group
extern bool			printGroupDocBeforeGroup;	// --before-class
extern McString			htmlSuffix;		// --suffix SUFFIX
extern McString			htmlStyleSheet;		// --stylesheet FILE

extern bool			onlyClassGraph;		// --class-graph
extern McString			texFile;		// --env FILE
extern bool			generateIndex;		// --index
extern McString			texOption;		// --style OPTION
extern McDArray<McString *>	texPackages;		// --package FILE
extern McString			texTitle;		// --title FILE
extern int			depthTOC;		// --depth DEPTH
extern bool			noLatex;		// --no-env
extern bool			noClassGraph;		// --no-class-graph
extern McString			texOutputName;		// --output FILE
extern bool			doListing;		// --source
extern bool			hideIndexOnEverySection;// --no-index

//{{{ Commented with a Quantel folding extension
enum _SectionType
{
    EMPTY_SEC		= 1,
    MANUAL_SEC		= 2,
    VARIABLE_SEC	= 4,
    FUNCTION_SEC	= 8,
    MACRO_SEC		= 16,
    CLASS_SEC		= 32,
    UNION_SEC		= 64,
    INTERFACE_SEC	= 128,
    PACKAGE_SEC		= 256,
    TYPEDEF_SEC		= 512,
    NAMESPACE_SEC	= 1024,
    ALL_SEC		= 0x0fff
};
//}}}

enum PROTECTION
{
    PUBL = 'l',
    PROT = 't',
    PRIV = 'v'
};

enum LANGUAGE
{
    LANG_UNKNOWN	= '?',
    LANG_JAVA		= 'j',
    LANG_CXX		= 'c',
    LANG_IDL		= 'i',
    LANG_PHP		= 'p'
};

/** An entry in the documentation database.

    This class is the primary data item created when scanning source. Each
    documented item, whether a class, function, or other item, is given its
    own object
*/
class Entry
{
public:
    /// Constructor
    Entry();

    /** Adds `name' to the list of base classes and to the list of `name''s
	subclasses.
    */
    void addBaseClass(const char *name, PROTECTION state);

    void addBaseClass(Entry *name, PROTECTION state);

    void addSubEntry(Entry *e);

    void clear();

    void dumpNumber(FILE *);

    void dumpNumber(McString &);

    /// Dump all class data
    void dump(FILE *foo, bool recursive = false);

    void findBases();

    Entry *findSub(const McString& name);
    Entry *findSub(const McString& name, int first, int last);

    void getPackage(McString &);

    /// Is this a class or an interface?
    bool isClass();

    /** Creates a fully qualified name with respect to the
        current import and package statements (Java-only)
    */
    void makeFullName(McString &name);

    /// Creates the full name containing the whole inheritance tree
    void makeFullName();

    /// Creates the base classes arrays
    void makeRefs();

    void makeSubList();

    Entry* newSubEntry();

    /// Remove the specified subentry
    void removeSub(Entry *);

    /// Entry type (class, function, etc.)
    unsigned short	section;

    /// True if this entry can be tied to a class
    bool		general;

    /// Access right (pulic, protected or private)
    char		protection;

    /// True if this entry will get it's own HTML page
    bool		ownPage;

    /// The parent of this entry
    Entry*		parent;

    /// The next entry at this level
    Entry*		next;

    /// Linked list of sub-entries at this level
    Entry*		sub;

   /// Indicates if the method is pure virtual
   bool			pureVirtual;

    /// Return type of the entry
    McString		type;

    /// Name of the entry
    McString		name;

    /// Full name of the entry (including the inheritance)
    McString		fullName;

    /// Arguments for the entry
    McString		args;

    /// Memo section documentation
    McString		memo;

    /// Main documentation
    McString		doc;

    McString		program;

    /// Author info
    McString		author;

    /// Version info
    McString		version;

    /// "Deprecated" info
    McString		deprecated;

    /// "Since" info
    McString		since;

    /// Array of "see also" notations
    McDArray<McString*> see;

    /// Array of parameters documentation
    McDArray<McString*> param;

    /// Array of fields documentation
    McDArray<McString*> field;

    /// Array of thrown exceptions documentation
    McDArray<McString*> exception;

    /// Array of return values documentation
    McDArray<McString*> retrn;

    /// Array of preconditions documentation
    McDArray<McString*> precondition;

    /// Array of postconditions documentation
    McDArray<McString*> postcondition;

    /// Array of invariants documentation
    McDArray<McString*> invariant;

    /// Array of friends' documentation
    McDArray<McString*> friends;

    /// Array of sub-entries to this one
    McDArray<Entry*>	sublist;

    McDArray<Entry*>	pubChilds;
    McDArray<Entry*>	proChilds;
    McDArray<Entry*>	priChilds;
    McDArray<Entry*>	baseclasses;		// to be removed !!!
    McDArray<Entry*>	pubBaseclasses;
    McDArray<Entry*>	proBaseclasses;
    McDArray<Entry*>	priBaseclasses;

    /// base classes that are not documented
    McDArray<McString*>	otherPubBaseclasses;
    McDArray<McString*>	otherProBaseclasses;
    McDArray<McString*>	otherPriBaseclasses;
    McDArray<McString*>	implements;
    McDArray<McString*>	extends;
    McDArray<McString*>	import;
    Entry*		currentParent;		// for temporary use in TeX output

    /** If this member overrides something from a baseclass, this points to
        the overridden member.
    */
    Entry*		override;

    McString		fileName;
    int			startLine;

    // These are the html-ified strings
    char*		htype;
    char*		hname;
    char*		hargs;
    char*		hmemo;
    char*		hsee;
    char*		hdoc;

    McString		file;
    int			line;
    int			number;
    int			subnumber;

    /// Is this entry going to be docified?
    bool		docify;

    bool		fromUnknownPackage;
};

struct namespace_entry {
    char *name;
    int innerCurlyCount;
};

extern Entry*	root;
extern void	printYYDOC(Entry *cur, const char *str, int escPrcnt = 1);
extern void	printYYDOCdbsgml(Entry *cur, const char *str, int escPrcnt = 1);
extern void	printYYDOCdbxml(Entry *cur, const char *str, int escPrcnt = 1);

// parsing comands
extern void	listing    (char *str);
extern void	usermanDBsgml (char *str, Entry *root);
extern void	usermanDBxml  (char *str, Entry *root);
extern void	usermanTeX (char *str, Entry *root);
extern void	parseCpp   (Entry *rt);
extern void	parseJava  (Entry *rt);
extern void	parsePHP   (Entry *rt);
extern void	parseDoc   (Entry *rt);
extern void	parseConfig(McString s);

extern void	getRefNames(McDArray<McString*>, const char *);
extern Entry*	getRefEntry(const char *name, Entry *entry);
extern Entry*	getRefEntry(McString &name, Entry *entry);
extern void	makeSubLists(Entry *rt);
extern void	entry2link(McString& u, Entry *ref,const char *linkname = 0);
extern Entry*	findEntry(Entry *start, const char *fullName, unsigned short section);
extern void	setupLanguageHash();
extern void	mergeEntries(Entry *root);
extern int	getNumChildren(Entry *tp);
extern void	checkPackages(Entry *tp);
extern void	reNumber(Entry *tp);
extern bool	relevantClassGraphs(Entry *tmp);

extern void	readfile(McString *in, const char *file, int startLine,
		    const McString& directory = "", int scanSubDirs = 0);

#define	HAS_BASES(entry)			\
	(entry->otherPubBaseclasses.size() ||	\
	entry->otherProBaseclasses.size() ||	\
	entry->pubBaseclasses.size() ||		\
	entry->proBaseclasses.size())

#define	MAKE_DOC(entry)						\
	((entry->name.length() > 0 ||				\
	entry->type.length() > 0 ||				\
	entry->args.length() > 2 ||				\
	entry->sub != 0 ||					\
	entry->see.size() > 0 ||				\
	entry->author.length() > 0 ||				\
	entry->version.length() > 0 ||				\
	entry->deprecated.length() > 0 ||			\
	entry->since.length() > 0 ||				\
	entry->param.size() > 0 ||				\
	entry->field.size() > 0 ||				\
	entry->retrn.size() > 0 ||				\
	entry->exception.size() > 0 ||				\
	entry->precondition.size() > 0 ||			\
	entry->postcondition.size() > 0 ||			\
	entry->invariant.size() > 0 ||				\
	entry->friends.size() > 0 ||				\
	entry->doc.length() > 0 ||				\
	entry->proChilds.size() > 0 ||				\
	entry->pubChilds.size() > 0 ||				\
	(entry->priChilds.size() > 0 && withPrivate) ||		\
	(entry->parent != 0 && entry->parent->parent == 0) ||	\
	HAS_BASES(entry)) && (!(entry->fromUnknownPackage)))

#define ENTRY_NAME(entry)					\
	((entry->parent != 0 &&					\
	(entry->parent->section == CLASS_SEC ||			\
	entry->parent->section == UNION_SEC ||			\
	entry->parent->section == TYPEDEF_SEC ||		\
	entry->parent->section == INTERFACE_SEC ||		\
	entry->parent->section == PACKAGE_SEC ||		\
	entry->parent->section == NAMESPACE_SEC)) ?		\
	entry->fullName :					\
	McString(entry->hname))

#endif
