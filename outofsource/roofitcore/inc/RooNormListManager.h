/*****************************************************************************
 * Project: RooFit                                                           *
 * Package: RooFitCore                                                       *
 *    File: $Id: RooNormListManager.h,v 1.8 2007/05/11 09:11:30 verkerke Exp $
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
#ifndef ROO_NORM_LIST_MANAGER
#define ROO_NORM_LIST_MANAGER

#include "Rtypes.h"

#include "RooNormSetCache.h"
#include "RooAbsReal.h"
#include "RooArgSet.h"
#include "RooArgList.h"

class RooNameSet ;
typedef RooArgList* pRooArgList ;

class RooNormListManager {

public:
  RooNormListManager(Int_t maxSize=10) ;
  RooNormListManager(const RooNormListManager& other, Bool_t sterileCopy=kFALSE) ;
  virtual ~RooNormListManager() ;
  
  RooArgList* getNormList(const RooAbsArg* self, const RooArgSet* nset, const RooArgSet* iset=0, 
                          Int_t* sterileIndex=0, const TNamed* isetRangeName=0) ;
  Int_t setNormList(const RooAbsArg* self, const RooArgSet* nset, const RooArgSet* iset, RooArgList* normColl, const TNamed* isetRangeName) ;  
  void reset() ;
  void sterilize() ;

  Int_t lastIndex() const { return _lastIndex ; }
  Int_t cacheSize() const { return _size ; }

  RooArgList* getNormListByIndex(Int_t index) const ;
  const RooNameSet* nameSet1ByIndex(Int_t index) const ;
  const RooNameSet* nameSet2ByIndex(Int_t index) const ;

  void setVerbose(Bool_t flag=kTRUE) const { _verbose = flag ; }
 
protected:

  Int_t _maxSize ;
  Int_t _size ;
  Int_t _lastIndex ;
  static Bool_t _verbose ;

  RooNormSetCache* _nsetCache ; //!
  pRooArgList* _normList ; //!

  ClassDef(RooNormListManager,1) // Manager class for PDF normalization integral lists
} ;

#endif 
