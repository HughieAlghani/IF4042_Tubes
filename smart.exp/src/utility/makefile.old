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

LDLIBS =

install all: $(BIN)/make_trie $(BIN)/news_filter
	touch install all

$(BIN)/make_trie: make_trie.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(LDFLAGS) $(TARGET_ARCH) \
        -o make_trie make_trie.c $(LDLIBS)
	if [ -f $(BIN)/$$f ]; then /bin/mv $@ $@.old; fi;
	/bin/mv make_trie $@

$(BIN)/news_filter: news_filter.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(LDFLAGS) $(TARGET_ARCH) \
        -o news_filter news_filter.c $(LDLIBS)
	if [ -f $(BIN)/$$f ]; then /bin/mv $@ $@.old; fi;
	/bin/mv news_filter $@

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
