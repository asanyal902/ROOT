 /***************************************************************************** 
  * Project: RooFit                                                           * 
  *                                                                           * 
  * Copyright (c) 2000-2005, Regents of the University of California          * 
  *                          and Stanford University. All rights reserved.    * 
  *                                                                           * 
  * Redistribution and use in source and binary forms,                        * 
  * with or without modification, are permitted according to the terms        * 
  * listed in LICENSE (http://roofit.sourceforge.net/license.txt)             * 
  *****************************************************************************/ 

//////////////////////////////////////////////////////////////////////////////
//
// BEGIN_HTML
// Class RooBarlowBeestonLL implements the profile likelihood estimator for
// a given likelihood and set of parameters of interest. The value return by 
// RooBarlowBeestonLL is the input likelihood nll minimized w.r.t all nuisance parameters
// (which are all parameters except for those listed in the constructor) minus
// the -log(L) of the best fit. Note that this function is slow to evaluate
// as a MIGRAD minimization step is executed for each function evaluation
// END_HTML
//

#include <stdexcept>
#include <math.h>

#include "Riostream.h" 

#include "RooFit.h"
#include "RooStats/HistFactory/RooBarlowBeestonLL.h" 
#include "RooAbsReal.h" 
#include "RooAbsData.h" 
//#include "RooMinuit.h"
#include "RooMsgService.h"
#include "RooRealVar.h"
#include "RooMsgService.h"
#include "RooNLLVar.h"

#include "RooStats/RooStatsUtils.h"
#include "RooProdPdf.h"
#include "RooCategory.h"
#include "RooSimultaneous.h"
#include "RooArgList.h"
#include "RooAbsCategoryLValue.h"

#include "RooStats/HistFactory/ParamHistFunc.h"
#include "RooStats/HistFactory/HistFactoryModelUtils.h"

using namespace std ;

ClassImp(RooBarlowBeestonLL) 


//_____________________________________________________________________________ 
 RooBarlowBeestonLL::RooBarlowBeestonLL() : 
   RooAbsReal("RooBarlowBeestonLL","RooBarlowBeestonLL"), 
   _nll(), 
//   _obs("paramOfInterest","Parameters of interest",this), 
//  _par("nuisanceParam","Nuisance parameters",this,kFALSE,kFALSE),
  _pdf(NULL), _data(NULL)
{ 
  // Default constructor 
  // Should only be used by proof. 
  //  _piter = _par.createIterator() ; 
  //  _oiter = _obs.createIterator() ; 
} 


//_____________________________________________________________________________
RooBarlowBeestonLL::RooBarlowBeestonLL(const char *name, const char *title, 
				       RooAbsReal& nllIn /*, const RooArgSet& observables*/) :
  RooAbsReal(name,title), 
  _nll("input","-log(L) function",this,nllIn),
  //  _obs("paramOfInterest","Parameters of interest",this),
  //  _par("nuisanceParam","Nuisance parameters",this,kFALSE,kFALSE),
  _pdf(NULL), _data(NULL)
{ 
  // Constructor of profile likelihood given input likelihood nll w.r.t
  // the given set of variables. The input log likelihood is minimized w.r.t
  // to all other variables of the likelihood at each evaluation and the
  // value of the global log likelihood minimum is always subtracted.

  // Determine actual parameters and observables
  /*
  RooArgSet* actualObs = nllIn.getObservables(observables) ;
  RooArgSet* actualPars = nllIn.getParameters(observables) ;

  _obs.add(*actualObs) ;
  _par.add(*actualPars) ;

  delete actualObs ;
  delete actualPars ;

  _piter = _par.createIterator() ;
  _oiter = _obs.createIterator() ;
  */
} 



//_____________________________________________________________________________
RooBarlowBeestonLL::RooBarlowBeestonLL(const RooBarlowBeestonLL& other, const char* name) :  
  RooAbsReal(other,name), 
  _nll("nll",this,other._nll),
  //  _obs("obs",this,other._obs),
  //  _par("par",this,other._par),
  _pdf(NULL), _data(NULL),
  _paramFixed(other._paramFixed)
{ 
  // Copy constructor

  //  _piter = _par.createIterator() ;
  //  _oiter = _obs.createIterator() ;

  // _paramAbsMin.addClone(other._paramAbsMin) ;
  // _obsAbsMin.addClone(other._obsAbsMin) ;
    
} 



