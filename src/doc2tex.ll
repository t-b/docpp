/*
  doc2tex.ll

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
#include <fstream.h>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "classgraph.h"
#include "doc.h"

#define YY_DECL int yylex()

#undef	YY_INPUT
#define	YY_INPUT(buf, result, max_size) result = yyread(buf, max_size);

static Entry*	current;
static int	inPos;
static int	lastContext;
static const char*	inStr;
static McString	img, imgArg1, imgArg2;

static enum {
    C_COMMENT,
    SHORT_C_COMMENT,
    CC_COMMENT
} commentMode;

static int	escapePercent	= 0;
static int	verbMode	= 0;
static int	skip		= 0;
static int	tab		= 0;
static int	yyLineNr	= 0;
static int	texBracketCount	= 0;
static McString	refStr		= "";
static Entry*	ref		= 0;
static Entry*	noref		= ref + 1;
static char	yyFileName[264];
bool		mathmode	= false;
static int	tabFlag		= 0;

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
    int doc2texYYwrap()
	{
	return 1;
	}
}

static void setupVerbMode()
{
    if(skip)
	{
	fprintf(out, "\\strut\\goodbreak\n");
	fprintf(out, "\\noindent{\\tiny\\em %s}", yyFileName);
	fprintf(out, "\\strut\\nopagebreak\\\\\n");
	skip = 0;
	}
    fprintf(out, "\\cxxCodeLine{%d} ", yyLineNr);
    verbMode = 1;
    tab = 0;
}

static void closeVerbMode()
{
    fprintf(out, "%c\n", 3);
    verbMode = 0;
}

%}

%x	Code

%x	Verb
%x	Ref
%x	texmode
%x	URL
%x	Link
%x	Image
%x	ImageArg1
%x	ImageArg2
%x	Label
%x	Listing
%x	Comment
%x	LVerb
%x	ShortComment
%x	RealComment
%x	Graph
%x	texverbatim
%x	TeXlist
%x	HTMLlist
%x	HTML
%x	HTMLverbatim

%%
"#\\#"			{
			fprintf(out, "$\\backslash$");
			}

"\\#"			{
			fprintf(out, "%s", yytext);
			}

"#"			{
			fprintf(out, "{\\tt\\strut ");
			BEGIN(Verb);
			}

"#define"		{
			fprintf(out, "\\#define");
			BEGIN(Verb);
			}

"\\"[T|t][E|e][X|x][ \t]*"{"	{
			BEGIN(texmode);
			texBracketCount = 1;
			}

<texmode>"\\}"		{
			yymore();
			}

<texmode>"\\{"		{
			yymore();
			}

<texmode>"{"		{
			++texBracketCount;
			yymore();
			}

<texmode>"}"		{
			if(--texBracketCount == 0)
			    {
			    yytext[--yyleng] = 0;
			    fprintf(out, yytext);
			    BEGIN(0);
			    }
			else
			    yymore();
			}

<texmode>.|\n		{
			yymore();
			}

"\\Ref"[ \t]*"{"	{
			BEGIN(Ref);
			}

<Ref>[^}]*		{
			if(ref == 0)
			    {
			    refStr = McString(yytext, 0, yyleng - 2);
			    ref = getRefEntry(refStr, current);
			    }
			REJECT;
			}

<Ref>"}"		{
			if(ref && ref != noref)
			    {
			    fprintf(out, " ($\\rightarrow$ ");
			    if(MAKE_DOC(ref))
				{
				ref->dumpNumber(out);
				fprintf(out, ",");
				}
			    fprintf(out, " {\\em page }\\pageref{cxx.");
			    ref->dumpNumber(out);
			    fprintf(out, "})");
			    }
			else
			    fprintf(out, refStr.c_str());
			ref = 0;
			refStr.clear();
			BEGIN(0);
			}

"\\URL["		{
			BEGIN(URL);
			}

<URL>[^\]]*		{
			char *p = yytext;
			while(*p != '\0')
			    {
			    switch(*p)
				{
				case '%':
				case '_':
				case '&':
				case '$':
				    fputc('\\', out);
				default:
				    fputc(*p, out);
				    break;
				}
			    ++p;
			    }
			}

<URL>"]{"[^}]*"}"	{
			BEGIN(0);
			}

"{@link"[ \t]*		{
			BEGIN(Link);
			}

<Link>[^ ]*		{
			if(ref == 0)
			    {
			    McString tmp = yytext;
			    ref = getRefEntry(tmp, current);
			    }
			REJECT;
			}

<Link>"}"		{
			if(ref && ref != noref)
			    {
			    fprintf(out, " ($\\rightarrow$ ");
			    if(MAKE_DOC(ref))
				{
				ref->dumpNumber(out);
				fprintf(out, ",");
				}
			    fprintf(out, " {\\em page }\\pageref{cxx.");
			    ref->dumpNumber(out);
			    fprintf(out, "})");
			    }
			ref = 0;
			BEGIN(0);
			}

"\\Label{"[^}]*"}"	{
			}

"\\"([I|i][M|m][G|g]|"includegraphics")[ \t]*"{"	{
			img.clear();
			imgArg1.clear();
			imgArg2.clear();
			BEGIN(Image);
			}

"\\"([I|i][M|m][G|g]|"includegraphics")[ \t]*"["	{
			img.clear();
			imgArg1.clear();
			imgArg2.clear();
			BEGIN(ImageArg1);
			}

<ImageArg1>[^\]]*"]{"	{
    			if(strlen(yytext) > 2 && imgArg1.length() == 0)
			    imgArg1 = McString(yytext, 0, strlen(yytext) - 2);
			BEGIN(Image);
			}

<ImageArg1>[^\]]*"]["	{
    			if(strlen(yytext) > 2 && imgArg1.length() == 0)
			    imgArg1 = McString(yytext, 0, strlen(yytext) - 2);
			BEGIN(ImageArg2);
			}

<ImageArg2>[^\]]*"]{"	{
			if(strlen(yytext) > 2 && imgArg2.length() == 0)
	    		    imgArg2 = McString(yytext, 0, strlen(yytext) - 2);
			BEGIN(Image);
			}

<Image>[^}]*"}"		{
			if(strlen(yytext) > 1 && img.length() == 0)
			    img = McString(yytext, 0, strlen(yytext) - 1);
			if(img.length() > 0)
			    {
        		    fprintf(out, "\\includegraphics");
        		    if(imgArg2.length() > 0)
            			{
            			fprintf(out, "[");
				fprintf(out, imgArg2.c_str());
				fprintf(out, "]");
            			}
        		    // remove extension if present
			    int i = img.index('.');
			    if(i > 0)
				img.remove(i, img.length() - i);
        		    fprintf(out, "{%s.eps}", img.c_str());
			    }
			img.clear();
			imgArg1.clear();
			imgArg2.clear();
                	BEGIN(0);
                	}

"\\"([D|d][A|a][T|t][E|e]|"today")	{
			time_t ltime;
			(void)time(&ltime);
			char timebuf[64];
			strftime(timebuf, 64, "%c", localtime(&ltime));
			fprintf(out, timebuf);
			}

<Code>[#]		{
			fprintf(out, "\\%c", *yytext);
			}

<Verb>"#"		{
			fprintf(out, "}");
			BEGIN(0);
			}

<LVerb>"#"		{
			fprintf(out, "}");
			BEGIN(Comment);
			}

<TeXlist>[ \t\n]*	{
			fprintf(out, "} & ");
			BEGIN(0);
			}

<HTMLlist>[ \t\n]*	{
			fprintf(out, "} & ");
			BEGIN(HTML);
			}

<HTMLlist,TeXlist,Ref,LVerb,Verb,Code>[%_&><{}$ ] {
			fprintf(out, "\\%c", *yytext);
			}

<HTMLlist,TeXlist,Ref,LVerb,Verb,Code>"~"	{
			fprintf(out, "\\cxxtilde ");
			}

<HTMLlist,TeXlist,Ref,LVerb,Verb,Code>\\	{
			fprintf(out, "{$\\backslash$\\relax}");
			}

<HTMLlist,TeXlist,Ref,LVerb,Verb,Code>\^	{
			fprintf(out, "$\\hat{\\;}$");
			}

<HTMLlist,TeXlist,Ref,LVerb,Verb,Code>(.|\n)	{
			fprintf(out, "%c", *yytext);
			}

"<"			{
			fprintf(out, (mathmode ? "<" : "\\<"));
			}

">"			{
			fprintf(out, (mathmode ? ">" : "\\>"));
			}

"_"			{
			fprintf(out, (mathmode ? "_" : "\\_"));
			}

"$"			{
			fprintf(out, "$");
			mathmode = !mathmode;
			}

"%"			{
			fprintf(out, "\\%c", *yytext);
			}

"~"			{
			fprintf(out, "\\cxxtilde ");
			}

"&"			{
			fprintf(out, tabFlag ? "&" : "\\&");
			}

"\\["			{
			fprintf(out, "\\[");
			mathmode = true;
			}

"\\]"			{
			fprintf(out, "\\]");
			mathmode = false;
			}

"\\"[%_&><{}$ ]		{
			fprintf(out, "%s", yytext);
			}

"\\begin{"("tabular"|"array")"}{"[lrc|]+"}"	{
			fprintf(out, "%s", yytext);
			tabFlag++;
			}

"\\end{"("tabular"|"array")"}"	{
			fprintf(out, "%s", yytext);
			tabFlag--;
			}

"\\begin{"("eqnarray"|"eqnarray\*"|"equation"|"displaymath")"}"	{
			fprintf(out, "%s", yytext);
			mathmode = true;
			}

"\\end{"("eqnarray"|"eqnarray\*"|"equation"|"displaymath")"}"	{
			fprintf(out, "%s", yytext);
			mathmode = false;
			}

"\\begin{verbatim}"	{
			fprintf(out, "%s", yytext);
			BEGIN(texverbatim);
			}

<texverbatim>.		{
			fprintf(out, "%s", yytext);
			}

<texverbatim>\n		{
			fprintf(out, "%s", yytext);
			}

<texverbatim>"\\end{verbatim}"	{
			fprintf(out, "%s", yytext);
			BEGIN(0);
			}

.			{
			fprintf(out, "%s", yytext);
			}

\n			{
			fprintf(out, "%s", yytext);
			}

<HTML>[_$#%{}&]		{
			fprintf(out, "\\%s", yytext);
			}

<HTML>[><]		{
			fprintf(out, "$%s$", yytext);
			}

<HTML>"<"[ \t]*[eE][mM][ \t]*">"	{
			fprintf(out, "{\\em ");
			}

<HTML>"<"[ \t]*([bB]|[sS][tT][rR][oO][nN][gG])[ \t]*">"	{
			fprintf(out, "{\\bf ");
			}

<HTML>"<"[ \t]*[iI][ \t]*">"	{
			fprintf(out, "{\\it ");
			}

<HTML>"</"[ \t]*([eE][mM]|[bB]|[sS][tT][rR][oO][nN][gG]|[iI])[ \t]*">"	{
			fprintf(out, "}");
			}

<HTML>"<"[ \t]*[tT][tT][ \t]*">"	{
			fprintf(out, "{\\tt\\strut ");
			}

<HTML>"<"[ \t]*"/"[tT][tT][ \t]*">"	{
			fprintf(out, "}");
			}

<HTML>"<"[ \t]*([pP][rR][eE]|[cC][oO][dD][eE])[ \t]*">"	{
			fprintf(out, "\\begin{verbatim}\n");
			lastContext = YY_START;
			BEGIN(HTMLverbatim);
			}

<HTML>"<"[ \t]*[oO][lL][ \t]*">"	{
			fprintf(out, "\\begin{enumerate}\n");
			}

<HTML>"</"[ \t]*[oO][lL][ \t]*">"	{
			fprintf(out, "\\end{enumerate}\n");
			}

<HTML>"<"[ \t]*[dD][lL][ \t]*">"	{
			fprintf(out, "\\begin{description}\n");
			}

<HTML>"</"[ \t]*[dD][lL][ \t]*">"	{
			fprintf(out, "\\end{description}\n");
			}

<HTML>"<"[ \t]*[dD][tT][ \t]*">"	{
			fprintf(out, "\\item[");
			}

<HTML>"<"[ \t]*[dD][dD][ \t]*">"	{
			fprintf(out, "]\t");
			}

<HTML>"<"[ \t]*[uU][lL][ \t]*">"	{
			fprintf(out, "\\begin{itemize}\n");
			}

<HTML>"</"[ \t]*[uU][lL][ \t]*">"	{
			fprintf(out, "\\end{itemize}\n");
			}

<HTML>"<"[ \t]*[lL][iI][ \t]*">"	{
			fprintf(out, "\\item\t");
			}

<HTML>"<"[ \t]*[pP][ \t]*">"	{
			fprintf(out, "\\strut\\\\");
			}

<HTML>"&lt;"		{
			fprintf(out, "\\<");
			}

<HTML>"&gt;"		{
			fprintf(out, "\\>");
			}

<HTML>"&amp;"		{
			fprintf(out, "&");
			}

<HTML>"&nbsp;"		{
			fprintf(out, "~");
			}

<HTML>"&forall;"	{
			fprintf(out, "$\\forall$");
			}

<HTML>"&exist;" 	{
			fprintf(out, "$\\exists$");
			}

<HTML>"&ne;"	 	{
			fprintf(out, "$\\neq$");
			}

<HTML>"&le;"	 	{
			fprintf(out, "$\\le$");
			}

<HTML>"&ge;"	 	{
			fprintf(out, "$\\ge$");
			}

<HTML>"&isin;"	 	{
			fprintf(out, "$\\in$");
			}

<HTML>"&notin;" 	{
			fprintf(out, "$\\not\\in$");
			}

<HTML>"&times;" 	{
			fprintf(out, "$\\times$");
			}

<HTML>"&empty;" 	{
			fprintf(out, "$\\emptyset$");
			}

<HTML>"&sub;"	 	{
			fprintf(out, "$\\subset$");
			}

<HTML>"&sup;"	 	{
			fprintf(out, "$\\superset$");
			}

<HTML>"&sube;"	 	{
			fprintf(out, "$\\subseteq$");
			}

<HTML>"&supe;"	 	{
			fprintf(out, "$\\superseteq$");
			}

<HTML>"<"[ \t]*[bB][rR][ \t]*">"	{
			fprintf(out, "\\\\");
			}

<HTML>\\		{
			fprintf(out, "$\\backslash$");
			}

<HTML>.			{
			fprintf(out, "%s", yytext);
			}

<HTML>\n		{
			fprintf(out, "%s", yytext);
			} 

<HTMLverbatim>(.|\n)	{
			fprintf(out, "%s", yytext);
			}

<HTMLverbatim>"</"[ \t]*([pP][rR][eE]|[cC][oO][dD][eE])[ \t]*">"	{
			fprintf(out, "\\end{verbatim}\n");
			BEGIN(lastContext);
			}

<*>\x06[^\x06]*\x06	{
			yyLineNr = 1;
			int i;
		   	for(i = 0; yytext[i + 1] != 6; i++)
		   	    yyFileName[i] = yytext[i + 1];
		   	yyFileName[i] = 0;
			fprintf(out, "\\strut\\\\\n");
			skip = 1;
		   	}

<Listing>^[ \t]*\n	{
			if(!skip)
			    fprintf(out, "\\cxxCodeLine{%d} %c\n\\\\\n", yyLineNr, 3);
			yyLineNr++;
			}

<Listing>\t		{
			if(!verbMode)
			    setupVerbMode();
			do
			    {
			    putc(' ', out);
			    }
			while(++tab % 8);
			}

<Listing>\"[^\n"]*\"	{
			if(!verbMode)
			    setupVerbMode();
			fprintf(out, "%s", &(yytext[0]));
			tab += yyleng;
			}

<Listing>.		{
			if(!verbMode)
			    setupVerbMode();
			putc(yytext[0], out);
			++tab;
			}

<Listing>\n		{
			if(!skip)
			    {
			    if(!verbMode)
				setupVerbMode();
			    closeVerbMode();
			    fprintf(out, "\\\\\n");
			    }
			yyLineNr++;
			}

<Listing>"//@"[^\n]*\n	{
			yyLineNr++;
			}

<Listing>"/*@"[^\n]*"*/"
<Listing>[ \t\n]*"/*@"	{
			BEGIN(RealComment);
			}

