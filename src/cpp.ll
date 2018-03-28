/*
  cpp.ll

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
  License along with this library; if not, write to the Free
  Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

%{

#include <assert.h>
#include <ctype.h>
#include <iostream>
#include <stdarg.h>
#include <stdio.h>

#include "doc.h"

static const char*	inputString;
static int		inputPosition;
static int		lastCContext;
static int		protection = PUBL;
static int		curlyBracketCount = 0;
static int		innerCurlyCount = 0;
static int              skipCurlyStart = 0;
static int		roundBracketCount = 0;
static int		skipReturn   = 0;
static int		sharpCount   = 0;
static Entry*		current_root = 0;
static Entry*		global_root  = 0;
static Entry*		current      = 0;
static Entry*		last         = 0; // For handling trailing comments
static int		argumentStartPos;

McDArray<namespace_entry *> namespace_table;

static int		yyLineNr = 0;
static char		yyFileName[264];

extern void addNamespace(Entry* entry);

void msg(const char *__fmt, ...)
{
    char s[1024];

    va_list argptr;
    va_start(argptr, __fmt);
    vsprintf(s, __fmt, argptr);
    va_end(argptr);
    fprintf(stderr, "%s(%d): %s\n", yyFileName, yyLineNr, s);
}

#ifdef DEBUG
void debug(const char *__fmt, ...)
{
    char s[1024];

    if(verb)
	{
	va_list argptr;
	va_start(argptr, __fmt);
	vsprintf(s, __fmt, argptr);
	va_end(argptr);
	printf("%s(%d): %s\n", yyFileName, yyLineNr, s);
	}
}
#endif

static void lineCount()
{
    const char *c;
    for(c = yytext; *c; ++c)
	yyLineNr += (*c == '\n');
}

static void addType(Entry *current)
{
    if(current->type.length() > 0 &&
	current->type[current->type.length() - 1] == '*' && current->name[0] == '*')
	return;
    if(current->type.length())
	current->type += ' ';
    current->type += current->name;
    current->name.clear();
    current->type += current->args;
    current->args.clear();
    current->startLine = yyLineNr;
}

static char nonewline(char c)
{
    return (c == '\n') ? ' ' : c;
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

static int addParamDoc()
{
    if(current->doc.length() == 0)
	return 0;

    char *start = current->args;
    start += argumentStartPos;
    argumentStartPos = current->args.length();

    char *end = start;
    while(*end != '\0' && *end != '=')
	end++;

    if(start == end)
	return 0;

    do {
	end--;
    } while(end != start && (isspace(*end) || *end == ',' || *end == ')'));

    char *identifier = end;
    while(start != identifier && (isalnum(*identifier) || *identifier == '_'))
	identifier--;

    if(identifier == end)
	return 0;

    current->doc += "\n@param ";
    while(identifier != end)
	current->doc += *++identifier;
    current->doc += ' ';
    return 1;
}

%}

%x	Cxx_Memo
%x	SubDoc
%x	SubDocComment
%x	SubDocCppComment
%x	Namespace
%x	UsingNamespace
%x	Doc
%x	VerbDoc
%x	Define
%x	DefineEnd
%x	DefineEnded
%x	StorageModifier
%x	File

%x	ClassName
%x      IdlUnion
%x      IdlSwitch
%x      IdlCase
%x	Bases

%x	NextSemi
%x	FindMembers
%x	FindMembersSuffix
%x	GrabSuffixMemo
%x	FindMembersName
%x	Function
%x	Operator
%x	Throws
%x	Union
%x	Friend

%x	Array
%x	Round
%x	Curly
%x	SkipCurly
%x	SkipInits
%x	SkipCPP
%x	SkipSemiOrCurly
%x	SkipSemiAndCurly
%x	Sharp

%x	Comment
%x	SkipComment
%x	SkipCxxComment

%x      Param_Doc
%x      Param_CxxDoc

%%

<*>\x06[^\x06]*\x06			{
					int i;
					for(i = 0; yytext[i + 1] != 6; i++)
					    yyFileName[i] = yytext[i + 1];
					yyFileName[i] = 0;
					}

<*>"'"[{}]"'"				{
					}

<*>^[ \t]*#(if|ifdef|ifndef|elif).*\n	{ // kill `#if', `#ifdef', `#ifndef', `#elif'
					}

<*>^[ \t]*#(else|endif).*\n 		{ // kill `#else', `#endif'
					}

^[ \t]*"$"[ \t]*"$"			{ // kill RCS keywords
					}

<NextSemi>"{"				{ // Array inits
					skipReturn = NextSemi;
					skipCurlyStart = curlyBracketCount;
					BEGIN(SkipCurly);
					}

<NextSemi>[;,]				{
					BEGIN(FindMembersSuffix);
					}

<FindMembers>[ \t]*"public"[ \t\n]*":"[ \t\n]* {
					current->protection = protection = PUBL;
					lineCount();
					}

<FindMembers>[ \t]*"protected"[ \t\n]*":"[ \t\n]* {
					current->protection = protection = PROT;
					lineCount();
					}

<FindMembers>[ \t]*"private"[ \t\n]*":"[ \t\n]*	{
					current->protection = protection = PRIV;
					lineCount();
					}

<FindMembers>[ \t]*"namespace"[ \t\n]+	{
					current->section = NAMESPACE_SEC;
					current->type = "namespace";
					lineCount();
					BEGIN(Namespace);
					}

<FindMembers>[ \t]*"using"[ \t\n]+"namespace"[ \t\n]+	{
					lineCount();
					BEGIN(UsingNamespace);
					}

<FindMembers>[ \t]*"typedef"[ \t\n]+"class"[ \t\n]+	{
					current->section = TYPEDEF_SEC;
					current->type = "typedef class";
					current->name.clear();
					lineCount();
					BEGIN(ClassName);
					}

<FindMembers>[ \t]*"typedef"[ \t\n]+"struct"[ \t\n]+	{
					current->section = TYPEDEF_SEC;
					current->type = "typedef struct";
					current->name.clear();
					lineCount();
					BEGIN(ClassName);
					}

<FindMembers>[ \t]*"typedef"[ \t\n]+"enum"[ \t\n]+	{
					current->section = TYPEDEF_SEC;
					current->type = "typedef enum";
					current->name.clear();
					lineCount();
					BEGIN(ClassName);
					}

<FindMembers>[ \t]*"typedef"[ \t\n]+"union"[ \t\n]+	{
					current->section = TYPEDEF_SEC;
					current->type = "typedef union";
					current->name.clear();
					lineCount();
					BEGIN(ClassName);
					}

<FindMembers>[ \t]*"typedef"[ \t\n]+	{
					current->section = TYPEDEF_SEC;
					current->type = "typedef";
					current->name.clear();
					lineCount();
					BEGIN(ClassName);
					}

<FindMembers>[ \t]*"class"[ \t\n]+	{
					current->section = CLASS_SEC;
					addType(current);
					current->type += "class";
					lineCount();
					BEGIN(ClassName);
					}

<FindMembers>[ \t]*"interface"[ \t\n]+	{
					// IDL mode
					current->section = INTERFACE_SEC;
					addType(current);
					current->type += "interface";
					lineCount();
					BEGIN(ClassName);
					}

<FindMembers>[ \t]*"struct"[ \t\n]+	{
					current->section = UNION_SEC;
					addType(current);
					current->type += "struct";
					lineCount();
					BEGIN(ClassName);
					}

<FindMembers>[ \t]*"exception"[ \t\n]+	{
                                        // IDL mode
					current->section = UNION_SEC;
					addType(current);
					current->type += "exception";
					lineCount();
					BEGIN(ClassName);
					}

<FindMembers>[ \t]*"enum"[ \t\n]+	{
					current->section = UNION_SEC;
					addType(current);
					current->type += "enum";
					lineCount();
					BEGIN(ClassName);
					}

<FindMembers>[ \t]*"union"[ \t\n]+	{
					current->section = UNION_SEC;
					addType(current);
					current->type += "union";
					lineCount();
					if(language == LANG_IDL)
					    BEGIN(IdlUnion);
                                    	else
  					    BEGIN(ClassName);
					}

<FindMembers>[\t]*"case"[ \t\n]+	{
					// IDL mode
					current->section = UNION_SEC;
					addType(current);
					current->type += "case ";
					lineCount();
					BEGIN(IdlCase);
					}

<IdlCase>[a-z_A-Z~.0-9]+		{
					current->type += yytext;
                			}

<IdlCase>":"[ \t\n]+			{
            				current->type += ": ";
                            		BEGIN(FindMembers);
                			}

<FindMembers>[ \t]*"//$$filename"[ \t]+	{
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

<FindMembers>[ \t]*"extern \"C\""[ \t\n]+	{
					current->clear();
					BEGIN(FindMembers);
					}

<FindMembers>[ \t]*"friend"[ \t\n]+	{
					BEGIN(Friend);
					}

<FindMembers>[ \t]*"inline"[ \t\n]+	{
                                        current->type += "inline ";
                                	}

<FindMembers>[ \t]*"static"[ \t\n]+     {
                                        current->type += "static ";
                                        }

<FindMembers>[ \t]*"volatile"[ \t\n]+   {
                                	current->type += "volatile ";
                                        }

<FindMembers>[ \t]*"__cdecl"[ \t\n]+    {
                                        current->type += "__cdecl ";
                                        }

<FindMembers>[ \t]*"ILVCALLBACK"[ \t\n]+	{
                                        current->type += "ILVCALLBACK ";
                                        }

<FindMembers>[ \t]*"virtual"[ \t\n]+    {
                                        current->type += "virtual ";
					}

<FindMembers>[ \t]*"LIBRARY"[ \t\n]+    {
// PK: This one I simply need in doc++.
// I have too many projects using this macro as a storage type modifier
                                        current->type += "LIBRARY ";
					}

<FindMembers>[ \t]*"WINAPI"[ \t\n]+    {
// WINAPI storage type modifier is now used instead of now obsolete __pascal,
// __fortran and __syscall calling conventions
                                        current->type += "WINAPI ";
					}

<FindMembers>[ \t]*"__declspec("[ \t\n]*    {
// Microsoft compiler specific: declarator
                                        current->type += "__declspec(";
					BEGIN(StorageModifier);
					}

<StorageModifier>[ \t]*"dllexport)"[ \t\n]*  {
// Microsoft compiler specific: DLL export symbol
                                        current->type += "dllexport) ";
					BEGIN(FindMembers);
					}

<StorageModifier>[ \t]*"dllimport)"[ \t\n]*  {
// Microsoft compiler specific: DLL import symbol
                                        current->type += "dllimport) ";
					BEGIN(FindMembers);
					}

<StorageModifier>[ \t]*"thread)"[ \t\n]*  {
// Microsoft compiler specific: thread local storage in multithreading
// environment
                                        current->type += "thread) ";
					BEGIN(FindMembers);
					}

<StorageModifier>[ \t]*"naked)"[ \t\n]*  {
// Microsoft compiler specific: naked function without prolog and epilog code
                                        current->type += "naked) ";
					BEGIN(FindMembers);
					}

<StorageModifier>[ \t]*"noreturn)"[ \t\n]*  {
// Microsoft compiler specific: prevents compiler to expect return values in
// all return branches (error handlers)
                                        current->type += "noreturn) ";
					BEGIN(FindMembers);
					}

<StorageModifier>[ \t]*"novtable)"[ \t\n]*  {
// Microsoft compiler specific: for pure class interfaces, which are never
// instantiated on their own (saves plenty of space)
                                        current->type += "novtable) ";
					BEGIN(FindMembers);
					}

<StorageModifier>[ \t]*"selectany)"[ \t\n]*  {
// Microsoft compiler specific: hint for linker to eliminate COMDAT
                                        current->type += "selectany) ";
					BEGIN(FindMembers);
					}

<StorageModifier>[ \t]*"nothrow)"[ \t\n]*  {
// Microsoft compiler specific: eliminates lifetime tracking for unwindable
// objects
                                        current->type += "nothrow) ";
					BEGIN(FindMembers);
					}

<FindMembers>"operator"/[^a-z_A-Z0-9]	{
					addType(current);
					current->name = yytext;
					BEGIN(Operator);
					}

<Operator>[ \t\n]*"()"?[^(]*		{
					current->name += yytext;
					BEGIN(FindMembersName);
					}

<FindMembers,FindMembersName>[ \t]*":"[ \t]*[0-9]+[ \t]*/";"
					{ // kill obscure bit-width stuff
					}