//_____________________________________________________________________________
RooBarlowBeestonLL::~RooBarlowBeestonLL()
{
  // Destructor

  // Delete instance of minuit if it was ever instantiated
  // if (_minuit) {
  //   delete _minuit ;
  // }

  
  //delete _piter ;
  //delete _oiter ;
}


//_____________________________________________________________________________
void RooBarlowBeestonLL::BarlowCache::SetBinCenter() const {
  TIterator* iter = bin_center->createIterator() ;
  RooRealVar* var;
  while((var=(RooRealVar*)iter->Next())) {
    RooRealVar* target = (RooRealVar*) observables->find(var->GetName()) ;
    target->setVal(var->getVal()) ;
  }
  delete iter;
}


//_____________________________________________________________________________
void RooBarlowBeestonLL::initializeBarlowCache() {

  if(!_data) {
    std::cout << "Error: Must initialize data before initializing cache" << std::endl;
    throw std::runtime_error("Uninitialized Data");    
  }
  if(!_pdf) {
    std::cout << "Error: Must initialize model pdf before initializing cache" << std::endl;
    throw std::runtime_error("Uninitialized model pdf");    
  }

  // Get the data bin values for all channels and bins
  std::map< std::string, std::vector<double> > ChannelBinDataMap;
  RooStats::getDataValuesForObservables( ChannelBinDataMap, _data, _pdf );

  // Get a list of constraint terms
  RooArgList obsTerms;
  RooArgList constraints;
  RooArgSet* obsSet = _pdf->getObservables(*_data);
  RooStats::FactorizePdf(*obsSet, *_pdf, obsTerms, constraints);

  // Get the channels for this pdf
  RooArgSet* channels = new RooArgSet();
  RooArgSet* channelsWithConstraints = new RooArgSet();
  RooStats::getChannelsFromModel( _pdf, channels, channelsWithConstraints );
  
  // Loop over the channels
  RooSimultaneous* simPdf = (RooSimultaneous*) _pdf;
  RooCategory* channelCat = (RooCategory*) (&simPdf->indexCat());
  TIterator* iter = channelCat->typeIterator() ;
  RooCatType* tt = NULL;
  while((tt=(RooCatType*) iter->Next())) {
    /*
      std::string ChannelName = tt->GetName();
      
      HHChannel_hh_edit
      
      TIterator* iter_channels = channelsWithConstraints->createIterator();
      RooAbsPdf* channelPdf=NULL;
      while(( channelPdf=(RooAbsPdf*)iter_channels->Next()  )) {
      
      std::string channel_name = RooStats::channelNameFromPdf( channelPdf );
    */

    // Warning: channel cat name is not necesarily the same name
    // as the pdf's (for example, if someone does edits)
    RooAbsPdf* channelPdf = simPdf->getPdf(tt->GetName());
    std::string channel_name = channelPdf->GetName();
    std::cout << "Sim Pdf: Channel Cat name: " << tt->GetName()
	      << " pdf name: " << channelPdf->GetName()
	      << std::endl;
    // First, we check if this channel uses Stat Uncertainties:
    RooArgList* gammas = new RooArgList();
    ParamHistFunc* param_func=NULL;
    bool hasStatUncert = RooStats::getStatUncertaintyFromChannel( channelPdf, param_func, gammas );
    if( ! hasStatUncert ) {
      std::cout << "Channel: " << channel_name
		<< " doesn't have statistical uncertainties"
		<< std::endl;
      continue;
    }

    // Now, loop over the bins in this channel
    // To Do: Check that the index convention
    // still works for 2-d (ie matches the
    // convention in ParamHistFunc, etc)
    int num_bins = param_func->numBins();
  
    // Initialize the vector to the number of bins, allowing
    // us to skip gamma's if they are constant
    std::cout << "Creating cache for channel: " << channel_name 
	      << " if necessary" << std::endl;

    std::vector<BarlowCache> temp_cache( num_bins );
    bool channel_has_stat_uncertainty=false;

    for( Int_t bin_index = 0; bin_index < num_bins; ++bin_index ) {

      // Create a cache object
      BarlowCache cache;

      // Get the gamma for this bin, and skip if it's constant
      RooRealVar* gamma_stat = &(param_func->getParameter(bin_index));
      if( gamma_stat->isConstant() ) {
	std::cout << "Ignoring constant gamma: " << gamma_stat->GetName() << std::endl;
	continue;
      }
      else {
	cache.hasStatUncert=true;
	channel_has_stat_uncertainty=true;
	cache.gamma = gamma_stat;
	_statUncertParams.insert( gamma_stat->GetName() );
      }

      // Store a snapshot of the bin center
      RooArgSet* bin_center = (RooArgSet*) param_func->get( bin_index )->snapshot();
      cache.bin_center = bin_center;
      cache.observables = obsSet;

      cache.binVolume = param_func->binVolume();

      // Get the gamma's constraint term
      RooAbsReal* pois_mean = NULL;
      RooRealVar* tau = NULL;
      RooStats::getStatUncertaintyConstraintTerm( &constraints, gamma_stat, pois_mean, tau );
      cache.tau = tau;
      cache.nom_pois_mean = pois_mean;

      // Get the RooRealSumPdf
      RooAbsPdf* sum_pdf = RooStats::getSumPdfFromChannel( channelPdf );
      cache.sumPdf = sum_pdf;

      // And set the data value for this bin
      std::cout << "Getting the data value for this channel" << std::endl;
      if( ChannelBinDataMap.find(channel_name) == ChannelBinDataMap.end() ) {
	std::cout << "Error: channel with name: " << channel_name
		  << " not found in BinDataMap" << std::endl;
	throw runtime_error("BinDataMap");
      }
      double nData = ChannelBinDataMap[channel_name].at(bin_index);
      cache.nData = nData;
      
      temp_cache.at(bin_index) = cache;
      //_barlowCache[channel_name].at(bin_index) = cache;

      std::cout << "All done with this bin" << std::endl;

    } // End: Loop over bins

    if( channel_has_stat_uncertainty ) {
      std::cout << "Adding channel: " << channel_name
		<< " to the barlow cache" << std::endl;
      _barlowCache[channel_name] = temp_cache;
    }


  } // End: Loop over channels



  // Successfully initialized the cache
  // Printing some info

  std::map< std::string, std::vector< BarlowCache > >::iterator iter_cache;
  for( iter_cache = _barlowCache.begin(); iter_cache != _barlowCache.end(); ++iter_cache ) {
    
    std::string channel_name = (*iter_cache).first;
    std::vector< BarlowCache >& channel_cache = (*iter_cache).second;


    for( unsigned int i = 0; i < channel_cache.size(); ++i ) {
      BarlowCache& bin_cache = channel_cache.at(i);


      RooRealVar* gamma = bin_cache.gamma;
      RooRealVar* tau = bin_cache.tau;
      RooAbsReal* pois_mean = bin_cache.nom_pois_mean;
      RooAbsPdf* sum_pdf = (RooAbsPdf*) bin_cache.sumPdf;
      double binVolume = bin_cache.binVolume;


      if( !bin_cache.hasStatUncert ) {
	std::cout << "Barlow Cache for Channel: " << channel_name
		  << " Bin: " << i
		  << " NO STAT UNCERT" 
		  << std::endl;
      }
      else {
      std::cout << "Barlow Cache for Channel: " << channel_name
		<< " Bin: " << i
		<< " gamma: " << gamma->GetName()
		<< " tau: " << tau->GetName()
		<< " pois_mean: " << pois_mean->GetName()
		<< " sum_pdf: " << sum_pdf->GetName()
		<< " binVolume: " << binVolume
		<< std::endl;
      }
    }
  }
    

}


