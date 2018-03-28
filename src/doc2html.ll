/*
  doc2html.ll

  Copyright (c) 1996 Roland Wunderling, Malte Zoeckler
  Copyright (c) 1998 Michael Meeks
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
#include <string.h>
#include <time.h>

#include "doc.h"
#include "nametable.h"

extern NameTable	gifs;

#define YY_DECL int yylex()

#undef	YY_INPUT
#define	YY_INPUT(buf, result, max_size) result = yyread(buf, max_size);

#define forbidden(c)	(c == ' ' || c == '\t' || c == '\n' || c == '}' || \
			c == ']' || c == '{' || c == '[' )

McString outStr;
static int inputPosition = 0, inLen = 0, texBracketCount = 0;
static const char *inStr;
static void push(char *);
static char *pop();
static void Output(const char *);
static void Output(const McString& o);
static char *tabFlag = 0;
static int doItem = 0;
static bool yyWithLinks = false;
static McString img, imgArg1, imgArg2;

static Entry *context;

static int yyread(char *buf, int max_size)
{
    int c = 0;
    while(c < max_size && inputPosition < inLen)
	{
	*buf = inStr[inputPosition++];
	c++;
	buf++;
	}
    return c;
}

static void outputTeX(const char *text)
{
    int num = 0;

    if(strstr(text, "\\hspace") == text)
	/* if the command starts with hspace it can not be visualized in a gif*/
	return;

    if(noGifs)
	{
	Output("<PRE>");
	Output(text);
	Output("</PRE>");
	}
    if(!gifs.has(text))
	{
	gifs.add(gifs.num() + 1, text);
	num = gifs.num();
	}
    else
	num = gifs[text];

    char buf[200];
    sprintf(buf, "<IMG BORDER=0 SRC=g%06d.gif>", num);
    Output(buf);
}

static char *getArg()
{
    yytext[--yyleng] = 0;
    while(forbidden(yytext[yyleng - 1]))
	yytext[--yyleng] = 0;

    while(yytext[yyleng - 1] != '{' && yytext[yyleng - 1] != '[')
	--yyleng;
    return &yytext[yyleng];
}

static char *getArgContainingSpaces()
{
    yytext[--yyleng] = 0;
    while(forbidden(yytext[yyleng - 1]))
	yytext[--yyleng] = 0;

    while(yytext[yyleng - 1] != '{' && yytext[yyleng - 1] != '[' && yytext[yyleng - 1] != ' ')
	--yyleng;
    return &yytext[yyleng];
}

static int initial;
    
%}
spaces  [ \t]*
name    [({alpha}|\_)({alpha}|{dig}|\_)*
num1    {dig}+\.?([eE][-+]?{dig}+)?
num2    {dig}*\.{dig}+([eE][-+]?{dig}+)?
BeginTab "\\begin{tabular}{"[lrc|]+"}"
arg      [^\n}\]]*


%x	Verbatim
%x	texmode
%x	Fence
%x	equation
%x	eqnarray
%x	eqn
%x	texgif
%x	JAVA_SEE
%x	Image
%x	ImageArg1
%x	ImageArg2

%x	DXXisCode
%x	DXXisHTML
%x	DXXisTeX

%%

<DXXisHTML,DXXisTeX>"\\Ref{"{arg}"}"	{
			char *arg = getArg();
			McString bla, tmp;
			tmp = arg;
			Entry *ref = getRefEntry(tmp, context);
			if(ref)
			    {
			    entry2link(bla, ref);
			    Output(bla);
			    }
			else
			    Output(arg);
                        }

<DXXisHTML,DXXisTeX>"{@link"{spaces}{arg}{spaces}{arg}"}"	{
                        char *arg2 = getArgContainingSpaces();
			char *arg1 = getArgContainingSpaces();
			McString bla, tmp(arg1);
			Entry *ref = getRefEntry(tmp, context);
			if(ref)
			    {
			    entry2link(bla, ref, arg2);
			    Output(bla);
			    }
			else
			    Output(arg2);
			}

