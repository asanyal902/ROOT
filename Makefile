# Top level Makefile for ROOT System
# Copyright (c) 2000 Rene Brun and Fons Rademakers
#
# Author: Fons Rademakers, 29/2/2000


##### include path/location macros (result of ./configure) #####

include config/Makefile.config

##### include machine dependent macros #####

include config/Makefile.$(ARCH)

##### allow local macros #####

-include MyConfig.mk

##### Modules to build #####

MODULES       = build cint utils base cont meta net zip clib matrix new \
                hist tree graf g3d gpad gui minuit histpainter proof \
                treeplayer treeviewer physics postscript rint html eg

ifneq ($(ARCH),win32)
MODULES      += unix x11 x3d rootx rootd proofd
SYSTEMO       = $(UNIXO)
SYSTEMDO      = $(UNIXDO)
else
MODULES      += winnt win32 gl
SYSTEMO       = $(WINNTO)
SYSTEMDO      = $(WINNTDO)
endif
ifneq ($(TTFINCDIR),)
ifneq ($(TTFLIBDIR),)
MODULES      += x11ttf
endif
endif
ifneq ($(OPENGLINCDIR),)
ifneq ($(OPENGLULIB),)
ifneq ($(OPENGLLIB),)
MODULES      += gl
endif
endif
endif
ifneq ($(MYSQLINCDIR),)
ifneq ($(MYSQLLIBDIR),)
MODULES      += mysql
endif
endif
ifneq ($(SHIFTLIB),)
MODULES      += rfio
endif
ifneq ($(OSTHREADLIB),)
MODULES      += thread
endif
ifneq ($(FPYTHIALIB),)
MODULES      += pythia
endif
ifneq ($(FPYTHIA6LIB),)
MODULES      += pythia6
endif
ifneq ($(FVENUSLIB),)
MODULES      += venus
endif
ifneq ($(STAR),)
MODULES      += star
endif
ifneq ($(SRPDIR),)
MODULES      += srputils
endif

ifneq ($(findstring $(MAKECMDGOALS),distclean maintainer-clean),)
MODULES      += unix winnt x11 x11ttf win32 gl rfio thread pythia pythia6 \
                venus star mysql srputils x3d rootx rootd proofd
MODULES      := $(sort $(MODULES))  # removes duplicates
endif

MODULES      += main   # must be last, $(ALLLIBS) must be fully formed

##### ROOT libraries #####

LPATH         = lib

ifneq ($(ARCH),win32)
RPATH        := -L$(LPATH)
CINTLIBS     := -lCint
NEWLIBS      := -lNew
ROOTLIBS     := -lCore -lCint -lHist -lGraf -lGraf3d -lTree -lMatrix
RINTLIBS     := -lRint
PROOFLIBS    := -lGpad -lProof -lTreePlayer
else
CINTLIBS     := $(LPATH)/libCint.lib
NEWLIBS      := $(LPATH)/libNew.lib
ROOTLIBS     := $(LPATH)/libCore.lib $(LPATH)/libCint.lib \
                $(LPATH)/libHist.lib $(LPATH)/libGraf.lib \
                $(LPATH)/libGraf3d.lib $(LPATH)/libTree.lib \
                $(LPATH)/libMatrix.lib
RINTLIBS     := $(LPATH)/libRint.lib
PROOFLIBS    := $(LPATH)/libGpad.lib $(LPATH)/libProof.lib \
                $(LPATH)/libTreePlayer.lib
endif

##### f77 options #####

ifeq ($(F77LD),)
F77LD        := $(LD)
endif
ifeq ($(F77OPT),)
F77OPT       := $(OPT)
endif
ifeq ($(F77LDFLAGS),)
F77LDFLAGS   := $(LDFLAGS)
endif

##### utilities #####

MAKEDEP       = build/unix/depend.sh
MAKELIB       = build/unix/makelib.sh $(MKLIBOPTIONS)
MAKEDIST      = build/unix/makedist.sh
MAKEDISTSRC   = build/unix/makedistsrc.sh
MAKEVERSION   = build/unix/makeversion.sh
IMPORTCINT    = build/unix/importcint.sh
MAKECOMPDATA  = build/unix/compiledata.sh
MAKEMAKEINFO  = build/unix/makeinfo.sh
MAKECHANGELOG = build/unix/makechangelog.sh
MAKEHTML      = build/unix/makehtml.sh
MAKELOGHTML   = build/unix/makeloghtml.sh
MAKECINTDLLS  = build/unix/makecintdlls.sh
ifeq ($(ARCH),win32)
MAKELIB       = build/win/makelib.sh
MAKEDIST      = build/win/makedist.sh
MAKECOMPDATA  = build/win/compiledata.sh
MAKEMAKEINFO  = build/win/makeinfo.sh
endif