//_____________________________________________________________________________
RooArgSet* RooBarlowBeestonLL::getParameters(const RooArgSet* depList, Bool_t stripDisconnected) const {

  RooArgSet* allArgs = RooAbsArg::getParameters( depList, stripDisconnected );

  TIterator* iter_args = allArgs->createIterator();
  RooRealVar* arg;
  while((arg=(RooRealVar*)iter_args->Next())) {
    std::string arg_name = arg->GetName();

    // If there is a gamma in the name,
    // strip it from the list of dependencies

    if( _statUncertParams.find(arg_name.c_str()) != _statUncertParams.end() ) {
      allArgs->remove( *arg, kTRUE );
    }

  }

  return allArgs;

}


/*
//_____________________________________________________________________________
const RooArgSet& RooBarlowBeestonLL::bestFitParams() const 
{
  validateAbsMin() ;
  return _paramAbsMin ;
}


//_____________________________________________________________________________
const RooArgSet& RooBarlowBeestonLL::bestFitObs() const 
{
  validateAbsMin() ;
  return _obsAbsMin ;
}
*/



//_____________________________________________________________________________
RooAbsReal* RooBarlowBeestonLL::createProfile(const RooArgSet& paramsOfInterest) 
{
  // Optimized implementation of createProfile for profile likelihoods.
  // Return profile of original function in terms of stated parameters 
  // of interest rather than profiling recursively.

  return nll().createProfile(paramsOfInterest) ;
}



