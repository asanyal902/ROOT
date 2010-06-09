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

#include "Math/IntegratorMultiDim.h"
#include "Math/RootFinder.h"
#include "RooFunctor.h"

#include "TAxis.h"

#include <cmath>

#include "TRandom.h"

ClassImp(RooStats::BayesianCalculator)

namespace RooStats { 


BayesianCalculator::BayesianCalculator() :
  fData(0),
  fPdf(0),
  fPriorPOI(0),
  fProductPdf (0), fLogLike(0), fLikelihood (0), fIntegratedLikelihood (0), fPosteriorPdf(0), 
  fLower(0), fUpper(0),
  fBrfPrecision(0.00005),
  fValidInterval(false),
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
  fLower(0), fUpper(0),
  fBrfPrecision(0.00005),
  fValidInterval(false),  
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
   fLower(0), fUpper(0),
   fBrfPrecision(0.00005),
   fValidInterval(false),
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
  /// Returns the value of the parameters for the point in
  /// parameter-space that is the most likely.
  // Should cover multi-dimensional cases...
  // How do we do if there are points that are equi-probable?

  return 0;
}


RooAbsReal* BayesianCalculator::GetPosteriorFunction() const
{
  /// build and return the posterior function (not normalized)

   // run some sanity checks
   if (!fPdf ) {
     std::cerr << "BayesianCalculator::GetPosteriorPdf - missing pdf model" << std::endl;
     return 0;
   }
   if (!fPriorPOI) { 
      std::cerr << "BayesianCalculator::GetPosteriorPdf - missing prior pdf" << std::endl;
   }
   if (fPOI.getSize() == 0) {
     std::cerr << "BayesianCalculator::GetPosteriorPdf - missing parameter of interest" << std::endl;
     return 0;
   }
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


   // fLogLike is actually the product of the  Likelihood  with all the prior. 
   // we need to integrate 1

   // 


   TString likeName = TString("likelihood_") + TString(fProductPdf->GetName());   
   fLikelihood = new RooFormulaVar(likeName,"exp(-@0)",RooArgList(*fLogLike));
   fIntegratedLikelihood = fLikelihood; 
   if (fNuisanceParameters.getSize() > 0) { 
      fIntegratedLikelihood = fLikelihood->createIntegral(fNuisanceParameters);
   }

//for debuggiung
//    RooArgSet allParams; 
//    allParams.add(fPOI);
//    allParams.add(fNuisanceParameters);
//    RooAbsReal * igll =  fLikelihood->createIntegral(allParams);
//    double norm = igll->getVal();
//    std::cout << "norm integral = " << norm << std::endl;

   delete constrainedParams;

   return fIntegratedLikelihood; 
}

RooAbsPdf* BayesianCalculator::GetPosteriorPdf() const
{

   if (!fIntegratedLikelihood) GetPosteriorFunction();

  /// build and return the posterior pdf (i.e posterior function normalized to all range of poi
   RooAbsReal * plike = fIntegratedLikelihood; 
   // create a unique name on the posterior from the names of the components
   TString posteriorName = this->GetName() + TString("_posteriorPdf_") + plike->GetName(); 
   fPosteriorPdf = new RooGenericPdf(posteriorName,"@0",*plike);


   return fPosteriorPdf;
}


RooPlot* BayesianCalculator::GetPosteriorPlot(bool norm, double precision ) const
{
  /// return a RooPlot with the posterior  and the credibility region

   RooAbsReal * posterior = fIntegratedLikelihood; 
   if (norm) posterior = fPosteriorPdf; 
   if (!posterior) { 
      posterior = GetPosteriorFunction();
      if (norm) posterior = GetPosteriorPdf();
   }
   if (!posterior) return 0;

   if (!fValidInterval) GetInterval();

   RooAbsRealLValue* poi = dynamic_cast<RooAbsRealLValue*>( fPOI.first() );
   assert(poi);

   RooPlot* plot = poi->frame();

   plot->SetTitle(TString("Posterior probability of parameter \"")+TString(poi->GetName())+TString("\""));  
   posterior->plotOn(plot,RooFit::Range(fLower,fUpper,kFALSE),RooFit::VLines(),RooFit::DrawOption("F"),RooFit::MoveToBack(),RooFit::FillColor(kGray),RooFit::Precision(precision));
   posterior->plotOn(plot);
   plot->GetYaxis()->SetTitle("posterior function");
   
   return plot; 
}


struct  LikelihoodFunction { 
   LikelihoodFunction(RooFunctor & f) : fFunc(f) {}

   double operator() (const double *x ) const { 
      double nll = fFunc(x);
      double likelihood =  std::exp(-nll); 
      return likelihood; 
   }
   RooFunctor & fFunc; 
};

struct PosteriorCdfFunction { 
   PosteriorCdfFunction(ROOT::Math::IntegratorMultiDim & ig, const double * xmin, double * xmax) : 
      fIntegrator(ig), fXmin(xmin), fXmax(xmax), fNorm(1.0), fOffset(0.0) {
      // compute first the normalization with  the poi 
      fNorm = (*this)(xmax[0] );  
      std::cout << "normalization of posterior is " << fNorm << std::endl;
   }

   double operator() (double x) const { 
      // integrate only first parameter 
      fXmax[0] = x;
      double cdf = fIntegrator.Integral(fXmin,fXmax);  
      //std::cout << "un-normalize cdf for mu =  " << x << " =  " << cdf; 
      double normcdf =  cdf/fNorm;  // normalize the cdf 
      //std::cout << "  normalized is " << normcdf << std::endl; 
      return normcdf - fOffset;  // apply an offset (for finding the roots) 
   }

