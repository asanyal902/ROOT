# Module.mk for peac module
# Copyright (c) 2004 Rene Brun and Fons Rademakers
#
# Author: Maarten Ballintijn 18/10/2004

MODDIR       := peac
MODDIRS      := $(MODDIR)/src
MODDIRI      := $(MODDIR)/inc

PEACDIR      := $(MODDIR)
PEACDIRS     := $(PEACDIR)/src
PEACDIRI     := $(PEACDIR)/inc

##### libPeac #####
PEACL        := $(MODDIRI)/LinkDef.h
PEACDS       := $(MODDIRS)/G__Peac.cxx
PEACDO       := $(PEACDS:.cxx=.o)
PEACDH       := $(PEACDS:.cxx=.h)

PEACH        := $(filter-out $(MODDIRI)/LinkDef%,$(wildcard $(MODDIRI)/*.h))
PEACS        := $(filter-out $(MODDIRS)/G__%,$(wildcard $(MODDIRS)/*.cxx))
PEACO        := $(PEACS:.cxx=.o)

PEACDEP      := $(PEACO:.o=.d) $(PEACDO:.o=.d)

PEACLIB      := $(LPATH)/libPeac.$(SOEXT)
PEACMAP      := $(PEACLIB:.$(SOEXT)=.rootmap)

##### libPeacGui #####
PEACGUIL     := $(MODDIRI)/LinkDefGui.h
PEACGUIDS    := $(MODDIRS)/G__PeacGui.cxx
PEACGUIDO    := $(PEACGUIDS:.cxx=.o)
PEACGUIDH    := $(PEACGUIDS:.cxx=.h)

PEACGUIH     := $(MODDIRI)/TProofStartupDialog.h
PEACGUIS     := $(MODDIRS)/TProofStartupDialog.cxx
PEACGUIO     := $(PEACGUIS:.cxx=.o)

PEACGUIDEP   := $(PEACGUIO:.o=.d) $(PEACGUIDO:.o=.d)

PEACGUILIB   := $(LPATH)/libPeacGui.$(SOEXT)
PEACGUIMAP   := $(PEACGUILIB:.$(SOEXT)=.rootmap)

# remove GUI files from PEAC files
PEACH        := $(filter-out $(PEACGUIH),$(PEACH))
PEACS        := $(filter-out $(PEACGUIS),$(PEACS))
PEACO        := $(filter-out $(PEACGUIO),$(PEACO))
PEACDEP      := $(filter-out $(PEACGUIDEP),$(PEACDEP))

# used in the main Makefile
ALLHDRS     += $(patsubst $(MODDIRI)/%.h,include/%.h,$(PEACH))
ALLHDRS     += $(patsubst $(MODDIRI)/%.h,include/%.h,$(PEACGUIH))
ALLLIBS     += $(PEACLIB) $(PEACGUILIB)
ALLMAPS     += $(PEACMAP) $(PEACGUIMAP)

# include all dependency files
INCLUDEFILES += $(PEACDEP) $(PEACGUIDEP)

##### local rules #####
include/%.h:    $(PEACDIRI)/%.h
		cp $< $@

$(PEACLIB):     $(PEACO) $(PEACDO) $(ORDER_) $(MAINLIBS) $(PEACLIBDEP)
		@$(MAKELIB) $(PLATFORM) $(LD) "$(LDFLAGS)" \
		   "$(SOFLAGS)" libPeac.$(SOEXT) $@ "$(PEACO) $(PEACDO)" \
		   "$(PEACLIBEXTRA)"

$(PEACDS):      $(PEACH) $(PEACL) $(PEACO) $(ROOTCINTNEW)
		@echo "Generating dictionary $@..."
		$(ROOTCINTNEW) -f $@ -o "$(PEACO)" -c $(PEACH) $(PEACL)

$(PEACMAP):     $(RLIBMAP) $(MAKEFILEDEP) $(PEACL)
		$(RLIBMAP) -o $(PEACMAP) -l $(PEACLIB) \
		   -d $(PEACLIBDEPM) -c $(PEACL)

$(PEACGUILIB):  $(PEACGUIO) $(PEACGUIDO) $(ORDER_) $(MAINLIBS) $(PEACGUILIBDEP)
		@$(MAKELIB) $(PLATFORM) $(LD) "$(LDFLAGS)" \
		   "$(SOFLAGS)" libPeacGui.$(SOEXT) $@ \
		   "$(PEACGUIO) $(PEACGUIDO)" \
		   "$(PEACGUILIBEXTRA)"

$(PEACGUIDS):   $(PEACGUIH) $(PEACGUIL) $(PEACGUIO) $(ROOTCINTNEW)
		@echo "Generating dictionary $@..."
		$(ROOTCINTNEW) -f $@ -o "$(PEACGUIO)" -c $(PEACGUIH) $(PEACGUIL)

$(PEACGUIMAP):  $(RLIBMAP) $(MAKEFILEDEP) $(PEACGUIL)
		$(RLIBMAP) -o $(PEACGUIMAP) -l $(PEACGUILIB) \
		   -d $(PEACGUILIBDEPM) -c $(PEACGUIL)

all-peac:       $(PEACLIB) $(PEACGUILIB) $(PEACMAP) $(PEACGUIMAP)

clean-peac:
		@rm -f $(PEACO) $(PEACDO) $(PEACGUIO) $(PEACGUIDO)

clean::         clean-peac

distclean-peac: clean-peac
		@rm -f $(PEACDEP) $(PEACDS) $(PEACDH) $(PEACLIB) \
		   $(PEACGUIDEP) $(PEACGUIDS) $(PEACGUIDH) $(PEACGUILIB) \
		   $(PEACMAP) $(PEACGUIMAP)

distclean::     distclean-peac

##### extra rules ######
$(PEACO):       CXXFLAGS += $(CLARENSINC)
