# Module.mk for castor module
# Copyright (c) 2000 Rene Brun and Fons Rademakers
#
# Author: Fons Rademakers, 29/2/2000

MODDIR       := castor
MODDIRS      := $(MODDIR)/src
MODDIRI      := $(MODDIR)/inc

CASTORDIR    := $(MODDIR)
CASTORDIRS   := $(CASTORDIR)/src
CASTORDIRI   := $(CASTORDIR)/inc

##### libRCastor #####
CASTORL      := $(MODDIRI)/LinkDef.h
CASTORDS     := $(MODDIRS)/G__CASTOR.cxx
CASTORDO     := $(CASTORDS:.cxx=.o)
CASTORDH     := $(CASTORDS:.cxx=.h)

CASTORH      := $(filter-out $(MODDIRI)/LinkDef%,$(wildcard $(MODDIRI)/*.h))
CASTORS      := $(filter-out $(MODDIRS)/G__%,$(wildcard $(MODDIRS)/*.cxx))
CASTORO      := $(CASTORS:.cxx=.o)

CASTORDEP    := $(CASTORO:.o=.d) $(CASTORDO:.o=.d)

CASTORLIB    := $(LPATH)/libRCastor.$(SOEXT)
CASTORMAP    := $(CASTORLIB:.$(SOEXT)=.rootmap)

# used in the main Makefile
ALLHDRS     += $(patsubst $(MODDIRI)/%.h,include/%.h,$(CASTORH))
ALLLIBS     += $(CASTORLIB)
ALLMAPS     += $(CASTORMAP)

# include all dependency files
INCLUDEFILES += $(CASTORDEP)

##### local rules #####
include/%.h:    $(CASTORDIRI)/%.h
		cp $< $@

$(CASTORLIB):   $(CASTORO) $(CASTORDO) $(ORDER_) $(MAINLIBS) $(CASTORLIBDEP)
		@$(MAKELIB) $(PLATFORM) $(LD) "$(LDFLAGS)" \
		   "$(SOFLAGS)" libRCastor.$(SOEXT) $@ \
		   "$(CASTORO) $(CASTORDO)" \
		   "$(CASTORLIBEXTRA) $(CASTORLIBDIR) $(CASTORCLILIB)"

$(CASTORDS):    $(CASTORH) $(CASTORL) $(ROOTCINTTMPEXE)
	@echo "Generating dictionary $@..."
		$(ROOTCINTTMP) -f $@ -c $(CASTORH) $(CASTORL)

$(CASTORMAP):   $(RLIBMAP) $(MAKEFILEDEP) $(CASTORL)
		$(RLIBMAP) -o $(CASTORMAP) -l $(CASTORLIB) \
		   -d $(CASTORLIBDEPM) -c $(CASTORL)

all-castor:     $(CASTORLIB) $(CASTORMAP)

clean-castor:
		@rm -f $(CASTORO) $(CASTORDO)

clean::         clean-castor

distclean-castor: clean-castor
		@rm -f $(CASTORDEP) $(CASTORDS) $(CASTORDH) $(CASTORLIB) $(CASTORMAP)

distclean::     distclean-castor

##### extra rules ######
ifeq ($(PLATFORM),win32)
$(CASTORO): CXXFLAGS += $(CASTORCFLAGS) $(CASTORINCDIR:%=-I%) -DNOGDI -D__INSIDE_CYGWIN__
else
$(CASTORO): CXXFLAGS += $(CASTORCFLAGS) $(CASTORINCDIR:%=-I%)
endif
