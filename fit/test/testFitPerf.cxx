#include "TH1.h"
#include "TF1.h"
#include "TF2.h"
#include "TMath.h"
#include "TSystem.h"
#include "TRandom3.h"
#include "TTree.h"
#include "TROOT.h"

#include "Fit/DataVector.h"
//#include "Fit/BinPoint.h"
#include "Fit/Fitter.h"
#include "THFitInterface.h"

#include "Math/IParamFunction.h"
#include "Math/WrappedTF1.h"
#include "Math/WrappedMultiTF1.h"
#include "Math/WrappedParamFunction.h"

#ifdef USE_MATHMORE_FUNC
#include "Math/WrappedParamFunction.h"
#endif

#include "Math/Polynomial.h"
#include "Math/DistFunc.h"

#include <string>
#include <iostream>

#include "TStopwatch.h"

#include "TVirtualFitter.h"
#include "TFitterMinuit.h"
// #include "TFitterFumili.h"
// #include "TFumili.h"

#include "GaussFunction.h"

#include "RooDataHist.h"
#include "RooDataSet.h"
#include "RooRealVar.h"
#include "RooGaussian.h"
#include "RooMinuit.h"
#include "RooChi2Var.h"
#include "RooGlobalFunc.h"
#include "RooFitResult.h"
#include "RooProdPdf.h"

#include <cassert>

#include "MinimizerTypes.h"

//#define DEBUG

int nfit;
const int N = 20; 
double iniPar[2*N]; 


void printData(const ROOT::Fit::UnBinData & data) {
   for (unsigned int i = 0; i < data.Size(); ++i) { 
      std::cout << data.Coords(i)[0] << "\t"; 
   }
   std::cout << "\ndata size is " << data.Size() << std::endl;
}    

void FillUnBinData(ROOT::Fit::UnBinData &d, TTree * tree ) { 
   // fill the unbin data set from a TTree

   if (std::string(tree->GetName()) == "t2") { 
      // large tree 
      unsigned int n = tree->GetEntries(); 
      std::cout << "number of unbin data is " << n << " of dim " << N << std::endl;
      d.Initialize(n,N);
      TBranch * bx = tree->GetBranch("x"); 
      double vx[N];
      bx->SetAddress(vx); 
      std::vector<double>  m(N);
      for (int unsigned i = 0; i < n; ++i) {
         bx->GetEntry(i);
         d.Add(vx);
         for (int j = 0; j < N; ++j) 
            m[j] += vx[j];
      }
      std::cout << "average values of means :\n"; 
      for (int j = 0; j < N; ++j) 
         std::cout << m[j]/n << "  ";
      std::cout << "\n";
      
      return; 
   }
   double * x = tree->GetV1(); 
   if (x == 0) { 
      unsigned int n = tree->GetEntries(); 
      //std::cout << "number of unbin data is " << n << std::endl;
      d.Initialize(n,2);
      TBranch * bx = tree->GetBranch("x"); 
      TBranch * by = tree->GetBranch("y"); 
      double v[2];
      bx->SetAddress(&v[0]); 
      by->SetAddress(&v[1]); 
      for (int unsigned i = 0; i < n; ++i) {
         bx->GetEntry(i);
         by->GetEntry(i);
         d.Add(v);
      }

      //printData(d);
   }
   else {
      // use array pre-allocated in tree->Draw . This is faster
      //assert(x != 0); 
      unsigned int n = tree->GetSelectedRows(); 
         
      //std::cout << "number of unbin data is " << n << std::endl;
         
      // if having drawn the tree before 
      d.Initialize(n);
      for (unsigned int i = 0; i < n; ++i) 
         d.Add(x[i]);
   }
      
   //std::copy(x,x+n, d.begin() );

} 




// print the data
template <class T> 
void printData(const T & data) {
   for (typename T::const_iterator itr = data.begin(); itr != data.end(); ++itr) { 
      std::cout << itr->Coords()[0] << "   " << itr->Value() << "   " << itr->Error() << std::endl; 
   }
   std::cout << "\ndata size is " << data.Size() << std::endl;
}    