<RealComment>.
<RealComment>\n		{
			yyLineNr++;
			}

<RealComment>"*/"	{
			BEGIN(Listing);
			}

<Listing>"//"		{ 
			if(!verbMode)
			    setupVerbMode();
			closeVerbMode();
			fprintf(out, "\\hbox{//");
			commentMode = CC_COMMENT;
			BEGIN(Comment);
			}

<Listing>"/*"		{ 
			if(verbMode)
			    closeVerbMode();
			fprintf(out, "\\hbox{/*");
			commentMode = SHORT_C_COMMENT;
			BEGIN(Comment);
			}

<Listing>[ \t]*\n[\n\t ]*"/*"	{
			if(verbMode)
			    closeVerbMode();
			commentMode = C_COMMENT;
			while(yyleng--)
			    yyLineNr += (yytext[yyleng] == '\n');
			BEGIN(Comment);
			fprintf(out, "\\strut\\\\");
			fprintf(out, "\\strut\\\\");
			}

<Comment>"*/"		{
			switch(commentMode)
			    {
			    case SHORT_C_COMMENT:
				fprintf(out, "%s", &(yytext[0]));
				fprintf(out, "}\\\\\n");
				BEGIN(Listing);
				break;
			    case C_COMMENT:
				skip = 1;
				BEGIN(Listing);
				break;
			    default:
				fprintf(out, "%s", &(yytext[0]));
				break;
			    }
			}

