# $Id$

ifdef ROOT
INSTALL_DIR	= $(ROOT)
else
$(error "no root directory!")
endif # ROOT

# set ARCH
ifndef ARCH
ifdef PIPS_ARCH
ARCH	= $(PIPS_ARCH)
else
ifdef NEWGEN_ARCH
ARCH	= $(NEWGEN_ARCH)
else
ifdef LINEAR_ARCH
ARCH	= $(LINEAR_ARCH)
else
ARCH	= DEFAULT
endif # LINEAR_ARCH
endif # NEWGEN_ARCH
endif # PIPS_ARCH
endif # ARCH

# checks consistency?
ifndef INSTALL_DIR
$(error "INSTALL_DIR macro is not defined!")
endif

ifeq ($(INSTALL_DIR),)
$(error "INSTALL_DIR macro is empty!")
endif

ifndef ARCH
$(error "ARCH macro is not defined")
endif

# where to install stuff
BIN.d	= $(INSTALL_DIR)/Bin/$(ARCH)
LIB.d	= $(INSTALL_DIR)/Lib/$(ARCH)
INC.d	= $(INSTALL_DIR)/Include
DOC.d	= $(INSTALL_DIR)/Doc
HTM.d	= $(INSTALL_DIR)/Html
UTL.d	= $(INSTALL_DIR)/Utils
SHR.d	= $(INSTALL_DIR)/Share
RTM.d	= $(INSTALL_DIR)/Runtime
MAKE.d	= $(INSTALL_DIR)/makes

include $(MAKE.d)/$(ARCH).mk

all: $(ARCH)

UTC_DATE = $(shell date -u | tr ' ' '_')
CPPFLAGS += -DSOFT_ARCH='"\$(ARCH)"'
# {C,CPP,LD,L,Y}OPT macros allow to *add* things from the command line
# as gmake CPPOPT="-DFOO=bar" ... that will be added to the defaults
# a typical interesting example is to put -pg in {C,LD}OPT
#
PREPROC	= $(CC) -E $(CANSI) $(CPPOPT) $(CPPFLAGS)
COMPILE	= $(CC) $(CANSI) $(CFLAGS) $(COPT) $(CPPOPT) $(CPPFLAGS) -c
F77CMP	= $(FC) $(FFLAGS) $(FOPT) -c
LINK	= $(LD) $(LDFLAGS) $(LDOPT) -o
SCAN	= $(LEX) $(LFLAGS) $(LOPT) -t
TYPECK	= $(LINT) $(LINTFLAGS) $(CPPFLAGS) $(LINT_LIBS)
PARSE	= $(YACC) $(YFLAGS) $(YOPT)
ARCHIVE = $(AR) $(ARFLAGS)
PROTOIZE= $(PROTO) $(PRFLAGS) -E "$(PREPROC) -DCPROTO_IS_PROTOTYPING"
M4FLT	= $(M4) $(M4OPT) $(M4FLAGS)
MAKEDEP	= $(CC) $(CMKDEP) $(CANSI) $(CFLAGS) $(COPT) $(CPPOPT) $(CPPFLAGS)
NODIR	= --no-print-directory
COPY	= cp
MOVE	= mv
JAVAC	= javac
JNI	= javah -jni
MKDIR	= mkdir -p
RMDIR	= rmdir
INSTALL	= install

# for easy debugging... e.g. gmake ECHO='something' echo
echo:; @echo $(ECHO)

$(ARCH)/%.o: %.c ; $(COMPILE) $< -o $@
$(ARCH)/%.o: %.f ; $(F77CMP) $< -o $@

%.class: %.java; $(JAVAC) $<
%.h: %.class; $(JNI) $*

# latex
%.dvi: %.tex
	-grep '\\\\makeindex' $*.tex && touch $*.ind
	$(LATEX) $<
	-grep '\\\\bibdata{' \*.aux && { $(BIBTEX) $* ; $(LATEX) $< ;}
	test ! -f $*.idx || { $(MAKEIDX) $*.idx ; $(LATEX) $< ;}
	$(LATEX) $<
	touch $@

%.ps: %.dvi
	$(DVIPS) -o $@ $<
	touch $@

%.newgen: %.tex
	$(RM) $@
	remove-latex-comments $<
	chmod a-w $@

%.o: %.f
	$(F77COMPILE) $< -o $@

%.f: %.m4f
	$(M4FILTER) $(M4FOPT) $< > $@

%.c: %.m4c
	$(M4FILTER) $(M4COPT) $< > $@

%.h: %.m4h
	$(M4FILTER) $(M4HOPT) $< > $@

ifdef INC_TARGET

current_directory=$(pwd)
dir_real_name=$(basename $current_directory)
dir_simple_name=$(echo $dir_real_name | tr '-' '_')

build-header-file:
	$(COPY) $(TARGET)-local.h $(INC_TARGET); \
	{ \
	  echo "/* header file built by \$(PROTO) */"; \
	  echo "#ifndef ${dir_simple_name}_header_included";\
	  echo "#define ${dir_simple_name}_header_included";\
	  cat $(TARGET)-local.h;\
	  $(PROTOIZE) $(INC_CFILES) | \
	  sed 's/struct _iobuf/FILE/g;s/__const/const/g;/_BUFFER_STATE/d;/__inline__/d;' ;\
	  echo "#endif /* ${dir_simple_name}_header_included */";\
	} > $(INC_TARGET).tmp
	$(MOVE) $(INC_TARGET).tmp $(INC_TARGET)

header:	.header $(INC_TARGET)

# .header carrie all dependencies for INC_TARGET:
.header: $(TARGET)-local.h $(DERIVED_HEADERS) $(INC_CFILES) 
	$(MAKE) $(GMKNODIR) build-header-file ; touch .header

$(INC_TARGET): $(TARGET)-local.h
	$(RM) .header; $(MAKE) $(GMKNODIR) .header
endif # INC_TARGET

# ARCH subdirectory
$(ARCH):; $(MKDIR) $(ARCH)
clean: arch-clean
arch-clean:; -$(RMDIR) $(ARCH)

# multiphase compilation
phase1: install_inc install_shr install_utl
phase2:	install_lib install_bin
phase3: install_htm install_doc
phase4: install_rtm

# includes
INSTALL_INC	+=   $(INC_TARGET)

$(INC.d):; $(MKDIR) $(INC.d)

install_inc: $(INSTALL_INC) $(INC.d)
	$(INSTALL) $(INSTALL_INC) $(INC.d)

# libraries
ifdef LIB_TARGET
INSTALL_LIB	+=   $(LIB_TARGET)
endif

$(LIB.d):; $(MKDIR) $(LIB.d)

install_lib: $(INSTALL_LIB)  $(LIB.d)
	$(INSTALL) $(INSTALL_LIB) $(LIB.d)

# binaries
INSTALL_BIN	+=   $(BIN_TARGET)

$(BIN.d):; $(MKDIR) $(BIN.d)

install_bin: $(INSTALL_BIN)  $(BIN.d)
	$(INSTALL) $(INSTALL_BIN) $(BIN.d)

# clean installation
uninstall:
	$(RM) -r $(INC.d) $(LIB.d) $(BIN.d)
	-$(RMDIR) $(ROOT)/Bin $(ROOT)/Lib
