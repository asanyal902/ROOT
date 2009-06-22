// @(#)root/roostats:$Id: MCMCInterval.cxx 26805 2009-06-17 14:31:02Z kbelasco $
// Author: Kevin Belasco        17/06/2009
/*************************************************************************
 * Copyright (C) 1995-2008, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

//_________________________________________________
/*
BEGIN_HTML
<p>
MCMCInterval is a concrete implementation of the RooStats::ConfInterval interface.  
It takes as input Markov Chain of data points in the parameter space generated by Monte Carlo
using the Metropolis algorithm.  The points in the Markov Chain are put into a histogram and the
interval is then calculated by adding the heights of the bins in decreasing order until the 
desired level of confidence has been reached.  Note that this means the actual confidence level
is >= the confidence level prescribed by the client (unless the user calls SetStrict(false)).
The level to use to make a contour plot is the cutoff histogram bin height.  Because a histogram is
used, MCMCInterval currently supports up to 3 dimensions for the interval (i.e. 3 parameters of
interest).
</p>

<p>
This is not the only way for the confidence interval to be determined, and other possibilities are
being considered being added, especially for the 1-dimensional case.
</p>

<p>
It one can ask an MCMCInterval for the lower and upper limits on a specific parameter of interest
in the interval.  Note that this works better for some distributions (ones with exactly one
maximum) than others, and sometimes has little value.
</p>
END_HTML
*/
//_________________________________________________

#ifndef ROOT_Rtypes
#include "Rtypes.h"
#endif

#include "RooStats/MCMCInterval.h"
#include "RooRealVar.h"
#include "RooArgList.h"
#include "RooDataSet.h"
#include <cstdlib>
#include <string>
#include <algorithm>
#include "TIterator.h"
#include "TH1.h"
#include "TFile.h"
#include "RooMsgService.h"

ClassImp(RooStats::MCMCInterval);

using namespace RooFit;
using namespace RooStats;
using namespace std;

MCMCInterval::MCMCInterval() : ConfInterval()
{
   fPreferredNumBins = DEFAULT_NUM_BINS;
   fConfidenceLevel = 0.0;
   fData = NULL;
   fAxes = NULL;
   fHist = NULL;
   fNumBins = NULL;
   fCutoff = 0;
   fDimension = 1;
   fIsStrict = true;
   fParameters = NULL;
}

MCMCInterval::MCMCInterval(const char* name) : ConfInterval(name, name)
{
   fPreferredNumBins = DEFAULT_NUM_BINS;
   fConfidenceLevel = 0.0;
   fData = NULL;
   fAxes = NULL;
   fHist = NULL;
   fNumBins = NULL;
   fCutoff = 0;
   fDimension = 1;
   fIsStrict = true;
   fParameters = NULL;
}

MCMCInterval::MCMCInterval(const char* name, const char* title)
   : ConfInterval(name, title)
{
   fPreferredNumBins = DEFAULT_NUM_BINS;
   fConfidenceLevel = 0.0;
   fData = NULL;
   fAxes = NULL;
   fHist = NULL;
   fNumBins = NULL;
   fCutoff = 0;
   fDimension = 1;
   fIsStrict = true;
   fParameters = NULL;
}

MCMCInterval::MCMCInterval(const char* name, const char* title,
        RooArgSet& parameters, RooDataSet& chain) : ConfInterval(name, title)
{
   fPreferredNumBins = DEFAULT_NUM_BINS;
   fNumBins = NULL;
   fConfidenceLevel = 0.0;
   fAxes = NULL;
   fData = &chain;
   fHist = NULL;
   fCutoff = 0;
   fIsStrict = true;
   SetParameters(parameters);
}

struct CompareBins { 

   CompareBins( TH1 * hist) : fHist(hist) {}
   bool operator() ( Int_t bin1 , Int_t bin2 ) { 
      // bins must have content >= 0, so this is safe:
      Double_t n1 = fHist->GetBinContent(bin1);
      Double_t n2 = fHist->GetBinContent(bin2);
      
      return    (n1 < n2) ;
   }
   TH1 * fHist; 
};


