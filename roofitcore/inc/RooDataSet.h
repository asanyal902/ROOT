/*****************************************************************************
 * Project: BaBar detector at the SLAC PEP-II B-factory
 * Package: RooFitCore
 *    File: $Id: RooDataSet.rdl,v 1.3 2001/03/16 07:59:11 verkerke Exp $
 * Authors:
 *   DK, David Kirkby, Stanford University, kirkby@hep.stanford.edu
 *   WV, Wouter Verkerke, UC Santa Barbara, verkerke@slac.stanford.edu
 * History:
 *   01-Nov-1999 DK Created initial version
 *   30-Nov-2000 WV Add support for multiple file reading with optional common path
 *   09-Mar-2001 WV Migrate from RooFitTools and adapt to RooFitCore
 *
 * Copyright (C) 1999 Stanford University
 *****************************************************************************/
#ifndef ROO_DATA_SET
#define ROO_DATA_SET

#include "TTree.h"

#include "RooFitCore/RooArgSet.hh"

class RooAbsArg;
class RooAbsReal;
class TIterator;
class TBranch;
class TH1F;
class TH2F;
class TPaveText;

class RooDataSet : public TTree {
public:

  // Constructors, factory methods etc.
  inline RooDataSet() { }
  RooDataSet(const char *name, const char *title, const RooArgSet& vars) ;
  RooDataSet(const char *name, const char *title, RooDataSet *ntuple, 
	     const RooArgSet& vars, const char *cuts);
  RooDataSet(const char *name, const char *title, TTree *ntuple, 
	     const RooArgSet& vars, const char *cuts);
  RooDataSet(const char *name, const char *filename, const char *treename, 
	     const RooArgSet& vars, const char *cuts);
  inline virtual ~RooDataSet() ;
  // Read data from a text file and create a dataset from it.
  // The possible options are: (D)ebug, (Q)uiet.
  static RooDataSet *read(const char *filename, RooArgSet &variables,
			  const char *opts, const char* commonPath="") ;

  // Add one ore more rows of data
  void add(const RooArgSet& data);
  void append(RooDataSet& data) ;

  // Load a given row of data
  const RooArgSet *get(Int_t index) const;

  // Plot a floating value 
  TH1F* Plot(RooAbsReal& var, const char* cuts="", const char* opts="") ;	
  
  // Printing interface (human readable)
  enum PrintOption { Standard=0 } ;
  virtual void printToStream(ostream& os, PrintOption opt=Standard) ;
  void print(PrintOption opt=Standard) { printToStream(cout,opt) ; }

  // Debug
  void dump() ;

protected:

  // Load data from another TTree
  void loadValues(TTree *ntuple, const char *cuts);
  void loadValues(const char *filename, const char *treename,
		  const char *cuts);

  // Column structure definition
  RooArgSet _vars, _truth;
  TString _blindString ;

private:

  RooDataSet(const RooDataSet &other); // cannot be copied

  void initialize(const RooArgSet& vars);
  TIterator *_iterator; //! don't make this data member persistent
  TBranch *_branch; //! don't make this data member persistent

  enum { bufSize = 8192 };
  ClassDef(RooDataSet,1) // a data set for fitting
};

#endif