<FindMembers>[ \t]*::[ \t]*		{ // Must append to name but if previous
					  // text is current classname discard it.
					if(current->name == current_root->name)
					    current->name.clear();
					else
				            current->name += "::";
					BEGIN(FindMembersName);
					}

<FindMembers>[ \t]*"Public"|"Peer"|"Package"|"Private"[ \t\n]+
					{ // Ignore keywords used by makeheader
                                        }

<FindMembers>[a-z_A-Z~.0-9]+		{ // Normal name
					addType(current);
					current->name = yytext;
					if(current_root->section == UNION_SEC &&
					    current_root->type.index("enum") != -1)
					    BEGIN(Union);
					}

<FindMembers>"}"			{
					if(current_root->section == NAMESPACE_SEC)
					    current_root = current_root->parent;
					}

<FindMembersName>"operator"/[^a-z_A-Z0-9] {
 					current->name += yytext;
 					BEGIN(Operator);
 					}

<FindMembersName>[a-z_A-Z~.0-9]+	{
					current->name += yytext;
					}

<FindMembersName>.			{
					yyless(0);
					BEGIN(FindMembers);
 					}

<FindMembers>^[ \t]*"#"			{
					BEGIN(SkipCPP);
					}

<SkipCPP>.
<SkipCPP>"\\"[ \t]*"\n"			{
					yyLineNr++;
					}

