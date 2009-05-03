// @(#)root/mathcore:$Id$

/*************************************************************************
 * Copyright (C) 1995-2007, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#ifdef __CINT__

#pragma link off all globals;
#pragma link off all classes;
#pragma link off all functions;

#pragma link C++ nestedclass;
#pragma link C++ nestedtypedef;

#pragma link C++ namespace ROOT;
#pragma link C++ namespace ROOT::Math;

// for automatic loading
#ifdef MAKE_MAPS
#pragma link C++ class TMath;
//#pragma link C++ class ROOT::Math;
#endif

#pragma link C++ class vector<Double_t>+;
#ifdef G__VECTOR_HAS_CLASS_ITERATOR
#pragma link C++ class vector<Double_t>::iterator; 
#pragma link C++ class vector<Double_t>::reverse_iterator; 
#pragma link C++ function operator!=(vector<Double_t>,vector<Double_t>); 
#pragma link C++ function operator==(vector<Double_t>,vector<Double_t>); 
#pragma link C++ function operator<=(vector<Double_t>,vector<Double_t>); 
#pragma link C++ function operator>=(vector<Double_t>,vector<Double_t>); 
#pragma link C++ function operator<(vector<Double_t>,vector<Double_t>); 
#pragma link C++ function operator>(vector<Double_t>,vector<Double_t>); 
#pragma link C++ function operator!=(vector<Double_t>::iterator,vector<Double_t>::iterator); 
#pragma link C++ function operator==(vector<Double_t>::iterator,vector<Double_t>::iterator); 
#pragma link C++ function operator<=(vector<Double_t>::iterator,vector<Double_t>::iterator); 
#pragma link C++ function operator>=(vector<Double_t>::iterator,vector<Double_t>::iterator); 
#pragma link C++ function operator<(vector<Double_t>::iterator,vector<Double_t>::iterator); 
#pragma link C++ function operator>(vector<Double_t>::iterator,vector<Double_t>::iterator); 
#pragma link C++ function operator+(long,vector<Double_t>::iterator); 
#pragma link C++ function operator-(vector<Double_t>::iterator,vector<Double_t>::iterator); 
#pragma link C++ function operator!=(vector<Double_t>::reverse_iterator,vector<Double_t>::reverse_iterator); 
#pragma link C++ function operator==(vector<Double_t>::reverse_iterator,vector<Double_t>::reverse_iterator); 
//#pragma link C++ function operator<=(vector<Double_t>::reverse_iterator,vector<Double_t>::reverse_iterator); 
//#pragma link C++ function operator>=(vector<Double_t>::reverse_iterator,vector<Double_t>::reverse_iterator); 
//#pragma link C++ function operator<(vector<Double_t>::reverse_iterator,vector<Double_t>::reverse_iterator); 
//#pragma link C++ function operator>(vector<Double_t>::reverse_iterator,vector<Double_t>::reverse_iterator); 
//#pragma link C++ function operator+(long,vector<Double_t>::reverse_iterator); 
//#pragma link C++ function operator-(vector<Double_t>::reverse_iterator,vector<Double_t>::reverse_iterator);
#endif

 
#pragma link C++ global gRandom;

#pragma link C++ class TRandom+;
#pragma link C++ class TRandom1+;
#pragma link C++ class TRandom2+;
#pragma link C++ class TRandom3-;

#pragma link C++ class TVirtualFitter+;

#pragma link C++ class TKDTree<Int_t, Double_t>+;
#pragma link C++ class TKDTree<Int_t, Float_t>+;
#pragma link C++ typedef TKDTreeID;
#pragma link C++ typedef TKDTreeIF;


// ROOT::Math namespace
#pragma link C++ typedef ROOT::Math::IGenFunction;
#pragma link C++ typedef ROOT::Math::IMultiGenFunction;
#pragma link C++ typedef ROOT::Math::IGradFunction;
#pragma link C++ typedef ROOT::Math::IMultiGradFunction;

#pragma link C++ class ROOT::Math::IBaseFunctionOneDim+;
#pragma link C++ class ROOT::Math::IGradientOneDim+;
#pragma link C++ class ROOT::Math::IGradientFunctionOneDim+;
#pragma link C++ class ROOT::Math::IBaseParam+;

#pragma link C++ class ROOT::Math::IParametricFunctionOneDim+;
#pragma link C++ class ROOT::Math::IParametricGradFunctionOneDim+;

#pragma link C++ class ROOT::Math::IBaseFunctionMultiDim+;
#pragma link C++ class ROOT::Math::IGradientMultiDim+;
#pragma link C++ class ROOT::Math::IGradientFunctionMultiDim+;
#pragma link C++ class ROOT::Math::IParametricFunctionMultiDim+;
#pragma link C++ class ROOT::Math::IParametricGradFunctionMultiDim+;

#pragma link C++ class ROOT::Math::ParamFunctor+;
#pragma link C++ class ROOT::Math::Functor-;
#pragma link C++ class ROOT::Math::GradFunctor-;
#pragma link C++ class ROOT::Math::Functor1D-;
#pragma link C++ class ROOT::Math::GradFunctor1D-;

#pragma link C++ class ROOT::Math::Minimizer+;
#pragma link C++ class ROOT::Math::MinimizerOptions+;
#pragma link C++ class ROOT::Math::IntegratorOneDim+;
#pragma link C++ class ROOT::Math::IntegratorMultiDim+;
#pragma link C++ class ROOT::Math::VirtualIntegrator+;
#pragma link C++ class ROOT::Math::VirtualIntegratorOneDim+;
#pragma link C++ class ROOT::Math::VirtualIntegratorMultiDim+;
#pragma link C++ class ROOT::Math::AdaptiveIntegratorMultiDim+;
#pragma link C++ typedef ROOT::Math::Integrator;

#pragma link C++ namespace ROOT::Math::IntegrationOneDim;
#pragma link C++ enum ROOT::Math::IntegrationOneDim::Type;
#pragma link C++ namespace ROOT::Math::IntegrationMultiDim;
#pragma link C++ enum ROOT::Math::IntegrationMultiDim::Type;


#pragma link C++ class ROOT::Math::BasicFitMethodFunction<ROOT::Math::IBaseFunctionMultiDim>+;
#ifndef _WIN32
#pragma link C++ class ROOT::Math::BasicFitMethodFunction<ROOT::Math::IGradientFunctionMultiDim>+;
#else
// problem due to virtual inheritance
#pragma link C++ class ROOT::Math::BasicFitMethodFunction<ROOT::Math::IGradientFunctionMultiDim>-;
#endif
// typedef's
#pragma link C++ typedef ROOT::Math::FitMethodFunction;
#pragma link C++ typedef ROOT::Math::FitMethodGradFunction;


#pragma link C++ class ROOT::Math::Factory+;

#pragma link C++ class ROOT::Math::GaussIntegrator+;
#pragma link C++ class ROOT::Math::GaussLegendreIntegrator+;
#pragma link C++ class ROOT::Math::RichardsonDerivator+;

#pragma link C++ class ROOT::Math::RootFinder+;
#pragma link C++ class ROOT::Math::IRootFinderMethod+;
#pragma link C++ class ROOT::Math::BrentRootFinder+;
#pragma link C++ class ROOT::Math::BrentMinimizer1D+;

#include "LinkDef_Func.h"

#endif
