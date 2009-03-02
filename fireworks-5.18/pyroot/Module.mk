# Module.mk for pyroot module
# Copyright (c) 2004 Rene Brun and Fons Rademakers
#
# Authors: Pere Mato, Wim Lavrijsen, 22/4/2004

MODDIR       := pyroot
MODDIRS      := $(MODDIR)/src
MODDIRI      := $(MODDIR)/inc

PYROOTDIR    := $(MODDIR)
PYROOTDIRS   := $(PYROOTDIR)/src
PYROOTDIRI   := $(PYROOTDIR)/inc

##### libPyROOT #####
PYROOTL      := $(MODDIRI)/LinkDef.h
PYROOTDS     := $(MODDIRS)/G__PyROOT.cxx
PYROOTDO     := $(PYROOTDS:.cxx=.o)
PYROOTDH     := $(PYROOTDS:.cxx=.h)

PYROOTH      := $(filter-out $(MODDIRI)/LinkDef%,$(wildcard $(MODDIRI)/*.h))
PYROOTS      := $(filter-out $(MODDIRS)/G__%,$(wildcard $(MODDIRS)/*.cxx))
PYROOTO      := $(PYROOTS:.cxx=.o)

PYROOTDEP    := $(PYROOTO:.o=.d) $(PYROOTDO:.o=.d)

PYROOTLIB    := $(LPATH)/libPyROOT.$(SOEXT)
ifeq ($(ARCH),win32)
PYROOTPYD    := bin/$(notdir $(PYROOTLIB:.$(SOEXT)=.pyd))
endif
PYROOTMAP    := $(PYROOTLIB:.$(SOEXT)=.rootmap)

ROOTPYS      := $(wildcard $(MODDIR)/*.py)
ifeq ($(PLATFORM),win32)
ROOTPY       := $(subst $(MODDIR),bin,$(ROOTPYS))
bin/%.py: $(MODDIR)/%.py; cp $< $@
else
ROOTPY       := $(subst $(MODDIR),$(LPATH),$(ROOTPYS))
$(LPATH)/%.py: $(MODDIR)/%.py; cp $< $@
endif
ROOTPYC      := $(ROOTPY:.py=.pyc)
ROOTPYO      := $(ROOTPY:.py=.pyo)

# used in the main Makefile
ALLHDRS     += $(patsubst $(MODDIRI)/%.h,include/%.h,$(PYROOTH))
ALLLIBS     += $(PYROOTLIB)
ALLMAPS     += $(PYROOTMAP)

# include all dependency files
INCLUDEFILES += $(PYROOTDEP)

##### local rules #####
include/%.h:    $(PYROOTDIRI)/%.h
		cp $< $@

%.pyc: %.py;    python -c 'import py_compile; py_compile.compile( "$<" )'
%.pyo: %.py;    python -O -c 'import py_compile; py_compile.compile( "$<" )'

$(PYROOTLIB):   $(PYROOTO) $(PYROOTDO) $(ROOTPY) $(ROOTPYC) $(ROOTPYO) \
                $(ROOTLIBSDEP)
		@$(MAKELIB) $(PLATFORM) $(LD) "$(LDFLAGS)" \
		  "$(SOFLAGS)" libPyROOT.$(SOEXT) $@ \
		  "$(PYROOTO) $(PYROOTDO)" \
		  "$(ROOTULIBS) $(RPATH) $(ROOTLIBS) \
		   $(PYTHONLIBDIR) $(PYTHONLIB) \
		   $(OSTHREADLIBDIR) $(OSTHREADLIB)" "$(PYTHONLIBFLAGS)"
ifeq ($(ARCH),win32)
		cp -f bin/$(notdir $@) $(PYROOTPYD)
endif

$(PYROOTDS):    $(PYROOTH) $(PYROOTL) $(ROOTCINTTMPEXE)
		@echo "Generating dictionary $@..."
		$(ROOTCINTTMP) -f $@ -c $(PYROOTH) $(PYROOTL)

$(PYROOTMAP):   $(RLIBMAP) $(MAKEFILEDEP) $(PYROOTL)
		$(RLIBMAP) -o $(PYROOTMAP) -l $(PYROOTLIB) \
		   -d $(PYROOTLIBDEPM) -c $(PYROOTL)

all-pyroot:     $(PYROOTLIB) $(PYROOTMAP)

clean-pyroot:
		@rm -f $(PYROOTO) $(PYROOTDO)

clean::         clean-pyroot

distclean-pyroot: clean-pyroot
		@rm -f $(PYROOTDEP) $(PYROOTDS) $(PYROOTDH) $(PYROOTLIB) \
		   $(ROOTPY) $(ROOTPYC) $(ROOTPYO) $(PYROOTMAP) \
		   $(PYROOTPYD)

distclean::     distclean-pyroot

##### extra rules ######
# Require Reflex support?
ifeq ($(BUILDREFLEX),yes)
$(PYROOTO): CXXFLAGS += -DPYROOT_USE_REFLEX $(PYTHONINCDIR:%=-I%)
$(PYROOTLIB): ROOTLIBS += $(RFLX_REFLEXLL)
$(PYROOTLIB): $(LPATH)/libReflex.$(SOEXT)
else
$(PYROOTO): CXXFLAGS += $(PYTHONINCDIR:%=-I%)
endif
