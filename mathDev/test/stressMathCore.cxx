// @(#)root/test:$Id: stressVector.cxx 19826 2007-09-19 19:56:11Z rdm $
// Author: Lorenzo Moneta   06/2005 
///////////////////////////////////////////////////////////////////////////////////
//
//  MathCore Benchmark test suite
//  ==============================
//
//  This program performs tests : 
//     - mathematical functions in particular the statistical functions by estimating 
//         pdf, cdf and quantiles. 
//     - cdf are estimated directly and compared with integral calulated ones 

#include "Math/DistFunc.h"
#include "Math/IParamFunction.h"
#include "Math/Integrator.h"
#include <iostream>
#include <limits>
#include "TBenchmark.h"
#include "TROOT.h"

using namespace ROOT::Math; 

int compare( std::string name, double v1, double v2, double scale = 2.0) {
  //  ntest = ntest + 1; 

   //std::cout << std::setw(50) << std::left << name << ":\t";   
   
  // numerical double limit for epsilon 
   double eps = scale* std::numeric_limits<double>::epsilon();
   int iret = 0; 
   double delta = v2 - v1;
   double d = 0;
   if (delta < 0 ) delta = - delta; 
   if (v1 == 0 || v2 == 0) { 
      if  (delta > eps ) { 
         iret = 1; 
      }
   }
   // skip case v1 or v2 is infinity
   else { 
      d = v1; 

      if ( v1 < 0) d = -d; 
      // add also case when delta is small by default
      if ( delta/d  > eps && delta > eps ) 
         iret =  1; 
   }

   if (iret) { 
      int pr = std::cout.precision (18);
      std::cout << "\nDiscrepancy in " << name.c_str() << "() :\n  " << v1 << " != " << v2 << " discr = " << int(delta/d/eps) 
                << "   (Allowed discrepancy is " << eps  << ")\n\n";
      std::cout.precision (pr);
      //nfail = nfail + 1;
   }
   //else  
      //  std::cout <<".";
   
   return iret; 
}

// trait class  for distinguishing the number of parameters for the various functions
template<class Func, unsigned int NPAR>
struct Evaluator { 
   static double F(Func f,  double x, const double * ) { 
      return f(x);
   }
};
template<class Func>
struct Evaluator<Func, 1> { 
   static double F(Func f,  double x, const double * p) { 
      return f(x,p[0]);
   }
};
template<class Func>
struct Evaluator<Func, 2> { 
   static double F(Func f,  double x, const double * p) { 
      return f(x,p[0],p[1]);
   }
};
template<class Func>
struct Evaluator<Func, 3> { 
   static double F(Func f,  double x, const double * p) { 
      return f(x,p[0],p[1],p[2]);
   }
};


// statistical function class 
// template on the number of parameters
template<class Func, class FuncQ, int NPAR, int NPARQ=NPAR-1> 
class StatFunction : public ROOT::Math::IParamFunction { 

public: 

   StatFunction(Func pdf, Func cdf, FuncQ quant) : fPdf(pdf), fCdf(cdf), fQuant(quant) 
   {
      fScale1 = 1.0E6; //scale for cdf test (integral)
      fScale2 = 10;  //scale for quantile test
      for(int i = 0; i< NPAR; ++i) fParams[i]=0;
   } 


   unsigned int NPar() const { return NPAR; } 
   const double * Parameters() const { return fParams; }
   ROOT::Math::IGenFunction * Clone() const { return new StatFunction(fPdf,fCdf,fQuant); }
   void SetParameters(const double * p) { std::copy(p,p+NPAR,fParams); }
   void SetParameters(double p0) { *fParams = p0; }
   void SetParameters(double p0, double p1) { *fParams = p0; *(fParams+1) = p1; }
   void SetParameters(double p0, double p1, double p2) { *fParams = p0; *(fParams+1) = p1; *(fParams+2) = p2; }

   double Cdf(double x) const { 
      return Evaluator<Func,NPAR>::F(fCdf,x, fParams); 
   }
   double Quantile(double x) const { 
      return Evaluator<FuncQ,NPARQ>::F(fQuant,x, fParams); 
   }

   // test cumulative function
   int Test(double x1, double x2, double xl = 1, double xu = 0, bool cumul = false); 
   
