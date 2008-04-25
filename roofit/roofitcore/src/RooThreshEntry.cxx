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

#include "RooFit.h"

#include "TClass.h"
#include "TClass.h"
#include "RooThreshEntry.h"

ClassImp(RooThreshEntry)
;

RooThreshEntry::RooThreshEntry(Double_t inThresh, const RooCatType& inCat) : 
  _thresh(inThresh), _cat(inCat) 
{
}


RooThreshEntry::RooThreshEntry(const RooThreshEntry& other) : 
  TObject(other), _thresh(other._thresh), _cat(other._cat) 
{
}


Int_t RooThreshEntry::Compare(const TObject* other) const 
{
  // Can only compare objects of same type
  if (!other->IsA()->InheritsFrom(RooThreshEntry::Class())) return 0 ;

  RooThreshEntry* otherTE = (RooThreshEntry*) other ;
  return (_thresh < otherTE->_thresh) ? -1 : 1 ;
}


