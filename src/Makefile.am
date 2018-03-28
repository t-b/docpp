# src/Makefile.am for DOC++
# Copyright (c) 1998-2001 Dragos Acostachioaie

localedir     = $(datadir)/locale
INCLUDES      = -I../intl -DLOCALEDIR=\"$(localedir)\"

# The `equate' program is still experimental.
bin_PROGRAMS = doc++ docify promote #equate
doc___SOURCES = Entry.cc McDirectory.cc McHashTable.cc McWildMatch.cc \
		classgraph.cc comment.ll config.ll cpp.ll doc.ll doc2dbxml.ll\
		doc2dbsgml.ll doc2html.ll doc2tex.ll doc2xml.ll getopt.c \
		getopt1.c html.cc java.ll main.cc nametable.cc php.ll \
		readfiles.ll tex2gif.cc McDArray.h McDirectory.h McHandable.h \
		McHashTable.h McSorter.h McString.h McWildMatch.h classgraph.h\
		datahashtable.h doc.h gifs.h java.h nametable.h portability.h \
		tex2gif.h

doc___LDADD = @INTLLIBS@
docify_SOURCES = docify.ll
promote_SOURCES = promote.ll

FLEXSRC	= comment.cc config.cc cpp.cc doc.cc doc2dbxml.cc doc2dbsgml.cc \
	  doc2html.cc doc2tex.cc doc2xml.cc java.cc readfiles.cc docify.cc \
	  php.cc promote.cc

# At the moment we require that $(LEX) accept a -P flag (which seems to be
# a GNU extension; certainly it isn't in Solaris lex).
# The way we could do without this requirement is to do `#define yyin commentYYin'
# etc. for all used symbols.  (This is in fact what flex does.)
LEX_OUTPUT_ROOT = lex.$*YY
AM_LFLAGS = -P$*YY

clean-local:
	rm -f $(FLEXSRC)

EXTRA_DIST = ClassGraph.java config.h.w32 makefile.w32 unistd.h.w32 equate.cc

# This isn't currently part of the build process; see comment at head
# of ClassGraph.java.
ClassGraph.class: ClassGraph.java
	javac -classpath $(srcdir): -O ClassGraph.java
