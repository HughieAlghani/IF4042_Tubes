#               LIBRARY MAKEFILE

# Module name
LIBNAME = lliblocal

# Files for this particular library module
# top_level procedures
INTER      = inter_web.o web.o
RETRIEVE   = loc_comb_sav.o sim_inner_m.o
PROC       = proc_local.o
COLL_SIM   = local_inv2.o

LIBFILES   = $(INTER) $(RETRIEVE) $(PROC) $(COLL_SIM)

#############################################################################
#  All code below this line (except for automatically created dependencies)
#  is independent of this particular makefile, and should not be changed!
#############################################################################

# Flags normally set in top level makefile.  Make sure you want these
# default if you compile locally in this directory
PREFIX = /home/smart/src
BIN = $(PREFIX)/bin
LIB = $(PREFIX)/objectlib
H   = ../../h


# Sun static libraries
#CC     = cc
#CFLAGS = -g -L$(LIB) -I$H -Bstatic

# Static libraries with gcc
CC       = gcc
CFLAGS   = -I$H -L$(LIB) -g -traditional -Wall

# Other macros used in some or all makefiles
PRINT  = print
AR     = -ar
ARFLAGS= urv
RANLIB = ranlib

# Entry points install and all are the same.  If you want to "make install"
# a non-test version in this directory alone, YOU MUST SPECIFY "PREFIX=..." ON
# THE COMMAND LINE  where ".../lib" gives the location you wish the
# library placed.  By default it will end up in the test directory.

install all:  $(LIB)/libdist.a


#########################################################################
# Compilation of Libraries                                              #
# Note that a copy of the library is NOT kept in the current directory. #
#########################################################################
$(LIB)/libdist.a : $(LIB)/libdist.a($(LIBFILES))
	$(RANLIB) $(LIB)/libdist.a

#$(LIB)/libdist.a : $(LIBFILES)
#        $(AR) $(ARFLAGS) $? $LIB/libdist.a
#	$(RANLIB) $(LIB)/libdist.a#


#########################################################################
# Lint                                                                  #
#########################################################################
# This invocation of lint is convoluted but semi-useful. Note that many
# routines are never explicitly called by others, but instead called
# through procedure tables.  "makelint" has to be run on all the libraries
# before "lint" will produce valid results.
makelint: $(LIB)/llib-l$(LIBNAME)dist.ln

$(LIB)/llib-l$(LIBNAME)dist.ln: *.c
	$(LINT) -I$H -C$(LIBNAME) *.c
	mv llib-l$(LIBNAME).ln $(LIB)/llib-l$(LIBNAME)dist.ln

lint: *.c
	for f in $? ;\
	   do echo $$f; $(LINT) -v -I$H $$f $(LIB)/llib* -lm\
		| sed -e /multiply/d -e /errno/d -e /proc_root_root/d;\
	done    
	touch lint


#########################################################################
# Odds and ends                                                         #
#########################################################################
clean:
	/bin/rm -f *.o *.BAK *~  lint $(LIB)/llib-l$(LIBNAME)dist.ln

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
		| sed -e 's?:[^"]*"\([^"]*\)".*?: \$H/\1?' \
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
# 		| sed -e 's?:[^"]*"\([^"]*\)".*?: \$H/\1?' \
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