// fitting using new fitter
typedef ROOT::Math::IParamMultiFunction Func;  
template <class MinType>
int DoFit(TH1 * hist, Func & func, bool debug = false, bool useGrad = false) {  

   ROOT::Fit::BinData d; 
   ROOT::Fit::FillData(d,hist);

   //printData(d);

   // create the fitter 
   //std::cout << "Fit parameter 2  " << f.Parameters()[2] << std::endl;

   ROOT::Fit::Fitter fitter; 
   fitter.Config().SetMinimizer(MinType::name(),MinType::name2());

   if (debug) 
      fitter.Config().MinimizerOptions().SetPrintLevel(3);


   // create the function
   if (!useGrad) { 

      // use simply TF1 wrapper 
      //ROOT::Math::WrappedMultiTF1 f(*func); 
      //ROOT::Math::WrappedTF1 f(*func); 
      fitter.SetFunction(func); 

   } else { // only for gaus fits
      // use function gradient
#ifdef USE_MATHMORE_FUNC
   // use mathmore for polynomial
      ROOT::Math::Polynomial pol(2); 
      assert(pol.NPar() == func->GetNpar());
      pol.SetParameters(func->GetParameters() );
      ROOT::Math::WrappedParamFunction<ROOT::Math::Polynomial> f(pol,1,func->GetParameters(),func->GetParameters()+func->GetNpar() );
#endif
      GaussFunction f; 
      f.SetParameters(func.Parameters());
      fitter.SetFunction(f);
   }


   bool ret = fitter.Fit(d);
   if (!ret) {
      std::cout << " Fit Failed " << std::endl;
      return -1; 
   }
   if (debug) 
      fitter.Result().Print(std::cout);    
   return 0; 
}

// unbin fit
template <class MinType>
int DoFit(TTree * tree, Func & func, bool debug = false, bool = false ) {  

   ROOT::Fit::UnBinData d; 
   // need to have done Tree->Draw() before fit
   FillUnBinData(d,tree);

   //printData(d);

   // create the fitter 
   //std::cout << "Fit parameter 2  " << f.Parameters()[2] << std::endl;

   ROOT::Fit::Fitter fitter; 
   fitter.Config().SetMinimizer(MinType::name(),MinType::name2());

   if (debug) 
      fitter.Config().MinimizerOptions().SetPrintLevel(3);


   // create the function

   fitter.SetFunction(func); 
   // need to fix param 0 , normalization in the unbinned fits
   //fitter.Config().ParSettings(0).Fix();

   bool ret = fitter.Fit(d);
   if (!ret) {
      std::cout << " Fit Failed " << std::endl;
      return -1; 
   }
   if (debug) 
      fitter.Result().Print(std::cout);    
   return 0; 
}

template <class MinType, class FitObj>
int FitUsingNewFitter(FitObj * fitobj, Func & func, bool useGrad=false) { 

   std::cout << "\n************************************************************\n"; 
   std::cout << "\tFit using new Fit::Fitter\n";
   std::cout << "\tMinimizer is " << MinType::name() << "  " << MinType::name2() << std::endl; 

   int iret = 0; 
   TStopwatch w; w.Start(); 

#ifdef DEBUG
   func.SetParameters(iniPar);
   iret |= DoFit<MinType>(fitobj,func,true, useGrad);

#else
   for (int i = 0; i < nfit; ++i) { 
      func.SetParameters(iniPar);
      iret = DoFit<MinType>(fitobj,func, false, useGrad);
      if (iret != 0) {
         std::cout << "Fit failed " << std::endl;
         break; 
      }
   }
#endif
   w.Stop(); 
   std::cout << "\nTime: \t" << w.RealTime() << " , " << w.CpuTime() << std::endl;  
   std::cout << "\n************************************************************\n"; 

   return iret; 
}


//------------old fit methods 

template<class MinType> 
void SetTFitter( MinType  ) { 
   TVirtualFitter::SetDefaultFitter(MinType::name().c_str());
}
// void SetTFitter( FUMILI2  ) { 
//    TVirtualFitter::SetDefaultFitter(MinType::name().c_str()););
// }
// void SetTFitter( TFUMILI  ) { 
//    TVirtualFitter::SetFitter(new TFumili(25));
// }



