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

// -- CLASS DESCRIPTION [CONT] --
// RooSetProxy is the concrete proxy for RooArgSet objects.
// A RooSetProxy is the general mechanism to store a RooArgSet
// with RooAbsArgs in a RooAbsArg.
//
// Creating a RooSetProxy adds all members of the proxied RooArgSet to the proxy owners
// server list (thus receiving value/shape dirty flags from it) and
// registers itself with the owning class. The latter allows the
// owning class to update the pointers of RooArgSet contents to reflect
// the serverRedirect changes.


#include "RooFit.h"

#include "RooSetProxy.h"
#include "RooSetProxy.h"
#include "RooArgSet.h"
#include "RooAbsArg.h"

ClassImp(RooSetProxy)
;


void* RooSetProxy::operator new (size_t bytes)
{
  return malloc(bytes) ;
}

void RooSetProxy::operator delete (void * /*ptr*/)
{
}

RooSetProxy::RooSetProxy(const char* name, const char* /*desc*/, RooAbsArg* owner, 
			 Bool_t defValueServer, Bool_t defShapeServer) :
  RooArgSet(name), _owner(owner), 
  _defValueServer(defValueServer), 
  _defShapeServer(defShapeServer)
{
  //SetTitle(desc) ;
  _owner->registerProxy(*this) ;
  _iter = createIterator() ;
}


RooSetProxy::RooSetProxy(const char* name, RooAbsArg* owner, const RooSetProxy& other) : 
  RooArgSet(other,name), _owner(owner),  
  _defValueServer(other._defValueServer), 
  _defShapeServer(other._defShapeServer)
{
  _owner->registerProxy(*this) ;
  _iter = createIterator() ;
}


RooSetProxy::~RooSetProxy()
{
  if (_owner) _owner->unRegisterProxy(*this) ;
  delete _iter ;
}


Bool_t RooSetProxy::add(const RooAbsArg& var, Bool_t valueServer, Bool_t shapeServer, Bool_t silent)
{
  Bool_t ret=RooArgSet::add(var,silent) ;
  if (ret) {
    _owner->addServer((RooAbsArg&)var,valueServer,shapeServer) ;
  }
  return ret ;  
}



Bool_t RooSetProxy::addOwned(RooAbsArg& var, Bool_t silent)
{
  Bool_t ret=RooArgSet::addOwned(var,silent) ;
  if (ret) {
    _owner->addServer((RooAbsArg&)var,_defValueServer,_defShapeServer) ;
  }
  return ret ;  
}
				 

RooAbsArg* RooSetProxy::addClone(const RooAbsArg& var, Bool_t silent) 
{
  RooAbsArg* ret=RooArgSet::addClone(var,silent) ;
  if (ret) {
    _owner->addServer((RooAbsArg&)var,_defValueServer,_defShapeServer) ;
  }
  return ret ;
}



Bool_t RooSetProxy::add(const RooAbsArg& var, Bool_t silent) 
{
  return add(var,_defValueServer,_defShapeServer,silent) ;
}


Bool_t RooSetProxy::replace(const RooAbsArg& var1, const RooAbsArg& var2) 
{
  Bool_t ret=RooArgSet::replace(var1,var2) ;
  if (ret) {
    if (!isOwning()) _owner->removeServer((RooAbsArg&)var1) ;
    _owner->addServer((RooAbsArg&)var2,_owner->isValueServer(var1),
		                       _owner->isShapeServer(var2)) ;
  }
  return ret ;
}


Bool_t RooSetProxy::remove(const RooAbsArg& var, Bool_t silent, Bool_t matchByNameOnly) 
{
  Bool_t ret=RooArgSet::remove(var,silent,matchByNameOnly) ;
  if (ret && !isOwning()) {
    _owner->removeServer((RooAbsArg&)var) ;
  }
  return ret ;
}


Bool_t RooSetProxy::remove(const RooAbsCollection& list, Bool_t silent, Bool_t matchByNameOnly) 
{
  // Remove each argument in the input list from our list using remove(const RooAbsArg&).
  // Return kFALSE in case of problems.

  Bool_t result(false) ;

  TIterator* iter = list.createIterator() ;
  RooAbsArg* arg ;
  while((arg=(RooAbsArg*)iter->Next())) {
    result |= remove(*arg,silent,matchByNameOnly) ;
  }
  delete iter ;

  return result;  
}




void RooSetProxy::removeAll() 
{
  if (!isOwning()) {
    TIterator* iter = createIterator() ;
    RooAbsArg* arg ;
    while ((arg=(RooAbsArg*)iter->Next())) {
      if (!isOwning()) {
	_owner->removeServer(*arg) ;
      }
    }
    delete iter ;
  }

  RooArgSet::removeAll() ;
}




RooSetProxy& RooSetProxy::operator=(const RooArgSet& other) 
{
  RooArgSet::operator=(other) ;
  return *this ;
}




Bool_t RooSetProxy::changePointer(const RooAbsCollection& newServerList, Bool_t nameChange) 
{
  if (getSize()==0) return kTRUE ;

  _iter->Reset() ;
  RooAbsArg* arg ;
  Bool_t error(kFALSE) ;
  while ((arg=(RooAbsArg*)_iter->Next())) {
    
    RooAbsArg* newArg= arg->findNewServer(newServerList, nameChange);
    if (newArg) error |= !RooArgSet::replace(*arg,*newArg) ;
  }
  return !error ;
}

