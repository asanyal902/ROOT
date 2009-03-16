/*****************************************************************************
 * Project: RooFit                                                           *
 * Package: RooFitCore                                                       *
 *    File: $Id$
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

#ifndef ROO_FACTORY_WS_TOOL
#define ROO_FACTORY_WS_TOOL

#include "RooArgSet.h"
#include "RooArgList.h"
#include <string>
#include <list>
#include <vector>
#include <stack>

class RooAbsReal ;
class RooAbsRealLValue ;
class RooAbsPdf ;
class RooWorkspace ;
class RooRealVar ;
class RooCategory ;
class RooAddPdf ;
class RooProdPdf ;
class RooSimultaneous ;
class RooDataHist ;
class RooDataSet ;
class RooAbsData ;
class RooFactoryWSToolSpec ;
class RooAbsCategoryLValue ;
class RooAbsCategory ;
class RooResolutionModel ;
class RooAddition ;
class RooProduct ;

class RooFactoryWSTool : public TNamed, public RooPrintable {

public:

  // Constructors, assignment etc
  RooFactoryWSTool(RooWorkspace& ws) ;
  virtual ~RooFactoryWSTool() ;

  // --- low level factory interface ---

  // Create variables
  RooRealVar* createVariable(const char* name, Double_t xmin, Double_t xmax) ;
  RooCategory* createCategory(const char* name, const char* stateNameList=0) ;

  // Create functions and p.d.f.s (any RooAbsArg)
  RooAbsArg* createArg(const char* className, const char* objName, const char* varList) ;
  std::vector<std::string> ctorArgs(const char* className) ;

  // Create operator p.d.f.s
  RooAddPdf* add(const char *objName, const char* specList, Bool_t recursiveCoefs=kFALSE) ;
  RooProdPdf* prod(const char *objName, const char* pdfList) ;
  RooSimultaneous* simul(const char* objName, const char* indexCat, const char* pdfMap) ;

  // Create operator functions
  RooAddition* addfunc(const char *objName, const char* specList) ;
  RooProduct* prodfunc(const char *objName, const char* pdfList) ;

  RooWorkspace& ws() { return *_ws ; }

  // --- High level factory interface ---

  // Composite object construction language parser
  RooAbsArg* process(const char* expr) ;
  std::string processExpression(const char* expr) ;
  std::vector<std::string> splitFunctionArgs(const char* funcExpr) ;


  // --- Internal stuff that must be public so that CINT can access it ---

  // CINT constructor interface
  static RooAbsArg& as_ARG(UInt_t idx) { checkIndex(idx) ; return _of->_of->asARG(_of->_args[idx].c_str()) ; }

  static RooAbsPdf& as_PDF(UInt_t idx) { checkIndex(idx) ; return _of->asPDF(_of->_args[idx].c_str()) ; }
  static RooAbsReal& as_FUNC(UInt_t idx) { checkIndex(idx) ; return _of->asFUNC(_of->_args[idx].c_str()) ; }
  static RooRealVar& as_VAR(UInt_t idx) { checkIndex(idx) ; return _of->asVAR(_of->_args[idx].c_str()) ; }
  static RooAbsRealLValue& as_VARLV(UInt_t idx) { checkIndex(idx) ; return _of->asVARLV(_of->_args[idx].c_str()) ; }
  static RooResolutionModel& as_RMODEL(UInt_t idx) { checkIndex(idx) ; return _of->asRMODEL(_of->_args[idx].c_str()) ; }

  static RooCategory& as_CAT(UInt_t idx) { checkIndex(idx) ; return _of->asCAT(_of->_args[idx].c_str()) ; }
  static RooAbsCategoryLValue& as_CATLV(UInt_t idx) { checkIndex(idx) ; return _of->asCATLV(_of->_args[idx].c_str()) ; }
  static RooAbsCategory& as_CATFUNC(UInt_t idx) { checkIndex(idx) ; return _of->asCATFUNC(_of->_args[idx].c_str()) ; }

  static RooArgSet as_SET(UInt_t idx) { checkIndex(idx) ; return _of->asSET(_of->_args[idx].c_str()) ; }
  static RooArgList as_LIST(UInt_t idx) { checkIndex(idx) ; return _of->asLIST(_of->_args[idx].c_str()) ; }

  static RooAbsData& as_DATA(UInt_t idx) { checkIndex(idx) ; return _of->asDATA(_of->_args[idx].c_str()) ; }
  static RooDataHist& as_DHIST(UInt_t idx) { checkIndex(idx) ; return _of->asDHIST(_of->_args[idx].c_str()) ; }
  static RooDataSet& as_DSET(UInt_t idx) { checkIndex(idx) ; return _of->asDSET(_of->_args[idx].c_str()) ; }

  static const char* as_STRING(UInt_t idx) { checkIndex(idx) ; return _of->asSTRING(_of->_args[idx].c_str()) ; }
  static Int_t as_INT(UInt_t idx) { checkIndex(idx) ; return _of->asINT(_of->_args[idx].c_str()) ; }
  static Double_t as_DOUBLE(UInt_t idx) { checkIndex(idx) ; return _of->asDOUBLE(_of->_args[idx].c_str()) ; }
  static Int_t as_INT(UInt_t idx, Int_t defVal) { checkIndex(idx) ;   if (idx>_of->_args.size()-1) return defVal ; return _of->asINT(_of->_args[idx].c_str()) ; }
  static Double_t as_DOUBLE(UInt_t idx, Double_t defVal) { checkIndex(idx) ;   if (idx>_of->_args.size()-1) return defVal ; return _of->asDOUBLE(_of->_args[idx].c_str()) ; }

  RooAbsArg& asARG(const char*) ;

  RooAbsPdf& asPDF(const char*) ;
  RooAbsReal& asFUNC(const char*) ;
  RooRealVar& asVAR(const char*) ;
  RooAbsRealLValue& asVARLV(const char*) ;
  RooResolutionModel& asRMODEL(const char*) ;

  RooCategory& asCAT(const char*) ;
  RooAbsCategoryLValue& asCATLV(const char*) ;
  RooAbsCategory& asCATFUNC(const char*) ;

  RooArgSet asSET(const char*) ; 
  RooArgList asLIST(const char*) ; 

  RooAbsData& asDATA(const char*) ;
  RooDataHist& asDHIST(const char*) ;
  RooDataSet& asDSET(const char*) ;

  
  class IFace {
  public:
    virtual std::string create(RooFactoryWSTool& ft, const char* typeName, const char* instanceName, std::vector<std::string> args) = 0 ;
  } ;

  class SpecialsIFace : public IFace {
  public:
    std::string create(RooFactoryWSTool& ft, const char* typeName, const char* instanceName, std::vector<std::string> args) ;    
  } ;

  static void registerSpecial(const char* typeName, RooFactoryWSTool::IFace* iface) ;

protected:

  std::stack<std::string> _autoNamePrefix ; 
  std::map<std::string,std::string> _typeAliases ;

  static void checkIndex(UInt_t index) ;

  const char* asSTRING(const char*) ;
  Int_t asINT(const char*) ;
  Double_t asDOUBLE(const char*) ;
  
  std::string processCompositeExpression(const char* arg) ;
  std::string processSingleExpression(const char* arg) ;
  std::string processListExpression(const char* arg) ;
  std::string processAliasExpression(const char* arg) ;

  std::string processCreateVar(std::string& func, std::vector<std::string>& args) ;
  std::string processCreateArg(std::string& func, std::vector<std::string>& args) ;
  std::string processMetaArg(std::string& func, std::vector<std::string>& args) ;

  TClass* resolveClassName(const char* className) ;

  // CINT constructor interface back end
  static RooFactoryWSTool* _of ;
  std::vector<std::string> _args ;    

  // Hooks for other tools
  static std::map<std::string,IFace*>& hooks() ;
  static std::map<std::string,IFace*>* _hooks ;

  RooWorkspace* _ws ; //! Associated workspace

  void logError() { _errorCount++ ; }
  void clearError() { _errorCount = 0 ; }
  Int_t errorCount() { return _errorCount ; }

  Int_t _errorCount ; // Error counter for a given expression processing

  RooFactoryWSTool(const RooFactoryWSTool&) ;

  ClassDef(RooFactoryWSTool,0) // RooFit class code and instance factory 
} ;



#endif
