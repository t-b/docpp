/*
  doc2xml.ll

  Copyright (c) 2001 Dragos Acostachioaie

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
#include "doc.h"

#define YY_DECL int yylex()

#undef	YY_INPUT
#define	YY_INPUT(buf, result, max_size) result = yyread(buf, max_size);

static int		inPos;
static const char*	inStr;

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
    int doc2xmlYYwrap()
	{
	return 1;
	}
}

%}

%%
"<"			{
			fprintf(out, "&lt;");
			}

">"			{
			fprintf(out, "&gt;");
			}

"&"			{
			fprintf(out, "&amp;");
			}

"\\"			{
			fprintf(out, "&apos;");
			}

"\""			{
			fprintf(out, "&quot;");
			}
%%

void usermanXML(char *str, Entry *root)
{
    fprintf(out, "<?xml version='1.0' encoding='ISO-8859-1' standalone='no'?>\n");
    fprintf(out, "<!DOCTYPE docpp SYSTEM \"doc++.dtd\">\n");

    fprintf(out, "<docpp>\n");
    fprintf(out, "</docpp>\n");
}
