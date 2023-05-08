#               LIBRARY MAKEFILE

# Module name
LIBNAME = libprint

# Files for this particular library module
# Low-level print procedures to print individual smart tuples to incore buffer
INDIV     = p_textloc.o p_long.o p_spec.o \
            p_sm_display.o p_vector.o p_string.o p_account.o p_tr_vec.o \
            p_float.o p_top_res.o p_full_res.o \
            p_fdbk_info.o p_inv.o p_smeval.o p_rr_vec.o p_dict.o \
            p_rel_hd.o p_buf.o p_tokensect.o p_tokendoc.o p_sm_condoc.o \
            p_consect.o p_simp_inv.o p_vec_list.o

# Low level Print procedures to print interpreted smart tuples (ie, 
# what the smart tuple describes instead of the smart tuple itself) to buffer
INTER     = pi_textdoc.o pi_textloc.o

# Low level print procedures to print a set of interpreted smart objects (ie, 
# what the smart object describes) to a buffer
INTER_OBJ = po_rclprn.o po_tr_eval.o po_ind_tr.o po_ind_rr.o

# Low level hierarchy procedures to print interpreted individual smart tuples
# to a buffer.
HIER_INTER   = ph_text_pp.o ph_text_raw.o ph_text_for.o ph_vec_dict.o \
               ph_text_fil.o

# Mid level hierarchy procedures to print entire relational objects to a file
# Calls one or more of low level procedures above.
HIER_OBJ   = pobj_text.o pobj_dict.o pobj_inv.o pobj_textloc.o \
             pobj_vec.o pobj_vecdict.o pobj_rr_vec.o pobj_tr_vec.o \
             pobj_tr_eval.o pobj_rr_eval.o pobj_rel_hd.o pobj_proc.o \
             pobj_did_nv.o
             

# Top level hierarchy procedure to call print procedures in HIER_OBJ
HIER_TOP   = print.o

LIBFILES  =  $(INDIV) $(INTER) $(INTER_OBJ) $(HIER_INTER) $(HIER_OBJ) \
             $(HIER_TOP)


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