// fit using Fit method
template <class MinType>
int FitUsingTH1Fit(TH1 * hist, TF1 * func) { 

   std::cout << "\n************************************************************\n"; 
   std::cout << "\tFit using TH1::Fit\n";
   std::cout << "\tMinimizer is " << MinType::name() << std::endl; 

      

   int iret = 0;
   SetTFitter(MinType());

   TStopwatch w; w.Start(); 
   for (int i = 0; i < nfit; ++i) { 
      func->SetParameters(iniPar);
      iret |= hist->Fit(func,"BFQ0");
      if (iret != 0) return iret; 
   }
   // std::cout << "iret " << iret << std::endl;
#ifdef DEBUG
   func->SetParameters(iniPar);
//    if (fitter == "Minuit2") { 
//       // increase print level 
//       TVirtualFitter * tvf = TVirtualFitter::GetFitter(); 
//       TFitterMinuit * minuit2 = dynamic_cast<TFitterMinuit * >(tvf);
//       assert (minuit2 != 0); 
//       minuit2->SetPrintLevel(3);
//    }
   iret |= hist->Fit(func,"BFV0");
   // get precice value of minimum
   int pr = std::cout.precision(18);
   std::cout << "Chi2 value = " << func->GetChisquare() << std::endl; 
   std::cout.precision(pr);

#endif
   w.Stop(); 
   std::cout << "\nTime: \t" << w.RealTime() << " , " << w.CpuTime() << std::endl;  
   std::cout << "\n************************************************************\n"; 

   return iret; 
}


// unbinned fit using Fit of trees
template <class MinType>
int FitUsingTTreeFit(TTree * tree, TF1 * func, const std::string & vars = "x") { 

   std::cout << "\n************************************************************\n"; 
   std::cout << "\tFit using TTree::UnbinnedFit\n";
   std::cout << "\tMinimizer is " << MinType::name() << std::endl; 

      
   std::string sel = "";

   int iret = 0;
   SetTFitter(MinType());

   TStopwatch w; w.Start(); 
   for (int i = 0; i < nfit; ++i) { 
      func->SetParameters(iniPar);
      iret |= tree->UnbinnedFit(func->GetName(),vars.c_str(),sel.c_str(),"Q");
      if (iret != 0) return iret; 
   }
   // std::cout << "iret " << iret << std::endl;
#ifdef DEBUG
   func->SetParameters(iniPar);

   iret |= tree->UnbinnedFit(func->GetName(),vars.c_str(),sel.c_str(),"V");

#endif
   w.Stop(); 
   std::cout << "\nTime: \t" << w.RealTime() << " , " << w.CpuTime() << std::endl;  
   std::cout << "\n************************************************************\n"; 

   return iret; 
}


// int FitUsingTVirtualFitter(TH1 * hist, TF1 * func, const std::string & fitter) { 

//    TVirtualFitter::SetDefaultFitter(fitter.c_str());
//    std::cout << "Fit using TVirtualFitter and " << fitter << "\t Time: \t" << w.RealTime() << " , " << w.CpuTime() < std::endl;  
// }


// ROoFit


//binned roo fit 
int  FitUsingRooFit(TH1 * hist, TF1 * func) { 

   int iret = 0; 
   std::cout << "\n************************************************************\n"; 
   std::cout << "\tFit using RooFit (Chi2 Fit)\n";
   std::cout << "\twith function " << func->GetName() << "\n";


   // start counting t he time
   TStopwatch w; w.Start(); 

   for (int i = 0; i < nfit; ++i) { 

      RooRealVar x("x","x",-5,5) ;
      
      RooDataHist data("bindata","bin dataset with x",x,hist) ;
     // define params
      RooAbsPdf * pdf = 0; 
      RooRealVar * mean = 0; 
      RooRealVar * sigma = 0; 

      func->SetParameters(iniPar);
      std::string fname = func->GetName(); 
      if (fname == "gaussian") { 
         double val,pmin,pmax; 
         val = func->GetParameter(1); //func->GetParLimits(1,-100,100); 
         RooRealVar * mean = new RooRealVar("mean","Mean of Gaussian",val) ;
         val = func->GetParameter(2); func->GetParLimits(1,pmin,pmax); 
         RooRealVar * sigma = new RooRealVar("sigma","Width of Gaussian",val) ;
         
         pdf = new RooGaussian("gauss","gauss(x,mean,sigma)",x,*mean,*sigma) ; 
      }
     
      assert(pdf != 0);
#define USE_CHI2_FIT
#ifdef USE_CHI2_FIT
      RooChi2Var chi2("chi2","chi2",*pdf,data) ;
      RooMinuit m(chi2) ;
      m.setPrintLevel(-1);
      m.fit("mh") ;
#else
      pdf->fitTo(data);
#endif
//      if (iret != 0) return iret; 
      delete pdf; 
      delete mean; delete sigma; 
   }

   w.Stop(); 
   std::cout << "\nTime: \t" << w.RealTime() << " , " << w.CpuTime() << std::endl;  
   std::cout << "\n************************************************************\n"; 
   return iret; 
}

