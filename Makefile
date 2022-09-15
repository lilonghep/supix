# $Id: Makefile 1222 2020-07-19 02:14:48Z mwang $
#
# Makefile for an application development with ROOT.
# - based on $ROOTSYS/share/doc/root/test/Makefile.
#
# 
# Created by:  WANG Meng <mwang@sdu.edu.cn> at 2016-05-02 14:03:58
########################################################################
# ----------------------------------------------------------------------
#		      ROOT system configuration
# ----------------------------------------------------------------------
### check ROOT installation
RC           := root-config
ifneq ($(shell which $(RC) 2>&1 | sed -ne "s@.*/$(RC)@$(RC)@p"),$(RC))
  ifneq ($(ROOTSYS),)
    RC1          := $(ROOTSYS)/bin/root-config
    ifneq ($(shell which $(RC1) 2>&1 | sed -ne "s@.*/$(RC)@$(RC)@p"),$(RC))
      $(error Please make sure $(RC1) can be found in path)
    else
      RC           := $(RC1)
    endif
  else
    $(error Please make sure $(RC) can be found in path)
  endif
endif

### include Makefile.arch of ROOT
MKARCH	:= $(wildcard $(shell $(RC) --etcdir)/Makefile.arch)
ifeq ($(MKARCH),)
  MKARCH	:= $(wildcard $(ROOTSYS)/test/Makefile.arch)
endif
ifeq ($(MKARCH),)
  $(error Makefile.arch NOT existing!!!)
endif

$(info include $(MKARCH) ......)
include $(MKARCH)

ROOTVER		:= $(shell $(RC) --version)
ROOTVERMAJ	:= $(shell $(RC) --version | cut -d. -f1)

# ----------------------------------------------------------------------
#			personal configuation
# ----------------------------------------------------------------------
CHECKVAR := (EXPLLINKLIBS) $(EXPLLINKLIBS)

# this library name
THISLIB		= supix

# this library source files: .cxx
THISLIBSRCS	= SupixFPGA.cxx
THISLIBSRCS	+= SupixDAQ.cxx
THISLIBSRCS	+= RunInfo.cxx
THISLIBSRCS	+= SupixAnly.cxx
# .C as well
THISLIBSRCS_C	= SupixTree.C


# standalone executables
EXESRCS		= control.cxx
EXESRCS		+= daq.cxx
EXESRCS		+= book.cxx
TESTS		= test_main.cxx test_hybrid.cxx


### test/
TESTSRCS	:= threads.cxx io_raw.cxx show_limits.cxx test_pipeline.cxx
TESTSRCS	+= test_thread.cxx
TESTSRCS	:= $(TESTSRCS:%=test/%)
TESTOBJS	 = $(TESTSRCS:%.cxx=%.o)
TESTEXES	 = $(TESTSRCS:%.cxx=%.exe)
OBJS	+= $(TESTOBJS)
#EXES	+= $(TESTEXES)

### general utilities
###
UTIL		= util
UTILSRCS 	= error.cxx util.cxx Timer.cxx pipeline.cxx
UTILHDRS 	= $(UTILSRCS:%.cxx=%.h)
UTILOBJS 	= $(UTILSRCS:%.cxx=%.o)
UTILSO		= lib$(UTIL).$(DllSuf)
LDUTIL		= -L. -l$(UTIL)
OBJS		+= $(UTILOBJS)
NEWLIBS		+= $(UTILSO)

### targets

.PHONY: all lib exes test
all: lib exes

test : $(TESTEXES)

$(TESTEXES): %.exe:%.o $(UTILSO)
	$(MSG)
	$(LD) $(OutPutOpt)$@ $(LDFLAGS) $< $(LDUTIL) $(EXELIBS)


### dependences could be added here, if any.

# header files dependence
$(UTILOBJS): %.o:%.h

$(TESTOBJS) : $(UTILSRCS:%.cxx=%.h)

test/test_pipeline.o : pipeline.h

THISLIBOBJS	 = $(THISLIBSRCS:%.cxx=%.o)
THISLIBOBJS	+= $(THISLIBSRCS_C:%.C=%.o)

$(THISLIBOBJS): mydefs.h

SupixFPGA.o: RunInfo.h

SupixAnly.o: RunInfo.h SupixTree.h

SupixDAQ.o : RunInfo.h $(UTILHDRS)

book.o : SupixAnly.h

