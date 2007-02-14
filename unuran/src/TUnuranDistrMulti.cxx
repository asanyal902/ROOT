// @(#)root/unuran:$Name:  $:$Id: TUnuranDistrMulti.cxx,v 1.1 2006/11/15 17:40:36 brun Exp $
// Author: L. Moneta Wed Sep 27 17:07:37 2006

/**********************************************************************
 *                                                                    *
 * Copyright (c) 2006  LCG ROOT Math Team, CERN/PH-SFT                *
 *                                                                    *
 *                                                                    *
 **********************************************************************/

// Implementation file for class TUnuranDistrMulti

#include "TUnuranDistrMulti.h"

#include "TF1.h"

#include <limits>



   /** 

   */ 
TUnuranDistrMulti::TUnuranDistrMulti (TF1 * func) : 
   fFunc(func), 
   fHasDomain(0)
{
   //Constructor from a TF1 objects
   assert(func != 0); 
   fDim = func->GetNdim();
} 


TUnuranDistrMulti::~TUnuranDistrMulti() 
{
   // Destructor implementation.
}

TUnuranDistrMulti::TUnuranDistrMulti(const TUnuranDistrMulti & rhs) 
{
   // Implementation of copy ctor using aassignment operator
   operator=(rhs);
}

TUnuranDistrMulti & TUnuranDistrMulti::operator = (const TUnuranDistrMulti &rhs) 
{
   // Implementation of assignment operator (copy only the funciton pointer not the function itself)
   if (this == &rhs) return *this;  // time saving self-test
   fFunc = rhs.fFunc;
   fDim = rhs.fDim;
   fXmin = rhs.fXmin;
   fXmax = rhs.fXmax;
   fHasDomain = rhs.fHasDomain;
   return *this;
}



double TUnuranDistrMulti::operator() ( const double * x) const {  
   // evaluate the destribution 
   return fFunc->EvalPar(x); 
}


void TUnuranDistrMulti::Gradient( const double * x, double * grad) const { 
      // do numerical derivation of gradient
   std::vector<double> g(fDim); 
   for (unsigned int i = 0; i < fDim; ++i) 
      g[i] = Derivative(x,i); 
      
   grad = &g.front();
   return;
}

double TUnuranDistrMulti::Derivative( const double * x, int coord) const { 
    // do numerical derivation of gradient using 5 point rule
   // use 5 point rule 

   //double eps = 0.001; 
   //const double kC1 = 8*std::numeric_limits<double>::epsilon();

   double h = 0.001; 

   std::vector<double> xx(fDim);
   double * params = fFunc->GetParameters();
   fFunc->InitArgs(&xx.front(), params);
 
   xx[coord] = x[coord]+h;     double f1 = fFunc->EvalPar(&xx.front(),params);
   //xx[coord] = x[coord];       double fx = fFunc->EvalPar(&xx.front(),params);
   xx[coord] = x[coord]-h;     double f2 = fFunc->EvalPar(&xx.front(),params);

   xx[coord] = x[coord]+h/2;   double g1 = fFunc->EvalPar(&xx.front(),params);
   xx[coord] = x[coord]-h/2;   double g2 = fFunc->EvalPar(&xx.front(),params);

   //compute the central differences
   double h2    = 1/(2.*h);
   double d0    = f1 - f2;
   double d2    = 2*(g1 - g2);
   //double error  = kC1*h2*fx;  //compute the error
   double deriv = h2*(4*d2 - d0)/3.;  
   return deriv;
}



