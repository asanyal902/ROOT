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
// A class description belongs here...


#include "RooFit.h"
#include "RooMsgService.h"
#include "Riostream.h"

#include "RooGenContext.h"
#include "RooGenContext.h"
#include "RooAbsPdf.h"
#include "RooDataSet.h"
#include "RooRealIntegral.h"
#include "RooAcceptReject.h"
#include "RooRealVar.h"
#include "RooDataHist.h"
#include "RooErrorHandler.h"

#include "TString.h"
#include "TIterator.h"



ClassImp(RooGenContext)
  ;


RooGenContext::RooGenContext(const RooAbsPdf &model, const RooArgSet &vars,
			     const RooDataSet *prototype, const RooArgSet* auxProto,
			     Bool_t verbose, const RooArgSet* forceDirect) :  
  RooAbsGenContext(model,vars,prototype,auxProto,verbose),
  _cloneSet(0), _pdfClone(0), _acceptRejectFunc(0), _generator(0),
  _maxVar(0), _uniIter(0), _updateFMaxPerEvent(0) 
{
  // Initialize a new context for generating events with the specified
  // variables, using the specified PDF model. A prototype dataset (if provided)
  // is not cloned and still belongs to the caller. The contents and shape
  // of this dataset can be changed between calls to generate() as long as the
  // expected columns to be copied to the generated dataset are present.

  cxcoutI(Generation) << "RooGenContext::ctor() setting up event generator context for p.d.f. " << model.GetName() 
			<< " for generation of observable(s) " << vars ;
  if (prototype) ccxcoutI(Generation) << " with prototype data for " << *prototype->get() ;
  if (auxProto && auxProto->getSize()>0)  ccxcoutI(Generation) << " with auxiliary prototypes " << *auxProto ;
  if (forceDirect && forceDirect->getSize()>0)  ccxcoutI(Generation) << " with internal generation forced for observables " << *forceDirect ;
  ccxcoutI(Generation) << endl ;


  // Clone the model and all nodes that it depends on so that this context
  // is independent of any existing objects.
  RooArgSet nodes(model,model.GetName());
  _cloneSet= (RooArgSet*) nodes.snapshot(kTRUE);
  if (!_cloneSet) {
    coutE(Generation) << "RooGenContext::RooGenContext(" << GetName() << ") Couldn't deep-clone PDF, abort," << endl ;
    RooErrorHandler::softAbort() ;
  }

  // Find the clone in the snapshot list
  _pdfClone = (RooAbsPdf*)_cloneSet->find(model.GetName());

  // Optionally fix RooAddPdf normalizations
  if (prototype&&_pdfClone->dependsOn(*prototype->get())) {
    RooArgSet fullNormSet(vars) ;
    fullNormSet.add(*prototype->get()) ;
    _pdfClone->fixAddCoefNormalization(fullNormSet) ;
  }
      
  // Analyze the list of variables to generate...
  _isValid= kTRUE;
  TIterator *iterator= vars.createIterator();
  TIterator *servers= _pdfClone->serverIterator();
  const RooAbsArg *tmp = 0;
  const RooAbsArg *arg = 0;
  while((_isValid && (tmp= (const RooAbsArg*)iterator->Next()))) {
    // is this argument derived?
    if(tmp->isDerived()) {
      coutE(Generation) << "RooGenContext::ctor(): cannot generate values for derived \""  << tmp->GetName() << "\"" << endl;
      _isValid= kFALSE;
      continue;
    }
    // lookup this argument in the cloned set of PDF dependents
    arg= (const RooAbsArg*)_cloneSet->find(tmp->GetName());
    if(0 == arg) {
      coutI(Generation) << "RooGenContext::ctor() WARNING model does not depend on \"" << tmp->GetName() 
			  << "\" which will have uniform distribution" << endl;
      _uniformVars.add(*tmp);
    }
    else {

      // does the model depend on this variable directly, ie, like "x" in
      // f(x) or f(x,g(x,y)) or even f(x,x) ?      
      const RooAbsArg *direct= arg ;
      if (forceDirect==0 || !forceDirect->find(direct->GetName())) {
	if (!_pdfClone->isDirectGenSafe(*arg)) {
	  cxcoutD(Generation) << "RooGenContext::ctor() observable " << arg->GetName() << " has been determined to be unsafe for internal generation" << endl;
	  direct=0 ;
	}
      }
      
      // does the model depend indirectly on this variable through an lvalue chain?	
      // otherwise, this variable will have to be generated with accept/reject
      if(direct) { 
	_directVars.add(*arg);
      } else {
	_otherVars.add(*arg);
      }
    }
  }
  delete servers;
  delete iterator;
  if(!isValid()) {
    coutE(Generation) << "RooGenContext::ctor() constructor failed with errors" << endl;
    return;
  }

  // At this point directVars are all variables that are safe to be generated directly
  //               otherVars are all variables that are _not_ safe to be generated directly
  if (_directVars.getSize()>0) {
    cxcoutD(Generation) << "RooGenContext::ctor() observables " << _directVars << " are safe for internal generator (if supported by p.d.f)" << endl ;
  }
  if (_otherVars.getSize()>0) {
    cxcoutD(Generation) << "RooGenContext::ctor() observables " << _otherVars << " are NOT safe for internal generator (if supported by p.d.f)" << endl ;
  }

  // If PDF depends on prototype data, direct generator cannot use static initialization
  // in initGenerator()
  Bool_t staticInitOK = !_pdfClone->dependsOn(_protoVars) ;
  if (!staticInitOK) {
    cxcoutD(Generation) << "RooGenContext::ctor() Model depends on supplied protodata observables, static initialization of internal generator will not be allowed" << endl ;
  }
  
  // Can the model generate any of the direct variables itself?
  RooArgSet generatedVars;
  if (_directVars.getSize()>0) {
    _code= _pdfClone->getGenerator(_directVars,generatedVars,staticInitOK);
    
    cxcoutD(Generation) << "RooGenContext::ctor() Model indicates that it can internally generate observables " 
			  << generatedVars << " with configuration identifier " << _code << endl ;
  } else {
    _code = 0 ;
  }

  // Move variables which cannot be generated into the list to be generated with accept/reject
  _directVars.remove(generatedVars);
  _otherVars.add(_directVars);

  // Update _directVars to only include variables that will actually be directly generated
  _directVars.removeAll();
  _directVars.add(generatedVars);

  cxcoutI(Generation) << "RooGenContext::ctor() Context will" ;
  if (_directVars.getSize()>0) ccxcoutI(Generation) << " let model internally generate observables " << _directVars ;
  if (_directVars.getSize()>0 && _otherVars.getSize()>0)  ccxcoutI(Generation) << " and" ;
  if (_otherVars.getSize()>0)  ccxcoutI(Generation) << " generate variables " << _otherVars << " with accept/reject sampling" ;
  if (_uniformVars.getSize()>0) ccxcoutI(Generation) << ", non-observable variables " << _uniformVars << " will be generated with flat distribution" ;
  ccxcoutI(Generation) << endl ;

  // initialize the accept-reject generator
  RooArgSet *depList= _pdfClone->getObservables(_theEvent);
  depList->remove(_otherVars);

  TString nname(_pdfClone->GetName()) ;
  nname.Append("_AccRej") ;
  TString ntitle(_pdfClone->GetTitle()) ;
  ntitle.Append(" (Accept/Reject)") ;

  if (_protoVars.getSize()==0) {

    // No prototype variables
    
    if(depList->getSize()==0) {
      // All variable are generated with accept-reject
      
      // Check if PDF supports maximum finding
      Int_t maxFindCode = _pdfClone->getMaxVal(_otherVars) ;
      if (maxFindCode != 0) {
	coutI(Generation) << "RooGenContext::ctor() no prototype data provided, all observables are generated with accept/reject and "
			      << "model supports maximum finding, initial sampling phase skipped" << endl ;
	Double_t maxVal = _pdfClone->maxVal(maxFindCode) / _pdfClone->getNorm(_theEvent) ;
	_maxVar = new RooRealVar("funcMax","function maximum",maxVal) ;
	cxcoutD(Generation) << "RooGenContext::ctor() maximum value returned by RooAbsPdf::maxVal() is " << maxVal << endl ;
      }
    }

    if (_otherVars.getSize()>0) {
      _pdfClone->getVal(&vars) ; // WVE debug
      _acceptRejectFunc= new RooRealIntegral(nname,ntitle,*_pdfClone,*depList,&vars);
      cxcoutI(Generation) << "RooGenContext::ctor() accept/reject sampling function is " << _acceptRejectFunc->GetName() << endl ;
    } else {
      _acceptRejectFunc = 0 ;
    }

  } else {

    // Generation with prototype variable
    depList->remove(_protoVars,kTRUE,kTRUE) ;
    _acceptRejectFunc= (RooRealIntegral*) _pdfClone->createIntegral(*depList,vars) ;
    cxcoutI(Generation) << "RooGenContext::ctor() accept/reject sampling function is " << _acceptRejectFunc->GetName() << endl ;
    
    if (_directVars.getSize()==0)  {
      
      // Check if PDF supports maximum finding
      Int_t maxFindCode = _pdfClone->getMaxVal(_otherVars) ;
      cout << "maxFindCode = " << maxFindCode << endl ;
      if (maxFindCode != 0) {
	
	// Special case: PDF supports max-finding in otherVars, no need to scan other+proto space for maximum
	coutI(Generation) << "RooGenContext::ctor() prototype data provided, all observables are generated with accept/reject and "
			    << "model supports maximum finding, initial sampling phase skipped" << endl ;
	_maxVar = new RooRealVar("funcMax","function maximum",1) ;
	_updateFMaxPerEvent = maxFindCode ;
	cxcoutD(Generation) << "RooGenContext::ctor() maximum value must be reevaluated for each event with configuration code " << maxFindCode << endl ;
      }
    }
    
    if (!_maxVar) {
      
      // Regular case: First find maximum in other+proto space
      RooArgSet otherAndProto(_otherVars) ;

      RooArgSet* protoDeps = model.getObservables(_protoVars) ;
      otherAndProto.add(*protoDeps) ;
      delete protoDeps ;
      
      if (_otherVars.getSize()>0) {      

	cxcoutD(Generation) << "RooGenContext::ctor() sampling observables through accept/reject sample and have prototype data and no " 
			      << "maximum function value provided by mode, will determine maximum value through initial sampling space "
			      << "of accept/reject observables plus prototype observables: " << otherAndProto << endl ;

	// Calculate maximum in other+proto space if there are any accept/reject generated observables
	RooAcceptReject maxFinder(*_acceptRejectFunc,otherAndProto,0,_verbose) ;
	Double_t max = maxFinder.getFuncMax() ;
	_maxVar = new RooRealVar("funcMax","function maximum",max) ;

	cxcoutD(Generation) << "RooGenContext::ctor() maximum function value found through initial sampling is " << max << endl ;
      }
    }
      
  }

  if (_acceptRejectFunc) {
    cxcoutD(Generation) << "RooGenContext::ctor() creating accept reject generator from a/r function for observables " << _otherVars << endl ;
    _generator= new RooAcceptReject(*_acceptRejectFunc,_otherVars,_maxVar,_verbose);
  } else {
    _generator = 0 ;
  }

  delete depList;
  _otherVars.add(_uniformVars);
}

