# Module.mk for krb5 authentication module
# Copyright (c) 2002 Rene Brun and Fons Rademakers
#
# Author: Fons Rademakers, 18/3/2002

MODDIR       := $(SRCDIR)/krb5auth
MODDIRS      := $(MODDIR)/src
MODDIRI      := $(MODDIR)/inc

KRB5AUTHDIR  := $(MODDIR)
KRB5AUTHDIRS := $(KRB5AUTHDIR)/src
KRB5AUTHDIRI := $(KRB5AUTHDIR)/inc

##### libKrb5Auth #####
KRB5AUTHL    := $(MODDIRI)/LinkDef.h
KRB5AUTHDS   := $(subst $(SRCDIR)/,,$(MODDIRS))/G__Krb5Auth.cxx
KRB5AUTHDO   := $(KRB5AUTHDS:.cxx=.o)
KRB5AUTHDH   := $(KRB5AUTHDS:.cxx=.h)

KRB5AUTHH1   := $(patsubst %,$(MODDIRI)/%,TKSocket.h)

KRB5AUTHH    := $(filter-out $(MODDIRI)/LinkDef%,$(wildcard $(MODDIRI)/*.h))
KRB5AUTHS    := $(filter-out $(MODDIRS)/G__%,$(wildcard $(MODDIRS)/*.cxx))
KRB5AUTHO    := $(subst $(SRCDIR)/,,$(KRB5AUTHS:.cxx=.o))

KRB5AUTHDEP  := $(KRB5AUTHO:.o=.d)

KRB5AUTHLIB  := $(LPATH)/libKrb5Auth.$(SOEXT)
KRB5AUTHMAP  := $(KRB5AUTHLIB:.$(SOEXT)=.rootmap)

# used in the main Makefile
ALLHDRS     += $(patsubst $(MODDIRI)/%.h,include/%.h,$(KRB5AUTHH))
ALLLIBS     += $(KRB5AUTHLIB)
ALLMAPS     += $(KRB5AUTHMAP)

# include all dependency files
INCLUDEFILES += $(KRB5AUTHDEP)

##### local rules #####
include/%.h:    $(KRB5AUTHDIRI)/%.h
		cp $< $@

$(KRB5AUTHLIB): $(KRB5AUTHO) $(KRB5AUTHDO) $(ORDER_) $(MAINLIBS) $(KRB5AUTHLIBDEP)
		@$(MAKELIB) $(PLATFORM) $(LD) "$(LDFLAGS)" \
		   "$(SOFLAGS)" libKrb5Auth.$(SOEXT) $@ \
		   "$(KRB5AUTHO) $(KRB5AUTHDO)" \
		   "$(KRB5AUTHLIBEXTRA) $(KRB5LIBDIR) $(KRB5LIB) \
		    $(COMERRLIBDIR) $(COMERRLIB) $(RESOLVLIB) \
		    $(CRYPTOLIBDIR) $(CRYPTOLIB) $(KRB5AUTHLIBEXTRA)"

$(KRB5AUTHDS):  $(KRB5AUTHH1) $(KRB5AUTHL) $(ROOTCINTTMPEXE)
		@echo "Generating dictionary $@..."
		$(ROOTCINTTMP) -f $@ -c $(KRB5INCDIR:%=-I%) $(KRB5AUTHH1) $(KRB5AUTHL)

$(KRB5AUTHMAP): $(RLIBMAP) $(MAKEFILEDEP) $(KRB5AUTHL)
		$(RLIBMAP) -o $(KRB5AUTHMAP) -l $(KRB5AUTHLIB) \
		   -d $(KRB5AUTHLIBDEPM) -c $(KRB5AUTHL)

all-krb5auth:   $(KRB5AUTHLIB) $(KRB5AUTHMAP)

clean-krb5auth:
		@rm -f $(KRB5AUTHO) $(KRB5AUTHDO)

clean::         clean-krb5auth

distclean-krb5auth: clean-krb5auth
		@rm -f $(KRB5AUTHDEP) $(KRB5AUTHDS) $(KRB5AUTHDH) \
		   $(KRB5AUTHLIB) $(KRB5AUTHMAP)

distclean::     distclean-krb5auth

##### extra rules ######
$(KRB5AUTHDO): CXXFLAGS += $(KRB5INCDIR:%=-I%)

$(KRB5AUTHO): CXXFLAGS += -DR__KRB5INIT="\"$(KRB5INIT)\"" $(KRB5INCDIR:%=-I%)
$(KRB5AUTHO): PCHCXXFLAGS =
