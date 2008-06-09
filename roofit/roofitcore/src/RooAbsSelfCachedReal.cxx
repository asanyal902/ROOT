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

 // -- CLASS DESCRIPTION [PDF] -- 
 // Your description goes here... 

#include "Riostream.h" 

#include "RooFit.h"
#include "RooAbsSelfCachedReal.h" 
#include "RooAbsReal.h" 
#include "RooMsgService.h"
#include "RooDataHist.h"
#include "RooHistPdf.h"

using namespace std ;

ClassImp(RooAbsSelfCachedReal) 



RooAbsSelfCachedReal::RooAbsSelfCachedReal(const char *name, const char *title, Int_t ipOrder) :
  RooAbsCachedReal(name,title,ipOrder)
 { 
 } 


RooAbsSelfCachedReal::RooAbsSelfCachedReal(const RooAbsSelfCachedReal& other, const char* name) :  
   RooAbsCachedReal(other,name)
 { 
 } 


RooAbsSelfCachedReal::~RooAbsSelfCachedReal() 
{
}


void RooAbsSelfCachedReal::fillCacheObject(RooAbsCachedReal::FuncCacheElem& cache) const 
{
  RooDataHist& cacheHist = *cache.hist() ;

  // Make deep clone of self in non-caching mde and attach to dataset observables
  RooArgSet* cloneSet = (RooArgSet*) RooArgSet(*this).snapshot(kTRUE) ;
  RooAbsSelfCachedReal* clone2 = (RooAbsSelfCachedReal*) cloneSet->find(GetName()) ;
  clone2->disableCache(kTRUE) ;
  clone2->attachDataSet(cacheHist) ;

  // Iterator over all bins of RooDataHist and fill weights
  for (Int_t i=0 ; i<cacheHist.numEntries() ; i++) {
    const RooArgSet* obs = cacheHist.get(i) ;
    Double_t wgt = clone2->getVal(obs) ;
    cacheHist.set(wgt) ;
  }

  delete cloneSet ;
}


RooArgSet* RooAbsSelfCachedReal::actualObservables(const RooArgSet& nset) const 
{
  // Make list of servers
  RooArgSet servers ;

  TIterator* siter = serverIterator() ;
  siter->Reset() ;
  RooAbsArg* server ;
  while((server=(RooAbsArg*)siter->Next())) {
    servers.add(*server) ;
  }
  
  // Return servers that are in common with given normalization set
  return (RooArgSet*) servers.selectCommon(nset) ;
  
}


RooArgSet* RooAbsSelfCachedReal::actualParameters(const RooArgSet& nset) const 
{  
  // Make list of servers
  RooArgSet *servers = new RooArgSet ;

  TIterator* siter = serverIterator() ;
  siter->Reset() ;
  RooAbsArg* server ;
  while((server=(RooAbsArg*)siter->Next())) {
    servers->add(*server) ;
  }
  
  // Remove all given observables from server list
  servers->remove(nset,kTRUE,kTRUE) ;

  return servers ;
}