#test_main.exe : mydefs.h

### additional libs added here
#ifeq ($(PLATFORM), linux)
MYCXXFLAGS	 = -I.
#endif

# uncomment and modify following lines if having personel lib in ~/rootlib/.
#MYCXXFLAGS	 = -I$(HOME)/rootlib
#MYLIBS		 = -L$(HOME)/rootlib -lmyroot
#THISLIBDEPS	 = libmyroot

# uncomment following lines if using GSL.
#MYCXXFLAGS	+= $(shell gsl-config --cflags)
#MYLIBS		+= $(shell gsl-config --libs)
#THISLIBDEPS	+= libgsl
#
# add dependent ROOT libraries here, if ANY.
#THISLIBDEPS	+= libPhysics


########################################################################
#
# NO needs to modify the following in general.
#
########################################################################
ExeSuf 	 = exe
EXES	+= $(EXESRCS:%.cxx=%.exe)
OBJS	+= $(EXESRCS:%.cxx=%.o)

# by default, all lib .cxx files having corresponding .h files.
OBJS		+= $(THISLIBOBJS)
$(THISLIBOBJS): %.o:%.h

THISLIBNAME	= lib$(THISLIB)
THISLIBLINKDEF	= $(THISLIBNAME)_LinkDef.h
THISLIBROOTMAP	= $(THISLIBNAME).rootmap
THISLIBDICT	= $(THISLIBNAME)_dict
THISLIBSO	= $(THISLIBNAME).$(DllSuf)
NEWLIBS		+= $(THISLIBSO)

#lib: $(THISLIBSO) $(THISLIBROOTMAP)
lib: $(NEWLIBS)

exes: $(EXES)

# ----------------------------------------------------------------------
#		       compile and link options
# ----------------------------------------------------------------------
ifdef DEBUG
CXXFLAGS	+= -DDEBUG
endif

ifdef LOCKALL
CXXFLAGS	+= -DLOCKALL
endif

CXXFLAGS	+= $(MYCXXFLAGS)

ifeq ($(shell root-config --has-mathmore),yes)
MORELIBS	+= -lMathMore
endif

### for executables
# memory checker???
#LIBS		+= -lNew

LIBS		+= $(MORELIBS)
LIBS		:= $(MYLIBS) $(LIBS)

### for shared libraries, ARCH dependent
ifneq ($(EXPLLINKLIBS),)
EXPLLINKLIBS	+= $(MORELIBS)
EXPLLINKLIBS	:= $(MYLIBS) $(EXPLLINKLIBS)
endif

# ----------------------------------------------------------------------
#				rules
# ----------------------------------------------------------------------
MSG	= @echo "====== update $@ for newer $? ......"

# add suffixes
.SUFFIXES: .$(SrcSuf) .$(ObjSuf) .$(DllSuf) .$(ExeSuf) .C

$(THISLIBSO): $(THISLIBOBJS) $(THISLIBDICT).o
	$(MSG)
ifeq ($(PLATFORM), macosx)
        # -install_name /path/to/libxxx.so
	$(LD) $(OutPutOpt)$@ $(SOFLAGS)$@ $(LDFLAGS) $^ $(LDUTIL) $(EXPLLINKLIBS)
else
	$(LD) $(OutPutOpt)$@ $(SOFLAGS)   $(LDFLAGS) $^ $(LDUTIL) $(EXPLLINKLIBS)
endif

ifeq ($(ROOTVERMAJ), 6)
else
$(THISLIBROOTMAP): $(THISLIBLINKDEF)
	$(MSG)
	rlibmap -o $@ -l $(THISLIBNAME) -d $(THISLIBDEPS) -c $<
endif

# --------------------------
# generate CINT dictionaries
#     man rootcint
# or
#     rootcint -h
# for more information.
# --------------------------
# LinkDef.h must be the last argument
# -f force to override the output file
# -c CINT method interface stubs also written to the output file
# -p use compiler's preprocessor instead of CINT's.

DICTHDRS	= RunInfo.h SupixAnly.h $(THISLIBSRCS_C:%.C=%.h)
# MYCXXFLAGS: include file dirs and preprocessor defines, before the 1st header file.
#$(THISLIBDICT).cxx: $(THISLIBSRCS:%.cxx=%.h) $(THISLIBSRCS_C:%.C=%.h) $(THISLIBLINKDEF)
$(THISLIBDICT).cxx: $(DICTHDRS) $(THISLIBLINKDEF)
	$(MSG)
