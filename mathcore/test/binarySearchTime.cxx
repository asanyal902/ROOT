#include <iostream>
#include <ctime>
#include <vector>

#include <TRandom2.h>
#include <TMath.h>
#include <TStopwatch.h>

#include <TApplication.h>
#include <TCanvas.h>
#include <TH2F.h>
#include <TGraph.h>
#include <TLegend.h>
#include <TAxis.h>

using namespace std;

const int npass = 100000;
const int maxint = 100;//20;
const int minsize = 1000;//20;
const int maxsize = 1500;//500;
const int increment = 10;
const int arraysize = (maxsize-minsize)/10 + 1;

template <typename T> void testBinarySearch(const int n, double* tTMath, double* tStd)
{
   vector<T> k(n);
   TStopwatch t; 

   TRandom2 r( time( 0 ) );
   for ( Int_t i = 0; i < n; i++) {
      k[i] = (T) r.Integer( maxint ); 
   }

   std::sort(k.begin(), k.end());

   t.Start(); 
   for (int j = 0; j < npass; ++j) { 
      for ( T elem = 0; elem < maxint; ++elem ) {
         /*Long_t index = */TMath::BinarySearch((Long_t) 20, &k[0], elem);
      }
   }
   t.Stop(); 
   *tTMath = t.RealTime();
   cout << "TMath::BinarySearch time :\t " << t.RealTime() << endl;


   t.Start(); 
   for (int j = 0; j < npass; ++j) { 
      for ( T elem = 0; elem < maxint; ++elem ) {
         T* pind;
         pind = std::lower_bound(&k[0], &k[20], elem);
         //Long_t index2 = ((*pind == elem)? (pind - &k[0]): ( pind - &k[0] - 1));
      }
   }
   t.Stop(); 
   *tStd = t.RealTime();
   std::cout << "std::binary_search time:\t " << t.RealTime() << '\n' << std::endl;

}

void binarySearchTime()
{
   vector<double> tM( arraysize );
   vector<double> tS( arraysize );
   vector<double> index( arraysize );

   //cout << (maxsize-minsize)/10 + 1 << endl;

   for ( int i = minsize; i <= maxsize; i += increment)
   {
      testBinarySearch<Double_t>(i, &tM[(i-minsize)/10], &tS[(i-minsize)/10]);
      index[(i-minsize)/10] = i;
   }

   for ( int i = minsize; i <= maxsize; i += increment)
      cout << tM[(i-minsize)/10] << ' ' << tS[(i-minsize)/10] << endl;

   TCanvas* c1 = new TCanvas("c1", "Comparision of Searching Time", 600, 400);
   TH2F* hpx = new TH2F("hpx", "Comparision of Searching Time", arraysize, minsize, maxsize, arraysize, 0.25,tM[arraysize-1]+0.25);
   hpx->SetStats(kFALSE);
   hpx->Draw();
   
   TGraph* gM = new TGraph(arraysize, &index[0], &tM[0]);
   gM->SetLineColor(2);
   gM->SetLineWidth(3);
   gM->SetTitle("TMath::BinarySearch()");
   gM->Draw("SAME");

   TGraph* gS = new TGraph(arraysize, &index[0], &tS[0]);
   gS->SetLineColor(3);
   gS->SetLineWidth(3);
   gS->SetTitle("std::binary_search()");
   gS->Draw("SAME");

   TLegend* legend = new TLegend(0.15,0.72,0.4,0.86);
   legend->AddEntry(gM, "TMath::BinarySearch()");
   legend->AddEntry(gS, "std::binary_search()");
   legend->Draw();

   hpx->GetXaxis()->SetTitle("Array Size");
   hpx->GetYaxis()->SetTitle("Time");
   

   c1->Show();

   cout << "Test done!" << endl;
}

int main(int argc, char **argv)
{
   TApplication theApp("App",&argc,argv);
   binarySearchTime();
   theApp.Run();

   return 0;
}
