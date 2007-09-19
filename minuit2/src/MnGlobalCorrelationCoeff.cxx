// @(#)root/minuit2:$Id: MnGlobalCorrelationCoeff.cxx,v 1.2 2006/04/13 08:39:23 moneta Exp $
// Authors: M. Winkler, F. James, L. Moneta, A. Zsenei   2003-2005  

/**********************************************************************
 *                                                                    *
 * Copyright (c) 2005 LCG ROOT Math team,  CERN/PH-SFT                *
 *                                                                    *
 **********************************************************************/

#include "Minuit2/MnGlobalCorrelationCoeff.h"
#include "Minuit2/MnPrint.h"
#include <cmath>

namespace ROOT {

   namespace Minuit2 {


MnGlobalCorrelationCoeff::MnGlobalCorrelationCoeff(const MnAlgebraicSymMatrix& cov) : fGlobalCC(std::vector<double>()), fValid(true) {
   // constructor: calculate global correlation given a symmetric matrix 
   
   MnAlgebraicSymMatrix inv(cov);
   int ifail = Invert(inv);
   if(ifail != 0) {
#ifdef WARNINGMSG
      std::cout<<"MnGlobalCorrelationCoeff: inversion of matrix fails."<<std::endl;
#endif
      fValid = false;
   } else {
      
      for(unsigned int i = 0; i < cov.Nrow(); i++) {
         double denom = inv(i,i)*cov(i,i);
         if(denom < 1. && denom > 0.) fGlobalCC.push_back(0.);
         else fGlobalCC.push_back(std::sqrt(1. - 1./denom));
      }
   }
}

   }  // namespace Minuit2

}  // namespace ROOT