<Comment>\n		{
			switch(commentMode)
			    {
			    case SHORT_C_COMMENT:
				commentMode = C_COMMENT;
				fprintf(out, "}\n");
			    case C_COMMENT:
				putc(yytext[0], out);
				break;
			    default:
				fprintf(out, "}\\strut\\\\\n");
				BEGIN(Listing);
				break;
			    }
			yyLineNr++;
			}

<Comment>"#\\#"		{
			fprintf(out, "$\\backslash$");
			}

<Comment>"\\#"		{
			fprintf(out, "%s", yytext);
			}

<Comment>"#"		{
			fprintf(out, "{\\tt\\strut ");
			BEGIN(LVerb);
			}

<Comment>.		{
			putc(yytext[0], out);
			}

<Graph>"SP"		{
			fprintf(out, "\\cxxNone");
			}

<Graph>"||"		{
			fprintf(out, "\\cxxLong");
			}

<Graph>"L."		{
			fprintf(out, upArrows ? "\\cxxLinkPriLeft" \
			    : "\\cxxPriLeft");
			}
<Graph>"L-"		{
			fprintf(out, upArrows ? "\\cxxLinkProLeft" \
			    : "\\cxxProLeft");
			}

<Graph>"L_"		{
			fprintf(out, upArrows ? "\\cxxLinkPubLeft" \
			    : "\\cxxPubLeft");
			}