<DXXisHTML,DXXisTeX>"\\URL["{arg}"]{"{arg}"}"	{
			Output("<A HREF=");
			char *arg2 = getArg();
                        char *arg = getArg();
                        Output(arg2);
                        Output(">");
			Output(arg);
			Output("</A>");
            		}

<DXXisHTML,DXXisTeX>"\\URL{"{arg}"}"	{
			Output("<A HREF=");
			char *arg = getArg();
                        Output(arg);
                        Output(">");
			Output(arg);
			Output("</A>");
                        }

<DXXisHTML,DXXisTeX>"\\Label{"{arg}"}"	{
			Output("<A NAME=");
			char *arg = getArg();
			Output(arg);
			Output("></A>");
			}

<DXXisHTML,DXXisTeX>"\\"([I|i][M|m][G|g]|"includegraphics")[ \t]*"{"	{
			img.clear();
			imgArg1.clear();
			imgArg2.clear();
			BEGIN(Image);
			}

<DXXisHTML,DXXisTeX>"\\"([I|i][M|m][G|g]|"includegraphics")[ \t]*"["	{
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
        		    Output("<IMG ");
        		    if(imgArg1.length() > 0)
            			{
            			Output(imgArg1);
            			Output(" ");
            			}
        		    Output("SRC=\"");
        		    Output(img);
        		    // check for extension
			    if(img.index('.') == -1)  // no extension: add `.gif'
            			Output(".gif");
        		    Output("\">");
			    }
			img.clear();
			imgArg1.clear();
			imgArg2.clear();
                	BEGIN(initial);
                	}

<DXXisHTML,DXXisTeX>"\\"([D|d][A|a][T|t][E|e]|"today")	{
			time_t ltime;
			(void)time(&ltime);
			char timebuf[64];
			strftime(timebuf, 64, "%c", localtime(&ltime));
			Output(timebuf);
			}

<DXXisTeX>"{"{spaces}"\\em"{spaces}	{
			Output("<EM>");
			push("</EM>");
			}

<DXXisTeX>"\\emph{"{spaces}	{
			Output("<EM>");
			push("</EM>");
			}

<DXXisTeX>"{"{spaces}"\\it"{spaces}	{
			Output("<I>");
			push("</I>");
			}

<DXXisTeX>"\\textit{"{spaces}	{
			Output("<I>");
			push("</I>");
			}

<DXXisTeX>"{"{spaces}"\\tiny"{spaces}	{
			Output("<FONT SIZE=\"-4\">");
			push("</FONT>");
			}

<DXXisTeX>"{"{spaces}"\\scriptsize"{spaces}	{
			Output("<FONT SIZE=\"-3\">");
			push("</FONT>");
			}

<DXXisTeX>"{"{spaces}"\\footnotesize"{spaces}	{
			Output("<FONT SIZE=\"-2\">");
			push("</FONT>");
			}

<DXXisTeX>"{"{spaces}"\\small"{spaces}	{
			Output("<FONT SIZE=\"-1\">");
			push("</FONT>");
			}

<DXXisTeX>"{"{spaces}"\\large"{spaces}	{
			Output("<FONT SIZE=\"+1\">");
			push("</FONT>");
			}

<DXXisTeX>"{"{spaces}"\\Large"{spaces}	{
			Output("<FONT SIZE=\"+2\">");
			push("</FONT>");
			}

<DXXisTeX>"{"{spaces}"\\LARGE"{spaces}	{
			Output("<FONT SIZE=\"+3\">");
			push("</FONT>");
			}

<DXXisTeX>"{"{spaces}"\\huge"{spaces}	{
			Output("<FONT SIZE=\"+4\">");
			push("</FONT>");
			}

<DXXisTeX>"{"{spaces}"\\Huge"{spaces}	{
			Output("<FONT SIZE=\"+5\">");
			push("</FONT>");
			}

<DXXisTeX>"{"{spaces}"\\HUGE"{spaces}	{
			Output("<FONT SIZE=\"+6\">");
			push("</FONT>");
			}

<DXXisTeX>"{"{spaces}"\\tt"{spaces}	{
			Output("<TT>");
			push("</TT>");
			}

<DXXisTeX>"\\texttt{"{spaces}	{
			Output("<TT>");
			push("</TT>");
			}

