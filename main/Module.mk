# Module.mk for main module
# Copyright (c) 2000 Rene Brun and Fons Rademakers
#
# Author: Fons Rademakers, 29/2/2000

MODDIR       := main
MODDIRS      := $(MODDIR)/src
MODDIRI      := $(MODDIR)/inc

MAINDIR      := $(MODDIR)
MAINDIRS     := $(MAINDIR)/src
MAINDIRI     := $(MAINDIR)/inc
MAINDIRW     := $(MAINDIR)/win32

##### root.exe #####
ROOTEXES     := $(MODDIRS)/rmain.cxx
ROOTEXEO     := $(ROOTEXES:.cxx=.o)
ROOTEXEDEP   := $(ROOTEXEO:.o=.d)
ifeq ($(ARCH),win32gcc)
ROOTEXE      := bin/root_exe.exe
else
ROOTEXE      := bin/root.exe
endif
ROOTNEXE     := bin/rootn.exe
ifeq ($(PLATFORM),win32)
ROOTICON     := icons/RootIcon.obj
endif

##### proofserv #####
PROOFSERVS   := $(MODDIRS)/pmain.cxx
PROOFSERVO   := $(PROOFSERVS:.cxx=.o)
PROOFSERVDEP := $(PROOFSERVO:.o=.d)
PROOFSERV    := bin/proofserv$(EXEEXT)

##### hadd #####
HADDS        := $(MODDIRS)/hadd.cxx
HADDO        := $(HADDS:.cxx=.o)
HADDDEP      := $(HADDO:.o=.d)
HADD         := bin/hadd$(EXEEXT)

