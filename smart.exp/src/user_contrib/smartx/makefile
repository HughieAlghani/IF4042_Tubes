###########################################################################
#                            THE
#                           SMART
#               INFORMATION RETRIEVAL SYSTEM
#
#   (c) copyright 1984,1988, 1989, 1991 Chris Buckley, Gerard Salton
#
#   This recursive makefile constructs all programs needed by the 
#   SMART system and places them in ../bin.
#
#   The main entry point for compilation is "install".
#   Each makefile in a source sub-directory has (at least) six
#   entry points:
#       all:     compile local copies of files
#       install: in addition, compile global copies of programs and libraries
#       rcs:     save all checked out files in RCS archives
#       clean:   remove all unnecessary files from directory
#       print:   print a hardcopy of all text (doesn't work (rcs))
#       depend:  change the makefile to establish all dependencies of source
#                files on .h files
#
#   This makefile has entry points corresponding to the six above, plus
#   an entry point to produce installed code that is profiled, and
#   another to produce installed optimized code.
############################################################################

ACTION   = all

#PREFIX = /smart/new
PREFIX = /home/ux5/ux5c/icsd/lit/smart/src.new
BIN = $(PREFIX)/bin
LIB = $(PREFIX)/lib 
H   = -I../h 

# Version numbers
MAJOR = 10
MINOR = 1

# SMART Test binaries and libraries
#TPREFIX = $(PREFIX)/test
TPREFIX = $(PREFIX)
TMAJOR = 11

# Sun shared libraries with cc  NOTE DOES NOT WORK WELL (TOO SLOW) WITH
# OUR DYNAMIC SETUP        
CC     = cc
CFLAGS = -PIC -L$$(LIB) $H
OPTFLAGS = -O4 -fsingle
LIBEXT = so
STATIC = -Bstatic

# Static libraries with gcc
# Flags for gcc 
#  -ansi,  -traditional ,  try no -g to get dynamic, -W, -Wall, -v, 
#  -fstrength-reduce, -fcombine-regs, -finline-functions, -fwritable-strings,
#  -fdelayed-branch, (-pedantic), (-static (implied by -g)), -pipe
#CC       = gcc
#CFLAGS   = $H -L$$(LIB) -g -traditional -Wall
#OPTFLAGS = -O -fstrength-reduce -fcombine-regs -finline-functions \
            -fdelayed-branch
#LIBEXT   = a
#STATIC   =

test: 
	-mkdir $(TPREFIX)
	-mkdir $(TPREFIX)/bin
	-mkdir $(TPREFIX)/lib
	make  ACTION=install MAJOR=$(TMAJOR) PREFIX=$(TPREFIX) \
              "CFLAGS=$(CFLAGS) -g" lib_action program_action
install: 
	make clean
	make  ACTION=install lib_action program_action
	make clean
rcs: 
	make  ACTION=rcs h_action lib_action program_action
clean:
	make  ACTION=clean h_action lib_action program_action
print:
	make  ACTION=print h_action lib_action program_action
depend:
	make  ACTION=depend lib_action program_action

profile_test:
	-mkdir $(PREFIX)/profile
	-mkdir $(PREFIX)/profile/bin
	-mkdir $(PREFIX)/profile/lib
	make  ACTION=install 'CFLAGS=$(CFLAGS) -pg $(OPTFLAGS)' \
              PREFIX=$(PREFIX)/profile lib_action program_action

optimize:
	-mkdir $(PREFIX)/opt
	-mkdir $(PREFIX)/opt/bin
	-mkdir $(PREFIX)/opt/lib
	make  ACTION=install 'CFLAGS=$(CFLAGS) $(OPTFLAGS)' \
              PREFIX=$(PREFIX)/opt  lib_action program_action

static:  
	-mkdir $(PREFIX)/static
	-mkdir $(PREFIX)/static/bin
	-mkdir $(PREFIX)/static/lib
	make  "ACTION=install" "CFLAGS=$(CFLAGS) $(STATIC) -g" \
              PREFIX=$(PREFIX)/static LIBEXT=a lib_action program_action

counts:  
	-mkdir $(PREFIX)/counts
	-mkdir $(PREFIX)/counts/bin
	-mkdir $(PREFIX)/counts/lib
	make  ACTION=install 'CFLAGS=$(CFLAGS) -a' \
              PREFIX=$(PREFIX)/counts  lib_action program_action

lint:
	-mkdir $(TPREFIX)
	-mkdir $(TPREFIX)/bin
	-mkdir $(TPREFIX)/lib
	make  ACTION=lint PREFIX=$(TPREFIX) lib_action program_action


LIBLIST = libx libconvert libcreate libdatabase libevaluate libexp \
          libfeedback libfile libgeneral \
          libindexing libinter libprint libproc libretrieve libtop
          
BINLIST = utility smart
SHLIST  = 
HLIST   = h


makefiles: 
	for dir in $(LIBLIST) ;\
                do awk '/LIBRARY MAKEFILE/, /All code below this line/' \
                    < $$dir/makefile > $$dir/makefile.new;\
        	cat makelib.templ >> $$dir/makefile.new;\
	        mv $$dir/makefile $$dir/makefile.old;\
        	mv $$dir/makefile.new $$dir/makefile;\
        done
	for dir in $(BINLIST) ;\
                do awk '/BINARY MAKEFILE/, /All code below this line/' \
                    < $$dir/makefile > $$dir/makefile.new;\
        	cat makebin.templ >> $$dir/makefile.new;\
	        mv $$dir/makefile $$dir/makefile.old;\
        	mv $$dir/makefile.new $$dir/makefile;\
        done
#	make depend


#########################################################################
# Actions on libraries                                                  #
#########################################################################
lib_action:
	for dir in $(LIBLIST) ;\
                do cd $$dir ;\
                make -k "$(ACTION)" "CFLAGS=$(CFLAGS)" PREFIX=$(PREFIX) \
                LIBEXT=$(LIBEXT) MAJOR=$(MAJOR) MINOR=$(MINOR);\
                cd ..;\
        done

                
#########################################################################
# Actions on programs                                                   #
# Note that compilation depends on the desired libraries being present. #
# These libraries are made above!                                       #
#########################################################################
program_action: 
	for dir in $(BINLIST) $(SHLIST) ;\
                do cd $$dir ;\
                make -k "$(ACTION)" "CFLAGS=$(CFLAGS)" PREFIX=$(PREFIX) \
                LIBEXT=$(LIBEXT) MAJOR=$(MAJOR) MINOR=$(MINOR);\
                cd ..;\
        done

#########################################################################
# Actions on include files
####################
#####################################################
h_action: 
	(cd h; make -k $(ACTION) )