//unbinned roo fit 
int  FitUsingRooFit(TTree * tree, TF1 * func) { 

   int iret = 0; 
   std::cout << "\n************************************************************\n"; 
   std::cout << "\tFit using RooFit (Likelihood Fit)\n";
   std::cout << "\twith function " << func->GetName() << "\n";


   // start counting t he time
   TStopwatch w; w.Start(); 

   for (int i = 0; i < nfit; ++i) { 

      RooRealVar x("x","x",-100,100) ;
      RooRealVar y("y","y",-100,100);

      RooDataSet data("unbindata","unbin dataset with x",tree,RooArgSet(x,y)) ;       


      RooRealVar mean("mean","Mean of Gaussian",iniPar[0], -100,100) ;
      RooRealVar sigma("sigma","Width of Gaussian",iniPar[1], -100, 100) ;
         
      RooGaussian pdfx("gaussx","gauss(x,mean,sigma)",x,mean,sigma);

      // for 2d data
      RooRealVar meany("meanx","Mean of Gaussian",iniPar[2], -100,100) ;
      RooRealVar sigmay("sigmay","Width of Gaussian",iniPar[3], -100, 100) ;         
      RooGaussian pdfy("gaussy","gauss(y,meanx,sigmay)",y,meany,sigmay);

      RooProdPdf pdf("gausxy","gausxy",RooArgSet(pdfx,pdfy) );

     
#ifdef DEBUG
      int level = 3; 
      std::cout << "num entries = " << data.numEntries() << std::endl;
      bool save = true; 
      (pdf.getVariables())->Print("v"); // print the parameters 
#else 
      int level = -1; 
      bool save = false; 
#endif

#ifndef _WIN32 // until a bug 30762 is fixed
      RooFitResult * result = pdf.fitTo(data, RooFit::Minos(0), RooFit::Hesse(1) , RooFit::PrintLevel(level), RooFit::Save(save) );
#else
      RooFitResult * result = pdf.fitTo(data );
#endif

#ifdef DEBUG
      mean.Print(); 
      sigma.Print();
      assert(result != 0); 
      std::cout << " Roofit status " << result->status() << std::endl; 
      result->Print();
#endif


//      if (iret != 0) return iret; 
      assert(iret == 0); 

   }

   w.Stop(); 
   std::cout << "\nTime: \t" << w.RealTime() << " , " << w.CpuTime() << std::endl;  
   std::cout << "\n************************************************************\n"; 
   return iret; 
}