<SkipCPP>\n				{
					yyLineNr++;
					BEGIN(FindMembers);
					}

<FindMembers>^[ \t]*"#"[ \t]*define[ \t]+ {
					current->type = "#define";
					BEGIN(Define);
					}

<Define>[a-z_A-Z:.0-9]+			{
					current->name = yytext;
					BEGIN(DefineEnd);
					}

<DefineEnd>[ \t]			{
					BEGIN(DefineEnd);
					}

<DefineEnd>"("[^)]*")"			{
					current->args = yytext;
					BEGIN(DefineEnded);
					}

<DefineEnd,DefineEnded>"\\"[ \t]*"\n"	{
					yyLineNr++;
					}

<DefineEnd,DefineEnded>"\n"		{
					if(!noDefines)
					    {
					    current->section = MACRO_SEC;
#ifdef DEBUG
					    debug("found macro `%s'", current->name.c_str());
#endif
					    current->file = yyFileName;
					    current_root->addSubEntry(current);
					    last = current;
					    current = new Entry;
					    current->protection = protection;
					    }
					else
					    current->clear();
					yyLineNr++;
					BEGIN(FindMembers);
					}

<FindMembers>[*&]+			{
					current->name += yytext;
					}

<FindMembers>[;=,]			{
					BEGIN(FindMembersSuffix);
#ifdef DEBUG
					debug("found `%s %s'",
					    current->type.c_str(),
					    current->name.c_str());
#endif
					if(current->section != TYPEDEF_SEC)
					    current->section = VARIABLE_SEC;
					current->file = yyFileName;
					current_root->addSubEntry(current);
				    	last = current;
					current = new Entry;
					// So `int a, b' gives `int a, int b'
					if(*yytext == ',')
					    current->type = last->type;
					current->protection = protection;
					if(*yytext == '=')
					    BEGIN(NextSemi);
					}