##### compiler directives #####

COMPILEDATA   = include/compiledata.h
MAKEINFO      = cint/MAKEINFO

##### libCore #####

COREO         = $(BASEO) $(CONTO) $(METAO) $(NETO) $(SYSTEMO) $(ZIPO) $(CLIBO)
COREDO        = $(BASEDO) $(CONTDO) $(METADO) $(NETDO) $(SYSTEMDO) $(CLIBDO)

CORELIB      := $(LPATH)/libCore.$(SOEXT)

##### if shared libs need to resolve all symbols (e.g.: aix, win32) #####

ifneq ($(EXPLICITLINK),)
MAINLIBS      = $(CORELIB) $(CINTLIB)
else
MAINLIBS      =
endif

##### all #####

ALLHDRS      :=
ALLLIBS      := $(CORELIB)
ALLEXECS     :=
INCLUDEFILES :=

##### RULES #####

.SUFFIXES: .cxx .d
.PRECIOUS: include/%.h

# special rules (need to be defined before generic ones)
cint/src/%.o: cint/src/%.cxx
	$(CXX) $(OPT) $(CINTCXXFLAGS) -o $@ -c $<

cint/src/%.o: cint/src/%.c
	$(CC) $(OPT) $(CINTCFLAGS) -o $@ -c $<

%.o: %.cxx
	$(CXX) $(OPT) $(CXXFLAGS) -o $@ -c $<

%.o: %.c
	$(CC) $(OPT) $(CFLAGS) -o $@ -c $<

%.o: %.f
ifeq ($(F77),f2c)
	f2c -a -A $<
	$(CC) $(F77OPT) $(CFLAGS) -o $@ -c $*.c
else
	$(F77) $(F77OPT) $(F77FLAGS) -o $@ -c $<
endif


##### TARGETS #####

.PHONY:         all fast config rootcint rootlibs rootexecs dist distsrc \
                clean distclean maintainer-clean compiledata importcint \
                version html changelog install showbuild cintdlls \
                debian redhat \
                $(patsubst %,all-%,$(MODULES)) \
                $(patsubst %,clean-%,$(MODULES)) \
                $(patsubst %,distclean-%,$(MODULES))

all:            rootexecs

fast:           rootexecs

include $(patsubst %,%/Module.mk,$(MODULES))

-include MyRules.mk            # allow local rules

ifeq ($(findstring $(MAKECMDGOALS),clean distclean maintainer-clean dist \
      distsrc version importcint install showbuild changelog html \
      debian redhat),)
ifeq ($(findstring $(MAKECMDGOALS),fast),)
include $(INCLUDEFILES)
endif
include build/dummy.d          # must be last include
endif


rootcint:       all-cint $(ROOTCINTTMP) $(ROOTCINT)

rootlibs:       rootcint compiledata $(ALLLIBS)

rootexecs:      rootlibs $(ALLEXECS)

compiledata:    $(COMPILEDATA) $(MAKEINFO)

config config/Makefile.:
	@(if [ ! -f config/Makefile.config ] ; then \
	   echo ""; echo "Please, run ./configure first"; echo ""; \
	   exit 1; \
	fi)

$(COMPILEDATA): config/Makefile.$(ARCH)
	@$(MAKECOMPDATA) $(COMPILEDATA) $(CXX) "$(OPT)" "$(CXXFLAGS)" \
	   "$(SOFLAGS)" "$(LDFLAGS)" "$(SOEXT)" "$(SYSLIBS)" "$(LIBDIR)" \
	   "$(ROOTLIBS)" "$(RINTLIBS)" "$(INCDIR)" "$(MAKESHAREDLIB)" \
	   "$(MAKEEXE)"

$(MAKEINFO): config/Makefile.$(ARCH)
	@$(MAKEMAKEINFO) $(MAKEINFO) $(CXX) $(CC) "$(CPPPREP)"

