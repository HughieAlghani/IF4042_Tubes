##1##########################################################################
#  The first section here is automatically generated. Do not change here; 
#  instead change ...smart/install/makebin.templ
#############################################################################

SRCDIR = /home/smart.exp/src
INSTALLBIN = /home/smart.exp/src/bin
INDEXED = /home/smart.exp/indexed_colls
TLIBDIR = /home/smart.exp/lib
TMPDIR = /tmp

# Flags normally set in top level makefile.  Make sure you want these
# defaults if you compile locally in this directory
PREFIX = /home/smart.exp/src
BIN = $(PREFIX)/bin
LIB = $(PREFIX)/lib
H   = -I$(SRCDIR)/h

# Static libraries with gcc
CC       = gcc
CFLAGS   = $H -L$(LIB) -g -static

# Static libraries with cc
#cc CC       = cc
#cc CFLAGS   = $H -L$(LIB) -g -Bstatic

# Other macros used in some or all makefiles
INSTALL = /bin/mv

#############################################################################
#  The second section starting here is directory-dependent instructions
##2##########################################################################
##3##########################################################################

SCRIPTS = $(BIN)/baserun $(BIN)/docsmart $(BIN)/fdbkrun $(BIN)/ismart \
          $(BIN)/make_docsmart $(BIN)/smprint

install all: $(SCRIPTS)
	touch install all

$(BIN)/smprint:  smprint
	sed -e '/INDEXED/s?INDEXED?$(INDEXED)?' \
            -e '/TLIBDIR/s?TLIBDIR?$(TLIBDIR)?' \
            -e '/BIN/s?BIN?$(BIN)?' \
            -e '/INSTALLBIN/s?INSTALLBIN?$(INSTALLBIN)?' \
            -e '/TMPDIR/s?TMPDIR?$(TMPDIR)?' \
            -e '/SRCDIR/s?SRCDIR?$(SRCDIR)?' \
            smprint > $(BIN)/smprint
	chmod 0775 $(BIN)/smprint
$(BIN)/baserun:  baserun
	sed -e '/INDEXED/s?INDEXED?$(INDEXED)?' \
            -e '/TLIBDIR/s?TLIBDIR?$(TLIBDIR)?' \
            -e '/BIN/s?BIN?$(BIN)?' \
            -e '/INSTALLBIN/s?INSTALLBIN?$(INSTALLBIN)?' \
            -e '/TMPDIR/s?TMPDIR?$(TMPDIR)?' \
            -e '/SRCDIR/s?SRCDIR?$(SRCDIR)?' \
            baserun > $(BIN)/baserun
	chmod 0775 $(BIN)/baserun
$(BIN)/fdbkrun:  fdbkrun
	sed -e '/INDEXED/s?INDEXED?$(INDEXED)?' \
            -e '/TLIBDIR/s?TLIBDIR?$(TLIBDIR)?' \
            -e '/BIN/s?BIN?$(BIN)?' \
            -e '/INSTALLBIN/s?INSTALLBIN?$(INSTALLBIN)?' \
            -e '/TMPDIR/s?TMPDIR?$(TMPDIR)?' \
            -e '/SRCDIR/s?SRCDIR?$(SRCDIR)?' \
            fdbkrun > $(BIN)/fdbkrun
	chmod 0775 $(BIN)/fdbkrun
$(BIN)/ismart:  ismart
	sed -e '/INDEXED/s?INDEXED?$(INDEXED)?' \
            -e '/TLIBDIR/s?TLIBDIR?$(TLIBDIR)?' \
            -e '/BIN/s?BIN?$(BIN)?' \
            -e '/INSTALLBIN/s?INSTALLBIN?$(INSTALLBIN)?' \
            -e '/TMPDIR/s?TMPDIR?$(TMPDIR)?' \
            -e '/SRCDIR/s?SRCDIR?$(SRCDIR)?' \
            ismart > $(BIN)/ismart
	chmod 0775 $(BIN)/ismart
$(BIN)/make_docsmart:  make_docsmart
	sed -e '/INDEXED/s?INDEXED?$(INDEXED)?' \
            -e '/TLIBDIR/s?TLIBDIR?$(TLIBDIR)?' \
            -e '/BIN/s?BIN?$(BIN)?' \
            -e '/INSTALLBIN/s?INSTALLBIN?$(INSTALLBIN)?' \
            -e '/TMPDIR/s?TMPDIR?$(TMPDIR)?' \
            -e '/SRCDIR/s?SRCDIR?$(SRCDIR)?g' \
            make_docsmart > $(BIN)/make_docsmart
	chmod 0775 $(BIN)/make_docsmart
$(BIN)/docsmart: docsmart
	sed -e '/INDEXED/s?INDEXED?$(INDEXED)?' \
            -e '/TLIBDIR/s?TLIBDIR?$(TLIBDIR)?' \
            -e '/BIN/s?BIN?$(BIN)?' \
            -e '/INSTALLBIN/s?INSTALLBIN?$(INSTALLBIN)?' \
            -e '/TMPDIR/s?TMPDIR?$(TMPDIR)?' \
            -e '/SRCDIR/s?SRCDIR?$(SRCDIR)?' \
            docsmart > $(BIN)/docsmart
	chmod 0775 $(BIN)/docsmart

##4##########################################################################
##5##########################################################################
#  All code below this line (except for automatically created dependencies)
#  is independent of this particular makefile, and should not be changed!
#############################################################################

#########################################################################
# Odds and ends                                                         #
#########################################################################
clean semiclean:
	/bin/rm -f *.o *.BAK install all *~ makefile.old

makelint:

lint: 
	for f in *.c ;\
                do echo $$f; $(LINT) -I../h $$f $(LIB)/llib* -lm;\
        done

bfly: makefile.bfly
	gnumake -k -f makefile.bfly "LDLIBS=$(LDLIBS) -lus"

makefile.bfly: makefile ../makebin.bfly.templ
	awk '/All code below this line/ {exit} {print}' \
                    < makefile > makefile.bfly.new
	awk '/START BFLY STUFF/, /END BFLY STUFF/ {print}' \
                    < makefile.bfly >> makefile.bfly.new
	cat ../makebin.bfly.templ >> makefile.bfly.new
	mv makefile.bfly makefile.bfly.old
	mv makefile.bfly.new makefile.bfly

#########################################################################
# Determining program dependencies                                      #
#########################################################################
depend:
	grep '^#[ ]*include' *.c \
		| sed -e 's?:[^"]*"\([^"]*\)".*?: \$H/\1?' \
			-e '/</d' \
			-e '/functions.h/d' \
		        -e 's/\.c/.o/' \
		        -e 's/\.y/.o/' \
		        -e 's/\.l/.o/' \
		> makedep
	echo '/^# DO NOT DELETE THIS LINE/+2,$$d' >eddep
	echo '$$r makedep' >>eddep
	echo 'w' >>eddep
	cp makefile makefile.bak
	ed - makefile < eddep
	/bin/rm eddep makedep
	echo '# DEPENDENCIES MUST END AT END OF FILE' >> makefile
	echo '# IF YOU PUT STUFF HERE IT WILL GO AWAY' >> makefile
	echo '# see make depend above' >> makefile

# DO NOT DELETE THIS LINE -- make depend uses it


# DEPENDENCIES MUST END AT END OF FILE
# IF YOU PUT STUFF HERE IT WILL GO AWAY
# see make depend 
##6##########################################################################
