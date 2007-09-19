// @(#)root/mathmore:$Id: ParamFunction.cxx,v 1.4 2006/11/23 17:28:57 moneta Exp $
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

// Implementation file for class ParamFunction
// 
// Created by: Lorenzo Moneta  at Wed Nov 10 16:38:34 2004
// 
// Last update: Wed Nov 10 16:38:34 2004
// 

#include "Math/ParamFunction.h"


namespace ROOT {
namespace Math {


ParamFunction::ParamFunction(unsigned int npar, bool providesGrad, bool providesParamGrad) :  
   fNpar(npar), 
   fProvGrad(providesGrad), 
   fProvParGrad(providesParamGrad)
{ 
   //constructor
   fParams = std::vector<double>(npar);
   fParGradient = std::vector<double>(npar);
}
      

ParamFunction::ParamFunction(const ParamFunction & rhs) : 
   BaseFunc(),
   BaseParFunc()
{
   // copy constructor, need to initialize base classes (they are virtual)
   operator=(rhs);
}

ParamFunction & ParamFunction::operator = (const ParamFunction &rhs) 
{
   // implementation of assignment operator
   if (this == &rhs) return *this;  // time saving self-test
   fNpar = rhs.fNpar; 
   fProvGrad = rhs.fProvGrad;  
   fProvParGrad = rhs.fProvParGrad; 
   fParams = rhs.fParams;
   fParGradient = rhs.fParGradient;
   return *this;
}

} // namespace Math
} // namespace ROOT
