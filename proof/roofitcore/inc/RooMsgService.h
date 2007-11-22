/*****************************************************************************
 * Project: RooFit                                                           *
 * Package: RooFitCore                                                       *
 *    File: $Id: RooMsgService.h,v 1.2 2007/07/13 21:50:24 wouter Exp $
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
#ifndef ROO_MSG_SERVICE
#define ROO_MSG_SERVICE

#include "Riostream.h"
#include <assert.h>
#include "TObject.h"
#include <map>
#include <string>
#include <vector>
#include <map>
#include "RooCmdArg.h"
class RooAbsArg ;

// Shortcut definitions 
#define coutI(a) RooMsgService::instance().log(this,RooMsgService::INFO,RooMsgService::a) 
#define coutW(a) RooMsgService::instance().log(this,RooMsgService::WARNING,RooMsgService::a) 
#define coutE(a) RooMsgService::instance().log(this,RooMsgService::ERROR,RooMsgService::a) 
#define coutF(a) RooMsgService::instance().log(this,RooMsgService::FATAL,RooMsgService::a) 

#define oocoutI(o,a) RooMsgService::instance().log(o,RooMsgService::INFO,RooMsgService::a) 
#define oocoutW(o,a) RooMsgService::instance().log(o,RooMsgService::WARNING,RooMsgService::a) 
#define oocoutE(o,a) RooMsgService::instance().log(o,RooMsgService::ERROR,RooMsgService::a) 
#define oocoutF(o,a) RooMsgService::instance().log(o,RooMsgService::FATAL,RooMsgService::a) 

#define ANYDEBUG (RooMsgService::_debugCount>0)

#define dologD(a) (ANYDEBUG && RooMsgService::instance().isActive(this,RooMsgService::a,RooMsgService::DEBUG))
#define dologI(a) (RooMsgService::instance().isActive(this,RooMsgService::a,RooMsgService::INFO))
#define dologW(a) (RooMsgService::instance().isActive(this,RooMsgService::a,RooMsgService::WARNING))
#define dologE(a) (RooMsgService::instance().isActive(this,RooMsgService::a,RooMsgService::ERROR))
#define dologF(a) (RooMsgService::instance().isActive(this,RooMsgService::a,RooMsgService::FATAL))

#define oodologD(o,a) (ANYDEBUG && RooMsgService::instance().isActive(o,RooMsgService::a,RooMsgService::DEBUG))
#define oodologI(o,a) (RooMsgService::instance().isActive(o,RooMsgService::a,RooMsgService::INFO))
#define oodologW(o,a) (RooMsgService::instance().isActive(o,RooMsgService::a,RooMsgService::WARNING))
#define oodologE(o,a) (RooMsgService::instance().isActive(o,RooMsgService::a,RooMsgService::ERROR))
#define oodologF(o,a) (RooMsgService::instance().isActive(o,RooMsgService::a,RooMsgService::FATAL))

// Shortcuts definitions with conditional execution of print expression -- USE WITH CAUTION 

#define cxcoutD(a) if (ANYDEBUG && RooMsgService::instance().isActive(this,RooMsgService::a,RooMsgService::DEBUG)) RooMsgService::instance().log(this,RooMsgService::DEBUG,RooMsgService::a) 
#define ccxcoutD(a) if (ANYDEBUG && RooMsgService::instance().isActive(this,RooMsgService::a,RooMsgService::DEBUG)) RooMsgService::instance().log(this,RooMsgService::DEBUG,RooMsgService::a,kTRUE) 
#define oocxcoutD(o,a) if (ANYDEBUG && RooMsgService::instance().isActive(o,RooMsgService::a,RooMsgService::DEBUG)) RooMsgService::instance().log(o,RooMsgService::DEBUG,RooMsgService::a) 
#define ooccxcoutD(o,a) if (ANYDEBUG && RooMsgService::instance().isActive(o,RooMsgService::a,RooMsgService::DEBUG)) RooMsgService::instance().log(o,RooMsgService::DEBUG,RooMsgService::a,kTRUE) 
#define cxcoutI(a) if (RooMsgService::instance().isActive(this,RooMsgService::a,RooMsgService::INFO)) RooMsgService::instance().log(this,RooMsgService::INFO,RooMsgService::a) 
#define ccxcoutI(a) if (RooMsgService::instance().isActive(this,RooMsgService::a,RooMsgService::INFO)) RooMsgService::instance().log(this,RooMsgService::INFO,RooMsgService::a,kTRUE) 
#define oocxcoutI(o,a) if (RooMsgService::instance().isActive(o,RooMsgService::a,RooMsgService::INFO)) RooMsgService::instance().log(o,RooMsgService::INFO,RooMsgService::a) 
#define ooccxcoutI(o,a) if (RooMsgService::instance().isActive(o,RooMsgService::a,RooMsgService::INFO)) RooMsgService::instance().log(o,RooMsgService::INFO,RooMsgService::a,kTRUE) 
#define cxcoutW(a) if (RooMsgService::instance().isActive(this,RooMsgService::a,RooMsgService::WARNING)) RooMsgService::instance().log(this,RooMsgService::WARNING,RooMsgService::a) 
#define ccxcoutW(a) if (RooMsgService::instance().isActive(this,RooMsgService::a,RooMsgService::WARNING)) RooMsgService::instance().log(this,RooMsgService::WARNING,RooMsgService::a,kTRUE) 
#define oocxcoutW(o,a) if (RooMsgService::instance().isActive(o,RooMsgService::a,RooMsgService::WARNING)) RooMsgService::instance().log(o,RooMsgService::WARNING,RooMsgService::a) 
#define ooccxcoutW(o,a) if (RooMsgService::instance().isActive(o,RooMsgService::a,RooMsgService::WARNING)) RooMsgService::instance().log(o,RooMsgService::WARNING,RooMsgService::a,kTRUE) 
#define cxcoutE(a) if (RooMsgService::instance().isActive(this,RooMsgService::a,RooMsgService::ERROR)) RooMsgService::instance().log(this,RooMsgService::ERROR,RooMsgService::a) 
#define ccxcoutE(a) if (RooMsgService::instance().isActive(this,RooMsgService::a,RooMsgService::ERROR)) RooMsgService::instance().log(this,RooMsgService::ERROR,RooMsgService::a,kTRUE) 
#define oocxcoutE(o,a) if (RooMsgService::instance().isActive(o,RooMsgService::a,RooMsgService::ERROR)) RooMsgService::instance().log(to,RooMsgService::ERROR,RooMsgService::a) 
#define ooccxcoutE(o,a) if (RooMsgService::instance().isActive(o,RooMsgService::a,RooMsgService::ERROR)) RooMsgService::instance().log(o,RooMsgService::ERROR,RooMsgService::a,kTRUE) 
#define cxcoutF(a) if (RooMsgService::instance().isActive(this,RooMsgService::a,RooMsgService::FATAL)) RooMsgService::instance().log(this,RooMsgService::FATAL,RooMsgService::a) 
#define ccxcoutF(a) if (RooMsgService::instance().isActive(this,RooMsgService::a,RooMsgService::FATAL)) RooMsgService::instance().log(this,RooMsgService::FATAL,RooMsgService::a,kTRUE) 
#define oocxcoutF(o,a) if (RooMsgService::instance().isActive(o,RooMsgService::a,RooMsgService::FATAL)) RooMsgService::instance().log(o,RooMsgService::FATAL,RooMsgService::a) 
#define ooccxcoutF(o,a) if (RooMsgService::instance().isActive(o,RooMsgService::a,RooMsgService::FATAL)) RooMsgService::instance().log(o,RooMsgService::FATAL,RooMsgService::a,kTRUE) 

class RooMsgService : public TObject {
public:

  virtual ~RooMsgService() ;

  enum MsgLevel { DEBUG=0, INFO=1, WARNING=2, ERROR=3, FATAL=4 } ;
  enum MsgTopic { Generation=1, Minimization=2, Plotting=4, Fitting=8, Integration=16, ChangeTracking=32, 
                  Eval=64, Caching=128, Optimization=256, Workspace=512, InputArguments=1024, Tracing=2048 } ;

  struct StreamConfig {

    Bool_t active ;
    Bool_t universal ;

    MsgLevel minLevel ;
    Int_t    topic ;
    std::string objectName ;
    std::string className ;
    std::string baseClassName ;
    std::string tagName ;
    Color_t color ;
    Bool_t prefix ;

    ostream* os ;

    Bool_t match(MsgLevel level, MsgTopic facility, const RooAbsArg* obj) ;
    Bool_t match(MsgLevel level, MsgTopic facility, const TObject* obj) ;

  } ;

  // Access to instance
  static RooMsgService& instance() ;
  static Bool_t anyDebug() { return instance()._debugCount>0 ; }

  // User interface -- Add or delete reporting streams ;
  Int_t addStream(MsgLevel level, const RooCmdArg& arg1=RooCmdArg(), const RooCmdArg& arg2=RooCmdArg(), const RooCmdArg& arg3=RooCmdArg(),
                    		  const RooCmdArg& arg4=RooCmdArg(), const RooCmdArg& arg5=RooCmdArg(), const RooCmdArg& arg6=RooCmdArg()); 
  void deleteStream(Int_t id) ;
  StreamConfig getStream(Int_t id) { return _streams[id] ; }

  Int_t numStreams() const { return _streams.size() ; }
  void setStreamStatus(Int_t id, Bool_t active) ;
  Bool_t getStreamStatus(Int_t id) const ;

  void Print(Option_t *options= 0) const ;

  // Back end -- Send message or check if particular logging configuration is active
  ostream& log(const RooAbsArg* self, MsgLevel level, MsgTopic facility, Bool_t forceSkipPrefix=kFALSE) ;
  ostream& log(const TObject* self, MsgLevel level, MsgTopic facility, Bool_t forceSkipPrefix=kFALSE) ;
  Bool_t isActive(const RooAbsArg* self, MsgTopic facility, MsgLevel level) ;
  Bool_t isActive(const TObject* self, MsgTopic facility, MsgLevel level) ;

  static Int_t _debugCount ;
  std::map<int,std::string> _levelNames ;
  std::map<int,std::string> _topicNames ;

  static void cleanup() ;

  // Print level support for RooFit-related messages that are not routed through RooMsgService (such as Minuit printouts)
  Bool_t silentMode() const { return _silentMode ; }  
  void setSilentMode(Bool_t flag) { _silentMode = flag ; }

  Int_t errorCount() const { return _errorCount ; }
  void clearErrorCount() { _errorCount = 0 ; }

protected:

  Int_t activeStream(const RooAbsArg* self, MsgTopic facility, MsgLevel level) ;
  Int_t activeStream(const TObject* self, MsgTopic facility, MsgLevel level) ;

  std::vector<StreamConfig> _streams ;
  ostream* _devnull ;

  std::map<string,ostream*> _files ;

  Bool_t _silentMode ; 

  Int_t _errorCount ; 

  // Private ctor -- singleton class
  RooMsgService() ;
  RooMsgService(const RooMsgService&) ;

  static RooMsgService* _instance ;
  
  ClassDef(RooMsgService,0) // RooFit Message Service Singleton class
};


#ifdef INST_MSG_SERVICE 
RooMsgService* gMsgService = 0 ;
#else
extern RooMsgService* gMsgService ;
#endif 


#endif
