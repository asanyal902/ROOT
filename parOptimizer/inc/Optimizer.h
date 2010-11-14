/**********************************************************************************
 * Project: TMVA - a Root-integrated toolkit for multivariate data analysis       *
 * Package: TMVA                                                                  *
 * Class  : Optimizer                                                             *
 * Web    : http://tmva.sourceforge.net                                           *
 *                                                                                *
 * Description: The Optimizer takes care of "scanning" the different tuning       *
 *              parameters in order to find the best set of tuning paraemters     *
 *              which will be used in the end                                     *
 *                                                                                *
 * Authors (alphabetical):                                                        *
 *      Helge Voss      <Helge.Voss@cern.ch>     - MPI-K Heidelberg, Germany      *
 *                                                                                *
 * Copyright (c) 2005:                                                            *
 *      CERN, Switzerland                                                         * 
 *      MPI-K Heidelberg, Germany                                                 * 
 *                                                                                *
 * Redistribution and use in source and binary forms, with or without             *
 * modification, are permitted according to the terms listed in LICENSE           *
 * (http://ttmva.sourceforge.net/LICENSE)                                         *
 **********************************************************************************/

#ifndef ROOT_TMVA_Optimizer
#define ROOT_TMVA_Optimizer


#ifndef ROOT_Rtypes
#include "Rtypes.h"
#endif

#ifndef ROOT_TString
#include "TString.h"
#endif

#ifndef ROOT_TMVA_MethodBase
#include "TMVA/MethodBase.h"
#endif

#ifndef ROOT_TMVA_DataSet
#include "TMVA/DataSet.h"
#endif

#ifndef ROOT_TMVA_IFitterTarget
#ifndef ROOT_IFitterTarget
#include "IFitterTarget.h"
#endif
#endif

#ifndef ROOT_TH1
#include "TH1.h"
#endif

namespace TMVA {

   class Optimizer : public IFitterTarget  {
      
   public:
      
      //default constructor
      Optimizer(MethodBase * const method, TString fomType="Separation");
      
      // destructor
      virtual ~Optimizer();
      // could later be changed to be set via option string... 
      // but for now it's impler like this
      void optimize(TString optimizationType = "GA"); 
      
      void optimizeScan();
      void optimizeFit(TString optimizationFitType);

      Double_t EstimatorFunction( std::vector<Double_t> & );

      Double_t GetFOM();
      
      MethodBase* GetMethod(){return fMethod;}
      
   private:

      std::vector<Float_t> fFOMvsIter; // graph showing the develompment of the Figure Of Merit values during the fit

      std::map< std::vector<Double_t> , Double_t>  fAlreadyTrainedParCombination; // save parameters for which the FOM is already known (GA seems to evaluate the same parameters several times)

     void GetMVADists();
     Double_t GetSeparation();
     Double_t GetROCIntegral();
     Double_t GetSigEffAt( Double_t bkgEff = 0.1);
     
     
     MethodBase* const fMethod; // The MVA method to be evaluated
     TString           fFOMType;    // the FOM type (Separation, ROC integra.. whaeter you implemented..
     
     TH1D             *fMvaSig; // MVA distrituion for signal events, used for spline fit
     TH1D             *fMvaBkg; // MVA distrituion for bakgr. events, used for spline fit
     
     TH1D             *fMvaSigFineBin; // MVA distrituion for signal events
     TH1D             *fMvaBkgFineBin; // MVA distrituion for bakgr. events

      ClassDef(Optimizer,0) // Interface to different separation critiera used in training algorithms
   };
} // namespace TMVA

#endif
