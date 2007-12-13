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

#ifndef ROOFFTCONVPDF
#define ROOFFTCONVPDF

#include "RooAbsCachedPdf.h"
#include "RooRealProxy.h"
#include "RooAbsReal.h"
#include "RooHistPdf.h"
#include "TVirtualFFT.h"
class RooRealVar ;

#include <list>
 
class RooFFTConvPdf : public RooAbsCachedPdf {
public:

  RooFFTConvPdf() {} ;
  RooFFTConvPdf(const char *name, const char *title, RooRealVar& convVar, RooAbsPdf& pdf1, RooAbsPdf& pdf2, Int_t ipOrder=0);
  RooFFTConvPdf(const RooFFTConvPdf& other, const char* name=0) ;
  virtual TObject* clone(const char* newname) const { return new RooFFTConvPdf(*this,newname); }
  virtual ~RooFFTConvPdf() ;

  Double_t bufferFraction() const { return _bufFrac ; }
  void setBufferFraction(Double_t frac) ;

protected:

  RooRealProxy _x ;
  RooRealProxy _pdf1 ;
  RooRealProxy _pdf2 ;

  Double_t*  scanPdf(RooRealVar& obs, RooAbsPdf& pdf, const RooDataHist& hist, const RooArgSet& slicePos, Int_t& N, Int_t& N2) const ;

  virtual Double_t evaluate() const { RooArgSet dummy(_x.arg()) ; return getVal(&dummy) ; } ; // dummy
  virtual const char* inputBaseName() const ;
  virtual RooArgSet* actualObservables(const RooArgSet& nset) const ;
  virtual RooArgSet* actualParameters(const RooArgSet& nset) const ;
  virtual void fillCacheObject(CacheElem& cache) const ;
  void fillCacheSlice(RooHistPdf& cachePdf, const RooArgSet& slicePosition) const ;

  class CacheAuxInfo {
  public:
    CacheAuxInfo() : fftr2c1(0),fftr2c2(0),fftc2r(0) {} 
    ~CacheAuxInfo() { delete fftr2c1 ; delete fftr2c2 ; delete fftc2r ; }
    TVirtualFFT* fftr2c1 ;
    TVirtualFFT* fftr2c2 ;
    TVirtualFFT* fftc2r ;
  };

  mutable map<const RooHistPdf*,CacheAuxInfo*> _cacheAuxInfo ; //! Auxilary Cache information (do not persist)
  Double_t _bufFrac ; // Sampling buffer size as fraction of domain size 

  virtual RooAbsGenContext* genContext(const RooArgSet &vars, const RooDataSet *prototype=0, 
                                       const RooArgSet* auxProto=0, Bool_t verbose= kFALSE) const ;

  friend class RooConvGenContext ;

private:

  ClassDef(RooFFTConvPdf,1) // Abstract base class for cached p.d.f.s
};
 
#endif
