// new HFit function 
//______________________________________________________________________________


#include "TH1.h"
#include "TH2.h"
#include "TF1.h"
#include "TF2.h"
#include "TF3.h"
#include "TError.h"
#include "TGraph.h"
#include "TMultiGraph.h"
#include "TGraph2D.h"

#include "Fit/Fitter.h"
#include "Fit/BinData.h"
#include "HFitInterface.h"
#include "Math/MinimizerOptions.h"

#include "Math/WrappedTF1.h"
#include "Math/WrappedMultiTF1.h"

#include "TList.h"
#include "TMath.h"

#include "TClass.h"
#include "TVirtualPad.h" // for gPad

#include "TBackCompFitter.h"

#include <cmath>
#include <memory>

//#define DEBUG

// utility functions used in TH1::Fit

namespace HFit { 

   int GetDimension(const TH1 * h1) { return h1->GetDimension(); }
   int GetDimension(const TGraph * ) { return 1; }
   int GetDimension(const TMultiGraph * ) { return 1; }
   int GetDimension(const TGraph2D * ) { return 2; }

   int CheckFitFunction(const TF1 * f1, int hdim);

   void FitOptionsMake(const char *option, Foption_t &fitOption);

   void CheckGraphFitOptions(const Foption_t &fitOption);


   void GetDrawingRange(TH1 * h1, ROOT::Fit::DataRange & range);
   void GetDrawingRange(TGraph * gr, ROOT::Fit::DataRange & range);
   void GetDrawingRange(TMultiGraph * mg, ROOT::Fit::DataRange & range);
   void GetDrawingRange(TGraph2D * gr, ROOT::Fit::DataRange & range);


   template <class FitObject>
   int Fit(FitObject * h1, TF1 *f1 , Foption_t & option , const ROOT::Math::MinimizerOptions & moption, const char *goption,  ROOT::Fit::DataRange & range); 

   template <class FitObject>
   void StoreAndDrawFitFunction(FitObject * h1, const TF1 * f1, const ROOT::Fit::DataRange & range, bool, bool, const char *goption);

} 

int HFit::CheckFitFunction(const TF1 * f1, int dim) { 
   // Check validity of fitted function
   if (!f1) {
      Error("Fit", "function may not be null pointer");
      return -1;
   }
   if (f1->IsZombie()) {
      Error("Fit", "function is zombie");
      return -2;
   }

   int npar = f1->GetNpar();
   if (npar <= 0) {
      Error("Fit", "function %s has illegal number of parameters = %d", f1->GetName(), npar);
      return -3;
   }

   // Check that function has same dimension as histogram
   if (f1->GetNdim() > dim) {
      Error("Fit","function %s dimension, %d, is greater than fit object dimension, %d",
            f1->GetName(), f1->GetNdim(), dim);
      return -4;
   }
   if (f1->GetNdim() < dim-1) {
      Error("Fit","function %s dimension, %d, is smaller than fit object dimension -1, %d",
            f1->GetName(), f1->GetNdim(), dim);
      return -5;
   }

   return 0; 

}

