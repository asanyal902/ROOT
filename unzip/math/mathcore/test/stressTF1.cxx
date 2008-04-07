#include <iostream>

#include "TMath.h"
#include "TF1.h"
#include "TCanvas.h"
#include "TGraph.h"
#include "TH2.h"
#include "TStopwatch.h"
#include <cmath>

using namespace std;

const double XMIN = 0, XMAX = 2*TMath::Pi();
const Int_t NB = 100;
const Int_t REP = 1000;

double sumTime = 0; 


void DrawFunction(TF1* f1)
{
   Double_t x[NB], y[NB], ceros[NB];

   for ( Int_t i = (Int_t) XMIN; i < NB; ++i )
   {
      x[i]  = (Double_t) i*(XMAX-XMIN)/(NB-1);
      y[i]  = f1->Eval(x[i]);
      ceros[i] = 0;
   }

   new TCanvas("c1", "Sin(x)", 600, 400);
   TH2F* hpx = new TH2F("hpx", "Sin(x)", NB, XMIN, XMAX, NB, -1,1);
   hpx->SetStats(kFALSE);
   hpx->Draw();
   
   TGraph* gf = new TGraph(NB, x, y);
   gf->SetLineColor(1);
   gf->SetLineWidth(3);
   gf->SetTitle("Function: sin(x)");
   gf->Draw("SAME");

   TGraph *axis = new TGraph(NB, x, ceros);
   axis->SetLineColor(1);
   axis->SetLineWidth(1);
   axis->SetLineStyle(2);
   axis->SetTitle("Function: axis");
   axis->Draw("SAME");
   
}

int PrintStatus(const char* begin, double result, double expected, double time)
{
   double difference = std::abs(result-expected);
   string passed = "FALSE";

   if ( difference < 1E-7 )
      passed = "OK";

   cout << begin << " Obtained: ";
   cout.width(12);
   cout << result << " Expected: ";
   cout.width(12);
   cout << expected << " Difference: ";
   cout.width(12);
   cout << difference << " Time: ";
   cout.width(12);
   cout << time << " .............." << passed
        <<  endl;

   return passed != "OK";
}

int TestRoot(TF1* f1)
{
   double x, root;
   int status = 0;
   TStopwatch w;
   double totalTime = 0;

   cout << "ROOT TEST\n" 
        << "---------------------------------------------------------"
        << endl;

   w.Start(kTRUE);
   for ( int j = 0; j < REP; ++j )
      x = f1->GetX(0, XMIN, 1);
   w.Stop();
   root = 0;
   status += PrintStatus("Root", x, root, w.RealTime()/REP * 1000 );
   totalTime += w.RealTime()/REP * 1000;

   w.Start(kTRUE);
   for ( int j = 0; j < REP; ++j )
      x = f1->GetX(0, 2.5, 3.5);
   w.Stop();
   root = TMath::Pi();
   status += PrintStatus("Root", x, root, w.RealTime()/REP * 1000 );
   totalTime += w.RealTime()/REP * 1000;

   w.Start(kTRUE);
   for ( int j = 0; j < REP; ++j )
      x = f1->GetX(0, 6, XMAX);
   w.Stop();
   root = TMath::Pi() * 2;
   status += PrintStatus("Root", x, root, w.RealTime()/REP * 1000 );
   totalTime += w.RealTime()/REP * 1000;

   cout << "Total Time: " << totalTime << endl;

   sumTime += totalTime;

   return status;
}

int TestMaxMin(TF1* f1)
{
   double x, maxmin;
   int status = 0;
   TStopwatch w;
   double totalTime = 0;

   cout << "MAXMIN TEST\n" 
        << "---------------------------------------------------------"
        << endl;

   w.Start(kTRUE);
   for ( int j = 0; j < REP; ++j )
      x = f1->GetMaximumX(XMIN, TMath::Pi());
   w.Stop();
   maxmin = TMath::Pi() / 2;
   status += PrintStatus("Maximum", x, maxmin, w.RealTime()/REP * 1000 );
   totalTime += w.RealTime()/REP * 1000;

   w.Start(kTRUE);
   for ( int j = 0; j < REP; ++j )
      x = f1->GetMinimumX(TMath::Pi(), XMAX);
   w.Stop();
   maxmin = TMath::Pi() * 1.5;
   status += PrintStatus("Minimum", x, maxmin, w.RealTime()/REP * 1000 );
   totalTime += w.RealTime()/REP * 1000;

   cout << "Total Time: " << totalTime << endl;

   sumTime += totalTime;

   return status;
}

int TestDerivative(TF1* f1)
{
   double x, derivative;
   int status = 0;
   TStopwatch w;
   double totalTime = 0;

   cout << "Derivative TEST\n" 
        << "---------------------------------------------------------"
        << endl;

   for ( double i = XMIN; i < XMAX; i += 1.5 )   
   {
      w.Start(kTRUE);
      for ( int j = 0; j < REP; ++j )
         x = f1->Derivative(i);
      w.Stop();
      derivative = TMath::Cos(i);
      status += PrintStatus("Derivative", x, derivative, w.RealTime()/REP * 1000 );
      totalTime += w.RealTime()/REP * 1000;
   }

   cout << "Total Time: " << totalTime << endl;

   sumTime += totalTime;

   return status;
}

int TestIntegral(TF1* f1)
{
   double x, integral;
   int status = 0;
   TStopwatch w;
   double totalTime = 0;

   cout << "Integral TEST\n" 
        << "---------------------------------------------------------"
        << endl;

   for ( double i = XMIN; i < XMAX; i += 1.5 )   
   {
      w.Start(kTRUE);
      for ( int j = 0; j < REP; ++j )
         x = f1->Integral(0, i);
      w.Stop();
      integral = - TMath::Cos(i) + 1;
      status += PrintStatus("Integral", x, integral, w.RealTime()/REP * 1000 );
      totalTime += w.RealTime()/REP * 1000;
   }

   cout << "Total Time: " << totalTime << endl;

   sumTime += totalTime;

   return status;
}

int stressTF1() 
{
   int status = 0;

   TF1* f1 = new TF1("f1", "sin(x)", XMIN, XMAX);
   //DrawFunction(f1);

   cout << "Starting Tests..." << endl;

   status += TestRoot(f1);
   status += TestMaxMin(f1);
   status += TestDerivative(f1);
   status += TestIntegral(f1);

   cout << "End of Tests..." << endl;
   cout << "Total time for all tests: " << sumTime << endl;
   

   return status;
}

int main()
{
   return stressTF1();
}