//unbinned roo fit (large tree)
int  FitUsingRooFit2(TTree * tree) { 

   int iret = 0; 
   std::cout << "\n************************************************************\n"; 
   std::cout << "\tFit using RooFit (Likelihood Fit)\n";


   // start counting t he time
   TStopwatch w; w.Start(); 

   for (int i = 0; i < nfit; ++i) { 

      RooArgSet xvars; 
      std::vector<RooRealVar *> x(N);
      std::vector<RooRealVar *> m(N);
      std::vector<RooRealVar *> s(N);

      std::vector<RooGaussian *> g(N);
      std::vector<RooProdPdf *> pdf(N);

      for (int j = 0; j < N; ++j) { 
         std::string xname = "x_" + ROOT::Math::Util::ToString(j);
         x[j] = new RooRealVar(xname.c_str(),xname.c_str(),-10000,10000) ;
         xvars.add( *x[j] );
      } 


      RooDataSet data("unbindata","unbin dataset with x",tree,xvars) ;       

      // create the gaussians
      for (int j = 0; j < N; ++j) { 
         std::string mname = "m_" + ROOT::Math::Util::ToString(j);
         std::string sname = "s_" + ROOT::Math::Util::ToString(j);

         
         m[j] = new RooRealVar(mname.c_str(),mname.c_str(),iniPar[2*j],-100,100) ;  
         s[j] = new RooRealVar(sname.c_str(),sname.c_str(),iniPar[2*j+1],-100,100) ;  

         std::string gname = "g_" + ROOT::Math::Util::ToString(j);
         g[j] = new RooGaussian(gname.c_str(),"gauss(x,mean,sigma)",*x[j],*m[j],*s[j]);

         std::string pname = "prod_" + ROOT::Math::Util::ToString(j);
         if (j == 1) { 
            pdf[1] = new RooProdPdf(pname.c_str(),pname.c_str(),RooArgSet(*g[1],*g[0]) );
         }
         else if (j > 1) { 
            pdf[j] = new RooProdPdf(pname.c_str(),pname.c_str(),RooArgSet(*g[j],*pdf[j-1]) );
         }
      } 

         

     
#ifdef DEBUG
      int level = 3; 
      std::cout << "num entries = " << data.numEntries() << std::endl;
      bool save = true; 
      (pdf[N-1]->getVariables())->Print("v"); // print the parameters 
      std::cout << "\n\nDo the fit now \n\n"; 
#else 
      int level = -1; 
      bool save = false; 
#endif


#ifndef _WIN32 // until a bug 30762 is fixed
      RooFitResult * result = pdf[N-1]->fitTo(data, RooFit::Minos(0), RooFit::Hesse(1) , RooFit::PrintLevel(level), RooFit::Save(save) );
#else 
      RooFitResult * result = pdf[N-1]->fitTo(data);
#endif

#ifdef DEBUG
      assert(result != 0); 
      std::cout << " Roofit status " << result->status() << std::endl; 
      result->Print();
#endif


//      if (iret != 0) return iret; 
      assert(iret == 0); 

      // free
      for (int j = 0; j < N; ++j) { 
         delete x[j]; 
         delete m[j]; 
         delete s[j]; 
         delete g[j];
         delete pdf[j]; 
      }


   }

   w.Stop(); 
   std::cout << "\nTime: \t" << w.RealTime() << " , " << w.CpuTime() << std::endl;  
   std::cout << "\n************************************************************\n"; 
   return iret; 
}


double poly2(const double *x, const double *p) { 
   return p[0] + (p[1]+p[2]*x[0] ) * x[0]; 
}

int testPolyFit() { 

   int iret = 0;


   std::cout << "\n\n************************************************************\n"; 
   std::cout << "\t POLYNOMIAL FIT\n";
   std::cout << "************************************************************\n"; 

   std::string fname("pol2");
   //TF1 * func = (TF1*)gROOT->GetFunction(fname.c_str());
   TF1 * f1 = new TF1("pol2",fname.c_str(),-5,5.);

   f1->SetParameter(0,1);
   f1->SetParameter(1,0.0);
   f1->SetParameter(2,1.0);


   // fill an histogram 
   TH1D * h1 = new TH1D("h1","h1",5,-5.,5.);
//      h1->FillRandom(fname.c_str(),100);
   for (int i = 0; i <100; ++i) 
      h1->Fill( f1->GetRandom() );

   //h1->Print();
   //h1->Draw();
   iniPar[0] = 2.; iniPar[1] = 2.; iniPar[2] = 2.;

   iret |= FitUsingTH1Fit<TMINUIT>(h1,f1);
   iret |= FitUsingTH1Fit<MINUIT2>(h1,f1);
   // dummy for testing
   //iret |= FitUsingNewFitter<DUMMY>(h1,f1);

   // use simply TF1 wrapper 
   //ROOT::Math::WrappedMultiTF1 f2(*f1); 
   ROOT::Math::WrappedParamFunction<> f2(poly2,1,iniPar,iniPar+3); 


   // if Minuit2 is later than TMinuit on Interl is much slower , why ??
   iret |= FitUsingNewFitter<MINUIT2>(h1,f2);
   iret |= FitUsingNewFitter<TMINUIT>(h1,f2);


   return iret;
}

double gaussian(const double *x, const double *p) { 
   //return p[0]*TMath::Gaus(x[0],p[1],p[2]);
   double tmp = (x[0]-p[1])/p[2];
   return p[0] * std::exp(-tmp*tmp/2);
}