template<class FitObject>
int HFit::Fit(FitObject * h1, TF1 *f1 , Foption_t & fitOption , const ROOT::Math::MinimizerOptions & minOption, const char *goption, ROOT::Fit::DataRange & range)
{
   // perform fit of histograms, or graphs using new fitting classes 
   // use same routines for fitting both graphs and histograms

#ifdef DEBUG
   printf("fit function %s\n",f1->GetName() ); 
#endif

   // replacement function using  new fitter
   int hdim = HFit::GetDimension(h1); 
   int iret = HFit::CheckFitFunction(f1, hdim);
   if (iret != 0) return iret; 



   // integral option is not supported in this case
   if (f1->GetNdim() < hdim ) { 
      if (fitOption.Integral) Info("Fit","Ignore Integral option. Model function dimension is less than the data object dimension");
      if (fitOption.Like) Info("Fit","Ignore Likelihood option. Model function dimension is less than the data object dimension");
      fitOption.Integral = 0; 
      fitOption.Like = 0; 
   }

   Int_t special = f1->GetNumber();
   Bool_t linear = f1->IsLinear();
   Int_t npar = f1->GetNpar();
   if (special==299+npar)
      linear = kTRUE;
   if (fitOption.Bound || fitOption.Like || fitOption.Errors || fitOption.Gradient || fitOption.More || fitOption.User|| fitOption.Integral || fitOption.Minuit)
      linear = kFALSE;


   // create the fitter
   std::auto_ptr<ROOT::Fit::Fitter> fitter = std::auto_ptr<ROOT::Fit::Fitter> (new ROOT::Fit::Fitter() );
   ROOT::Fit::FitConfig & fitConfig = fitter->Config();

   // create options 
   ROOT::Fit::DataOptions opt; 
   opt.fIntegral = fitOption.Integral; 
   opt.fUseRange = fitOption.Range; 
   if (fitOption.Like) opt.fUseEmpty = true;  // use empty bins in log-likelihood fits 
   if (linear) opt.fCoordErrors = false; // cannot use coordinate errors in a linear fit


   if (opt.fUseRange) { 
#ifdef DEBUG
      printf("use range \n" ); 
#endif
      // check if function has range 
      Double_t fxmin, fymin, fzmin, fxmax, fymax, fzmax;
      f1->GetRange(fxmin, fymin, fzmin, fxmax, fymax, fzmax);
      // support only one range - so add only if was not set before
      if (range.Size(0) == 0) range.AddRange(0,fxmin,fxmax);
      if (range.Size(1) == 0) range.AddRange(1,fymin,fymax);
      if (range.Size(2) == 0) range.AddRange(2,fzmin,fzmax);
   }
#ifdef DEBUG
   printf("range  size %d\n", range.Size(0) ); 
   if (range.Size(0)) {
      double x1; double x2; range.GetRange(0,x1,x2); 
      printf(" range in x = [%f,%f] \n",x1,x2);
   }
#endif

   // fill data  
   //std::auto_ptr<ROOT::Fit::BinData> fitdata = std::auto_ptr<ROOT::Fit::BinData> (new ROOT::Fit::BinData(opt,range) );
   ROOT::Fit::BinData * fitdata = new ROOT::Fit::BinData(opt,range);
   ROOT::Fit::FillData(*fitdata, h1, f1); 

#ifdef DEBUG
   printf("HFit:: data size is %d \n",fitdata->Size());
   for (unsigned int i = 0; i < fitdata->Size(); ++i) { 
      if (fitdata->NDim() == 1) printf(" x[%d] = %f - value = %f \n", i,*(fitdata->Coords(i)),fitdata->Value(i) ); 
   }
#endif   

   // this functions use the TVirtualFitter
   if (special != 0 && !fitOption.Bound && !linear) { 
      if      (special == 100)      ROOT::Fit::InitGaus(*fitdata,f1);  // gaussian
      else if (special == 400)      ROOT::Fit::InitGaus(*fitdata,f1);  // landau (use the same)
   // need to do a linear fit first for expo and poly ? 
//             else if (special == 200)      H1InitExpo();
//             else if (special == 299+npar) H1InitPolynom();
   }


   // set the fit function 
   // if option grad is specified use gradient (works only for 1D) 
   if ( (linear || fitOption.Gradient)  && f1->GetNdim() == 1) 
      fitter->SetFunction(ROOT::Math::WrappedTF1(*f1) );
   else 
      fitter->SetFunction(ROOT::Math::WrappedMultiTF1(*f1) );

   // error normalization in case of zero error in the data
   if (fitdata->GetErrorType() == ROOT::Fit::BinData::kNoError) fitConfig.SetNormErrors(true);

   
   // here need to get some static extra information (like max iterations, error def, etc...)


   // parameter settings and transfer the parameters values, names and limits from the functions
   // is done automatically in the Fitter.cxx 
   for (int i = 0; i < npar; ++i) { 
      ROOT::Fit::ParameterSettings & parSettings = fitConfig.ParSettings(i); 
      double plow,pup; 
      f1->GetParLimits(i,plow,pup);  
      if (plow*pup != 0 && plow >= pup) { // this is a limitation - cannot fix a parameter to zero value
         parSettings.Fix();
      }
      else if (plow < pup ) 
         parSettings.SetLimits(plow,pup);
   }

   // needed for setting precision ? 
   //   - Compute sum of squares of errors in the bin range
   // should maybe use stat[1] ??
 //   Double_t ey, sumw2=0;
//    for (i=hxfirst;i<=hxlast;i++) {
//       ey = GetBinError(i);
//       sumw2 += ey*ey;
//    }



   if (linear) { 
      if (fitOption.Robust && (fitOption.hRobust > 0 && fitOption.hRobust < 1.) ) { 
         fitConfig.SetMinimizer("Linear","Robust");
         fitConfig.MinimizerOptions().SetTolerance(fitOption.hRobust); // use tolerance for passing robust parameter
      }
      else 
         fitConfig.SetMinimizer("Linear","");
   }
   else { 
      // set all minimizer options (tolerance, max iterations, etc..)
      fitConfig.SetMinimizerOptions(minOption); 
      if (fitOption.More) fitConfig.SetMinimizer("Minuit","MigradImproved");
   }

   //override case in case print level is defined also in minOption ??
   if (!fitOption.Verbose) fitConfig.MinimizerOptions().SetPrintLevel(0); 
   else fitConfig.MinimizerOptions().SetPrintLevel(3); 


   // do fitting 

#ifdef DEBUG
   if (fitOption.Like)   printf("do  likelihood fit...\n");
   if (linear)    printf("do linear fit...\n");
   printf("do now  fit...\n");
#endif   
 
   bool fitok = false; 

   if (fitOption.Like)
      fitok = fitter->LikelihoodFit(*fitdata);
   else 
      fitok = fitter->Fit(*fitdata); 


   if ( !fitok  && !fitOption.Quiet )
      Warning("Fit","Abnormal termination of minimization.");
   iret |= !fitok; 
    
   // chech if Minos or more options
   if (fitOption.Errors) { 
      // run Hesse and Minos
      fitConfig.SetParabErrors(true);
      fitConfig.SetMinosErrors(true);
   }
    

   const ROOT::Fit::FitResult & fitResult = fitter->Result(); 
   // one could set directly the fit result in TF1
   iret = fitResult.Status(); 
   if (!fitResult.IsEmpty() ) { 
      // set in f1 the result of the fit      
      f1->SetChisquare(fitResult.Chi2() );
      f1->SetNDF(fitResult.Ndf() );

      f1->SetParameters( &(fitResult.Parameters().front()) ); 
      if ( int( fitResult.Errors().size()) >= f1->GetNpar() ) 
         f1->SetParErrors( &(fitResult.Errors().front()) ); 
  
      if (!fitResult.IsValid() ) iret = 1;
   }
   else {
      iret = -1; 
   }

//   - Store fitted function in histogram functions list and draw
      if (!fitOption.Nostore) {
         HFit::GetDrawingRange(h1, range);
         HFit::StoreAndDrawFitFunction(h1, f1, range, !fitOption.Plus, !fitOption.Nograph, goption); 
      }

      // store result in the backward compatible VirtualFitter
      TVirtualFitter * lastFitter = TVirtualFitter::GetFitter(); 
      if (lastFitter) delete lastFitter; 
      // pass ownership of fitdata to TBackCompFitter (should do also fitter)
      TBackCompFitter * bcfitter = new TBackCompFitter(*fitter,fitdata);
      bcfitter->SetFitOption(fitOption); 
      bcfitter->SetObjectFit(h1); 
      TVirtualFitter::SetFitter( bcfitter ); 

      // print results
//       if (!fitOption.Quiet) fitResult.Print(std::cout);
//       if (fitOption.Verbose) fitResult.PrintCovMatrix(std::cout); 

      // use old-style for printing the results
      if (fitOption.Verbose) bcfitter->PrintResults(2,0.);
      else if (!fitOption.Quiet) bcfitter->PrintResults(1,0.);

      return iret; 
}


