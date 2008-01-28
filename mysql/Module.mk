# Module.mk for mysql module
# Copyright (c) 2000 Rene Brun and Fons Rademakers
#
# Author: Fons Rademakers, 29/2/2000

MODDIR       := mysql
MODDIRS      := $(MODDIR)/src
MODDIRI      := $(MODDIR)/inc

MYSQLDIR     := $(MODDIR)
MYSQLDIRS    := $(MYSQLDIR)/src
MYSQLDIRI    := $(MYSQLDIR)/inc

##### libRMySQL #####
MYSQLL       := $(MODDIRI)/LinkDef.h
MYSQLDS      := $(MODDIRS)/G__MySQL.cxx
MYSQLDO      := $(MYSQLDS:.cxx=.o)
MYSQLDH      := $(MYSQLDS:.cxx=.h)

MYSQLH       := $(filter-out $(MODDIRI)/LinkDef%,$(wildcard $(MODDIRI)/*.h))
MYSQLS       := $(filter-out $(MODDIRS)/G__%,$(wildcard $(MODDIRS)/*.cxx))
MYSQLO       := $(MYSQLS:.cxx=.o)

MYSQLDEP     := $(MYSQLO:.o=.d) $(MYSQLDO:.o=.d)

MYSQLLIB     := $(LPATH)/libRMySQL.$(SOEXT)
MYSQLMAP     := $(MYSQLLIB:.$(SOEXT)=.rootmap)

# used in the main Makefile
ALLHDRS     += $(patsubst $(MODDIRI)/%.h,include/%.h,$(MYSQLH))
ALLLIBS     += $(MYSQLLIB)
ALLMAPS     += $(MYSQLMAP)

# include all dependency files
INCLUDEFILES += $(MYSQLDEP)

##### local rules #####
include/%.h:    $(MYSQLDIRI)/%.h
		cp $< $@

$(MYSQLLIB):    $(MYSQLO) $(MYSQLDO) $(ORDER_) $(MAINLIBS) $(MYSQLLIBDEP)
		@$(MAKELIB) $(PLATFORM) $(LD) "$(LDFLAGS)" \
		   "$(SOFLAGS)" libRMySQL.$(SOEXT) $@ "$(MYSQLO) $(MYSQLDO)" \
		   "$(MYSQLLIBEXTRA) $(MYSQLLIBDIR) $(MYSQLCLILIB)"

$(MYSQLDS):     $(MYSQLH) $(MYSQLL) $(MYSQLO) $(ROOTCINTTMPEXE)
		@echo "Generating dictionary $@..."
		$(ROOTCINTTMP) -f $@ -c $(MYSQLINCDIR:%=-I%) $(MYSQLH) $(MYSQLL)

$(MYSQLMAP):    $(RLIBMAP) $(MAKEFILEDEP) $(MYSQLL)
		$(RLIBMAP) -o $(MYSQLMAP) -l $(MYSQLLIB) \
		   -d $(MYSQLLIBDEPM) -c $(MYSQLL)

all-mysql:      $(MYSQLLIB) $(MYSQLMAP)

clean-mysql:
		@rm -f $(MYSQLO) $(MYSQLDO)

clean::         clean-mysql

distclean-mysql: clean-mysql
		@rm -f $(MYSQLDEP) $(MYSQLDS) $(MYSQLDH) $(MYSQLLIB) $(MYSQLMAP)

distclean::     distclean-mysql

##### extra rules ######
$(MYSQLO) $(MYSQLDO): CXXFLAGS += $(MYSQLINCDIR:%=-I%)
