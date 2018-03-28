/*
  java.ll

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

%{

#include <assert.h>
#include <ctype.h>
#include <iostream>
#include <stdio.h>

#include "doc.h"

static const char	*inputString;
static int		inputPosition;
static int		lastContext;
static int		bracketCount  = 0;
static Entry		*current_root = 0;
static Entry		*global_root  = 0;
static Entry		*current      = 0;
// this one trackes whether we're looking for first-level classes (true)
// or members and nested classes (false).
static bool		findClasses  = false;

static int		yyLineNr = 0;
static char		yyFileName[264];

static char		ssEndChar;
static int		ssSave;

// these two variables count brackets in two new flex-start-conditions.
static int		skip_to_semi_count = 0;
static int		skip_to_bracket_count = 0;

extern void msg(const char *__fmt, ...);
#ifdef DEBUG
extern void debug(const char *__fmt, ...);
#endif

static void lineCount()
{
    for(const char *c = yytext; *c; ++c)
	yyLineNr += (*c == '\n');
}

#undef	YY_INPUT
#define	YY_INPUT(buf, result, max_size) result = yyread(buf, max_size);

static int yyread(char *buf, int max_size)
{
    int c = 0;

    while(c < max_size && inputString[inputPosition])
	{
	*buf = inputString[inputPosition++];
	c++;
	buf++;
	}
    return c;
}

%}

%x	FindClasses
%x	ClassName
%x	ClassBody
%x	Package
%x	Import
%x	Extends
%x	Implements

%x	FindMembers
%x	Member
%x	SkipToBracket
%x	SkipToSemi
%x	Args
%x	Function
%x	Throws

%x	Comment
%x	SkipComment
%x	SkipString

%x	JavaDoc
%x	See
%x	Author
%x	Version
%x	Param
%x	Return
%x	Exception
%x	Precondition
%x	Postcondition
%x	Invariant

%x	File

%%
<*>\x06[^\x06]*\x06			{
					int i;
					yyLineNr = 1;
					if(bracketCount != 0 || YY_START != FindClasses)
					    msg("Warning end of duff file: {} %d, %d\n",
						bracketCount, YY_START);
					for(i = 0; yytext[i + 1] != 6; i++)
					    yyFileName[i] = yytext[i + 1];
					yyFileName[i] = 0;
					current_root  = global_root;
					}

<FindClasses>^[ \t]*"package"[ \t]*	{
					current_root = global_root;
					BEGIN(Package);
					}

<FindClasses>^[ \t]*"import"[ \t]*	{
					BEGIN(Import);
					}

<FindClasses>[ \t]*"private"[ \t]+	{
					current->protection = PRIV;
					if(current->type.length() == 0)
					    current->type += "private ";
					}

<FindClasses>[ \t]*"public"[ \t]+	{
					current->protection = PUBL;
					if(current->type.length() == 0)
					    current->type += "public ";
					}

<FindClasses>[ \t]*"protected"[ \t]+	{
					current->protection = PROT;
					if(current->type.length() == 0)
					    current->type += "protected ";
					}

<FindClasses>[ \t]*"class"[ \t]+	{
					current->section = CLASS_SEC;
					current->type += "class";
					findClasses = true;
					BEGIN(ClassName);
					}

<FindClasses>[ \t]*"interface"[ \t]+	{
					current->section = INTERFACE_SEC;
					current->type += "interface";
					BEGIN(ClassName);
					}

<Package>[a-z_A-Z0-9]*			{
					Entry *find = current_root->sub;
					for(; find; find = find->next)
					    if(find->section == PACKAGE_SEC)
						if(find->name == (const char*)yytext)
						    break;
					if(find == 0)
					    {
					    find = new Entry;
					    find->section = PACKAGE_SEC;
					    find->name = yytext;
					    current_root->addSubEntry(find);
					    }
					current_root = find;
					}

<Package>;				{
					BEGIN(FindClasses);
					if(current->doc.length())
					    {
					    current_root->doc = current->doc;
					    current->doc.clear();
					    }
					}

<Import>[a-z_A-Z0-9.*]*			{
					current->import.append(new McString(yytext));
					}

<Import>;				{
					BEGIN(FindClasses);
					}

<ClassName>[ \t]*"extends"[ \t]*	{
					BEGIN(Extends);
					}

<ClassName>[ \t]*"implements"[ \t]*	{
					BEGIN(Implements);
					}

<ClassName>[ \t]*"{"[ \t]*		{
					BEGIN(ClassBody);
    					current->file = yyFileName;
					current->startLine = yyLineNr;
					}

<ClassName>[a-z_A-Z0-9]+		{
					current->name = yytext;
					msg("found class `%s'", yytext);
					}

<Extends>[a-z_A-Z.0-9]+			{
					current->extends.append(new McString(yytext));
					BEGIN(ClassName);
					}

<Implements>[a-z_A-Z.0-9]+		{
					current->implements.append(new McString(yytext));
					BEGIN(ClassName);
					}

<Implements>[a-z_A-Z.0-9]+[\n \t]*","[\n \t]*	{
					McString *tmp = new McString(yytext);
					lineCount();
                                        tmp->remove(tmp->length() - 1);
                                        while(!isalpha(tmp->last()))
					    tmp->remove(tmp->length() - 1);
					current->implements.append(tmp);
					BEGIN(Implements);
					}

<ClassBody>[ \t\n]*"/*""*"*[ \t]*	{
					current->program += yytext;
					lineCount();
					lastContext = YY_START;
					BEGIN(Comment);
					}

<ClassBody>"{"				{
					current->program += yytext;
					++bracketCount;
					}

<ClassBody>"}"				{
					if(bracketCount)
					    {
					    current->program += yytext;
					    --bracketCount;
					    }
					else
					    {
					    if(current->section == CLASS_SEC &&
						!current->extends.size())
						    current->extends.append(new McString("java.lang.Object"));
					    current_root->addSubEntry(current);
					    current = new Entry;
					    if(findClasses)
						BEGIN(FindClasses);
					    else
						BEGIN(FindMembers);
					    }
					}

<ClassBody>\n				{
					current->program += yytext;
					yyLineNr++;
					}

<ClassBody>\/\/.*\n			{
					current->program += yytext;
					}

<ClassBody>[\"\']			{
					ssEndChar = *yytext;
					ssSave = 0;
					lastContext = YY_START;
					BEGIN(SkipString);
					}

<ClassBody>.				{
					current->program += yytext;
					}

<FindMembers>";"			{
					current->name.clear();
					current->type.clear();
					current->args.clear();
					current->program.clear();
					current->doc.clear();
					current->section = EMPTY_SEC;
					}

<FindMembers>[ \t]*"private"[ \t]+	{
//
// Here is the major change:
// <FindMembers> now scans until (including)
// the first word that is not a (here allowed keyword)

// Therefore, the following three rules doesn't change start condition anymore.

					current->protection = PRIV;
					if(current->type.length() == 0)
					    current->type += "private ";
					}

<FindMembers>[ \t]*"public"[ \t]+	{
					current->protection = PUBL;
					if(current->type.length() == 0)
					    current->type += "public ";
					}

<FindMembers>[ \t]*"protected"[ \t]+	{
					current->protection = PROT;
					if(current->type.length() == 0)
					    current->type += "protected ";
					}

<FindMembers>[ \t]*"class"[ \t]*	{ 
// I added the following rules for FindMembers

// if we find "class", this is a nested class. We make note of it and
// then go for ClassName

					current->section = CLASS_SEC;
					current->type += "class";
					msg("found class `%s'", current->name.c_str());
					BEGIN(ClassName);
					}

<FindMembers>[ \t]*"interface"[ \t]+	{
// same for a nested interface here
					current->section = INTERFACE_SEC;
					current->type += "interface";
					msg("found interface `%s'", current->name.c_str());
					BEGIN(ClassName);
					}

<FindMembers>[ \t]*"static"[ \t]*	{
// we notice all of the allowed keywords here but don't do anything accept
// adding it to type
					current->type += "static ";
					}

<FindMembers>[ \t]*"synchronized"[ \t]*	{
					current->type += "synchronized ";
					}

<FindMembers>[ \t]*"volatile"[ \t]*	{
					current->type += "volatile ";
					}

<FindMembers>[ \t]*"transient"[ \t]*	{
					current->type += "transient ";
					}

<FindMembers>[ \t]*"native"[ \t]*	{
					current->type += "native ";
					}

<FindMembers>[ \t]*"final"[ \t]*	{
					current->type += "final ";
					}

<FindMembers>[ \t]*"///$$filename"[ \t]+	{
					// search for file name marker inserted
					// in `readfiles.ll'
					BEGIN(File);
					}

<File>.*				{
					// found a file name: now use it as the
					// current file name
					strcpy(yyFileName, yytext);
					}

<File>\n				{
					BEGIN(FindMembers);
					}

<FindMembers>[a-z_A-Z.0-9]+		{
// if we find a non-keyword word, make note of it and go to rule <Member>
					if(current->type.length())
					    current->type += ' ';
					current->type += current->name;
					current->name = yytext;
					BEGIN(Member);
					}

<Member>[a-z_A-Z.0-9]+			{
// The rule <Member> I had to make a small adjustment:
// splitting <Member>[;=] into <Member>";" and <Member>"="
// (see below for explaination)

					if(current->type.length())
					    current->type += ' ';
					current->type += current->name;
					current->name = yytext;
					}

<Member>";"				{
// this is "business as usual"
					current->section = VARIABLE_SEC;
					current_root->addSubEntry(current);
					msg("found field `%s'", current->name.c_str());
					current = new Entry;
					BEGIN(FindMembers);
					}

<Member>"="				{
// we made this an extra rule because otherwise the initialized had been
// treated as Member (which it obviously isn't). So if we have an "=", we go to
// a new rule <SkipToSemi> where we skip everything including the ending ";"
// then we go to FindMembers 

					current->section = VARIABLE_SEC;
					current_root->addSubEntry(current);
					msg("found field `%s'", current->name.c_str());
					current = new Entry;
					skip_to_semi_count = 0; // we count bracket-levels "{}"
					BEGIN(SkipToSemi);
					}

<SkipToSemi>"{"				{
// We take care for bracket-levels because there might be definition of an
// anonymous class here that makes use of Semicolons we are not interested in.
					skip_to_semi_count++;
					}

<SkipToSemi>"("				{
					skip_to_semi_count++;
					}

<SkipToSemi>"["				{
					skip_to_semi_count++;
					}

<SkipToSemi>"}"				{
					skip_to_semi_count--;
					}

<SkipToSemi>")"				{
					skip_to_semi_count--;
					}

<SkipToSemi>"]"				{
					skip_to_semi_count--;
					}

<SkipToSemi>";"				{
// So if there is a semicolon at "our" bracket level, we expect members again.
					if(skip_to_semi_count == 0)
					    BEGIN(FindMembers); 
					}

<Member>"("				{
					current->section = FUNCTION_SEC;
					current->args = yytext;
					msg("found method `%s'", current->name.c_str());
					BEGIN(Args);
					}

<Args>")"				{
					current->args += *yytext;
					BEGIN(Function);
					}

<Args>.					{
					current->args += *yytext;
					}

<Function>[ \t]*"throws"[ \t]+		{
					BEGIN(Throws);
// I changed <Function> in order to skip method(function)-body *totally*. I
// hope this is ok.
// In order to do so, after getting the "{", we skip to the corresponding "}"
// using an extra new rule <SkipToBracket>
					current->args += " throws ";
					}

<Function>";"				{
					current_root->addSubEntry(current);
					current = new Entry;
					BEGIN(FindMembers);
					}

<Function>"{"				{
					current_root->addSubEntry(current);
					current = new Entry;
					skip_to_bracket_count = 1; // we take care of bracket level
					BEGIN(SkipToBracket);
					}

<SkipToBracket>"{"			{ 
// this skips the function body.
					skip_to_bracket_count++;
					}

<SkipToBracket>"}"			{
					if(--skip_to_bracket_count == 0)
   					    BEGIN(FindMembers);
					}

<Throws>[a-z_A-Z.0-9]+[ \t]*,		{
					current->args += yytext;
					}

<Throws>[a-z_A-Z.0-9]+			{
					current->args += yytext;
					BEGIN(Function);
					}

<Comment>\n				{
					current->program += yytext;
					yyLineNr++;
					}

<Comment>.				{
					current->program += yytext;
					}

<Comment>.*"*/"				{
					current->program += yytext;
					BEGIN(lastContext);
					}