void HFit::GetDrawingRange(TH1 * h1, ROOT::Fit::DataRange & range) { 
   // get range from histogram and update the DataRange class  
   // if a ranges already exist in that dimension use that one

   Int_t ndim = GetDimension(h1);

   double xmin = 0, xmax = 0, ymin = 0, ymax = 0, zmin = 0, zmax = 0; 
   if (range.Size(0) == 0) { 
      TAxis  & xaxis = *(h1->GetXaxis()); 
      Int_t hxfirst = xaxis.GetFirst();
      Int_t hxlast  = xaxis.GetLast();
      Double_t binwidx = xaxis.GetBinWidth(hxlast);
      xmin    = xaxis.GetBinLowEdge(hxfirst);
      xmax    = xaxis.GetBinLowEdge(hxlast) +binwidx;
      range.AddRange(xmin,xmax);
   } 

   if (ndim > 1) {
      if (range.Size(1) == 0) { 
         TAxis  & yaxis = *(h1->GetYaxis()); 
         Int_t hyfirst = yaxis.GetFirst();
         Int_t hylast  = yaxis.GetLast();
         Double_t binwidy = yaxis.GetBinWidth(hylast);
         ymin    = yaxis.GetBinLowEdge(hyfirst);
         ymax    = yaxis.GetBinLowEdge(hylast) +binwidy;
         range.AddRange(1,ymin,ymax);
      }
   }      
   if (ndim > 2) {
      if (range.Size(2) == 0) { 
         TAxis  & zaxis = *(h1->GetZaxis()); 
         Int_t hzfirst = zaxis.GetFirst();
         Int_t hzlast  = zaxis.GetLast();
         Double_t binwidz = zaxis.GetBinWidth(hzlast);
         zmin    = zaxis.GetBinLowEdge(hzfirst);
         zmax    = zaxis.GetBinLowEdge(hzlast) +binwidz;
         range.AddRange(2,zmin,zmax);
      }
   }      
#ifdef DEBUG
   std::cout << "xmin,xmax" << xmin << "  " << xmax << std::endl;
#endif

}

