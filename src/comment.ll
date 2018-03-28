/*
  comment.ll

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

%{

#include <assert.h>
#include <ctype.h>
#include <iostream>
#include <stdarg.h>
#include <stdio.h>

#include "doc.h"

static const char	*inputString;
static int		inputPosition;

static Entry		*current = 0;

static int		yyLineNr = 0;

static void lineCount()
{
    const char *c = yytext;
    for(; *c; ++c)
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

static char nonewline(char c)
{
    return (c == '\n') ? ' ' : c;
}

%}

%x	CppDoc
%x	CppMemo
%x	See
%x	Args
%x	Type
%x	Author
%x	Name
%x	Version
%x	Deprecated
%x	Since
%x	Param
%x	Field
%x	Memo
%x	Return
%x	Exception
%x	Precondition
%x	Postcondition
%x	Invariant

%x	Filename

%%

<CppMemo>[^.]                           {
					current->memo += nonewline(*yytext);
					current->doc += yytext;
					}

<CppMemo>[.?!][ \t\n]+			{
					BEGIN(CppDoc);
					lineCount();
					current->memo += *yytext;
					current->doc += yytext;
					}

<CppMemo>\n[ *\t\n]*\n                  {
					BEGIN(CppDoc);
					lineCount();
                                        current->doc += yytext;
					}

<CppDoc>(\n|.)				{
					current->doc += *yytext;
					lineCount();
					}

<CppMemo>"/"("*"|"/")			{
					current->memo += yytext;
					}

<CppDoc>"/"("*"|"/")			{
					current->doc += yytext;
					}

<*>^[@\\]"see"[ \t\n]*			{
					lineCount();
					current->see.append(new McString);
					BEGIN(See);
					}

<*>^[@\\]"author"[ \t\n]*		{
					lineCount();
					BEGIN(Author);
					}

<*>^[@\\]"version"[ \t\n]*		{
					lineCount();
					BEGIN(Version);
					}

<*>^[@\\]"deprecated"[ \t\n]*		{
					lineCount();
					BEGIN(Deprecated);
					}

<*>^[@\\]"since"[ \t\n]*		{
					lineCount();
					BEGIN(Since);
					}

<*>^[@\\]"param"[ \t\n]*		{
					lineCount();
					current->param.append(new McString);
					BEGIN(Param);
					}

<*>^[@\\]"field"[ \t\n]*		{
					lineCount();
					current->field.append(new McString);
					BEGIN(Field);
					}

<*>^[@\\]"exception"[ \t\n]*		{
					lineCount();
					current->exception.append(new McString);
					BEGIN(Exception);
					}

<*>^[@\\]"precondition"[ \t\n]*		{
					lineCount();
					current->precondition.append(new McString);
					BEGIN(Precondition);
					}

<*>^[@\\]"postcondition"[ \t\n]*	{
					lineCount();
					current->postcondition.append(new McString);
					BEGIN(Postcondition);
					}

<*>^[@\\]"invariant"[ \t\n]*		{
					lineCount();
					current->invariant.append(new McString);
					BEGIN(Invariant);
					}

<*>^[@\\]"return"[ \t\n]*		{
					lineCount();
					current->retrn.append(new McString);
					BEGIN(Return);
					}

<*>^[@\\]"name"[ \t]*			{
					lineCount();
					current->name.clear();
					current->section = MANUAL_SEC;
					BEGIN(Name);
					}

<*>^[@\\]"memo"[ \t\n]*			{
					lineCount();
					current->memo.clear();
					BEGIN(Memo);
					}

<*>^[@\\]"type"[ \t\n]*			{
					lineCount();
					current->type.clear();
					BEGIN(Type);
					}

<*>^[@\\]"args"[ \t\n]*			{
					lineCount();
					current->args.clear();
					BEGIN(Args);
					}

<*>^[@\\]"doc"[ \t\n]+			{
					BEGIN(CppDoc);
                                        }

<*>^[@\\]"filename"[ \t\n]+		{
					BEGIN(Filename);
					}

<Name>.*				{
					current->name = yytext;
					}

<Name>\n				{
					BEGIN(CppDoc);
					yyLineNr++;
					}

<Type>.*				{
					current->type = yytext;
					}

<Type>\n				{
					BEGIN(CppDoc);
					yyLineNr++;
					}

<Args>.*				{
					current->args = yytext;
					}

<Args>\n				{
					BEGIN(CppDoc);
					yyLineNr++;
					}

<Author>.				{
					current->author += *yytext;
					}

<Version>.				{
					current->version += *yytext;
					}

<Deprecated>.				{
					current->deprecated += *yytext;
					}

<Since>.				{
					current->since += *yytext;
					}

<See>.					{
					*(current->see.last()) += *yytext;
					}

<Param>.				{
					*(current->param.last()) += *yytext;
					}

<Field>.				{
					*(current->field.last()) += *yytext;
					}

<Exception>.				{
					*(current->exception.last()) += *yytext;
					}

<Precondition>.				{
					*(current->precondition.last()) += *yytext;
					}

<Postcondition>.			{
					*(current->postcondition.last()) += *yytext;
					}

<Invariant>.				{
					*(current->invariant.last()) += *yytext;
					}

<Memo>.					{
					current->memo += nonewline(*yytext);
					}

<Return>.				{
					*(current->retrn.last()) += *yytext;
					}

<Author>\n[ \t\n]*			{
					current->author += '\n';
					}

<Version>\n[ \t\n]*			{
					current->version += '\n';
					}

<Deprecated>\n[ \t\n]*			{
					current->deprecated += '\n';
					}

<Since>\n[ \t\n]*			{
					current->since += '\n';
					}

<See>\n[ \t\n]*				{
					*(current->see.last()) += '\n';
					}

<Param>\n[ \t\n]*			{
					*(current->param.last()) += '\n';
					}

<Field>\n[ \t\n]*			{
					*(current->field.last()) += '\n';
					}

<Exception>\n[ \t\n]*			{
					*(current->exception.last()) += '\n';
					}

<Precondition>\n[ \t\n]*	{
					*(current->precondition.last()) += '\n';
					}

<Postcondition>\n[ \t\n]*   {
					*(current->postcondition.last()) += '\n';
					}

<Invariant>\n[ \t\n]*	    {
					*(current->invariant.last()) += '\n';
					}

<Memo>\n[ \t\n]*			{
					current->memo += nonewline(*yytext);
					}

<Return>\n[ \t\n]*			{
					*(current->retrn.last()) += '\n';
					}

<Filename>.*				{
					current->fileName = yytext;
					}

<Filename>\n				{
					BEGIN(CppDoc);
					yyLineNr++;
					}

<*>.
<*>\n					{
					yyLineNr++;
					}

%%

void parseDoc(Entry *rt)
{
    Entry *_current = current;

    if(rt->doc.length())
	{
	rt->program = rt->doc;
	rt->doc.clear();
	inputString = rt->program.c_str();
	inputPosition = 0;
	current = rt;
	commentYYrestart(commentYYin);
	if(rt->memo.length())
	    BEGIN(CppDoc);
	else
	    BEGIN(CppMemo);
	commentYYlex();
	rt->program.clear();

	/* Fill in the name if there is none.
           This is for stand alone documentation without an associated
           class, member, etc.
	*/
	if(rt->name.length() == 0 && rt->memo.length() != 0)
	    {
	    rt->name = rt->memo;
	    rt->memo.clear();
	    }
	current = _current;
	}
}

extern "C" {
    int commentYYwrap()
	{
	return 1;
	}
};
