#               LIBRARY MAKEFILE
########################################################

# Module name
LIBNAME = libindexing

# Files for this particular library module
PREPARSE = pp_preparse.o pp_parts.o pp_generic.o
NEXTID   = next_vecid_1.o next_vecid.o
ADDTEXTLOC = add_textloc.o
TOKEN    = token_sect.o
PARSE    = parse_full.o parse_no.o parse_asis.o parse_name.o \
           parse_adj.o parse_fulla.o parse_phrase.o
STEM     = triestem.o trie_suff.o trie_recode.o remove_s.o stem_no.o \
           stem_phrase.o
STOP     = stop_dict.o stop_no.o
TOKCON   = tokcon_dict.o tokcon_noss.o tokcon_cont.o
MAKEVEC  = makevec.o 
EXPAND   = expand_no.o
OUTPUT   = store_vec.o store_vaux.o store_aux.o
STORE    = vec_collstat.o
TOP      = index_coll.o index_exp.o add_parts.o parts_coll.o
INDEX    = index_vec.o index_pp.o
UTIL     = parse_util.o

LIBFILES  = $(PREPARSE) $(NEXTID) $(ADDTEXTLOC) $(TOKEN) $(PARSE) \
            $(STEM) $(STOP) $(TOKCON) $(EXPAND) $(MAKEVEC) \
            $(WEIGHT) $(OUTPUT) $(STORE) \
            $(TOP) $(INDEX)  $(UTIL)
firstentry: all

trie_suff.c : suffcode.rules 
	echo "suffix_trie" > suffcode.temp
	awk '{suff = ""; for (i = length($$1); i > 0; i--) {\
            suff = suff substr($$1,i,1); } print suff, $$2; }' \
            suffcode.rules | sort >> suffcode.temp
	$(BIN)/make_trie < suffcode.temp > trie_suff.c
	/bin/rm -f suffcode.temp

trie_recode.c : recode.rules
	echo "recode_trie" > recode.temp
	awk '{suff = ""; for (i = length($$1); i > 0; i--) {\
	    suff = suff substr($$1,i,1); } print suff, $$2; }' \
	    recode.rules | sort >> recode.temp
	$(BIN)/make_trie < recode.temp > trie_recode.c
	/bin/rm -f recode.temp

#############################################################################
#  All code below this line (except for automatically created dependencies)
#  is independent of this particular makefile, and should not be changed!
#############################################################################

# Flags normally set in top level makefile.  Make sure you want these
# default if you compile locally in this directory
PREFIX = /home/smart.exp/src
SRCDIR = /home/smart.exp/src
BIN = $(PREFIX)/bin
LIB = $(PREFIX)/lib
H   = -I$(SRCDIR)/h
LINTH  = $(SRCDIR)/h


# Static libraries with gcc
CC       = gcc
CFLAGS   = $H -L$(LIB) -O -g -static

# Static libraries with cc
#cc CC       = cc
#cc CFLAGS   = $H -L$(LIB) -g -Bstatic


# Other macros used in some or all makefiles
PRINT  = print
AR     = -ar
ARFLAGS= urv
RANLIB = ranlib

# Entry points install and all are the same.  If you want to "make install"
# a non-test version in this directory alone, YOU MUST SPECIFY "PREFIX=..." ON
# THE COMMAND LINE  where ".../lib" gives the location you wish the
# library placed.  By default it will end up in the test directory.

install all:  $(LIB)/$(LIBNAME).a


#########################################################################
# Compilation of Libraries                                              #
# Note that a copy of the library is NOT kept in the current directory. #
#########################################################################
#libsun $(LIB)/$(LIBNAME).a : $(LIB)/$(LIBNAME).a($(LIBFILES))
#libsun 	$(RANLIB) $(LIB)/$(LIBNAME).a

$(LIB)/$(LIBNAME).a : $(LIBFILES)
	$(AR) $(ARFLAGS) $(LIB)/$(LIBNAME).a $?
	$(RANLIB) $(LIB)/$(LIBNAME).a


#########################################################################
# Lint                                                                  #
#########################################################################
# This invocation of lint is convoluted but semi-useful. Note that many
# routines are never explicitly called by others, but instead called
# through procedure tables.  "makelint" has to be run on all the libraries
# before "lint" will produce valid results.
makelint: $(LIB)/llib-l$(LIBNAME).ln

$(LIB)/llib-l$(LIBNAME).ln: *.c
	$(LINT) $H -C$(LIBNAME) *.c
	mv llib-l$(LIBNAME).ln $(LIB)

lint: *.c
	for f in $? ;\
	   do echo $$f; $(LINT) -v $H $$f $(LIB)/llib* -lm\
		| sed -e /multiply/d -e /errno/d -e /proc_root_root/d;\
	done    
	touch lint


#########################################################################
# Odds and ends                                                         #
#########################################################################
clean:
	/bin/rm -f *.o *.BAK *~  lint $(LIB)/llib-l$(LIBNAME).ln \
                  $(LIB)/$(LIBNAME).a makefile.old

semiclean:
	/bin/rm -f *.o

bfly: makefile.bfly
	gnumake -k -f makefile.bfly 

makefile.bfly: makefile ../makelib.bfly.templ
	awk '/All code below this line/ {exit} {print}' \
                    < makefile > makefile.bfly.new
	awk '/START BFLY STUFF/, /END BFLY STUFF/ {print}' \
                    < makefile.bfly >> makefile.bfly.new
	cat ../makelib.bfly.templ >> makefile.bfly.new
	mv makefile.bfly makefile.bfly.old
	mv makefile.bfly.new makefile.bfly

#########################################################################
# Determining program dependencies                                      #
#########################################################################
depend:
	grep '^#[ ]*include' *.c \
		| sed -e 's?:[^"]*"\([^"]*\)".*?: \$(LINT)H/\1?' \
		-e '/</d' \
		-e '/functions.h/d' \
	        -e 's/\.c/.o/' \
	        -e 's/\.y/.o/' \
	        -e 's/\.l/.o/' \
                -e 's?\(.*\)\.o?\$$(LIB)/\$$(LIBNAME)\.a(\1.o)?' \
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

# depend:
# 	grep '^#[ ]*include' *.c \
# 		| sed -e 's?:[^"]*"\([^"]*\)".*?: \$(LINTH)/\1?' \
# 		-e '/</d' \
# 		-e '/functions.h/d' \
# 	        -e 's/\.c/.o/' \
# 	        -e 's/\.y/.o/' \
# 	        -e 's/\.l/.o/' \
# 	> makedep
# 
# 	echo '/^# DO NOT DELETE THIS LINE/+2,$$d' >eddep
# 	echo '$$r makedep' >>eddep
# 	echo 'w' >>eddep
# 	cp makefile makefile.bak
# 	ed - makefile < eddep
# 	/bin/rm eddep makedep
# 	echo '# DEPENDENCIES MUST END AT END OF FILE' >> makefile
# 	echo '# IF YOU PUT STUFF HERE IT WILL GO AWAY' >> makefile
# 	echo '# see make depend above' >> makefile
# 
# DO NOT DELETE THIS LINE -- make depend uses it

# DEPENDENCIES MUST END AT END OF FILE
# IF YOU PUT STUFF HERE IT WILL GO AWAY
# see make depend above