<Graph>"l."		{
			fprintf(out, upArrows ? "\\cxxLinkPrileft" \
			    : "\\cxxPrileft");
			}

<Graph>"l-"		{
			fprintf(out, upArrows ? "\\cxxLinkProleft" \
			    : "\\cxxProleft");
			}

<Graph>"l_"		{
			fprintf(out, upArrows ? "\\cxxLinkPubleft" \
			    : "\\cxxPubleft");
			}

<Graph>"D."		{
			fprintf(out, upArrows ? "\\cxxLinkPriLeft" \
			    : "\\cxxLastPriLeft");
			}

<Graph>"D-"		{
			fprintf(out, upArrows ? "\\cxxLinkProLeft" \
			    : "\\cxxLastProLeft");
			}

<Graph>"D_"		{
			fprintf(out, upArrows ? "\\cxxLinkPubLeft" \
			    : "\\cxxLastPubLeft");
			}

<Graph>"d."		{
			fprintf(out, upArrows ? "\\cxxLinkPrileft" \
			    : "\\cxxLastPrileft");
			}

<Graph>"d-"		{
			fprintf(out, upArrows ? "\\cxxLinkProleft" \
			    : "\\cxxLastProleft");
			}

<Graph>"d_"		{
			fprintf(out, upArrows ? "\\cxxLinkPubleft" \
			    : "\\cxxLastPubleft");
			}

<Graph>"R."		{
			fprintf(out, upArrows ? "\\cxxPriRight" \
			    : "\\cxxLinkPriRight");
			}

<Graph>"R-"		{
			fprintf(out, upArrows ? "\\cxxProRight" \
			    : "\\cxxLinkProRight");
			}

<Graph>"R_"		{
			fprintf(out, upArrows ? "\\cxxPubRight" \
			    : "\\cxxLinkPubRight");
			}

<Graph>"r."		{
			fprintf(out, upArrows ? "\\cxxPriright" \
			    : "\\cxxLinkPriright");
			}

<Graph>"r-"		{
			fprintf(out, upArrows ? "\\cxxProright" \
			    : "\\cxxLinkProright");
			}

<Graph>"r_"		{
			fprintf(out, upArrows ? "\\cxxPubright" \
			    : "\\cxxLinkPubright");
			}

<Graph>"^."		{
			fprintf(out, upArrows ? "\\cxxFirstPriright" \
			    : "\\cxxLinkPriright");
			}

<Graph>"^-"		{
			fprintf(out, upArrows ? "\\cxxFirstProright" \
			    : "\\cxxLinkProright");
			}

<Graph>"^_"		{
			fprintf(out, upArrows ? "\\cxxFirstPubright" \
			    : "\\cxxLinkPubright");
			}

<Graph>"|."		{
			fprintf(out, upArrows ? "\\cxxFirstPriRight" \
			    : "\\cxxLinkPriRight");
			}

<Graph>"|-"		{
			fprintf(out, upArrows ? "\\cxxFirstProRight" \
			    : "\\cxxLinkProRight");
			}

<Graph>"|_"		{
			fprintf(out, upArrows ? "\\cxxFirstPubRight" \
			    : "\\cxxLinkPubRight");
			}
%%

static void initLatex()
{
    char ch;
    int	 i;

    if(texFile.length() > 0)
	{
	ifstream env(texFile.c_str());
	if(env)
	    {
	    while(env)
		{
		env.get(ch);		// "env >> ch;" never returns whitespaces and special chars!!!
		putc(ch, out);
		}
	    if(!noLatex)
		{
		if(generateIndex)
		    fprintf(out, "\\makeindex\n");

		for(i = 0; i < texPackages.size(); i++)
		    fprintf(out, "\\usepackage{%s}\n", texPackages[i]->c_str());

		fprintf(out, "\\usepackage{docxx}\n");
		fprintf(out, "\\begin{document}\n");
		fprintf(out, "\\pagebreak\n");
		fprintf(out, "\\sloppy\n");
		}
	    }
	else
	    cerr << "could not open " << texFile.c_str() << endl;
	}
    else
	if(!noLatex)
	    {
	    fprintf(out, "\\documentclass");
	    if(texOption.length() > 0)
		fprintf(out, "[%s]", texOption.c_str());
	    fprintf(out, "{article}\n");

	    if(generateIndex)
		fprintf(out, "\\makeindex\n");

	    for(i = 0; i < texPackages.size(); i++)
		fprintf(out, "\\usepackage{%s}\n", texPackages[i]->c_str());

	    fprintf(out, "\\usepackage{docxx}\n");
	    fprintf(out, "\\begin{document}\n");
	    fprintf(out, "\\pagebreak\n");
	    fprintf(out, "\\sloppy\n");
	    }

    if(texTitle.length() > 0 && !noLatex)
	{
	ifstream title(texTitle.c_str());

	if(title)
	    while(title)
		{
		title.get(ch);
		putc(ch, out);
		}
	else
	    cerr << "could not open " << texTitle.c_str() << endl;
	}
}

