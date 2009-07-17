// @(#)root/mathcore:$Id$
// Author: L. Moneta Thu Sep 21 16:21:29 2006

/**********************************************************************
 *                                                                    *
 * Copyright (c) 2006  LCG ROOT Math Team, CERN/PH-SFT                *
 *                                                                    *
 *                                                                    *
 **********************************************************************/

// Implementation file for class FitConfig

#include "Fit/FitConfig.h"

#include "Math/IParamFunction.h"
#include "Math/Util.h"

#include "Math/Minimizer.h"
#include "Math/Factory.h"

#include <cmath> 

#include <string> 
#include <sstream> 

#include "Math/Error.h"

//#define DEBUG
#ifdef DEBUG
#include <iostream>
#endif

namespace ROOT { 

namespace Fit { 



FitConfig::FitConfig(unsigned int npar) : 
   fNormErrors(false),
   fParabErrors(false), // ensure that in any case correct parabolic errors are estimated
   fMinosErrors(false),    // do full Minos error analysis for all parameters
   fSettings(std::vector<ParameterSettings>(npar) )  
{
   // constructor implementation
}


FitConfig::~FitConfig() 
{
   // destructor implementation. No Op
}

FitConfig::FitConfig(const FitConfig &rhs) { 
   // Implementation of copy constructor
   (*this) = rhs; 
}

FitConfig & FitConfig::operator = (const FitConfig &rhs) { 
   // Implementation of assignment operator.
   if (this == &rhs) return *this;  // time saving self-test

   fNormErrors = rhs.fNormErrors; 
   fParabErrors = rhs.fParabErrors; 
   fMinosErrors = rhs.fMinosErrors; 

   fSettings = rhs.fSettings; 
   fMinosParams = rhs.fMinosParams; 

   fMinimizerOpts = rhs.fMinimizerOpts;

   return *this;
}

void FitConfig::SetParamsSettings(unsigned int npar, const double *params, const double * vstep ) { 
   // initialize fit config from parameter values
   if (params == 0) { 
      fSettings =  std::vector<ParameterSettings>(npar); 
      return; 
   }
   // if a vector of parameters is given and parameters are not existing or are of different size
   bool createNew = false; 
   if (npar != fSettings.size() ) { 
      fSettings.clear(); 
      fSettings.reserve(npar); 
      createNew = true; 
   }
   unsigned int i = 0; 
   const double * end = params+npar;
   for (const double * ipar = params; ipar !=  end; ++ipar) {  
      double val = *ipar;       
      double step = 0; 
      if (vstep == 0) {  
         step = 0.3*std::fabs(val);   // step size is 30% of par value
         //double step = 2.0*std::fabs(val);   // step size is 30% of par value
         if (val ==  0) step  =  0.3; 
      }
      else 
         step = vstep[i]; 

      if (createNew) 
         fSettings.push_back( ParameterSettings("Par_" + ROOT::Math::Util::ToString(i), val, step ) ); 
      else {
         fSettings[i].SetValue(val); 
         fSettings[i].SetStepSize(step); 
      }

      i++;
   }
}

void FitConfig::CreateParamsSettings(const ROOT::Math::IParamMultiFunction & func) { 
   // initialize from model function
   // set the parameters values from the function
   unsigned int npar = func.NPar(); 
   const double * begin = func.Parameters(); 
   if (begin == 0) { 
      fSettings =  std::vector<ParameterSettings>(npar); 
      return; 
   }

   fSettings.clear(); 
   fSettings.reserve(npar); 
   const double * end =  begin+npar; 
   unsigned int i = 0; 
   for (const double * ipar = begin; ipar !=  end; ++ipar) {  
      double val = *ipar; 
      double step = 0.3*std::fabs(val);   // step size is 30% of par value
      //double step = 2.0*std::fabs(val);   // step size is 30% of par value
      if (val ==  0) step  =  0.3; 
      
      fSettings.push_back( ParameterSettings(func.ParameterName(i), val, step ) ); 
#ifdef DEBUG
      std::cout << "FitConfig: add parameter " <<  func.ParameterName(i) << " val = " << val << std::endl;
#endif
      i++;
   } 

}

ROOT::Math::Minimizer * FitConfig::CreateMinimizer() { 
   // create minimizer according to the chosen configuration using the 
   // plug-in manager

   const std::string & minimType = fMinimizerOpts.MinimizerType(); 
   const std::string & algoType  = fMinimizerOpts.MinimizerAlgorithm(); 

   ROOT::Math::Minimizer * min = ROOT::Math::Factory::CreateMinimizer(minimType, algoType); 

   if (min == 0) { 
      std::string minim2 = "Minuit2";
      if ( minimType != "Minuit" )  minim2 = "Minuit";
      if (minimType != minim2 ) {
         std::string msg = "Could not create the " + minimType + " minimizer. Try using the minimizer " + minim2; 
         MATH_WARN_MSG("FitConfig::CreateMinimizer",msg.c_str());
         min = ROOT::Math::Factory::CreateMinimizer(minim2,"Migrad"); 
         if (min == 0) { 
            MATH_ERROR_MSG("FitConfig::CreateMinimizer","Could not create the Minuit2 minimizer");
            return 0; 
         }
         SetMinimizer( minim2.c_str(),"Migrad"); 
      }
      else {
         std::string msg = "Could not create the Minimizer " + minimType; 
         MATH_ERROR_MSG("FitConfig::CreateMinimizer",msg.c_str());
         return 0;
      }
   } 

   // set default max of function calls according to the number of parameters
   // formula from Minuit2 (adapted)
   if (fMinimizerOpts.MaxFunctionCalls() == 0) {  
      unsigned int npar =  fSettings.size();      
      int maxfcn = 1000 + 100*npar + 5*npar*npar;
      fMinimizerOpts.SetMaxFunctionCalls(maxfcn); 
   }


   // set default minimizer control parameters 
   min->SetPrintLevel( fMinimizerOpts.PrintLevel() ); 
   min->SetMaxFunctionCalls( fMinimizerOpts.MaxFunctionCalls() ); 
   min->SetMaxIterations( fMinimizerOpts.MaxIterations() ); 
   min->SetTolerance( fMinimizerOpts.Tolerance() ); 
   min->SetValidError( fParabErrors );
   min->SetStrategy( fMinimizerOpts.Strategy() );
   min->SetErrorDef( fMinimizerOpts.ErrorDef() );


   return min; 
} 

void FitConfig::SetDefaultMinimizer(const char * type, const char *algo ) { 
   // set the default minimizer type and algorithms
   ROOT::Math::MinimizerOptions::SetDefaultMinimizer(type, algo); 
} 

void FitConfig::SetMinimizerOptions(const ROOT::Math::MinimizerOptions & minopt) {  
   // set minimizer options
   fMinimizerOpts = minopt; 
//    fMinimizerType = minopt.MinimizerType(); 
//    fMinimAlgoType = minopt.MinimizerAlgorithm(); 
//    fMinimizerOpts.SetTolerance(minopt.Tolerance() ); 
//    fMinimizerOpts.SetMaxFunctionCalls(minopt.MaxFunctionCalls() ); 
//    fMinimizerOpts.SetMaxIterations(minopt.MaxIterations() ); 
//    fMinimizerOpts.SetStrategy(minopt.Strategy() ); 
//    fMinimizerOpts.SetPrintLevel(minopt.PrintLevel() ); 
//    fMinimizerOpts.SetErrorDef(minopt.ErrorDef() ); 

}


   } // end namespace Fit

} // end namespace ROOT

