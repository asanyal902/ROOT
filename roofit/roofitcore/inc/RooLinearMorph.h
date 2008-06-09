/*****************************************************************************
 * Project: RooFit                                                           *
 *                                                                           *
 * Copyright (c) 2000-2007, Regents of the University of California          *
 *                          and Stanford University. All rights reserved.    *
 *                                                                           *
 * Redistribution and use in source and binary forms,                        *
 * with or without modification, are permitted according to the terms        *
 * listed in LICENSE (http://roofit.sourceforge.net/license.txt)             *
 *****************************************************************************/

#ifndef ROOLINEARMORPH
#define ROOLINEARMORPH

#include "RooAbsCachedPdf.h"
#include "RooRealProxy.h"
#include "RooCategoryProxy.h"
#include "RooAbsReal.h"
#include "RooAbsCategory.h"
class RooBrentRootFinder ;

class TH1D ;
 
class RooLinearMorph : public RooAbsCachedPdf {
public:
  RooLinearMorph() {} ; 
  RooLinearMorph(const char *name, const char *title,
	      RooAbsReal& _pdf1,
	      RooAbsReal& _pdf2,
  	      RooAbsReal& _x,
	      RooAbsReal& _alpha, Bool_t cacheAlpha=kFALSE);
  RooLinearMorph(const RooLinearMorph& other, const char* name=0) ;
  virtual TObject* clone(const char* newname) const { return new RooLinearMorph(*this,newname); }
  inline virtual ~RooLinearMorph() { }

  Bool_t selfNormalized() const { return kTRUE ; }
  void setCacheAlpha(Bool_t flag) { _cacheMgr.sterilize() ; _cacheAlpha = flag ; }
  Bool_t cacheAlpha() const { return _cacheAlpha ; }

  virtual void preferredObservableScanOrder(const RooArgSet& obs, RooArgSet& orderedObs) const ;

  class MorphCacheElem : public PdfCacheElem {
  public:
    MorphCacheElem(RooLinearMorph& self, const RooArgSet* nset) ;
    ~MorphCacheElem() ;
    void calculate(TIterator* iter) ;
    virtual RooArgList containedArgs(Action) ;

  protected:
    
    void findRange() ;
    Double_t calcX(Double_t y, Bool_t& ok) ;
    Int_t binX(Double_t x) ;
    void fillGap(Int_t ixlo, Int_t ixhi,Double_t splitPoint=0.5) ;
    void interpolateGap(Int_t ixlo, Int_t ixhi) ;

    RooLinearMorph* _self ; //
    RooArgSet* _nset ; 
    RooAbsPdf* _pdf1 ; // PDF1
    RooAbsPdf* _pdf2 ; // PDF2
    RooRealVar* _x   ; // X
    RooAbsReal* _alpha ; // ALPHA 
    RooAbsReal* _c1 ; // CDF of PDF 1
    RooAbsReal* _c2 ; // CDF of PDF 2
    RooAbsFunc* _cb1 ; // Binding of CDF1
    RooAbsFunc* _cb2 ; // Binding of CDF2
    RooBrentRootFinder* _rf1 ; // ROOT finder on CDF1
    RooBrentRootFinder* _rf2 ; // ROOT finder of CDF2 ;

    Double_t* _yatX ; //
    Double_t* _calcX; //
    Int_t _yatXmin, _yatXmax ;
    Int_t _ccounter ;

    Double_t _ycutoff ;

  } ;

protected:

  friend class MorphCacheElem ;  
  virtual PdfCacheElem* createCache(const RooArgSet* nset) const ;
  virtual const char* inputBaseName() const ;
  virtual RooArgSet* actualObservables(const RooArgSet& nset) const ;
  virtual RooArgSet* actualParameters(const RooArgSet& nset) const ;
  virtual void fillCacheObject(PdfCacheElem& cache) const ;
  
  RooRealProxy pdf1 ;
  RooRealProxy pdf2 ;
  RooRealProxy x ;
  RooRealProxy alpha ;
  Bool_t _cacheAlpha ;
  mutable MorphCacheElem* _cache ;

 
  Double_t evaluate() const ;

private:

  ClassDef(RooLinearMorph,1) // Your description goes here...
};
 
#endif