<Union>[;=,\n]				{
					BEGIN(FindMembersSuffix);
#ifdef DEBUG
					debug("found `%s %s'",
					    current->type.c_str(),
					    current->name.c_str());
#endif
					if(current->section != TYPEDEF_SEC)
					    current->section = VARIABLE_SEC;
					current->file = yyFileName;
					current_root->addSubEntry(current);
				    	last = current;
					current = new Entry;
					// So `int a, b' gives `int a, int b'
					if(*yytext == ',')
					    current->type = last->type;
					current->protection = protection;
					if(*yytext == '=')
					    BEGIN(NextSemi);
					}

<Friend>[^;\n]+				{
					current->clear();
					current_root->friends.append(new McString(yytext));
					}

<Friend>[;\n]				{
					lineCount();
					BEGIN(FindMembers);
					}

<FindMembers>"["			{
					current->args += yytext;
					sharpCount = 1;
					BEGIN(Array);
					}

<Array>"]"				{
					current->args += *yytext;
					if(--sharpCount <= 0)
	                            	    BEGIN(FindMembers);
					}

<Array>"["				{
					current->args += *yytext;
					sharpCount++;
					}

<Array>.				{
					current->args += *yytext;
					}

<FindMembers>"<"			{
					addType(current);
					current->type += yytext;
					sharpCount = 1;
					BEGIN(Sharp);
					}

<Sharp>">"				{
					current->type += *yytext;
					if(--sharpCount <= 0)
	                            	    BEGIN(FindMembers);
					}

<Sharp>"<"				{
					current->type += *yytext;
					sharpCount++;
					}

<Sharp>.				{
					current->type += *yytext;
					}

<Curly>[^\n{}"/]*			{
					current->program += yytext;
					}

<Curly>"//".*				{
					current->program += yytext;
					}

<Curly>\"[^\n"]*]\"			{
					current->program += yytext;
					}

<Curly>"/*"\**[ \t]*			{
					current->program += yytext;
					BEGIN(Comment);
					}

<Curly>"/*"\**[ \t]*\n			{
					current->program += yytext;
					++yyLineNr;
					BEGIN(Comment);
					}

<Curly>"{"				{
					current->program += yytext;
					++curlyBracketCount;
					++innerCurlyCount;
					}

<Curly>"}"				{
					if(curlyBracketCount > 0)
					    --curlyBracketCount;
					if(innerCurlyCount > 0)
					    {
					    // do we have any ``using namespace'' at this level?
					    // if so, remove them
					    for(int i = 0; i < namespace_table.size(); i++)
						if(namespace_table[i]->innerCurlyCount == innerCurlyCount)
						    {
						    delete namespace_table[i]->name;
						    namespace_table.remove(i);
						    }
					    current->program += yytext;
					    --innerCurlyCount;
					    }
					else
					    {
					    current_root->addSubEntry(current);
					    current->file = yyFileName;
					    last = current;
					    current = new Entry;
					    current->protection = protection;
					    BEGIN(NextSemi);
					    }
					}

<Curly>\n				{
					current->program += yytext;
					yyLineNr++;
					}

<Curly>.				{
					current->program += yytext;
					}

