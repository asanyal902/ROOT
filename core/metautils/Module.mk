# Module.mk for utilities for libMeta and rootcint
# Copyright (c) 2002 Rene Brun and Fons Rademakers
#
# Author: Philippe Canal 9/1/2004

MODNAME        := metautils
MODDIR         := core/$(MODNAME)
MODDIRS        := $(MODDIR)/src
MODDIRI        := $(MODDIR)/inc

METAUTILSDIR   := $(MODDIR)
METAUTILSDIRS  := $(METAUTILSDIR)/src
METAUTILSDIRI  := $(METAUTILSDIR)/inc

##### $(METAUTILSO) #####
METAUTILSH     := $(filter-out $(MODDIRI)/LinkDef%,$(wildcard $(MODDIRI)/*.h))
METAUTILSS     := $(filter-out $(MODDIRS)/G__%,$(wildcard $(MODDIRS)/*.cxx))
METAUTILSO     := $(METAUTILSS:.cxx=.o)

METAUTILSL     := $(MODDIRI)/LinkDef.h
METAUTILSDS    := $(MODDIRS)/G__MetaUtils.cxx
METAUTILSDO    := $(METAUTILSDS:.cxx=.o)
METAUTILSDH    := $(METAUTILSDS:.cxx=.h)

METAUTILSDEP   := $(METAUTILSO:.o=.d) $(METAUTILSDO:.o=.d)

# used in the main Makefile
ALLHDRS     += $(patsubst $(MODDIRI)/%.h,include/%.h,$(METAUTILSH))

# include all dependency files
INCLUDEFILES += $(METAUTILSDEP)

##### local rules #####
.PHONY:         all-$(MODNAME) clean-$(MODNAME) distclean-$(MODNAME)

include/%.h:    $(METAUTILSDIRI)/%.h
		cp $< $@

# $(ROOTCINTTMP) not yet known at this stage, use explicit path of rootcint_tmp
$(METAUTILSDS): $(METAUTILSH) $(METAUTILSL) core/utils/src/rootcint_tmp.o $(ORDER_) core/utils/src/rootcint_tmp$(EXEEXT)
		@echo "Generating dictionary $@..."
		$(ROOTCINTTMP) -f $@ -c -DG__API $(METAUTILSH) $(METAUTILSL)

all-$(MODNAME): $(METAUTILSO) $(METAUTILSDO)

clean-$(MODNAME):
		@rm -f $(METAUTILSO) $(METAUTILSDO)

clean::         clean-$(MODNAME)

distclean-$(MODNAME): clean-$(MODNAME)
		@rm -f $(METAUTILSDEP) $(METAUTILSDS) $(METAUTILSDH)

distclean::     distclean-$(MODNAME)

##### extra rules ######
$(METAUTILSO):  PCHCXXFLAGS =
