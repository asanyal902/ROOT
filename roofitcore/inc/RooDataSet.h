/*****************************************************************************
 * Project: BaBar detector at the SLAC PEP-II B-factory
 * Package: RooFitCore
 *    File: $Id: RooDataSet.rdl,v 1.43 2002/02/20 19:46:21 verkerke Exp $
 * Authors:
 *   DK, David Kirkby, Stanford University, kirkby@hep.stanford.edu
 *   WV, Wouter Verkerke, UC Santa Barbara, verkerke@slac.stanford.edu
 *   AB, Adrian Bevan, Liverpool University, bevan@slac.stanford.edu
 * History:
 *   01-Nov-1999 DK Created initial version
 *   30-Nov-2000 WV Add support for multiple file reading with optional common path
 *   09-Mar-2001 WV Migrate from RooFitTools and adapt to RooFitCore
 *   24-Aug-2001 AB Added TH2F * createHistogram method
 *
 * Copyright (C) 1999 Stanford University
 *****************************************************************************/
#ifndef ROO_DATA_SET
#define ROO_DATA_SET

class TDirectory ;
class RooAbsRealLValue ;
class RooRealVar ;
#include "RooFitCore/RooTreeData.hh"
#include "RooFitCore/RooDirItem.hh"


class RooDataSet : public RooTreeData, public RooDirItem {
public:

  // Constructors, factory methods etc.
  RooDataSet() ; 
  RooDataSet(const char *name, const char *title, const RooArgSet& vars, const char* wgtVarName=0) ;
  RooDataSet(const char *name, const char *title, RooDataSet *ntuple, 
	     const RooArgSet& vars, const char *cuts=0, const char* wgtVarName=0);
  RooDataSet(const char *name, const char *title, RooDataSet *t, 
	     const RooArgSet& vars, const RooFormulaVar& cutVar, const char* wgtVarName=0) ;
  RooDataSet(const char *name, const char *title, TTree *t, 
	     const RooArgSet& vars, const RooFormulaVar& cutVar, const char* wgtVarName=0) ;
  RooDataSet(const char *name, const char *title, TTree *ntuple, 
	     const RooArgSet& vars, const char *cuts=0, const char* wgtVarName=0);
  RooDataSet(const char *name, const char *filename, const char *treename, 
	     const RooArgSet& vars, const char *cuts=0, const char* wgtVarName=0);  
  RooDataSet(RooDataSet const & other, const char* newname=0) ;
  virtual TObject* Clone(const char* newname=0) const { return new RooDataSet(*this,newname?newname:GetName()) ; }
  virtual ~RooDataSet() ;

  virtual RooAbsData* emptyClone(const char* newName=0, const char* newTitle=0) const {
    return new RooDataSet(newName?newName:GetName(),newTitle?newTitle:GetTitle(),*get()) ; 
  }

  // Read data from a text file and create a dataset from it.
  // The possible options are: (D)ebug, (Q)uiet.
  static RooDataSet *read(const char *filename, const RooArgList &variables,
			  const char *opts= "", const char* commonPath="",
			  const char *indexCatName=0) ;
  Bool_t write(const char* filename) ;

  void setWeightVar(const char* name=0) ;
  void setWeightVar(const RooAbsArg& arg) { setWeightVar(arg.GetName()) ; }

  virtual Double_t weight() const ; 
  virtual const RooArgSet* get(Int_t index) const;
  virtual const RooArgSet* get() const ; 

  // Add one ore more rows of data
  virtual void add(const RooArgSet& row, Double_t weight=1.0);
  void append(RooDataSet& data) ;
  Bool_t merge(RooDataSet* data1, RooDataSet* data2=0, RooDataSet* data3=0, 
	       RooDataSet* data4=0, RooDataSet* data5=0, RooDataSet* data6=0) ;

  virtual RooAbsArg* addColumn(RooAbsArg& var) ;
  virtual RooArgSet* addColumns(const RooArgList& varList) ;

  // Plot the distribution of a real valued arg
  TH2F* createHistogram(const RooAbsRealLValue& var1, const RooAbsRealLValue& var2, const char* cuts="", 
			const char *name= "hist") const;	 
  TH2F* createHistogram(const RooAbsRealLValue& var1, const RooAbsRealLValue& var2, Int_t nx, Int_t ny,
                        const char* cuts="", const char *name="hist") const;

  void printToStream(ostream& os, PrintOption opt, TString indent) const ;

protected:

  friend class RooProdGenContext ;
  Bool_t merge(const TList& data) ;

  void initialize(const char* wgtVarName) ;
  
  // Cache copy feature is not publicly accessible
  RooAbsData* reduceEng(const RooArgSet& varSubset, const RooFormulaVar* cutVar, Bool_t copyCache=kTRUE) ;
  RooDataSet(const char *name, const char *title, RooDataSet *ntuple, 
	     const RooArgSet& vars, const RooFormulaVar* cutVar, Bool_t copyCache);
  
  RooArgSet addWgtVar(const RooArgSet& origVars, const RooAbsArg* wgtVar) ;

  RooArgSet _varsNoWgt ;   // Vars without weight variable
  RooRealVar* _wgtVar ;

  ClassDef(RooDataSet,1) // Unbinned data set
};

#endif
