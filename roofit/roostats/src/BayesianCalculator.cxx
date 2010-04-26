// @(#)root/roostats:$Id$
// Author: Kyle Cranmer, Lorenzo Moneta, Gregory Schott, Wouter Verkerke
/*************************************************************************
 * Copyright (C) 1995-2008, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

/**
   BayesianCalculator class
**/

// include other header files

#include "RooAbsFunc.h"
#include "RooAbsReal.h"
#include "RooRealVar.h"
#include "RooArgSet.h"
#include "RooBrentRootFinder.h"
#include "RooFormulaVar.h"
#include "RooGenericPdf.h"
#include "RooPlot.h"
#include "RooProdPdf.h"

// include header file of this class 
#include "RooStats/BayesianCalculator.h"
#include "RooStats/ModelConfig.h"

#include "TAxis.h"

ClassImp(RooStats::BayesianCalculator)

namespace RooStats { 


BayesianCalculator::BayesianCalculator() :
  fData(0),
  fPdf(0),
  fPriorPOI(0),
  fProductPdf (0), fLogLike(0), fLikelihood (0), fIntegratedLikelihood (0), fPosteriorPdf(0), 
  fLower(0), fUpper(0), fValidInterval(false),
  fSize(0.05)
{
   // default constructor
}

BayesianCalculator::BayesianCalculator( /* const char* name,  const char* title, */						   
						    RooAbsData& data,
                                                    RooAbsPdf& pdf,
						    const RooArgSet& POI,
						    RooAbsPdf& priorPOI,
						    const RooArgSet* nuisanceParameters ) :
   //TNamed( TString(name), TString(title) ),
  fData(&data),
  fPdf(&pdf),
  fPOI(POI),
  fPriorPOI(&priorPOI),
  fProductPdf (0), fLogLike(0), fLikelihood (0), fIntegratedLikelihood (0), fPosteriorPdf(0),
  fLower(0), fUpper(0), fValidInterval(false),
  fSize(0.05)
{
   // constructor
   if (nuisanceParameters) fNuisanceParameters.add(*nuisanceParameters); 
}

BayesianCalculator::BayesianCalculator( RooAbsData& data,
                       ModelConfig & model) : 
   fData(&data), 
   fPdf(model.GetPdf()),
   fPriorPOI( model.GetPriorPdf()),
   fProductPdf (0), fLogLike(0), fLikelihood (0), fIntegratedLikelihood (0), fPosteriorPdf(0),
   fLower(0), fUpper(0), fValidInterval(false),
   fSize(0.05)
{
   // constructor from Model Config
   SetModel(model);
}


BayesianCalculator::~BayesianCalculator()
{
   // destructor
   ClearAll(); 
}

void BayesianCalculator::ClearAll() const { 
   // clear cached pdf objects
   if (fProductPdf) delete fProductPdf; 
   if (fLogLike) delete fLogLike; 
   if (fLikelihood) delete fLikelihood; 
   if (fIntegratedLikelihood) delete fIntegratedLikelihood; 
   if (fPosteriorPdf) delete fPosteriorPdf;      
   fPosteriorPdf = 0; 
   fProductPdf = 0;
   fLogLike = 0; 
   fLikelihood = 0; 
   fIntegratedLikelihood = 0; 
   fLower = 0;
   fUpper = 0;
   fValidInterval = false;
}

void BayesianCalculator::SetModel(const ModelConfig & model) {
   // set the model
   fPdf = model.GetPdf();
   fPriorPOI =  model.GetPriorPdf(); 
   // assignment operator = does not do a real copy the sets (must use add method) 
   fPOI.removeAll();
   fNuisanceParameters.removeAll();
   if (model.GetParametersOfInterest()) fPOI.add( *(model.GetParametersOfInterest()) );
   if (model.GetNuisanceParameters())  fNuisanceParameters.add( *(model.GetNuisanceParameters() ) );

   // invalidate the cached pointers
   ClearAll(); 
}