<DXXisTeX>"{"{spaces}"\\bf"{spaces}	{
			Output("<B>");
			push("</B>");
			}

<DXXisTeX>"\\textbf{"{spaces}	{
			Output("<B>");
			push("</B>");
			}

<DXXisTeX>"}"		{
			Output(pop());
			}

<DXXisTeX>"{"           {
			Output("{");
			push("}");
			}

<DXXisTeX>"\\{"        	{
			Output("{");
			}

<DXXisTeX>"\\"[T|t][E|e][X|x][ \t]*"{"	{
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
			texBracketCount++;
			yymore();
			}

<texmode>"}"		{
			if(--texBracketCount == 0)
			    {
			    yytext[--yyleng] = 0;
			    outputTeX(yytext);
			    BEGIN(initial);
			    }
			else
			    yymore();
			}

<texmode>.|\n		{
			yymore();
			}


<DXXisTeX>"\\begin{verbatim}"	{
			Output("<PRE>");
			BEGIN(Verbatim);
			}

<DXXisTeX>"\\begin{equation}"	{
			yymore();
			BEGIN(equation);
			}

<equation>.|\n		{
			yymore();
			}

<equation>"\\end{equation"[ *]*"}"	{
                        Output("<BR><CENTER>");
			outputTeX(yytext);
			Output("<BR></CENTER>");
			BEGIN(initial);
			}

<DXXisTeX>"\\["		{
			yymore();
			BEGIN(eqn);
			}

<eqn>.|\n		{
			yymore();
			}

<eqn>"\\]"		{
                        Output("<BR><CENTER>");
			outputTeX(yytext);
			Output("<BR></CENTER>");
			BEGIN(initial);
			}

<DXXisTeX>\$[^$]+\$	{
			outputTeX(yytext);
			}

<DXXisTeX>"\\begin{eqnarray}"	{
			yymore();
			BEGIN(eqnarray);
			}

<eqnarray>.|\n		{
			yymore();
			}

<eqnarray>"\\end{eqnarray}"	{
                        Output("<BR><CENTER>");
			outputTeX(yytext);
			Output("<BR></CENTER>");
			BEGIN(initial);
			}

<DXXisTeX>"\\begin{eqnarray}"[^}]*"}"	{
			Output("<PRE>");
			BEGIN(Verbatim);
			}

<DXXisTeX>"\$\$"	{
			Output("<PRE>");
			BEGIN(Verbatim);
			}

<DXXisTeX>"\\begin{center}"	{
			Output("<CENTER>");
			}

<DXXisTeX>"\\end{center}"	{
			Output("</CENTER>");
			}

<DXXisTeX>"\\begin{flushleft}"	{
			Output(" ");
			}

<DXXisTeX>"\\end{flushleft}"	{
			Output(" ");
			}

<DXXisTeX>"\\begin{flushright}"	{
			Output(" ");
			}

<DXXisTeX>"\\end{flushright}"	{
			Output(" ");
			}

<DXXisTeX>"\\begin{itemize}"	{
			Output("<UL>");
			}

<DXXisTeX>"\\end{itemize}"	{
			Output("</UL>");
			}

<DXXisTeX>"\\begin{enumerate}"	{
			Output("<OL>");
			}

<DXXisTeX>"\\item"	{
			Output("<LI>");
			}

<DXXisTeX>"\\end{enumerate}"	{
			Output("</OL>");
			}

<DXXisTeX>"\\begin{description}"	{
			Output("<DL>");
			}

<DXXisTeX>"\\item["	{
			Output("<DT><B>");
			doItem = 1;
			}

<DXXisTeX>"]"		{
			if(doItem )
			    {
			    Output("</B><DD>");
			    doItem = 0;
			    }
			else
			    Output("]");
			}

<DXXisTeX>"\\end{description}"	{
			Output("</DL>");
			}

<Verbatim>"\\end{verbatim}"	{
			Output("</PRE>");
			BEGIN(initial);
			}

<Verbatim>"\n"		{
			Output(yytext);
			}

<DXXisTeX>"\\hline"	{
			if(tabFlag)
			    Output("</TD></TR><TR><TD>");
			else
			    Output("<HR>");
			}