ifeq ($(ROOTVERMAJ), 6)
	rootcling -f $@ -rml $(THISLIBSO) -rmf $(THISLIBROOTMAP) -c $(MYCXXFLAGS) $^
else
	rootcint -f $@ -c -p $(MYCXXFLAGS) $^
endif

###

$(UTILSO) : $(UTILOBJS)
	$(MSG)
ifeq ($(PLATFORM), macosx)
        # -install_name /path/to/libxxx.so
	$(LD) $(OutPutOpt)$@ $(SOFLAGS)$@ $(LDFLAGS) $^ $(EXPLLINKLIBS)
else
	$(LD) $(OutPutOpt)$@ $(SOFLAGS)   $(LDFLAGS) $^ $(EXPLLINKLIBS)
endif

### implicit rules

# for make .exe files
# NOLIB=1	not link any libs, unit tests
ifndef NOLIB
EXELIBS	= -L. -l$(THISLIB) $(LIBS)
endif


%.o: %.c
	$(MSG)
	$(CXX) $(OutPutOpt)$@ -c $(CXXFLAGS) $<

%.o: %.C
	$(MSG)
	$(CXX) $(OutPutOpt)$@ -c $(CXXFLAGS) $<

%.o: %.cxx
	$(MSG)
	$(CXX) $(OutPutOpt)$@ -c $(CXXFLAGS) $<

%.exe: %.o $(THISLIBSO) $(UTILSO)
	$(MSG)
	$(LD) $(OutPutOpt)$@ $(LDFLAGS) $< $(LDUTIL) $(EXELIBS)

# ----------------------------------------------------------------------
#			      utilities
# ----------------------------------------------------------------------
help:
	@echo
	@echo "Usage:	make [ lib | test | help[root] | [dist]clean ] [DEBUG=1] [LOCKALL=1] [NOLIB=1]"
	@echo

helproot:
	@echo
	@echo "Variables defined by: $(MKARCH)"
	@echo
	@echo "ROOTSYS		= $(ROOTSYS)"
	@echo "ARCH		= $(ARCH)"
	@echo "PLATFORM	= $(PLATFORM)"
	@echo "CC		= $(CC)"
	@echo "CXX		= $(CXX)"
	@echo "F77		= $(F77)"
	@echo "LD		= $(LD)"
	@echo "CFLAGS		= $(CFLAGS)"
	@echo "CXXFLAGS	= $(CXXFLAGS)"
	@echo "MYCXXFLAGS	= $(MYCXXFLAGS)"
	@echo "LDFLAGS		= $(LDFLAGS)"
	@echo "SOFLAGS		= $(SOFLAGS)"
	@echo "MYLIBS		= $(MYLIBS)"
	@echo "LIBS		= $(LIBS)"
	@echo "GLIBS		= $(GLIBS)"
	@echo "EXPLLINKLIBS	= $(EXPLLINKLIBS)"
	@echo "OutPutOpt	= '$(OutPutOpt)'"
	@echo "SrcSuf		= $(SrcSuf)"
	@echo "ObjSuf		= $(ObjSuf)"
	@echo "DllSuf		= $(DllSuf)"
	@echo "ExeSuf		= '$(ExeSuf)'"
	@echo "EXELIBS		= $(EXELIBS)"
#	@echo "MORELIBS		= $(MORELIBS)"
ifeq ($(PLATFORM), macosx)
	@echo "MACOSX_MINOR	= $(MACOSX_MINOR)"
	@echo "MACOSXTARGET	= $(MACOSXTARGET)"
endif
	@echo "CHECKVAR	= $(CHECKVAR)"
	@echo "THISLIBSRCS	= $(THISLIBSRCS)"
	@echo "THISLIBSRCS_C	= $(THISLIBSRCS_C)"


# temporary files
#   *_dict.*		by rootcint
#   *_rdict.pcm
#   *_C.{d,so}		by root> .L xxx.C+
# ----------------------------------------------------------------------
clean:
	@rm -vf $(OBJS) *_dict.* *_rdict.pcm *_C.{d,so} *_cxx.{d,so}


distclean: clean
	@rm -vf $(EXES) $(TESTEXES) $(NEWLIBS) $(THISLIBROOTMAP) *.exe
