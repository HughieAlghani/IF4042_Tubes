# Install makefile.  The top part of this file should be changed to
# represent the local conditions.  The include file src/h/param.h
# may need to be changed; in particular the MMAP facility is very useful,
# but is not compatibile on all systems.  The include file
# src/h/sysfunc.h may need to be editted to reflect your local system.
# It gives return value types for all system functions called by smart.
# Functions may be changed directly there, or you may wish to just include
# the appropriate system include files.

# After making all above changes, type "make install" in this 
# directory to install and test smart.  "make update" is a useful
# entrypoint if you have updated SMART by importing new versions of
# makefiles or scripts.

# Pathnames

# Note: none of these pathnames will occur within the SMART executable.
# The pathnames are used by shell-scripts (see src/scripts) and by
# makefiles to construct smart itself.
SMART   = /home/smart.exp

# SRCDIR gives the directory for the smart source code
SRCDIR  = $(SMART)/src

# TLIBDIR gives the directory for the smart text libraries (files here give
# things like default specifications and lists of common stopwords)
TLIBDIR  = $(SMART)/lib

# TMPDIR gives a directory for putting temporary files (/tmp is normally
# fine, depending on what you want to do)
TMPDIR  = /tmp

# PREFIX gives the location for the compiled object code, binaries, scripts
# (object libraries will appear in PREFIX/lib, executatables in PREFIX/bin)
PREFIX  = $(SRCDIR)

# INSTALLBIN gives the directory in which to install the top-level shell 
# script that a user (as opposed to database administrator) will invoke.
# INSTALLBIN = /usr/local/bin
INSTALLBIN = $(SRCDIR)/bin

# INDEXED gives the default location for indexed versions of collections
INDEXED = $(SMART)/indexed_colls


# Compiler options

# Set COMPILER to be either 'gcc' or 'cc' in order to un-comment the
# appropriate compiler flags in makefile.dist, makebin.templ, and 
# makelib.templ.  You may change other options in those files directly.  
# If you are using a different compiler, set COMPILER to 'unknown'
COMPILER = gcc

# If your version of make understands Sun style library specifications, eg
#       libname.a: libname.a(a.o b.o c.o)
# then set LIBSTYLE to 'libsun', else set it to 'libnotsun'
LIBSTYLE = libnotsun

# If you will be adding substantial additional functionality to smart,
# set CHANGE to 'change', else set it to 'nochange'.  'change' will
# create an entire duplicate hierarchy under src/liblocal that all local
# additions should go in.  (Research users should use 'change', others
# will be happy with 'nochange').  
# For version 11.0 only, all users want "change"
CHANGE  = change


# For standard SMART, nothing beyond this point should need changing.
##############################################################################
install:
	-mv $(SRCDIR)/makefile $(SRCDIR)/makefile.old
	sed -e '/#srcdir/s?.*?SRCDIR = $(SRCDIR)?' \
            -e '/#prefix/s?.*?PREFIX = $(PREFIX)?' \
            -e '/#$(COMPILER) /s?#$(COMPILER) ??' \
            -e '/#$(CHANGE) /s?#$(CHANGE) ??' \
            -e '/#$(LIBSTYLE) /s?#$(LIBSTYLE) ??' \
            makefile.top > $(SRCDIR)/makefile
	-mv $(SRCDIR)/makebin.templ $(SRCDIR)/makebin.old
	sed -e '/#srcdir/s?.*?SRCDIR = $(SRCDIR)?' \
            -e '/#prefix/s?.*?PREFIX = $(PREFIX)?' \
            -e '/#installbin/s?.*?INSTALLBIN = $(INSTALLBIN)?' \
            -e '/#indexed/s?.*?INDEXED = $(INDEXED)?' \
            -e '/#tlibdir/s?.*?TLIBDIR = $(TLIBDIR)?' \
            -e '/#tmpdir/s?.*?TMPDIR = $(TMPDIR)?' \
            -e '/#$(COMPILER) /s?#$(COMPILER) ??' \
            makebin.templ > $(SRCDIR)/makebin.templ
	-mv $(SRCDIR)/makelib.templ $(SRCDIR)/makelib.old
	sed -e '/#srcdir/s?.*?SRCDIR = $(SRCDIR)?' \
            -e '/#prefix/s?.*?PREFIX = $(PREFIX)?' \
            -e '/#$(COMPILER) /s?#$(COMPILER) ??' \
            -e '/#$(LIBSTYLE) /s?#$(LIBSTYLE) ??' \
            makelib.templ > $(SRCDIR)/makelib.templ
	-mkdir $(TLIBDIR)
	-mv $(TLIBDIR)/spec.default $(TLIBDIR)/spec.default.d
	sed -e '/TLIBDIR/s?TLIBDIR?$(TLIBDIR)?' \
            spec.default > $(TLIBDIR)/spec.default
	-mv $(TLIBDIR)/spec.expcoll $(TLIBDIR)/spec.expcoll.d
	sed -e '/TLIBDIR/s?TLIBDIR?$(TLIBDIR)?' \
            spec.expcoll > $(TLIBDIR)/spec.expcoll
	-mv $(TLIBDIR)/common_words $(TLIBDIR)/common_words.d
	cp common_words $(TLIBDIR)/common_words
	if [ ! -d $(SRCDIR)/liblocal -a $(CHANGE) = "change" ]; then\
                 cp -ri ./liblocalhier $(SRCDIR)/liblocal; fi;
	if [ ! -d $(SRCDIR)/liblocal -a $(CHANGE) = "nochange" ]; then\
                 cp -ri ./liblocaldir $(SRCDIR)/liblocal; fi;
	-mv $(SRCDIR)/test_adi/make_adi $(SRCDIR)/test_adi/make_adi.d
	sed -e '/SRCDIR/s?SRCDIR?$(SRCDIR)?' \
            -e '/BIN/s?BIN?$(PREFIX)/bin?' \
            -e '/TLIBDIR/s?TLIBDIR?$(TLIBDIR)?' \
            make_adi > $(SRCDIR)/test_adi/make_adi
	chmod 0755 $(SRCDIR)/test_adi/make_adi
	-mv $(SRCDIR)/test_adi/test_adi $(SRCDIR)/test_adi/test_adi.d
	sed -e '/BIN/s?BIN?$(PREFIX)/bin?' \
            test_adi > $(SRCDIR)/test_adi/test_adi
	chmod 0755 $(SRCDIR)/test_adi/test_adi

	(cd $(SRCDIR); make makefiles default test_suite)

