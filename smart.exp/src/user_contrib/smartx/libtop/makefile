#               LIBRARY MAKEFILE

# Module name
LIBNAME = libtop

TOP       = inter.o retrieve.o ret_fdbk.o # examine.o
VARIANTS  = inter_sent.o inter_match.o x_inter_match.o
GET_QUERY =  get_q_dsmart.o get_q_smart.o get_q_text.o get_q_vec.o \
             get_q_random_vec.o
LIBFILES  = $(TOP) $(GET_QUERY) $(VARIANTS)



#############################################################################
#  All code below this line (except for automatically created dependencies)
#  is independent of this particular makefile, and should not be changed!
#############################################################################

# Flags normally set in top level makefile.  Make sure you want these
# default if you compile locally in this directory
PREFIX = /home/ux5/ux5c/icsd/lit/smart/src.new
BIN = $(PREFIX)/bin
LIB = $(PREFIX)/lib
H   = ../h

# Version numbers
MAJOR = 10
MINOR = 1

# Sun static libraries
CC     = cc
CFLAGS = -g -L$(LIB) -I../$H -Bstatic

# Static libraries with gcc and optimization
#CC       = gcc
#OPTFLAGS = -O -fstrength-reduce -fcombine-regs -finline-functions \
#            -fdelayed-branch
#CFLAGS   = -I$H -L$(LIB) -g -traditional -Wall $(OPTFLAGS)

# Static libraries with gcc
#CC       = gcc
#CFLAGS   = -I$H -L$(LIB) -g -traditional -Wall

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
$(LIB)/$(LIBNAME).a : $(LIB)/$(LIBNAME).a($(LIBFILES))
	$(RANLIB) $(LIB)/$(LIBNAME).a

#########################################################################
# Saving program versions                                               #
#########################################################################
rcs:
	-ci -q "-mAutomatic checkin" *.c

#########################################################################
# Odds and ends                                                         #
#########################################################################
clean:
	/bin/rm -f *.o *.BAK *~ $(LIBNAME).a $(LIBNAME).so $(LIB)/$(LIBNAME).a

print: # $(FILES) makefile
	$(PRINT) $?
	cat >print </dev/null

# This version of lint is no longer useful, since most routines never
# explicitly called by others
lint:
	$(LINT) -I$H -C$(LIBNAME) *.c
	mv llib-l$(LIBNAME).ln $(LIB)


#########################################################################
# Determining program dependencies                                      #
#########################################################################
depend:
	grep '^#[ ]*include' *.c \
		| sed -e 's?:[^"]*"\([^"]*\)".*?: \$H/\1?' \
		-e '/</d' \
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

# DO NOT DELETE THIS LINE -- make depend uses it

# DEPENDENCIES MUST END AT END OF FILE
# IF YOU PUT STUFF HERE IT WILL GO AWAY
# see make depend above