   RooArgSet* BayesianCalculator::GetMode(RooArgSet* /* parameters */) const
{
  return 0;
}

RooAbsPdf* BayesianCalculator::GetPosteriorPdf() const
{
   // get posterior pdf  

   // run some checks
   if (!fPdf ) return 0; 
   if (!fPriorPOI) { 
      std::cerr << "BayesianCalculator::GetPosteriorPdf - missing prior pdf" << std::endl;
   }
   if (fPOI.getSize() == 0) return 0; 
   if (fPOI.getSize() > 1) { 
      std::cerr << "BayesianCalculator::GetPosteriorPdf - current implementation works only on 1D intervals" << std::endl;
      return 0; 
   }


   // create a unique name for the product pdf 
   TString prodName = TString("product_") + TString(fPdf->GetName()) + TString("_") + TString(fPriorPOI->GetName() );   
   fProductPdf = new RooProdPdf(prodName,"",RooArgList(*fPdf,*fPriorPOI));

   RooArgSet* constrainedParams = fProductPdf->getParameters(*fData);

   // use RooFit::Constrain() to make product of likelihood with prior pdf
   fLogLike = fProductPdf->createNLL(*fData, RooFit::Constrain(*constrainedParams) );

   TString likeName = TString("likelihood_") + TString(fProductPdf->GetName());   
   fLikelihood = new RooFormulaVar(likeName,"exp(-@0)",RooArgList(*fLogLike));
   RooAbsReal * plike = fLikelihood; 
   if (fNuisanceParameters.getSize() > 0) { 
      fIntegratedLikelihood = fLikelihood->createIntegral(fNuisanceParameters);
      plike = fIntegratedLikelihood; 
   }

   // create a unique name on the posterior from the names of the components
   TString posteriorName = this->GetName() + TString("_posteriorPdf_") + plike->GetName(); 
   fPosteriorPdf = new RooGenericPdf(posteriorName,"@0",*plike);

   delete constrainedParams;

   return fPosteriorPdf;
}


RooPlot* BayesianCalculator::GetPosteriorPlot() const
{
  /// return a RooPlot with the posterior PDF and the credibility region

  if (!fPosteriorPdf) GetPosteriorPdf();
  if (!fValidInterval) GetInterval();

  RooAbsRealLValue* poi = dynamic_cast<RooAbsRealLValue*>( fPOI.first() );
  assert(poi);

   RooPlot* plot = poi->frame();

   plot->SetTitle(TString("Posterior probability of parameter \"")+TString(poi->GetName())+TString("\""));  
   fPosteriorPdf->plotOn(plot,RooFit::Range(fLower,fUpper,kFALSE),RooFit::VLines(),RooFit::DrawOption("F"),RooFit::MoveToBack(),RooFit::FillColor(kGray));
   fPosteriorPdf->plotOn(plot);
   plot->GetYaxis()->SetTitle("posterior probability");
   
   return plot; 
}


SimpleInterval* BayesianCalculator::GetInterval() const
{
   /// computes and returns a SimpleInterval with the lower/upper limit on the scanned variable
   if (fValidInterval) 
      std::cout << "BayesianCalculator::GetInterval:" 
                << "Warning : recomputing interval for the same CL and same model" << std::endl;

   RooRealVar* poi = dynamic_cast<RooRealVar*>( fPOI.first() ); 
   assert(poi);

   if (!fPosteriorPdf) fPosteriorPdf = (RooAbsPdf*) GetPosteriorPdf();

   RooAbsReal* cdf = fPosteriorPdf->createCdf(fPOI);

   RooAbsFunc* cdf_bind = cdf->bindVars(fPOI,&fPOI);
   RooBrentRootFinder brf(*cdf_bind);
   brf.setTol(0.00005); // precision

   double tmpVal = poi->getVal(); // patch because findRoot changes the value of poi

   double y = fSize/2;
   brf.findRoot(fLower,poi->getMin(),poi->getMax(),y);

   y=1-fSize/2;
   bool ret = brf.findRoot(fUpper,poi->getMin(),poi->getMax(),y);
   if (!ret) std::cout << "BayesianCalculator::GetInterval: Warning:"
                       << "Error returned from Root finder, estimated interval is not fully correct" 
                       << std::endl;

   poi->setVal(tmpVal); // patch: restore the original value of poi

   delete cdf_bind;
   delete cdf;
   fValidInterval = true; 

   TString interval_name = TString("BayesianInterval_a") + TString(this->GetName());
   SimpleInterval * interval = new SimpleInterval(interval_name,*poi,fLower,fUpper,ConfidenceLevel());
   interval->SetTitle("SimpleInterval from BayesianCalculator");

   return interval;
}

} // end namespace RooStats

