#               LIBRARY MAKEFILE

# Module name
LIBNAME = libretrieve

# Files for this particular library module
# top_level procedures
COLL       = sim_vec_inv.o sim_text.o sim_seq.o sim_part_inv.o \
             local_inv.o 
VEC        = part_thresh.o sim_vec_vec.o local_thresh.o
CTYPE_COLL = sim_cty_inv.o
CTYPE_VEC  = sim_inner.o
VECS_VECS  = vecs_vecs.o
OUTPUT     = ret_tr.o ret_tr_rr.o
RANK       = rank_did.o rank_parts_m.o rank_parts_s.o rank_parts_a.o \
	     local_comb_r.o local_comb_f.o local_comb_v.o
GETSEEN    = get_seen.o        
GETQUERY   = get_q_vec.o get_q_text.o get_q_text.o \
             get_q_user.o get_q_parts.o \
             user_q_cat.o  user_q_edit.o
TOP        = retrieve.o ret_fdbk.o retrieve_all.o

LIBFILES   = $(COLL) $(VEC) $(CTYPE_COLL) $(CTYPE_VEC) $(VECS_VECS) \
             $(OUTPUT) $(RANK) $(GETSEEN) $(GETQUERY) $(TOP)

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

