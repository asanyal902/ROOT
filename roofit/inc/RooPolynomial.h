/*****************************************************************************
 * Project: BaBar detector at the SLAC PEP-II B-factory
 * Package: RooFitTools
 *    File: $Id: RooPolynomial.rdl,v 1.2 2001/09/26 21:44:06 verkerke Exp $
 * Authors:
 *   WV, Wouter Verkerke, UC Santa Barbara, verkerke@slac.stanford.edu
 * History:
 *   07-Feb-2000 WV Initial RFC version
 *
 * Copyright (C) 2001 University of California
 *****************************************************************************/
#ifndef ROO_POLYNOMIAL
#define ROO_POLYNOMIAL

#include "RooFitCore/RooAbsPdf.hh"
#include "RooFitCore/RooRealProxy.hh"
#include "RooFitCore/RooListProxy.hh"

class RooRealVar;
class RooArgList ;

class RooPolynomial : public RooAbsPdf {
public:

  RooPolynomial() ;
  RooPolynomial(const char* name, const char* title, RooAbsReal& x) ;
  RooPolynomial(const char *name, const char *title,
		RooAbsReal& _x, const RooArgList& _coefList, Int_t lowestOrder=1) ;

  RooPolynomial(const RooPolynomial& other, const char* name = 0);
  virtual TObject* clone(const char* newname) const { return new RooPolynomial(*this, newname); }
  virtual ~RooPolynomial() ;

  Int_t getAnalyticalIntegral(RooArgSet& allVars, RooArgSet& analVars) const ;
  Double_t analyticalIntegral(Int_t code) const ;

protected:

  RooRealProxy _x;
  RooListProxy _coefList ;
  Int_t _lowestOrder ;
  TIterator* _coefIter ;  //! do not persist

  Double_t evaluate() const;

  ClassDef(RooPolynomial,1) // Polynomial PDF
};

#endif
