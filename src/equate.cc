/*
  equate.cc

  Copyright (c) 1999 Paul Schulz
  Copyright (c) 1999-2002 Dragos Acostachioaie

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

#include "config.h"

#include <fstream>
#include <getopt.h>
#include <iostream>
#include <stdio.h>
#include <string.h>

#ifdef __BORLANDC__
#include <dir.h>
#elif defined(__VISUALC__) || defined(__WATCOMC__)
#include <direct.h>
#else
#include <unistd.h>
#endif

#include "McString.h"
#include "doc.h"
#include "tex2gif.h"

#define EQUATE_VERSION	"0.1"
#define	GIF_FILE_NAME	"gifs.db"

void help()
{
    printf(_("equate %s, generate GIFs of equations from GIF database file\n"),
	EQUATE_VERSION);
    printf(_("Usage: equate [options] [gifdb_file]\n\n"));
    printf(_("Options:\n"));
    printf(_("  -d  --dir DIR          use DIR for the output directory\n"));
    printf(_("  -f  --file FILE        read FILE instead of gif.db as GIF database\n"));
    printf(_("                         (relative to . or DIR)\n\n"));
    printf(_("Mail bug reports and suggestions to <docpp-users@lists.sourceforge.net>\n"));
}

void missingArg(char *s)
{
    fprintf(stderr, _("Ignoring option `%s': required argument missing\n"), s);
}

FILE *out;

main(int argc, char** argv)
{       
    McString dir = ".";
    McString gifdb = "gifs.db";
    int c;

    static struct option long_options[] = {
	{ "dir", required_argument, 0, 'd' },
	{ "file", required_argument, 0, 'f' },
	{ "help", no_argument, 0, 'h' },
	{ "version", no_argument, 0, 'V' },
	{ 0, 0, 0, 0 }
    };

    opterr = 0;

    while((c = getopt_long(argc, argv, "+:d:fhV", long_options, (int *)0)) != EOF)
	switch(c)
	    {
	    case 'd':
		if(optarg[0] == '-')
		    {
		    missingArg("--dir");
		    break;
		    }
		dir = optarg;
		break;
	    case 'f':
		if(optarg[0] == '-')
		    {
		    missingArg("--file");
		    break;
		    }
		gifdb = optarg;
		break;
	    case 'h':
		help();
		exit(0);
	    case 'V':
		printf("equate %s\n", EQUATE_VERSION);
		exit(0);
	    }

    makeGifs(dir, gifdb);
}