<SkipComment>[ \t]*"*/"			{
					BEGIN(lastContext);
					}

<SkipComment>"/*""*"*"*/"
<SkipComment>[ \t\n]*"/**""*"*[ \t]*

<SkipString>[\"\']			{
					if(*yytext == ssEndChar)
					    BEGIN(lastContext);
					if(ssSave)
					    current->program += *yytext;
					}

<SkipString>\\.				{
					if(ssSave)
					    current->program += yytext;
					}

<SkipString>.				{
					if(ssSave)
					    current->program += *yytext;
					}

<FindClasses,ClassName,Package,Import,Extends,Implements,FindMembers,Member,Args,Function,Throws,See,Author,Version,Param,Return,Exception,Precondition,Postcondition,Invariant>[ \t\n]*"/**""*"*[ \t]*	{
					lastContext = YY_START;
					lineCount();
					current->doc.clear();
					BEGIN(JavaDoc);
					}

<JavaDoc>\n				{
					current->doc += *yytext;
					yyLineNr++;
					}

<JavaDoc>[\n \t]*"*"+"/"		{
// Before the fix, it was context-sensitive which state to go next - depending
// on current->section. I changed this, since it doesn't make sense.
					BEGIN(lastContext);
					}

<JavaDoc>\n[ \t]*"*"*[ \t]*		{
					current->doc += '\n';
					yyLineNr++;
					}