<FindMembers>"("			{
					current->args = yytext;
					argumentStartPos = 0;
					BEGIN(Round);
					}

<Round>"("				{
					current->args += *yytext;
					++roundBracketCount;
					}

<Round>")"				{
					current->args += *yytext;
					if(roundBracketCount)
					    --roundBracketCount;
					else
					    BEGIN(Function);
					}

<Round>[ \t\n]*"/**"                    {
                                        lineCount();
                                        if(addParamDoc())
                                            BEGIN(Param_Doc);
                                        }

<Round>[ \t\n]*"///"                    {
                                        lineCount();
                                        if(addParamDoc())
                                            BEGIN(Param_CxxDoc);
                                        }

<Round>[ \t\n]*","[ \t\n]*		{
					lineCount();
					current->args += ", ";
					}

<Round>[ \t\n]+				{
					lineCount();
					current->args += ' ';
					}

<Round>.				{
					current->args += *yytext;
					}

<Param_Doc>\n                           {
					current->doc += yytext;
					yyLineNr++;
					}

<Param_Doc>.                            {
					current->doc += yytext;
					}

<Param_Doc>"*/"                         {
					BEGIN(Round);
					}

<Param_CxxDoc>.*                        {
					current->doc += yytext;
					}

<Param_CxxDoc>\n                        {
					yyLineNr++;
					BEGIN(Round);
					}

<Function>[ \t]*"const"[ \t\n]*		{
					current->args += " const ";
					lineCount();
					}

<Function>[ \t]*"throw"[ \t\n]*"("	{
					current->args += " throw(";
					lineCount();
					BEGIN(Round);
					}

<Function>[ \t]*"raises"[ \t\n]*"("	{
					// IDL mode
					current->args += " raises(";
					lineCount();
					BEGIN(Round);
					}

<Function>"("				{
					current->type += current->name;
					current->name = current->args;
					current->args = yytext;
					BEGIN(Round);
					}

<Function>[ \t]*"="[ \t]*"0"		{
#ifdef DEBUG
					debug("found pure virtual method %s %s%s",
					    current->type.c_str(),
					    current->name.c_str(),
					    current->args.c_str());
#endif
					current->pureVirtual = true;
					}

