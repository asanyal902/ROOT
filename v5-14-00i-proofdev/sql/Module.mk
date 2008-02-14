# Module.mk for sql module
# Copyright (c) 2005 Rene Brun and Fons Rademakers
#
# Author: Fons Rademakers, 7/12/2005

MODDIR       := sql
MODDIRS      := $(MODDIR)/src
MODDIRI      := $(MODDIR)/inc

SQLDIR       := $(MODDIR)
SQLDIRS      := $(SQLDIR)/src
SQLDIRI      := $(SQLDIR)/inc

##### libSQL #####
SQLL         := $(MODDIRI)/LinkDef.h
SQLDS        := $(MODDIRS)/G__SQL.cxx
SQLDO        := $(SQLDS:.cxx=.o)
SQLDH        := $(SQLDS:.cxx=.h)

SQLH         := $(filter-out $(MODDIRI)/LinkDef%,$(wildcard $(MODDIRI)/*.h))
SQLS         := $(filter-out $(MODDIRS)/G__%,$(wildcard $(MODDIRS)/*.cxx))
SQLO         := $(SQLS:.cxx=.o)

SQLDEP       := $(SQLO:.o=.d) $(SQLDO:.o=.d)

SQLLIB       := $(LPATH)/libSQL.$(SOEXT)

# used in the main Makefile
ALLHDRS      += $(patsubst $(MODDIRI)/%.h,include/%.h,$(SQLH))
ALLLIBS      += $(SQLLIB)

# include all dependency files
INCLUDEFILES += $(SQLDEP)

##### local rules #####
include/%.h:    $(SQLDIRI)/%.h
		cp $< $@

$(SQLLIB):      $(SQLO) $(SQLDO) $(ORDER_) $(MAINLIBS) $(SQLLIBDEP)
		@$(MAKELIB) $(PLATFORM) $(LD) "$(LDFLAGS)" \
		   "$(SOFLAGS)" libSQL.$(SOEXT) $@ "$(SQLO) $(SQLDO)" \
		   "$(SQLLIBEXTRA)"

$(SQLDS):       $(SQLH) $(SQLL) $(ROOTCINTTMPEXE)
		@echo "Generating dictionary $@..."
		$(ROOTCINTTMP) -f $@ -c $(SQLH) $(SQLL)

all-sql:        $(SQLLIB)

map-sql:        $(RLIBMAP)
		$(RLIBMAP) -r $(ROOTMAP) -l $(SQLLIB) \
		   -d $(SQLLIBDEP) -c $(SQLL)

map::           map-sql

clean-sql:
		@rm -f $(SQLO) $(SQLDO)

clean::         clean-sql

distclean-sql: clean-sql
		@rm -f $(SQLDEP) $(SQLDS) $(SQLDH) $(SQLLIB)

distclean::     distclean-sql