double gausnorm(const double *x, const double *p) { 
   //return p[0]*TMath::Gaus(x[0],p[1],p[2]);
   double invsig = 1./p[1]; 
   double tmp = (x[0]-p[0]) * invsig; 
   const double sqrt_2pi = 1./std::sqrt(2.* 3.14159 );
   return std::exp(-0.5 * tmp*tmp ) * sqrt_2pi * invsig; 
}
double gausnorm2D(const double *x, const double *p) { 
   //return p[0]*TMath::Gaus(x[0],p[1],p[2]);
   return gausnorm(x,p)*gausnorm(x+1,p+2);
}
double gausnormN(const double *x, const double *p) { 
   //return p[0]*TMath::Gaus(x[0],p[1],p[2]);
   double f = 1; 
   for (int i = 0; i < N; ++i) 
      f *= gausnorm(x+i,p+2*i);

   return f; 
}

int testGausFit() { 

   int iret = 0; 

   std::cout << "\n\n************************************************************\n"; 
   std::cout << "\t GAUSSIAN FIT\n";
   std::cout << "************************************************************\n"; 



   std::string fname = std::string("gaus");
   //TF1 * func = (TF1*)gROOT->GetFunction(fname.c_str());
//   TF1 * f2 = new TF1("gaus",fname.c_str(),-5,5.);
   TF1 * f1 = new TF1("gaussian",gaussian,-5,5.,3);
   //f2->SetParameters(0,1,1);

   // fill an histogram 
   int nbin = 10000; 
   TH1D * h2 = new TH1D("h2","h2",nbin,-5.,5.);
//      h1->FillRandom(fname.c_str(),100);
   for (int i = 0; i < 10000000; ++i) 
      h2->Fill( gRandom->Gaus(0,10) );

   iniPar[0] = 100.; iniPar[1] = 2.; iniPar[2] = 2.;


   iret |= FitUsingTH1Fit<TMINUIT>(h2,f1);
   //iret |= FitUsingTH1Fit<MINUIT2>(h2,f1);

   // use simply TF1 wrapper 
   //ROOT::Math::WrappedMultiTF1 f2(*f1); 
   ROOT::Math::WrappedParamFunction<> f2(gaussian,1,iniPar,iniPar+3); 


   iret |= FitUsingNewFitter<MINUIT2>(h2,f2);
   iret |= FitUsingNewFitter<TMINUIT>(h2,f2);
//    iret |= FitUsingNewFitter<GSL_PR>(h2,f2);

//#ifdef LATER
   // test using grad function
   std::cout << "\n\nTest Using pre-calculated gradients\n\n";
   bool useGrad=true; 
   iret |= FitUsingNewFitter<MINUIT2>(h2,f2,useGrad);
   iret |= FitUsingNewFitter<TMINUIT>(h2,f2,useGrad);
   iret |= FitUsingNewFitter<GSL_FR>(h2,f2,useGrad);
   iret |= FitUsingNewFitter<GSL_PR>(h2,f2,useGrad);
   iret |= FitUsingNewFitter<GSL_BFGS>(h2,f2,useGrad);
   iret |= FitUsingNewFitter<GSL_BFGS2>(h2,f2,useGrad);


   // test LS algorithm 
   std::cout << "\n\nTest Least Square algorithms\n\n";
   iret |= FitUsingNewFitter<GSL_NLS>(h2,f2);
   iret |= FitUsingNewFitter<FUMILI2>(h2,f2);

   iret |= FitUsingTH1Fit<FUMILI2>(h2,f1);
   iret |= FitUsingTH1Fit<TFUMILI>(h2,f1);
//#endif

   iret |= FitUsingRooFit(h2,f1);

   return iret; 
}

int testTreeFit() { 

   std::cout << "\n\n************************************************************\n"; 
   std::cout << "\t UNBINNED TREE (GAUSSIAN)  FIT\n";
   std::cout << "************************************************************\n"; 


   TTree t1("t1","a simple Tree with simple variables");
   double  x, y; 
   Int_t ev;
   t1.Branch("x",&x,"x/D");
   t1.Branch("y",&y,"y/D");
//          t1.Branch("pz",&pz,"pz/F");
//          t1.Branch("random",&random,"random/D");
   t1.Branch("ev",&ev,"ev/I");
   
   //fill the tree
   for (Int_t i=0;i<10000;i++) {
      gRandom->Rannor(x,y);
      x *= 2; x += 1.;
      y *= 3; y -= 2; 

      ev = i;
      t1.Fill();
      
   }
   //t1.Draw("x"); // to select fit variable 

   //TF1 * f = new TF1("gausnorm", gausnorm, -10,10, 2); 
   TF2 * f1 = new TF2("gausnorm2D", gausnorm2D, -10,10, -10,10, 4); 
   iniPar[0] = 0; 
   iniPar[1] = 1; 
   iniPar[2] = 0; 
   iniPar[3] = 1; 

   // use simply TF1 wrapper 
   //ROOT::Math::WrappedMultiTF1 f2(*f1); 
   ROOT::Math::WrappedParamFunction<> f2(gausnorm2D,2,iniPar,iniPar+4); 

   int iret = 0; 
   iret |= FitUsingNewFitter<MINUIT2>(&t1,f2);

   iret |= FitUsingRooFit(&t1,f1);

   iret |= FitUsingTTreeFit<MINUIT2>(&t1,f1,"x:y");
   iret |= FitUsingTTreeFit<TMINUIT>(&t1,f1,"x:y");
   
   return iret; 

}