<Function>[:;{]				{
#ifdef DEBUG
					debug("found method `%s %s%s'",
					    current->type.c_str(),
					    current->name.c_str(),
					    current->args.c_str());
#endif
					current->section = FUNCTION_SEC;
					current->file = yyFileName;
					current_root->addSubEntry(current);
					for(int i = 0; i < current->args.length(); i++)
					    if(i > 0 && current->args[i] == '*' && current->args[i - 1] == ' ')
						{
						current->args.remove(i - 1);
						current->args.insert(i, " ");
						}
					last = current;
					current = new Entry;
					current->protection = protection;
					if(*yytext == '{')
					    {
					    skipReturn = SkipSemiAndCurly;
					    skipCurlyStart = curlyBracketCount;
					    BEGIN(SkipCurly);
					    }
					else
					    if(*yytext == ':')
						BEGIN(SkipInits);
					    else
						BEGIN(FindMembersSuffix);
					}

<Function>.				{
					}

<FindMembersSuffix>\n			{
					yyless(0);
                                        BEGIN(FindMembers);
					}

<FindMembersSuffix>(.*"///"[ \t]*)|(.*"/**"[ \t]*) {
					if(QuantelExtn)
				    	    {
#ifdef DEBUG
				            debug("appended note to `%s'", last->name.c_str());
#endif
					    last->docify = true;
				            last->memo += " ";
				            BEGIN(GrabSuffixMemo);
					    }
					else
					    {
					    yyless(0);
					    BEGIN(FindMembers);
					    }
					}

<FindMembersSuffix>.*"//"[ \t]*		{
					if(commentExtn)
				 	    {
#ifdef DEBUG
				            debug("appended note to `%s'", last->name.c_str());
#endif
					    last->docify = true;
				            last->memo += " ";
				            BEGIN(GrabSuffixMemo);
					    }
					else
					    {
					    yyless(0);
 					    BEGIN(FindMembers);
					    }
					}

<FindMembersSuffix>.			{
					yyless(0);
					BEGIN(FindMembers);
					}

<GrabSuffixMemo>.			{
					last->memo += nonewline(*yytext);
					}

<GrabSuffixMemo>"*/"|(\n)	       	{
					lineCount();
                                        BEGIN(FindMembers);
  					}

<SkipSemiOrCurly>;			{
					BEGIN(FindMembers);
					}

<SkipSemiOrCurly>"{"			{
					skipReturn = FindMembers;
					skipCurlyStart = curlyBracketCount;
					BEGIN(SkipCurly);
					}

<SkipCurly>"{"				{
					++curlyBracketCount;
					}

<SkipCurly>"}"				{
					if(curlyBracketCount > skipCurlyStart)
					    --curlyBracketCount;
					else
					    BEGIN(skipReturn);
					}

<SkipInits>"{"				{
					skipReturn = SkipSemiAndCurly;
					skipCurlyStart = curlyBracketCount;
					BEGIN(SkipCurly);
					}

<SkipInits>";"				{
					BEGIN(FindMembers);
					}

<SkipSemiAndCurly>[ \t\n]
<SkipSemiAndCurly>("///")|("/**")	{
					yyless(0);
					BEGIN(FindMembers);
					}

<SkipSemiAndCurly>.			{ // i.e. a semicolon (or anything else)
					if(*yytext != ';')
					    yyless(0);
					BEGIN(FindMembers);
					}

<Bases>";"				{
					current->section = VARIABLE_SEC;
					current_root->addSubEntry(current);
					current->file = yyFileName;
					last = current;
					current = new Entry;
					current->protection = protection;
					BEGIN(FindMembersSuffix);
					}

<IdlUnion>"switch"                      {
                                        current->args += " switch";
                                        BEGIN(IdlSwitch);
                                        }

<IdlUnion>[a-z_A-Z0-9\[\]*&]+           {
					current->name += yytext;
                                        }

<IdlSwitch>[a-z_A-Z0-9\[\]*&]+          {
                                        current->args += " ";
                                        current->args += yytext;
                                        current->args += " ";
                                        }

<IdlSwitch>[()]                         {
                                        current->args += yytext;
                                        }

<ClassName>"("				{
					current->args = yytext;
					argumentStartPos = 0;
					BEGIN(Round);
					}

<ClassName>[a-z_A-Z0-9\[\]*&]+("::"[a-z_A-Z0-9\[\]*&]+)*	{
					current->type += ' ';
					current->type += current->name;
					current->name = yytext;
					}

<ClassName>"<".*">"			{
					// FIXME: dirty hack to allow partially
					// specialized templates
					current->name += yytext;
					}

<ClassName>[ \t]*":"[ \t]*		{
					current->args = ":";
					BEGIN(Bases);
					}

<Bases,ClassName,IdlSwitch>[ \t]*"{"[ \t]*	{
#ifdef DEBUG
					debug("found `%s %s'",
					    current->type.c_str(),
					    current->name.c_str());
#endif
					current->file = yyFileName;
					current->startLine = yyLineNr;
					++curlyBracketCount;
					innerCurlyCount = 0;
					BEGIN(Curly);
					}

<Bases,ClassName>[ \t]*";"		{
					current->file = yyFileName;
					current->startLine = yyLineNr;
					innerCurlyCount = 0;
					if(current->section == TYPEDEF_SEC ||
					    current->section == UNION_SEC)
					    {
#ifdef DEBUG
					    debug("found `%s %s'",
						current->type.c_str(),
						current->name.c_str());
#endif

					    if(current->section == UNION_SEC)
						// hack for C-style structs
						current->section = VARIABLE_SEC;

					    current_root->addSubEntry(current);
					    current->file = yyFileName;
					    last = current;
					    current = new Entry;
					    current->protection = protection;
					    BEGIN(FindMembersSuffix);
					    }
					else
					    {
#ifdef DEBUG
					    debug("found forward class declaration `%s'",
						current->name.c_str());
#endif
					    current->clear();
					    }
					BEGIN(FindMembers);
					}

<Bases>[a-z_A-Z*.<>0-9:]+		{
					current->extends.append(
					    new McString(yytext));
					current->args += ' ';
					current->args += yytext;
					}

<Bases>","				{
					current->args += ',';
					}

<Comment>\n				{
					current->program += yytext;
					yyLineNr++;
					}

<Comment>"//"
<Comment>.				{
					current->program += yytext;
					}

<Comment>.*"*/"				{
					current->program += yytext;
					BEGIN(Curly);
					}

<FindMembers>[ \t\n]*("///"|"//{{{")[ \t]* {
					lineCount();
					current->docify = true;
					if(current->doc.length() > 0 ||
					    current->memo.length() > 0)
					    {
#ifdef DEBUG
					    debug("found commented entry");
#endif
					    current->file = yyFileName;
					    current_root->addSubEntry(current);
					    last = current;
					    current = new Entry;
					    }
					else
					    current->clear();
					BEGIN(Cxx_Memo);
					}

<Cxx_Memo>.*				{
					current->doc += yytext;
					}

<Cxx_Memo>\n				{
					yyLineNr++;
					BEGIN(FindMembers);
					}

<FindMembers>[ \t\n]*"/*""*"+"/"
<FindMembers>[ \t\n]*"/***""*"*		{
					lastCContext = YY_START;
					lineCount();
					BEGIN(SkipComment);
					}

<FindMembers>[ \t\n]*"////"*		{
					lastCContext = YY_START;
					lineCount();
					}

<FindMembers>[ \t\n]*("/**"|"/*{{{")[ \t]* {
					lineCount();
					current->docify = true;
        				if(current->doc.length() > 0 ||
					    current->memo.length() > 0)
					    {
#ifdef DEBUG
					    debug("found commented entry");
#endif
					    current_root->addSubEntry(current);
					    current->file = yyFileName;
					    last = current;
				    	    current = new Entry;
					    }
					else
					    current->clear();
					BEGIN(Doc);
					}

<VerbDoc,Doc>\n[ \t]*"*"+"/"		{
					lineCount();
					BEGIN(FindMembers);
					}

<Namespace>[a-z_A-Z0-9]*		{
					McString tmp;
					// maybe we have to deal with a
					// hierarchy of namespaces here
					if(current_root->section == NAMESPACE_SEC)
					    tmp = current_root->fullName + "::";
					else
					    addNamespace(current_root);
					tmp += yytext;
					Entry *find = findEntry(current_root->sub, tmp.c_str(), NAMESPACE_SEC);
					if(find)
					    {
					    current_root = find;
                                    	    if(find->doc.length() == 0 &&
                                        	current->doc.length() > 0)
                                        	find->doc = current->doc;
                                            if(find->memo.length() == 0 &&
                                        	current->memo.length() > 0)
                                        	find->memo = current->memo;
                                    	    last = find;
                                    	    // do not doc this namespace twice
                                    	    delete current;
                                    	    current = new Entry;
					    }
					else
					    if(current->doc.length() > 0 || current->memo.length() > 0 || !onlyDocs)
					    // ignore the namespace if it's not
					    // doc++-commented or not in
					    // 'document all' mode
                                    		{
#ifdef DEBUG
						debug("found namespace `%s'", yytext);
#endif
						current->docify = true;
						current->name = yytext;
						current->file = yyFileName;
						current_root->addSubEntry(current);
						current_root = current;
					        last = current;
						current = new Entry;
						}
					}

<Namespace>"{"				{
					++curlyBracketCount;
					BEGIN(FindMembers);
					}

<UsingNamespace>[a-z_A-Z0-9]+("::"[a-z_A-Z0-9]+)*	{
					Entry *find = findEntry(current_root->sub, yytext, NAMESPACE_SEC);
					if(find)
					    {
					    // add the namespace to the lookup
					    // table, which will tell us where
					    // to look when a identifier isn't
					    // known
					    namespace_entry *n = new namespace_entry;
					    n->name = new char[strlen(yytext) + 1];
					    strcpy(n->name, yytext);
					    n->innerCurlyCount = innerCurlyCount;
					    namespace_table.append(n);
#ifdef DEBUG
					    debug("Namespace `%s' appended to lookup table",
						yytext);
#endif
					    }
					else
					    msg("Warning: unknown namespace `%s', ignoring `using' keyword\n",
						yytext);
					}

<UsingNamespace>;			{
					BEGIN(FindMembers);
					}

<Doc>"\\begin{verbatim}"		{
					current->doc += yytext;
					if(!HTMLsyntax)
					    BEGIN(VerbDoc);
					}

<Doc>"<"[pP][rR][eE]">"                 {
					if(HTMLsyntax)
					    {
                                            current->doc += "<PRE>";
                                            BEGIN(VerbDoc);
                                    	    }
					else
                                            current->doc += yytext;
                                        }

<VerbDoc>\n				{
					current->doc += '\n';
					yyLineNr++;
					}

<VerbDoc>"\\end{verbatim}"		{
					current->doc += yytext;
					if(!HTMLsyntax)
					    BEGIN(Doc);
					}

<VerbDoc>"</"[pP][rR][eE]">"            {
					if(HTMLsyntax)
					    {
                                            current->doc += "</PRE>";
                                            BEGIN(Doc);
                                    	    }
					else
                                             current->doc += yytext;
                                        }

<Doc>\n[ \t]*"*"*[ \t]*			{
					current->doc += '\n';
					yyLineNr++;
					}

<VerbDoc,Doc>.				{
					current->doc += *yytext;
					}

<VerbDoc,Doc>"//"			{
					current->doc += yytext;
					}

<VerbDoc,Doc>"/*"			{
					current->doc += yytext;
					}

<VerbDoc,Doc>"*/"			{
					BEGIN(FindMembers);
					}

<FindMembers>("//@{".*\n)|("/*@{"[^*]*\*+"/") {
					lineCount();
					current->file = yyFileName;
					current->startLine = yyLineNr;
					innerCurlyCount = 0;
					BEGIN(SubDoc);
					}

<SubDoc>"/*"				{
					current->program += yytext;
					BEGIN(SubDocComment);
					}

<SubDoc>"//"				{
					current->program += yytext;
					BEGIN(SubDocCppComment);
					}

<SubDoc>.				{
					current->program += *yytext;
					}

<SubDoc>\n				{
					current->program += *yytext;
					++yyLineNr;
					}

<SubDoc>("//@{".*\n)|("/*@{"[^*]*\*+"/") {
					lineCount();
					current->program += yytext;
					++curlyBracketCount;
					++innerCurlyCount;
					}

<SubDoc>("//@}".*\n)|("/*@}"[^*]*\*+"/") {
					lineCount();
					if(curlyBracketCount > 0)
					    --curlyBracketCount;
					if(innerCurlyCount > 0)
					    {
					    current->program += yytext;
					    --innerCurlyCount;
					    }
					else
					    {
#ifdef DEBUG
					    debug("found explicit subentry");
#endif
					    current->docify = true;
					    current_root->addSubEntry(current);
					    last = current;
					    current = new Entry;
					    current->protection = protection;
					    BEGIN(FindMembers);
					    }
					}

<SubDocComment,SubDocCppComment>"/*"	{
					current->program += yytext;
					}

<SubDocComment,SubDocCppComment>"//"	{
					current->program += yytext;
					}

<SubDocComment>.			{
					current->program += yytext;
					}

<SubDocComment>\n			{
					current->program += yytext;
					++yyLineNr;
					}

<SubDocComment>"*/"			{
					current->program += yytext;
					BEGIN(SubDoc);
					}

<SubDocCppComment>.			{
					current->program += yytext;
					}

<SubDocCppComment>\n			{
					current->program += yytext;
					++yyLineNr;
					BEGIN(SubDoc);
					}

<SkipComment>"//"
<SkipComment>[ \t]*"*/"			{
					BEGIN(lastCContext);
					}

<*>"//"					{
					lastCContext = YY_START;
					BEGIN(SkipCxxComment);
					}

<SkipCxxComment>.*\n			{
					yyLineNr++;
					BEGIN(lastCContext);
					}

<*>.
<*>\n					{
					yyLineNr++;
					}

<*>"/*"					{
					lastCContext = YY_START;
					BEGIN(SkipComment);
					}
%%

void callcppYYlex()
{
    cppYYlex();
    if(current->name.length() || current->program.length() ||
	current->memo.length() || current->doc.length())
	{
	current->docify = true;
	if(current->section == EMPTY_SEC)
	    current->section = VARIABLE_SEC;
	current_root->addSubEntry(current);
	last = current;
	current->file = yyFileName;
	current = new Entry;
	current->protection = protection;
	}
}

void parseCppClasses(Entry *rt)
{
    Entry *cr;

    if(rt == 0)
	return;
    for(cr = rt->sub; cr; cr = cr->next)
	{
#ifdef DEBUG
	if(verb)
	    {
	    printf("Scanning `%s %s%s'", cr->type.c_str(), cr->name.c_str(),
		cr->args.c_str());
	    if(cr->program.length())
		printf("...");
	    printf("\n");
	    }
#endif
	if(cr->program.length() > 0)
	    {
	    cr->program += '\n';
	    inputString = cr->program.c_str();
	    inputPosition = 0;
	    cppYYrestart(cppYYin);
	    BEGIN(FindMembers);
	    current_root = cr;
	    strcpy(yyFileName, cr->file.c_str());
	    yyLineNr = cr->startLine;
	    current->clear();
	    if(cr->section & CLASS_SEC)
		current->protection = protection = PRIV;
	    else
		current->protection = protection = PUBL;
	    callcppYYlex();
	    cr->program.clear();
	    }
        parseCppClasses(cr);
	}
    parseDoc(rt);
}

void buildFullName(Entry *entry)
{
    Entry *tmp;

    for(tmp = entry; tmp; tmp = tmp->next)
	{
	tmp->makeFullName();
	buildFullName(tmp->sub);
	}
}

void removeNotDocified(Entry *entry)
{
    Entry *index = entry, *tmp;

    for(; index; index = tmp)
	{
	if(index->sub)
	    removeNotDocified(index->sub);
	tmp = index->next;
	if((!index->docify || !MAKE_DOC(index)) && index != root)
	    index->parent->removeSub(index);
	}
}

void parseCpp(Entry *rt)
{
    assert(rt);

    current_root = rt;
    global_root = rt;
    protection = PUBL;
    current = new Entry;
    last = current;

    inputString = rt->program.c_str();
    inputPosition = 0;
    cppYYrestart(cppYYin);
    BEGIN(FindMembers);
    callcppYYlex();
    rt->program.clear();
    parseCppClasses(rt);
    removeNotDocified(rt);
    buildFullName(rt);

    delete current;
}

extern "C" {
    int cppYYwrap()
	{
	return 1;
	}
};
