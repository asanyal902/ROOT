// @(#)root/roostats:$Id: MCMCCalculator.h 26805 2009-06-17 14:31:02Z kbelasco $
// Author: Kevin Belasco        17/06/2009
/*************************************************************************
 * Copyright (C) 1995-2008, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#ifndef ROOSTATS_MCMCCalculator
#define ROOSTATS_MCMCCalculator

#ifndef ROOT_Rtypes
#include "Rtypes.h"
#endif

#include "RooAbsPdf.h"
#include "RooAbsData.h"
#include "RooArgSet.h"
#include "RooWorkspace.h"
#include "RooStats/ProposalFunction.h"
#include "RooStats/IntervalCalculator.h"
#include "RooStats/MCMCInterval.h"

namespace RooStats {

   class MCMCCalculator : public IntervalCalculator {

   public:
      MCMCCalculator();

      MCMCCalculator(RooWorkspace& ws, RooAbsData& data, RooAbsPdf& pdf,
         RooArgSet& paramsOfInterest, ProposalFunction& proposalFunction,
         Int_t numIters, Double_t size = 0.05);

      MCMCCalculator(RooAbsData& data, RooAbsPdf& pdf,
         RooArgSet& paramsOfInterest, ProposalFunction& proposalFunction,
         Int_t numIters, Double_t size = 0.05);

      virtual ~MCMCCalculator()
      {
         if (fOwnsWorkspace)
            delete fWS;
      }
    
      // Main interface to get a ConfInterval
      virtual MCMCInterval* GetInterval() const;

      // Get the size of the test (eg. rate of Type I error)
      virtual Double_t Size() const {return fSize;}
      // Get the Confidence level for the test
      virtual Double_t ConfidenceLevel() const {return 1.-fSize;}

      // set a workspace that owns all the necessary components for the analysis
      virtual void SetWorkspace(RooWorkspace & ws)
      {
         if (!fWS)
            fWS = &ws;
         else {
            //RooMsgService::instance().setGlobalKillBelow(RooMsgService::ERROR) ;
            fWS->merge(ws);
            //RooMsgService::instance().setGlobalKillBelow(RooMsgService::DEBUG) ;
         }
      }

      virtual void SetData(const char* data) { fDataName = data; }

      // Set the DataSet, add to the the workspace if not already there
      virtual void SetData(RooAbsData& data)
      {      
         if (!fWS) {
            fWS = new RooWorkspace();
            fOwnsWorkspace = true; 
         }
         if (! fWS->data(data.GetName()) ) {
            //RooMsgService::instance().setGlobalKillBelow(RooMsgService::ERROR) ;
            fWS->import(data);
            //RooMsgService::instance().setGlobalKillBelow(RooMsgService::DEBUG) ;
         }
         SetData(data.GetName());
      }

      virtual void SetPdf(const char* name) { fPdfName = name; }

      // Set the Pdf, add to the the workspace if not already there
      virtual void SetPdf(RooAbsPdf& pdf)
      {
         if (!fWS) 
            fWS = new RooWorkspace();
         if (! fWS->pdf( pdf.GetName() ))
         {
            //RooMsgService::instance().setGlobalKillBelow(RooMsgService::ERROR) ;
            fWS->import(pdf);
            //RooMsgService::instance().setGlobalKillBelow(RooMsgService::DEBUG) ;
         }
         SetPdf(pdf.GetName());
      }

      // specify the parameters of interest in the interval
      virtual void SetParameters(RooArgSet& set) {fPOI = &set;}
      // specify the nuisance parameters (eg. the rest of the parameters)
      virtual void SetNuisanceParameters(RooArgSet& set) {fNuisParams = &set;}
      // set the size of the test (rate of Type I error) ( Eg. 0.05 for a 95% Confidence Interval)
      virtual void SetTestSize(Double_t size) {fSize = size;}
      // set the confidence level for the interval (eg. 0.95 for a 95% Confidence Interval)
      virtual void SetConfidenceLevel(Double_t cl) {fSize = 1.-cl;}
      // set the proposal function for suggesting new points for the MCMC
      virtual void SetProposalFunction(ProposalFunction& proposalFunction)
      { fPropFunc = &proposalFunction; }
      // set the number of iterations to run the metropolis algorithm
      virtual void SetNumIters(Int_t numIters)
      { fNumIters = numIters; }

   protected:
      Double_t fSize; // size of the test (eg. specified rate of Type I error)
      RooWorkspace* fWS; // owns all the components used by the calculator
      RooArgSet* fPOI; // parameters of interest for interval
      RooArgSet* fNuisParams; // nuisance parameters for interval
      Bool_t fOwnsWorkspace; // whether we own the workspace
      ProposalFunction* fPropFunc; // Proposal function for MCMC integration
      const char* fPdfName; // name of common PDF in workspace
      const char* fDataName; // name of data set in workspace
      Int_t fNumIters; // number of iterations to run metropolis algorithm

      ClassDef(MCMCCalculator,1) // Markov Chain Monte Carlo calculator for Bayesian credible intervals
   };
}


#endif
