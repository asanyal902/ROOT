# Module.mk for rint module
# Copyright (c) 2000 Rene Brun and Fons Rademakers
#
# Author: Fons Rademakers, 29/2/2000

MODDIR       := $(SRCDIR)/rint
MODDIRS      := $(MODDIR)/src
MODDIRI      := $(MODDIR)/inc

RINTDIR      := $(MODDIR)
RINTDIRS     := $(RINTDIR)/src
RINTDIRI     := $(RINTDIR)/inc

##### libRint #####
RINTL        := $(MODDIRI)/LinkDef.h
RINTDS       := $(subst $(SRCDIR)/,,$(MODDIRS))/G__Rint.cxx
RINTDO       := $(RINTDS:.cxx=.o)
RINTDH       := $(RINTDS:.cxx=.h)

RINTH        := $(filter-out $(MODDIRI)/LinkDef%,$(wildcard $(MODDIRI)/*.h))
RINTS        := $(filter-out $(MODDIRS)/G__%,$(wildcard $(MODDIRS)/*.cxx))
RINTO        := $(subst $(SRCDIR)/,,$(RINTS:.cxx=.o))

RINTDEP      := $(RINTO:.o=.d) $(RINTDO:.o=.d)

RINTLIB      := $(LPATH)/libRint.$(SOEXT)
RINTMAP      := $(RINTLIB:.$(SOEXT)=.rootmap)

# used in the main Makefile
ALLHDRS     += $(patsubst $(MODDIRI)/%.h,include/%.h,$(RINTH))
ALLLIBS     += $(RINTLIB)
ALLMAPS     += $(RINTMAP)

# include all dependency files
INCLUDEFILES += $(RINTDEP)

##### local rules #####
include/%.h:    $(RINTDIRI)/%.h
		cp $< $@

$(RINTLIB):     $(RINTO) $(RINTDO) $(ORDER_) $(MAINLIBS)
		@$(MAKELIB) $(PLATFORM) $(LD) "$(LDFLAGS)" \
		   "$(SOFLAGS)" libRint.$(SOEXT) $@ "$(RINTO) $(RINTDO)" \
		   "$(RINTLIBEXTRA)"

$(RINTDS):      $(RINTH) $(RINTL) $(ROOTCINTTMPEXE)
		@echo "Generating dictionary $@..."
		$(ROOTCINTTMP) -f $@ -c $(RINTH) $(RINTL)

$(RINTMAP):     $(RLIBMAP) $(MAKEFILEDEP) $(RINTL)
		$(RLIBMAP) -o $(RINTMAP) -l $(RINTLIB) \
		   -d $(RINTLIBDEPM) -c $(RINTL)

all-rint:       $(RINTLIB) $(RINTMAP)

clean-rint:
		@rm -f $(RINTO) $(RINTDO)

clean::         clean-rint

distclean-rint: clean-rint
		@rm -f $(RINTDEP) $(RINTDS) $(RINTDH) $(RINTLIB) $(RINTMAP)

distclean::     distclean-rint
