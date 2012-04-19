// @(#)root/roostats:$Id: FrequentistCalculator.cxx 37084 2010-11-29 21:37:13Z moneta $
// Author: Kyle Cranmer, Sven Kreiss   23/05/10
/*************************************************************************
 * Copyright (C) 1995-2008, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

/**
Does a frequentist hypothesis test. Nuisance parameters are fixed to their
MLEs.
*/

#include "RooStats/FrequentistCalculator.h"
#include "RooStats/ToyMCSampler.h"
#include "RooMinuit.h"
#include "RooProfileLL.h"


ClassImp(RooStats::FrequentistCalculator)

using namespace RooStats;

void FrequentistCalculator::PreHook() const {
   if (fFitInfo != NULL) {
      delete fFitInfo;
      fFitInfo = NULL;
   }
   if (fStoreFitInfo) {
      fFitInfo = new RooArgSet();
   }
}

void FrequentistCalculator::PostHook() const {
}

int FrequentistCalculator::PreNullHook(RooArgSet *parameterPoint, double obsTestStat) const {

   // ****** any TestStatSampler ********

   // create profile keeping everything but nuisance parameters fixed
   RooArgSet * allParams = fNullModel->GetPdf()->getParameters(*fData);
   RemoveConstantParameters(allParams);

   oocoutI((TObject*)0,InputArguments) << "Profiling conditional MLEs for Null." << endl;
   // note: making nll or profile class variables can only be done in the constructor
   // as all other hooks are const (which has to be because GetHypoTest is const). However,
   // when setting it only in constructor, they would have to be changed every time SetNullModel
   // or SetAltModel is called. Simply put, converting them into class variables breaks
   // encapsulation.

   RooArgSet allButNuisance(*allParams);
   if( fNullModel->GetNuisanceParameters() ) allButNuisance.remove(*fNullModel->GetNuisanceParameters());
   if( fConditionalMLEsNull ) {
      oocoutI((TObject*)0,InputArguments) << "Using given conditional MLEs for Null." << endl;
      *allParams = *fConditionalMLEsNull;
      allButNuisance.add( *fConditionalMLEsNull );
   }
   RooFit::MsgLevel msglevel = RooMsgService::instance().globalKillBelow();
   RooMsgService::instance().setGlobalKillBelow(RooFit::FATAL);

   RooAbsReal* nll = fNullModel->GetPdf()->createNLL(*const_cast<RooAbsData*>(fData), RooFit::CloneData(kFALSE), RooFit::Constrain(*allParams));
   RooProfileLL* profile = dynamic_cast<RooProfileLL*>(nll->createProfile(allButNuisance));
   profile->getVal(); // this will do fit and set nuisance parameters to profiled values
   
   // Hack to extract a RooFitResult
   if (fStoreFitInfo) {
	   RooMinuit *minuitUsed = profile->minuit();
	   RooFitResult *result = minuitUsed->save();
	   fFitInfo->addOwned(*DetailedOutputAggregator::GetAsArgSet(result, "fit0_"));
	   delete result;
   }

   // add nuisance parameters to parameter point
   if(fNullModel->GetNuisanceParameters())
      parameterPoint->add(*fNullModel->GetNuisanceParameters());

   delete profile;
   delete nll;

   delete allParams;
   RooMsgService::instance().setGlobalKillBelow(msglevel);



   // ***** ToyMCSampler specific *******

   // check whether TestStatSampler is a ToyMCSampler
   ToyMCSampler *toymcs = dynamic_cast<ToyMCSampler*>(GetTestStatSampler());
   if(toymcs) {
      oocoutI((TObject*)0,InputArguments) << "Using a ToyMCSampler. Now configuring for Null." << endl;

      // variable number of toys
      if(fNToysNull >= 0) toymcs->SetNToys(fNToysNull);

      // set the global observables to be generated by the ToyMCSampler
      toymcs->SetGlobalObservables(*fNullModel->GetGlobalObservables());

      // adaptive sampling
      if(fNToysNullTail) {
         oocoutI((TObject*)0,InputArguments) << "Adaptive Sampling" << endl;
         if(GetTestStatSampler()->GetTestStatistic()->PValueIsRightTail()) {
            toymcs->SetToysRightTail(fNToysNullTail, obsTestStat);
         }else{
            toymcs->SetToysLeftTail(fNToysNullTail, obsTestStat);
         }
      }else{
         toymcs->SetToysBothTails(0, 0, obsTestStat); // disable adaptive sampling
      }

      GetNullModel()->LoadSnapshot();
   }

   return 0;
}


