/*
  tex2gif.cc

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

#include <iostream>
#include <fstream.h>
#include <stdio.h>
#include <string.h>

#ifdef __BORLANDC__
#include <dir.h>
#elif defined(__VISUALC__) || defined(__WATCOMC__)
#include <direct.h>
#else
#include <unistd.h>
#endif

#include "doc.h"
#include "nametable.h"
#include "tex2gif.h"

void _system(const char *b)
{
#ifdef DEBUG
    printf(_("Running `%s'\n"), b);
#endif
    system(b);
}

void makeGifs(const char *dir, const char *gifdb)
{       
    NameTable gifs;
    McDArray<int> tog;		// Table of GIFs

    int i, gifNum, x1, x2, y1, y2;
    char buf[200];
    char buf2[200];
    char *olddir;
    char c;
    bool ok = true;

    if(dir)
	{
	olddir = getcwd(NULL, 200);
	chdir(dir);
	}

#ifdef DEBUG
    printf(_("Processing file `%s'\n"), gifdb);
#endif

    ifstream gifFile((const char *)gifdb);
    if(gifFile)
	{
	gifFile >> gifs;
	gifNum = gifs.num();
	}

#ifdef DEBUG
    printf(_("Read %d GIF record(s).\n"), gifNum);
#endif

    FILE *texfile = fopen("dxxgifs.tex", "w");
    if(!texfile)
	{
	fprintf(stderr, _("Can't open `%s/dxxgifs.tex' for writing\n"), dir);
	return;
	}

    if(texFile.length() > 0)
	{
	ifstream env(texFile.c_str());
	if(env)
	    while(env)
		{
		env.get(c);
		putc(c, texfile);
		}
	else
	    fprintf(stderr, _("Couldn't open `%s'\n"), texFile.c_str());
	}
    else
	{
	fprintf(texfile, "\\documentclass");
	if(texOption.length() > 0)
	    fprintf(texfile, "[%s]", texOption.c_str());
	fprintf(texfile, "{article}\n");

	for(i = 0; i < texPackages.size(); i++)
	    fprintf(texfile, "\\usepackage{%s}\n", texPackages[i]->c_str());
	}
    fprintf(texfile, "\\pagestyle{empty}\n");
    fprintf(texfile, "\\begin{document}\n");

    if(texTitle.length() > 0)
	{
	ifstream title(texTitle.c_str());
	if(title)
	    while(title)
		{
		title.get(c);
		putc(c, texfile);
		}
	else
	    fprintf(stderr, _("Could not open `%s'\n"), texTitle.c_str());
	}

    for(gifs.first(); gifs.current(); gifs.next())
	{
	int n = gifs[gifs.current()];
	sprintf(buf, "g%06d.gif", n);
	FILE *exist = fopen(buf, "r");
	if(!exist || forceGifs)
	    {
	    out = texfile;
	    printYYDOC(0, gifs.current());
	    fprintf(texfile, "\n\n\n\\pagebreak\n\n\n");
	    tog.append(n);
	    }
	if(exist)
	    fclose(exist);
	}
    fprintf(texfile, "\\end{document}\n");
    fclose(texfile);
    
    if((gifNum = tog.size()))
	_system("latex dxxgifs.tex");

    for(i = 0; i < gifNum; i++)
	{
	sprintf(buf, "dvips -D 600 -E -n 1 -p %d -o dxx%04d.eps dxxgifs.dvi",
    	    i + 1, i);
	_system(buf);
	}

    for(i = 0, gifs.first(); i < gifNum; i++, gifs.next())
	{
	int n = gifs[gifs.current()];
	sprintf(buf, "dxx%04d.eps", i);
	FILE *in = fopen(buf, "r");
	if(in)
	    {
	    int num = 0;
	    while(!feof(in))
		{
		fgets(buf2, 200, in);
		if(strncmp("%%BoundingBox:", buf2, 14) == 0)
		    {
		    num = sscanf(buf2, "%%%%BoundingBox:%d %d %d %d",
			&x1, &y1, &x2, &y2);
		    break;
	            }
		}
	    if(num != 4)
		fprintf(stderr, _("Couldn't extract BoundingBox "
		    "from dxx%04d.eps.\n"), i);
	    fclose(in);

	    FILE *psfile = fopen("dxxps.ps", "w");

	    fprintf(psfile,
		".7 .7 .7 setrgbcolor newpath -1 -1 moveto %d -1 lineto %d %d "
		"lineto -1 %d lineto closepath fill \n"
		"-%d -%d translate "
		"0 0 0 setrgbcolor \n (dxx%04d.eps) run",
		x2 - x1 + 2, x2 - x1 + 2, y2 - y1 + 2, y2 - y1 + 2, x1, y1, i);

	    fclose(psfile);
	    float resfac = 4;

	    int gx = (int)((x2 - x1) * resfac);
	    int gy = (int)((y2 - y1) * resfac);

	    gx = ((gx + 7) / 8) * 8;

	    sprintf(buf, "gs -g%dx%d -r%dx%d -sDEVICE=ppmraw -sOutputFile=dxxtmp.pnm -DNOPAUSE -- dxxps.ps",
		gx, gy, (int)(resfac * 72), (int)(resfac * 72));
	    _system(buf);

	    sprintf(buf, "pnmscale -xscale .333 -yscale .333 dxxtmp.pnm | "
		"pnmgamma .9 | ppmquant 256 |"
		"ppmtogif -transparent rgb:ac/ac/ac > g%06d.gif", n);
	    _system(buf);

	    sprintf(buf, "dxx%04d.eps", i);
	    unlink(buf);
	    }
	else
	    {
	    ok = false;
	    fprintf(stderr, _("Warning: problems generating GIFs. Check file `dxxgifs.tex'.\n"));
	    break;
            }
	}

    if(ok)
	{
	unlink("dxxgifs.aux");
	unlink("dxxgifs.dvi");
	unlink("dxxgifs.log");
	unlink("dxxps.ps");
	unlink("dxxtmp.pnm");
	unlink("dxxgifs.tex");
	}

    if(olddir)
	{
	chdir(olddir);
	free(olddir);
	}
}
