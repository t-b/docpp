/*
  promote.ll

  Copyright (c) 1996 Roland Wunderling, Malte Zoeckler
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
#include "portability.h"
%}

%x	Man
%x	Memo

%%
"//@Man:"[ \t]*\n[ \t\n]*"//@Memo:"	{
			fprintf(yyout, "///");
			}

"//@Man:"[ \t]*\n[ \t\n]*"/*@Doc:"	{
			fprintf(yyout, "/**");
			}

"//@Man:"[ \t]*\n[ \t\n]*"/**"	{
			fprintf(yyout, "/**");
			}

"//@Man:"[ \t]*		{
			fprintf(yyout, "/**");
			BEGIN(Man);
			}

<Man>[^\n]+		{
			fprintf(yyout, "@name ");
			fprintf(yyout, "%s", yytext);
			}

<Man>\n[ \t\n]*"/*@Doc:"	{
			fprintf(yyout, "\n");
			BEGIN(0);
			}

<Man>\n			{
			fprintf(yyout, " */\n");
			BEGIN(0);
			}

"///"[ \t]*\n[ \t\n]*"/*@Doc:"	{
			fprintf(yyout, "/**");
			}

"///"[ \t]*\n[ \t\n]*"/**"	{
			fprintf(yyout, "/**");
			}

"//@ManMemo:"[ \t]*\n[ \t\n]*"/*@Doc:"	{
			fprintf(yyout, "/**");
			}

"//@ManMemo:"[ \t]*\n[ \t\n]*"/**"	{
			fprintf(yyout, "/**");
			}

"/*@ManMemo:"		{
			fprintf(yyout, "/**@memo ");
			}

"/*@ManDoc:"		{
			fprintf(yyout, "/**");
			}

"/*@Doc:"[ \t\n]*"*"+"/"
"//@ManMemo:"		{
			fprintf(yyout, "///");
			}

("///"|"//@ManMemo:")[ \t]*	{
			BEGIN(Memo);
			}

<Memo>[^\n]*\n		{
			fprintf(yyout, "/// %s", yytext);
			BEGIN(0);
			}

<Memo>[^\n]*\n[ \t\n]*"/*@Doc:"	{
			int i;
			fprintf(yyout, "/** ");
			for(i = 0; yytext[i] != '\n'; ++i)
			    fprintf(yyout, "%c", yytext[i]);
			fprintf(yyout, ".\n");
			BEGIN(0);
			}

"/**"[ \t]*"*/"		{
			fprintf(yyout, "///");
			}

"/**"[^.\n]*".?"[ \t]*"*/"	{
			yytext[yyleng - 2] = 0;
			fprintf(yyout, "/// %s", yytext + 4);
			}

%%

int yywrap()
{
    return 1;
}

int main()
{
    yylex();
}