Bool_t MCMCInterval::IsInInterval(RooArgSet& point) 
{
   Int_t bin;
   if (fDimension == 1) {
      bin = fHist->FindBin(point.getRealValue(fAxes[0]->GetName()));
   } else if (fDimension == 2) {
      bin = fHist->FindBin(point.getRealValue(fAxes[0]->GetName()),
                           point.getRealValue(fAxes[1]->GetName()));
   } else if (fDimension == 3) {
      bin = fHist->FindBin(point.getRealValue(fAxes[0]->GetName()),
                           point.getRealValue(fAxes[1]->GetName()),
                           point.getRealValue(fAxes[2]->GetName()));
   } else {
      coutE(Eval) << "* Error in MCMCInterval::IsInInterval: " <<
                     "Couldn't handle dimension: " << fDimension << endl;
      return false;
   }

   return fHist->GetBinContent(bin) >= (Double_t)fCutoff;
}

void MCMCInterval::SetConfidenceLevel(Double_t cl)
{
   fConfidenceLevel = cl;
   DetermineInterval();
}

void MCMCInterval::SetNumBins(Int_t numBins)
{
   if (!fNumBins) {
      coutE(Eval) << "* Error in MCMCInterval::SetNumBins: " <<
                     "fNumBins not initialized.  Returning immediately" << endl;
      return;
   }
   if (numBins > 0) {
      fPreferredNumBins = numBins;
      for (Int_t d = 0; d < fDimension; d++)
         fNumBins[d] = numBins;
   }
   else {
      coutE(Eval) << "* Error in MCMCInterval::SetNumBins: " <<
                     "Negative number of bins given: " << numBins << endl;
      return;
   }

   // If the histogram already exists, recreate it with the new bin numbers
   if (fHist != NULL)
      CreateHistogram();
}

void MCMCInterval::SetAxes(RooArgList& axes)
{
   Int_t size = axes.getSize();
   if (size != fDimension) {
      coutE(Eval) << "* Error in MCMCInterval::SetAxes: " <<
                     "list size (" << size << ") and " << "dimension ("
                     << fDimension << ") don't match" << endl;
      return;
   }
   for (Int_t i = 0; i < size; i++)
      fAxes[i] = (RooRealVar*)axes.at(i);
}

void MCMCInterval::CreateHistogram()
{
   if (fNumBins == NULL || fAxes == NULL || fData == NULL) {
      coutE(Eval) << "* Error in MCMCInterval::CreateHistogram: " <<
                     "Crucial data member was NULL." << endl;
      coutE(Eval) << "Make sure to fully construct/initialize." << endl;
      return;
   }

   if (fDimension == 1)
      fHist = fData->createHistogram("hist", *fAxes[0],
              Binning(fNumBins[0]), RooFit::Scaling(kFALSE));

   else if (fDimension == 2)
      fHist = fData->createHistogram("hist", *fAxes[0],
            Binning(fNumBins[0]), YVar(*fAxes[1], Binning(fNumBins[1])),
            RooFit::Scaling(kFALSE));

   else if (fDimension == 3) 
      fHist = fData->createHistogram("hist", *fAxes[0],
              Binning(fNumBins[0]), YVar(*fAxes[1], Binning(fNumBins[1])),
              ZVar(*fAxes[2], Binning(fNumBins[2])), RooFit::Scaling(kFALSE));

   else
      coutE(Eval) << "* Error in MCMCInterval::DetermineInterval: " <<
                     "Couldn't handle dimension: " << fDimension << endl;

}

void MCMCInterval::SetParameters(RooArgSet& parameters)
{
   fParameters = &parameters;
   fDimension = fParameters->getSize();
   if (fAxes != NULL)
      delete[] fAxes;
   fAxes = new RooRealVar*[fDimension];
   if (fNumBins != NULL)
      delete[] fNumBins;
   fNumBins = new Int_t[fDimension];
   TIterator* it = fParameters->createIterator();
   Int_t n = 0;
   TObject* obj;
   while ((obj = it->Next()) != NULL) {
      if (dynamic_cast<RooRealVar*>(obj) != NULL)
         fAxes[n] = (RooRealVar*)obj;
      else
         coutE(Eval) << "* Error in MCMCInterval::SetParameters: " <<
                     obj->GetName() << " not a RooRealVar*" << std::endl;
      fNumBins[n] = fPreferredNumBins;
      n++;
   }
}