static void finitLatex()
{
    char indName[50];
    char *p;
    FILE *file;

    if(!noLatex)
	{
 	if(!onlyClassGraph && generateIndex && texOutputName.length() > 0)
 	    {
 	    strcpy((char *)&indName, texOutputName.c_str());
 
 	    p = strrchr((char *)&indName, '.');

 	    if(p) 
 		*p = '\0';

 	    strcat((char *)&indName, ".ind");

 	    fprintf(out, "\\label{Index}\n");
 	    fprintf(out, "\\input %s\n", (char *)&indName);

 	    file = fopen((char *)&indName, "r");

 	    if(!file)
 		file = fopen((char *)&indName, "w");

 	    if(file)
 		fclose(file);
 	    }

	fprintf(out, "\\end{document}\n");
	}
}

void listing(char *str)
{
    inPos = 0;
    inStr = str;
    skip = 1;

    initLatex();
    doc2texYYrestart(0);
    BEGIN(Listing);
    doc2texYYlex();
    finitLatex();
}

static void printCode(Entry *cur, const char *str)
{
    inPos = 0;
    inStr = str;
    current = cur;
    if(cur && cur->section == MANUAL_SEC)
	{
	printYYDOC(cur, str);
	return;
	}
    doc2texYYrestart(0);
    BEGIN(Code);
    doc2texYYlex();
}

static void printCode(Entry *cur, const McString& str)
{
    printCode(cur, str.c_str());
}

void printYYDOC(Entry *cur, const char *str, int escapePrcnt)
{
    if(strlen(str) > 0)
	{
	inPos = 0;
	inStr = str;
	current = cur;
	escapePercent = escapePrcnt;

	doc2texYYrestart(0);
	BEGIN(HTMLsyntax ? HTML : 0);
	doc2texYYlex();
	}

    escapePercent = 0;
}

void printYYDOC(Entry *cur, const McString& str, int escapePrcnt)
{
    printYYDOC(cur, str.c_str(), escapePrcnt);
}

char *getEntryName(Entry *entry)
{
    McString *name;

    if(entry->fromUnknownPackage && entry->parent)
	{
	name = new McString(getEntryName (entry->parent));

	if(name->length() > 0)
	    *name += ".";

	*name += entry->name;

	return *name;
	}
    else
	return entry->name;
}

static void dumpCxxItem(Entry *tmp)
{
    fprintf(out, "{");
    printCode(tmp, tmp->type);
    fprintf(out, "}\n        {");
    printCode(tmp, getEntryName(tmp));
    fprintf(out, "}\n        {");
    printCode(tmp, tmp->args);
    fprintf(out, "}\n        {");

    printYYDOC(tmp, tmp->memo.c_str());
    fprintf(out, "}\n        {");
    if(MAKE_DOC(tmp))
	{
	tmp->dumpNumber(out);
	fprintf(out, "}\n");
	}
    else
	{
	fprintf(out, "}\n");
	fprintf(out, "\\label{cxx.");
	tmp->dumpNumber(out);
	fprintf(out, "}\n");
	}
}

static int baseDepth(Entry *entry, int n = 1)
{
    if(!HAS_BASES(entry))
	return n;

    int i, j, nn;
    nn = n + 1;

    for(i = 0 ; i < entry->pubBaseclasses.size(); ++i)
	{
	j = baseDepth(entry->pubBaseclasses[i], n + 1);
	if(j > nn)
	    nn = j;
	}
    for(i = 0 ; i < entry->proBaseclasses.size(); ++i)
	{
	j = baseDepth(entry->proBaseclasses[i], n + 1);
	if(j > nn)
	    nn = j;
	}

    return nn;
}

void printYYGraph(const McString& str)
{
    inPos = 0;
    inStr = str.c_str();

    doc2texYYrestart(0);
    BEGIN(Graph);
    doc2texYYlex();
}

static void dumpInheritanceEntry(ClassGraph *cls)
{
    int i;

    fprintf(out, "\\cxxInheritanceEntry{");
	for(i = 0 ; i < cls->indent; ++i)
	    fprintf(out, "\\cxxNone");
    printYYGraph(cls->before);
    fprintf(out, "}{");

    if(cls->entry)
	if(!cls->entry->fromUnknownPackage && !onlyClassGraph)
	    cls->entry->dumpNumber(out);
    fprintf(out, "}{");

    if(cls->entry)
	printCode(cls->entry, getEntryName (cls->entry));
    else
	printCode(0, cls->name.c_str());
    
    fprintf(out, "}{");

    printYYGraph(cls->after);
    fprintf(out, "}\n");
}

