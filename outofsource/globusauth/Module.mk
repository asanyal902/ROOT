# Module.mk for krb5 authentication module
# Copyright (c) 2002 Rene Brun and Fons Rademakers
#
# Author: Fons Rademakers, 18/3/2002 (for krb5auth)
# Mod by: Gerardo Ganis, 18/1/2003

MODDIR       := $(SRCDIR)/globusauth
MODDIRS      := $(MODDIR)/src
MODDIRI      := $(MODDIR)/inc

GLBSAUTHDIR  := $(MODDIR)
GLBSAUTHDIRS := $(GLBSAUTHDIR)/src
GLBSAUTHDIRI := $(GLBSAUTHDIR)/inc

##### libGlobusAuth #####
GLBSAUTHH    := $(wildcard $(MODDIRI)/*.h)
GLBSAUTHS    := $(wildcard $(MODDIRS)/*.cxx)
GLBSAUTHO    := $(subst $(SRCDIR)/,,$(GLBSAUTHS:.cxx=.o))

GLBSAUTHDEP  := $(GLBSAUTHO:.o=.d)

GLBSAUTHLIB  := $(LPATH)/libGlobusAuth.$(SOEXT)

##### experimental patch #####
GLBPATCHS     :=
GLBPATCHO     :=$(subst $(SRCDIR)/,,)
GLBPATCHDEP   :=
ifneq ($(GLBPATCHFLAGS),)
GLBPATCHS     := $(MODDIRS)/globus_gsi_credential.c
GLBPATCHO     := $(subst $(SRCDIR)/,,$(GLBPATCHS:.c=.o))
GLBPATCHDEP   := $(GLBPATCHO:.o=.d)
endif

# used in the main Makefile
ALLHDRS     += $(patsubst $(MODDIRI)/%.h,include/%.h,$(GLBSAUTHH))
ALLLIBS     += $(GLBSAUTHLIB)

# include all dependency files
INCLUDEFILES += $(GLBSAUTHDEP)

##### local rules #####
include/%.h:    $(GLBSAUTHDIRI)/%.h
		cp $< $@

$(GLBSAUTHLIB): $(GLBSAUTHO) $(GLBPATCHO) $(ORDER_) $(MAINLIBS)
		@$(MAKELIB) $(PLATFORM) $(LD) "$(LDFLAGS)" \
		   "$(SOFLAGS)" libGlobusAuth.$(SOEXT) $@ \
		   "$(GLBSAUTHO) $(GLBPATCHO)" \
		   "$(GLBSAUTHLIBEXTRA) $(GLOBUSLIBDIR) $(GLOBUSLIB)"

all-globusauth: $(GLBSAUTHLIB)

clean-globusauth:
		@rm -f $(GLBSAUTHO)

clean::         clean-globusauth

distclean-globusauth: clean-globusauth
		@rm -f $(GLBSAUTHDEP) $(GLBSAUTHLIB)

distclean::     distclean-globusauth

##### extra rules ######
$(GLBSAUTHO): CXXFLAGS += $(GLOBUSINCDIR:%=-I%)
$(GLBPATCHO): CFLAGS += $(GLBPATCHFLAGS) $(GLOBUSINCDIR:%=-I%)
