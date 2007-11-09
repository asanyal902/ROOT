// @(#)root/fit:$Id: src/Fitter.cxx,v 1.0 2006/01/01 12:00:00 moneta Exp $
// Author: L. Moneta Mon Sep  4 17:00:10 2006

/**********************************************************************
 *                                                                    *
 * Copyright (c) 2006  LCG ROOT Math Team, CERN/PH-SFT                *
 *                                                                    *
 *                                                                    *
 **********************************************************************/

// Implementation file for class Fitter


#include "Fit/Fitter.h"
#include "Fit/Chi2FCN.h"
//#include "Fit/Chi2GradFCN.h"
#include "Fit/LogLikelihoodFCN.h"
#include "Math/Minimizer.h"
#include "Fit/BinPoint.h"
#include "Fit/DataVector.h"

#include <memory> 

#include "Math/IParamFunction.h" 

#include "Math/WrappedParamFunction.h"

namespace ROOT { 

   namespace Fit { 



Fitter::Fitter() : 
   fFunc(0)
{
   // Default constructor implementation.
}

Fitter::~Fitter() 
{
   // Destructor implementation.

   if (fFunc != 0) delete fFunc; 
}

Fitter::Fitter(const Fitter &) 
{
   // Implementation of copy constructor.
}

Fitter & Fitter::operator = (const Fitter &rhs) 
{
   // Implementation of assignment operator.
   if (this == &rhs) return *this;  // time saving self-test
   return *this;
}

void Fitter::SetFunction(const IModelFunction & func) 
{ 
   //  set the fit model function (clone the given one and keep a copy ) 
   //std::cout << "set a non-grad function" << std::endl; 

   fFunc = dynamic_cast<IModelFunction *> ( func.Clone() ); 

   // function parameters
//    for (int i = 0; i < fFunc->NPar(); ++i) 
//       std::cout << "Func params " << fFunc->Parameters()[i] << std::endl; 
   
   // creates the parameter  settings 
   fConfig.SetParameterSettings(*fFunc); 
}

void Fitter::SetFunction(const IModel1DFunction & func) 
{ 
   std::cout << "set a grad function" << std::endl; 
   //  set the fit model function from 1D func (clone the given one and keep a copy ) 
   // need to wrap the 1D func in a multim func
   std::auto_ptr<IModel1DFunction> fPtr( dynamic_cast<IModel1DFunction *> ( func.Clone() ) );
   assert (fPtr.get() != 0); 
   fFunc = new ROOT::Math::WrappedParamFunction<std::auto_ptr<IModel1DFunction> > 
      (fPtr, 1, fPtr->Parameters(), fPtr->Parameters() + fPtr->NPar() ); 
}


bool Fitter::DoLeastSquareFit(const BinData & data) { 
   // perform a chi2 fit on a set of binned data 


   // create Minimizer  
   std::auto_ptr<ROOT::Math::Minimizer> minimizer = std::auto_ptr<ROOT::Math::Minimizer> ( fConfig.CreateMinimizer() );

   if (minimizer.get() == 0) return false; 
   
   if (fFunc == 0) return false; 

#ifdef DEBUG
   std::cout << "Fitter ParamSettings " << Config().ParamsSettings()[3].IsBound() << " lower limit " <<  Config().ParamsSettings()[3].LowerLimit() << " upper limit " <<  Config().ParamsSettings()[3].UpperLimit() << std::endl;
#endif


   // check if fFunc provides gradient
   IGradModelFunction * gradFun = dynamic_cast<IGradModelFunction *>(fFunc); 
   typedef IModelFunction::BaseFunc BaseFunc; 
   if (gradFun == 0) { 
      // do minimzation without using the gradient
      Chi2FCN<BaseFunc> chi2(data,*fFunc); 

//       double xxx[3] = {5.11,-1.485,1.846}; 
//       std::cout << " chi2(x) " << chi2(xxx) << std::endl; 
//       double xxx2[3] = {2,2,2}; 
//       std::cout << " chi2(2,2,2) " << chi2(xxx2) << std::endl; 

      return DoMinimization<BaseFunc> (*minimizer, chi2, data.Size()); 
   } 
#ifdef LATER
   else { 
      // use gradient 
      typedef IGradModelFunction::BaseGradFunc BaseGradFunc; 
      Chi2FCN<BaseGradFunc> chi2(data,*gradFun); 
      return DoMinimization<BaseGradFunc> (*minimizer, chi2, data.Size()); 
   }
#endif
   return false; 
}

template<class ObjFunc> 
bool Fitter::DoMinimization(ROOT::Math::Minimizer & minimizer, const ObjFunc & objFunc, unsigned int dataSize) { 

   minimizer.SetFunction(objFunc);
   minimizer.SetVariables(fConfig.ParamsSettings().begin(), fConfig.ParamsSettings().end() ); 

   //minimizer.SetPrintLevel(3);
   
   // do minimization
//    if (minimizer.Minimize()) {  
//       fResult = FitResult(minimizer,*fFunc,dataSize ); 
//       return true;
//    } 
//    return false; 

   bool ret = minimizer.Minimize(); 
   fResult = FitResult(minimizer,*fFunc,dataSize, ret );
   return ret; 
}



bool Fitter::DoLikelihoodFit(const BinData & data) { 

   // perform a likelihood fit on a set of binned data 
   return true;
}

bool Fitter::DoLikelihoodFit(const UnBinData & data) { 
   // perform a likelihood fit on a set of unbinned data 

   // create Minimizer  
   std::auto_ptr<ROOT::Math::Minimizer> minimizer = std::auto_ptr<ROOT::Math::Minimizer> ( fConfig.CreateMinimizer() );

   if (minimizer.get() == 0) return false; 
   
   if (fFunc == 0) return false; 

#ifdef DEBUG
   int ipar = 0;
   std::cout << "Fitter ParamSettings " << Config().ParamsSettings()[ipar].IsBound() << " lower limit " <<  Config().ParamsSettings()[ipar].LowerLimit() << " upper limit " <<  Config().ParamsSettings()[ipar].UpperLimit() << std::endl;
#endif

   // logl fit (error is 0.5)
   minimizer->SetErrorUp(0.5);

   // check if fFunc provides gradient
   IGradModelFunction * gradFun = dynamic_cast<IGradModelFunction *>(fFunc); 
   if (gradFun == 0) { 
      // do minimzation without using the gradient
      LogLikelihoodFCN logl(data,*fFunc); 

      return DoMinimization<LogLikelihoodFCN::BaseObjFunction> (*minimizer, logl, data.Size()); 
   } 
   else { 
//       // use gradient 
//       Chi2GradFCN chi2(data,*gradFun); 
//       return DoMinimization<Chi2GradFCN::BaseObjFunction> (*minimizer, chi2, data.Size()); 
   }
   return false; 
}

bool Fitter::DoLinearFit(const BinData & data) { 

   // perform a linear fit on a set of binned data 
   return true; 
}



   } // end namespace Fit

} // end namespace ROOT