/*
void RooBarlowBeestonLL::FactorizePdf(const RooArgSet &observables, RooAbsPdf &pdf, RooArgList &obsTerms, RooArgList &constraints) const {
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
  } else if (id == typeid(RooSimultaneous) ) {    //|| id == typeid(RooSimultaneousOpt)) {
    RooSimultaneous *sim  = dynamic_cast<RooSimultaneous *>(&pdf);
    RooAbsCategoryLValue *cat = (RooAbsCategoryLValue *) sim->indexCat().Clone();
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
*/                                         



//_____________________________________________________________________________
Double_t RooBarlowBeestonLL::evaluate() const 
{ 

  //clock_t time_1, time_2, time_3, time_4, time_5;

  /*
  // Loop over the cached bins and channels
  RooArgSet* channels = new RooArgSet();
  RooArgSet* channelsWithConstraints = new RooArgSet();
  RooStats::getChannelsFromModel( _pdf, channels, channelsWithConstraints );

  // Loop over channels
  TIterator* iter_channels = channelsWithConstraints->createIterator();
  RooAbsPdf* channelPdf=NULL;
  while(( channelPdf=(RooAbsPdf*)iter_channels->Next()  )) {
    std::string channel_name = channelPdf->GetName(); //RooStats::channelNameFromPdf( channelPdf );
  */





  // Loop over the channels (keys to the map)
  std::map< std::string, std::vector< BarlowCache > >::iterator iter_cache;
  for( iter_cache = _barlowCache.begin(); iter_cache != _barlowCache.end(); ++iter_cache ) {
    
    std::string channel_name = (*iter_cache).first;

    // Get the vector of bin uncertainty caches for this channel
    if( _barlowCache.find( channel_name ) == _barlowCache.end() ) {
      std::cout << "Error: channel: " << channel_name 
		<< " not found in barlow Cache" << std::endl; 
      throw runtime_error("Channel not in barlow cache");
    }

    std::vector< BarlowCache >& channel_cache = _barlowCache[ channel_name ];


    // Loop over the bins in the cache
    // Set all gamma's to 0
    for( unsigned int i = 0; i < channel_cache.size(); ++i ) {
      BarlowCache& bin_cache = channel_cache.at(i);
      if( !bin_cache.hasStatUncert ) continue;
      RooRealVar* gamma = bin_cache.gamma;
      gamma->setVal(0.0);
    }
    std::vector< double > nu_b_vec( channel_cache.size() );
    for( unsigned int i = 0; i < channel_cache.size(); ++i ) {
      BarlowCache& bin_cache = channel_cache.at(i);
      if( !bin_cache.hasStatUncert ) continue;

      RooAbsPdf* sum_pdf = (RooAbsPdf*) bin_cache.sumPdf;
      RooArgSet* obsSet = bin_cache.observables;
      double binVolume = bin_cache.binVolume;

      bin_cache.SetBinCenter();
      double nu_b = sum_pdf->getVal(*obsSet)*sum_pdf->expectedEvents(*obsSet)*binVolume;
      nu_b_vec.at(i) = nu_b;
    }

    // Loop over the bins in the cache
    // Set all gamma's to 1
    for( unsigned int i = 0; i < channel_cache.size(); ++i ) {
      BarlowCache& bin_cache = channel_cache.at(i);
      if( !bin_cache.hasStatUncert ) continue;
      RooRealVar* gamma = bin_cache.gamma;
      gamma->setVal(1.0);
    }
    std::vector< double > nu_b_stat_vec( channel_cache.size() );
    for( unsigned int i = 0; i < channel_cache.size(); ++i ) {
      BarlowCache& bin_cache = channel_cache.at(i);
      if( !bin_cache.hasStatUncert ) continue;

      RooAbsPdf* sum_pdf = (RooAbsPdf*) bin_cache.sumPdf;
      RooArgSet* obsSet = bin_cache.observables;
      double binVolume = bin_cache.binVolume;

      bin_cache.SetBinCenter();
      double nu_b_stat = sum_pdf->getVal(*obsSet)*sum_pdf->expectedEvents(*obsSet)*binVolume - nu_b_vec.at(i);
      nu_b_stat_vec.at(i) = nu_b_stat;
    }

    
    // Loop over the bins in the cache
    for( unsigned int i = 0; i < channel_cache.size(); ++i ) {
      
      BarlowCache& bin_cache = channel_cache.at(i);

      if( !bin_cache.hasStatUncert ) {
	//std::cout << "Bin: " << i << " of " << channel_cache.size() 
	//	  << " in channel: " << channel_name
	//	  << " doesn't have stat uncertainties" << std::endl;
	continue;
      }

      // Set the observable to the bin center
      bin_cache.SetBinCenter();

      RooRealVar* gamma = bin_cache.gamma;
      RooRealVar* tau = bin_cache.tau;
      RooAbsReal* pois_mean = bin_cache.nom_pois_mean;
      RooAbsPdf* sum_pdf = (RooAbsPdf*) bin_cache.sumPdf;
      RooArgSet* obsSet = bin_cache.observables;
      double binVolume = bin_cache.binVolume;

      //sum_pdf->getParameters(*obsSet)->Print("V");

      //std::cout << "Observables: " << std::endl;
      //obsSet->Print("V");

      //gamma->setVal(0.0);
      //double nu_b = sum_pdf->getVal(*obsSet)*sum_pdf->expectedEvents(*obsSet)*binVolume;
      double nu_b = nu_b_vec.at(i);
      /*
      sum_pdf->getParameters(*obsSet)->Print("V");
      std::cout << "Expected Events: " << sum_pdf->expectedEvents(*obsSet) 
		<< " Pdf Val: " << sum_pdf->getVal(*obsSet) << std::endl;
      */

      //gamma->setVal(1.0);
      //double nu_b_stat = sum_pdf->getVal(*obsSet)*sum_pdf->expectedEvents(*obsSet)*binVolume - nu_b;
      double nu_b_stat = nu_b_stat_vec.at(i);
      /*
      sum_pdf->getParameters(*obsSet)->Print("V");
      std::cout << "Expected Events: " << sum_pdf->expectedEvents(*obsSet) 
		<< " Pdf Val: " << sum_pdf->getVal(*obsSet) << std::endl;
      */

      double tau_val = tau->getVal();
      double nData = bin_cache.nData;
      double m_val = pois_mean->getVal();

      double A = nu_b_stat*nu_b_stat + tau_val*nu_b_stat;
      double B = nu_b*tau_val + nu_b*nu_b_stat - nData*nu_b_stat - m_val*nu_b_stat;
      double C = -1*m_val*nu_b;

      double discrim = B*B-4*A*C;

      if( discrim < 0 ) {
	std::cout << "Warning: Discriminant (B*B - 4AC) < 0" << std::endl;
	std::cout << "Warning: Taking B*B - 4*A*C == 0Discriminant (B*B - 4AC) < 0" << std::endl;
	discrim=0;
	//throw runtime_error("BarlowBeestonLL::evaluate() : B*B - 4AC < 0");
      }
      if( A <= 0 ) {
	std::cout << "Warning: A <= 0" << std::endl;
	throw runtime_error("BarlowBeestonLL::evaluate() : A < 0");
      }

      double gamma_hat_hat = ( -1*B + TMath::Sqrt(discrim) ) / (2*A);

      // Check for NAN
      if( isnan(gamma_hat_hat) ) { // != gamma_hat_hat ) {
	std::cout << "ERROR: gamma hat hat is NAN" << std::endl;
	throw runtime_error("BarlowBeestonLL::evaluate() : gamma hat hat is NAN");
      }

      if( gamma_hat_hat <= 0 ) {
	std::cout << "WARNING: gamma hat hat <= 0.  Setting to 0" << std::endl;
	gamma_hat_hat = 0;
      }

      /*
      std::cout << "n: " << bin_cache.nData << " "
		<< "nu_stat: " << nu_b_stat << " "
		<< "nu: " << nu_b << " "
		<< "tau: " << tau->getVal() << " "
		<< "m: " << pois_mean->getVal() << " "
		<< "A: " << A << " "
		<< "B: " << B << " "
		<< "C: " << C << " "
		<< "gamma hat hat: " << gamma_hat_hat 
		<< std::endl;
      */
      gamma->setVal( gamma_hat_hat );

    }
  } 

  return _nll;  

}



