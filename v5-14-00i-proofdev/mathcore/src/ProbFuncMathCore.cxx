// @(#)root/mathcore:$Id$
// Authors: L. Moneta, A. Zsenei   06/2005 


#include <cmath>
#include "Math/ProbFuncMathCore.h"
#include "Math/SpecFuncMathCore.h"

#ifndef M_PI
#define M_PI        3.14159265358979323846   /* pi */
#endif


namespace ROOT {
namespace Math {

  


   double breitwigner_cdf_c(double x, double gamma, double x0) {
      
      return 0.5 - std::atan(2.0 * (x-x0) / gamma) / M_PI;
      
   }
   
   
   
   double breitwigner_cdf(double x, double gamma, double x0) {
      
      return 0.5 + std::atan(2.0 * (x-x0) / gamma) / M_PI;
      
   }
   
   
   
   double cauchy_cdf_c(double x, double b, double x0) {
      
      return 0.5 - std::atan( (x-x0) / b) / M_PI;
      
   }
   
   
   
   double cauchy_cdf(double x, double b, double x0) {
      
      return 0.5 + std::atan( (x-x0) / b) / M_PI;
      
   }
   
   /**
   
    double chisquared_cdf_c(double x, double r) {
       
       return gsl_cdf_chisq_Q(x, r);
       
    }
    
    
    
    double chisquared_cdf(double x, double r) {
       
       return gsl_cdf_chisq_P(x, r);
       
    }
    */
   
   
   double exponential_cdf_c(double x, double lambda, double x0) {
      
      if ((x-x0) < 0) {
         
         return 1.0;
         
      } else {
         
         return std::exp(- lambda * (x-x0));
         
      }
      
   }
   
   
   
   double exponential_cdf(double x, double lambda, double x0) {
      
      if ((x-x0) < 0) {
         
         return 0.0;
         
      } else {
         
         return 1.0 - std::exp(- lambda * (x-x0));
         
      }
      
   }
   
   
   /**
   double fdistribution_cdf_c(double x, double n, double m) {
      
      return gsl_cdf_fdist_Q(x, n, m);
      
   }
    
    
    
    double fdistribution_cdf(double x, double n, double m) {
       
       return gsl_cdf_fdist_P(x, n, m);
       
    }
    
    
    
    double gamma_cdf_c(double x, double alpha, double theta) {
       
       return gsl_cdf_gamma_Q(x, alpha, theta);
       
    }
    
    
    
    double gamma_cdf(double x, double alpha, double theta) {
       
       return gsl_cdf_gamma_P(x, alpha, theta);
       
    }
    */
   
   
   
   double gaussian_cdf_c(double x, double sigma, double x0) {
      
      return 0.5*(1.0 - ROOT::Math::erf((x-x0)/(sigma*std::sqrt(2.0))));
      
   }
   
   
   
   double gaussian_cdf(double x, double sigma, double x0) {
      
      return 0.5*(1.0 + ROOT::Math::erf((x-x0)/(sigma*std::sqrt(2.0))));
      
   }
   
   
   
   double lognormal_cdf_c(double x, double m, double s, double x0) {
      
      return 0.5*(1.0 - ROOT::Math::erf((std::log((x-x0))-m)/(s*std::sqrt(2.0))));
      
   }
   
   
   
   double lognormal_cdf(double x, double m, double s, double x0) {
      
      return 0.5*(1.0 + ROOT::Math::erf((std::log((x-x0))-m)/(s*std::sqrt(2.0))));
      
   }
   
   
   
   double normal_cdf_c(double x, double sigma, double x0) {
      
      return 0.5*(1.0 - ROOT::Math::erf((x-x0)/(sigma*std::sqrt(2.0))));
      
   }
   
   
   
   double normal_cdf(double x, double sigma, double x0) {
      
      return 0.5*(1 + ROOT::Math::erf((x-x0)/(sigma*std::sqrt(2.0))));
      
   }
   
   
   /**
   double tdistribution_cdf_c(double x, double r) {
      
      return gsl_cdf_tdist_Q(x, r);
      
   }
    
    
    
    double tdistribution_cdf(double x, double r) {
       
       return gsl_cdf_tdist_P(x, r);
       
    }
    */
   
   
   double uniform_cdf_c(double x, double a, double b, double x0) {
      
      if ((x-x0) < a) {
         return 1.0;
      } else if ((x-x0) >= b) {
         return 0.0;
      } else {
         return (b-(x-x0))/(b-a);
      }
   }
   
   
   
   double uniform_cdf(double x, double a, double b, double x0) {
      
      if ((x-x0) < a) {
         return 0.0;
      } else if ((x-x0) >= b) {
         return 1.0;
      } else {
         return ((x-x0)-a)/(b-a);
      }    
   }
   
   



} // namespace Math
} // namespace ROOT