static void dumpHierarchy(Entry *entry)
{
    int i;
    ClassGraph *cls;
    ClassGraph cg(entry, 0);
    cg.addBases();
    cg.addDirectChilds();

    fprintf(out, "\\begin{cxxInheritance}\n");
    fprintf(out, "\\paragraph{Inheritance}\\strut\\smallskip\\strut\\\\\n");

    for(cls = cg.firstLine; cls != &cg; cls = cls->nextLine)
	dumpInheritanceEntry(cls);

    fprintf(out, "\\cxxInheritanceEntry[x]{");
    for(i = 0; i < cls->indent; ++i)
	fprintf(out, "\\cxxNone");
    fprintf(out, "}{");

    if(cls->entry)
	if(!cls->entry->fromUnknownPackage && !onlyClassGraph)
	    cls->entry->dumpNumber(out);
    fprintf(out, "}{");
    if(cls->entry)
	printCode(cls->entry, getEntryName(cls->entry));
    else
	printCode(0, cls->name.c_str());
    
    fprintf(out, "}{");
    fprintf(out, "}\n");

    for(cls = cls->nextLine; cls; cls = cls->nextLine)
	dumpInheritanceEntry(cls);

    fprintf(out, "\\end{cxxInheritance}\n");
}

static char *checkStr(const char *str)
{
    char *new_str = (char *)malloc(strlen(str) + 1);
    char *return_str = new_str;
    bool sep_flag = false;

    strcpy(new_str, str);

    while(*new_str != '\0')
	if(*new_str == '\n')
	    strcpy(new_str, new_str + 1);
	else
	    {
	    if(*new_str == ' ')
		sep_flag = true;

	    new_str++;
	    }

    new_str = return_str;

    if(!sep_flag)
	{
	return_str = (char *)malloc(strlen(new_str) + 2);
	strcpy(return_str, new_str);
	strcat(return_str, " ");
	strcpy(new_str, return_str);
	}

    return_str = (char *)malloc(strlen(new_str) + 2);
    strcpy(return_str, new_str);
    strcat(return_str, "\n");

    return return_str;
}

static void dumpTeXList(Entry *entry, int *fields, McDArray<McString *> list,
			char *description)
{
    int i;

    if(list.size() > 0)
	{
	fprintf(out, "\\cxx%s{\n", description);
	fprintf(out, "\\begin{tabular}[t]{lp{0.5\\textwidth}}\n");

	for(i = 0; i < list.size(); ++i)
	    {
	    if(i)
		fprintf(out, "\\\\\n");
	    fprintf(out, "{\\tt\\strut ");
	    inPos = 0;
	    inStr = checkStr(list[i]->c_str());
	    current = entry;
	    doc2texYYrestart(0);
	    BEGIN(HTMLsyntax ? HTMLlist : TeXlist);
	    doc2texYYlex();
	    }
	fprintf(out, "\\end{tabular}}\n");
	(*fields)++;
	}
}

static void dumpTeXField(Entry *entry, int *fields, McString text, char *description)
{
    if(text.length() > 0)
	{
	fprintf(out, "\\cxx%s{\n", description);
	inPos = 0;
	inStr = text.c_str();
	current = entry;
	doc2texYYrestart(0);
	BEGIN(HTMLsyntax ? HTML : 0);
	doc2texYYlex();
	fprintf(out, "\\strut}\n");
	(*fields)++;
	}
}

static int dumpFields(Entry *entry)
{
    int	i, fields = 0;

    dumpTeXList(entry, &fields, entry->invariant, "Invariants");
    dumpTeXList(entry, &fields, entry->exception, "Exceptions");
    dumpTeXList(entry, &fields, entry->param, "Parameter");
    dumpTeXList(entry, &fields, entry->retrn, "Return");
    dumpTeXList(entry, &fields, entry->precondition, "Preconditions");
    dumpTeXList(entry, &fields, entry->postcondition, "Postconditions");

    dumpTeXField(entry, &fields, entry->author, "Author");
    dumpTeXField(entry, &fields, entry->version, "Version");
    dumpTeXField(entry, &fields, entry->deprecated, "Deprecated");
    dumpTeXField(entry, &fields, entry->since, "Since");

    if(entry->see.size())
	{
	fprintf(out, "\\cxxSee{");
	for(i = 0; i < entry->see.size(); ++i)
	    {
	    if(i)
		fprintf(out, "\\\\\n");
	    if(HTMLsyntax)
		fprintf(out, "{\\tt\\strut ");
	    inPos = 0;
	    inStr = entry->see[i]->c_str();
	    current = entry;
	    doc2texYYrestart(0);
	    BEGIN(HTMLsyntax ? HTMLlist : Code);
	    doc2texYYlex();
	    Entry *ref = getRefEntry(*entry->see[i], current);
	    if(ref)
		{
		fprintf(out, " ($\\rightarrow$");
		if(MAKE_DOC(ref))
		    {
		    ref->dumpNumber(out);
		    fprintf(out, ",");
		    }
		fprintf(out, " {\\em page \\pageref{cxx.");
		ref->dumpNumber(out);
		fprintf(out, "}})");
		}
	    }
	fprintf(out, "\\strut}\n");
	fields++;
	}

    return fields;
}

static void strlatex(char *s1, const char *s2)
{
    for(;; ++s2)
	{
	if((*s2) == '\0')
	    {
	    *s1 = *s2;
	    break;
	    }
	if((*s2) == '_')
	    {
	    *s1 = '\\';
	    ++s1;
	    }
	*s1 = *s2;++s1;
	}
}