/*
//_____________________________________________________________________________
void RooBarlowBeestonLL::validateAbsMin() const 
{
  // Check that parameters and likelihood value for 'best fit' are still valid. If not,
  // because the best fit has never been calculated, or because constant parameters have
  // changed value or parameters have changed const/float status, the minimum is recalculated

  // Check if constant status of any of the parameters have changed
  if (_absMinValid) {
    _piter->Reset() ;
    RooAbsArg* par ;
    while((par=(RooAbsArg*)_piter->Next())) {
      if (_paramFixed[par->GetName()] != par->isConstant()) {
	cxcoutI(Minimization) << "RooBarlowBeestonLL::evaluate(" << GetName() << ") constant status of parameter " << par->GetName() << " has changed from " 
				<< (_paramFixed[par->GetName()]?"fixed":"floating") << " to " << (par->isConstant()?"fixed":"floating") 
				<< ", recalculating absolute minimum" << endl ;
	_absMinValid = kFALSE ;
	break ;
      }
    }
  }


  // If we don't have the absolute minimum w.r.t all observables, calculate that first
  if (!_absMinValid) {

    cxcoutI(Minimization) << "RooBarlowBeestonLL::evaluate(" << GetName() << ") determining minimum likelihood for current configurations w.r.t all observable" << endl ;


    // Save current values of non-marginalized parameters
    RooArgSet* obsStart = (RooArgSet*) _obs.snapshot(kFALSE) ;

    // Start from previous global minimum 
    if (_paramAbsMin.getSize()>0) {
      const_cast<RooSetProxy&>(_par).assignValueOnly(_paramAbsMin) ;
    }
    if (_obsAbsMin.getSize()>0) {
      const_cast<RooSetProxy&>(_obs).assignValueOnly(_obsAbsMin) ;
    }

    // Find minimum with all observables floating
    const_cast<RooSetProxy&>(_obs).setAttribAll("Constant",kFALSE) ;  
    _minuit->migrad() ;

    // Save value and remember
    _absMin = _nll ;
    _absMinValid = kTRUE ;

    // Save parameter values at abs minimum as well
    _paramAbsMin.removeAll() ;

    // Only store non-constant parameters here!
    RooArgSet* tmp = (RooArgSet*) _par.selectByAttrib("Constant",kFALSE) ;
    _paramAbsMin.addClone(*tmp) ;
    delete tmp ;

    _obsAbsMin.addClone(_obs) ;

    // Save constant status of all parameters
    _piter->Reset() ;
    RooAbsArg* par ;
    while((par=(RooAbsArg*)_piter->Next())) {
      _paramFixed[par->GetName()] = par->isConstant() ;
    }
    
    if (dologI(Minimization)) {
      cxcoutI(Minimization) << "RooBarlowBeestonLL::evaluate(" << GetName() << ") minimum found at (" ;

      RooAbsReal* arg ;
      Bool_t first=kTRUE ;
      _oiter->Reset() ;
      while ((arg=(RooAbsReal*)_oiter->Next())) {
	ccxcoutI(Minimization) << (first?"":", ") << arg->GetName() << "=" << arg->getVal() ;	
	first=kFALSE ;
      }      
      ccxcoutI(Minimization) << ")" << endl ;            
    }

    // Restore original parameter values
    const_cast<RooSetProxy&>(_obs) = *obsStart ;
    delete obsStart ;

  }
}
*/


//_____________________________________________________________________________
Bool_t RooBarlowBeestonLL::redirectServersHook(const RooAbsCollection& /*newServerList*/, Bool_t /*mustReplaceAll*/, 
					 Bool_t /*nameChange*/, Bool_t /*isRecursive*/) 
{ 
  /*
  if (_minuit) {
    delete _minuit ;
    _minuit = 0 ;
  }
  */
  return kFALSE ;
} 


