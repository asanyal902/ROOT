// @(#)root/mathmore:$Id: GSLFunctionWrapper.h,v 1.3 2005/09/19 13:06:53 brun Exp $
// Authors: L. Moneta, A. Zsenei   08/2005 

 /**********************************************************************
  *                                                                    *
  * Copyright (c) 2004 ROOT Foundation,  CERN/PH-SFT                   *
  *                                                                    *
  * This library is free software; you can redistribute it and/or      *
  * modify it under the terms of the GNU General Public License        *
  * as published by the Free Software Foundation; either version 2     *
  * of the License, or (at your option) any later version.             *
  *                                                                    *
  * This library is distributed in the hope that it will be useful,    *
  * but WITHOUT ANY WARRANTY; without even the implied warranty of     *
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU   *
  * General Public License for more details.                           *
  *                                                                    *
  * You should have received a copy of the GNU General Public License  *
  * along with this library (see file COPYING); if not, write          *
  * to the Free Software Foundation, Inc., 59 Temple Place, Suite      *
  * 330, Boston, MA 02111-1307 USA, or contact the author.             *
  *                                                                    *
  **********************************************************************/

// Header file for class GSLFunctionWrapper
// 
// Created by: moneta  at Sat Nov 13 14:54:41 2004
// 
// Last update: Sat Nov 13 14:54:41 2004
// 
#ifndef ROOT_Math_GSLFunctionWrapper
#define ROOT_Math_GSLFunctionWrapper

#include "gsl/gsl_math.h"

#include "Math/IGenFunction.h"


namespace ROOT {
namespace Math {



  typedef double ( * GSLFuncPointer ) ( double, void *);
  typedef void ( * GSLFdfPointer ) ( double, void *, double *, double *);


  /**
     class to wrap a gsl_function and have a signature like a ROOT::Math::IGenFunction
   */
  class GSLFunctionWrapper { 

  public: 

    GSLFunctionWrapper() : fIGenFunc(0) {}
    ~GSLFunctionWrapper() {if (fIGenFunc) delete fIGenFunc;}

    void SetFuncPointer( GSLFuncPointer f) { fFunc.function = f; } 
    void SetParams ( void * p) { fFunc.params = p; }
    void SetFunction(const IGenFunction &f) { 
 
       if (fIGenFunc) delete fIGenFunc;
 
       fIGenFunc = f.Clone();
       const void * p = fIGenFunc; 
       //const void * p = 0; 
       SetFuncPointer(&GSLFunctionAdapter<IGenFunction >::F);
       SetParams(const_cast<void *>(p));
       //SetFuncPointer(const_cast<void *>(p));
       //SetParams(0);
     }
    
    gsl_function * GetFunc() { return &fFunc; } 

    // evaluate the function 
    double operator() (double x) {  return GSL_FN_EVAL(&fFunc, x); }

  private: 
    gsl_function fFunc; 
    IGenFunction * fIGenFunc;

  };


  /**
     class to wrap a gsl_function_fdf (with derivatives)   
   */
  class GSLFunctionDerivWrapper { 

  public: 

    void SetFuncPointer( GSLFuncPointer f) { fFunc.f = f; } 
    void SetDerivPointer( GSLFuncPointer f) { fFunc.df = f; } 
    void SetFdfPointer( GSLFdfPointer f) { fFunc.fdf = f; } 
    void SetParams ( void * p) { fFunc.params = p; }

    
    gsl_function_fdf * GetFunc() { return &fFunc; } 

    // evaluate the function and derivatives
    double operator() (double x) {  return GSL_FN_FDF_EVAL_F(&fFunc, x); }

    double Gradient (double x) { return GSL_FN_FDF_EVAL_DF(&fFunc, x); } 

    void Fdf(double x, double & f, double & df) { 
      return GSL_FN_FDF_EVAL_F_DF(&fFunc, x, &f, &df);
    }

  private: 
    gsl_function_fdf fFunc; 

  };



} // namespace Math
} // namespace ROOT

#endif /* ROOT_Math_GSLFunctionWrapper */
