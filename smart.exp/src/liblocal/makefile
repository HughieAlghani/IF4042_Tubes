LIBLIST = libproc libindexing libinter libretrieve libfile libconvert \
          libprint
#          libevaluate libfeedback libgeneral


default all install bfly clean semiclean depend profile optimize \
count ansi lint makelint:
	for dir in $(LIBLIST) ;\
                do cd $$dir ;\
                make -k "$@" "CFLAGS=$(CFLAGS)" PREFIX=$(PREFIX) ; \
                cd ..;\
        done

makefiles:
	for dir in $(LIBLIST) ;\
                do awk '/All code below this line/ {exit} {print}' \
                    < $$dir/makefile > $$dir/makefile.new;\
        	cat ../makelib.templ >> $$dir/makefile.new;\
	        mv $$dir/makefile $$dir/makefile.old;\
        	mv $$dir/makefile.new $$dir/makefile;\
        done

#########################################################################
# Emacs tags file
#########################################################################
tags:
	for dir in $(LIBLIST) ; do \
		(cd ..; etags -atf TAGS $(TAGDIR)/$$dir/*.[ch] ) ;\
	done
