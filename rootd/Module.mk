# Module.mk for rootd module
# Copyright (c) 2000 Rene Brun and Fons Rademakers
#
# Author: Fons Rademakers, 29/2/2000

MODDIR       := rootd
MODDIRS      := $(MODDIR)/src
MODDIRI      := $(MODDIR)/inc

ROOTDDIR     := $(MODDIR)
ROOTDDIRS    := $(ROOTDDIR)/src
ROOTDDIRI    := $(ROOTDDIR)/inc

##### rootd #####
ROOTDH       := $(wildcard $(MODDIRI)/*.h)
ROOTDS       := $(wildcard $(MODDIRS)/*.cxx)
ROOTDO       := $(ROOTDS:.cxx=.o)
ROOTDDEP     := $(ROOTDO:.o=.d)
ROOTD        := bin/rootd

##### use shadow passwords for authentication #####
SHADOWFLAGS  := #-DR__SHADOWPW
SHADOWLIBS   :=

##### use AFS for authentication #####
ifneq ($(AFSDIR),)
AFSFLAGS     := -DR__AFS
AFSLIBS      := -L$(AFSDIR)/lib -L$(AFSDIR)/lib/afs -lkauth -lprot \
                -lubik -lauth -lrxkad -lsys -ldes -lrx -llwp \
                -lcmd -lcom_err -laudit $(AFSDIR)/lib/afs/util.a
endif

##### use SRP for authentication #####
ifneq ($(SRPDIR),)
SRPFLAGS     := -DR__SRP -I$(SRPDIR)/include
SRPLIBS      := -L$(SRPDIR)/lib -lsrp -lgmp
endif

AUTHFLAGS    := $(SHADOWFLAGS) $(AFSFLAGS) $(SRPFLAGS)
AUTHLIBS     := $(SHADOWLIBS) $(AFSLIBS) $(SRPLIBS)

# used in the main Makefile
ALLHDRS      += $(patsubst $(MODDIRI)/%.h,include/%.h,$(ROOTDH))
ALLEXECS     += $(ROOTD)

# include all dependency files
INCLUDEFILES += $(ROOTDDEP)

##### local rules #####
include/%.h:    $(ROOTDDIRI)/%.h
		cp $< $@

$(ROOTD):       $(ROOTDO)
		$(LD) $(LDFLAGS) -o $@ $(ROOTDO) $(AUTHLIBS) $(CRYPTLIBS) \
		   $(SYSLIBS)

all-rootd:      $(ROOTD)

clean-rootd:
		@rm -f $(ROOTDO)

clean::         clean-rootd

distclean-rootd: clean-rootd
		@rm -f $(ROOTDDEP) $(ROOTD)

distclean::     distclean-rootd

##### extra rules ######
$(ROOTDDIRS)/rootd.o: $(ROOTDDIRS)/rootd.cxx
	$(CXX) $(OPT) $(CXXFLAGS) $(AUTHFLAGS) -o $@ -c $<