##### h2root #####
H2ROOTS1     := $(MODDIRS)/h2root.cxx
H2ROOTS2     := $(HBOOKS2)
# Symbols in cfopei.obj is already provided in packmd.lib,
#H2ROOTS3    := $(wildcard $(MAINDIRW)/*.c)
H2ROOTS3     := $(filter-out $(MAINDIRW)/cfopei.c, $(wildcard $(MAINDIRW)/*.c))
H2ROOTS4     := $(MAINDIRW)/tzvers.f
H2ROOTO      := $(H2ROOTS1:.cxx=.o) $(H2ROOTS2:.f=.o)
ifeq ($(PLATFORM),win32)
H2ROOTO      += $(H2ROOTS3:.c=.o) $(H2ROOTS4:.f=.o)
endif
H2ROOTDEP    := $(H2ROOTS1:.cxx=.d)
H2ROOT       := bin/h2root$(EXEEXT)

##### g2root #####
G2ROOTS      := $(MODDIRS)/g2root.f
G2ROOTO      := $(G2ROOTS:.f=.o)
ifeq ($(PLATFORM),win32)
G2ROOTO      += $(H2ROOTS3:.c=.o) $(H2ROOTS4:.f=.o)
endif
G2ROOT       := bin/g2root$(EXEEXT)

##### g2rootold #####
G2ROOTOLDS      := $(MODDIRS)/g2rootold.f
G2ROOTOLDO      := $(G2ROOTOLDS:.f=.o)
ifeq ($(PLATFORM),win32)
G2ROOTOLDO      += $(H2ROOTS3:.c=.o) $(H2ROOTS4:.f=.o)
endif
G2ROOTOLD       := bin/g2rootold$(EXEEXT)

##### ssh2rpd #####
SSH2RPDS        := $(MODDIRS)/ssh2rpd.cxx
SSH2RPDO        := $(SSH2RPDS:.cxx=.o)
SSH2RPDDEP      := $(SSH2RPDO:.o=.d)
SSH2RPD         := bin/ssh2rpd$(EXEEXT)

# used in the main Makefile
ALLEXECS     += $(ROOTEXE) $(ROOTNEXE) $(PROOFSERV) $(HADD)
ifneq ($(PLATFORM),win32)
ALLEXECS     += $(SSH2RPD)
endif
ifneq ($(CERNLIBS),)
ALLEXECS     += $(H2ROOT)
ifneq ($(PLATFORM),win32)
ALLEXECS     += $(G2ROOT) $(G2ROOTOLD)
endif
endif

# include all dependency files
INCLUDEFILES += $(ROOTEXEDEP) $(PROOFSERVDEP) $(HADDDEP) $(H2ROOTDEP) \
                $(SSH2RPDDEP)

##### local rules #####
$(ROOTEXE):     $(ROOTEXEO) $(CORELIB) $(CINTLIB) $(HISTLIB) \
                $(GRAFLIB) $(G3DLIB) $(TREELIB) $(MATRIXLIB) $(RINTLIB)
		$(LD) $(LDFLAGS) -o $@ $(ROOTEXEO) $(ROOTICON) $(ROOTULIBS) \
		   $(RPATH) $(ROOTLIBS) $(RINTLIBS) $(SYSLIBS)

$(ROOTNEXE):    $(ROOTEXEO) $(NEWLIB) $(CORELIB) $(CINTLIB) $(HISTLIB) \
                $(GRAFLIB) $(G3DLIB) $(TREELIB) $(MATRIXLIB) $(RINTLIB)
		$(LD) $(LDFLAGS) -o $@ $(ROOTEXEO) $(ROOTICON) $(ROOTULIBS) \
		   $(RPATH) $(NEWLIBS) $(ROOTLIBS) $(RINTLIBS) $(SYSLIBS)

$(PROOFSERV):   $(PROOFSERVO) $(CORELIB) $(CINTLIB) $(HISTLIB) \
                $(GRAFLIB) $(G3DLIB) $(TREELIB) $(MATRIXLIB) $(PROOFLIB) \
                $(TREEPLAYERLIB)
		$(LD) $(LDFLAGS) -o $@ $(PROOFSERVO) $(ROOTULIBS) \
		   $(RPATH) $(ROOTLIBS) $(PROOFLIBS) $(SYSLIBS)

$(HADD):        $(HADDO) $(CORELIB) $(CINTLIB) $(HISTLIB) \
                $(GRAFLIB) $(G3DLIB) $(TREELIB) $(MATRIXLIB)
		$(LD) $(LDFLAGS) -o $@ $(HADDO) $(RPATH) $(ROOTLIBS) $(SYSLIBS)

$(SSH2RPD):     $(SSH2RPDO) $(SNPRINTFO)
		$(LD) $(LDFLAGS) -o $@ $(SSH2RPDO) $(SNPRINTFO) $(SYSLIBS)

$(H2ROOT):      $(H2ROOTO) $(CORELIB) $(CINTLIB) $(HISTLIB) \
                $(GRAFLIB) $(G3DLIB) $(TREELIB) $(MATRIXLIB)
		$(LD) $(LDFLAGS) -o $@ $(H2ROOTO) \
		   $(RPATH) $(ROOTLIBS) \
		   $(CERNLIBDIR) $(CERNLIBS) $(RFIOLIBEXTRA) $(SHIFTLIBDIR) \
		   $(SHIFTLIB) $(F77LIBS) $(SYSLIBS)

$(G2ROOT):      $(G2ROOTO)
		$(F77LD) $(F77LDFLAGS) -o $@ $(G2ROOTO) \
		   $(CERNLIBDIR) $(CERNLIBS) $(RFIOLIBEXTRA) $(SHIFTLIBDIR) \
		   $(SHIFTLIB) $(F77LIBS) $(SYSLIBS)

$(G2ROOTOLD):   $(G2ROOTOLDO)
		$(F77LD) $(F77LDFLAGS) -o $@ $(G2ROOTOLDO) \
		   $(CERNLIBDIR) $(CERNLIBS) $(RFIOLIBEXTRA) $(SHIFTLIBDIR) \
		   $(SHIFTLIB) $(F77LIBS) $(SYSLIBS)

ifneq ($(CERNLIBS),)
ifneq ($(PLATFORM),win32)
all-main:      $(ROOTEXE) $(ROOTNEXE) $(PROOFSERV) $(HADD) $(H2ROOT) $(G2ROOT) \
               $(G2ROOTOLD) $(SSH2RPD)
else
all-main:      $(ROOTEXE) $(ROOTNEXE) $(PROOFSERV) $(HADD) $(H2ROOT) $(SSH2RPD)
endif
else
all-main:      $(ROOTEXE) $(ROOTNEXE) $(PROOFSERV) $(HADD) $(SSH2RPD)
endif

clean-main:
		@rm -f $(ROOTEXEO) $(PROOFSERVO) $(HADDO) $(H2ROOTO) \
		   $(G2ROOTO) $(G2ROOTOLDO) $(SSH2RPDO)

clean::         clean-main

distclean-main: clean-main
		@rm -f $(ROOTEXEDEP) $(ROOTEXE) $(ROOTNEXE) $(PROOFSERVDEP) \
		   $(PROOFSERV) $(HADDDEP) $(HADD) $(H2ROOTDEP) $(H2ROOT) \
		   $(G2ROOT) $(G2ROOTOLD) $(SSH2RPDDEP) $(SSH2RPD)

distclean::     distclean-main
