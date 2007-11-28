# Module.mk for mathcore module
# Copyright (c) 2000 Rene Brun and Fons Rademakers
#
# Author: Fons Rademakers, 20/6/2005

MODDIR       := mathcore
MODDIRS      := $(MODDIR)/src
MODDIRI      := $(MODDIR)/inc

MATHCOREDIR  := $(MODDIR)
MATHCOREDIRS := $(MATHCOREDIR)/src
MATHCOREDIRI := $(MATHCOREDIR)/inc

##### libMathCore #####
MATHCOREL    := $(MODDIRI)/Math/LinkDef.h
MATHCOREL32  := $(MODDIRI)/Math/LinkDef_GenVector32.h
MATHCORELINC := $(MODDIRI)/Math/LinkDef_Func.h \
                $(MODDIRI)/Math/LinkDef_GenVector.h \
                $(MODDIRI)/Math/LinkDef_Point3D.h \
                $(MODDIRI)/Math/LinkDef_Vector3D.h \
                $(MODDIRI)/Math/LinkDef_Vector4D.h \
                $(MODDIRI)/Math/LinkDef_Rotation.h
MATHCOREDS   := $(MODDIRS)/G__MathCore.cxx
MATHCOREDS32 := $(MODDIRS)/G__MathCore32.cxx
MATHCOREDO   := $(MATHCOREDS:.cxx=.o)
MATHCOREDO32 := $(MATHCOREDS32:.cxx=.o)
MATHCOREDH   := $(MATHCOREDS:.cxx=.h)

MATHCOREDH1  :=  $(MODDIRI)/Math/Vector2D.h \
                 $(MODDIRI)/Math/Point2D.h \
                 $(MODDIRI)/Math/Vector3D.h \
                 $(MODDIRI)/Math/Point3D.h \
                 $(MODDIRI)/Math/Vector4D.h \
                 $(MODDIRI)/Math/Rotation3D.h \
                 $(MODDIRI)/Math/RotationZYX.h \
                 $(MODDIRI)/Math/RotationX.h \
                 $(MODDIRI)/Math/RotationY.h \
                 $(MODDIRI)/Math/RotationZ.h \
                 $(MODDIRI)/Math/LorentzRotation.h \
                 $(MODDIRI)/Math/Boost.h    \
                 $(MODDIRI)/Math/BoostX.h    \
                 $(MODDIRI)/Math/BoostY.h    \
                 $(MODDIRI)/Math/BoostZ.h    \
                 $(MODDIRI)/Math/EulerAngles.h \
                 $(MODDIRI)/Math/AxisAngle.h \
                 $(MODDIRI)/Math/Quaternion.h \
                 $(MODDIRI)/Math/Transform3D.h \
                 $(MODDIRI)/Math/Translation3D.h \
                 $(MODDIRI)/Math/Plane3D.h \
                 $(MODDIRI)/Math/VectorUtil_Cint.h  \
                 $(MODDIRI)/Math/SpecFuncMathCore.h \
                 $(MODDIRI)/Math/DistFuncMathCore.h \
                 $(MODDIRI)/Math/IParamFunction.h \
                 $(MODDIRI)/Math/IFunction.h \
                 $(MODDIRI)/Math/Functor.h \
                 $(MODDIRI)/Math/Minimizer.h \
                 $(MODDIRI)/Math/Integrator.h \
                 $(MODDIRI)/Math/VirtualIntegrator.h \
                 $(MODDIRI)/Math/AllIntegrationTypes.h \
                 $(MODDIRI)/Math/AdaptiveIntegratorMultiDim.h \
                 $(MODDIRI)/Math/IntegratorMultiDim.h 
#                 $(MODDIRI)/Math/Functor.h \


MATHCOREDH132:=  $(MODDIRI)/Math/Vector2D.h \
	         $(MODDIRI)/Math/Point2D.h \
	         $(MODDIRI)/Math/Vector3D.h \
                 $(MODDIRI)/Math/Point3D.h \
                 $(MODDIRI)/Math/Vector4D.h \



