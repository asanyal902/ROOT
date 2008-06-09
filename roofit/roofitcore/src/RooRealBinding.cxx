/*****************************************************************************
 * Project: RooFit                                                           *
 * Package: RooFitCore                                                       *
 * @(#)root/roofitcore:$Id$
 * Authors:                                                                  *
 *   WV, Wouter Verkerke, UC Santa Barbara, verkerke@slac.stanford.edu       *
 *   DK, David Kirkby,    UC Irvine,         dkirkby@uci.edu                 *
 *                                                                           *
 * Copyright (c) 2000-2005, Regents of the University of California          *
 *                          and Stanford University. All rights reserved.    *
 *                                                                           *
 * Redistribution and use in source and binary forms,                        *
 * with or without modification, are permitted according to the terms        *
 * listed in LICENSE (http://roofit.sourceforge.net/license.txt)             *
 *****************************************************************************/

// -- CLASS DESCRIPTION [AUX] --
// Lightweight interface adaptor that binds a RooAbsReal object to a subset
// of its servers.


#include "RooFit.h"
#include "Riostream.h"

#include "RooRealBinding.h"
#include "RooAbsReal.h"
#include "RooArgSet.h"
#include "RooAbsRealLValue.h"
#include "RooNameReg.h"
#include "RooMsgService.h"

#include <assert.h>



ClassImp(RooRealBinding)
;

RooRealBinding::RooRealBinding(const RooAbsReal& func, const RooArgSet &vars, const RooArgSet* nset, Bool_t clipInvalid, const TNamed* rangeName) :
  RooAbsFunc(vars.getSize()), _func(&func), _vars(0), _nset(nset), _clipInvalid(clipInvalid), _xsave(0), _rangeName(rangeName)
{
  // allocate memory
  _vars= new RooAbsRealLValue*[getDimension()];
  if(0 == _vars) {
    _valid= kFALSE;
    return;
  }
  // check that all of the arguments are real valued and store them
  RooAbsArg *var = 0;
  TIterator* iter = vars.createIterator() ;
  Int_t index(0) ;
  while((var=(RooAbsArg*)iter->Next())) {
    _vars[index]= dynamic_cast<RooAbsRealLValue*>(var);
    if(0 == _vars[index]) {
      oocoutE((TObject*)0,InputArguments) << "RooRealBinding: cannot bind to " << var->GetName() << endl ;
      _valid= kFALSE;
    }
    index++ ;
  }
  delete iter ;
}

RooRealBinding::~RooRealBinding() {
  if(0 != _vars) delete[] _vars;
  if (_xsave) delete[] _xsave ;
}


void RooRealBinding::saveXVec() const
{
  if (!_xsave) {
    _xsave = new Double_t[getDimension()] ;    
    for (UInt_t i=0 ; i<getDimension() ; i++) {
      _xsave[i] = _vars[i]->getVal() ;
    }
  }
}

void RooRealBinding::restoreXVec() const
{
  if (!_xsave) {
    return ;
  }
  for (UInt_t i=0 ; i<getDimension() ; i++) {
   _vars[i]->setVal(_xsave[i]) ;
  }
}


void RooRealBinding::loadValues(const Double_t xvector[]) const {
  _xvecValid = kTRUE ;
  for(UInt_t index= 0; index < _dimension; index++) {
    if (_clipInvalid && !_vars[index]->isValidReal(xvector[index])) {
      _xvecValid = kFALSE ;
    } else {
      _vars[index]->setVal(xvector[index],RooNameReg::instance().constStr(_rangeName));
    }
  }
}  

Double_t RooRealBinding::operator()(const Double_t xvector[]) const {
  assert(isValid());
  _ncall++ ;
  loadValues(xvector);
  //cout << getName() << "(x=" << xvector[0] << ")=" << _func->getVal(_nset) << endl ;
  return _xvecValid ? _func->getVal(_nset) : 0. ;
}

Double_t RooRealBinding::getMinLimit(UInt_t index) const {
  assert(isValid());
  return _vars[index]->getMin(RooNameReg::str(_rangeName));
}

Double_t RooRealBinding::getMaxLimit(UInt_t index) const {
  assert(isValid());
  return _vars[index]->getMax(RooNameReg::str(_rangeName));
}

const char* RooRealBinding::getName() const 
{ 
  return _func->GetName() ; 
} 
