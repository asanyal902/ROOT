/*****************************************************************************
 * Project: RooFit                                                           *
 * Package: RooFitCore                                                       *
 *    File: $Id: RooProdGenContext.rdl,v 1.6 2002/09/05 04:33:49 verkerke Exp $
 * Authors:                                                                  *
 *   WV, Wouter Verkerke, UC Santa Barbara, verkerke@slac.stanford.edu       *
 *   DK, David Kirkby,    UC Irvine,         dkirkby@uci.edu                 *
 *                                                                           *
 * Copyright (c) 2000-2002, Regents of the University of California          *
 *                          and Stanford University. All rights reserved.    *
 *                                                                           *
 * Redistribution and use in source and binary forms,                        *
 * with or without modification, are permitted according to the terms        *
 * listed in LICENSE (http://roofit.sourceforge.net/license.txt)             *
 *****************************************************************************/
#ifndef ROO_PROD_GEN_CONTEXT
#define ROO_PROD_GEN_CONTEXT

#include "TList.h"
#include "RooFitCore/RooAbsGenContext.hh"
#include "RooFitCore/RooArgSet.hh"

class RooProdPdf;
class RooDataSet;
class RooRealIntegral;
class RooAcceptReject;
class TRandom;
class TIterator;
class RooSuperCategory ;

class RooProdGenContext : public RooAbsGenContext {
public:
  RooProdGenContext(const RooProdPdf &model, const RooArgSet &vars, const RooDataSet *prototype= 0,
		Bool_t _verbose= kFALSE);
  virtual ~RooProdGenContext();

protected:

  virtual void initGenerator(const RooArgSet &theEvent);
  virtual void generateEvent(RooArgSet &theEvent, Int_t remaining);

  void updateCCDTable() ;


  RooProdGenContext(const RooProdGenContext& other) ;
  
  RooArgSet _commonCats ;        // Common category dependents
  RooArgSet* _ccdCloneSet ;
  RooSuperCategory* _ccdSuper ;  // SuperCategory of Common category dependents
  RooArgSet* _pdfCloneSet ;
  RooAbsPdf* _pdfClone ;
  RooRealIntegral* _pdfCcdInt ;
  Bool_t _ccdRefresh ;
  Double_t * _ccdTable ;
  const RooProdPdf *_pdf ;       //  Original PDF
  TList _gcList ;                //  List of component generator contexts
  TIterator* _gcIter ;           //! Iterator over gcList

  ClassDef(RooProdGenContext,0) // Context for generating a dataset from a PDF
};

#endif
