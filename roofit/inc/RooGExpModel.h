/*****************************************************************************
 * Project: BaBar detector at the SLAC PEP-II B-factory
 * Package: RooFitCore
 *    File: $Id: RooGExpModel.rdl,v 1.6 2002/06/04 23:24:01 verkerke Exp $
 * Authors:
 *   WV, Wouter Verkerke, UC Santa Barbara, verkerke@slac.stanford.edu
 * History:
 *   05-Jun-2001 WV Created initial version
 *
 * Copyright (C) 2001 University of California
 *****************************************************************************/
#ifndef ROO_GEXP_MODEL
#define ROO_GEXP_MODEL

#include "RooFitCore/RooResolutionModel.hh"
#include "RooFitCore/RooRealProxy.hh"
#include "RooFitCore/RooComplex.hh"
#include "RooFitCore/RooMath.hh"

class RooGExpModel : public RooResolutionModel {
public:

  enum RooGExpBasis { noBasis=0, expBasisMinus= 1, expBasisSum= 2, expBasisPlus= 3,
		                 sinBasisMinus=11, sinBasisSum=12, sinBasisPlus=13,
                                 cosBasisMinus=21, cosBasisSum=22, cosBasisPlus=23 } ;
  enum BasisType { none=0, expBasis=1, sinBasis=2, cosBasis=3 } ;
  enum BasisSign { Both=0, Plus=+1, Minus=-1 } ;
  enum Type { Normal, Flipped };

  // Constructors, assignment etc
  inline RooGExpModel() { }
  RooGExpModel(const char *name, const char *title, RooRealVar& x, 
	       RooAbsReal& sigma, RooAbsReal& rlife, 
	       Bool_t nlo=kFALSE, Type type=Normal) ; 

  RooGExpModel(const char *name, const char *title, RooRealVar& x, 
	       RooAbsReal& sigma, RooAbsReal& rlife, 
	       RooAbsReal& srSF, 
	       Bool_t nlo=kFALSE, Type type=Normal) ; 

  RooGExpModel(const char *name, const char *title, RooRealVar& x, 
	       RooAbsReal& sigma, RooAbsReal& rlife, 
	       RooAbsReal& sigmaSF, RooAbsReal& rlifeSF,
	       Bool_t nlo=kFALSE, Type type=Normal) ; 

  RooGExpModel(const RooGExpModel& other, const char* name=0);
  virtual TObject* clone(const char* newname) const { return new RooGExpModel(*this,newname) ; }
  virtual ~RooGExpModel();
  
  virtual Int_t basisCode(const char* name) const ;
  virtual Int_t getAnalyticalIntegral(RooArgSet& allVars, RooArgSet& analVars) const ;
  virtual Double_t analyticalIntegral(Int_t code) const ;

  Int_t getGenerator(const RooArgSet& directVars, RooArgSet &generateVars, Bool_t staticInitOK=kTRUE) const;
  void generateEvent(Int_t code);

  void advertiseFlatScaleFactorIntegral(Bool_t flag) { _flatSFInt = flag ; }

protected:

  Double_t calcDecayConv(Double_t sign, Double_t tau, Double_t sig, Double_t rtau) const ;
  RooComplex calcSinConv(Double_t sign, Double_t sig, Double_t tau, Double_t omega, Double_t rtau, Double_t fsign) const ;
  RooComplex calcSinConvNorm(Double_t sign, Double_t tau, Double_t omega) const ;

  virtual Double_t evaluate() const ;
  RooComplex evalCerfApprox(Double_t swt, Double_t u, Double_t c) const ;

  // Calculate exp(-u^2) cwerf(swt*c + i(u+c)), taking care of numerical instabilities
  inline RooComplex evalCerf(Double_t swt, Double_t u, Double_t c) const {
    RooComplex z(swt*c,u+c);
    return (z.im()>-4.0) ? RooMath::FastComplexErrFunc(z)*exp(-u*u) : evalCerfApprox(swt,u,c) ;
  }
    
  // Calculate Re(exp(-u^2) cwerf(swt*c + i(u+c))), taking care of numerical instabilities
  inline Double_t evalCerfRe(Double_t swt, Double_t u, Double_t c) const {
    RooComplex z(swt*c,u+c);
    return (z.im()>-4.0) ? RooMath::FastComplexErrFuncRe(z)*exp(-u*u) : evalCerfApprox(swt,u,c).re() ;
  }
  
  // Calculate Im(exp(-u^2) cwerf(swt*c + i(u+c))), taking care of numerical instabilities
  inline Double_t evalCerfIm(Double_t swt, Double_t u, Double_t c) const {
    RooComplex z(swt*c,u+c);
    return (z.im()>-4.0) ? RooMath::FastComplexErrFuncIm(z)*exp(-u*u) : evalCerfApprox(swt,u,c).im() ;
  }
  

  RooRealProxy sigma ;
  RooRealProxy rlife ;
  RooRealProxy ssf ;
  RooRealProxy rsf ;
  Bool_t _flip ;
  Bool_t _nlo ;
  Bool_t _flatSFInt ;

  ClassDef(RooGExpModel,1) // GExp Resolution Model
};

#endif