int FrequentistCalculator::PreAltHook(RooArgSet *parameterPoint, double obsTestStat) const {

   // ****** any TestStatSampler ********

   // create profile keeping everything but nuisance parameters fixed
   RooArgSet * allParams = fAltModel->GetPdf()->getParameters(*fData);
   RemoveConstantParameters(allParams);

   oocoutI((TObject*)0,InputArguments) << "Profiling conditional MLEs for Alt." << endl;

   RooArgSet allButNuisance(*allParams);
   if( fAltModel->GetNuisanceParameters() ) allButNuisance.remove(*fAltModel->GetNuisanceParameters());
   if( fConditionalMLEsAlt ) {
      oocoutI((TObject*)0,InputArguments) << "Using given conditional MLEs for Alt." << endl;
      *allParams = *fConditionalMLEsAlt;
      allButNuisance.add( *fConditionalMLEsAlt );
   }
   RooFit::MsgLevel msglevel = RooMsgService::instance().globalKillBelow();
   RooMsgService::instance().setGlobalKillBelow(RooFit::FATAL);
   
   RooAbsReal* nll = fAltModel->GetPdf()->createNLL(*const_cast<RooAbsData*>(fData), RooFit::CloneData(kFALSE), RooFit::Constrain(*allParams));
   RooProfileLL* profile = dynamic_cast<RooProfileLL*>(nll->createProfile(allButNuisance));
   profile->getVal(); // this will do fit and set nuisance parameters to profiled values

   // Hack to extract a RooFitResult
   if (fStoreFitInfo) {
	   RooMinuit *minuitUsed = profile->minuit();
	   RooFitResult *result = minuitUsed->save();
	   fFitInfo->addOwned(*DetailedOutputAggregator::GetAsArgSet(result, "fit1_"));
	   delete result;
   }

   // add nuisance parameters to parameter point
   if(fAltModel->GetNuisanceParameters())
      parameterPoint->add(*fAltModel->GetNuisanceParameters());
   delete profile;
   delete nll;
   
   delete allParams;
   RooMsgService::instance().setGlobalKillBelow(msglevel);




   // ***** ToyMCSampler specific *******

   // check whether TestStatSampler is a ToyMCSampler
   ToyMCSampler *toymcs = dynamic_cast<ToyMCSampler*>(GetTestStatSampler());
   if(toymcs) {
      oocoutI((TObject*)0,InputArguments) << "Using a ToyMCSampler. Now configuring for Alt." << endl;

      // variable number of toys
      if(fNToysAlt >= 0) toymcs->SetNToys(fNToysAlt);

      // set the global observables to be generated by the ToyMCSampler
      toymcs->SetGlobalObservables(*fAltModel->GetGlobalObservables());

      // adaptive sampling
      if(fNToysAltTail) {
         oocoutI((TObject*)0,InputArguments) << "Adaptive Sampling" << endl;
         if(GetTestStatSampler()->GetTestStatistic()->PValueIsRightTail()) {
            toymcs->SetToysLeftTail(fNToysAltTail, obsTestStat);
         }else{
            toymcs->SetToysRightTail(fNToysAltTail, obsTestStat);
         }
      }else{
         toymcs->SetToysBothTails(0, 0, obsTestStat); // disable adaptive sampling
      }

   }

   return 0;
}