void HFit::GetDrawingRange(TGraph * gr,  ROOT::Fit::DataRange & range) { 
   // get range for graph (used sub-set histogram)
   // N.B. : this is different than in previous implementation of TGraph::Fit where range used was from xmin to xmax.
   HFit::GetDrawingRange(gr->GetHistogram(), range);
}
void HFit::GetDrawingRange(TMultiGraph * mg,  ROOT::Fit::DataRange & range) { 
   // get range for multi-graph (used sub-set histogram)
   // N.B. : this is different than in previous implementation of TMultiGraph::Fit where range used was from data xmin to xmax.
   HFit::GetDrawingRange(mg->GetHistogram(), range);
}
void HFit::GetDrawingRange(TGraph2D * gr,  ROOT::Fit::DataRange & range) { 
   // get range for graph2D (used sub-set histogram)
   // N.B. : this is different than in previous implementation of TGraph2D::Fit. There range used was always(0,0)
   HFit::GetDrawingRange(gr->GetHistogram(), range);
}


template<class FitObject>
void HFit::StoreAndDrawFitFunction(FitObject * h1, const TF1 * f1, const ROOT::Fit::DataRange & range, bool delOldFunction, bool drawFunction, const char *goption) { 
//   - Store fitted function in histogram functions list and draw
// should have separate functions for 1,2,3d ? t.b.d in case

#ifdef DEBUG
   std::cout <<"draw and store fit function " << f1->GetName() << std::endl;
#endif
 
   TF1 *fnew1;
   TF2 *fnew2;
   TF3 *fnew3;

   Int_t ndim = GetDimension(h1);
   double xmin = 0, xmax = 0, ymin = 0, ymax = 0, zmin = 0, zmax = 0; 
   range.GetRange(xmin,xmax,ymin,ymax,zmin,zmax); 

#ifdef DEBUG
   std::cout <<"draw and store fit function " << f1->GetName() 
             << " Range in x = [ " << xmin << " , " << xmax << " ]" << std::endl;
#endif

   TList * funcList = h1->GetListOfFunctions();
   if (funcList == 0){
      Error("StoreAndDrawFitFunction","Empty funciton list- cannot store fitted function");
      return;
   } 

   if (delOldFunction) {
      TIter next(funcList, kIterBackward);
      TObject *obj;
      while ((obj = next())) {
         if (obj->InheritsFrom(TF1::Class())) {
            funcList->Remove(obj);
            delete obj;
         }
      }
   }

   // copy TF1 using TClass to avoid slicing in case of derived classes
   if (ndim < 2) {
      fnew1 = (TF1*)f1->IsA()->New();
      f1->Copy(*fnew1);
      funcList->Add(fnew1);
      fnew1->SetParent( h1 );
      fnew1->SetRange(xmin,xmax);
      fnew1->Save(xmin,xmax,0,0,0,0);
      if (!drawFunction) fnew1->SetBit(TF1::kNotDraw);
      fnew1->SetBit(TFormula::kNotGlobal);
   } else if (ndim < 3) {
      fnew2 = (TF2*)f1->IsA()->New();
      f1->Copy(*fnew2);
      funcList->Add(fnew2);
      fnew2->SetRange(xmin,ymin,xmax,ymax);
      fnew2->SetParent( h1 );
      fnew2->Save(xmin,xmax,ymin,ymax,0,0);
      if (!drawFunction) fnew2->SetBit(TF1::kNotDraw);
      fnew2->SetBit(TFormula::kNotGlobal);
   } else {
      // 3D- why f3d is not saved ???
      fnew3 = (TF3*)f1->IsA()->New();
      f1->Copy(*fnew3);
      funcList->Add(fnew3);
      fnew3->SetRange(xmin,ymin,zmin,xmax,ymax,zmax);
      fnew3->SetParent( h1 );
      fnew3->SetBit(TFormula::kNotGlobal);
   }
   if (h1->TestBit(kCanDelete)) return;
   // draw only in case of histograms
   if (drawFunction && ndim < 3 && h1->InheritsFrom(TH1::Class() ) ) h1->Draw(goption);
   if (gPad) gPad->Modified(); // this is not in TH1 code (needed ??)
   
   return; 
}