<DXXisTeX>"\\c{c}"	{
			Output("&ccedil;");
			}

<DXXisTeX>"\\c{C}"	{
			Output("&Ccedil;");
			}

<DXXisTeX>"\\`a"	{
			Output("&agrave;");
			}

<DXXisTeX>"\\^a"	{
			Output("&acirc;");
			}

<DXXisTeX>"\\\"a"	{
			Output("&auml;");
			}

<DXXisTeX>"\\\"A"	{
			Output("&Auml;");
			}

<DXXisTeX>"{\\\"a}"	{
			Output("&auml;");
			}

<DXXisTeX>"{\\\"A}"	{
			Output("&Auml;");
			}

<DXXisTeX>"\\'e"	{
			Output("&eacute;");
			}

<DXXisTeX>"\\`e"	{
			Output("&egrave;");
			}

<DXXisTeX>"\\^e"	{
			Output("&ecirc;");
			}

<DXXisTeX>"\\\"e"	{
			Output("&euml;");
			}

<DXXisTeX>"\\'E"	{
			Output("&Eacute;");
			}

<DXXisTeX>"\\`E"	{
			Output("&Egrave;");
			}

<DXXisTeX>"\\^E"	{
			Output("&Ecirc;");
			}

<DXXisTeX>"\\\"E"	{
			Output("&Euml;");
			}

<DXXisTeX>"\\^i"	{
			Output("&icirc;");
			}

<DXXisTeX>"\\^I"	{
			Output("&Icirc;");
			}

<DXXisTeX>"\\\"o"	{
			Output("&ouml;");
			}

<DXXisTeX>"\\\"O"	{
			Output("&Ouml;");
			}

<DXXisTeX>"{\\\"o}"	{
			Output("&ouml;");
			}

<DXXisTeX>"{\\\"O}"	{
			Output("&Ouml;");
			}

<DXXisTeX>"\\^o"	{
			Output("&ocirc;");
			}

<DXXisTeX>"\\^O"	{
			Output("&Ocirc;");
			}

<DXXisTeX>"\\\"u"	{
			Output("&uuml;");
			}

<DXXisTeX>"\\\"U"	{
			Output("&Uuml;");
			}

<DXXisTeX>"{\\\"u}"	{
			Output("&uuml;");
			}

<DXXisTeX>"{\\\"U}"	{
			Output("&Uuml;");
			}

<DXXisTeX>"\\^u"	{
			Output("&ucirc;");
			}

<DXXisTeX>"\\^U"	{
			Output("&Ucirc;");
			}

<DXXisTeX>"\\`u"	{
			Output("&ugrave;");
			}

<DXXisTeX>"\\`U"	{
			Output("&Ugrave;");
			}

<DXXisTeX>"\\ss"	{
			Output("&szlig;");
			}

<DXXisTeX>"{\\ss}"	{
			Output("&szlig;");
			}

<DXXisTeX>"\\3"		{
			Output("&#223;");
			}

<DXXisTeX>"\\#"		{
			Output("#");
			}

<DXXisTeX>"#define"	{
			Output("#define");
			}

<DXXisTeX>"\\ "		{
			Output(" ");
			}

<DXXisTeX>"\\_"		{
			Output("_");
			}

<DXXisTeX>"\\%"		{
			Output("%");
			}

<DXXisTeX>"\\&"		{
			Output("&amp;");
			}

<DXXisCode,DXXisTeX>"<"	{
			Output("&lt;");
			}

<DXXisCode,DXXisTeX>">"	{
			Output("&gt;");
			}

<DXXisCode>"&"		{
			Output("&amp;");
			}

<DXXisTeX>"#"		{
			Output("<TT>");
			BEGIN(Fence);
			}

<Fence>[#\n]		{
			Output("</TT>");
			BEGIN(initial);
			}

<Fence,Verbatim>"&"	{
			Output("&amp;");
			}

<Fence,Verbatim>"<"	{
			Output("&lt;");
			}

<Fence,Verbatim>">"	{
			Output("&gt;");
			}

<Fence,Verbatim>"\""	{
			Output("&quot;");
			}

