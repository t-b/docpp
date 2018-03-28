/*
  docify.ll

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
#include <stdio.h>

#include "McString.h"

#include "portability.h"

McString	yyText;
FILE*		out;
bool		docified = false;
int		braCnt = 0;
bool		fastNotSmall = false;

extern "C" {
    int yywrap()
	{
	return 1;
	}
};

void yyMore()
{
    yyText += yytext;
}

void yyDump()
{
    fprintf(out, "%s", yyText.c_str());
    yyText.clear();
}

void docify()
{
    if(!docified)
	{
	if(yyText.size() && yyText[0] == '/')
	    {
	    if(yyText[1] == '/')
		fprintf(out, "///");
	    else
		fprintf(out, "/**");
	    yyText.remove(0, 2);
	    yyDump();
	    }
	else
	    {
	    yyDump();
	    for(int i = 0; yytext[i] == ' ' || yytext[i] == '\n'; i++)
		fprintf(out, "%c", yytext[i]);
	    fprintf(out, "///\n");
	    }
	}
    yyMore();
    docified = false;
}

#undef	ECHO
#define	ECHO

%}

%s	Start
%x	Comment
%x	CPP
%x	Code
%x	Bracket

%%
<Start>[ \t\n]		{
			ECHO;
			yyMore();
			}

<Start>"//*"[^*].*\n	{
			ECHO;
			docified = true;
			yyMore();
			}

<Start>"//*\n"		{
			ECHO;
			docified = true;
			yyMore();
			}

<Start>"///"[^/].*\n	{
			ECHO;
			docified = true;
			yyMore();
			}

<Start>"///\n"		{
			ECHO;
			docified = true;
			yyMore();
			}

<Start>"//@".*\n	{
			ECHO;
			docified = true;
			yyMore();
			}

<Start>"/**"[^*/]	{
			ECHO;
			docified = true;
			yyMore();
			BEGIN(Comment);
			}

<Start>"/*/"[^*/]	{
			ECHO;
			docified = true;
			yyMore();
			BEGIN(Comment);
			}

<Start>"/*@"		{
			ECHO;
			docified = true;
			yyMore();
			BEGIN(Comment);
			}

<Comment>.		{
			ECHO;
			yyMore();
			}

<Comment>\n		{
			ECHO;
			yyMore();
			}

<Comment>"*/"		{
			ECHO;
			yyMore();
			BEGIN(Start);
			}

<Start>"//".*\n		{
			ECHO;
			yyDump();
			yyMore();
			}

<Start>"/*"		{
			ECHO;
			yyDump();
			yyMore();
			BEGIN(Comment);
			}

<Start>"#"		{
			ECHO;
			yyMore();
			BEGIN(CPP);
			}

<CPP>.			{
			ECHO;
			yyMore();
			}

<CPP>"\\\n"		{
			ECHO;
			yyMore();
			}

<CPP>[ \t]*		{
			ECHO;
			yyMore();
			}

<CPP>\n			{
			ECHO;
                        yyMore();
			yyDump();
			docified = false;
			BEGIN(Start);
			}

<Start>[ \t]*template[ \t\n]?[^{]*"{"	{
			ECHO;
			docify();
			}

<Start>[ \t]*"class".*	{
			ECHO;
			docify();
			}

<Start>[ \t]*"struct"[ \t\n][^{]*"{"	{
			ECHO;
			docify();
			}

<Start>[ \t]*"typedef".*";"	{
			ECHO;
			docify();
			}

<Start>[ \t]*"public:"	{
			ECHO;
			yyMore();
			}

<Start>[ \t]*"protected:"	{
			ECHO;
			yyMore();
			}

<Start>[ \t]*"private:"	{
			ECHO;
			yyMore();
			}

<Start>[ \t]*[a-zA-Z_~]	{
			ECHO;
			docify();
			BEGIN(Code);
			}

<Code>"{"		{
			ECHO;
			yyMore();
			BEGIN(Bracket);
			}

<Code>";"		{
			ECHO;
			yyMore();
			yyDump();
			BEGIN(Start);
			}

<Code>.			{
			ECHO;
			yyMore();
			}

<Code>\n		{
			ECHO;
			yyMore();
			}

<Bracket>"{"		{
			ECHO;
			yyMore();
			braCnt++;
			}

<Bracket>"}"		{
			ECHO;
			yyMore();
			if(braCnt)
			    braCnt--;
			else
			    {
			    yyDump();
			    BEGIN(Start);
			    }
			}

<Bracket>\n		{
			ECHO;
			yyMore();
			}

<Bracket>.		{
			ECHO;
			yyMore();
			}

<Start>.		{
			ECHO;
			yyMore();
			}
%%

int main(int argc, char **argv)
{
    FILE *ain;
    FILE *aout;
    yyin = stdin;
    out  = stdout;

    if(argc > 1)
	{
	if(argv[1][0] == '-')
	    {
	    fprintf(stderr, "Usage: docify [infile [outfile]]\n");
	    return 0;
	    }
	ain = fopen(argv[1], "r");
	if(ain)
	    yyin = ain;
	else
	    fprintf(stderr, "Could not open file `%s' for reading; using stdin as default\n",
		argv[1]);
	}

    if(argc > 2)
	{
	aout = fopen(argv[2], "w");
	if(aout)
	    {
	    yyout = aout;
	    out = aout;
	    }
	else
	    fprintf(stderr, "Could not open file `%s' for writing; using stdout as default\n",
		argv[2]);
	}

    BEGIN(Start);
    yylex();
    yyDump();
    return 0;
}