void HFit::FitOptionsMake(const char *option, Foption_t &fitOption) { 
   //   - Decode list of options into fitOption (used by the TGraph)
   Double_t h=0;
   TString opt = option;
   opt.ToUpper();
   opt.ReplaceAll("ROB", "H");

   //for robust fitting, see if # of good points is defined
   // decode parameters for robust fitting
   if (opt.Contains("H=0.")) {
      int start = opt.Index("H=0.");
      int numpos = start + strlen("H=0.");
      int numlen = 0;
      int len = opt.Length();
      while( (numpos+numlen<len) && isdigit(opt[numpos+numlen]) ) numlen++;
      TString num = opt(numpos,numlen);
      opt.Remove(start+strlen("H"),strlen("=0.")+numlen);
      h = atof(num.Data());
      h*=TMath::Power(10, -numlen);
   }

   if (opt.Contains("U")) fitOption.User    = 1;
   if (opt.Contains("Q")) fitOption.Quiet   = 1;
   if (opt.Contains("V")){fitOption.Verbose = 1; fitOption.Quiet   = 0;}
   if (opt.Contains("L")) fitOption.Like    = 1;
   if (opt.Contains("LL")) fitOption.Like   = 2;
   if (opt.Contains("W")) fitOption.W1      = 1;
   if (opt.Contains("E")) fitOption.Errors  = 1;
   if (opt.Contains("M")) fitOption.More    = 1;
   if (opt.Contains("R")) fitOption.Range   = 1;
   if (opt.Contains("G")) fitOption.Gradient= 1;
   if (opt.Contains("N")) fitOption.Nostore = 1;
   if (opt.Contains("0")) fitOption.Nograph = 1;
   if (opt.Contains("+")) fitOption.Plus    = 1;
   if (opt.Contains("B")) fitOption.Bound   = 1;
   if (opt.Contains("C")) fitOption.Nochisq = 1;
   if (opt.Contains("F")) fitOption.Minuit  = 1;
   if (opt.Contains("H")) { fitOption.Robust  = 1;   fitOption.hRobust = h; } 

}

