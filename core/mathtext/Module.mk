MODNAME      := mathtext
MODDIR       := $(ROOT_SRCDIR)/core/$(MODNAME)
MODDIRS      := $(MODDIR)/src
MODDIRI      := $(MODDIR)/inc

MATHTEXTDIR  := $(MODDIR)
MATHTEXTDIRS := $(MATHTEXTDIR)/src
MATHTEXTDIRI := $(MATHTEXTDIR)/inc

##### libmathtext #####
MATHTEXTH    := $(filter-out $(MODDIRI)/LinkDef%,$(wildcard $(MODDIRI)/*.h))
MATHTEXTS    := $(filter-out $(MODDIRS)/G__%,$(wildcard $(MODDIRS)/*.cc))
MATHTEXTO    := $(MATHTEXTS:.cc=.o)

MATHTEXTDEP  := $(MATHTEXTO:.o=.d) $(MATHTEXTDO:.o=.d)

MATHTEXTLIB  := $(LPATH)/libmathtext.a

# used in the main Makefile
ALLHDRS     += $(patsubst $(MODDIRI)/%.h,include/%.h,$(MATHTEXTH))
ALLLIBS     += $(MATHTEXTLIB)

# include all dependency files
INCLUDEFILES += $(MATHTEXTDEP)

MATHTEXTINC  := $(MATHTEXTDIRI:%=-I%)
MATHTEXTDEP  := $(MATHTEXTLIB)
MATHTEXTLDFLAGS :=

##### local rules #####
.PHONY:         all-$(MODNAME) clean-$(MODNAME) distclean-$(MODNAME)

include/%.h:    $(MATHTEXTDIRI)/%.h
		cp $< $@

$(MATHTEXTLIB): $(MATHTEXTO) $(ORDER_)
		$(AR) cru $@ $(MATHTEXTO)

all-$(MODNAME): $(MATHTEXTLIB)

clean-$(MODNAME):
		@rm -f $(MATHTEXTO)

clean::         clean-$(MODNAME)

distclean-$(MODNAME): clean-$(MODNAME)
		@rm -f $(MATHTEXTDEP) $(MATHTEXTDS) $(MATHTEXTDH) \
	$(MATHTEXTLIB)

distclean::     distclean-$(MODNAME)

##### extra rules ######
