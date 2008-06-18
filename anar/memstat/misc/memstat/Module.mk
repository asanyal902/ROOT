# Module.mk for newdelete module
# Copyright (c) 2005 Rene Brun and Fons Rademakers
#
# Author: Anar Manafov 17/06/2008

MODNAME     := memstat
MODDIR      := misc/$(MODNAME)
MODDIRS     := $(MODDIR)/src
MODDIRI     := $(MODDIR)/inc

MEMSTATDIR    := $(MODDIR)
MEMSTATDIRS   := $(MEMSTATDIR)/src
MEMSTATDIRI   := $(MEMSTATDIR)/inc

##### libMemstat #####
MEMSTATL      := $(MODDIRI)/LinkDef.h
MEMSTATDS     := $(MODDIRS)/G__memstat.cxx
MEMSTATDO     := $(MEMSTATDS:.cxx=.o)
MEMSTATDH     := $(MEMSTATDS:.cxx=.h)

MEMSTATH      := $(MODDIRI)/TMemStatHelpers.h $(MODDIRI)/TMemStatDepend.h $(MODDIRI)/TMemStat.h \
		 $(MODDIRI)/TMemStatManager.h $(MODDIRI)/TMemStatInfo.h
MEMSTATS      := $(MODDIRS)/TMemStat.cxx $(MODDIRS)/TMemStatManager.cxx \
		 $(MODDIRS)/TMemStatDepend.cxx $(MODDIRS)/TMemStatInfo.cxx
MEMSTATO      := $(MEMSTATS:.cxx=.o)

MEMSTATDEP    := $(MEMSTATO:.o=.d) $(MEMSTATDO:.o=.d)

MEMSTATLIB    := $(LPATH)/libMemStat.$(SOEXT)
MEMSTATMAP    := $(MEMSTATLIB:.$(SOEXT)=.rootmap)

##### libMemstatGUI #####
MEMSTATGUIL      := $(MODDIRI)/LinkDefGUI.h
MEMSTATGUIDS     := $(MODDIRS)/G__memstatGUI.cxx
MEMSTATGUIDO     := $(MEMSTATGUIDS:.cxx=.o)
MEMSTATGUIDH     := $(MEMSTATGUIDS:.cxx=.h)

MEMSTATGUIH      := $(MODDIRI)/TMemStat.h $(MODDIRI)/TMemViewerGUI.h $(MODDIRI)/TMemStatDrawDlg.h $(MODDIRI)/TMemStatResource.h
MEMSTATGUIS      := $(MODDIRS)/TMemViewerGUI.cxx $(MODDIRS)/TMemStatDrawDlg.cxx
MEMSTATGUIO      := $(MEMSTATGUIS:.cxx=.o)

MEMSTATGUIDEP    := $(MEMSTATGUIO:.o=.d) $(MEMSTATGUIDO:.o=.d)

MEMSTATGUILIB    := $(LPATH)/libMemStatGUI.$(SOEXT)
MEMSTATGUIMAP    := $(MEMSTATGUILIB:.$(SOEXT)=.rootmap)

# used in the main Makefile
ALLHDRS     += $(patsubst $(MODDIRI)/%.h,include/%.h,$(MEMSTATH))
ALLHDRS     += $(patsubst $(MODDIRI)/%.h,include/%.h,$(MEMSTATGUIH))
ALLLIBS     += $(MEMSTATLIB) $(MEMSTATGUILIB)
ALLMAPS     += $(MEMSTATMAP) $(MEMSTATGUIMAP)

# include all dependency files
INCLUDEFILES += $(MEMSTATDEP)

##### local rules #####
.PHONY:         all-$(MODNAME) clean-$(MODNAME) distclean-$(MODNAME)

include/%.h:    $(MEMSTATDIRI)/%.h
		cp $< $@

##### libMemstat #####
$(MEMSTATLIB):   $(MEMSTATO) $(MEMSTATDO) $(ORDER_) $(MAINLIBS) $(MEMSTATLIBDEP)
		@$(MAKELIB) $(PLATFORM) $(LD) "$(LDFLAGS)" \
		"$(SOFLAGS)" libMemstat.$(SOEXT) $@ "$(MEMSTATO) $(MEMSTATDO)"

$(MEMSTATDS):    $(MEMSTATH) $(MEMSTATL) $(ROOTCINTTMPEXE)
		@echo "Generating dictionary $@..."
		$(ROOTCINTTMP) -f $@ -c $(MEMSTATH) $(MEMSTATL)

$(MEMSTATMAP):  $(RLIBMAP) $(MAKEFILEDEP) $(MEMSTATL)
		$(RLIBMAP) -o $(MEMSTATMAP) -l $(MEMSTATLIB) \
		-d $(MEMSTATLIBDEPM) -c $(MEMSTATL)

##### libMemstatGUI #####
$(MEMSTATGUILIB):   $(MEMSTATGUIO) $(MEMSTATGUIDO) $(ORDER_) $(MAINLIBS) $(MEMSTATGUILIBDEP)
		@$(MAKELIB) $(PLATFORM) $(LD) "$(LDFLAGS)" \
		"$(SOFLAGS)" libMemstatGUI.$(SOEXT) $@ "$(MEMSTATGUIO) $(MEMSTATGUIDO)"

$(MEMSTATGUIDS): $(MEMSTATGUIH) $(MEMSTATGUIL) $(ROOTCINTTMPEXE)
		@echo "Generating dictionary $@..."
		$(ROOTCINTTMP) -f $@ -c $(MEMSTATGUIH) $(MEMSTATGUIL)

$(MEMSTATGUIMAP):  $(RLIBMAP) $(MAKEFILEDEP) $(MEMSTATGUIL)
		$(RLIBMAP) -o $(MEMSTATGUIMAP) -l $(MEMSTATGUILIB) \
		-d $(MEMSTATGUILIBDEPM) -c $(MEMSTATGUIL)


all-$(MODNAME): $(MEMSTATLIB) $(MEMSTATMAP) $(MEMSTATGUILIB) $(MEMSTATGUIMAP)


clean-$(MODNAME):
		@rm -f $(MEMSTATO) $(MEMSTATDO) $(MEMSTATGUIO) $(MEMSTATGUIDO)

clean::         clean-$(MODNAME)

distclean-$(MODNAME): clean-$(MODNAME)
		@rm -f $(MEMSTATDEP) $(MEMSTATDS) $(MEMSTATDH) $(MEMSTATLIB) $(MEMSTATMAP)
		@rm -f $(MEMSTATGUIDEP) $(MEMSTATGUIDS) $(MEMSTATGUIDH) $(MEMSTATGUILIB) $(MEMSTATGUIMAP)

distclean::     distclean-$(MODNAME)
