# Module.mk for thread module
# Copyright (c) 2000 Rene Brun and Fons Rademakers
#
# Author: Fons Rademakers, 29/2/2000

MODDIR       := thread
MODDIRS      := $(MODDIR)/src
MODDIRI      := $(MODDIR)/inc

THREADDIR    := $(MODDIR)
THREADDIRS   := $(THREADDIR)/src
THREADDIRI   := $(THREADDIR)/inc

##### libThread #####
THREADL      := $(MODDIRI)/LinkDef.h
THREADDS     := $(MODDIRS)/G__Thread.cxx
THREADDO     := $(THREADDS:.cxx=.o)
THREADDH     := $(THREADDS:.cxx=.h)

THREADH      := $(MODDIRI)/TCondition.h $(MODDIRI)/TConditionImp.h \
                $(MODDIRI)/TMutex.h $(MODDIRI)/TMutexImp.h \
                $(MODDIRI)/TRWLock.h $(MODDIRI)/TSemaphore.h \
                $(MODDIRI)/TThread.h $(MODDIRI)/TThreadFactory.h \
                $(MODDIRI)/TThreadImp.h $(MODDIRI)/TAtomicCount.h \
                $(MODDIRI)/TLockFile.h
ifneq ($(ARCH),win32)
THREADH      += $(MODDIRI)/TPosixCondition.h $(MODDIRI)/TPosixMutex.h \
                $(MODDIRI)/TPosixThread.h $(MODDIRI)/TPosixThreadFactory.h \
                $(MODDIRI)/PosixThreadInc.h
# Headers that should be copied to $ROOTSYS/include but should not be
# passed directly to rootcint
THREADH_EXT  += $(MODDIRI)/TAtomicCountGcc.h $(MODDIRI)/TAtomicCountPthread.h
else
THREADH      += $(MODDIRI)/TWin32Condition.h $(MODDIRI)/TWin32Mutex.h \
                $(MODDIRI)/TWin32Thread.h $(MODDIRI)/TWin32ThreadFactory.h
THREADH_EXT  += $(MODDIRI)/TAtomicCountWin32.h
endif

THREADS      := $(MODDIRS)/TCondition.cxx $(MODDIRS)/TConditionImp.cxx \
                $(MODDIRS)/TMutex.cxx $(MODDIRS)/TMutexImp.cxx \
                $(MODDIRS)/TRWLock.cxx $(MODDIRS)/TSemaphore.cxx \
                $(MODDIRS)/TThread.cxx $(MODDIRS)/TThreadFactory.cxx \
                $(MODDIRS)/TThreadImp.cxx $(MODDIRS)/TLockFile.cxx
ifneq ($(ARCH),win32)
THREADS      += $(MODDIRS)/TPosixCondition.cxx $(MODDIRS)/TPosixMutex.cxx \
                $(MODDIRS)/TPosixThread.cxx $(MODDIRS)/TPosixThreadFactory.cxx
else
THREADS      += $(MODDIRS)/TWin32Condition.cxx $(MODDIRS)/TWin32Mutex.cxx \
                $(MODDIRS)/TWin32Thread.cxx $(MODDIRS)/TWin32ThreadFactory.cxx
endif

THREADO      := $(THREADS:.cxx=.o)

THREADDEP    := $(THREADO:.o=.d) $(THREADDO:.o=.d)

THREADLIB    := $(LPATH)/libThread.$(SOEXT)
THREADMAP    := $(THREADLIB:.$(SOEXT)=.rootmap)

# used in the main Makefile
ALLHDRS      += $(patsubst $(MODDIRI)/%.h,include/%.h,$(THREADH) $(THREADH_EXT))
ALLLIBS      += $(THREADLIB)
ALLMAPS      += $(THREADMAP)

CXXFLAGS     += $(OSTHREADFLAG)
CFLAGS       += $(OSTHREADFLAG)
CINTCXXFLAGS += $(OSTHREADFLAG)
CINTCFLAGS   += $(OSTHREADFLAG)

ifeq ($(PLATFORM),win32)
CXXFLAGS     += -D_WIN32_WINNT=0x0400
endif

# include all dependency files
INCLUDEFILES += $(THREADDEP)

##### local rules #####
include/%.h:    $(THREADDIRI)/%.h
		cp $< $@

$(THREADLIB):   $(THREADO) $(THREADDO) $(ORDER_) $(MAINLIBS) $(THREADLIBDEP)
		@$(MAKELIB) $(PLATFORM) $(LD) "$(LDFLAGS)" \
		   "$(SOFLAGS)" libThread.$(SOEXT) $@ "$(THREADO) $(THREADDO)" \
		   "$(THREADLIBEXTRA) $(OSTHREADLIBDIR) $(OSTHREADLIB)"

$(THREADDS):    $(THREADH) $(THREADL) $(ROOTCINTTMPDEP)
		@echo "Generating dictionary $@..."
		$(ROOTCINTTMP) -f $@ -c $(THREADH) $(THREADL)

$(THREADMAP):   $(RLIBMAP) $(MAKEFILEDEP) $(THREADL)
		$(RLIBMAP) -o $(THREADMAP) -l $(THREADLIB) \
		   -d $(THREADLIBDEPM) -c $(THREADL)

all-thread:     $(THREADLIB) $(THREADMAP)

clean-thread:
		@rm -f $(THREADO) $(THREADDO)

clean::         clean-thread

distclean-thread: clean-thread
		@rm -f $(THREADDEP) $(THREADDS) $(THREADDH) $(THREADLIB) $(THREADMAP)

distclean::     distclean-thread

##### cintdlls ######

ifneq ($(ARCH),win32)
$(CINTDIRDLLS)/pthread.dll: cint/lib/pthread/pthd.h $(ROOTCINTTMPDEP) $(CINTTMP)
	@$(MAKECINTDLL) $(PLATFORM) C pthread pthread pthd.h \
           "$(CINTTMP)" "$(ROOTCINTTMP)" \
	   "$(MAKELIB)" "$(CXX)" "$(CC)" "$(LD)" "$(OPT)" "$(CINTCXXFLAGS)" \
	   "$(CINTCFLAGS)" "$(LDFLAGS)" "$(SOFLAGS)" "$(SOEXT)" "$(COMPILER)" \
	   "$(CXXOUT)"
endif