   void ScaleTol1(double s) { fScale1 *= s; }  
   void ScaleTol2(double s) { fScale2 *= s; }

private: 


   double DoEval(double x) const { 
      return Evaluator<Func,NPAR>::F(fPdf,x, fParams); 
   }

   Func fPdf; 
   Func fCdf;
   FuncQ fQuant; 
   double fParams[NPAR];
   double fScale1;
   double fScale2;
};

void PrintStatus(int iret) { 
   if (iret == 0) 
      std::cout <<"\t\t\t\t OK" << std::endl;
   else  
      std::cout <<"\t\t\t\t FAILED " << std::endl;
}

// test cdf at value f 
template<class F1, class F2, int N1, int N2> 
int StatFunction<F1,F2,N1,N2>::Test(double xmin, double xmax, double xlow, double xup, bool c) {

   int iret = 0; 
   const int NFuncTest = 1000; 

   // scan all values from xmin to xmax
   double dx = (xmax-xmin)/NFuncTest; 
   for (int i = 0; i < NFuncTest; ++i) { 
      double v1 = xmin + dx*i;  // value used  for testing
      double q1 = Cdf(v1);
      //std::cout << "v1 " << v1 << " pdf " << (*this)(v1) << " cdf " << q1 << " quantile " << Quantile(q1) << std::endl;  
      // calculate integral of pdf
      Integrator ig(IntegrationOneDim::ADAPTIVESINGULAR, 1.E-12,1.E-12,100000);
      ig.SetFunction(*this);
      double q2 = 0; 
      if (!c) { 
         // lower intergal (cdf) 
         if (xlow >= xup || xlow > xmin) 
            q2 = ig.IntegralLow(v1); 
         else 
            q2 = ig.Integral(xlow,v1); 

         // use a larger scale (integral error is 10-9)
         iret |= compare("test _cdf", q1, q2, fScale1 );
         // test the quantile 
         double v2 = Quantile(q1); 
         iret |= compare("test _quantile", v1, v2, fScale2 );
      }
      else { 
         // upper integral (cdf_c)
         if (xlow >= xup || xup < xmax) 
            q2 = ig.IntegralUp(v1); 
         else 
            q2 = ig.Integral(v1,xup);
 
         iret |= compare("test _cdf_c", q1, q2, fScale1);
         double v2 = Quantile(q1); 
         iret |= compare("test _quantile_c", v1, v2, fScale2 );
      }
      if (iret)  { 
         std::cout << "Failed test for x = " << v1 << " p = "; 
         for (int j = 0; j < N1; ++j) std::cout << fParams[j] << "\t"; 
         std::cout << std::endl;
         break;
      } 
   }
   if (c || iret != 0) PrintStatus(iret);
   return iret; 
}

// typedef defining the functions
typedef double ( * F0) ( double); 
typedef double ( * F1) ( double, double); 
typedef double ( * F2) ( double, double, double); 
typedef double ( * F3) ( double, double, double, double); 

typedef StatFunction<F2,F2,2,2> Dist_beta; 
typedef StatFunction<F2,F1,2> Dist_breitwigner; 
typedef StatFunction<F2,F1,2> Dist_chisquared; 
typedef StatFunction<F3,F2,3> Dist_fdistribution; 
typedef StatFunction<F3,F2,3> Dist_gamma; 
typedef StatFunction<F2,F1,2> Dist_gaussian; 
typedef StatFunction<F3,F2,3> Dist_lognormal; 
typedef StatFunction<F2,F1,2> Dist_tdistribution; 
 

#define CREATE_DIST(name) Dist_ ##name  dist( name ## _pdf, name ## _cdf, name ##_quantile );
#define CREATE_DIST_C(name) Dist_ ##name  distc( name ## _pdf, name ## _cdf_c, name ##_quantile_c );


template<class Distribution> 
int TestDist(Distribution & d, double x1, double x2) { 
   int ir = 0; 
   ir |= d.Test(x1,x2); 
   return ir; 
}

