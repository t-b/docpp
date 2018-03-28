/*
  readfiles.ll

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
#include <sys/types.h>

#include "McDirectory.h"
#include "McSorter.h"
#include "McString.h"
#include "doc.h"
#include "nametable.h"

static NameTable	fileTable;
static McString*	inputFile;
static McString		fileName;
static McString		currDir;
static int		lineNr, obr = 0, cbr = 0;

static void startfile(const McString& file, int line)
{
    lineNr = line;
    fileName = file.c_str();
    *inputFile += char(6);
    *inputFile += file;
    *inputFile += char(6);
    *inputFile += '\n';

    /* Add a marker designating the filename into input stream. This will be
       parsed and used by code in `cpp.ll' and `java.ll'. Without this marker
       all files which get included by an `@Include' statement will get the
       filename of the outermost include file.
       The reason for this is the two stage strategy for file access DOC++ uses:
       1. read all files from all includes into one big buffer
       2. parse that buffer as one single file (with one filename)
    */
    *inputFile += "//$$filename ";
    *inputFile += file;
    *inputFile += '\n';
}

extern "C" {
    int readfilesYYwrap()
	{
	return 1;
	}
}

%}

%x Include
%x Includetwo
%x IncludeHeader
%x CharConst
%x StrConst
%x Comment
%x LineComment

%%
^[ \t]*"/*@Include:"[ \t]*	{
				BEGIN(Includetwo);
				}

<Includetwo>[^ \t\n]*		{
                                int tmpNr = lineNr;
                                McString tmpName = fileName;
                                McString tmpDir = currDir;
                                YY_BUFFER_STATE tmpBuf = YY_CURRENT_BUFFER;
                                readfile(inputFile, yytext, 1, currDir);
                                currDir = tmpDir;
                                startfile(tmpName, tmpNr);
                                yy_switch_to_buffer(tmpBuf);
                                BEGIN(Includetwo);
				}

<Includetwo>[ \t]*"*/"[ \t]*\n	{
				*inputFile += *yytext;
				lineNr++;
				BEGIN(0);
				}

^[ \t]*"//@Include:"[ \t]*	{
				BEGIN(Include);
				}

<Include>[^ \t\n]*		{
				int tmpNr = lineNr;
				McString tmpName = fileName;
				McString tmpDir = currDir;
				YY_BUFFER_STATE	tmpBuf = YY_CURRENT_BUFFER;
				readfile(inputFile, yytext, 1, currDir);
				currDir = tmpDir;
				startfile(tmpName, tmpNr);
				yy_switch_to_buffer(tmpBuf);
				BEGIN(Include);
				}

<Include>.
<Include>\n			{
				*inputFile += *yytext;
				lineNr++;
				BEGIN(0);
				}

^[ \t]*"#include"[ \t]*[<"]	{
				BEGIN(IncludeHeader);
				}

<IncludeHeader>[^>"]*		{
				if(scanIncludes)
				    {
				    int tmpNr = lineNr;
				    McString tmpName = fileName;
				    McString tmpDir = currDir;
				    YY_BUFFER_STATE tmpBuf = YY_CURRENT_BUFFER;
				    readfile(inputFile, yytext, 1, currDir);
				    currDir = tmpDir;
				    startfile(tmpName, tmpNr);
				    yy_switch_to_buffer(tmpBuf);
				    }
				}

<IncludeHeader>[>"].*\n		{
				lineNr++;
				BEGIN(0);
				}

"/*i"				{
				*inputFile += "/*";
				if(internalDoc)
				    *inputFile += '*';
				BEGIN(0);
				}

"//i"				{
				*inputFile += "//";
				if(internalDoc)
				    *inputFile += '/';
				BEGIN(0);
				}

"/*e"				{
				*inputFile += "/**";
				BEGIN(0);
				}

"//e"				{
				*inputFile += "///";
				BEGIN(0);
				}

'                               {
                                *inputFile += '\'';
                                BEGIN(CharConst);
                                }

<CharConst>\\'                  {
				*inputFile += "\\'";
				}

<CharConst>'                    {
                                *inputFile += '\'';
                                BEGIN(0);
                                }

\"                              {
                                *inputFile += '"';
                                BEGIN(StrConst);
                                }

<StrConst>\\\"                  {
				*inputFile += "\\\"";
				}

<StrConst>\"                    {
                                *inputFile += '\"';
                                BEGIN(0);
                                }

<CharConst,StrConst>\\\\        {
				*inputFile += "\\\\";
				}

"/*"                            {
                                *inputFile += "/*";
                                BEGIN(Comment);
                                }

<Comment>"*/"                   {
                                *inputFile += "*/";
                                BEGIN(0);
                                }

"//"                            {
				*inputFile += yytext;
				BEGIN(LineComment);
				}

