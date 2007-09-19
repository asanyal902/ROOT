// @(#)root/unuran:$Id$
// Author: L. Moneta Wed Sep 27 11:53:27 2006

/**********************************************************************
 *                                                                    *
 * Copyright (c) 2006  LCG ROOT Math Team, CERN/PH-SFT                *
 *                                                                    *
 *                                                                    *
 **********************************************************************/

// Header file for class TUnuranDistr

#ifndef ROOT_Math_TUnuranDistr
#define ROOT_Math_TUnuranDistr


#include <cassert> 
#include "TF1.h"

/** 
   TUnuranDistr class 
   wrapper class for one dimensional distribution
*/ 
class TUnuranDistr {

public: 

   /** 
      Default constructor.
      Set by default a value of  min > max for the distribution range, which means 
      that  is undefined. If the variable fHasDomain is not set the range is used only for 
      finding the mode of the distribution
   */ 
   TUnuranDistr() : 
      fFunc(0), 
      fCdf(0),
      fDeriv(0), 
      fXmin(1.), fXmax(-1.), 
      fHasDomain(0)
   {}

   /** 
      Constructor from a TF1 objects
   */ 
   TUnuranDistr (const TF1 * func, const TF1 * cdf = 0, const TF1 * deriv = 0 ) : 
      fFunc(func), 
      fCdf(cdf),
      fDeriv(deriv), 
      fXmin(1.), fXmax(-1.), // if min > max range is undef.
      fHasDomain(0)
   {
      assert(func != 0); 
      // use by default the range specified in TF1
      func->GetRange(fXmin, fXmax);
   } 

   /** 
      Destructor (no operations)
   */ 
   ~TUnuranDistr () {}


   /** 
      Copy constructor
   */ 
   TUnuranDistr(const TUnuranDistr &); 

   /** 
      Assignment operator
   */ 
   TUnuranDistr & operator = (const TUnuranDistr & rhs); 


   /// evaluate the destribution 
   inline double operator() ( double x) const { 
      return fFunc->Eval(x); 
   }

   /// evaluate the derivative of the function
   inline double Derivative( double x) const { 
      if (fDeriv != 0) return fDeriv->Eval(x); 
      // do numerical derivation
      return fFunc->Derivative(x); 
   }

   /// evaluate the integral (cdf)  on the domain
   inline double Cdf(double x) const {   
      if (fCdf != 0) return fCdf->Eval(x);
      TF1 * f =  const_cast<TF1*>(fFunc);
      return f->Integral(fXmin, x); 
   }

   bool GetDomain(double & xmin, double & xmax) const { 
      xmin = fXmin; 
      xmax = fXmax; 
      return fHasDomain; 
   }

   void SetDomain(double xmin, double xmax)  { 
      fXmin = xmin; 
      fXmax = xmax; 
      fHasDomain = true;
   }

   /// get the mode   (x location of function maximum)  
   double Mode() const { 
      return fFunc->GetMaximumX(fXmin, fXmax); 
   }

protected: 


private: 

   const TF1 * fFunc; 
   const TF1 * fCdf; 
   const TF1 * fDeriv; 
   double fXmin; 
   double fXmax; 
   bool fHasDomain;
}; 



#endif /* ROOT_Math_TUnuranDistr */