   ROOT::Math::IntegratorMultiDim & fIntegrator; 
   const double * fXmin; 
   mutable double * fXmax; 
   double fNorm; 
   double fOffset;
   
};

SimpleInterval* BayesianCalculator::GetInterval() const
{
  /// returns a SimpleInterval with lower and upper bounds on the
  /// parameter of interest. Applies the central ordering rule to
  /// compute the credibility interval. Covers only the case with one
  /// single parameter of interest

   if (fValidInterval) 
      std::cout << "BayesianCalculator::GetInterval:" 
                << "Warning : recomputing interval for the same CL and same model" << std::endl;

   RooRealVar* poi = dynamic_cast<RooRealVar*>( fPOI.first() ); 
   assert(poi);

   if (!fPosteriorPdf) fPosteriorPdf = (RooAbsPdf*) GetPosteriorPdf();

   // use integration method if there are nuisance parameters 
   if (fNuisanceParameters.getSize() > 0) { 
   
      // need to remove the constant parameters
      RooArgList bindParams; 
      bindParams.add(fPOI);
      bindParams.add(fNuisanceParameters);
      
//    std::cout << "DEBUG - NLL = " << fLogLike->getVal() << std::endl; 
      
      
      RooFunctor functor_nll(*fLogLike, bindParams, RooArgList());

   
      // compute the intergal of the exp(-nll) function
      LikelihoodFunction fll(functor_nll);
      
      const ROOT::Math::IntegrationMultiDim::Type integType = ROOT::Math::IntegrationMultiDim::kADAPTIVE; 

// #ifdef R__HAS_MATHMORE
//       integType = IntegrationMultiDim::kVEGAS; 
// #endif
      
      ROOT::Math::IntegratorMultiDim ig(integType); 
      ig.SetFunction(fll,bindParams.getSize()); 
      
      std::vector<double> pmin(bindParams.getSize());
      std::vector<double> pmax(bindParams.getSize());
      std::vector<double> par(bindParams.getSize());
      for (unsigned int i = 0; i < pmin.size(); ++i) { 
         RooRealVar & var = (RooRealVar &) bindParams[i]; 
         pmin[i] = var.getMin(); 
         pmax[i] = var.getMax();
         par[i] = var.getVal();
      } 
      
//    std::cout << "DEBUG - FUNCTOR - value " << functor_nll(&par[0]) << std::endl;
//    std::cout << "DEBUG - LL - value " << fll(&par[0]) << std::endl;
      
//    for (int i = 0; i < par.size(); ++i) { 
//       par[i] = gRandom->Uniform(pmin[i],pmax[i]);
//       RooRealVar & var = (RooRealVar &) bindParams[i]; 
//       var.setVal(par[i] );
//    }

      bindParams.Print("V");
//    std::cout << "DEBUG - NLL = " << fLogLike->getVal() << std::endl;    
//    std::cout << "DEBUG - FUNCTOR - value " << functor_nll(&par[0]) << std::endl;
//    std::cout << "DEBUG - LL - value " << fll(&par[0]) << std::endl;


      PosteriorCdfFunction cdf(ig, &pmin[0], &pmax[0] ); 

   
      //std::cout << "posterior cdf at current param value mu " << par[0] << "  = " << cdf(par[0]) << std::endl;

   //find the roots 
#ifdef R__HAS_MATHMORE
      ROOT::Math::RootFinder::EType rfType = ROOT::Math::RootFinder::kGSL_BRENT; 
#else 
      ROOT::Math::RootFinder::EType rfType = ROOT::Math::RootFinder::kBRENT; 
#endif
      ROOT::Math::RootFinder rf(rfType); 
      cdf.fOffset = fSize/2;
      bool ok = false; 
      ok = rf.Solve(cdf, poi->getMin(),poi->getMax() , 200,1.E-4, 1.E-4); 
      if (!ok) std::cout << "Error from ROOT finder when searching lower limit !!! " << std::endl;
      fLower = rf.Root(); 
      cdf.fOffset = 1-fSize/2;
      ok = rf.Solve(cdf, fLower,poi->getMax() , 200,1.E-4, 1.E-4); 
      if (!ok) std::cout << "Error from ROOT finder when searching upper limit !!! " << std::endl;
      fUpper = rf.Root(); 
   }

   // case of no nuisance - just use createCdf
   else { 
      RooAbsReal* cdf = fPosteriorPdf->createCdf(fPOI,RooFit::ScanParameters(100,2));
      //RooAbsReal* cdf = fPosteriorPdf->createCdf(fPOI,RooFit::ScanNoCdf());

      RooAbsFunc* cdf_bind = cdf->bindVars(fPOI,&fPOI);
      RooBrentRootFinder brf(*cdf_bind);
      brf.setTol(fBrfPrecision); // set the brf precision
      
      double tmpVal = poi->getVal();  // patch used because findRoot changes the value of poi
   
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
   }

   fValidInterval = true; 

   TString interval_name = TString("BayesianInterval_a") + TString(this->GetName());
   SimpleInterval * interval = new SimpleInterval(interval_name,*poi,fLower,fUpper,ConfidenceLevel());
   interval->SetTitle("SimpleInterval from BayesianCalculator");

   return interval;
}

} // end namespace RooStats

