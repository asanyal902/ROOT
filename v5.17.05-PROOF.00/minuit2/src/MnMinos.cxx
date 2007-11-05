// @(#)root/minuit2:$Id$
// Authors: M. Winkler, F. James, L. Moneta, A. Zsenei   2003-2005  

/**********************************************************************
 *                                                                    *
 * Copyright (c) 2005 LCG ROOT Math team,  CERN/PH-SFT                *
 *                                                                    *
 **********************************************************************/

#include "Minuit2/MnMinos.h"
#include "Minuit2/FunctionMinimum.h"
#include "Minuit2/FCNBase.h"
#include "Minuit2/MnFunctionCross.h"
#include "Minuit2/MnCross.h"
#include "Minuit2/MinosError.h"

//#define DEBUG

#if defined(DEBUG) || defined(WARNINGMSG)
#include "Minuit2/MnPrint.h" 
#endif


namespace ROOT {

   namespace Minuit2 {


MnMinos::MnMinos(const FCNBase& fcn, const FunctionMinimum& min, unsigned int stra ) : 
   fFCN(fcn), 
   fMinimum(min), 
   fStrategy(MnStrategy(stra)) 
{
   // construct from FCN + Minimum
   // check if Error definition  has been changed, in case re-update errors
   if (fcn.Up() != min.Up() ) { 
#ifdef WARNINGMSG
      MN_INFO_MSG("MnMinos UP value has changed, need to update FunctionMinimum class");
#endif            
   }
} 

MnMinos::MnMinos(const FCNBase& fcn, const FunctionMinimum& min,  const MnStrategy& stra) : 
   fFCN(fcn), 
   fMinimum(min), 
   fStrategy(stra) 
{
   // construct from FCN + Minimum
   // check if Error definition  has been changed, in case re-update errors
   if (fcn.Up() != min.Up() ) { 
#ifdef WARNINGMSG
      MN_INFO_MSG("MnMinos UP value has changed, need to update FunctionMinimum class");
#endif            
   }
} 


std::pair<double,double> MnMinos::operator()(unsigned int par, unsigned int maxcalls) const {
   // do Minos analysis given the parameter index returning a pair for (lower,upper) errors
   MinosError mnerr = Minos(par, maxcalls);
   return mnerr();
}

double MnMinos::Lower(unsigned int par, unsigned int maxcalls) const {
   // get lower error for parameter par
   MnUserParameterState upar = fMinimum.UserState();
   double err = fMinimum.UserState().Error(par);
   
   MnCross aopt = Loval(par, maxcalls);
   
   double lower = aopt.IsValid() ? -1.*err*(1.+ aopt.Value()) : (aopt.AtLimit() ? upar.Parameter(par).LowerLimit() : upar.Value(par));
   
   return lower;
}

double MnMinos::Upper(unsigned int par, unsigned int maxcalls) const {
   // upper error for parameter par
   MnCross aopt = Upval(par, maxcalls);
   
   MnUserParameterState upar = fMinimum.UserState();
   double err = fMinimum.UserState().Error(par);
   
   double upper = aopt.IsValid() ? err*(1.+ aopt.Value()) : (aopt.AtLimit() ? upar.Parameter(par).UpperLimit() : upar.Value(par));
   
   return upper;
}

MinosError MnMinos::Minos(unsigned int par, unsigned int maxcalls) const {
   // do full minos error anlysis (lower + upper) for parameter par 
   assert(fMinimum.IsValid());  
   assert(!fMinimum.UserState().Parameter(par).IsFixed());
   assert(!fMinimum.UserState().Parameter(par).IsConst());
   
   MnCross up = Upval(par, maxcalls);
   MnCross lo = Loval(par, maxcalls);
   
   return MinosError(par, fMinimum.UserState().Value(par), lo, up);
}

MnCross MnMinos::Upval(unsigned int par, unsigned int maxcalls) const {


#ifdef DEBUG
   std::cout << "\n--------- MnMinos --------- \n Determination of positive Minos error for parameter " 
             << par << std::endl;
#endif

   // get crossing value in the upper parameter direction 
   assert(fMinimum.IsValid());  
   assert(!fMinimum.UserState().Parameter(par).IsFixed());
   assert(!fMinimum.UserState().Parameter(par).IsConst());
   if(maxcalls == 0) {
      unsigned int nvar = fMinimum.UserState().VariableParameters();
      maxcalls = 2*(nvar+1)*(200 + 100*nvar + 5*nvar*nvar);
   }
   
   std::vector<unsigned int> para(1, par);
   
   MnUserParameterState upar = fMinimum.UserState();
   double err = upar.Error(par);
   double val = upar.Value(par) + err;
   std::vector<double> xmid(1, val);
   std::vector<double> xdir(1, err);
   
   double up = fFCN.Up();
   unsigned int ind = upar.IntOfExt(par);
   MnAlgebraicSymMatrix m = fMinimum.Error().Matrix();
   //LM:  change to use err**2 (m(i,i) instead of err as in F77 version
   double xunit = sqrt(up/m(ind,ind));
   for(unsigned int i = 0; i < m.Nrow(); i++) {
      if(i == ind) continue;
      double xdev = xunit*m(ind,i);
      unsigned int ext = upar.ExtOfInt(i);

#ifdef DEBUG     
      std::cout << "Parameter " << ext << " is set from " << upar.Value(ext) << " to " <<  upar.Value(ext) + xdev << std::endl;
#endif
      upar.SetValue(ext, upar.Value(ext) + xdev);
   }
   
   upar.Fix(par);
   upar.SetValue(par, val);

#ifdef DEBUG
   std::cout << "Parameter " << par << " is fixed and set from " << fMinimum.UserState().Value(par) << " to " << val << std::endl;
#endif   
   
   //   double edmmax = 0.5*0.1*fFCN.Up()*1.e-3;
   double toler = 0.01; // value used in F77
   MnFunctionCross cross(fFCN, upar, fMinimum.Fval(), fStrategy);
   
   MnCross aopt = cross(para, xmid, xdir, toler, maxcalls);
   
#ifdef DEBUG
   std::cout<<"----- MnMinos: aopt found from MnFunctionCross = "<<aopt.Value()<<std::endl << std::endl;
#endif
   
#ifdef WARNINGMSG
   if(aopt.AtLimit()) 
      MN_INFO_VAL2("MnMinos Parameter is at Upper limit.",par);
   if(aopt.AtMaxFcn())
      MN_INFO_VAL2("MnMinos maximum number of function calls exceeded for Parameter ",par);
   if(aopt.NewMinimum())
      MN_INFO_VAL2("MnMinos new Minimum found while looking for Parameter ",par);
   if(!aopt.IsValid()) 
      MN_INFO_VAL2("MnMinos could not find Upper Value for Parameter ",par);
#endif
   
   return aopt;
}

MnCross MnMinos::Loval(unsigned int par, unsigned int maxcalls) const {
   // return crossing in the lower parameter direction

#ifdef DEBUG
   std::cout << "\n--------- MnMinos --------- \n Determination of negative Minos error for parameter " 
             << par << std::endl;
#endif   

   assert(fMinimum.IsValid());  
   assert(!fMinimum.UserState().Parameter(par).IsFixed());
   assert(!fMinimum.UserState().Parameter(par).IsConst());
   if(maxcalls == 0) {
      unsigned int nvar = fMinimum.UserState().VariableParameters();
      maxcalls = 2*(nvar+1)*(200 + 100*nvar + 5*nvar*nvar);
   }
   std::vector<unsigned int> para(1, par);
   
   MnUserParameterState upar = fMinimum.UserState();
   double err = upar.Error(par);
   double val = upar.Value(par) - err;
   std::vector<double> xmid(1, val);
   std::vector<double> xdir(1, -err);
   
   double up = fFCN.Up();
   unsigned int ind = upar.IntOfExt(par);
   MnAlgebraicSymMatrix m = fMinimum.Error().Matrix();
   double xunit = sqrt(up/m(ind,ind));
   for(unsigned int i = 0; i < m.Nrow(); i++) {
      if(i == ind) continue;
      double xdev = xunit*m(ind,i);
      unsigned int ext = upar.ExtOfInt(i);

#ifdef DEBUG     
      std::cout << "Parameter " << ext << " is set from " << upar.Value(ext) << " to " <<  upar.Value(ext) - xdev << std::endl;
#endif
      upar.SetValue(ext, upar.Value(ext) - xdev);
   }
   
   upar.Fix(par);
   upar.SetValue(par, val);

#ifdef DEBUG
   std::cout << "Parameter " << par << " is fixed and set from " << fMinimum.UserState().Value(par) << " to " << val << std::endl;
#endif   
   
   //   double edmmax = 0.5*0.1*fFCN.Up()*1.e-3;
   double toler = 0.01;
   MnFunctionCross cross(fFCN, upar, fMinimum.Fval(), fStrategy);
   
   MnCross aopt = cross(para, xmid, xdir, toler, maxcalls);
   
#ifdef DEBUG
   std::cout<<"----- MnMinos: aopt found from MnFunctionCross = "<<aopt.Value()<<std::endl << std::endl;
#endif
   
#ifdef WARNINGMSG
   if(aopt.AtLimit()) 
      MN_INFO_VAL2("MnMinos Parameter is at Lower limit.",par);
   if(aopt.AtMaxFcn())
      MN_INFO_VAL2("MnMinos maximum number of function calls exceeded for Parameter ",par);
   if(aopt.NewMinimum())
      MN_INFO_VAL2("MnMinos new Minimum found while looking for Parameter ",par);
   if(!aopt.IsValid()) 
      MN_INFO_VAL2("MnMinos could not find Lower Value for Parameter ",par);
#endif
   
   return aopt;
   
}


   }  // namespace Minuit2

}  // namespace ROOT
