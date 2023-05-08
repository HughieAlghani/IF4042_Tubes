#               BINARY MAKEFILE

LDLIBS = -lproc \
         -ltop -linter \
	 -lx -lxview -lolgx -lX -lguide -lguidexv -lX11\
         -lindexing -levaluate -lretrieve -lfeedback -lcreate\
         -lconvert -lprint \
         -lexp -lfile -ldatabase -lgeneral -lm

# SIMPLE programs are those composed only of a main program in C (the rest
# being supplied by libraries)
SIMPLE = 
COMPLEX = smart 


# Sun shared libraries with cc
CC     = cc
CFLAGS = -g -PIC -L$(LIB) -L/usr/openwin/lib -L/usr/local/guide/lib $H -Bstatic
LIBEXT = so

all: $(SIMPLE) $(COMPLEX)

smart: smart.c

smart:
	$(CC) $(CFLAGS) -Bstatic $(CPPFLAGS) $(LDFLAGS) $(TARGET_ARCH) \
        -o smart smart.c version.c $(LDLIBS)  -L/usr/local/guide/lib

smart: ../lib/*.a 

smart_malloc: 
	$(CC) -DDEBUG2  $(CFLAGS) $(CPPFLAGS) $(LDFLAGS) $(TARGET_ARCH) \
        -o smart_malloc smart.c version.c /usr/lib/debug/malloc.o $(LDLIBS) 

#############################################################################
#  All code below this line (except for automatically created dependencies)
#  is independent of this particular makefile, and should not be changed!
#############################################################################


# Flags normally set in top level makefile.  Make sure you want these
# default if you compile locally in this directory
PREFIX = /home/ux5/ux5c/icsd/lit/smart/src.new
BIN = $(PREFIX)/bin 
LIB = $(PREFIX)/lib /usr/openwin/lib
H   = -I../h

# Static libraries with gcc
#CC       = gcc
#CFLAGS   = $H -L$(LIB) -L/usr/openwin/lib -g -traditional -Wall
#LIBEXT   = a

# Gcc macros for object files, cc for final linking (to get shared libraries)
#CC       = gcc
#CFLAGS   = $H -L$(LIB) -O -g -traditional
#CFLAGS2   = $H -L$(LIB) -g
#LIBEXT   = so
#.c:
#	$(COMPILE.c) $<
#	cc $(CFLAGS2) $(LDFLAGS) $(TARGET_ARCH) -o $@ $@.o $(LDLIBS)
#	$(RM) $@.o


# Other macros used in some or all makefiles
PRINT  = print
AR     = -ar
ARFLAGS= urv
RANLIB = ranlib
INSTALL = /bin/cp


# Note that "install" and "all" share the same entry-point.  If you wish
# to install a binary in a non-test directory, you must make sure the
# binary does not exist (eg. "make clean") and then run make specifying
# the proper value for PREFIX.  This ensures that the binary will be
# linked with the proper library (possibly at run-time).
install all: $(SIMPLE) $(COMPLEX)
	$(INSTALL) $? $(BIN)
	touch all install

#########################################################################
# Saving program versions                                               #
#########################################################################
rcs: 
	-ci -q "-mAutomatic Checkin" *.c

#########################################################################
# Odds and ends                                                         #
#########################################################################
clean:
	/bin/rm -f *.o *.BAK $(SIMPLE) $(COMPLEX)


print: $(SIMPLE) $(COMPLEX) $(FILES)
	for f in $? ;\
                do $(PRINT) $$f.* ;\
        done
	touch print

lint: 
	for f in *.c ;\
                do echo $$f; $(LINT) -I../h $$f $(LIB)/llib* ;\
        done

#########################################################################
# Determining program dependencies                                      #
#########################################################################
depend:
	grep '^#[ ]*include' *.c *.l *.y \
		| sed -e 's?:[^"]*"\([^"]*\)".*?: \$H/\1?' \
			-e '/</d' \
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
# see make depend above