MATHCOREAH   := $(filter-out $(MODDIRI)/Math/LinkDef%, $(wildcard $(MODDIRI)/Math/*.h))
MATHCOREGVH  := $(filter-out $(MODDIRI)/Math/GenVector/LinkDef%, $(wildcard $(MODDIRI)/Math/GenVector/*.h))
MATHCOREH    := $(MATHCOREAH) $(MATHCOREGVH)
MATHCORES    := $(filter-out $(MODDIRS)/G__%,$(wildcard $(MODDIRS)/*.cxx))
MATHCOREO    := $(MATHCORES:.cxx=.o)

MATHCOREDEP  := $(MATHCOREO:.o=.d)  $(MATHCOREDO:.o=.d) $(MATHCOREDO32:.o=.d)

MATHCORELIB  := $(LPATH)/libMathCore.$(SOEXT)
MATHCOREMAP  := $(MATHCORELIB:.$(SOEXT)=.rootmap)

# used in the main Makefile
ALLHDRS      += $(patsubst $(MODDIRI)/Math/%.h,include/Math/%.h,$(MATHCOREH))
ALLLIBS      += $(MATHCORELIB)
ALLMAPS      += $(MATHCOREMAP)

# include all dependency files
INCLUDEFILES += $(MATHCOREDEP)

##### local rules #####
include/Math/%.h: $(MATHCOREDIRI)/Math/%.h
		@(if [ ! -d "include/Math/GenVector" ]; then   \
		   mkdir -p include/Math/GenVector;       \
		fi)
		cp $< $@

$(MATHCORELIB): $(MATHCOREO) $(MATHCOREDO) $(MATHCOREDO32) $(ORDER_) $(MAINLIBS)
		@$(MAKELIB) $(PLATFORM) $(LD) "$(LDFLAGS)"  \
		   "$(SOFLAGS)" libMathCore.$(SOEXT) $@     \
		   "$(MATHCOREO) $(MATHCOREDO) $(MATHCOREDO32)"    \
		   "$(MATHCORELIBEXTRA)"

$(MATHCOREDS):  $(MATHCOREDH1) $(MATHCOREL) $(MATHCORELINC) $(ROOTCINTTMPEXE)
		@echo "Generating dictionary $@..."
		@echo "for files $(MATHCOREDH1)"
		$(ROOTCINTTMP) -f $@ -c $(MATHCOREDH1) $(MATHCOREL)
#		genreflex $(MATHCOREDIRS)/MathCoreDict.h  --selection_file=$(MATHCOREDIRS)/selection_MathCore.xml -o $(MATHCOREDIRS)/G__MathCore.cxx -I$(MATHCOREDIRI)

$(MATHCOREDS32):$(MATHCOREDH132) $(MATHCOREL) $(MATHCORELINC) $(ROOTCINTTMPEXE)
		@echo "Generating dictionary $@..."
		@echo "for files $(MATHCOREDH132)"
		$(ROOTCINTTMP) -f $@ -c $(MATHCOREDH132) $(MATHCOREL32)

$(MATHCOREMAP): $(RLIBMAP) $(MAKEFILEDEP) $(MATHCOREL) $(MATHCORELINC) $(MATHCOREL32)
		$(RLIBMAP) -o $(MATHCOREMAP) -l $(MATHCORELIB) \
		   -d $(MATHCORELIBDEPM) -c $(MATHCOREL) $(MATHCORELINC) $(MATHCOREL32)

all-mathcore:   $(MATHCORELIB) $(MATHCOREMAP)

clean-mathcore:
		@rm -f $(MATHCOREO) $(MATHCOREDO)

clean::         clean-mathcore

distclean-mathcore: clean-mathcore
		@rm -f $(MATHCOREDEP) $(MATHCOREDS) $(MATHCOREDS32) $(MATHCOREDH) \
		   $(MATHCORELIB) $(MATHCOREMAP)
		@rm -rf include/Math

distclean::     distclean-mathcore

test-mathcore:	all-mathcore
		@cd $(MATHCOREDIR)/test; make

##### extra rules ######
$(MATHCOREO): CXXFLAGS += -DUSE_ROOT_ERROR
$(MATHCOREDO): CXXFLAGS += -DUSE_ROOT_ERROR

