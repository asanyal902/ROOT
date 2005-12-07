// @(#)root/cintex:$Name:  $:$Id: Cintex.cxx,v 1.4 2005/11/17 14:12:33 roiser Exp $
// Author: Pere Mato 2005

// Copyright CERN, CH-1211 Geneva 23, 2004-2005, All rights reserved.
//
// Permission to use, copy, modify, and distribute this software for any
// purpose is hereby granted without fee, provided that this copyright and
// permissions notice appear in all copies and derivatives.
//
// This software is provided "as is" without express or implied warranty.

#include "Reflex/Callback.h"
#include "Reflex/Type.h"
#include "Reflex/Member.h"
#include "Reflex/Member.h"
#include "Reflex/Builder/ReflexBuilder.h"

#include "Cintex/Cintex.h"
#include "CINTClassBuilder.h"
#include "CINTFunctionBuilder.h"
#include "CINTTypedefBuilder.h"
#ifndef G__CINT
#include "ROOTClassEnhancer.h"
#endif
#include <iostream>

using namespace ROOT::Reflex;
using namespace ROOT::Cintex;
using namespace std;

//Build the Cintex dictionary on Reflex and convert it to CINT
namespace {
  struct Cintex_dict { 
    public:
      Cintex_dict() {
        //--Reflex class builder
        //NamespaceBuilder( "ROOT::Cintex" );
        Type t_void = TypeBuilder("void");
        Type t_int  = TypeBuilder("int");
        ClassBuilderT< Cintex >("Cintex", PUBLIC)
         .AddFunctionMember(FunctionTypeBuilder(t_void), "enable", Enable, 0, 0, PUBLIC | STATIC)
         .AddFunctionMember(FunctionTypeBuilder(t_void, t_int), "setDebug", SetDebug, 0, 0, PUBLIC | STATIC)
         .AddFunctionMember(FunctionTypeBuilder(t_int), "debug", Debug, 0, 0, PUBLIC | STATIC);
        //--CINT class builder
	      Type t = Type::ByName("Cintex");
        ROOT::Cintex::CINTClassBuilder::Get(t).Setup();
      }
      static void* Enable(void*, const std::vector<void*>&, void*) {
        Cintex::Enable();
        return 0;
      }
      static void* SetDebug(void*, const std::vector<void*>& arg, void*) {
        Cintex::SetDebug(*(bool*)arg[0]);
        return 0;
      }
      static void* Debug(void*, const std::vector<void*>&, void*) {
        static int b = Cintex::Debug();
        return &b;
      }

  };
  static Cintex_dict s_dict;
}


namespace ROOT {
  namespace Cintex {
    
  Cintex& Cintex::Instance() {
    static Cintex s_instance;
    return s_instance;
  }

  Cintex::Cintex() {
    fCallback = new Callback();
    fRootcreator = 0;
    fDbglevel = 0;
    fEnabled = false;
  }

  Cintex::~Cintex() {
    if( fCallback ) UninstallClassCallback( fCallback );
    delete fCallback;
  }

  void Cintex::Enable() {
    if ( Instance().fEnabled ) return;
    //---Install the callback to fothcoming classes ----//
    InstallClassCallback( Instance().fCallback );        
    //---Convert to CINT all existing classes ---//
    for( size_t i = 0; i < Type::TypeSize(); i++ ) {

      ( * Instance().fCallback)( Type::TypeAt(i) );
    }
    //---Convert to CINT all existing free functions
    for ( size_t n = 0; n < Scope::ScopeSize(); n++ ) {
      Scope ns = Scope::ScopeAt(n);
      if ( ns.IsNamespace() ) {
        for( size_t m = 0; m < ns.FunctionMemberSize(); m++ ) {
          ( * Instance().fCallback)( ns.FunctionMemberAt(m) );
        }
      }
    }
    Instance().fEnabled = true;
  } 

  void Cintex::SetROOTCreator(ROOTCreator c) {
    Instance().fRootcreator = c;
  }

  ROOTCreator Cintex::GetROOTCreator() {
    return Instance().fRootcreator;
  }

  int Cintex::Debug() {
    return Instance().fDbglevel;
  }

  void Cintex::SetDebug(int l) {
    Instance().fDbglevel = l;
  }
  
  void Callback::operator () ( const Type& t ) {
    if ( t.IsClass() || t.IsStruct() ) {
#ifndef G__CINT
      ROOTClassEnhancer enhancer(t);
      enhancer.Setup();
#endif
      CINTClassBuilder::Get(t).Setup();
#ifndef G__CINT
      enhancer.CreateInfo();
#endif
    }
    else if ( t.IsTypedef() ) {
      CINTTypedefBuilder::Setup(t);
    }
  }
  
  void Callback::operator () ( const Member& m ) {
    if ( m.IsFunctionMember() ) {
      if( Cintex::Debug() ) cout << "Building function " << m.Name(SCOPED|QUALIFIED) << endl; 
      CINTFunctionBuilder(m).Setup();
    }
  }
}}
