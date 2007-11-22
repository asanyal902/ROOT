# Module.mk for x11ttf module
# Copyright (c) 2000 Rene Brun and Fons Rademakers
#
# Author: Fons Rademakers, 29/2/2000

MODDIR       := $(SRCDIR)/x11ttf
MODDIRS      := $(MODDIR)/src
MODDIRI      := $(MODDIR)/inc

X11TTFDIR    := $(MODDIR)
X11TTFDIRS   := $(X11TTFDIR)/src
X11TTFDIRI   := $(X11TTFDIR)/inc

##### libGX11TTF #####
X11TTFL      := $(MODDIRI)/LinkDef.h
X11TTFDS     := $(subst $(SRCDIR)/,,$(MODDIRS))/G__X11TTF.cxx
X11TTFDO     := $(X11TTFDS:.cxx=.o)
X11TTFDH     := $(X11TTFDS:.cxx=.h)

X11TTFH      := $(filter-out $(MODDIRI)/LinkDef%,$(wildcard $(MODDIRI)/*.h))
X11TTFS      := $(filter-out $(MODDIRS)/G__%,$(wildcard $(MODDIRS)/*.cxx))
X11TTFO      := $(subst $(SRCDIR)/,,$(X11TTFS:.cxx=.o))

X11TTFDEP    := $(X11TTFO:.o=.d) $(X11TTFDO:.o=.d)

X11TTFLIB    := $(LPATH)/libGX11TTF.$(SOEXT)
X11TTFMAP    := $(X11TTFLIB:.$(SOEXT)=.rootmap)

# used in the main Makefile
ALLHDRS     += $(patsubst $(MODDIRI)/%.h,include/%.h,$(X11TTFH))
ALLLIBS     += $(X11TTFLIB)
ALLMAPS     += $(X11TTFMAP)

#ifeq ($(XFTLIB),yes)
XLIBS       += $(X11LIBDIR) -lXft
#endif

# include all dependency files
INCLUDEFILES += $(X11TTFDEP)

##### local rules #####
include/%.h:    $(X11TTFDIRI)/%.h
		cp $< $@

$(X11TTFLIB):   $(X11TTFO) $(X11TTFDO) $(FREETYPEDEP) $(ORDER_) $(MAINLIBS) $(X11TTFLIBDEP)
		@$(MAKELIB) $(PLATFORM) $(LD) "$(LDFLAGS)" \
		   "$(SOFLAGS)" libGX11TTF.$(SOEXT) $@ \
		   "$(X11TTFO) $(X11TTFDO)" \
		   "$(FREETYPELDFLAGS) $(FREETYPELIB) \
		    $(X11TTFLIBEXTRA) $(XLIBS)"

$(X11TTFDS):    $(X11TTFH) $(X11TTFL) $(ROOTCINTTMPEXE)
		@echo "Generating dictionary $@..."
		$(ROOTCINTTMP) -f $@ -c $(FREETYPEINC) $(X11TTFH) $(X11TTFL)

$(X11TTFMAP):   $(RLIBMAP) $(MAKEFILEDEP) $(X11TTFL)
		$(RLIBMAP) -o $(X11TTFMAP) -l $(X11TTFLIB) \
		   -d $(X11TTFLIBDEPM) -c $(X11TTFL)

all-x11ttf:     $(X11TTFLIB) $(X11TTFMAP)

clean-x11ttf:
		@rm -f $(X11TTFO) $(X11TTFDO)

clean::         clean-x11ttf

distclean-x11ttf: clean-x11ttf
		@rm -f $(X11TTFDEP) $(X11TTFDS) $(X11TTFDH) $(X11TTFLIB) $(X11TTFMAP)

distclean::     distclean-x11ttf

##### extra rules ######
$(X11TTFO) $(X11TTFDO): $(FREETYPEDEP)
ifeq ($(PLATFORM),macosx)
$(X11TTFO) $(X11TTFDO): CXXFLAGS += -I/usr/X11R6/include $(FREETYPEINC)
else
$(X11TTFO) $(X11TTFDO): CXXFLAGS += $(FREETYPEINC)
endif