<LineComment>.*\n		{
				*inputFile += yytext;
				lineNr++;
				BEGIN(0);
				}

<CharConst,StrConst,Comment>\n  {
				*inputFile += '\n';
				lineNr++;
				}

<CharConst,StrConst,Comment,LineComment>.   {
				*inputFile += *yytext;
				}

"{"				{
				*inputFile += '{';
				obr++;
				}

"}"				{
				*inputFile += '}';
				cbr++;
				}

\r\n				{
				*inputFile += '\n';
				lineNr++;
				}

\n\r				{
				*inputFile += '\n';
				lineNr++;
				}

\r				{
				*inputFile += '\n';
				lineNr++;
				}

\n				{
				*inputFile += '\n';
				lineNr++;
				}

.				{
				*inputFile += *yytext;
				}

%%

/** Read files prior to extracting documentation.

   @param in The input text to document
   @param file The name of the file to read
   @param startLine The starting line number of the file being scanned
   @param directory The name of the directory to scan, default NULL
   @param scanSubDirs Whether to scan sub-directories
*/
void readfile(McString *in, const char *file, int startLine,
    const McString& directory, int scanSubDirs)
{
    McString path;    
    int i, n = 0;

    /* If we're told to scan sub-directories and there's a directory, recurse
       to get all header files in sub-directories
    */
    if(scanSubDirs && directory.length() > 0)
	{
	McString d = directory;
	McDArray<char *> list;
        McDirectory::scan(d, list, "*");
#ifdef DEBUG
	if(verb)
	    printf("Scanning for subdirs in `%s', found %d files\n",
		d.c_str(), list.size());
#endif
	for(i = 0; i < list.size(); i++)
	    {
	    McString tmp = d;
	    tmp += McDirectory::pathDelimiter();
	    tmp += list[i];
	    if(McDirectory::isDirectory(tmp))
		{
		readfile(in, file, 1, tmp, 1);
		n++;
		}
	    free(list[i]);
	    }
#ifdef DEBUG
	if(verb)
	    printf("done scanning for subdirs in `%s', found %d files\n",
		d.c_str(), n);
#endif
	}

    /* Set our path to be the directory unless the file starts with a path
       delimiter.
    */
    if(file[0] == McDirectory::pathDelimiter()) 
	path = file;
    else
	{
	if(directory.length())
	    {
	    path = directory.c_str();
	    if(path[path.length() - 1] != McDirectory::pathDelimiter())
		path += McDirectory::pathDelimiter();
	    }
	else
	    path.clear();
	    path += file; 
	}

    if(fileTable.has(path.c_str()))
	return;

    int slash = path.rindex(McDirectory::pathDelimiter());

    McString realFile;

    if(slash > -1)
	{
	McString dir(path, 0, slash + 1);
	currDir = dir;
	realFile = McString(path, slash + 1, path.length() - slash - 1);
	}
    else
	{
	currDir = "";
	realFile = path;
	}
    int d = realFile.index('*');
    if(d > -1)
	{
	McString dr = currDir;
	if(!dr.length())
	    {
	    dr = ".";
	    dr += McDirectory::pathDelimiter();
	    }	
	McDArray<char *> list;
	McDirectory::scan(dr, list, realFile.c_str());

#ifdef DEBUG
	if(verb) 
	printf("`%s': dir=`%s', %d files\n",
	    realFile.c_str(), dr.c_str(), list.size());
#endif
	for(i = 0; i < list.size(); i++)
	    {
	    readfile(in, list[i], 1, dr);
	    free(list[i]);
	    }
	return;
	}

    if(McDirectory::isDirectory(path))
	{
#ifdef DEBUG
	if(verb)
	    printf("`%s' is a directory, reading all entries\n", path.c_str());
#endif
	if(java)
	    readfile(in, "*.java", 1, path, 1);
	else
	    readfile(in, "*.h*", 1, realFile, 1);
	return;
	}

#ifdef DEBUG
    if(verb)
	printf("Opening `%s'\n", path.c_str());
#endif

    FILE *newin = fopen(path.c_str(), "r");
    if(newin)
	{
        yyin = newin;
        fileTable.add(0, path.c_str());
        inputFile = in;
        if(showFilePath)
	    startfile(path, startLine);
	else
	    startfile(file, startLine);
        yy_switch_to_buffer(yy_create_buffer(yyin, YY_BUF_SIZE));
	BEGIN(0);
	int lastobr = obr;
	int lastcbr = cbr;
	obr = 0;
	cbr = 0;
	readfilesYYlex();
	if(obr != cbr)
	    fprintf(stderr, "Warning: %d opening `{', but %d closing `}' in file `%s'.\n",
		obr, cbr, path.c_str());
	obr = lastobr;
	cbr = lastcbr;
	
	fclose(yyin);
	}
    else
	fprintf(stderr, _("Could not open `%s'\n"), path.c_str());
}