void MCMCInterval::DetermineInterval()
{
   Int_t numBins;
   if (fHist == NULL)
      CreateHistogram();

   if (fDimension == 1)
      numBins = fNumBins[0];
   else if (fDimension == 2)
      numBins = fNumBins[0] * fNumBins[1];
   else if (fDimension == 3)
      numBins = fNumBins[0] * fNumBins[1] * fNumBins[2];
   else {
      coutE(Eval) << "* Error in MCMCInterval::DetermineInterval: " <<
                     "Couldn't handle dimension: " << fDimension << endl;
      numBins = 0;
   }

   //TFile chainHistFile("chainHist.root", "recreate");
   //fHist->Write();
   //chainHistFile.Close();

   std::vector<Int_t> bins(numBins);
   // index 1 to numBins because TH1 uses bin 0 for underflow and 
   // bin numBins+1 for overflow
   for (Int_t ibin = 1; ibin <= numBins; ibin++)
      bins[ibin - 1] = ibin;
   std::stable_sort( bins.begin(), bins.end(), CompareBins(fHist) );

   Double_t nEntries = fHist->GetSumOfWeights();
   Double_t sum = 0;
   Double_t content;
   Int_t i;
   for (i = numBins - 1; i >= 0; i--) {
      content = fHist->GetBinContent(bins[i]);
      if ((sum + content) / nEntries >= fConfidenceLevel) {
         fCutoff = content;
         if (fIsStrict) {
            sum += content;
            i--;
            break;
         } else {
            i++;
            break;
         }
      }
      sum += content;
   }

   if (fIsStrict) {
      // keep going to find the sum
      for ( ; i >= 0; i--) {
         content = fHist->GetBinContent(bins[i]);
         if (content == fCutoff)
            sum += content;
         else
            break; // content must be < fCutoff
      }
   } else {
      // backtrack to find the cutoff and sum
      for ( ; i < numBins; i++) {
         content = fHist->GetBinContent(bins[i]);
         if (content > fCutoff) {
            fCutoff = content;
            break;
         } else // content == fCutoff
            sum -= content;
         if (i == numBins - 1)
            // still haven't set fCutoff correctly yet, and we have no bins
            // left, so set fCutoff to something higher than the tallest bin
            fCutoff = fHist->GetBinContent(bins[i]) + 1.0;
      }
   }

   fIntervalSum = sum;
}

// Determine the lower limit for param on this interval
Double_t MCMCInterval::LowerLimit(RooRealVar& param)
{
   for (Int_t d = 0; d < fDimension; d++) {
      if (strcmp(fAxes[d]->GetName(), param.GetName()) == 0) {
         Int_t numBins = fNumBins[d];
         for (Int_t i = 1; i <= numBins; i++)
            if (fHist->GetBinContent(i) >= fCutoff)
                return fHist->GetBinCenter(i);
      }
   }
   return param.getMin();
}

// Determine the upper limit for each param on this interval
Double_t MCMCInterval::UpperLimit(RooRealVar& param)
{
   for (Int_t d = 0; d < fDimension; d++) {
      if (strcmp(fAxes[d]->GetName(), param.GetName()) == 0) {
         Int_t numBins = fNumBins[d];
         Double_t upperLimit = param.getMin();
         for (Int_t i = 1; i <= numBins; i++)
            if (fHist->GetBinContent(i) >= fCutoff)
               upperLimit = fHist->GetBinCenter(i);
         return upperLimit;
      }
   }
   return param.getMax();
}

TH1* MCMCInterval::GetPosteriorHist()
{
  if(fConfidenceLevel == 0)
      coutE(Eval) << "Error in MCMCInterval::GetPosteriorHist, confidence level not set " << endl;
  return (TH1*) fHist->Clone("MCMCposterior");
}

RooArgSet* MCMCInterval::GetParameters() const
{  
   // returns list of parameters
   return (RooArgSet*) fParameters->clone((std::string(fParameters->GetName())+"_clone").c_str());
}

Bool_t MCMCInterval::CheckParameters(RooArgSet& parameterPoint) const
{  
   // check that the parameters are correct

   if (parameterPoint.getSize() != fParameters->getSize() ) {
     coutE(Eval) << "MCMCInterval: size is wrong, parameters don't match" << std::endl;
     return false;
   }
   if ( ! parameterPoint.equals( *fParameters ) ) {
     coutE(Eval) << "MCMCInterval: size is ok, but parameters don't match" << std::endl;
     return false;
   }
   return true;
}