<Fence,Verbatim>"\ "	{
			Output("&nbsp;");
			}

<Fence,Verbatim>.	{
			Output(yytext);
			}

<DXXisTeX>{BeginTab}	{
			Output("<TABLE BORDER>\n<TR><TD>");
			tabFlag++;
			}

<DXXisTeX>"&"		{
			if(tabFlag)
			    Output("</TD><TD>");
			else
			    Output("&amp;");
			}

<DXXisTeX>"\\\\"	{
			if(tabFlag)
			    Output("</TD></TR><TR><TD>");
			else
			    Output("<BR>");
			}

<DXXisTeX,DXXisHTML>"\n"[ \t]*"\n"	{
			Output("\n\n<P>");
			}

<DXXisTeX>"\\end{tabular}"	{
			Output("</TR></TABLE>");
			tabFlag--;
			}

<DXXisCode,DXXisTeX,DXXisHTML,JAVA_SEE>[a-z_A-Z0-9:.]*	{
			if(yyWithLinks)
			    {
                            Entry *ref = getRefEntry(yytext, context);
			    if(ref)
				{
				McString tmp;
				entry2link(tmp, ref, (const char *)yytext);
				Output(tmp);			
		    		}
			    else
				Output(yytext);
                    	    }
			else
			    Output(yytext);
    			}

<JAVA_SEE>[A-Za-z_.0-9]*"#"[A-Za-z_.0-9]*	{
			McString tmp(yytext);
                        McString base(tmp, 0, tmp.index('#'));
                        McString label(tmp, tmp.index('#') + 1, tmp.length() - tmp.index('#') - 1);
                        if(base.length())
			    {
	                    if(context)
				context->makeFullName(base);
		            base += htmlSuffix;
			    }
                        Output("<A HREF=\"");
                        Output(base); 
                        Output("#"); 
                        Output(label); 
                        Output("\">"); 
                        Output(label); 
                        Output("</A>"); 
                	}

<JAVA_SEE>.		{
			Output(yytext);
			}

<JAVA_SEE>\n		{
			Output(yytext);
			}
 		     
<DXXisCode,DXXisTeX,DXXisHTML>. 	{
			Output(yytext);
			}

<DXXisCode,DXXisTeX,DXXisHTML>"\n"	{
			Output(yytext);
			}

<*>\n
%%

McDArray <char *> stack;

extern "C" {
    int doc2htmlYYwrap()
	{
	return 1;
	}
}

void push(char *s)
{
    stack.append(s);
}

char *pop()
{
    if(stack.size())
	{
	char *s = stack.last();
	stack.removeLast();
	return s;
	}
    else
	return ("}");
}

static void Output(const char *o)
{
    outStr += o;
}

static void Output(const McString& o)
{
    outStr += o;
}

// This is really called lots, so add a quick exit if nothing to do!
static char blank[] = "";
char *strToHtml(McString &in, char *dest, Entry* ct, bool withLinks, bool pureCode)
{
    if(in.length() == 0) // This really does save time.
	if(dest)
	    {
	    strcpy(dest, blank);
	    return dest;
	    }
	else
    	    return blank;

    context = ct ? ct : root;
    inStr = in.c_str();
    outStr = "";
    inLen = strlen(inStr);
    inputPosition = 0;
    stack.resize(0);
    yyWithLinks = withLinks;
    if(pureCode)
        initial = DXXisCode;
    else
	if(HTMLsyntax)
	    initial = DXXisHTML;
	else
	    initial = DXXisTeX;
    BEGIN(initial);
    doc2htmlYYlex();
    if(dest)
	{
	strcpy(dest, outStr.c_str());
	return dest;
	}
    else 
	return strdup(outStr.c_str());
}

char *seeToHtml(McString &in, Entry *ct)
{
    if(in.length() == 0)
	return blank;
    context = ct ? ct : root;
    inStr = in.c_str();
    outStr = "";
    inLen = strlen(inStr);
    inputPosition = 0;
    stack.resize(0);
    yyWithLinks = true;
    BEGIN(JAVA_SEE);
    doc2htmlYYlex();
    return strdup(outStr.c_str());
}