void HFit::CheckGraphFitOptions(const Foption_t & foption) { 
   if (foption.Like) Info("CheckGraphFitOptions","L (Log Likelihood fit) is an invalid option when fitting a graph. It is ignored");
   if (foption.Integral) Info("CheckGraphFitOptions","I (use function integral) is an invalid option when fitting a graph. It is ignored");
   return;
} 

// implementations of ROOT::Fit::FitObject functions (defined in HFitInterface) in terms of the template HFit::Fit

int ROOT::Fit::FitObject(TH1 * h1, TF1 *f1 , Foption_t & foption , const ROOT::Math::MinimizerOptions & moption, const char *goption, ROOT::Fit::DataRange & range) { 
   // histogram fitting
   return HFit::Fit(h1,f1,foption,moption,goption,range); 
}

int ROOT::Fit::FitObject(TGraph * gr, TF1 *f1 , Foption_t & foption , const ROOT::Math::MinimizerOptions & moption, const char *goption, ROOT::Fit::DataRange & range) { 
   // TGraph fitting
   return HFit::Fit(gr,f1,foption,moption,goption,range); 
}

int ROOT::Fit::FitObject(TMultiGraph * gr, TF1 *f1 , Foption_t & foption , const ROOT::Math::MinimizerOptions & moption, const char *goption, ROOT::Fit::DataRange & range) { 
   // TMultiGraph fitting
   return HFit::Fit(gr,f1,foption,moption,goption,range); 
}

int ROOT::Fit::FitObject(TGraph2D * gr, TF1 *f1 , Foption_t & foption , const ROOT::Math::MinimizerOptions & moption, const char *goption, ROOT::Fit::DataRange & range) { 
   // TGraph2D fitting
   return HFit::Fit(gr,f1,foption,moption,goption,range); 
}


// implementations of DoFit member functions in data objects (TH1, TGrah, etc...)

Int_t TH1::DoFit(TF1 *f1 ,Option_t *option ,Option_t *goption, Double_t xxmin, Double_t xxmax) { 
//   - Decode list of options into fitOption
   Foption_t fitOption;

   if (!FitOptionsMake(option,fitOption)) return 0;
   // create range and minimizer options with default values 
   ROOT::Fit::DataRange range(xxmin,xxmax); 
   ROOT::Math::MinimizerOptions minOption; 
   
   return ROOT::Fit::FitObject(this, f1 , fitOption , minOption, goption, range); 
}

Int_t TGraph::DoFit(TF1 *f1 ,Option_t *option ,Option_t *goption, Axis_t rxmin, Axis_t rxmax) { 
   // internal graph fitting methods
   Foption_t fitOption;
   HFit::FitOptionsMake(option,fitOption);
   // exclude options not valid for graphs
   HFit::CheckGraphFitOptions(fitOption);
   // create range and minimizer options with default values 
   ROOT::Fit::DataRange range(rxmin,rxmax); 
   ROOT::Math::MinimizerOptions minOption; 
   return ROOT::Fit::FitObject(this, f1 , fitOption , minOption, goption, range); 
}

Int_t TMultiGraph::DoFit(TF1 *f1 ,Option_t *option ,Option_t *goption, Axis_t rxmin, Axis_t rxmax) { 
   // internal multigraph fitting methods
   Foption_t fitOption;
   HFit::FitOptionsMake(option,fitOption);
   HFit::CheckGraphFitOptions(fitOption);

   // create range and minimizer options with default values 
   ROOT::Fit::DataRange range(rxmin,rxmax); 
   ROOT::Math::MinimizerOptions minOption; 
   return ROOT::Fit::FitObject(this, f1 , fitOption , minOption, goption, range); 
}


Int_t TGraph2D::DoFit(TF2 *f2 ,Option_t *option ,Option_t *goption) { 
   // internal graph2D fitting methods
   Foption_t fitOption;
   HFit::FitOptionsMake(option,fitOption);
   HFit::CheckGraphFitOptions(fitOption);

   // create range and minimizer options with default values 
   ROOT::Fit::DataRange range(2); 
   ROOT::Math::MinimizerOptions minOption; 
   return ROOT::Fit::FitObject(this, f2 , fitOption , minOption, goption, range); 
}