build/dummy.d: config $(RMKDEP) $(BINDEXP) $(ALLHDRS)
	@(if [ ! -f $@ ] ; then \
	   touch $@; \
	fi)

%.d: %.c $(RMKDEP)
	$(MAKEDEP) $@ "$(CFLAGS)" $*.c > $@

%.d: %.cxx $(RMKDEP)
	$(MAKEDEP) $@ "$(CXXFLAGS)" $*.cxx > $@

$(CORELIB): $(COREO) $(COREDO) $(CINTLIB) $(CORELIBDEP)
	@$(MAKELIB) $(PLATFORM) $(LD) "$(LDFLAGS)" \
	   "$(SOFLAGS)" libCore.$(SOEXT) $@ "$(COREO) $(COREDO)" \
	   "$(CORELIBEXTRA)"

dist:
	@$(MAKEDIST)

distsrc:
	@$(MAKEDISTSRC)

debian:
	@if [ ! -f /usr/bin/debuild -o ! -f /usr/bin/dh_testdir ]; then \
	   echo "You must have debuild and debhelper installed to"; \
	   echo "make the Debian GNU/Linux package"; exit 1; fi
	@echo "Nothing yet - sorry"

redhat:
	@if [ ! -f /bin/rpm ]; then \
	   echo "You must have rpm installed to make the Redhat package"; \
	   exit 1; fi
	@echo "Nothing yet - sorry"

clean::
	@rm -f __compiledata __makeinfo *~ core

ifeq ($(CXX),KCC)
clean::
	@find . -name "ti_files" -exec rm -rf {} \; >/dev/null 2>&1
endif
ifeq ($(SUNCC5),true)
clean::
	@find . -name "SunWS_cache" -exec rm -rf {} \; >/dev/null 2>&1
endif

