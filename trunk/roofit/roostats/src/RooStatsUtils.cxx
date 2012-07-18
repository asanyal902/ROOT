// @(#)root/roostats:$Id$
// Author: Kyle Cranmer   28/07/2008

/*************************************************************************
 * Copyright (C) 1995-2008, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

/////////////////////////////////////////
// RooStats
//
// namespace for classes and functions of the RooStats package
/////////////////////////////////////////
#include "Rtypes.h"

#if !defined(R__ALPHA) && !defined(R__SOLARIS) && !defined(R__ACC) && !defined(R__FBSD)
NamespaceImp(RooStats)
#endif

#include "TTree.h"

#include "RooUniform.h"
#include "RooProdPdf.h"
#include "RooExtendPdf.h"
#include "RooSimultaneous.h"
#include "RooStats/ModelConfig.h"
#include "RooStats/RooStatsUtils.h"
#include <typeinfo>

using namespace std;

// this file is only for the documentation of RooStats namespace

namespace RooStats { 

   void FactorizePdf(const RooArgSet &observables, RooAbsPdf &pdf, RooArgList &obsTerms, RooArgList &constraints) {
   // utility function to factorize constraint terms from a pdf 
   // (from G. Petrucciani)
      const std::type_info & id = typeid(pdf);
      if (id == typeid(RooProdPdf)) {
         RooProdPdf *prod = dynamic_cast<RooProdPdf *>(&pdf);
         RooArgList list(prod->pdfList());
         for (int i = 0, n = list.getSize(); i < n; ++i) {
            RooAbsPdf *pdfi = (RooAbsPdf *) list.at(i);
            FactorizePdf(observables, *pdfi, obsTerms, constraints);
         }
      } else if (id == typeid(RooExtendPdf)) {
         TIterator *iter = pdf.serverIterator(); 
         // extract underlying pdf which is extended; first server is the pdf; second server is the number of events variable
         RooAbsPdf *updf = dynamic_cast<RooAbsPdf *>(iter->Next());
         assert(updf != 0);
         delete iter;
         FactorizePdf(observables, *updf, obsTerms, constraints);
      } else if (id == typeid(RooSimultaneous)) {    //|| id == typeid(RooSimultaneousOpt)) {
         RooSimultaneous *sim  = dynamic_cast<RooSimultaneous *>(&pdf);
         assert(sim != 0);
         RooAbsCategoryLValue *cat = (RooAbsCategoryLValue *) sim->indexCat().clone(sim->indexCat().GetName());
         for (int ic = 0, nc = cat->numBins((const char *)0); ic < nc; ++ic) {
            cat->setBin(ic);
            FactorizePdf(observables, *sim->getPdf(cat->getLabel()), obsTerms, constraints);
         }
         delete cat;
      } else if (pdf.dependsOn(observables)) {
         if (!obsTerms.contains(pdf)) obsTerms.add(pdf);
      } else {
         if (!constraints.contains(pdf)) constraints.add(pdf);
      }
   }


   void FactorizePdf(RooStats::ModelConfig &model, RooAbsPdf &pdf, RooArgList &obsTerms, RooArgList &constraints) {
      // utility function to factorize constraint terms from a pdf 
      // (from G. Petrucciani)
      if (!model.GetObservables() ) { 
         oocoutE((TObject*)0,InputArguments) << "RooStatsUtils::FactorizePdf - invalid input model: missing observables" << endl;
         return;
      }
      return FactorizePdf(*model.GetObservables(), pdf, obsTerms, constraints);
   }


   RooAbsPdf * MakeNuisancePdf(RooAbsPdf &pdf, const RooArgSet &observables, const char *name) { 
      // make a nuisance pdf by factorizing out all constraint terms in a common pdf 
      RooArgList obsTerms, constraints;
      FactorizePdf(observables, pdf, obsTerms, constraints);
      if(constraints.getSize() == 0) {
         oocoutW((TObject *)0, Eval) << "RooStatsUtils::MakeNuisancePdf - no constraints found on nuisance parameters in the input model" << endl;
         return 0;
      } else if(constraints.getSize() == 1) {
         return dynamic_cast<RooAbsPdf *>(constraints.first());
      }
      return new RooProdPdf(name,"", constraints);
   }

   RooAbsPdf * MakeNuisancePdf(const RooStats::ModelConfig &model, const char *name) { 
      // make a nuisance pdf by factorizing out all constraint terms in a common pdf
      if (!model.GetPdf() || !model.GetObservables() ) { 
         oocoutE((TObject*)0, InputArguments) << "RooStatsUtils::MakeNuisancePdf - invalid input model: missing pdf and/or observables" << endl;
         return 0;
      }
      return MakeNuisancePdf(*model.GetPdf(), *model.GetObservables(), name);
   }

   RooAbsPdf * StripConstraints(RooAbsPdf &pdf, const RooArgSet &observables) { 
      const std::type_info & id = typeid(pdf);

      if (id == typeid(RooProdPdf)) {

         RooProdPdf *prod = dynamic_cast<RooProdPdf *>(&pdf);
         RooArgList list(prod->pdfList()); RooArgList newList;

         for (int i = 0, n = list.getSize(); i < n; ++i) {
            RooAbsPdf *pdfi = (RooAbsPdf *) list.at(i);
            RooAbsPdf *newPdfi = StripConstraints(*pdfi, observables);
            if(newPdfi != 0) newList.add(*newPdfi);
         }

         if(newList.getSize() == 0) return 0; // only constraints
         else if(newList.getSize() == 1) return dynamic_cast<RooAbsPdf *>(newList.at(0)); // return single component (no longer a product)
         else return new RooProdPdf(TString::Format("%s_unconstrained", prod->GetName()).Data(),
            TString::Format("%s without constraints", prod->GetTitle()).Data(), newList);

      } else if (id == typeid(RooExtendPdf)) {

         TIterator *iter = pdf.serverIterator(); 
         // extract underlying pdf which is extended; first server is the pdf; second server is the number of events variable
         RooAbsPdf *uPdf = dynamic_cast<RooAbsPdf *>(iter->Next());
         RooAbsReal *extended_term = dynamic_cast<RooAbsReal *>(iter->Next());
         assert(uPdf != 0); assert(extended_term != 0); assert(iter->Next() == 0);
         delete iter;
         
         RooAbsPdf *newUPdf = StripConstraints(*uPdf, observables);
         if(newUPdf == 0) return 0; // only constraints in underlying pdf
         else return new RooExtendPdf(TString::Format("%s_unconstrained", pdf.GetName()).Data(),
            TString::Format("%s without constraints", pdf.GetTitle()).Data(), *newUPdf, *extended_term);
         
      } else if (id == typeid(RooSimultaneous)) {    //|| id == typeid(RooSimultaneousOpt)) {

         RooSimultaneous *sim  = dynamic_cast<RooSimultaneous *>(&pdf); assert(sim != 0);
         RooAbsCategoryLValue *cat = (RooAbsCategoryLValue *) sim->indexCat().Clone(); assert(cat != 0);
         RooArgList pdfList;

         for (int ic = 0, nc = cat->numBins((const char *)0); ic < nc; ++ic) {
            cat->setBin(ic);
            RooAbsPdf *newPdf = StripConstraints(*sim->getPdf(cat->getLabel()), observables);
            if(newPdf == 0) { delete cat; return 0; } // all channels must have observables
            pdfList.add(*newPdf);
         }

         return new RooSimultaneous(TString::Format("%s_unconstrained", sim->GetName()).Data(), 
            TString::Format("%s without constraints", sim->GetTitle()).Data(), pdfList, *cat); 

      } else if (pdf.dependsOn(observables)) {  
         return (RooAbsPdf *) pdf.clone(TString::Format("%s_unconstrained", pdf.GetName()).Data());
      }

      return 0; // just  a constraint term
   }

   RooAbsPdf * MakeUnconstrainedPdf(RooAbsPdf &pdf, const RooArgSet &observables, const char *name = 0) { 
      // make a clone pdf without all constraint terms in a common pdf
      RooAbsPdf * unconstrainedPdf = StripConstraints(pdf, observables);
      if(!unconstrainedPdf) {
         oocoutE((TObject *)0, Eval) << "RooStats::MakeUnconstrainedPdf - invalid observable list passed (not found in original pdf) or certain channels consist only of constraints" << endl;
         return 0;
      }
      if(name != 0) unconstrainedPdf->SetName(name);
      return unconstrainedPdf;   
   }

   RooAbsPdf * MakeUnconstrainedPdf(const RooStats::ModelConfig &model, const char *name = 0) { 
      // make a clone pdf without all constraint terms in a common pdf 
      if(!model.GetPdf() || !model.GetObservables()) {
         oocoutE((TObject *)0, InputArguments) << "RooStatsUtils::MakeUnconstrainedPdf - invalid input model: missing pdf and/or observables" << endl;
         return 0;
      }
      return MakeUnconstrainedPdf(*model.GetPdf(), *model.GetObservables(), name);
   }

   // Helper class for GetAsTTree
   class BranchStore {
      public:
         std::map<TString, Double_t> varVals;
         double inval;

         BranchStore(const vector <TString> &params = vector <TString>(), double _inval = -999.) {
            inval = _inval;
            for(unsigned int i = 0;i<params.size();i++)
               varVals[params[i]] = _inval;
         }

         void AssignToTTree(TTree &myTree) {
            for(std::map<TString, Double_t>::iterator it = varVals.begin();it!=varVals.end();it++) {
               const TString& name = it->first;
               myTree.Branch( name, &varVals[name], TString::Format("%s/D", name.Data()));
            }
         }
         void ResetValues() {
            for(std::map<TString, Double_t>::iterator it = varVals.begin();it!=varVals.end();it++) {
               const TString& name = it->first;
               varVals[name] = inval;
            }
         }
   };

   BranchStore* CreateBranchStore(const RooDataSet& data) {
      if (data.numEntries() == 0) {
         return new BranchStore;
      }
      vector <TString> V;
      const RooArgSet* aset = data.get(0);
      RooAbsArg *arg(0);
      TIterator *it = aset->createIterator();
      for(;(arg = dynamic_cast<RooAbsArg*>(it->Next()));) {
         RooRealVar *rvar = dynamic_cast<RooRealVar*>(arg);
         if (rvar == NULL)
            continue;
         V.push_back(rvar->GetName());
         if (rvar->hasAsymError()) {
            V.push_back(TString::Format("%s_errlo", rvar->GetName()));
            V.push_back(TString::Format("%s_errhi", rvar->GetName()));
         }
         else if (rvar->hasError()) {
            V.push_back(TString::Format("%s_err", rvar->GetName()));
         }
      }
      delete it;
      return new BranchStore(V);
   }

   void FillTree(TTree &myTree, const RooDataSet &data) {
      BranchStore *bs = CreateBranchStore(data);
      bs->AssignToTTree(myTree);

      for(int entry = 0;entry<data.numEntries();entry++) {
         bs->ResetValues();
         const RooArgSet* aset = data.get(entry);
         RooAbsArg *arg(0);
         RooLinkedListIter it = aset->iterator();
         for(;(arg = dynamic_cast<RooAbsArg*>(it.Next()));) {
            RooRealVar *rvar = dynamic_cast<RooRealVar*>(arg);
            if (rvar == NULL)
               continue;
            bs->varVals[rvar->GetName()] = rvar->getValV();
            if (rvar->hasAsymError()) {
               bs->varVals[TString::Format("%s_errlo", rvar->GetName())] = rvar->getAsymErrorLo();
               bs->varVals[TString::Format("%s_errhi", rvar->GetName())] = rvar->getAsymErrorHi();
            }
            else if (rvar->hasError()) {
               bs->varVals[TString::Format("%s_err", rvar->GetName())] = rvar->getError();
            }
         }
         myTree.Fill();
      }
      delete bs;
   }

   TTree * GetAsTTree(TString name, TString desc, const RooDataSet& data) {
      TTree* myTree = new TTree(name, desc);
      FillTree(*myTree, data);
      return myTree;
   }

}