<JavaDoc>.				{
					current->doc += *yytext;
					}

<FindClasses,ClassName,Package,Import,Extends,Implements,FindMembers,Member,Args,Function,Throws,See,Author,Version,Param,Return,Exception,Precondition,Postcondition,Invariant>"/*"	{ 
					if(YY_START != SkipComment) // Default rules are hellspawn
					    lastContext = YY_START;
					BEGIN(SkipComment);
					}

<FindClasses,ClassName,Package,Import,Extends,Implements,FindMembers,Member,Args,Function,Throws,See,Author,Version,Param,Return,Exception,Precondition,Postcondition,Invariant>[ \t]*"*/"	{
					BEGIN(lastContext);
					}

<FindClasses,ClassName,Package,Import,Extends,Implements,FindMembers,Member,Args,Function,Throws,See,Author,Version,Param,Return,Exception,Precondition,Postcondition,Invariant>"/*""*"*"*/"

<FindClasses,ClassName,Package,Import,Extends,Implements,FindMembers,Member,Args,Function,Throws,See,Author,Version,Param,Return,Exception,Precondition,Postcondition,Invariant>[\"\']	{
					ssEndChar = *yytext;
					ssSave = 0;
					lastContext = YY_START;
					BEGIN(SkipString);
					}

<FindClasses,ClassName,Package,Import,Extends,Implements,FindMembers,Member,Args,Function,Throws,See,Author,Version,Param,Return,Exception,Precondition,Postcondition,Invariant>\/\/.*\n
<*>.
<*>\n					{
					yyLineNr++;
					}
%%

void parseJavaClasses(Entry *rt)
{
    if(rt == 0)
	return;
    for(Entry *cr = rt->sub; cr; cr = cr->next)
	{
	if(cr->program.length())
	    {
	    inputString = cr->program.c_str();
	    inputPosition = 0;
	    javaYYrestart(javaYYin);

	    // We're looking for members and nested classes.
	    findClasses = false;
	    BEGIN(FindMembers);

	    current_root = cr;
	    strcpy(yyFileName, cr->file.c_str());
	    yyLineNr = cr->startLine;
	    javaYYlex();
	    cr->program.clear();
	    }
	parseJavaClasses(cr);
	}
    parseDoc(rt);
}

void parseJava(Entry *rt)
{
    assert(rt);

    current_root = rt;
    global_root = rt;
    current = new Entry;

    inputString = rt->program.c_str();
    inputPosition = 0;
    javaYYrestart(javaYYin);

    // We're looking for first-level Classes.
    BEGIN(FindClasses);
    findClasses = true;

    javaYYlex();
    rt->program.clear();

    parseJavaClasses(rt);

    delete current;
}

extern "C" {
  int javaYYwrap()
    {
    return 1;
    }
};