distclean:: clean
	@mv -f include/config.h include/config.hh
	@rm -f include/*.h $(MAKEINFO) $(CORELIB)
	@mv -f include/config.hh include/config.h
	@rm -f build/dummy.d bin/*.dll lib/*.def lib/*.exp lib/*.lib .def
	@rm -f tutorials/*.root tutorials/*.ps tutorials/*.gif so_locations
	@rm -f tutorials/pca.C tutorials/*.so
	@rm -f $(CINTDIR)/include/*.dl* $(CINTDIR)/stl/*.dll README/ChangeLog
	@rm -rf htmldoc
	@cd test && $(MAKE) distclean

maintainer-clean:: distclean
	@rm -rf bin lib include system.rootrc config/Makefile.config \
	   test/Makefile

version: $(CINTTMP)
	@$(MAKEVERSION)

cintdlls: $(CINTTMP)
	@$(MAKECINTDLLS) $(PLATFORM) $(CINTTMP) $(MAKELIB) $(CXX) \
	   $(CC) $(LD) "$(OPT)" "$(CINTCXXFLAGS)" "$(CINTCFLAGS)" \
	   "$(LDFLAGS)" "$(SOFLAGS)" "$(SOEXT)"

importcint: distclean-cint
	@$(IMPORTCINT)

changelog:
	@$(MAKECHANGELOG)

html: $(ROOTEXE) changelog
	@$(MAKELOGHTML)
	@$(MAKEHTML)

install:
	@if [ -d $(BINDIR) ]; then \
	   inode1=`ls -id $(BINDIR) | awk '{ print $$1 }'`; \
	fi; \
	inode2=`ls -id $$(pwd)/bin | awk '{ print $$1 }'`; \
	if [ -d $(BINDIR) ] && [ $$inode1 -eq $$inode2 ]; then \
	   echo "Everything already installed..."; \
	else \
	   echo "Installing binaries in $(DESTDIR)$(BINDIR)"; \
	   $(INSTALLDIR)                        $(DESTDIR)$(BINDIR); \
	   $(INSTALL) $(CINT)                   $(DESTDIR)$(BINDIR); \
	   $(INSTALL) $(MAKECINT)               $(DESTDIR)$(BINDIR); \
	   $(INSTALL) $(ROOTCINT)               $(DESTDIR)$(BINDIR); \
	   $(INSTALL) $(RMKDEP)                 $(DESTDIR)$(BINDIR); \
	   if [ "x$(BINDEXP)" != "x" ] ; then \
	      $(INSTALL) $(BINDEXP)             $(DESTDIR)$(BINDIR); \
           fi; \
	   $(INSTALL) bin/root-config           $(DESTDIR)$(BINDIR); \
	   $(INSTALL) $(ALLEXECS)               $(DESTDIR)$(BINDIR); \
	   echo "Installing libraries in $(DESTDIR)$(LIBDIR)"; \
	   $(INSTALLDIR)                        $(DESTDIR)$(LIBDIR); \
	   chmod u+w                            $(DESTDIR)$(LIBDIR)/*; \
	   echo "[possible error from chmod is ok]"; \
	   for lib in $(ALLLIBS) $(CINTLIB); do \
	      $(INSTALL) $$lib*                 $(DESTDIR)$(LIBDIR); \
	   done ; \
	   echo "Installing headers in $(DESTDIR)$(INCDIR)"; \
	   $(INSTALLDIR)                        $(DESTDIR)$(INCDIR); \
	   $(INSTALLDATA) include/*.h           $(DESTDIR)$(INCDIR); \
	   echo "Installing main/src/rmain.cxx in $(DESTDIR)$(INCDIR)"; \
	   $(INSTALLDATA) main/src/rmain.cxx    $(DESTDIR)$(INCDIR); \
	   echo "Installing $(MAKEINFO) in $(DESTDIR)$(CINTINCDIR)"; \
	   $(INSTALLDIR)                        $(DESTDIR)$(CINTINCDIR); \
	   $(INSTALLDATA) $(MAKEINFO)           $(DESTDIR)$(CINTINCDIR); \
	   echo "Installing cint/include cint/lib and cint/stl in $(DESTDIR)$(CINTINCDIR)"; \
	   $(INSTALLDATA) cint/include          $(DESTDIR)$(CINTINCDIR); \
	   $(INSTALLDATA) cint/lib              $(DESTDIR)$(CINTINCDIR); \
	   $(INSTALLDATA) cint/stl              $(DESTDIR)$(CINTINCDIR); \
	   echo "Installing PROOF files in $(DESTDIR)$(PROOFDATADIR)"; \
	   $(INSTALLDIR)                        $(DESTDIR)$(PROOFDATADIR); \
	   $(INSTALLDATA) proof/etc             $(DESTDIR)$(PROOFDATADIR); \
	   $(INSTALLDATA) proof/utils           $(DESTDIR)$(PROOFDATADIR); \
	   rm -rf $(DESTDIR)$(PROOFDATADIR)/etc/CVS; \
	   rm -rf $(DESTDIR)$(PROOFDATADIR)/utils/CVS; \
	   echo "Installing icons in $(DESTDIR)$(ICONPATH)"; \
	   $(INSTALLDIR)                        $(DESTDIR)$(ICONPATH); \
	   $(INSTALLDATA) icons/*.xpm           $(DESTDIR)$(ICONPATH); \
	   echo "Installing misc docs in  $(DESTDIR)$(DOCDIR)" ; \
	   $(INSTALLDIR)                        $(DESTDIR)$(DOCDIR); \
	   $(INSTALLDATA) LICENSE               $(DESTDIR)$(DOCDIR); \
	   $(INSTALLDATA) README/README         $(DESTDIR)$(DOCDIR); \
	   $(INSTALLDATA) README/README.PROOF   $(DESTDIR)$(DOCDIR); \
	   $(INSTALLDATA) README/ChangeLog-2-24 $(DESTDIR)$(DOCDIR); \
	   $(INSTALLDATA) README/CREDITS        $(DESTDIR)$(DOCDIR); \
	   echo "Installing tutorials in $(DESTDIR)$(TUTDIR)"; \
	   $(INSTALLDIR)                        $(DESTDIR)$(TUTDIR); \
	   $(INSTALLDATA) tutorials/*           $(DESTDIR)$(TUTDIR); \
	   echo "Installing tests in $(DESTDIR)$(TESTDIR)"; \
	   $(INSTALLDIR)                        $(DESTDIR)$(TESTDIR); \
	   $(INSTALLDATA) test/*                $(DESTDIR)$(TESTDIR); \
	   echo "Installing macros in $(DESTDIR)$(MACRODIR)"; \
	   $(INSTALLDIR)                        $(DESTDIR)$(MACRODIR); \
	   $(INSTALLDATA) macros/*              $(DESTDIR)$(MACRODIR); \
	   echo "Installing man(1) pages in $(DESTDIR)$(MANDIR)"; \
	   $(INSTALLDIR)                        $(DESTDIR)$(MANDIR); \
	   $(INSTALLDATA) man/*                 $(DESTDIR)$(MANDIR); \
	   echo "Installing config files in $(DESTDIR)$(ETCDIR)"; \
	   $(INSTALLDIR)                        $(DESTDIR)$(ETCDIR); \
	   $(INSTALLDATA) etc/*                 $(DESTDIR)$(ETCDIR); \
	   echo "Installing utils in $(DESTDIR)$(DATADIR)"; \
	   $(INSTALLDIR)                        $(DESTDIR)$(DATADIR); \
	   $(INSTALLDATA) build/misc/*          $(DESTDIR)$(DATADIR); \
	fi

showbuild:
	@echo "ROOTSYS            = $(ROOTSYS)"
	@echo "PLATFORM           = $(PLATFORM)"
	@echo "OPT                = $(OPT)"
	@echo ""
	@echo "CXX                = $(CXX)"
	@echo "CC                 = $(CC)"
	@echo "F77                = $(F77)"
	@echo "CPP                = $(CPP)"
	@echo "LD                 = $(LD)"
	@echo "F77LD              = $(F77LD)"
	@echo ""
	@echo "CXXFLAGS           = $(CXXFLAGS)"
	@echo "CINTCXXFLAGS       = $(CINTCXXFLAGS)"
	@echo "EXTRA_CXXFLAGS     = $(EXTRA_CXXFLAGS)"
	@echo "CFLAGS             = $(CFLAGS)"
	@echo "CINTCFLAGS         = $(CINTCFLAGS)"
	@echo "EXTRA_CFLAGS       = $(EXTRA_CFLAGS)"
	@echo "F77FLAGS           = $(F77FLAGS)"
	@echo "LDFLAGS            = $(LDFLAGS)"
	@echo "EXTRA_LDFLAGS      = $(EXTRA_LDFLAGS)"
	@echo "SOFLAGS            = $(SOFLAGS)"
	@echo "SOEXT              = $(SOEXT)"
	@echo ""
	@echo "SYSLIBS            = $(SYSLIBS)"
	@echo "XLIBS              = $(XLIBS)"
	@echo "CILIBS             = $(CILIBS)"
	@echo "F77LIBS            = $(F77LIBS)"
	@echo ""
	@echo "FPYTHIALIBDIR      = $(FPYTHIALIBDIR)"
	@echo "FPYTHIA6LIBDIR     = $(FPYTHIA6LIBDIR)"
	@echo "FVENUSLIBDIR       = $(FVENUSLIBDIR)"
	@echo "STAR               = $(STAR)"
	@echo "XPMLIBDIR          = $(XPMLIBDIR)"
	@echo "TTFLIBDIR          = $(TTFLIBDIR)"
	@echo "TTFINCDIR          = $(TTFINCDIR)"
	@echo "TTFFONTDIR         = $(TTFFONTDIR)"
	@echo "OPENGLULIB         = $(OPENGLULIB)"
	@echo "OPENGLLIB          = $(OPENGLLIB)"
	@echo "OPENGLINCDIR       = $(OPENGLINCDIR)"
	@echo "CERNLIBDIR         = $(CERNLIBDIR)"
	@echo "OSTHREADLIB        = $(OSTHREADLIB)"
	@echo "SHIFTLIB           = $(SHIFTLIB)"
	@echo "MYSQLINCDIR        = $(MYSQLINCDIR)"
	@echo "SRPDIR             = $(SRPDIR)"
	@echo "AFSDIR             = $(AFSDIR)"
	@echo ""
	@echo "INSTALL            = $(INSTALL)"
	@echo "MAKEDEP            = $(MAKEDEP)"
	@echo "MAKELIB            = $(MAKELIB)"
	@echo "MAKEDIST           = $(MAKEDIST)"
	@echo "MAKEDISTSRC        = $(MAKEDISTSRC)"
	@echo "MAKEVERSION        = $(MAKEVERSION)"
	@echo "IMPORTCINT         = $(IMPORTCINT)"