RooGenContext::~RooGenContext() 
{
  // Destructor.

  // Clean up the cloned objects used in this context.
  delete _cloneSet;

  // Clean up our accept/reject generator
  if (_generator) delete _generator;
  if (_acceptRejectFunc) delete _acceptRejectFunc;
  if (_maxVar) delete _maxVar ;
  if (_uniIter) delete _uniIter ;
}

void RooGenContext::initGenerator(const RooArgSet &theEvent) {

  // Attach the cloned model to the event buffer we will be filling.
  _pdfClone->recursiveRedirectServers(theEvent,kFALSE);
  if (_acceptRejectFunc) {
    _acceptRejectFunc->recursiveRedirectServers(theEvent,kFALSE) ; // WVE DEBUG
  }

  // Attach the RooAcceptReject generator the event buffer
  if (_generator) {
    _generator->attachParameters(theEvent) ;
  }

  // Reset the cloned model's error counters.
  _pdfClone->resetErrorCounters();

  // Initialize the PDFs internal generator
  if (_directVars.getSize()>0) {
    cxcoutD(Generation) << "RooGenContext::initGenerator() initializing internal generator of model with code " << _code << endl ;
    _pdfClone->initGenerator(_code) ;
  }

  // Create iterator for uniform vars (if any)
  if (_uniformVars.getSize()>0) {
    _uniIter = _uniformVars.createIterator() ;
  }
}

