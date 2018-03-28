/*
  config.ll

  Copyright (c) 2000-2001 Dragos Acostachioaie

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

#include <errno.h>
#include <stdio.h>

#include "McDArray.h"
#include "doc.h"

static const char*	inputString;
static int		inputPosition;
bool			reverseValue = false;
McString*		string_data;
long*			integer_data;
double*			real_data;
bool*			boolean_data;
McDArray<McString *>*	list_data;

bool			onlyDocs       			= true;
bool			commentExtn    			= false;
McString		configFile			("doc++.conf");
bool			HTMLsyntax     			= false;
McString		fileList;
bool			java           			= false;
bool			php           			= false;
bool			noDefines			= false;
bool			withPrivate    			= false;
bool			fastNotSmall   			= false;
bool			QuantelExtn    			= false;
bool			internalDoc			= false;
bool			doTeX          			= false;
bool			upArrows       			= false;
bool			verb				= false;
bool			scanIncludes			= false;
bool			idl				= false;
bool			doDOCBOOK			= false;
bool			doDOCBOOKXML			= false;
bool			doXML				= false;
McDArray<McString *>	inputFiles;

bool			withTables     			= false;
bool			withBorders    			= false;
McString		ownFooter;
McString		outputDir			= ".";
bool			showFilenames  			= false;
bool			showFilePath   			= false;
bool			noGifs         			= false;
bool			forceGifs      			= false;
bool			showInherited  			= true;
bool			javaGraphs     			= true;
bool			trivialGraphs  			= false;
bool			alwaysPrintDocSection   	= true;
bool			showMembersInTOC		= false;
bool			useGeneral     			= true;
bool			sortEntries    			= false;
McString		ownHeader;
bool			printGroupDocBeforeGroup	= false;
bool			printClassDocBeforeGroup	= false;
McString		htmlSuffix			(".html");
McString		htmlStyleSheet;

bool			onlyClassGraph			= false;
McString		texFile;
bool			generateIndex			= false;
McString		texOption;
McDArray<McString *>	texPackages;
McString		texTitle;
int			depthTOC			= 1;
bool			noLatex        			= false;
bool			noClassGraph			= false;
McString		texOutputName;
bool			doListing			= false;
bool			hideIndexOnEverySection		= false;

#undef YY_INPUT
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

%x	Start
%x	SkipComment
%x	String
%x	Integer
%x	Real
%x	Boolean
%x	List

%%

<*>\x0d

<Start>"#"		{
			BEGIN(SkipComment);
			}

<Start>"documentAll"[ \t]*	{
			reverseValue = true;
			boolean_data = &onlyDocs;
			BEGIN(Boolean);
			}

<Start>"useNormalComments"[ \t]*	{
			boolean_data = &commentExtn;
			BEGIN(Boolean);
			}

<Start>"HTMLSyntax"[ \t]*	{
			boolean_data = &HTMLsyntax;
			BEGIN(Boolean);
			}

<Start>"fileList"[ \t]*	{
			string_data = &fileList;
			BEGIN(String);
			}

<Start>"parseJava"[ \t]*	{
			boolean_data = &java;
			BEGIN(Boolean);
			}

<Start>"ignoreDefines"[ \t]*	{
			boolean_data = &noDefines;
			BEGIN(Boolean);
			}

<Start>"documentPrivateMembers"[ \t]*	{
			boolean_data = &withPrivate;
			BEGIN(Boolean);
			}

<Start>"optimizeForSpeed"[ \t]*	{
			boolean_data = &fastNotSmall;
			BEGIN(Boolean);
			}

<Start>"quantelExtensions"[ \t]*	{
			boolean_data = &QuantelExtn;
			BEGIN(Boolean);
			}

<Start>"internalDoc"[ \t]*	{
			boolean_data = &internalDoc;
			BEGIN(Boolean);
			}

<Start>"doTeX"[ \t]*	{
			boolean_data = &doTeX;
			BEGIN(Boolean);
			}

<Start>"upwardsArrows"[ \t]*	{
			boolean_data = &upArrows;
			BEGIN(Boolean);
			}

<Start>"verboseOperation"[ \t]*	{
			boolean_data = &verb;
			BEGIN(Boolean);
			}

<Start>"scanIncludes"[ \t]*	{
			boolean_data = &scanIncludes;
			BEGIN(Boolean);
			}

<Start>"parseIDL"[ \t]*	{
			boolean_data = &idl;
			BEGIN(Boolean);
			}

<Start>"parsePHP"[ \t]*	{
			boolean_data = &php;
			BEGIN(Boolean);
			}

<Start>"doDocBook"[ \t]*	{
			boolean_data = &doDOCBOOK;
			BEGIN(Boolean);
			}

<Start>"doDocBookXXL"[ \t]*	{
			boolean_data = &doDOCBOOKXML;
			BEGIN(Boolean);
			}

<Start>"doXML"[ \t]*	{
			boolean_data = &doXML;
			BEGIN(Boolean);
			}

<Start>"inputFiles"[ \t]*	{
			list_data = &inputFiles;
			BEGIN(List);
			}

<Start>"useTables"[ \t]*	{
			boolean_data = &withTables;
			BEGIN(Boolean);
			}

<Start>"useTablesWithBorders"[ \t]*	{
			boolean_data = &withBorders;
			BEGIN(Boolean);
			}

<Start>"footer"[ \t]*	{
			string_data = &ownFooter;
			BEGIN(String);
			}

<Start>"outputDir"[ \t]*	{
			string_data = &outputDir;
			BEGIN(String);
			}

<Start>"showFilenames"[ \t]*	{
			boolean_data = &showFilenames;
			BEGIN(Boolean);
			}

<Start>"showFilenamesWithPath"[ \t]*	{
			boolean_data = &showFilePath;
			BEGIN(Boolean);
			}

<Start>"noGifs"[ \t]*	{
			boolean_data = &noGifs;
			BEGIN(Boolean);
			}

<Start>"forceGifs"[ \t]*	{
			boolean_data = &forceGifs;
			BEGIN(Boolean);
			}

<Start>"noInheritedMembers"[ \t]*	{
			reverseValue = true;
			boolean_data = &showInherited;
			BEGIN(Boolean);
			}

<Start>"noJavaGraphs"[ \t]*	{
			reverseValue = true;
			boolean_data = &javaGraphs;
			BEGIN(Boolean);
			}

<Start>"trivialGraphs"[ \t]*	{
			boolean_data = &trivialGraphs;
			BEGIN(Boolean);
			}

<Start>"noMembers"[ \t]*	{
			reverseValue = true;
			boolean_data = &alwaysPrintDocSection;
			BEGIN(Boolean);
			}

<Start>"showMembersInTOC"[ \t]*	{
			boolean_data = &showMembersInTOC;
			BEGIN(Boolean);
			}

<Start>"discardGeneral"[ \t]*	{
			reverseValue = true;
			boolean_data = &useGeneral;
			BEGIN(Boolean);
			}

<Start>"sortEntries"[ \t]*	{
			boolean_data = &sortEntries;
			BEGIN(Boolean);
			}

<Start>"header"[ \t]*	{
			string_data = &ownHeader;
			BEGIN(String);
			}

<Start>"groupBeforeGroup"[ \t]*	{
			boolean_data = &printGroupDocBeforeGroup;
			BEGIN(Boolean);
			}

<Start>"classBeforeGroup"[ \t]*	{
			boolean_data = &printClassDocBeforeGroup;
			BEGIN(Boolean);
			}

<Start>"htmlSuffix"[ \t]*	{
			string_data = &htmlSuffix;
			BEGIN(String);
			}

<Start>"htmlStyleSheet"[ \t]*	{
			string_data = &htmlStyleSheet;
			BEGIN(String);
			}

<Start>"onlyClassGraph"[ \t]*	{
			boolean_data = &onlyClassGraph;
			BEGIN(Boolean);
			}

<Start>"environment"[ \t]*	{
			string_data = &texFile;
			BEGIN(String);
			}

<Start>"generateIndex"[ \t]*	{
			boolean_data = &generateIndex;
			BEGIN(Boolean);
			}

<Start>"style"[ \t]*	{
			// FIXME: this should be a list of style options
			string_data = &texOption;
			BEGIN(String);
			}

<Start>"usePackage"[ \t]*	{
			list_data = &texPackages;
			BEGIN(List);
			}

<Start>"title"[ \t]*	{
			string_data = &texTitle;
			BEGIN(String);
			}

<Start>"minimumDepth"[ \t]*	{
			integer_data = (long *)&depthTOC;
			BEGIN(Integer);
			}

<Start>"noEnvironment"[ \t]*	{
			boolean_data = &noLatex;
			BEGIN(Boolean);
			}

<Start>"noClassGraph"[ \t]*	{
			boolean_data = &noClassGraph;
			BEGIN(Boolean);
			}

<Start>"outputFilename"[ \t]*	{
			string_data = &texOutputName;
			BEGIN(String);
			}

<Start>"generateSourceListing"[ \t]*	{
			boolean_data = &doListing;
			BEGIN(Boolean);
			}

<Start>"hideIndex"[ \t]*	{
			boolean_data = &hideIndexOnEverySection;
			BEGIN(Boolean);
			}

<Start>[a-z_A-Z0-9]+	{
			fprintf(stderr, "Unknown token `%s'\n", yytext);
			}

<SkipComment>.*\n	{
			BEGIN(Start);
			}

<String>[^ \"\t\r\n]+	{
			*string_data = yytext;
			}

<Boolean>[^ \"\t\r\n]+	{
			if(strcmp(yytext, "yes") == 0 || strcmp(yytext, "true") == 0)
			    *boolean_data = true;
			else
			    if(strcmp(yytext, "no") == 0 || strcmp(yytext, "false") == 0)
				*boolean_data = false;
			    else
				fprintf(stderr, "Invalid value `%s' for token\n",
				    yytext);
			if(reverseValue)
			    {
			    *boolean_data = !(*boolean_data);
			    reverseValue = false;
			    }
			}

<Integer>[^ \"\t\r\n]+	{
			long tmp = strtol(yytext, 0, 0);
			if(errno == ERANGE)
			    fprintf(stderr, "Invalid value `%s' for token\n",
				yytext);
			else
			    *integer_data = tmp;
			}

<List>[^ ,;\"\t\r\n]+	{
			list_data->append(new McString(yytext));
			}

<String,Boolean,Integer,List>\n	{
			BEGIN(Start);
			}

<*>.
<*>\n

%%

void parseConfig(const McString s)
{
    inputString = s;
    inputPosition = 0;
    configYYrestart(configYYin);
    BEGIN(Start);
    configYYlex();
}

extern "C" {
    int configYYwrap()
	{
	return 1;
	}
};
