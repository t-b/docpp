/*
  main.cc

  Copyright (c) 1996 Roland Wunderling, Malte Zoeckler
  Copyright (c) 1998 Michael Meeks
  Copyright (c) 1998-2002 Dragos Acostachioaie

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

#include <assert.h>
#include <fstream>
#include <getopt.h>
#include <locale.h>
#include <stdio.h>

#include "McDirectory.h"
#include "McString.h"
#include "doc.h"
#include "nametable.h"
#include "tex2gif.h"

#define	GIF_FILE_NAME	"gifs.db"

FILE *out;
char language;
Entry *root;
NameTable gifs;

extern void doHTML(const char *dir, Entry *root);

void help()
{
    printf(_("DOC++ %s, a documentation system for C, C++, IDL and Java\n"),
	DOCXX_VERSION);
    printf(_("Usage: doc++ [options] [files]\n\n"));

    printf(_("Options:\n"));
    printf(_("  -A  --all              document all\n"));
    printf(_("  -c  --c-comments       use C/C++ comments as doc++-comments\n"));
    printf(_("  -C  --config FILE      read options from FILE\n"));
    printf(_("  -h  --help             display this help and exit\n"));
    printf(_("  -H  --html             parse HTML syntax instead of TeX\n"));
    printf(_("  -I  --input FILE       read a list of input files from FILE\n"));
    printf(_("  -J  --java             parse Java instead of C/C++\n"));
    printf(_("  -nd --no-define        ignore `#define' macros\n"));
    printf(_("  -ng --no-class-graph   do not generate class graph\n"));
    printf(_("  -p  --private          document private members\n"));
    printf(_("  -q  --quick            optimize for speed instead of size\n"));
    printf(_("  -Q  --quantel          Quantel extensions, folding & appended comments\n"));
    printf(_("  -R  --internal-doc     generate internal documentation\n"));
    printf(_("  -u  --upwards-arrows   upwards arrows in class graphs\n"));
    printf(_("  -v  --verbose          turn verbose mode on\n"));
    printf(_("  -V  --version          output version information and exit\n"));
    printf(_("  -y  --scan-includes    scan `#include'ed header files\n"));
    printf(_("  -Y  --idl              parse IDL instead of C/C++\n"));
    printf(_("  -z  --php              parse PHP instead of C/C++\n\n"));

    printf(_("Alternate output types instead of HTML:\n"));
    printf(_("  -t  --tex              TeX output output\n"));
    printf(_("  -Z  --docbook          DocBook SGML output\n"));
    printf(_("  -L  --docbookxml       DocBook XML output\n"));

    printf(_("Additional options for HTML output:\n"));
    printf(_("  -a  --tables           use tables instead of description lists\n"));
    printf(_("  -b  --tables-border    use tables with borders\n"));
    printf(_("  -B  --footer FILE      use FILE as footer on HTML pages\n"));
    printf(_("  -d  --dir DIR          use DIR for the output directory\n"));
    printf(_("  -f  --filenames        show filenames in manual pages\n"));
    printf(_("  -F  --filenames-path   show filenames with path in manual pages\n"));
    printf(_("  -g  --no-gifs          do not generate GIFs for equations, etc.\n"));
    printf(_("  -G  --gifs             force generation of GIFs\n"));
    printf(_("  -i  --no-inherited     don't show inherited members\n"));
    printf(_("  -j  --no-java-graphs   suppress Java class graphs\n"));
    printf(_("  -k  --trivial-graphs   keep trivial class graphs\n"));
    printf(_("  -K  --stylesheet FILE  use FILE as HTML style sheet\n"));
    printf(_("  -m  --no-members       don't show all members in DOC section\n"));
    printf(_("  -M  --full-toc         show members in TOC\n"));
    printf(_("  -P  --no-general       discard general stuff\n"));
    printf(_("  -S  --sort             sort entries alphabetically\n"));
    printf(_("  -T  --header FILE      use FILE as header on HTML pages\n"));
    printf(_("  -w  --before-group     print group documentation before group\n"));
    printf(_("  -W  --before-class     print class documentation before group\n"));
    printf(_("  -x  --suffix SUFFIX    use SUFFIX instead of \".html\"\n\n"));

    printf(_("Additional options for TeX output:\n"));
    printf(_("  -ec --class-graph      only generates class graph\n"));
    printf(_("  -ef --env FILE         read TeX environment from FILE\n"));
    printf(_("  -ei --index            generate the index only\n"));
    printf(_("  -eo --style OPTION     setup TeX style OPTION\n"));
    printf(_("  -ep --package FILE     setup TeX to use package FILE\n"));
    printf(_("  -et --title FILE       use content of FILE as TeX title page\n"));
    printf(_("  -D  --depth DEPTH      set minimum depth (number of levels) in TOC\n"));
    printf(_("  -l  --no-env           do not generate TeX environment\n"));
    printf(_("  -o  --output FILE      set output file name\n"));
    printf(_("  -s  --source           generate source code listing\n"));
    printf(_("  -X  --hide-index       don't generate index at section start\n\n"));

    printf(_("Mail bug reports and suggestions to <docpp-users@lists.sourceforge.net>\n"));
}

void missingArg(char *s)
{
    fprintf(stderr, _("Ignoring option `%s': required argument missing\n"), s);
}

void missingArgE(char c)
{
    fprintf(stderr, _("Ignoring option `-e%c': required argument missing\n"), c);
}

void unknownOption(char c1, char c2 = 0)
{
    if(c2 == 0)
	fprintf(stderr, _("Unknown option `-%c'. Try `doc++ --help'\n"), c1);
    else
	fprintf(stderr, _("Unknown option `-%c%c'. Try `doc++ --help'\n"), c1, c2);
}

int main(int argc, char **argv)
{
    int i, c, depth, gifNum = 0;

    setlocale(LC_ALL, "");
#ifdef ENABLE_NLS
    bindtextdomain(PACKAGE, LOCALEDIR);
    textdomain(PACKAGE);
#endif

    root = new Entry;
    root->docify = true;
    McString& inputFile = root->program;

    out = stdout;
    language = LANG_CXX;

    static struct option long_options[] = {

	// general options
	{ "all", no_argument, 0, 'A' },
        { "c-comments", no_argument, 0, 'c' },
	{ "config", required_argument, 0, 'C' },
        { "help", no_argument, 0, 'h' },
        { "html", no_argument, 0, 'H' },
        { "java", no_argument, 0, 'J' },
        { "trivial-graphs", no_argument, 0, 'k' },
        { "no-defines", no_argument, 0, 'N' },
        { "no-class-graph", no_argument, 0, '7' },
        { "private", no_argument, 0, 'p' },
        { "quick", no_argument, 0, 'q' },
        { "quantel", no_argument, 0, 'Q' },
	{ "internal-doc", no_argument, 0, 'R' },
        { "tex", no_argument, 0, 't' },
        { "upwards-arrows", no_argument, 0, 'u' },
        { "verbose", no_argument, 0, 'v' },
        { "version", no_argument, 0, 'V' },
	{ "scan-includes", no_argument, 0, 'y' },
	{ "idl", no_argument, 0, 'Y' },
        { "php", no_argument, 0, 'z' },
	{ "docbook", no_argument, 0, 'Z' },
	{ "docbookxml", no_argument, 0, 'L' },

        // additional options for HTML output
        { "tables", no_argument, 0, 'a' },
        { "tables-border", no_argument, 0, 'b' },
        { "footer", required_argument, 0, 'B' },
        { "dir", required_argument, 0, 'd' },
        { "filenames", no_argument, 0, 'f' },
        { "filenames-path", no_argument, 0, 'F' },
        { "no-gifs", no_argument, 0, 'g' },
        { "gifs", no_argument, 0, 'G' },
        { "no-inherited", no_argument, 0, 'i' },
        { "no-java-graphs", no_argument, 0, 'j' },
        { "no-members", no_argument, 0, 'm' },
        { "full-toc", no_argument, 0, 'M' },
        { "no-general", no_argument, 0, 'P' },
        { "sort", no_argument, 0, 'S' },
        { "header", required_argument, 0, 'T' },
	{ "before-group", no_argument, 0, 'w' },
	{ "before-class", no_argument, 0, 'W' },
	{ "suffix", required_argument, 0, 'x' },
	{ "stylesheet", required_argument, 0, 'K' },

        // additional options for TeX output
        { "class_graph", no_argument, 0, '1' },
        { "env", required_argument, 0, '2' },
        { "index", no_argument, 0, '3' },
        { "style", required_argument, 0, '4' },
        { "package", required_argument, 0, '5' },
        { "title", required_argument, 0, '6' },
        { "depth", required_argument, 0, 'D' },
        { "no-env", no_argument, 0, 'l' },
        { "output", required_argument, 0, 'o' },
        { "source", no_argument, 0, 's' },
	{ "hide-index", no_argument, 0, 'X' },
        { 0, 0, 0, 0 }
    };

    opterr = 0;

    while((c = getopt_long(argc, argv, "+AcC:hHI:Jkn:pqQRtuvVyYZLabB:d:fFgGijmMPST:wWx:K:e:D:lo:sX",
	long_options, (int *)0)) != EOF)
	switch(c)
	    {
	    case 'A':
		onlyDocs = false;
		break;
	    case 'c':
		commentExtn = true;
		break;
	    case 'C':
		if(optarg[0] == '-')
		    {
		    missingArg("--config");
		    break;
		    }
		configFile = optarg;
		break;
            case 'h':
		help();
		exit(0);
	    case 'H':
		HTMLsyntax = true;
		break;
	    case 'I':
		if(optarg[0] == '-')
		    {
		    missingArg("--input");
		    break;
		    }
		fileList = optarg;
		break;
	    case 'J':
		java = true;
		language = LANG_JAVA;
		break;
    	    case 'k':
    		trivialGraphs = true;
		break;
	    case 'L':
		doDOCBOOKXML = true;
		break;
	    case 'N':
		noDefines = true;
		break;
	    case '7':
    		noClassGraph = true;
		break;
	    case 'n':
		switch(optarg[0])
		    {
		    case 'd':
			noDefines = true;
			break;
		    case 'g':
			noClassGraph = true;
			break;
		    default:
			unknownOption('n', optarg[0]);
			exit(-1);
		    }
		break;
	    case 'p':
    		withPrivate = true;
		break;
	    case 'q':
    		fastNotSmall = true;
		break;
	    case 'Q':
    		QuantelExtn = true;
		break;
	    case 'R':
		internalDoc = true;
		break;
    	    case 't':
    		doTeX = true;
		break;
	    case 'u':
    		upArrows = true;
		break;
	    case 'v':
    		verb = true;
		break;
	    case 'V':
    		printf("DOC++ %s\n", DOCXX_VERSION);
		exit(0);
	    case 'y':
		scanIncludes = true;
		break;
	    case 'Y':
		withPrivate = true;	// IDL provides no access control
		language = LANG_IDL;
		break;
	    case 'z':
		php = true;
		language = LANG_PHP;
		break;
	    case 'Z':
		doDOCBOOK = true;
		break;
	    case 'a':
    		withTables = true;
		break;
	    case 'b':
    		withTables = true;
		withBorders = true;
		break;
	    case 'B':
		if(optarg[0] == '-')
		    {
		    missingArg("--footer");
		    break;
		    }
    		ownFooter = optarg;
		break;
	    case 'd':
		if(optarg[0] == '-')
		    {
		    missingArg("--dir");
		    break;
		    }
    		outputDir = optarg;
		break;
	    case 'f':
    		showFilenames = true;
		break;
	    case 'F':
    		showFilenames = true;
		showFilePath = true;
		break;
	    case 'g':
    		noGifs = true;
		break;
	    case 'G':
    		forceGifs = true;
		break;
	    case 'i':
    		showInherited = false;
		break;
	    case 'j':
    		javaGraphs = false;
		break;
	    case 'm':
    		alwaysPrintDocSection = false;
		break;
	    case 'M':
    		showMembersInTOC = true;
		break;
            case 'P':
    		useGeneral = false;
		break;
	    case 'S':
    		sortEntries = true;
		break;
	    case 'T':
		if(optarg[0] == '-')
		    {
		    missingArg("--header");
		    break;
		    }
    		ownHeader = optarg;
		break;
	    case 'w':
		printGroupDocBeforeGroup = true;
		break;
	    case 'W':
		printClassDocBeforeGroup = true;
		break;
	    case 'x':
		if(optarg[0] == '-')
		    {
		    missingArg("--suffix");
		    break;
		    }
		htmlSuffix = optarg;
		break;
	    case 'K':
		if(optarg[0] == '-')
		    {
		    missingArg("--stylesheet");
		    break;
		    }
		htmlStyleSheet = optarg;
		break;
	    case '1':
    		onlyClassGraph = true;
		break;
	    case '2':
		if(optarg[0] == '-')
		    {
		    missingArg("--env");
		    break;
		    }
    		texFile = optarg;
		break;
	    case '3':
    		generateIndex = true;
		break;
	    case '4':
		if(optarg[0] == '-')
		    {
		    missingArg("--style");
		    break;
		    }
    		texOption = optarg;
		break;
	    case '5':
		if(optarg[0] == '-')
		    {
		    missingArg("--package");
		    break;
		    }
		texPackages.append(new McString(optarg));
		break;
	    case '6':
		if(optarg[0] == '-')
		    {
		    missingArg("--title");
		    break;
		    }
    		texTitle = optarg;
		break;
	    case 'X':
		hideIndexOnEverySection = true;
		break;
	    case 'D':
		if(optarg[0] == '-')
		    {
		    missingArg("--depth");
		    break;
		    }
    		if(!sscanf(optarg, "%d", &depth))
	    	    fprintf(stderr, _("Ignoring option `--depth': bad argument specified\n"));
		else
		    depthTOC = depth;
		break;
	    case 'l':
    		noLatex = true;
		break;
	    case 'o':
		if(optarg[0] == '-')
		    {
		    missingArg("--output");
		    break;
		    }
		FILE *tmp;
		if((tmp = fopen(optarg, "w")))
		    {
	            texOutputName = optarg;
		    out = tmp;
		    }
		else
		    fprintf(stderr, _("Ignoring option `--output': cannot open file `%s'\n"),
			optarg);
		break;
	    case 's':
    		doListing = true;
		break;
	    case 'e':
    		switch(optarg[0])
	    	    {
		    case 'c':
        		onlyClassGraph = true;
			break;
		    case 'f':
			if(argv[optind][0] == '-')
	            	    {
			    missingArgE('f');
			    break;
			    }
    			texFile = argv[optind++];
			break;
		    case 'i':
        		generateIndex = true;
			break;
		    case 'o':
			if(argv[optind][0] == '-')
			    {
			    missingArgE('o');
			    break;
			    }
    			texOption = argv[optind++];
			break;
		    case 'p':
			if(argv[optind][0] == '-')
			    {
			    missingArgE('p');
			    break;
			    }
			texPackages.append(new McString(argv[optind++]));
			break;
		    case 't':
			if(argv[optind][0] == '-')
			    {
			    missingArgE('t');
			    break;
			    }
        		texTitle = argv[optind++];
			break;
		    default:
			unknownOption('e', optarg[0]);
			exit(-1);
		    }
    		break;
	    case '?':
		unknownOption(optopt);
    		exit(-1);
            }

    // read configuration file (if possible)
    if(configFile.length() > 0)
	{
#ifdef WIN32
    std::ifstream f(configFile.c_str(), ios::nocreate|ios::in);
#else
    std::ifstream f(configFile.c_str());
#endif
	if(!f)
	    {
	    if(configFile != "doc++.conf")
		fprintf(stderr, _("Error opening configuration file `%s'\n"),
		    configFile.c_str());
	    }
	else
	    {
	    McString s;
	    while(f)
		s += f.get();
	    f.close();
	    parseConfig(s);
	    if(java)
		language = LANG_JAVA;
	    if(idl)
		language = LANG_IDL;
	    if(php)
		language = LANG_PHP;
	    }
	}

    if(optind >= argc && fileList.length() == 0 && inputFiles.size() == 0)
	{
	fprintf(stderr, _("No input files. Try `doc++ --help'\n"));
	exit(-1);
	}

    // Preload GIFs
    if(!noGifs && !forceGifs && !doTeX)
	{
	McString gifDB(outputDir);
	gifDB += PATH_DELIMITER;
	gifDB += GIF_FILE_NAME;

  std::ifstream gifFile(gifDB.c_str());
	if(gifFile)
	    {
            gifFile >> gifs;
            gifNum = gifs.num();
    	    }
	}

    // Read input files into buffer
    if(verb)
	printf(_("Reading files...\n"));

    if(fileList.length() > 0)
	{
    std::ifstream i_file(fileList.c_str());
	if(!i_file)
	    {
            fprintf(stderr, _("Error opening file list `%s'\n"), fileList.c_str());
            exit(-1);
            }

	char line[1024];
	while(i_file)
    	    {
            i_file.getline(line, 1024);
	    if(i_file)
		readfile(&inputFile, line, 1);
            }
	i_file.close();
	}
    else
	if(inputFiles.size() == 0)
	    for(i = optind; i < argc; i++)
		readfile(&inputFile, argv[i], 1);
	else
	    for(i = 0; i < inputFiles.size(); i++)
		readfile(&inputFile, inputFiles[i]->c_str(), 1);
    if(verb)
	printf(_("%d bytes read\n"), inputFile.length());

    // Do TeX source code listing if that is what the user want
    if(doTeX && doListing)
	{
	if(verb)
    	    printf(_("Generating source code listing...\n"));
	listing(inputFile);
	if(verb)
	    printf(_("Done.\n"));
	exit(0);
	}

    // Parse buffer
    if(verb)
	printf(_("Parsing...\n"));
    if(language == LANG_JAVA)
	parseJava(root);
    else
	if(language == LANG_PHP)
	    parsePHP(root);
	else
	    parseCpp(root);

    checkPackages(root);
    setupLanguageHash();

    // Merge duplicated entries, etc.
    if(verb)
	printf(_("Merging duplicate entries...\n"));
    mergeEntries(root);

    // Sort entries
    if(verb)
	printf(_("Sorting entries...\n"));
    makeSubLists(root);

    // Resolve references
    if(verb)
	printf(_("Resolving references...\n"));
    reNumber(root);
    root->makeRefs();

    // Create user manual
    if(doTeX)
	usermanTeX(inputFile, root);
    else
	if(doDOCBOOK)
	    usermanDBsgml(inputFile, root);
        else
	    if(doDOCBOOKXML)
		usermanDBxml(inputFile, root);
	    else
		doHTML(outputDir.c_str(), root);

    // Create GIFs (if any)
    gifNum = gifs.num();
    if(gifNum > 0)
	{
	McString gifDB(outputDir);
	gifDB += PATH_DELIMITER;
	gifDB += GIF_FILE_NAME;
	if(verb)
	    printf(_("Writing GIF database file `%s'...\n"), gifDB.c_str());

  std::ofstream gifFile(gifDB.c_str());
	gifFile << gifs;
	makeGifs(outputDir.c_str(), GIF_FILE_NAME);
	}

    // That's all
    if(verb)
	printf(_("Done.\n"));

    return 0;
}