void dumpEntry(Entry *entry)
{
    Entry *tmp;
    Entry *stack[20];
    int	fields;
    int	stack_cnt = 0;
    char *sectionType = 0;

    if(entry->fromUnknownPackage)
	return;

    if(generateIndex)
	{
 	fprintf(out, "\\index{");
     	tmp = entry;

     	while(tmp->parent)
	    {
 	    stack[stack_cnt++] = tmp;
 	    tmp = tmp->parent;
     	    }

     	while(stack_cnt)
     	    {
    	    tmp = stack[--stack_cnt];

 	    printYYDOC(tmp, tmp->name, 0);

 	    if(stack_cnt)
 		fprintf(out, "!");
     	    }

     	fprintf(out, "}\n");

     	switch(entry->section)
	    {
     	    case VARIABLE_SEC:
 		sectionType = "Variables/Constants";
 		break;
     	    case FUNCTION_SEC:
 		sectionType = "Functions/Methods";
 		break;
    	    case MACRO_SEC:
 		sectionType = "Macros";
 		break;
    	    case CLASS_SEC:
 		sectionType = "Classes";
 		break;
     	    case INTERFACE_SEC:
 		sectionType = "Interfaces";
 		break;
     	    case UNION_SEC:
 		sectionType = "Unions";
 		break;
     	    case NAMESPACE_SEC:
 		sectionType = "Namespaces";
 		break;
     	    default:
 		sectionType = (char *)0;
 	    }

 	if(sectionType)
 	    {
	    if(!(entry->section & CLASS_SEC) &&
		!(entry->section & INTERFACE_SEC) &&
		(entry->section != NAMESPACE_SEC))
		{
 	    	fprintf(out, "\\index{{\\bf %s}!", sectionType);
		printYYDOC(entry->parent, entry->parent->name, 0);
		fprintf(out, "!");
 	    	printYYDOC(entry, entry->name, 0);
 	    	fprintf(out, "}\n");
		}
	    else
		{
 	    	fprintf(out, "\\index{{\\bf %s}!", sectionType);
 	    	printYYDOC(entry, entry->name, 0);
 	    	fprintf(out, "}\n");
		}
 	    }
	}

    switch(entry->section)
	{
	case VARIABLE_SEC:
	    fprintf(out, "\\begin{cxxvariable}\n");
	    break;
	case FUNCTION_SEC:
	    fprintf(out, "\\begin{cxxfunction}\n");
	    break;
	case MACRO_SEC:
	    fprintf(out, "\\begin{cxxmacro}\n");
	    break;
	case CLASS_SEC:
	    fprintf(out, "\\begin{cxxclass}\n");
	    break;
	case INTERFACE_SEC:
	    fprintf(out, "\\begin{cxxinterface}\n");
	    break;
	case UNION_SEC:
	    fprintf(out, "\\begin{cxxunion}\n");
	    break;
	case NAMESPACE_SEC:
	    fprintf(out, "\\begin{cxxnamespace}\n");
	    break;
	default:
	    fprintf(out, "\\begin{cxxentry}\n");
	    break;
	}

    dumpCxxItem(entry);

    char file_name[BUFSIZ];
    strlatex(file_name, entry->file.c_str());
    if(showFilenames &&
	entry->section != PACKAGE_SEC &&
	entry->section != MANUAL_SEC)
	fprintf(out, "In file %s:%d \\\\\n", (const char *)file_name, entry->startLine);

    fields = dumpFields(entry);

    if(HAS_BASES(entry) || entry->pubChilds.size() || entry->proChilds.size())
	dumpHierarchy(entry);

    if(entry->sub)
	if((entry->section & INTERFACE_SEC) || (entry->section & CLASS_SEC))
	    {
	    for(tmp = entry->sub; tmp; tmp = tmp->next)
		if(tmp->protection == PUBL)
		    {
		    fprintf(out, "\\begin{cxxpublic}\n");
		    for(tmp = entry->sub; tmp; tmp = tmp->next)
			if(tmp->protection == PUBL)
			    {
			    fprintf(out, "\\cxxitem");
			    dumpCxxItem(tmp);
			    }
		    fprintf(out, "\\end{cxxpublic}\n");
		    break;
		    }
	    for(tmp = entry->sub; tmp; tmp = tmp->next)
		if(tmp->protection == PROT)
		    {
		    fprintf(out, "\\begin{cxxprotected}\n");
		    for(tmp = entry->sub; tmp; tmp = tmp->next)
			if(tmp->protection == PROT)
			    {
			    fprintf(out, "\\cxxitem");
			    dumpCxxItem(tmp);
			    }
		    fprintf(out, "\\end{cxxprotected}\n");
		    break;
		    }
	    if(withPrivate)
		{
		for(tmp = entry->sub; tmp; tmp = tmp->next)
		    if(tmp->protection == PRIV)
			{
			fprintf(out, "\\begin{cxxprivate}\n");
			for(tmp = entry->sub; tmp; tmp = tmp->next)
			    if(tmp->protection == PRIV)
				{
				fprintf(out, "\\cxxitem");
				dumpCxxItem(tmp);
				}
			fprintf(out, "\\end{cxxprivate}\n");
			break;
			}
		}
	    }
	else
	    if(!hideIndexOnEverySection)
		{
		fprintf(out, "\\begin{cxxnames}\n");
	    
		for(tmp = entry->sub; tmp; tmp = tmp->next)
		    {
		    fprintf(out, "\\cxxitem");
		    dumpCxxItem(tmp);
		    }
		fprintf(out, "\\end{cxxnames}\n");
		}

    if(entry->doc.length())
	{
	fprintf(out, "\\begin{cxxdoc}\n");
	printYYDOC(entry, entry->doc, 0);
	fprintf(out, "\n\\end{cxxdoc}\n");
	}

    for(tmp = entry->sub; tmp; tmp = tmp->next)
	if(tmp->protection == PUBL && MAKE_DOC(tmp))
	    dumpEntry(tmp);

    for(tmp = entry->sub; tmp; tmp = tmp->next)
	if(tmp->protection == PROT && MAKE_DOC(tmp))
	    dumpEntry(tmp);

    if(withPrivate)
	for(tmp = entry->sub; tmp; tmp = tmp->next)
	    if(tmp->protection == PRIV && MAKE_DOC(tmp))
		dumpEntry(tmp);

    switch(entry->section)
	{
	case VARIABLE_SEC:
	    fprintf(out, "\\end{cxxvariable}\n");
	    break;
	case FUNCTION_SEC:
	    fprintf(out, "\\end{cxxfunction}\n");
	    break;
	case UNION_SEC:
	    fprintf(out, "\\end{cxxunion}\n");
	    break;
	case MACRO_SEC:
	    fprintf(out, "\\end{cxxmacro}\n");
	    break;
	case CLASS_SEC:
	    fprintf(out, "\\end{cxxclass}\n");
	    break;
	case INTERFACE_SEC:
	    fprintf(out, "\\end{cxxinterface}\n");
	    break;
	case NAMESPACE_SEC:
	    fprintf(out, "\\end{cxxnamespace}\n");
	    break;
	default:
	    fprintf(out, "\\end{cxxentry}\n");
	    break;
	}
}

