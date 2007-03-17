# Module.mk for proof module
# Copyright (c) 2000 Rene Brun and Fons Rademakers
#
# Author: Fons Rademakers, 29/2/2000

MODDIR       := proof
MODDIRS      := $(MODDIR)/src
MODDIRI      := $(MODDIR)/inc

PROOFDIR     := $(MODDIR)
PROOFDIRS    := $(PROOFDIR)/src
PROOFDIRI    := $(PROOFDIR)/inc

##### libProof #####
PROOFL       := $(MODDIRI)/LinkDef.h
PROOFDS      := $(MODDIRS)/G__Proof.cxx
PROOFDO      := $(PROOFDS:.cxx=.o)
PROOFDH      := $(PROOFDS:.cxx=.h)

PROOFH       := $(filter-out $(MODDIRI)/LinkDef%,$(wildcard $(MODDIRI)/*.h))
PROOFS       := $(filter-out $(MODDIRS)/G__%,$(wildcard $(MODDIRS)/*.cxx))
PROOFO       := $(PROOFS:.cxx=.o)

PROOFDEP     := $(PROOFO:.o=.d) $(PROOFDO:.o=.d)

PROOFLIB     := $(LPATH)/libProof.$(SOEXT)

# used in the main Makefile
ALLHDRS     += $(patsubst $(MODDIRI)/%.h,include/%.h,$(PROOFH))
ALLLIBS     += $(PROOFLIB)

# include all dependency files
INCLUDEFILES += $(PROOFDEP)

##### local rules #####
include/%.h:    $(PROOFDIRI)/%.h
		cp $< $@

$(PROOFLIB):    $(PROOFO) $(PROOFDO) $(ORDER_) $(MAINLIBS) $(PROOFLIBDEP)
		@$(MAKELIB) $(PLATFORM) $(LD) "$(LDFLAGS)" \
		   "$(SOFLAGS)" libProof.$(SOEXT) $@ "$(PROOFO) $(PROOFDO)" \
		   "$(PROOFLIBEXTRA)"

$(PROOFDS):     $(PROOFH) $(PROOFL) $(ROOTCINTTMPEXE)
		@echo "Generating dictionary $@..."
		$(ROOTCINTTMP) -f $@ -c $(PROOFH) $(PROOFL)

all-proof:      $(PROOFLIB)

map-proof:      $(RLIBMAP)
		$(RLIBMAP) -r $(ROOTMAP) -l $(PROOFLIB) \
		   -d $(PROOFLIBDEP) -c $(PROOFL)

map::           map-proof

clean-proof:
		@rm -f $(PROOFO) $(PROOFDO)

clean::         clean-proof

distclean-proof: clean-proof
		@rm -f $(PROOFDEP) $(PROOFDS) $(PROOFDH) $(PROOFLIB)

distclean::     distclean-proof