//int main(int argc,const char *argv[]) { 
int main() { 

   TBenchmark bm; 
   bm.Start("stressMathCore");
   
   int iret = 0; 
   // test statistical function 
   { 
      std::cout << "Beta distribution      \t\t"; 
      CREATE_DIST(beta);
      dist.SetParameters( 2, 2);
      iret |= dist.Test(0.01,0.99,0.,1.);
      CREATE_DIST_C(beta);
      distc.SetParameters( 2, 2);
      iret |= distc.Test(0.01,0.99,0.,1.,true);
   }

   {
      std::cout << "Gamma distribution     \t\t"; 
      CREATE_DIST(gamma);
      dist.SetParameters( 2, 1);
      iret |= dist.Test(0.05,5, 0.,1.);
      CREATE_DIST_C(gamma);
      distc.SetParameters( 2, 1);
      iret |= distc.Test(0.05,5, 0.,1.,true);
   }

   {
      std::cout << "Chisquare distribution \t\t"; 
      CREATE_DIST(chisquared);
      dist.SetParameters( 10, 0);
      dist.ScaleTol2(10);
      iret |= dist.Test(0.05,30, 0.);
      CREATE_DIST_C(chisquared);
      distc.SetParameters( 10, 0);
      distc.ScaleTol2(10000000);  // t.b.c.
      iret |= distc.Test(0.05,30, 0.,1.,true);
   }
   {
      std::cout << "Normal distribution  \t\t"; 
      CREATE_DIST(gaussian);
      dist.SetParameters( 1, 0);
      dist.ScaleTol2(100);
      iret |= dist.Test(-4,4);
      CREATE_DIST_C(gaussian);
      distc.SetParameters( 1, 0);
      distc.ScaleTol2(100);
      iret |= distc.Test(-4,4,1,0,true);
   }
   {
      std::cout << "BreitWigner distribution \t"; 
      CREATE_DIST(breitwigner);
      dist.SetParameters( 1);
      dist.ScaleTol2(10);
      iret |= dist.Test(-5,5);
      CREATE_DIST_C(breitwigner);
      distc.SetParameters( 1);
      distc.ScaleTol2(10);
      iret |= distc.Test(-5,5,1,0,true);
   }
   {
      std::cout << "F    distribution  \t\t"; 
      CREATE_DIST(fdistribution);
      dist.SetParameters( 5, 4);
      dist.ScaleTol1(1000000);
      dist.ScaleTol2(10);
      // if enlarge scale test fails
      iret |= dist.Test(0.05,5,0);
      CREATE_DIST_C(fdistribution);
      distc.SetParameters( 5, 4);
//       dist.ScaleTol1(1000000);
      distc.ScaleTol2(10);
      // if enlarge scale test fails
      iret |= distc.Test(0.05,5,0,1,true);
   }
   {
      std::cout << "t    distribution  \t\t"; 
      CREATE_DIST(tdistribution);
      dist.SetParameters( 10 );
//       dist.ScaleTol1(1000);
       dist.ScaleTol2(5000);
      iret |= dist.Test(-10,10);
      CREATE_DIST_C(tdistribution);
      distc.SetParameters( 10 );
      distc.ScaleTol2(10000);  // t.b.c.
      iret |= distc.Test(-10,10,1,0,true);
   }
   {
      std::cout << "lognormal distribution  \t"; 
      CREATE_DIST(lognormal);
      dist.SetParameters(1,1 );
      dist.ScaleTol1(1000);
      iret |= dist.Test(0.01,5,0);
      CREATE_DIST_C(lognormal);
      distc.SetParameters(1,1 );
      distc.ScaleTol2(1000000); // t.b.c.
      iret |= distc.Test(0.01,5,0,1,true);
   }

   bm.Stop("stressMathCore");
   std::cout <<"******************************************************************************\n";
   bm.Print("stressMathCore");
   const double reftime = 3.67; // ref time on  macbook pro (intel core duo 2.2 GHz)
   double rootmarks = 800 * reftime / bm.GetCpuTime("stressMathCore");
   std::cout << " ROOTMARKS = " << rootmarks << " ROOT version: " << gROOT->GetVersion() << "\t" 
             << gROOT->GetSvnBranch() << "@" << gROOT->GetSvnRevision() << std::endl;
   std::cout <<"*******************************************************************************\n";

   return iret; 
}