int testLargeTreeFit() { 

   std::cout << "\n\n************************************************************\n"; 
   std::cout << "\t UNBINNED TREE (GAUSSIAN MULTI-DIM)  FIT\n";
   std::cout << "************************************************************\n"; 

   TTree t1("t2","a large Tree with simple variables");
   double  x[N];
   Int_t ev;
   t1.Branch("x",x,"x[20]/D");
   t1.Branch("ev",&ev,"ev/I");
   
   //fill the tree
   TRandom3 r; 
   for (Int_t i=0;i<1000;i++) {
      for (int j = 0;  j < N; ++j) { 
         double mu = double(j)/10.; 
         double s  = 1.0 + double(j)/10.;  
         x[j] = r.Gaus(mu,s);
      }

      ev = i;
      t1.Fill();
      
   }
   //t1.Draw("x"); // to select fit variable 


   for (int i = 0; i <N; ++i) {
      iniPar[2*i] = 0; 
      iniPar[2*i+1] = 1; 
   }

   // use simply TF1 wrapper 
   //ROOT::Math::WrappedMultiTF1 f2(*f1); 
   ROOT::Math::WrappedParamFunction<> f2(gausnormN,N,2*N,iniPar); 

   int iret = 0; 
   iret |= FitUsingNewFitter<MINUIT2>(&t1,f2);
   iret |= FitUsingNewFitter<TMINUIT>(&t1,f2);
   iret |= FitUsingNewFitter<GSL_BFGS2>(&t1,f2);

   return iret; 

}
int testLargeTreeRooFit() { 

   int iret = 0; 

   TTree t2("t2b","a large Tree with simple variables");
   double  x[N];
   Int_t ev;
   for (int j = 0; j < N; ++j) { 
      std::string xname = "x_" + ROOT::Math::Util::ToString(j);
      std::string xname2 = "x_" + ROOT::Math::Util::ToString(j) + "/D";
      t2.Branch(xname.c_str(),&x[j],xname2.c_str());
   }
   t2.Branch("ev",&ev,"ev/I");
   //fill the tree
   TRandom3 r; 
   for (Int_t i=0;i<1000;i++) {
      for (int j = 0;  j < N; ++j) { 
         double mu = double(j)/10.; 
         double s  = 1.0 + double(j)/10.;  
         x[j] = r.Gaus(mu,s);
      }

      ev = i;
      t2.Fill();      
   }   


   for (int i = 0; i <N; ++i) {
      iniPar[2*i] = 0; 
      iniPar[2*i+1] = 1; 
   }


   //TF1 * f = new TF1("gausnormN", gausnormN, -100,100, 2*N); 
   
   iret |= FitUsingRooFit2(&t2);

   
   return iret; 

}

int testFitPerf() { 

   int iret = 0; 

 //return iret; 


#ifdef DEBUG
   nfit = 1; 
#else 
   nfit = 1; 
#endif
  iret |= testTreeFit(); 


#ifndef DEBUG
   nfit = 10000; 
#endif
   iret |= testPolyFit(); 

#ifndef DEBUG
   nfit = 10; 
#endif
  iret |= testGausFit(); 


  nfit = 1;
 iret |= testLargeTreeRooFit(); 
 iret |= testLargeTreeFit(); 


   if (iret != 0) 
      std::cerr << "testFitPerf :\t FAILED " << std::endl; 
   else 
      std::cerr << "testFitPerf :\t OK " << std::endl; 
   return iret;
}

int main() { 
   return testFitPerf();
}
   