static int atMostDepth(Entry *tmp, int depth)
{
    if(depth && tmp)
	return atMostDepth(tmp->parent, depth - 1);

    return !tmp;
}

static bool relevantContents(Entry *tmp)
{
    for(tmp = tmp->sub; tmp; tmp = tmp->next)
	if(MAKE_DOC(tmp) && (atMostDepth(tmp, depthTOC + 1) ||
	    (tmp->section & CLASS_SEC) ||
	    tmp->section == UNION_SEC ||
	    tmp->section == MANUAL_SEC ||
	    (tmp->section & INTERFACE_SEC) ||
	    tmp->section == PACKAGE_SEC ||
	    tmp->section == NAMESPACE_SEC))
	    return true;
    return false;
}

static void dumpContents(Entry *tmp)
{
    for(tmp = tmp->sub; tmp; tmp = tmp->next)
	if(MAKE_DOC(tmp) && (atMostDepth(tmp, depthTOC + 1) ||
	    (tmp->section & CLASS_SEC) ||
	    tmp->section == UNION_SEC ||
	    tmp->section == MANUAL_SEC ||
	    (tmp->section & INTERFACE_SEC) ||
	    tmp->section == PACKAGE_SEC ||
	    tmp->section == NAMESPACE_SEC))
	    {
	    fprintf(out, "\\cxxContentsEntry{");
	    tmp->dumpNumber(out);
	    fprintf(out, "}{");
	    printCode(tmp, getEntryName(tmp));
	    fprintf(out, "}{");
	    printYYDOC(tmp, tmp->memo.c_str());
	    fprintf(out, "}\n");
	    if(tmp->sub && relevantContents(tmp))
		{
		fprintf(out, "\\begin{cxxContents}\n");
		dumpContents(tmp);
		fprintf(out, "\\end{cxxContents}\n");
		}
	    }
}

static void dumpClassGraph(Entry *entry)
{
    ClassGraph cg(entry, 0);
    ClassGraph *cls = &cg;
    cg.addBases();
    cg.addAllChilds();

    fprintf(out, "\\strut\\\\[5pt]\\goodbreak\n");

    if((cls->entry->fromUnknownPackage) || (onlyClassGraph))
    	fprintf(out, "\\cxxClassGraphEntryUnknownPackage{");
    else
	{
    	fprintf(out, "\\cxxClassGraphEntry{");

   	fprintf(out, "}{");
	cls->entry->dumpNumber(out);
	}

    fprintf(out, "}{");
    printCode(cls->entry, getEntryName(cls->entry));

    fprintf(out, "}{");
    fprintf(out, "}\n");

    for(cls = cls->nextLine; cls; cls = cls->nextLine)
	{
	if((cls->entry->fromUnknownPackage) || (onlyClassGraph))
	    fprintf(out, "\\cxxClassGraphEntryUnknownPackage{");
	else
	    fprintf(out, "\\cxxClassGraphEntry{");

	for(int i = 0; i < cls->indent; ++i)
	    fprintf(out, "\\cxxNone");

	printYYGraph(cls->before);

	if((!cls->entry->fromUnknownPackage) && (!onlyClassGraph))
	    {
	    fprintf(out, "}{");

	    cls->entry->dumpNumber(out);
	    }

	fprintf(out, "}{");

	printCode(cls->entry, getEntryName (cls->entry));

	fprintf(out, "}{");
	fprintf(out, "}\n");
	}
}

static void dumpClassGraphs(Entry *tmp)
{
    for(; tmp; tmp = tmp->next)
	{
	if((tmp->section & CLASS_SEC) &&
	    tmp->proBaseclasses.size() == 0 &&
	    tmp->pubBaseclasses.size() == 0)
	    {
	    tmp->currentParent = 0;
	    dumpClassGraph(tmp);
	    }
	dumpClassGraphs(tmp->sub);
	}
}

void usermanTeX(char *str, Entry *root)
{
    Entry *tmp;

    initLatex();

    if(root->sub && root->sub->next == 0 && root->sub->section == MANUAL_SEC)
	{
	root = root->sub;
	root->parent = 0;
	}

    if(!onlyClassGraph)
	{
    	if(root->name.length())
	    {
	    dumpFields(root);
	    fprintf(out, "\\cxxTitle");
	    dumpCxxItem(root);
    	    }

	fprintf(out, "\\begin{cxxContents}\n");
    	dumpContents(root);
	if(!noClassGraph && relevantClassGraphs(root))
    	    fprintf(out, "\\cxxContentsEntry{}{Class Graph}{}");
    	fprintf(out, "\\end{cxxContents}\n");

    	if(root->name.length())
    	    {
	    fprintf(out, "\\clearpage\\pagebreak\n");

	    if(root->doc.length())
		{
	    	fprintf(out, "\\begin{cxxdoc}\n");
	    	printYYDOC(root, root->doc, 0);
	    	fprintf(out, "\n\\end{cxxdoc}\n");
		}
    	    }

    	for(tmp = root->sub; tmp; tmp = tmp->next)
	    dumpEntry(tmp);
	}

    if(!noClassGraph && relevantClassGraphs(root))
	{
	fprintf(out, "\\begin{cxxClassGraph}\n");
	fprintf(out, "\\label{cxx.}\n");
	root->currentParent = 0;
	dumpClassGraphs(root);
	fprintf(out, "\\end{cxxClassGraph}\n");
	}

    finitLatex();
}