void RooGenContext::generateEvent(RooArgSet &theEvent, Int_t remaining) {
  // Generate variables for a new event.

  if(_otherVars.getSize() > 0) {
    // call the accept-reject generator to generate its variables

    if (_updateFMaxPerEvent!=0) {
      Double_t max = _pdfClone->maxVal(_updateFMaxPerEvent)/_pdfClone->getNorm(_otherVars) ;
      cxcoutD(Generation) << "RooGenContext::initGenerator() reevaluation of maximum function value is required for each event, new value is  " << max << endl ;
      _maxVar->setVal(max) ;
    }

    if (_generator) {
      const RooArgSet *subEvent= _generator->generateEvent(remaining);

      if(0 == subEvent) {
	coutE(Generation) << "RooGenContext::generateEvent ERROR accept/reject generator failed" << endl ;
	return;
      }
      theEvent= *subEvent;
      
    }
  }

  // Use the model's optimized generator, if one is available.
  // The generator writes directly into our local 'event' since we attached it above.
  if(_directVars.getSize() > 0) {
    _pdfClone->generateEvent(_code);
  }

  // Generate uniform variables (non-dependents)  
  if (_uniIter) {
    _uniIter->Reset() ;
    RooAbsArg* uniVar ;
    while((uniVar=(RooAbsArg*)_uniIter->Next())) {
      RooAbsLValue* arglv = dynamic_cast<RooAbsLValue*>(uniVar) ;
      if (!arglv) {
	coutE(Generation) << "RooGenContext::generateEvent(" << GetName() << ") ERROR: uniform variable " << uniVar->GetName() << " is not an lvalue" << endl ;
	RooErrorHandler::softAbort() ;
      }
      arglv->randomize() ;
    }
    theEvent = _uniformVars ;
  }

}

void RooGenContext::printToStream(ostream &os, PrintOption opt, TString indent) const
{
  RooAbsGenContext::printToStream(os,opt,indent);
  if(opt >= Standard) {
    PrintOption less= lessVerbose(opt);
    TString deeper(indent);
    indent.Append("  ");
    os << indent << "Using PDF ";
    _pdfClone->printToStream(os,less,deeper);
    if(opt >= Verbose) {
      os << indent << "Use PDF generator for ";
      _directVars.printToStream(os,less,deeper);
      os << indent << "Use accept/reject for ";
      _otherVars.printToStream(os,less,deeper);
    }
  }
}
