/*
  doc.ll

  Copyright (c) 1996 Roland Wunderling, Malte Zoeckler
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

%{
#include <assert.h>
#include <fstream>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "doc.h"

#define YY_DECL int yylex()

#undef	YY_INPUT
#define	YY_INPUT(buf, result, max_size) result = yyread(buf, max_size);

static int inPos;
static const char *inStr;
static Entry *current = 0;
static McString templateArgs;
static int templateDepth;
static PROTECTION baseProtection = PRIV;

static int yyread(char *buf, int max_size)
{
    int c = 0;

    while(c < max_size && inStr[inPos])
	{
	*buf = inStr[inPos++];
	c++;
	buf++;
	}
    return c;
}

extern "C" {
    int docYYwrap()
	{
	return 1;
	}
}
%}

classNameId	[A-Za-z_:][A-Za-z:0-9_]*
name		[({alpha}|\_)({alpha}|{dig}|\_)*
spaces		[ \t]*

%x findBaseClasses1
%x findBaseClasses2
%x findBaseClasses3

%%

<findBaseClasses1>{spaces}":"{spaces}	{
			if(language == LANG_IDL)
			    baseProtection = PUBL;
			else
			    baseProtection = PROT;
			BEGIN(findBaseClasses2);
			}

<findBaseClasses2>[ \t,]*
<findBaseClasses2>"private"    {
			baseProtection = PRIV;
			}

<findBaseClasses2>"protected"	{
			baseProtection = PROT;
			}

<findBaseClasses2>"public"	{
			baseProtection = PUBL;
			}

<findBaseClasses2>"virtual"	{
			baseProtection = PUBL;
			}

<findBaseClasses2>{classNameId}	{
			current->addBaseClass(yytext, baseProtection);
			}

<findBaseClasses2>"<"	{
			templateArgs = "<";
			templateDepth = 1;
                        BEGIN(findBaseClasses3);
			}

<findBaseClasses3>[^<>]+	{
			templateArgs += yytext;
			}

<findBaseClasses3>"<"	{
			templateDepth++;
			templateArgs += "<";
			}

<findBaseClasses3>">"	{
			templateDepth--;
			templateArgs += ">";
                        if(templateDepth == 0)
			    BEGIN(findBaseClasses2);
			}

<*>'\n'
%%

void Entry::findBases()
{
    if(!((section & CLASS_SEC) || (section & INTERFACE_SEC)))
	{
	fprintf(stderr, "Entry::findBases() called for non class - it shouldn't happen!\n");
	return;
	}
    if(args.length() == 0)
	return;
    inPos = 0;
    inStr = args.c_str();
    current = this;
    docYYrestart(0);
    BEGIN(findBaseClasses1);
    docYYlex();
}
