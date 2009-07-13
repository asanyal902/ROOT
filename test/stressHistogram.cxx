// @(#)root/test:$name:  $:$id: stressHistogram.cxx,v 1.15 2002/10/25 10:47:51 rdm exp $
// Authors: David Gonzalez Maline November 2008

//*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*//
//                                                                               //
//                                                                               //
// Here there is a set of tests for the histogram classes (including             //
// histograms and profiles). The methods tested work on:                         //
//                                                                               //
// 1. Projection testing (with and without weights)                              //
// 2. Rebinning                                                                  //
// 3. Addition, multiplication an division operations.                           //
// 4. Building and copying instances.                                            //
// 5. I/O functionality (including reference with older versions).               //
// 6. Labeling.                                                                  //
// 7. Interpolation                                                              //
//                                                                               //
// To see the tests individually, at the bottom of the file the tests            //
// are exectued using the structure TTestSuite, that defines the                 //
// subset, the number of routines to be tested as well as the pointes            //
// for these. Every tests is mean to be simple enough to be understood           //
// without much comments.                                                        //
//                                                                               //
// Finally, for debugging reasons, the struct compareOptions can be              //
// used to define the level of output of the tests, beging set                   //
// generally for the whole suit in defaultEqualOptions.                          //
//                                                                               //
//                                                                               //
// An example of output when all the tests run OK is shown below:                //
// ****************************************************************************  //
// *  Starting  stress  H I S T O G R A M                                     *  //
// ****************************************************************************  //
// Test  1: Testing Projections without weights..............................OK  //
// Test  2: Testing Projections with weights.................................OK  //
// Test  3: Histogram Rebinning..............................................OK  //
// Test  4: Add tests for 1D, 2D and 3D Histograms and Profiles..............OK  //
// Test  5: Multiply tests for 1D, 2D and 3D Histograms......................OK  //
// Test  6: Divide tests for 1D, 2D and 3D Histograms........................OK  //
// Test  7: Copy tests for 1D, 2D and 3D Histograms and Profiles.............OK  //
// Test  8: Read/Write tests for 1D, 2D and 3D Histograms and Profiles.......OK  //
// Test  9: Merge tests for 1D, 2D and 3D Histograms and Profiles............OK  //
// Test 10: Label tests for 1D Histograms (TAxis)............................OK  //
// Test 11: Interpolation tests for Histograms...............................OK  //
// Test 12: Reference File Read for Histograms and Profiles..................OK  //
// ****************************************************************************  //
// stressHistogram: Real Time =  48.82 seconds Cpu Time =  48.66 seconds         //
//  ROOTMARKS = 565.557 ROOT version: 5.23/01	branches/dev/mathDev@27607       //
// ****************************************************************************  //
//                                                                               //
//*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*//


#include <sstream>
#include <cmath>

#include "TH2.h"
#include "TH3.h"
#include "TH2.h"
#include "THnSparse.h"

#include "TProfile.h"
#include "TProfile2D.h"
#include "TProfile3D.h"

#include "TApplication.h"
#include "TBenchmark.h"
#include "Riostream.h"
#include "TMath.h"
#include "TRandom2.h"
#include "TFile.h"
#include "TClass.h"

#include "TROOT.h"
#include <algorithm>

const unsigned int __DRAW__ = 0;

const Double_t minRange = 1;
const Double_t maxRange = 5;

const Double_t minRebin = 3;
const Double_t maxRebin = 7;

const int minBinValue = 1;
const int maxBinValue = 10;

const int nEvents = 1000;
const int numberOfBins = 10;

enum compareOptions {
   cmpOptNone=0,
   cmpOptPrint=1,
   cmpOptDebug=2,
   cmpOptNoError=4,
   cmpOptStats=8
};

const int defaultEqualOptions = 0; //cmpOptPrint;
//int defaultEqualOptions = cmpOptPrint;

const double defaultErrorLimit = 1.E-10;

enum RefFileEnum {
   refFileRead = 1,
   refFileWrite = 2
};

const int refFileOption = 1;
TFile * refFile = 0;
const char* refFileName = "http://root.cern.ch/files/stressHistogram.5.18.00.root";

TRandom2 r;
// set to zero if want to run different every time
const int initialSeed = 0;   



typedef bool ( * pointer2Test) ();

struct TTestSuite {
   unsigned int nTests;
   char suiteName[75];
   pointer2Test* tests;
};

// Methods for histogram comparisions (later implemented)
void printResult(const char* msg, bool status);
void FillVariableRange(Double_t v[numberOfBins+1]);
void FillHistograms(TH1D* h1, TH1D* h2, Double_t c1 = 1.0, Double_t c2 = 1.0);
void FillProfiles(TProfile* p1, TProfile* p2, Double_t c1 = 1.0, Double_t c2 = 1.0);
int equals(const char* msg, TH1D* h1, TH1D* h2, int options = 0, double ERRORLIMIT = defaultErrorLimit);
int equals(const char* msg, TH2D* h1, TH2D* h2, int options = 0, double ERRORLIMIT = defaultErrorLimit);
int equals(const char* msg, TH3D* h1, TH3D* h2, int options = 0, double ERRORLIMIT = defaultErrorLimit);
int equals(const char* msg, THnSparse* h1, THnSparse* h2, int options = 0, double ERRORLIMIT = defaultErrorLimit);
int equals(Double_t n1, Double_t n2, double ERRORLIMIT = defaultErrorLimit);
int compareStatistics( TH1* h1, TH1* h2, bool debug, double ERRORLIMIT = defaultErrorLimit);
ostream& operator<<(ostream& out, TH1D* h);
// old stresHistOpts.cxx file

bool testAdd1() 
{
   // Tests the first Add method for 1D Histograms

   Double_t c1 = r.Rndm();
   Double_t c2 = r.Rndm();

   TH1D* h1 = new TH1D("t1D1-h1", "h1-Title", numberOfBins, minRange, maxRange);
   TH1D* h2 = new TH1D("t1D1-h2", "h2-Title", numberOfBins, minRange, maxRange);
   TH1D* h3 = new TH1D("t1D1-h3", "h3=c1*h1+c2*h2", numberOfBins, minRange, maxRange);

   h1->Sumw2();h2->Sumw2();h3->Sumw2();

   FillHistograms(h1, h3, 1.0, c1);
   FillHistograms(h2, h3, 1.0, c2);

   TH1D* h4 = new TH1D("t1D1-h4", "h4=c1*h1+h2*c2", numberOfBins, minRange, maxRange);
   h4->Add(h1, h2, c1, c2);

   bool ret = equals("Add1D1", h3, h4, cmpOptStats, 1E-13);
   delete h1;
   delete h2;
   delete h3;
   return ret;
}

bool testAddProfile1() 
{
   // Tests the first Add method for 1D Profiles

   Double_t c1 = r.Rndm();
   Double_t c2 = r.Rndm();

   TProfile* p1 = new TProfile("t1D1-p1", "p1-Title", numberOfBins, minRange, maxRange);
   TProfile* p2 = new TProfile("t1D1-p2", "p2-Title", numberOfBins, minRange, maxRange);
   TProfile* p3 = new TProfile("t1D1-p3", "p3=c1*p1+c2*p2", numberOfBins, minRange, maxRange);

   for ( Int_t e = 0; e < nEvents; ++e ) {
      Double_t x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      p1->Fill(x, y, 1.0);
      p3->Fill(x, y,  c1);
   }

   for ( Int_t e = 0; e < nEvents; ++e ) {
      Double_t x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      p2->Fill(x, y, 1.0);
      p3->Fill(x, y,  c2);
   }

   TProfile* p4 = new TProfile("t1D1-p4", "p4=c1*p1+p2*c2", numberOfBins, minRange, maxRange);
   p4->Add(p1, p2, c1, c2);

   bool ret = equals("Add1DProfile1", p3, p4, cmpOptStats, 1E-13);
   delete p1;
   delete p2;
   delete p3;
   return ret;
}

bool testAdd2() 
{
   // Tests the second Add method for 1D Histograms

   Double_t c2 = r.Rndm();

   TH1D* h5 = new TH1D("t1D2-h5", "h5=   h6+c2*h7", numberOfBins, minRange, maxRange);
   TH1D* h6 = new TH1D("t1D2-h6", "h6-Title", numberOfBins, minRange, maxRange);
   TH1D* h7 = new TH1D("t1D2-h7", "h7-Title", numberOfBins, minRange, maxRange);

   h5->Sumw2();h6->Sumw2();h7->Sumw2();

   FillHistograms(h6, h5, 1.0, 1.0);
   FillHistograms(h7, h5, 1.0, c2);

   h6->Add(h7, c2);
   
   bool ret = equals("Add1D2", h5, h6, cmpOptStats, 1E-13);
   delete h5;
   delete h7;
   return ret;
}

bool testAddProfile2() 
{
   // Tests the second Add method for 1D Profiles

   Double_t c2 = r.Rndm();

   TProfile* p5 = new TProfile("t1D2-p5", "p5=   p6+c2*p7", numberOfBins, minRange, maxRange);
   TProfile* p6 = new TProfile("t1D2-p6", "p6-Title", numberOfBins, minRange, maxRange);
   TProfile* p7 = new TProfile("t1D2-p7", "p7-Title", numberOfBins, minRange, maxRange);

   for ( Int_t e = 0; e < nEvents; ++e ) {
      Double_t x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      p6->Fill(x, y, 1.0);
      p5->Fill(x, y, 1.0);
   }

   for ( Int_t e = 0; e < nEvents; ++e ) {
      Double_t x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      p7->Fill(x, y, 1.0);
      p5->Fill(x, y,  c2);
   }

   p6->Add(p7, c2);
   
   bool ret = equals("Add1DProfile2", p5, p6, cmpOptStats, 1E-13);
   delete p5;
   delete p7;
   return ret;
}

bool testAdd3() 
{
   // Tests the first add method to do scalation of 1D Histograms

   Double_t c1 = r.Rndm();

   TH1D* h1 = new TH1D("t1D1-h1", "h1-Title", numberOfBins, minRange, maxRange);
   TH1D* h2 = new TH1D("t1D1-h2", "h2=c1*h1+c2*h2", numberOfBins, minRange, maxRange);

   h1->Sumw2();h2->Sumw2();

   for ( Int_t e = 0; e < nEvents; ++e ) {
      Double_t value = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      h1->Fill(value,  1.0);
      h2->Fill(value, c1 / h1->GetBinWidth( h1->FindBin(value) ) );
   }

   TH1D* h3 = new TH1D("t1D1-h3", "h3=c1*h1", numberOfBins, minRange, maxRange);
   h3->Add(h1, h1, c1, -1);

   bool ret = equals("Add1D3", h2, h3, cmpOptStats, 1E-13);
   delete h1;
   delete h2;
   return ret;
}

bool testAddVar1()
{
   // Tests the second Add method for 1D Histograms with variable bin size

   Double_t v[numberOfBins+1];
   FillVariableRange(v);

   Double_t c1 = r.Rndm();
   Double_t c2 = r.Rndm();

   TH1D* h1 = new TH1D("h1", "h1-Title", numberOfBins, v);
   TH1D* h2 = new TH1D("h2", "h2-Title", numberOfBins, v);
   TH1D* h3 = new TH1D("h3", "h3=c1*h1+c2*h2", numberOfBins, v);

   h1->Sumw2();h2->Sumw2();h3->Sumw2();

   FillHistograms(h1, h3, 1.0, c1);
   FillHistograms(h2, h3, 1.0, c2);

   TH1D* h4 = new TH1D("t1D1-h4", "h4=c1*h1+h2*c2", numberOfBins, v);
   h4->Add(h1, h2, c1, c2);

   bool ret = equals("AddVar1D1", h3, h4, cmpOptStats, 1E-13);
   delete h1;
   delete h2;
   delete h3;
   return ret;
}

bool testAddVarProf1()
{

   // Tests the first Add method for 1D Profiles with variable bin size

   Double_t v[numberOfBins+1];
   FillVariableRange(v);

   Double_t c1 = r.Rndm();
   Double_t c2 = r.Rndm();

   TProfile* p1 = new TProfile("t1D1-p1", "p1-Title", numberOfBins, v);
   TProfile* p2 = new TProfile("t1D1-p2", "p2-Title", numberOfBins, v);
   TProfile* p3 = new TProfile("t1D1-p3", "p3=c1*p1+c2*p2", numberOfBins, v);

   FillProfiles(p1, p3, 1.0, c1);
   FillProfiles(p2, p3, 1.0, c2);

   TProfile* p4 = new TProfile("t1D1-p4", "p4=c1*p1+p2*c2", numberOfBins, v);
   p4->Add(p1, p2, c1, c2);

   bool ret = equals("AddVar1DProf1", p3, p4, cmpOptStats, 1E-13);
   delete p1;
   delete p2;
   delete p3;

   return ret;
}

bool testAddVar2()
{
   // Tests the second Add method for 1D Histograms with variable bin size

   Double_t v[numberOfBins+1];
   FillVariableRange(v);

   Double_t c2 = r.Rndm();

   TH1D* h5 = new TH1D("t1D2-h5", "h5=   h6+c2*h7", numberOfBins, v);
   TH1D* h6 = new TH1D("t1D2-h6", "h6-Title", numberOfBins, v);
   TH1D* h7 = new TH1D("t1D2-h7", "h7-Title", numberOfBins, v);

   h5->Sumw2();h6->Sumw2();h7->Sumw2();

   FillHistograms(h6, h5, 1.0, 1.0);
   FillHistograms(h7, h5, 1.0, c2);

   h6->Add(h7, c2);
   
   bool ret = equals("AddVar1D2", h5, h6, cmpOptStats, 1E-13);
   delete h5;
   delete h7;
   return ret;
}

bool testAddVarProf2()
{
   // Tests the second Add method for 1D Profiles with variable bin size

   Double_t v[numberOfBins+1];
   FillVariableRange(v);

   Double_t c2 = r.Rndm();

   TProfile* p5 = new TProfile("t1D2-p5", "p5=   p6+c2*p7", numberOfBins, v);
   TProfile* p6 = new TProfile("t1D2-p6", "p6-Title", numberOfBins, v);
   TProfile* p7 = new TProfile("t1D2-p7", "p7-Title", numberOfBins, v);

   p5->Sumw2();p6->Sumw2();p7->Sumw2();

   FillProfiles(p6, p5, 1.0, 1.0);
   FillProfiles(p7, p5, 1.0, c2);

   p6->Add(p7, c2);
   
   bool ret = equals("AddVar1D2", p5, p6, cmpOptStats, 1E-13);
   delete p5;
   delete p7;
   return ret;
}

bool testAdd2D3()
{
   // Tests the first add method to do scalation of 2D Histograms

   Double_t c1 = r.Rndm();

   TH2D* h1 = new TH2D("t1D1-h1", "h1-Title", 
                       numberOfBins, minRange, maxRange,
                       numberOfBins+2, minRange, maxRange);
   TH2D* h2 = new TH2D("t1D1-h2", "h2=c1*h1+c2*h2", 
                       numberOfBins, minRange, maxRange,
                       numberOfBins+2, minRange, maxRange);

   h1->Sumw2();h2->Sumw2();

   for ( Int_t e = 0; e < nEvents * nEvents; ++e ) {
      Double_t x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      h1->Fill(x, y, 1.0);
      Int_t binx = h1->GetXaxis()->FindBin(x);
      Int_t biny = h1->GetYaxis()->FindBin(y);
      Double_t area = h1->GetXaxis()->GetBinWidth( binx ) * h1->GetYaxis()->GetBinWidth( biny );
      h2->Fill(x, y, c1 / area);
   }

   TH2D* h3 = new TH2D("t1D1-h3", "h3=c1*h1", 
                       numberOfBins, minRange, maxRange,
                       numberOfBins+2, minRange, maxRange);
   h3->Add(h1, h1, c1, -1);

   bool ret = equals("Add1D2", h2, h3, cmpOptStats, 1E-10);
   delete h1;
   delete h2;
   return ret;
}

bool testAdd3D3()
{
   // Tests the first add method to do scalation of 3D Histograms

   Double_t c1 = r.Rndm();

   TH3D* h1 = new TH3D("t1D1-h1", "h1-Title", 
                       numberOfBins, minRange, maxRange,
                       numberOfBins+1, minRange, maxRange,
                       numberOfBins+2, minRange, maxRange);
   TH3D* h2 = new TH3D("t1D1-h2", "h2=c1*h1+c2*h2", 
                       numberOfBins, minRange, maxRange,
                       numberOfBins+1, minRange, maxRange,
                       numberOfBins+2, minRange, maxRange);

   h1->Sumw2();h2->Sumw2();

   for ( Int_t e = 0; e < nEvents * nEvents; ++e ) {
      Double_t x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t z = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      h1->Fill(x, y, z, 1.0);
      Int_t binx = h1->GetXaxis()->FindBin(x);
      Int_t biny = h1->GetYaxis()->FindBin(y);
      Int_t binz = h1->GetZaxis()->FindBin(z);
      Double_t area = h1->GetXaxis()->GetBinWidth( binx ) * 
                      h1->GetYaxis()->GetBinWidth( biny ) * 
                      h1->GetZaxis()->GetBinWidth( binz );
      h2->Fill(x, y, z, c1 / area);
   }

   TH3D* h3 = new TH3D("t1D1-h3", "h3=c1*h1", 
                       numberOfBins, minRange, maxRange,
                       numberOfBins+1, minRange, maxRange,
                       numberOfBins+2, minRange, maxRange);
   h3->Add(h1, h1, c1, -1);

   bool ret = equals("Add2D3", h2, h3, cmpOptStats, 1E-10);
   delete h1;
   delete h2;
   return ret;
}

bool testAdd2D1() 
{
   // Tests the first Add method for 2D Histograms

   Double_t c1 = r.Rndm();
   Double_t c2 = r.Rndm();

   TH2D* h1 = new TH2D("t2D1-h1", "h1", 
                       numberOfBins, minRange, maxRange,
                       numberOfBins + 2, minRange, maxRange);

   TH2D* h2 = new TH2D("t2D1-h2", "h2", 
                       numberOfBins, minRange, maxRange,
                       numberOfBins + 2, minRange, maxRange);

   TH2D* h3 = new TH2D("t2D1-h3", "h3=c1*h1+c2*h2", 
                       numberOfBins, minRange, maxRange,
                       numberOfBins + 2, minRange, maxRange);

   h1->Sumw2();h2->Sumw2();h3->Sumw2();

   for ( Int_t e = 0; e < nEvents * nEvents; ++e ) {
      Double_t x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      h1->Fill(x, y,  1.0);
      h3->Fill(x, y, c1);
   }

   for ( Int_t e = 0; e < nEvents * nEvents; ++e ) {
      Double_t x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      h2->Fill(x, y,  1.0);
      h3->Fill(x, y, c2);
   }

   TH2D* h4 = new TH2D("t2D1-h4", "h4=c1*h1+c2*h2", 
                       numberOfBins, minRange, maxRange,
                       numberOfBins + 2, minRange, maxRange);
   h4->Add(h1, h2, c1, c2);
   bool ret = equals("Add2D1", h3, h4, cmpOptStats , 1E-10);
   delete h1;
   delete h2;
   delete h3;
   return ret;
}

bool testAdd2DProfile1() 
{
   // Tests the first Add method for 1D Profiles

   Double_t c1 = r.Rndm();
   Double_t c2 = r.Rndm();

   TProfile2D* p1 = new TProfile2D("t2D1-p1", "p1", 
                                   numberOfBins, minRange, maxRange,
                                   numberOfBins + 2, minRange, maxRange);

   TProfile2D* p2 = new TProfile2D("t2D1-p2", "p2", 
                                   numberOfBins, minRange, maxRange,
                                   numberOfBins + 2, minRange, maxRange);

   TProfile2D* p3 = new TProfile2D("t2D1-p3", "p3=c1*p1+c2*p2", 
                                   numberOfBins, minRange, maxRange,
                                   numberOfBins + 2, minRange, maxRange);

   for ( Int_t e = 0; e < nEvents * nEvents; ++e ) {
      Double_t x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t z = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      p1->Fill(x, y, z, 1.0);
      p3->Fill(x, y, z, c1);
   }

   for ( Int_t e = 0; e < nEvents * nEvents; ++e ) {
      Double_t x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t z = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      p2->Fill(x, y, z, 1.0);
      p3->Fill(x, y, z, c2);
   }

   TProfile2D* p4 = new TProfile2D("t2D1-p4", "p4=c1*p1+c2*p2", 
                       numberOfBins, minRange, maxRange,
                       numberOfBins + 2, minRange, maxRange);
   p4->Add(p1, p2, c1, c2);
   bool ret = equals("Add2DProfile1", p3, p4, cmpOptStats , 1E-10);
   delete p1;
   delete p2;
   delete p3;
   return ret;
}

bool testAdd2D2() 
{
   // Tests the second Add method for 2D Histograms

   Double_t c2 = r.Rndm();

   TH2D* h1 = new TH2D("t2D2-h1", "h1", 
                       numberOfBins, minRange, maxRange,
                       numberOfBins + 2, minRange, maxRange);

   TH2D* h2 = new TH2D("t2D2-h2", "h2", 
                       numberOfBins, minRange, maxRange,
                       numberOfBins + 2, minRange, maxRange);

   TH2D* h3 = new TH2D("t2D2-h3", "h3=h1+c2*h2", 
                       numberOfBins, minRange, maxRange,
                       numberOfBins + 2, minRange, maxRange);

   h1->Sumw2();h2->Sumw2();h3->Sumw2();

   for ( Int_t e = 0; e < nEvents * nEvents; ++e ) {
      Double_t x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      h1->Fill(x, y,  1.0);
      h3->Fill(x, y,  1.0);
   }

   for ( Int_t e = 0; e < nEvents * nEvents; ++e ) {
      Double_t x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      h2->Fill(x, y,  1.0);
      h3->Fill(x, y, c2);
   }

   h1->Add(h2, c2);
   bool ret = equals("Add2D2", h3, h1, cmpOptStats, 1E-10);
   delete h2;
   delete h3;
   return ret;
}

bool testAdd2DProfile2() 
{
   // Tests the second Add method for 2D Profiles

   Double_t c2 = r.Rndm();

   TProfile2D* p1 = new TProfile2D("t2D2-p1", "p1", 
                                   numberOfBins, minRange, maxRange,
                                   numberOfBins + 2, minRange, maxRange);

   TProfile2D* p2 = new TProfile2D("t2D2-p2", "p2", 
                                   numberOfBins, minRange, maxRange,
                                   numberOfBins + 2, minRange, maxRange);

   TProfile2D* p3 = new TProfile2D("t2D2-p3", "p3=p1+c2*p2", 
                                   numberOfBins, minRange, maxRange,
                                   numberOfBins + 2, minRange, maxRange);

   for ( Int_t e = 0; e < nEvents * nEvents; ++e ) {
      Double_t x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t z = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      p1->Fill(x, y, z, 1.0);
      p3->Fill(x, y, z, 1.0);
   }

   for ( Int_t e = 0; e < nEvents * nEvents; ++e ) {
      Double_t x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t z = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      p2->Fill(x, y, z, 1.0);
      p3->Fill(x, y, z,  c2);
   }

   p1->Add(p2, c2);
   bool ret = equals("Add2DProfile2", p3, p1, cmpOptStats, 1E-10);
   delete p2;
   delete p3;
   return ret;
}

bool testAdd3D1() 
{
   // Tests the first Add method for 3D Histograms

   Double_t c1 = r.Rndm();
   Double_t c2 = r.Rndm();

   TH3D* h1 = new TH3D("t3D1-h1", "h1", 
                       numberOfBins, minRange, maxRange,
                       numberOfBins + 1, minRange, maxRange,
                       numberOfBins + 2, minRange, maxRange);

   TH3D* h2 = new TH3D("t3D1-h2", "h2", 
                       numberOfBins, minRange, maxRange,
                       numberOfBins + 1, minRange, maxRange,
                       numberOfBins + 2, minRange, maxRange);

   TH3D* h3 = new TH3D("t3D1-h3", "h3=c1*h1+c2*h2", 
                       numberOfBins, minRange, maxRange,
                       numberOfBins + 1, minRange, maxRange,
                       numberOfBins + 2, minRange, maxRange);

   h1->Sumw2();h2->Sumw2();h3->Sumw2();

   for ( Int_t e = 0; e < nEvents * nEvents; ++e ) {
      Double_t x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t z = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      h1->Fill(x, y, z,  1.0);
      h3->Fill(x, y, z, c1);
   }

   for ( Int_t e = 0; e < nEvents * nEvents; ++e ) {
      Double_t x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t z = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      h2->Fill(x, y, z,  1.0);
      h3->Fill(x, y, z, c2);
   }

   TH3D* h4 = new TH3D("t3D1-h4", "h4=c1*h1+c2*h2", 
                       numberOfBins, minRange, maxRange,
                       numberOfBins + 1, minRange, maxRange,
                       numberOfBins + 2, minRange, maxRange);
   h4->Add(h1, h2, c1, c2);
   bool ret = equals("Add3D1", h3, h4, cmpOptStats, 1E-10);
   delete h1;
   delete h2;
   delete h3;
   return ret;
}

bool testAdd3DProfile1() 
{
   // Tests the second Add method for 3D Profiles

   Double_t c1 = r.Rndm();
   Double_t c2 = r.Rndm();

   TProfile3D* p1 = new TProfile3D("t3D1-p1", "p1", 
                                   numberOfBins, minRange, maxRange,
                                   numberOfBins + 1, minRange, maxRange,
                                   numberOfBins + 2, minRange, maxRange);

   TProfile3D* p2 = new TProfile3D("t3D1-p2", "p2", 
                                   numberOfBins, minRange, maxRange,
                                   numberOfBins + 1, minRange, maxRange,
                                   numberOfBins + 2, minRange, maxRange);

   TProfile3D* p3 = new TProfile3D("t3D1-p3", "p3=c1*p1+c2*p2", 
                                   numberOfBins, minRange, maxRange,
                                   numberOfBins + 1, minRange, maxRange,
                                   numberOfBins + 2, minRange, maxRange);

   for ( Int_t e = 0; e < nEvents * nEvents; ++e ) {
      Double_t x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t z = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t t = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      p1->Fill(x, y, z, t, 1.0);
      p3->Fill(x, y, z, t,  c1);
   }

   for ( Int_t e = 0; e < nEvents * nEvents; ++e ) {
      Double_t x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t z = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t t = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      p2->Fill(x, y, z, t, 1.0);
      p3->Fill(x, y, z, t,  c2);
   }

   TProfile3D* p4 = new TProfile3D("t3D1-p4", "p4=c1*p1+c2*p2", 
                                   numberOfBins, minRange, maxRange,
                                   numberOfBins + 1, minRange, maxRange,
                                   numberOfBins + 2, minRange, maxRange);
   p4->Add(p1, p2, c1, c2);
   bool ret = equals("Add3DProfile1", p3, p4, cmpOptStats, 1E-10);
   delete p1;
   delete p2;
   delete p3;
   return ret;
}

bool testAdd3D2() 
{
   // Tests the second Add method for 3D Histograms

   Double_t c2 = r.Rndm();

   TH3D* h1 = new TH3D("t3D2-h1", "h1", 
                       numberOfBins, minRange, maxRange,
                       numberOfBins + 1, minRange, maxRange,
                       numberOfBins + 2, minRange, maxRange);

   TH3D* h2 = new TH3D("t3D2-h2", "h2", 
                       numberOfBins, minRange, maxRange,
                       numberOfBins + 1, minRange, maxRange,
                       numberOfBins + 2, minRange, maxRange);

   TH3D* h3 = new TH3D("t3D2-h3", "h3=h1+c2*h2", 
                       numberOfBins, minRange, maxRange,
                       numberOfBins + 1, minRange, maxRange,
                       numberOfBins + 2, minRange, maxRange);

   h1->Sumw2();h2->Sumw2();h3->Sumw2();

   for ( Int_t e = 0; e < nEvents * nEvents; ++e ) {
      Double_t x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t z = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      h1->Fill(x, y, z,  1.0);
      h3->Fill(x, y, z,  1.0);
   }

   for ( Int_t e = 0; e < nEvents * nEvents; ++e ) {
      Double_t x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t z = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      h2->Fill(x, y, z,  1.0);
      h3->Fill(x, y, z, c2);
   }

   h1->Add(h2, c2);
   bool ret = equals("Add3D2", h3, h1, cmpOptStats, 1E-10);
   delete h2;
   delete h3;
   return ret;
}

bool testAdd3DProfile2() 
{
   // Tests the second Add method for 3D Profiles

   Double_t c2 = r.Rndm();

   TProfile3D* p1 = new TProfile3D("t3D2-p1", "p1", 
                                   numberOfBins, minRange, maxRange,
                                   numberOfBins + 1, minRange, maxRange,
                                   numberOfBins + 2, minRange, maxRange);
   
   TProfile3D* p2 = new TProfile3D("t3D2-p2", "p2", 
                                   numberOfBins, minRange, maxRange,
                                   numberOfBins + 1, minRange, maxRange,
                                   numberOfBins + 2, minRange, maxRange);
   
   TProfile3D* p3 = new TProfile3D("t3D2-p3", "p3=p1+c2*p2", 
                                   numberOfBins, minRange, maxRange,
                                   numberOfBins + 1, minRange, maxRange,
                                   numberOfBins + 2, minRange, maxRange);
   
   for ( Int_t e = 0; e < nEvents * nEvents; ++e ) {
      Double_t x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t z = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t t = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      p1->Fill(x, y, z, t, 1.0);
      p3->Fill(x, y, z, t, 1.0);
   }

   for ( Int_t e = 0; e < nEvents * nEvents; ++e ) {
      Double_t x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t z = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t t = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      p2->Fill(x, y, z, t, 1.0);
      p3->Fill(x, y, z, t,  c2);
   }

   p1->Add(p2, c2);
   bool ret = equals("Add3DProfile2", p3, p1, cmpOptStats, 1E-10);
   delete p2;
   delete p3;
   return ret;
}

bool testAddSparse()
{
   // Tests the Add method for Sparse Histograms

   Double_t c = r.Rndm();

   Int_t bsize[] = { TMath::Nint( r.Uniform(1, 5) ),
                          TMath::Nint( r.Uniform(1, 5) ),
                          TMath::Nint( r.Uniform(1, 5) )};
   Double_t xmin[] = {minRange, minRange, minRange};
   Double_t xmax[] = {maxRange, maxRange, maxRange};

   THnSparseD* s1 = new THnSparseD("tS-s1", "s1", 3, bsize, xmin, xmax);
   THnSparseD* s2 = new THnSparseD("tS-s2", "s2", 3, bsize, xmin, xmax);
   THnSparseD* s3 = new THnSparseD("tS-s3", "s3=s1+c*s2", 3, bsize, xmin, xmax);

   s1->Sumw2();s2->Sumw2();s3->Sumw2();

   for ( Int_t e = 0; e < nEvents * nEvents; ++e ) {
      Double_t points[3];
      points[0] = r.Uniform( minRange * .9 , maxRange * 1.1 );
      points[1] = r.Uniform( minRange * .9 , maxRange * 1.1 );
      points[2] = r.Uniform( minRange * .9 , maxRange * 1.1 );
      s1->Fill(points);
      s3->Fill(points);
   }

   for ( Int_t e = 0; e < nEvents * nEvents; ++e ) {
      Double_t points[3];
      points[0] = r.Uniform( minRange * .9 , maxRange * 1.1 );
      points[1] = r.Uniform( minRange * .9 , maxRange * 1.1 );
      points[2] = r.Uniform( minRange * .9 , maxRange * 1.1 );
      s2->Fill(points);
      s3->Fill(points, c);
   }

   s1->Add(s2, c);
   bool ret = equals("AddSparse", s3, s1, cmpOptStats , 1E-10);
   delete s2;
   delete s3;
   return ret;
}

bool testMul1() 
{
   // Tests the first Multiply method for 1D Histograms

   Double_t c1 = r.Rndm();
   Double_t c2 = r.Rndm();

   TH1D* h1 = new TH1D("m1D1-h1", "h1-Title", numberOfBins, minRange, maxRange);
   TH1D* h2 = new TH1D("m1D1-h2", "h2-Title", numberOfBins, minRange, maxRange);
   TH1D* h3 = new TH1D("m1D1-h3", "h3=c1*h1*c2*h2", numberOfBins, minRange, maxRange);

   h1->Sumw2();h2->Sumw2();h3->Sumw2();

   UInt_t seed = r.GetSeed();
   // For possible problems
   r.SetSeed(seed);
   for ( Int_t e = 0; e < nEvents; ++e ) {
      Double_t value = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      h1->Fill(value, 1.0);
   }

   for ( Int_t e = 0; e < nEvents; ++e ) {
      Double_t value = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      h2->Fill(value,  1.0);
      h3->Fill(value,  c1*c2*h1->GetBinContent( h1->GetXaxis()->FindBin(value) ) );
   }

   // h3 has to be filled again so that the erros are properly calculated
   r.SetSeed(seed);
   for ( Int_t e = 0; e < nEvents; ++e ) {
      Double_t value = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      h3->Fill(value,  c1*c2*h2->GetBinContent( h2->GetXaxis()->FindBin(value) ) );
   }

   // No the bin contents has to be reduced, as it was filled twice!
   for ( Int_t bin = 0; bin <= h3->GetNbinsX() + 1; ++bin ) {
      h3->SetBinContent(bin, h3->GetBinContent(bin) / 2 );
   }

   TH1D* h4 = new TH1D("m1D1-h4", "h4=h1*h2", numberOfBins, minRange, maxRange);
   h4->Multiply(h1, h2, c1, c2);

   bool ret = equals("Multiply1D1", h3, h4, cmpOptStats  , 1E-14);
   delete h1;
   delete h2;
   delete h3;
   return ret;
}

bool testMulVar1() 
{
   // Tests the first Multiply method for 1D Histograms with variable bin size

   Double_t v[numberOfBins+1];
   FillVariableRange(v);

   Double_t c1 = r.Rndm();
   Double_t c2 = r.Rndm();

   TH1D* h1 = new TH1D("m1D1-h1", "h1-Title", numberOfBins, v);
   TH1D* h2 = new TH1D("m1D1-h2", "h2-Title", numberOfBins, v);
   TH1D* h3 = new TH1D("m1D1-h3", "h3=c1*h1*c2*h2", numberOfBins, v);

   h1->Sumw2();h2->Sumw2();h3->Sumw2();

   UInt_t seed = r.GetSeed();
   // For possible problems
   r.SetSeed(seed);
   for ( Int_t e = 0; e < nEvents; ++e ) {
      Double_t value = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      h1->Fill(value, 1.0);
   }

   for ( Int_t e = 0; e < nEvents; ++e ) {
      Double_t value = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      h2->Fill(value,  1.0);
      h3->Fill(value,  c1*c2*h1->GetBinContent( h1->GetXaxis()->FindBin(value) ) );
   }

   // h3 has to be filled again so that the erros are properly calculated
   r.SetSeed(seed);
   for ( Int_t e = 0; e < nEvents; ++e ) {
      Double_t value = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      h3->Fill(value,  c1*c2*h2->GetBinContent( h2->GetXaxis()->FindBin(value) ) );
   }

   // No the bin contents has to be reduced, as it was filled twice!
   for ( Int_t bin = 0; bin <= h3->GetNbinsX() + 1; ++bin ) {
      h3->SetBinContent(bin, h3->GetBinContent(bin) / 2 );
   }

   TH1D* h4 = new TH1D("m1D1-h4", "h4=h1*h2", numberOfBins, v);
   h4->Multiply(h1, h2, c1, c2);

   bool ret = equals("MultiVar1D1", h3, h4, cmpOptStats, 1E-14);
   delete h1;
   delete h2;
   delete h3;
   return ret;
}

bool testMul2() 
{
   // Tests the second Multiply method for 1D Histograms

   TH1D* h1 = new TH1D("m1D2-h1", "h1-Title", numberOfBins, minRange, maxRange);
   TH1D* h2 = new TH1D("m1D2-h2", "h2-Title", numberOfBins, minRange, maxRange);
   TH1D* h3 = new TH1D("m1D2-h3", "h3=h1*h2", numberOfBins, minRange, maxRange);

   h1->Sumw2();h2->Sumw2();h3->Sumw2();

   UInt_t seed = r.GetSeed();
   // For possible problems
   r.SetSeed(seed);
   for ( Int_t e = 0; e < nEvents; ++e ) {
      Double_t value = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      h1->Fill(value, 1.0);
   }

   for ( Int_t e = 0; e < nEvents; ++e ) {
      Double_t value = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      h2->Fill(value,  1.0);
      h3->Fill(value,  h1->GetBinContent( h1->GetXaxis()->FindBin(value) ) );
   }

   r.SetSeed(seed);
   for ( Int_t e = 0; e < nEvents; ++e ) {
      Double_t value = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      h3->Fill(value,  h2->GetBinContent( h2->GetXaxis()->FindBin(value) ) );
   }

   for ( Int_t bin = 0; bin <= h3->GetNbinsX() + 1; ++bin ) {
      h3->SetBinContent(bin, h3->GetBinContent(bin) / 2 );
   }

   h1->Multiply(h2);

   bool ret = equals("Multiply1D2", h3, h1, cmpOptStats, 1E-14);
   delete h2;
   delete h3;
   return ret;
}

bool testMulVar2() 
{
   // Tests the second Multiply method for 1D Histograms with variable bin size

   Double_t v[numberOfBins+1];
   FillVariableRange(v);

   TH1D* h1 = new TH1D("m1D2-h1", "h1-Title", numberOfBins, v);
   TH1D* h2 = new TH1D("m1D2-h2", "h2-Title", numberOfBins, v);
   TH1D* h3 = new TH1D("m1D2-h3", "h3=h1*h2", numberOfBins, v);

   h1->Sumw2();h2->Sumw2();h3->Sumw2();

   UInt_t seed = r.GetSeed();
   // For possible problems
   r.SetSeed(seed);
   for ( Int_t e = 0; e < nEvents; ++e ) {
      Double_t value = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      h1->Fill(value, 1.0);
   }

   for ( Int_t e = 0; e < nEvents; ++e ) {
      Double_t value = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      h2->Fill(value,  1.0);
      h3->Fill(value,  h1->GetBinContent( h1->GetXaxis()->FindBin(value) ) );
   }

   r.SetSeed(seed);
   for ( Int_t e = 0; e < nEvents; ++e ) {
      Double_t value = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      h3->Fill(value,  h2->GetBinContent( h2->GetXaxis()->FindBin(value) ) );
   }

   for ( Int_t bin = 0; bin <= h3->GetNbinsX() + 1; ++bin ) {
      h3->SetBinContent(bin, h3->GetBinContent(bin) / 2 );
   }

   h1->Multiply(h2);

   bool ret = equals("MultiVar1D2", h3, h1, cmpOptStats, 1E-14);
   delete h2;
   delete h3;
   return ret;
}

bool testMul2D1() 
{
   // Tests the first Multiply method for 2D Histograms

   Double_t c1 = r.Rndm();
   Double_t c2 = r.Rndm();

   TH2D* h1 = new TH2D("m2D1-h1", "h1-Title", 
                       numberOfBins, minRange, maxRange,
                       numberOfBins + 2, minRange, maxRange);
   TH2D* h2 = new TH2D("m2D1-h2", "h2-Title", 
                       numberOfBins, minRange, maxRange,
                       numberOfBins + 2, minRange, maxRange);
   TH2D* h3 = new TH2D("m2D1-h3", "h3=c1*h1*c2*h2",
                       numberOfBins, minRange, maxRange,
                       numberOfBins + 2, minRange, maxRange);

   h1->Sumw2();h2->Sumw2();h3->Sumw2();

   UInt_t seed = r.GetSeed();
   // For possible problems
   r.SetSeed(seed);
   for ( Int_t e = 0; e < nEvents * nEvents; ++e ) {
      Double_t x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      h1->Fill(x, y, 1.0);
   }

   for ( Int_t e = 0; e < nEvents * nEvents; ++e ) {
      Double_t x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      h2->Fill(x, y,  1.0);
      h3->Fill(x, y,  c1*c2*h1->GetBinContent( h1->GetXaxis()->FindBin(x),
                                               h1->GetYaxis()->FindBin(y) ) );
   }

   // h3 has to be filled again so that the erros are properly calculated
   r.SetSeed(seed);
   for ( Int_t e = 0; e < nEvents * nEvents; ++e ) {
      Double_t x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      h3->Fill(x, y,  c1*c2*h2->GetBinContent( h2->GetXaxis()->FindBin(x),
                                               h2->GetYaxis()->FindBin(y) ) );
   }

   // No the bin contents has to be reduced, as it was filled twice!
   for ( Int_t i = 0; i <= h3->GetNbinsX() + 1; ++i ) {
      for ( Int_t j = 0; j <= h3->GetNbinsY() + 1; ++j ) {
         h3->SetBinContent(i, j, h3->GetBinContent(i, j) / 2 );
      }
   }

   TH2D* h4 = new TH2D("m2D1-h4", "h4=h1*h2", 
                       numberOfBins, minRange, maxRange,
                       numberOfBins + 2, minRange, maxRange);
   h4->Multiply(h1, h2, c1, c2);

   bool ret = equals("Multiply2D1", h3, h4, cmpOptStats, 1E-12);
   delete h1;
   delete h2;
   delete h3;
   return ret;
}

bool testMul2D2() 
{
   // Tests the second Multiply method for 2D Histograms

   TH2D* h1 = new TH2D("m2D2-h1", "h1-Title", 
                       numberOfBins, minRange, maxRange,
                       numberOfBins + 2, minRange, maxRange);
   TH2D* h2 = new TH2D("m2D2-h2", "h2-Title", 
                       numberOfBins, minRange, maxRange,
                       numberOfBins + 2, minRange, maxRange);
   TH2D* h3 = new TH2D("m2D2-h3", "h3=h1*h2",
                       numberOfBins, minRange, maxRange,
                       numberOfBins + 2, minRange, maxRange);

   h1->Sumw2();h2->Sumw2();h3->Sumw2();

   UInt_t seed = r.GetSeed();
   // For possible problems
   r.SetSeed(seed);
   for ( Int_t e = 0; e < nEvents * nEvents; ++e ) {
      Double_t x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      h1->Fill(x, y, 1.0);
   }

   for ( Int_t e = 0; e < nEvents * nEvents; ++e ) {
      Double_t x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      h2->Fill(x, y,  1.0);
      h3->Fill(x, y,  h1->GetBinContent( h1->GetXaxis()->FindBin(x),
                                         h1->GetYaxis()->FindBin(y) ) );
   }

   // h3 has to be filled again so that the erros are properly calculated
   r.SetSeed(seed);
   for ( Int_t e = 0; e < nEvents * nEvents; ++e ) {
      Double_t x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      h3->Fill(x, y,  h2->GetBinContent( h2->GetXaxis()->FindBin(x),
                                         h2->GetYaxis()->FindBin(y) ) );
   }

   // No the bin contents has to be reduced, as it was filled twice!
   for ( Int_t i = 0; i <= h3->GetNbinsX() + 1; ++i ) {
      for ( Int_t j = 0; j <= h3->GetNbinsY() + 1; ++j ) {
         h3->SetBinContent(i, j, h3->GetBinContent(i, j) / 2 );
      }
   }

   h1->Multiply(h2);

   bool ret = equals("Multiply2D2", h3, h1, cmpOptStats, 1E-12);
   delete h2;
   delete h3;
   return ret;
}

bool testMul3D1() 
{
   // Tests the first Multiply method for 3D Histograms

   Double_t c1 = r.Rndm();
   Double_t c2 = r.Rndm();

   TH3D* h1 = new TH3D("m3D1-h1", "h1-Title", 
                       numberOfBins, minRange, maxRange,
                       numberOfBins + 1, minRange, maxRange,
                       numberOfBins + 2, minRange, maxRange);
   TH3D* h2 = new TH3D("m3D1-h2", "h2-Title", 
                       numberOfBins, minRange, maxRange,
                       numberOfBins + 1, minRange, maxRange,
                       numberOfBins + 2, minRange, maxRange);
   TH3D* h3 = new TH3D("m3D1-h3", "h3=c1*h1*c2*h2",
                       numberOfBins, minRange, maxRange,
                       numberOfBins + 1, minRange, maxRange,
                       numberOfBins + 2, minRange, maxRange);

   h1->Sumw2();h2->Sumw2();h3->Sumw2();

   UInt_t seed = r.GetSeed();
   // For possible problems
   r.SetSeed(seed);
   for ( Int_t e = 0; e < nEvents * nEvents; ++e ) {
      Double_t x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t z = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      h1->Fill(x, y, z, 1.0);
   }

   for ( Int_t e = 0; e < nEvents * nEvents; ++e ) {
      Double_t x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t z = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      h2->Fill(x, y, z,  1.0);
      h3->Fill(x, y, z,  c1*c2*h1->GetBinContent( h1->GetXaxis()->FindBin(x),
                                                  h1->GetYaxis()->FindBin(y),
                                                  h1->GetZaxis()->FindBin(z) ) );
   }

   // h3 has to be filled again so that the erros are properly calculated
   r.SetSeed(seed);
   for ( Int_t e = 0; e < nEvents * nEvents; ++e ) {
      Double_t x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t z = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      h3->Fill(x, y, z,  c1*c2*h2->GetBinContent( h2->GetXaxis()->FindBin(x),
                                                  h2->GetYaxis()->FindBin(y),
                                                  h2->GetZaxis()->FindBin(z) ) );
   }

   // No the bin contents has to be reduced, as it was filled twice!
   for ( Int_t i = 0; i <= h3->GetNbinsX() + 1; ++i ) {
      for ( Int_t j = 0; j <= h3->GetNbinsY() + 1; ++j ) {
         for ( Int_t h = 0; h <= h3->GetNbinsZ() + 1; ++h ) {
            h3->SetBinContent(i, j, h, h3->GetBinContent(i, j, h) / 2 );
         }
      }
   }

   TH3D* h4 = new TH3D("m3D1-h4", "h4=h1*h2", 
                       numberOfBins, minRange, maxRange,
                       numberOfBins + 1, minRange, maxRange,
                       numberOfBins + 2, minRange, maxRange);
   h4->Multiply(h1, h2, c1, c2);

   bool ret = equals("Multiply3D1", h3, h4, cmpOptStats, 1E-13);
   delete h1;
   delete h2;
   delete h3;
   return ret;
}

bool testMul3D2() 
{
   // Tests the second Multiply method for 3D Histograms

   TH3D* h1 = new TH3D("m3D2-h1", "h1-Title", 
                       numberOfBins, minRange, maxRange,
                       numberOfBins + 1, minRange, maxRange,
                       numberOfBins + 2, minRange, maxRange);
   TH3D* h2 = new TH3D("m3D2-h2", "h2-Title", 
                       numberOfBins, minRange, maxRange,
                       numberOfBins + 1, minRange, maxRange,
                       numberOfBins + 2, minRange, maxRange);
   TH3D* h3 = new TH3D("m3D2-h3", "h3=h1*h2",
                       numberOfBins, minRange, maxRange,
                       numberOfBins + 1, minRange, maxRange,
                       numberOfBins + 2, minRange, maxRange);

   h1->Sumw2();h2->Sumw2();h3->Sumw2();

   UInt_t seed = r.GetSeed();
   // For possible problems
   r.SetSeed(seed);
   for ( Int_t e = 0; e < nEvents * nEvents; ++e ) {
      Double_t x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t z = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      h1->Fill(x, y, z, 1.0);
   }

   for ( Int_t e = 0; e < nEvents * nEvents; ++e ) {
      Double_t x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t z = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      h2->Fill(x, y, z,  1.0);
      h3->Fill(x, y, z, h1->GetBinContent( h1->GetXaxis()->FindBin(x),
                                           h1->GetYaxis()->FindBin(y),
                                           h1->GetZaxis()->FindBin(z) ) );
   }

   // h3 has to be filled again so that the errors are properly calculated
   r.SetSeed(seed);
   for ( Int_t e = 0; e < nEvents * nEvents; ++e ) {
      Double_t x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t z = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      h3->Fill(x, y, z, h2->GetBinContent( h2->GetXaxis()->FindBin(x),
                                           h2->GetYaxis()->FindBin(y),
                                           h2->GetZaxis()->FindBin(z) ) );
   }

   // No the bin contents has to be reduced, as it was filled twice!
   for ( Int_t i = 0; i <= h3->GetNbinsX() + 1; ++i ) {
      for ( Int_t j = 0; j <= h3->GetNbinsY() + 1; ++j ) {
         for ( Int_t h = 0; h <= h3->GetNbinsZ() + 1; ++h ) {
            h3->SetBinContent(i, j, h, h3->GetBinContent(i, j, h) / 2 );
         }
      }
   }

   h1->Multiply(h2);

   bool ret = equals("Multiply3D2", h3, h1, cmpOptStats, 1E-13);
   delete h2;
   delete h3;
   return ret;
}

bool testMulSparse()
{
  // Tests the Multiply method for Sparse Histograms

   Int_t bsize[] = { TMath::Nint( r.Uniform(1, 5) ),
                          TMath::Nint( r.Uniform(1, 5) ),
                          TMath::Nint( r.Uniform(1, 5) )};
   Double_t xmin[] = {minRange, minRange, minRange};
   Double_t xmax[] = {maxRange, maxRange, maxRange};

   THnSparseD* s1 = new THnSparseD("m3D2-s1", "s1-Title", 3, bsize, xmin, xmax);
   THnSparseD* s2 = new THnSparseD("m3D2-s2", "s2-Title", 3, bsize, xmin, xmax);
   THnSparseD* s3 = new THnSparseD("m3D2-s3", "s3=s1*s2", 3, bsize, xmin, xmax);

   s1->Sumw2();s2->Sumw2();s3->Sumw2();

   UInt_t seed = r.GetSeed();
   // For possible problems
   r.SetSeed(seed);
   for ( Int_t e = 0; e < nEvents * nEvents; ++e ) {
      Double_t points[3];
      points[0] = r.Uniform( minRange * .9 , maxRange * 1.1 );
      points[1] = r.Uniform( minRange * .9 , maxRange * 1.1 );
      points[2] = r.Uniform( minRange * .9 , maxRange * 1.1 );
      s1->Fill(points, 1.0);
   }

   for ( Int_t e = 0; e < nEvents * nEvents; ++e ) {
      Double_t points[3];
      points[0] = r.Uniform( minRange * .9 , maxRange * 1.1 );
      points[1] = r.Uniform( minRange * .9 , maxRange * 1.1 );
      points[2] = r.Uniform( minRange * .9 , maxRange * 1.1 );
      s2->Fill(points, 1.0);
      Int_t points_s1[3];
      points_s1[0] = s1->GetAxis(0)->FindBin( points[0] );
      points_s1[1] = s1->GetAxis(1)->FindBin( points[1] );
      points_s1[2] = s1->GetAxis(2)->FindBin( points[2] );
      s3->Fill(points, s1->GetBinContent( points_s1 ) );
   }

   // s3 has to be filled again so that the errors are properly calculated
   r.SetSeed(seed);
   for ( Int_t e = 0; e < nEvents * nEvents; ++e ) {
      Double_t points[3];
      points[0] = r.Uniform( minRange * .9 , maxRange * 1.1 );
      points[1] = r.Uniform( minRange * .9 , maxRange * 1.1 );
      points[2] = r.Uniform( minRange * .9 , maxRange * 1.1 );
      Int_t points_s2[3];
      points_s2[0] = s2->GetAxis(0)->FindBin( points[0] );
      points_s2[1] = s2->GetAxis(1)->FindBin( points[1] );
      points_s2[2] = s2->GetAxis(2)->FindBin( points[2] );
      s3->Fill(points, s2->GetBinContent( points_s2 ) );
   }

   // No the bin contents has to be reduced, as it was filled twice!
   for ( Long64_t i = 0; i < s3->GetNbins(); ++i ) {
      Int_t bin[3];
      Double_t v = s3->GetBinContent(i, bin);
      s3->SetBinContent( bin, v / 2 );
   }

   s1->Multiply(s2);

   bool ret = equals("MultSparse", s3, s1, cmpOptNone, 1E-10);
   delete s2;
   delete s3;
   return ret;
}

bool testDivide1() 
{
   // Tests the first Divide method for 1D Histograms

   Double_t c1 = r.Rndm() + 1;
   Double_t c2 = r.Rndm() + 1;

   TH1D* h1 = new TH1D("d1D1-h1", "h1-Title", numberOfBins, minRange, maxRange);
   TH1D* h2 = new TH1D("d1D1-h2", "h2-Title", numberOfBins, minRange, maxRange);

   h1->Sumw2();h2->Sumw2();

   UInt_t seed = r.GetSeed();
   // For possible problems
   r.SetSeed(seed);
   for ( Int_t e = 0; e < nEvents; ++e ) {
      Double_t value;
      value = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      h1->Fill(value, 1.0);
      value = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      h2->Fill(value,  1.0);
   }
   // avoid bins in h2 with zero content
   for (int i = 0; i < h2->GetSize(); ++i) 
      if (h2->GetBinContent(i) == 0) h2->SetBinContent(i,1);


   TH1D* h3 = new TH1D("d1D1-h3", "h3=(c1*h1)/(c2*h2)", numberOfBins, minRange, maxRange);
   h3->Divide(h1, h2, c1, c2);
      
   TH1D* h4 = new TH1D("d1D1-h4", "h4=h3*h2)", numberOfBins, minRange, maxRange);
   h4->Multiply(h2, h3, c2/c1, 1);
   for ( Int_t bin = 0; bin <= h4->GetNbinsX() + 1; ++bin ) {
      Double_t error = h4->GetBinError(bin) * h4->GetBinError(bin);
      error -= (2*(c2*c2)/(c1*c1)) * h3->GetBinContent(bin)*h3->GetBinContent(bin)*h2->GetBinError(bin)*h2->GetBinError(bin); 
      h4->SetBinError( bin, sqrt(error) );
   }
   h4->SetEntries( h4->GetEffectiveEntries() ); 

   h1->ResetStats(); 

   bool ret = equals("Divide1D1", h1, h4, cmpOptStats );
   delete h1;
   delete h2;
   delete h3;
   return ret;
}

bool testDivideVar1() 
{
   // Tests the first Divide method for 1D Histograms with variable bin size

   Double_t v[numberOfBins+1];
   FillVariableRange(v);

   Double_t c1 = r.Rndm() + 1;
   Double_t c2 = r.Rndm() + 1;

   TH1D* h1 = new TH1D("d1D1-h1", "h1-Title", numberOfBins, v);
   TH1D* h2 = new TH1D("d1D1-h2", "h2-Title", numberOfBins, v);

   h1->Sumw2();h2->Sumw2();

   UInt_t seed = r.GetSeed();
   // For possible problems
   r.SetSeed(seed);
   for ( Int_t e = 0; e < nEvents; ++e ) {
      Double_t value;
      value = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      h1->Fill(value, 1.0);
      value = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      h2->Fill(value,  1.0);
   }
   // avoid bins in h2 with zero content
   for (int i = 0; i < h2->GetSize(); ++i) 
      if (h2->GetBinContent(i) == 0) h2->SetBinContent(i,1);


   TH1D* h3 = new TH1D("d1D1-h3", "h3=(c1*h1)/(c2*h2)", numberOfBins, v);
   h3->Divide(h1, h2, c1, c2);
      
   TH1D* h4 = new TH1D("d1D1-h4", "h4=h3*h2)", numberOfBins, v);
   h4->Multiply(h2, h3, c2/c1, 1);
   for ( Int_t bin = 0; bin <= h4->GetNbinsX() + 1; ++bin ) {
      Double_t error = h4->GetBinError(bin) * h4->GetBinError(bin);
      error -= (2*(c2*c2)/(c1*c1)) * h3->GetBinContent(bin)*h3->GetBinContent(bin)*h2->GetBinError(bin)*h2->GetBinError(bin); 
      h4->SetBinError( bin, sqrt(error) );
   }
   h4->SetEntries( h4->GetEffectiveEntries() ); 
   h1->ResetStats(); 

   bool ret = equals("DivideVar1D1", h1, h4, cmpOptStats);
   delete h1;
   delete h2;
   delete h3;
   return ret;
}


bool testDivideProf1() 
{
   // Tests the first Divide method for 1D Profiles

   Double_t c1 = 1;//r.Rndm();
   Double_t c2 = 1;//r.Rndm();

   TProfile* p1 = new TProfile("d1D1-p1", "p1-Title", numberOfBins, minRange, maxRange);
   TProfile* p2 = new TProfile("d1D1-p2", "p2-Title", numberOfBins, minRange, maxRange);

   p1->Sumw2();p2->Sumw2();

   UInt_t seed = r.GetSeed();
   // For possible problems
   r.SetSeed(seed);
   for ( Int_t e = 0; e < nEvents; ++e ) {
      Double_t x, y;
      x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      p1->Fill(x, y, 1.0);
      x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      p2->Fill(x, y, 1.0);
   }


   TProfile* p3 = new TProfile("d1D1-p3", "p3=(c1*p1)/(c2*p2)", numberOfBins, minRange, maxRange);
   p3->Divide(p1, p2, c1, c2);

   // There is no Multiply method to tests. And the errors are wrongly
   // calculated in the TProfile::Division method, so there is no
   // point to make the tests. Once the method is fixed, the tests
   // will be finished.

   return 0;
}

bool testDivide2() 
{
   // Tests the second Divide method for 1D Histograms

   TH1D* h1 = new TH1D("d1D2-h1", "h1-Title", numberOfBins, minRange, maxRange);
   TH1D* h2 = new TH1D("d1D2-h2", "h2-Title", numberOfBins, minRange, maxRange);

   h1->Sumw2();h2->Sumw2();

   UInt_t seed = r.GetSeed();
   // For possible problems
   r.SetSeed(seed);
   for ( Int_t e = 0; e < nEvents; ++e ) {
      Double_t value;
      value = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      h1->Fill(value, 1.0);
      value = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      h2->Fill(value,  1.0);
   }
   // avoid bins in h2 with zero content
   for (int i = 0; i < h2->GetSize(); ++i) 
      if (h2->GetBinContent(i) == 0) h2->SetBinContent(i,1);

   TH1D* h3 = static_cast<TH1D*>( h1->Clone() );
   h3->Divide(h2);
      
   TH1D* h4 = new TH1D("d1D2-h4", "h4=h3*h2)", numberOfBins, minRange, maxRange);
   h4->Multiply(h2, h3, 1.0, 1.0);
   for ( Int_t bin = 0; bin <= h4->GetNbinsX() + 1; ++bin ) {
      Double_t error = h4->GetBinError(bin) * h4->GetBinError(bin);
      error -= 2 * h3->GetBinContent(bin)*h3->GetBinContent(bin)*h2->GetBinError(bin)*h2->GetBinError(bin); 
      h4->SetBinError( bin, sqrt(error) );
   }

   h4->SetEntries( h4->GetEffectiveEntries() ); 
   h1->ResetStats(); 

   bool ret = equals("Divide1D2", h1, h4, cmpOptStats);
   delete h1;
   delete h2;
   delete h3;
   return ret;
}

bool testDivideVar2() 
{
   // Tests the second Divide method for 1D Histograms with variable bin size

   Double_t v[numberOfBins+1];
   FillVariableRange(v);

   TH1D* h1 = new TH1D("d1D2-h1", "h1-Title", numberOfBins, v);
   TH1D* h2 = new TH1D("d1D2-h2", "h2-Title", numberOfBins, v);

   h1->Sumw2();h2->Sumw2();

   UInt_t seed = r.GetSeed();
   // For possible problems
   r.SetSeed(seed);
   for ( Int_t e = 0; e < nEvents; ++e ) {
      Double_t value;
      value = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      h1->Fill(value, 1.0);
      value = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      h2->Fill(value,  1.0);
   }
   // avoid bins in h2 with zero content
   for (int i = 0; i < h2->GetSize(); ++i) 
      if (h2->GetBinContent(i) == 0) h2->SetBinContent(i,1);

   TH1D* h3 = static_cast<TH1D*>( h1->Clone() );
   h3->Divide(h2);
      
   TH1D* h4 = new TH1D("d1D2-h4", "h4=h3*h2)", numberOfBins, v);
   h4->Multiply(h2, h3, 1.0, 1.0);
   for ( Int_t bin = 0; bin <= h4->GetNbinsX() + 1; ++bin ) {
      Double_t error = h4->GetBinError(bin) * h4->GetBinError(bin);
      error -= 2 * h3->GetBinContent(bin)*h3->GetBinContent(bin)*h2->GetBinError(bin)*h2->GetBinError(bin); 
      h4->SetBinError( bin, sqrt(error) );
   }

   h4->SetEntries( h4->GetEffectiveEntries() ); 
   h1->ResetStats(); 

   bool ret = equals("DivideVar1D2", h1, h4, cmpOptStats);
   delete h1;
   delete h2;
   delete h3;
   return ret;
}

bool testDivide2D1() 
{
   // Tests the first Divide method for 2D Histograms

   Double_t c1 = r.Rndm() + 1;
   Double_t c2 = r.Rndm() + 1;

   TH2D* h1 = new TH2D("d2D1-h1", "h1-Title", 
                       numberOfBins, minRange, maxRange,
                       numberOfBins + 2, minRange, maxRange);
   TH2D* h2 = new TH2D("d2D1-h2", "h2-Title", 
                       numberOfBins, minRange, maxRange,
                       numberOfBins + 2, minRange, maxRange);

   h1->Sumw2();h2->Sumw2();

   UInt_t seed = r.GetSeed();
   // For possible problems
   r.SetSeed(seed);
   for ( Int_t e = 0; e < nEvents*nEvents; ++e ) {
      Double_t x,y;
      x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      h1->Fill(x, y, 1.0);
      x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      h2->Fill(x, y, 1.0);
   }
   // avoid bins in h2 with zero content
   for (int i = 0; i < h2->GetSize(); ++i) 
      if (h2->GetBinContent(i) == 0) h2->SetBinContent(i,1);

   TH2D* h3 = new TH2D("d2D1-h3", "h3=(c1*h1)/(c2*h2)", 
                       numberOfBins, minRange, maxRange,
                       numberOfBins + 2, minRange, maxRange);
   h3->Divide(h1, h2, c1, c2);
      
   TH2D* h4 = new TH2D("d2D1-h4", "h4=h3*h2)", 
                       numberOfBins, minRange, maxRange,
                       numberOfBins + 2, minRange, maxRange);
   h4->Multiply(h2, h3, c2/c1, 1);
   for ( Int_t i = 0; i <= h4->GetNbinsX() + 1; ++i ) {
      for ( Int_t j = 0; j <= h4->GetNbinsY() + 1; ++j ) {
         Double_t error = h4->GetBinError(i,j) * h4->GetBinError(i,j);
         error -= (2*(c2*c2)/(c1*c1)) * h3->GetBinContent(i,j)*h3->GetBinContent(i,j)*h2->GetBinError(i,j)*h2->GetBinError(i,j); 
         h4->SetBinError( i, j, sqrt(error) );
      }
   }

   h4->SetEntries( h4->GetEffectiveEntries() ); 
   h1->ResetStats(); 

   bool ret = equals("Divide2D1", h1, h4, cmpOptStats );
   delete h1;
   delete h2;
   delete h3;
   return ret;
}

bool testDivide2D2() 
{
   // Tests the second Divide method for 2D Histograms

   TH2D* h1 = new TH2D("d2D2-h1", "h1-Title", 
                       numberOfBins, minRange, maxRange,
                       numberOfBins + 2, minRange, maxRange);
   TH2D* h2 = new TH2D("d2D2-h2", "h2-Title", 
                       numberOfBins, minRange, maxRange,
                       numberOfBins + 2, minRange, maxRange);

   h1->Sumw2();h2->Sumw2();

   UInt_t seed = r.GetSeed();
   // For possible problems
   r.SetSeed(seed);
   for ( Int_t e = 0; e < nEvents*nEvents; ++e ) {
      Double_t x,y;
      x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      h1->Fill(x, y, 1.0);
      x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      h2->Fill(x, y, 1.0);
   }
   // avoid bins in h2 with zero content
   for (int i = 0; i < h2->GetSize(); ++i) 
      if (h2->GetBinContent(i) == 0) h2->SetBinContent(i,1);

   TH2D* h3 = static_cast<TH2D*>( h1->Clone() );
   h3->Divide(h2);
      
   TH2D* h4 = new TH2D("d2D2-h4", "h4=h3*h2)", 
                       numberOfBins, minRange, maxRange,
                       numberOfBins + 2, minRange, maxRange);
   h4->Multiply(h2, h3, 1.0, 1.0);
   for ( Int_t i = 0; i <= h4->GetNbinsX() + 1; ++i ) {
      for ( Int_t j = 0; j <= h4->GetNbinsY() + 1; ++j ) {
          Double_t error = h4->GetBinError(i,j) * h4->GetBinError(i,j);
         error -= 2 * h3->GetBinContent(i,j)*h3->GetBinContent(i,j)*h2->GetBinError(i,j)*h2->GetBinError(i,j); 
         h4->SetBinError( i, j, sqrt(error) );
      }
   }

   h4->SetEntries( h4->GetEffectiveEntries() ); 
   h1->ResetStats(); 

   bool ret = equals("Divide2D2", h1, h4, cmpOptStats);
   delete h1;
   delete h2;
   delete h3;
   return ret;
}

bool testDivide3D1() 
{
   // Tests the first Divide method for 3D Histograms

   Double_t c1 = r.Rndm() + 1;
   Double_t c2 = r.Rndm() + 1;

   TH3D* h1 = new TH3D("d3D1-h1", "h1-Title", 
                       numberOfBins, minRange, maxRange,
                       numberOfBins + 1, minRange, maxRange,
                       numberOfBins + 2, minRange, maxRange);
   TH3D* h2 = new TH3D("d3D1-h2", "h2-Title", 
                       numberOfBins, minRange, maxRange,
                       numberOfBins + 1, minRange, maxRange,
                       numberOfBins + 2, minRange, maxRange);

   h1->Sumw2();h2->Sumw2();

   UInt_t seed = r.GetSeed();
   // For possible problems
   r.SetSeed(seed);
   for ( Int_t e = 0; e < nEvents*nEvents; ++e ) {
      Double_t x,y,z;
      x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      z = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      h1->Fill(x, y, z, 1.0);
      x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      z = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      h2->Fill(x, y, z, 1.0);
   }
   // avoid bins in h2 with zero content
   for (int i = 0; i < h2->GetSize(); ++i) 
      if (h2->GetBinContent(i) == 0) h2->SetBinContent(i,1);

   TH3D* h3 = new TH3D("d3D1-h3", "h3=(c1*h1)/(c2*h2)", 
                       numberOfBins, minRange, maxRange,
                       numberOfBins + 1, minRange, maxRange,
                       numberOfBins + 2, minRange, maxRange);
   h3->Divide(h1, h2, c1, c2);
      
   TH3D* h4 = new TH3D("d3D1-h4", "h4=h3*h2)", 
                       numberOfBins, minRange, maxRange,
                       numberOfBins + 1, minRange, maxRange,
                       numberOfBins + 2, minRange, maxRange);
   h4->Multiply(h2, h3, c2/c1, 1.0);
   for ( Int_t i = 0; i <= h4->GetNbinsX() + 1; ++i ) {
      for ( Int_t j = 0; j <= h4->GetNbinsY() + 1; ++j ) {
         for ( Int_t h = 0; h <= h4->GetNbinsZ() + 1; ++h ) {
            Double_t error = h4->GetBinError(i,j,h) * h4->GetBinError(i,j,h);
            //error -= 2 * h3->GetBinContent(i,j,h)*h3->GetBinContent(i,j,h)*h2->GetBinError(i,j,h)*h2->GetBinError(i,j,h); 
            error -= (2*(c2*c2)/(c1*c1)) * 
               h3->GetBinContent(i,j,h)*h3->GetBinContent(i,j,h)*h2->GetBinError(i,j,h)*h2->GetBinError(i,j,h); 
            h4->SetBinError( i, j, h, sqrt(error) );
         }
      }
   }

   h4->SetEntries( h4->GetEffectiveEntries() ); 
   h1->ResetStats(); 

   bool ret = equals("Divide3D1", h1, h4, cmpOptStats);
   delete h1;
   delete h2;
   delete h3;
   return ret;
}

bool testDivide3D2() 
{
   // Tests the second Divide method for 3D Histograms

   TH3D* h1 = new TH3D("d3D2-h1", "h1-Title", 
                       numberOfBins, minRange, maxRange,
                       numberOfBins + 1, minRange, maxRange,
                       numberOfBins + 2, minRange, maxRange);
   TH3D* h2 = new TH3D("d3D2-h2", "h2-Title", 
                       numberOfBins, minRange, maxRange,
                       numberOfBins + 1, minRange, maxRange,
                       numberOfBins + 2, minRange, maxRange);

   h1->Sumw2();h2->Sumw2();

   UInt_t seed = r.GetSeed();
   // For possible problems
   r.SetSeed(seed);
   for ( Int_t e = 0; e < nEvents*nEvents; ++e ) {
      Double_t x,y,z;
      x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      z = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      h1->Fill(x, y, z, 1.0);
      x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      z = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      h2->Fill(x, y, z, 1.0);
   }
   // avoid bins in h2 with zero content
   for (int i = 0; i < h2->GetSize(); ++i) 
      if (h2->GetBinContent(i) == 0) h2->SetBinContent(i,1);

   TH3D* h3 = static_cast<TH3D*>( h1->Clone() );
   h3->Divide(h2);
      
   TH3D* h4 = new TH3D("d3D2-h4", "h4=h3*h2)", 
                       numberOfBins, minRange, maxRange,
                       numberOfBins + 1, minRange, maxRange,
                       numberOfBins + 2, minRange, maxRange);
   h4->Multiply(h2, h3, 1.0, 1.0);
   for ( Int_t i = 0; i <= h4->GetNbinsX() + 1; ++i ) {
      for ( Int_t j = 0; j <= h4->GetNbinsY() + 1; ++j ) {
         for ( Int_t h = 0; h <= h4->GetNbinsZ() + 1; ++h ) {
            Double_t error = h4->GetBinError(i,j,h) * h4->GetBinError(i,j,h);
            error -= 2 * h3->GetBinContent(i,j,h)*h3->GetBinContent(i,j,h)*h2->GetBinError(i,j,h)*h2->GetBinError(i,j,h); 
            h4->SetBinError( i, j, h, sqrt(error) );
         }
      }
   }

   h4->SetEntries( h4->GetEffectiveEntries() ); 
   h1->ResetStats(); 

   bool ret = equals("Divide3D2", h1, h4, cmpOptStats);
   delete h1;
   delete h2;
   delete h3;
   return ret;
}

bool testAssign1D()
{
   // Tests the operator=() method for 1D Histograms

   TH1D* h1 = new TH1D("=1D-h1", "h1-Title", numberOfBins, minRange, maxRange);

   h1->Sumw2();

   for ( Int_t e = 0; e < nEvents; ++e ) {
      Double_t value = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      h1->Fill(value, 1.0);
   }

   TH1D* h2 = new TH1D("=1D-h2", "h2-Title", numberOfBins, minRange, maxRange);
   *h2 = *h1;

   bool ret = equals("Assign Oper Hist '='  1D", h1, h2, cmpOptStats);
   delete h1;
   return ret;
}

bool testAssignVar1D()
{
   // Tests the operator=() method for 1D Histograms with variable bin size

   Double_t v[numberOfBins+1];
   FillVariableRange(v);

   TH1D* h1 = new TH1D("=1D-h1", "h1-Title", numberOfBins, v);

   h1->Sumw2();

   for ( Int_t e = 0; e < nEvents; ++e ) {
      Double_t value = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      h1->Fill(value, 1.0);
   }

   TH1D* h2 = new TH1D("=1D-h2", "h2-Title", numberOfBins, v);
   *h2 = *h1;

   bool ret = equals("Assign Oper VarH '='  1D", h1, h2, cmpOptStats);
   delete h1;
   return ret;
}

bool testAssignProfile1D()
{
   // Tests the operator=() method for 1D Profiles

   TProfile* p1 = new TProfile("=1D-p1", "p1-Title", numberOfBins, minRange, maxRange);

   for ( Int_t e = 0; e < nEvents; ++e ) {
      Double_t x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      p1->Fill(x, y, 1.0);
   }

   TProfile* p2 = new TProfile("=1D-p2", "p2-Title", numberOfBins, minRange, maxRange);
   *p2 = *p1;

   bool ret = equals("Assign Oper Prof '='  1D", p1, p2, cmpOptStats);
   delete p1;
   return ret;
}

bool testAssignProfileVar1D()
{
   // Tests the operator=() method for 1D Profiles with variable bin size

   Double_t v[numberOfBins+1];
   FillVariableRange(v);

   TProfile* p1 = new TProfile("=1D-p1", "p1-Title", numberOfBins, v);

   for ( Int_t e = 0; e < nEvents; ++e ) {
      Double_t x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      p1->Fill(x, y, 1.0);
   }

   TProfile* p2 = new TProfile("=1D-p2", "p2-Title", numberOfBins, v);
   *p2 = *p1;

   bool ret = equals("Assign Oper VarP '='  1D", p1, p2, cmpOptStats);
   delete p1;
   return ret;
}

bool testCopyConstructor1D()
{
   // Tests the copy constructor for 1D Histograms

   TH1D* h1 = new TH1D("cc1D-h1", "h1-Title", numberOfBins, minRange, maxRange);

   h1->Sumw2();

   for ( Int_t e = 0; e < nEvents; ++e ) {
      Double_t value = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      h1->Fill(value, 1.0);
   }

   TH1D* h2 = new TH1D(*h1);

   bool ret = equals("Copy Constructor Hist 1D", h1, h2, cmpOptStats);
   delete h1;
   return ret;
}

bool testCopyConstructorVar1D()
{
   // Tests the copy constructor for 1D Histograms with variable bin size

   Double_t v[numberOfBins+1];
   FillVariableRange(v);
   
   TH1D* h1 = new TH1D("cc1D-h1", "h1-Title", numberOfBins, v);

   h1->Sumw2();

   for ( Int_t e = 0; e < nEvents; ++e ) {
      Double_t value = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      h1->Fill(value, 1.0);
   }

   TH1D* h2 = new TH1D(*h1);

   bool ret = equals("Copy Constructor VarH 1D", h1, h2, cmpOptStats);
   delete h1;
   return ret;
}

bool testCopyConstructorProfile1D()
{
   // Tests the copy constructor for 1D Profiles

   TProfile* p1 = new TProfile("cc1D-p1", "p1-Title", numberOfBins, minRange, maxRange);

   for ( Int_t e = 0; e < nEvents; ++e ) {
      Double_t x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      p1->Fill(x, y, 1.0);
   }

   TProfile* p2 = new TProfile(*p1);

   bool ret = equals("Copy Constructor Prof 1D", p1, p2, cmpOptStats);
   delete p1;
   return ret;
}

bool testCopyConstructorProfileVar1D()
{
   // Tests the copy constructor for 1D Profiles with variable bin size

   Double_t v[numberOfBins+1];
   FillVariableRange(v);

   TProfile* p1 = new TProfile("cc1D-p1", "p1-Title", numberOfBins, v);

   for ( Int_t e = 0; e < nEvents; ++e ) {
      Double_t x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      p1->Fill(x, y, 1.0);
   }

   TProfile* p2 = new TProfile(*p1);

   bool ret = equals("Copy Constructor VarP 1D", p1, p2, cmpOptStats);
   delete p1;
   return ret;
}

bool testClone1D()
{
   // Tests the clone method for 1D Histograms

   TH1D* h1 = new TH1D("cl1D-h1", "h1-Title", numberOfBins, minRange, maxRange);

   h1->Sumw2();

   for ( Int_t e = 0; e < nEvents; ++e ) {
      Double_t value = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      h1->Fill(value, 1.0);
   }

   TH1D* h2 = static_cast<TH1D*> ( h1->Clone() );

   bool ret = equals("Clone Function Hist   1D", h1, h2, cmpOptStats);
   delete h1;
   return ret;
}

bool testCloneVar1D()
{
   // Tests the clone method for 1D Histograms with variable bin size

   Double_t v[numberOfBins+1];
   FillVariableRange(v);

   TH1D* h1 = new TH1D("cl1D-h1", "h1-Title", numberOfBins, v);

   h1->Sumw2();

   for ( Int_t e = 0; e < nEvents; ++e ) {
      Double_t value = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      h1->Fill(value, 1.0);
   }

   TH1D* h2 = static_cast<TH1D*> ( h1->Clone() );

   bool ret = equals("Clone Function VarH   1D", h1, h2, cmpOptStats);
   delete h1;
   return ret;
}

bool testCloneProfile1D()
{
   // Tests the clone method for 1D Profiles

   TProfile* p1 = new TProfile("cl1D-p1", "p1-Title", numberOfBins, minRange, maxRange);

   for ( Int_t e = 0; e < nEvents; ++e ) {
      Double_t x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      p1->Fill(x, y, 1.0);
   }

   TProfile* p2 = static_cast<TProfile*> ( p1->Clone() );

   bool ret = equals("Clone Function Prof   1D", p1, p2, cmpOptStats);
   delete p1;
   return ret;
}

bool testCloneProfileVar1D()
{
   // Tests the clone method for 1D Profiles with variable bin size

   Double_t v[numberOfBins+1];
   FillVariableRange(v);

   TProfile* p1 = new TProfile("cl1D-p1", "p1-Title", numberOfBins, v);

   for ( Int_t e = 0; e < nEvents; ++e ) {
      Double_t x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      p1->Fill(x, y, 1.0);
   }

   TProfile* p2 = static_cast<TProfile*> ( p1->Clone() );

   bool ret = equals("Clone Function VarP   1D", p1, p2, cmpOptStats);
   delete p1;
   return ret;
}

bool testAssign2D()
{
   // Tests the operator=() method for 2D Histograms

   TH2D* h1 = new TH2D("=2D-h1", "h1-Title", 
                       numberOfBins, minRange, maxRange,
                       numberOfBins + 2, minRange, maxRange);

   h1->Sumw2();

   for ( Int_t e = 0; e < nEvents * nEvents; ++e ) {
      Double_t x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      h1->Fill(x, y, 1.0);
   }
   
   TH2D* h2 = new TH2D("=2D-h2", "h2-Title", 
                       numberOfBins, minRange, maxRange, 
                       numberOfBins + 2, minRange, maxRange);
   *h2 = *h1;

   bool ret = equals("Assign Oper Hist '='  2D", h1, h2, cmpOptStats);
   delete h1;
   return ret;
}

bool testAssignProfile2D()
{
   // Tests the operator=() method for 2D Profiles

   TProfile2D* p1 = new TProfile2D("=2D-p1", "p1-Title", 
                                   numberOfBins, minRange, maxRange,
                                   numberOfBins + 2, minRange, maxRange);

   for ( Int_t e = 0; e < nEvents * nEvents; ++e ) {
      Double_t x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t z = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      p1->Fill(x, y, z, 1.0);
   }

   TProfile2D* p2 = new TProfile2D("=2D-p2", "p2-Title", 
                                   numberOfBins, minRange, maxRange, 
                                   numberOfBins + 2, minRange, maxRange);
   *p2 = *p1;

   bool ret = equals("Assign Oper Prof '='  2D", p1, p2, cmpOptStats);
   delete p1;
   return ret;
}


bool testCopyConstructor2D()
{
   // Tests the copy constructor for 2D Histograms

   TH2D* h1 = new TH2D("cc2D-h1", "h1-Title", 
                       numberOfBins, minRange, maxRange,
                       numberOfBins + 2, minRange, maxRange);

   h1->Sumw2();

   for ( Int_t e = 0; e < nEvents * nEvents; ++e ) {
      Double_t x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      h1->Fill(x, y, 1.0);
   }

   TH2D* h2 = new TH2D(*h1);

   bool ret = equals("Copy Constructor Hist 2D", h1, h2, cmpOptStats);
   delete h1;
   return ret;
}

bool testCopyConstructorProfile2D()
{
   // Tests the copy constructor for 2D Profiles

   TProfile2D* p1 = new TProfile2D("cc2D-p1", "p1-Title", 
                                   numberOfBins, minRange, maxRange,
                                   numberOfBins + 2, minRange, maxRange);

   for ( Int_t e = 0; e < nEvents * nEvents; ++e ) {
      Double_t x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t z = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      p1->Fill(x, y, z, 1.0);
   }

   TProfile2D* p2 = new TProfile2D(*p1);

   bool ret = equals("Copy Constructor Prof 2D", p1, p2, cmpOptStats);
   delete p1;
   return ret;
}

bool testClone2D()
{
   // Tests the clone method for 2D Histograms

   TH2D* h1 = new TH2D("cl2D-h1", "h1-Title", 
                       numberOfBins, minRange, maxRange,
                       numberOfBins + 2, minRange, maxRange);

   h1->Sumw2();

   for ( Int_t e = 0; e < nEvents * nEvents; ++e ) {
      Double_t x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      h1->Fill(x, y, 1.0);
   }

   TH2D* h2 = static_cast<TH2D*> ( h1->Clone() );

   bool ret = equals("Clone Function Hist   2D", h1, h2, cmpOptStats);
   delete h1;
   return ret;
}

bool testCloneProfile2D()
{
   // Tests the clone method for 2D Profiles

   TProfile2D* p1 = new TProfile2D("cl2D-p1", "p1-Title", 
                                   numberOfBins, minRange, maxRange,
                                   numberOfBins + 2, minRange, maxRange);

   for ( Int_t e = 0; e < nEvents * nEvents; ++e ) {
      Double_t x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t z = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      p1->Fill(x, y, z, 1.0);
   }

   TProfile2D* p2 = static_cast<TProfile2D*> ( p1->Clone() );

   bool ret = equals("Clone Function Prof   2D", p1, p2, cmpOptStats);
   delete p1;
   return ret;
}

bool testAssign3D()
{
   // Tests the operator=() method for 3D Histograms

   TH3D* h1 = new TH3D("=3D-h1", "h1-Title", 
                       numberOfBins, minRange, maxRange,
                       numberOfBins + 1, minRange, maxRange,
                       numberOfBins + 2, minRange, maxRange);

   h1->Sumw2();

   for ( Int_t e = 0; e < nEvents * nEvents; ++e ) {
      Double_t x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t z = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      h1->Fill(x, y, z, 1.0);
   }

   TH3D* h2 = new TH3D("=3D-h2", "h2-Title", 
                       numberOfBins, minRange, maxRange, 
                       numberOfBins + 1, minRange, maxRange, 
                       numberOfBins + 2, minRange, maxRange);
   *h2 = *h1;

   bool ret = equals("Assign Oper Hist '='  3D", h1, h2, cmpOptStats);
   delete h1;
   return ret;
}

bool testAssignProfile3D()
{
   // Tests the operator=() method for 3D Profiles

   TProfile3D* p1 = new TProfile3D("=3D-p1", "p1-Title", 
                                   numberOfBins, minRange, maxRange,
                                   numberOfBins + 1, minRange, maxRange,
                                   numberOfBins + 2, minRange, maxRange);

   for ( Int_t e = 0; e < nEvents * nEvents; ++e ) {
      Double_t x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t z = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t t = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      p1->Fill(x, y, z, t, 1.0);
   }

   TProfile3D* p2 = new TProfile3D("=3D-p2", "p2-Title", 
                                   numberOfBins, minRange, maxRange, 
                                   numberOfBins + 1, minRange, maxRange, 
                                   numberOfBins + 2, minRange, maxRange);
   *p2 = *p1;

   bool ret = equals("Assign Oper Prof '='  3D", p1, p2);
   delete p1;
   return ret;
}

bool testCopyConstructor3D()
{
   // Tests the copy constructor for 3D Histograms

   TH3D* h1 = new TH3D("cc3D-h1", "h1-Title", 
                       numberOfBins, minRange, maxRange,
                       numberOfBins + 1, minRange, maxRange,
                       numberOfBins + 2, minRange, maxRange);

   h1->Sumw2();

   for ( Int_t e = 0; e < nEvents * nEvents; ++e ) {
      Double_t x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t z = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      h1->Fill(x, y, z, 1.0);
   }

   TH3D* h2 = new TH3D(*h1);

   bool ret = equals("Copy Constructor Hist 3D", h1, h2, cmpOptStats);
   delete h1;
   return ret;
}

bool testCopyConstructorProfile3D()
{
   // Tests the copy constructor for 3D Profiles

   TProfile3D* p1 = new TProfile3D("cc3D-p1", "p1-Title", 
                                   numberOfBins, minRange, maxRange,
                                   numberOfBins + 1, minRange, maxRange,
                                   numberOfBins + 2, minRange, maxRange);

   for ( Int_t e = 0; e < nEvents * nEvents; ++e ) {
      Double_t x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t z = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t t = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      p1->Fill(x, y, z, t, 1.0);
   }

   TProfile3D* p2 = new TProfile3D(*p1);

   bool ret = equals("Copy Constructor Prof 3D", p1, p2/*, cmpOptStats*/);
   delete p1;
   return ret;
}

bool testClone3D()
{
   // Tests the clone method for 3D Histograms

   TH3D* h1 = new TH3D("cl3D-h1", "h1-Title", 
                       numberOfBins, minRange, maxRange,
                       numberOfBins + 1, minRange, maxRange,
                       numberOfBins + 2, minRange, maxRange);

   h1->Sumw2();

   for ( Int_t e = 0; e < nEvents * nEvents; ++e ) {
      Double_t x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t z = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      h1->Fill(x, y, z, 1.0);
   }

   TH3D* h2 = static_cast<TH3D*> ( h1->Clone() );

   bool ret = equals("Clone Function Hist   3D", h1, h2, cmpOptStats);
   delete h1;
   return ret;
}

bool testCloneProfile3D()
{
   // Tests the clone method for 3D Profiles

   TProfile3D* p1 = new TProfile3D("cl3D-p1", "p1-Title", 
                       numberOfBins, minRange, maxRange,
                       numberOfBins + 1, minRange, maxRange,
                       numberOfBins + 2, minRange, maxRange);

   for ( Int_t e = 0; e < nEvents * nEvents; ++e ) {
      Double_t x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t z = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t t = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      p1->Fill(x, y, z, t, 1.0);
   }

   TProfile3D* p2 = static_cast<TProfile3D*> ( p1->Clone() );

   bool ret = equals("Clone Function Prof   3D", p1, p2);
   delete p1;
   return ret;
}

bool testCloneSparse()
{
   // Tests the clone method for Sparse histograms

   Int_t bsize[] = { TMath::Nint( r.Uniform(1, 5) ),
                     TMath::Nint( r.Uniform(1, 5) ),
                     TMath::Nint( r.Uniform(1, 5) )
   };
   Double_t xmin[] = {minRange, minRange, minRange};
   Double_t xmax[] = {maxRange, maxRange, maxRange};

   THnSparseD* s1 = new THnSparseD("clS-s1","s1-Title", 3, bsize, xmin, xmax);

   for ( Int_t i = 0; i < nEvents * nEvents; ++i ) {
      Double_t points[3];
      points[0] = r.Uniform( minRange * .9, maxRange * 1.1);
      points[1] = r.Uniform( minRange * .9, maxRange * 1.1);
      points[2] = r.Uniform( minRange * .9, maxRange * 1.1);
      s1->Fill(points);
   }

   THnSparseD* s2 = (THnSparseD*) s1->Clone();

   bool ret = equals("Clone Function THnSparse", s1, s2);
   delete s1;
   return ret;
}

bool testWriteRead1D()
{
   // Tests the write and read methods for 1D Histograms

   TH1D* h1 = new TH1D("wr1D-h1", "h1-Title", numberOfBins, minRange, maxRange);

   h1->Sumw2();

   for ( Int_t e = 0; e < nEvents; ++e ) {
      Double_t value = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      h1->Fill(value, 1.0);
   }

   TFile f("tmpHist.root", "RECREATE");
   h1->Write();
   f.Close();

   TFile f2("tmpHist.root");
   TH1D* h2 = static_cast<TH1D*> ( f2.Get("wr1D-h1") );

   bool ret = equals("Read/Write Hist 1D", h1, h2, cmpOptStats);
   delete h1;
   return ret;
}

bool testWriteReadVar1D()
{
   // Tests the write and read methods for 1D Histograms with variable bin size

   Double_t v[numberOfBins+1];
   FillVariableRange(v);

   TH1D* h1 = new TH1D("wr1D-h1", "h1-Title", numberOfBins, v);

   h1->Sumw2();

   for ( Int_t e = 0; e < nEvents; ++e ) {
      Double_t value = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      h1->Fill(value, 1.0);
   }

   TFile f("tmpHist.root", "RECREATE");
   h1->Write();
   f.Close();

   TFile f2("tmpHist.root");
   TH1D* h2 = static_cast<TH1D*> ( f2.Get("wr1D-h1") );

   bool ret = equals("Read/Write VarH 1D", h1, h2, cmpOptStats);
   delete h1;
   return ret;
}

bool testWriteReadProfile1D()
{
   // Tests the write and read methods for 1D Profiles

   TProfile* p1 = new TProfile("wr1D-p1", "p1-Title", numberOfBins, minRange, maxRange);

   for ( Int_t e = 0; e < nEvents; ++e ) {
      Double_t x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      p1->Fill(x, y, 1.0);
   }

   TFile f("tmpHist.root", "RECREATE");
   p1->Write();
   f.Close();

   TFile f2("tmpHist.root");
   TProfile* p2 = static_cast<TProfile*> ( f2.Get("wr1D-p1") );

   bool ret = equals("Read/Write Prof 1D", p1, p2, cmpOptStats);
   delete p1;
   return ret;
}

bool testWriteReadProfileVar1D()
{
   // Tests the write and read methods for 1D Profiles with variable bin size

   Double_t v[numberOfBins+1];
   FillVariableRange(v);

   TProfile* p1 = new TProfile("wr1D-p1", "p1-Title", numberOfBins, v);

   for ( Int_t e = 0; e < nEvents; ++e ) {
      Double_t x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      p1->Fill(x, y, 1.0);
   }

   TFile f("tmpHist.root", "RECREATE");
   p1->Write();
   f.Close();

   TFile f2("tmpHist.root");
   TProfile* p2 = static_cast<TProfile*> ( f2.Get("wr1D-p1") );

   bool ret = equals("Read/Write VarP 1D", p1, p2, cmpOptStats);
   delete p1;
   return ret;
}

bool testWriteRead2D()
{
   // Tests the write and read methods for 2D Histograms

   TH2D* h1 = new TH2D("wr2D-h1", "h1-Title", 
                       numberOfBins, minRange, maxRange,
                       numberOfBins + 2, minRange, maxRange);

   h1->Sumw2();

   for ( Int_t e = 0; e < nEvents * nEvents; ++e ) {
      Double_t x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      h1->Fill(x, y, 1.0);
   }

   TFile f("tmpHist.root", "RECREATE");
   h1->Write();
   f.Close();

   TFile f2("tmpHist.root");
   TH2D* h2 = static_cast<TH2D*> ( f2.Get("wr2D-h1") );

   bool ret = equals("Read/Write Hist 2D", h1, h2, cmpOptStats);
   delete h1;
   return ret;
}

bool testWriteReadProfile2D()
{
   // Tests the write and read methods for 2D Profiles

   TProfile2D* p1 = new TProfile2D("wr2D-p1", "p1-Title", 
                                   numberOfBins, minRange, maxRange,
                                   numberOfBins + 2, minRange, maxRange);

   for ( Int_t e = 0; e < nEvents * nEvents; ++e ) {
      Double_t x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t z = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      p1->Fill(x, y, z, 1.0);
   }

   TFile f("tmpHist.root", "RECREATE");
   p1->Write();
   f.Close();

   TFile f2("tmpHist.root");
   TProfile2D* p2 = static_cast<TProfile2D*> ( f2.Get("wr2D-p1") );

   bool ret = equals("Read/Write Prof 2D", p1, p2, cmpOptStats);
   delete p1;
   return ret;
}

bool testWriteRead3D()
{
   // Tests the write and read methods for 3D Histograms

   TH3D* h1 = new TH3D("wr3D-h1", "h1-Title", 
                       numberOfBins, minRange, maxRange,
                       numberOfBins + 1, minRange, maxRange,
                       numberOfBins + 2, minRange, maxRange);

   h1->Sumw2();

   for ( Int_t e = 0; e < nEvents * nEvents; ++e ) {
      Double_t x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t z = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      h1->Fill(x, y, z, 1.0);
   }

   TFile f("tmpHist.root", "RECREATE");
   h1->Write();
   f.Close();

   TFile f2("tmpHist.root");
   TH3D* h2 = static_cast<TH3D*> ( f2.Get("wr3D-h1") );

   bool ret = equals("Read/Write Hist 3D", h1, h2, cmpOptStats);
   delete h1;
   return ret;
}

bool testWriteReadProfile3D()
{
   // Tests the write and read methods for 3D Profile

   TProfile3D* p1 = new TProfile3D("wr3D-p1", "p1-Title", 
                                 numberOfBins, minRange, maxRange,
                                 numberOfBins + 1, minRange, maxRange,
                                 numberOfBins + 2, minRange, maxRange);

   for ( Int_t e = 0; e < nEvents * nEvents; ++e ) {
      Double_t x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t z = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t t = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      p1->Fill(x, y, z, t, 1.0);
   }

   TFile f("tmpHist.root", "RECREATE");
   p1->Write();
   f.Close();

   TFile f2("tmpHist.root");
   TProfile3D* p2 = static_cast<TProfile3D*> ( f2.Get("wr3D-p1") );

   // In this particular case the statistics are not checked. The
   // Chi2Test is not properly implemented for the TProfile3D
   // class. If the cmpOptStats flag is set, then there will be a
   // crash.
   bool ret = equals("Read/Write Prof 3D", p1, p2);
   delete p1;
   return ret;
}

bool testWriteReadSparse()
{
   // Tests the write and read methods for Sparse Histograms

   Int_t bsize[] = { TMath::Nint( r.Uniform(1, 5) ),
                     TMath::Nint( r.Uniform(1, 5) ),
                     TMath::Nint( r.Uniform(1, 5) )
   };
   Double_t xmin[] = {minRange, minRange, minRange};
   Double_t xmax[] = {maxRange, maxRange, maxRange};
   
   THnSparseD* s1 = new THnSparseD("wrS-s1","s1-Title", 3, bsize, xmin, xmax);
   s1->Sumw2();

   for ( Int_t i = 0; i < nEvents * nEvents; ++i ) {
      Double_t points[3];
      points[0] = r.Uniform( minRange * .9, maxRange * 1.1);
      points[1] = r.Uniform( minRange * .9, maxRange * 1.1);
      points[2] = r.Uniform( minRange * .9, maxRange * 1.1);
      s1->Fill(points);
   }

   TFile f("tmpHist.root", "RECREATE");
   s1->Write();
   f.Close();

   TFile f2("tmpHist.root");
   THnSparseD* s2 = static_cast<THnSparseD*> ( f2.Get("wrS-s1") );

   bool ret = equals("Read/Write Hist 3D", s1, s2, cmpOptNone);
   delete s1;
   return ret;
}


bool testMerge1D() 
{
   // Tests the merge method for 1D Histograms

   TH1D* h1 = new TH1D("merge1D-h1", "h1-Title", numberOfBins, minRange, maxRange);
   TH1D* h2 = new TH1D("merge1D-h2", "h2-Title", numberOfBins, minRange, maxRange);
   TH1D* h3 = new TH1D("merge1D-h3", "h3-Title", numberOfBins, minRange, maxRange);
   TH1D* h4 = new TH1D("merge1D-h4", "h4-Title", numberOfBins, minRange, maxRange);

   h1->Sumw2();h2->Sumw2();h3->Sumw2();

   FillHistograms(h1, h4);
   FillHistograms(h2, h4);
   FillHistograms(h3, h4);

   TList *list = new TList;
   list->Add(h2);
   list->Add(h3);

   h1->Merge(list);

   bool ret = equals("Merge1D", h1, h4, cmpOptStats, 1E-10);
   delete h1;
   delete h2;
   delete h3;
   return ret;
}

bool testMergeVar1D() 
{
   // Tests the merge method for 1D Histograms with variable bin size

   Double_t v[numberOfBins+1];
   FillVariableRange(v);

   TH1D* h1 = new TH1D("merge1D-h1", "h1-Title", numberOfBins, v);
   TH1D* h2 = new TH1D("merge1D-h2", "h2-Title", numberOfBins, v);
   TH1D* h3 = new TH1D("merge1D-h3", "h3-Title", numberOfBins, v);
   TH1D* h4 = new TH1D("merge1D-h4", "h4-Title", numberOfBins, v);

   h1->Sumw2();h2->Sumw2();h3->Sumw2();

   FillHistograms(h1, h4);
   FillHistograms(h2, h4);
   FillHistograms(h3, h4);

   TList *list = new TList;
   list->Add(h2);
   list->Add(h3);

   h1->Merge(list);

   bool ret = equals("MergeVar1D", h1, h4, cmpOptStats, 1E-10);
   delete h1;
   delete h2;
   delete h3;
   return ret;
}

bool testMergeProf1D() 
{
   // Tests the merge method for 1D Profiles

   TProfile* p1 = new TProfile("merge1D-p1", "p1-Title", numberOfBins, minRange, maxRange);
   TProfile* p2 = new TProfile("merge1D-p2", "p2-Title", numberOfBins, minRange, maxRange);
   TProfile* p3 = new TProfile("merge1D-p3", "p3-Title", numberOfBins, minRange, maxRange);
   TProfile* p4 = new TProfile("merge1D-p4", "p4-Title", numberOfBins, minRange, maxRange);

   FillProfiles(p1, p4);
   FillProfiles(p2, p4);
   FillProfiles(p3, p4);

   TList *list = new TList;
   list->Add(p2);
   list->Add(p3);

   p1->Merge(list);

   bool ret = equals("Merge1DProf", p1, p4, cmpOptStats, 1E-10);
   delete p1;
   delete p2;
   delete p3;
   return ret;
}

bool testMergeProfVar1D() 
{
   // Tests the merge method for 1D Profiles with variable bin size

   Double_t v[numberOfBins+1];
   FillVariableRange(v);

   TProfile* p1 = new TProfile("merge1D-p1", "p1-Title", numberOfBins, v);
   TProfile* p2 = new TProfile("merge1D-p2", "p2-Title", numberOfBins, v);
   TProfile* p3 = new TProfile("merge1D-p3", "p3-Title", numberOfBins, v);
   TProfile* p4 = new TProfile("merge1D-p4", "p4-Title", numberOfBins, v);

   FillProfiles(p1, p4);
   FillProfiles(p2, p4);
   FillProfiles(p3, p4);

   TList *list = new TList;
   list->Add(p2);
   list->Add(p3);

   p1->Merge(list);

   bool ret = equals("Merge1DVarP", p1, p4, cmpOptStats, 1E-10);
   delete p1;
   delete p2;
   delete p3;
   return ret;
}

bool testMerge2D() 
{
   // Tests the merge method for 2D Histograms

   TH2D* h1 = new TH2D("merge2D-h1", "h1-Title",
                       numberOfBins, minRange, maxRange,
                       numberOfBins + 2, minRange, maxRange);
   TH2D* h2 = new TH2D("merge2D-h2", "h2-Title",
                       numberOfBins, minRange, maxRange,
                       numberOfBins + 2, minRange, maxRange);
   TH2D* h3 = new TH2D("merge2D-h3", "h3-Title",
                       numberOfBins, minRange, maxRange,
                       numberOfBins + 2, minRange, maxRange);
   TH2D* h4 = new TH2D("merge2D-h4", "h4-Title",
                       numberOfBins, minRange, maxRange,
                       numberOfBins + 2, minRange, maxRange);

   h1->Sumw2();h2->Sumw2();h3->Sumw2();

   for ( Int_t e = 0; e < nEvents * nEvents; ++e ) {
      Double_t x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      h1->Fill(x, y, 1.0);
      h4->Fill(x, y, 1.0);
   }

   for ( Int_t e = 0; e < nEvents * nEvents; ++e ) {
      Double_t x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      h2->Fill(x, y, 1.0);
      h4->Fill(x, y, 1.0);
   }

   for ( Int_t e = 0; e < nEvents * nEvents; ++e ) {
      Double_t x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      h3->Fill(x, y, 1.0);
      h4->Fill(x, y, 1.0);
   }

   TList *list = new TList;
   list->Add(h2);
   list->Add(h3);

   h1->Merge(list);

   bool ret = equals("Merge2D", h1, h4, cmpOptStats, 1E-10);
   delete h1;
   delete h2;
   delete h3;
   return ret;
}

bool testMergeProf2D() 
{
   // Tests the merge method for 2D Profiles

   TProfile2D* p1 = new TProfile2D("merge2D-p1", "p1-Title",
                                   numberOfBins, minRange, maxRange,
                                   numberOfBins + 2, minRange, maxRange);
   TProfile2D* p2 = new TProfile2D("merge2D-p2", "p2-Title",
                                   numberOfBins, minRange, maxRange,
                                   numberOfBins + 2, minRange, maxRange);
   TProfile2D* p3 = new TProfile2D("merge2D-p3", "p3-Title",
                                   numberOfBins, minRange, maxRange,
                                   numberOfBins + 2, minRange, maxRange);
   TProfile2D* p4 = new TProfile2D("merge2D-p4", "p4-Title",
                                   numberOfBins, minRange, maxRange,
                                   numberOfBins + 2, minRange, maxRange);

   for ( Int_t e = 0; e < nEvents * nEvents; ++e ) {
      Double_t x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t z = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      p1->Fill(x, y, z, 1.0);
      p4->Fill(x, y, z, 1.0);
   }

   for ( Int_t e = 0; e < nEvents * nEvents; ++e ) {
      Double_t x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t z = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      p2->Fill(x, y, z, 1.0);
      p4->Fill(x, y, z, 1.0);
   }

   for ( Int_t e = 0; e < nEvents * nEvents; ++e ) {
      Double_t x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t z = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      p3->Fill(x, y, z, 1.0);
      p4->Fill(x, y, z, 1.0);
   }

   TList *list = new TList;
   list->Add(p2);
   list->Add(p3);

   p1->Merge(list);

   bool ret = equals("Merge2DProf", p1, p4, cmpOptStats, 1E-10);
   delete p1;
   delete p2;
   delete p3;
   return ret;
}

bool testMerge3D() 
{
   // Tests the merge method for 3D Histograms

   TH3D* h1 = new TH3D("merge3D-h1", "h1-Title",
                       numberOfBins, minRange, maxRange,
                       numberOfBins + 1, minRange, maxRange,
                       numberOfBins + 2, minRange, maxRange);
   TH3D* h2 = new TH3D("merge3D-h2", "h2-Title",
                       numberOfBins, minRange, maxRange,
                       numberOfBins + 1, minRange, maxRange,
                       numberOfBins + 2, minRange, maxRange);
   TH3D* h3 = new TH3D("merge3D-h3", "h3-Title",
                       numberOfBins, minRange, maxRange,
                       numberOfBins + 1, minRange, maxRange,
                       numberOfBins + 2, minRange, maxRange);
   TH3D* h4 = new TH3D("merge3D-h4", "h4-Title",
                       numberOfBins, minRange, maxRange,
                       numberOfBins + 1, minRange, maxRange,
                       numberOfBins + 2, minRange, maxRange);

   h1->Sumw2();h2->Sumw2();h3->Sumw2();

   for ( Int_t e = 0; e < nEvents * nEvents; ++e ) {
      Double_t x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t z = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      h1->Fill(x, y, z, 1.0);
      h4->Fill(x, y, z, 1.0);
   }

   for ( Int_t e = 0; e < nEvents * nEvents; ++e ) {
      Double_t x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t z = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      h2->Fill(x, y, z, 1.0);
      h4->Fill(x, y, z, 1.0);
   }

   for ( Int_t e = 0; e < nEvents * nEvents; ++e ) {
      Double_t x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t z = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      h3->Fill(x, y, z, 1.0);
      h4->Fill(x, y, z, 1.0);
   }

   TList *list = new TList;
   list->Add(h2);
   list->Add(h3);

   h1->Merge(list);

   bool ret = equals("Merge3D", h1, h4, cmpOptStats, 1E-10);
   delete h1;
   delete h2;
   delete h3;
   return ret;
}

bool testMergeProf3D() 
{
   // Tests the merge method for 3D Profiles

   TProfile3D* p1 = new TProfile3D("merge3D-p1", "p1-Title",
                                   numberOfBins, minRange, maxRange,
                                   numberOfBins + 1, minRange, maxRange,
                                   numberOfBins + 2, minRange, maxRange);
   TProfile3D* p2 = new TProfile3D("merge3D-p2", "p2-Title",
                                   numberOfBins, minRange, maxRange,
                                   numberOfBins + 1, minRange, maxRange,
                                   numberOfBins + 2, minRange, maxRange);
   TProfile3D* p3 = new TProfile3D("merge3D-p3", "p3-Title",
                                   numberOfBins, minRange, maxRange,
                                   numberOfBins + 1, minRange, maxRange,
                                   numberOfBins + 2, minRange, maxRange);
   TProfile3D* p4 = new TProfile3D("merge3D-p4", "p4-Title",
                                   numberOfBins, minRange, maxRange,
                                   numberOfBins + 1, minRange, maxRange,
                                   numberOfBins + 2, minRange, maxRange);

   for ( Int_t e = 0; e < nEvents * nEvents; ++e ) {
      Double_t x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t z = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t t = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      p1->Fill(x, y, z, t, 1.0);
      p4->Fill(x, y, z, t, 1.0);
   }

   for ( Int_t e = 0; e < nEvents * nEvents; ++e ) {
      Double_t x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t z = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t t = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      p2->Fill(x, y, z, t, 1.0);
      p4->Fill(x, y, z, t, 1.0);
   }

   for ( Int_t e = 0; e < nEvents * nEvents; ++e ) {
      Double_t x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t z = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t t = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      p3->Fill(x, y, z, t, 1.0);
      p4->Fill(x, y, z, t, 1.0);
   }

   TList *list = new TList;
   list->Add(p2);
   list->Add(p3);

   p1->Merge(list);

   bool ret = equals("Merge3DProf", p1, p4, cmpOptStats, 1E-10);
   delete p1;
   delete p2;
   delete p3;
   return ret;
}

bool testMergeSparse() 
{
   // Tests the merge method for Sparse Histograms

   Int_t bsize[] = { TMath::Nint( r.Uniform(1, 5) ),
                     TMath::Nint( r.Uniform(1, 5) ),
                     TMath::Nint( r.Uniform(1, 5) )
   };
   Double_t xmin[] = {minRange, minRange, minRange};
   Double_t xmax[] = {maxRange, maxRange, maxRange};

   THnSparseD* s1 = new THnSparseD("mergeS-s1", "s1-Title", 3, bsize, xmin, xmax);
   THnSparseD* s2 = new THnSparseD("mergeS-s2", "s2-Title", 3, bsize, xmin, xmax);
   THnSparseD* s3 = new THnSparseD("mergeS-s3", "s3-Title", 3, bsize, xmin, xmax);
   THnSparseD* s4 = new THnSparseD("mergeS-s4", "s4-Title", 3, bsize, xmin, xmax);

   s1->Sumw2();s2->Sumw2();s3->Sumw2();

   for ( Int_t e = 0; e < nEvents * nEvents; ++e ) {
      Double_t points[3];
      points[0] = r.Uniform( minRange * .9, maxRange * 1.1);
      points[1] = r.Uniform( minRange * .9, maxRange * 1.1);
      points[2] = r.Uniform( minRange * .9, maxRange * 1.1);
      s1->Fill(points, 1.0);
      s4->Fill(points, 1.0);
   }

   for ( Int_t e = 0; e < nEvents * nEvents; ++e ) {
      Double_t points[3];
      points[0] = r.Uniform( minRange * .9, maxRange * 1.1);
      points[1] = r.Uniform( minRange * .9, maxRange * 1.1);
      points[2] = r.Uniform( minRange * .9, maxRange * 1.1);
      s2->Fill(points, 1.0);
      s4->Fill(points, 1.0);
   }

   for ( Int_t e = 0; e < nEvents * nEvents; ++e ) {
      Double_t points[3];
      points[0] = r.Uniform( minRange * .9, maxRange * 1.1);
      points[1] = r.Uniform( minRange * .9, maxRange * 1.1);
      points[2] = r.Uniform( minRange * .9, maxRange * 1.1);
      s3->Fill(points, 1.0);
      s4->Fill(points, 1.0);
   }

   TList *list = new TList;
   list->Add(s2);
   list->Add(s3);

   s1->Merge(list);

   bool ret = equals("MergeSparse", s1, s4, cmpOptNone, 1E-10);
   delete s1;
   delete s2;
   delete s3;
   return ret;
}

bool testMerge1DLabelSame()
{
   // Tests the merge with some equal labels method for 1D Histograms

   TH1D* h1 = new TH1D("merge1DLabelSame-h1", "h1-Title", numberOfBins, minRange, maxRange);
   TH1D* h2 = new TH1D("merge1DLabelSame-h2", "h2-Title", numberOfBins, minRange, maxRange);
   TH1D* h3 = new TH1D("merge1DLabelSame-h3", "h3-Title", numberOfBins, minRange, maxRange);
   TH1D* h4 = new TH1D("merge1DLabelSame-h4", "h4-Title", numberOfBins, minRange, maxRange);

   h1->GetXaxis()->SetBinLabel(4, "alpha");
   h2->GetXaxis()->SetBinLabel(4, "alpha");
   h3->GetXaxis()->SetBinLabel(4, "alpha");
   h4->GetXaxis()->SetBinLabel(4, "alpha");

   for ( Int_t e = 0; e < nEvents * nEvents; ++e ) {
      Double_t x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      h1->Fill(x, 1.0);
      h4->Fill(x, 1.0);
   }

   for ( Int_t e = 0; e < nEvents * nEvents; ++e ) {
      Double_t x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      h2->Fill(x, 1.0);
      h4->Fill(x, 1.0);
   }

   for ( Int_t e = 0; e < nEvents * nEvents; ++e ) {
      Double_t x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      h3->Fill(x, 1.0);
      h4->Fill(x, 1.0);
   }

   TList *list = new TList;
   list->Add(h2);
   list->Add(h3);

   h1->Merge(list);

   bool ret = equals("MergeLabelSame1D", h1, h4, cmpOptStats, 1E-10);
   delete h1;
   delete h2;
   delete h3;
   return ret;
}

bool testMerge2DLabelSame()
{
   // Tests the merge with some equal labels method for 2D Histograms

   TH2D* h1 = new TH2D("merge2DLabelSame-h1", "h1-Title", 
                       numberOfBins, minRange, maxRange,
                       numberOfBins + 2, minRange, maxRange);
   TH2D* h2 = new TH2D("merge2DLabelSame-h2", "h2-Title",
                       numberOfBins, minRange, maxRange,
                       numberOfBins + 2, minRange, maxRange);
   TH2D* h3 = new TH2D("merge2DLabelSame-h3", "h3-Title", 
                       numberOfBins, minRange, maxRange,
                       numberOfBins + 2, minRange, maxRange);
   TH2D* h4 = new TH2D("merge2DLabelSame-h4", "h4-Title", 
                       numberOfBins, minRange, maxRange,
                       numberOfBins + 2, minRange, maxRange);

   h1->GetXaxis()->SetBinLabel(4, "alpha");
   h2->GetXaxis()->SetBinLabel(4, "alpha");
   h3->GetXaxis()->SetBinLabel(4, "alpha");
   h4->GetXaxis()->SetBinLabel(4, "alpha");

   for ( Int_t e = 0; e < nEvents * nEvents; ++e ) {
      Double_t x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      h1->Fill(x, y, 1.0);
      h4->Fill(x, y, 1.0);
   }

   for ( Int_t e = 0; e < nEvents * nEvents; ++e ) {
      Double_t x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      h2->Fill(x, y, 1.0);
      h4->Fill(x, y, 1.0);
   }

   for ( Int_t e = 0; e < nEvents * nEvents; ++e ) {
      Double_t x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      h3->Fill(x, y, 1.0);
      h4->Fill(x, y, 1.0);
   }

   TList *list = new TList;
   list->Add(h2);
   list->Add(h3);

   h1->Merge(list);

   bool ret = equals("MergeLabelSame2D", h1, h4, cmpOptStats, 1E-10);
   delete h1;
   delete h2;
   delete h3;
   return ret;
}

bool testMerge3DLabelSame()
{
   // Tests the merge with some equal labels method for 3D Histograms

   TH3D* h1 = new TH3D("merge3DLabelSame-h1", "h1-Title", 
                       numberOfBins, minRange, maxRange,
                       numberOfBins + 1, minRange, maxRange,
                       numberOfBins + 2, minRange, maxRange);
   TH3D* h2 = new TH3D("merge3DLabelSame-h2", "h2-Title",
                       numberOfBins, minRange, maxRange,
                       numberOfBins + 1, minRange, maxRange,
                       numberOfBins + 2, minRange, maxRange);
   TH3D* h3 = new TH3D("merge3DLabelSame-h3", "h3-Title", 
                       numberOfBins, minRange, maxRange,
                       numberOfBins + 1, minRange, maxRange,
                       numberOfBins + 2, minRange, maxRange);
   TH3D* h4 = new TH3D("merge3DLabelSame-h4", "h4-Title", 
                       numberOfBins, minRange, maxRange,
                       numberOfBins + 1, minRange, maxRange,
                       numberOfBins + 2, minRange, maxRange);

   h1->GetXaxis()->SetBinLabel(4, "alpha");
   h2->GetXaxis()->SetBinLabel(4, "alpha");
   h3->GetXaxis()->SetBinLabel(4, "alpha");
   h4->GetXaxis()->SetBinLabel(4, "alpha");

   for ( Int_t e = 0; e < nEvents * nEvents; ++e ) {
      Double_t x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t z = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      h1->Fill(x, y, z, 1.0);
      h4->Fill(x, y, z, 1.0);
   }

   for ( Int_t e = 0; e < nEvents * nEvents; ++e ) {
      Double_t x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t z = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      h2->Fill(x, y, z, 1.0);
      h4->Fill(x, y, z, 1.0);
   }

   for ( Int_t e = 0; e < nEvents * nEvents; ++e ) {
      Double_t x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t z = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      h3->Fill(x, y, z, 1.0);
      h4->Fill(x, y, z, 1.0);
   }

   TList *list = new TList;
   list->Add(h2);
   list->Add(h3);

   h1->Merge(list);

   bool ret = equals("MergeLabelSame3D", h1, h4, cmpOptStats, 1E-10);
   delete h1;
   delete h2;
   delete h3;
   return ret;
}

bool testMergeProf1DLabelSame()
{
   // Tests the merge with some equal labels method for 1D Profiles

   TProfile* p1 = new TProfile("merge1DLabelSame-p1", "p1-Title", numberOfBins, minRange, maxRange);
   TProfile* p2 = new TProfile("merge1DLabelSame-p2", "p2-Title", numberOfBins, minRange, maxRange);
   TProfile* p3 = new TProfile("merge1DLabelSame-p3", "p3-Title", numberOfBins, minRange, maxRange);
   TProfile* p4 = new TProfile("merge1DLabelSame-p4", "p4-Title", numberOfBins, minRange, maxRange);

   // It does not work properly! Look, the bins with the same labels
   // are different ones and still the tests passes! This is not
   // consistent with TH1::Merge()
   p1->GetXaxis()->SetBinLabel(4, "alpha");
   p2->GetXaxis()->SetBinLabel(6, "alpha");
   p3->GetXaxis()->SetBinLabel(8, "alpha");
   p4->GetXaxis()->SetBinLabel(4, "alpha");

   for ( Int_t e = 0; e < nEvents * nEvents; ++e ) {
      Double_t x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      p1->Fill(x, y, 1.0);
      p4->Fill(x, y, 1.0);
   }

   for ( Int_t e = 0; e < nEvents * nEvents; ++e ) {
      Double_t x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      p2->Fill(x, y, 1.0);
      p4->Fill(x, y, 1.0);
   }

   for ( Int_t e = 0; e < nEvents * nEvents; ++e ) {
      Double_t x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      p3->Fill(x, y, 1.0);
      p4->Fill(x, y, 1.0);
   }

   TList *list = new TList;
   list->Add(p2);
   list->Add(p3);
   
   p1->Merge(list);

   bool ret = equals("MergeLabelSame1DProf", p1, p4, cmpOptStats, 1E-10);
   delete p1;
   delete p2;
   delete p3;
   return ret;
}

bool testMergeProf2DLabelSame()
{
   // Tests the merge with some equal labels method for 2D Profiles

   TProfile2D* p1 = new TProfile2D("merge2DLabelSame-p1", "p1-Title",
                                   numberOfBins, minRange, maxRange, 
                                   numberOfBins + 2, minRange, maxRange);
   TProfile2D* p2 = new TProfile2D("merge2DLabelSame-p2", "p2-Title",
                                   numberOfBins, minRange, maxRange,
                                   numberOfBins + 2, minRange, maxRange);
   TProfile2D* p3 = new TProfile2D("merge2DLabelSame-p3", "p3-Title", 
                                   numberOfBins, minRange, maxRange,
                                   numberOfBins + 2, minRange, maxRange);
   TProfile2D* p4 = new TProfile2D("merge2DLabelSame-p4", "p4-Title",
                                   numberOfBins, minRange, maxRange, 
                                   numberOfBins + 2, minRange, maxRange);

   // It does not work properly! Look, the bins with the same labels
   // are different ones and still the tests passes! This is not
   // consistent with TH1::Merge()
   p1->GetXaxis()->SetBinLabel(4, "alpha");
   p2->GetXaxis()->SetBinLabel(6, "alpha");
   p3->GetXaxis()->SetBinLabel(8, "alpha");
   p4->GetXaxis()->SetBinLabel(4, "alpha");

   for ( Int_t e = 0; e < nEvents * nEvents; ++e ) {
      Double_t x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t z = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      p1->Fill(x, y, z, 1.0);
      p4->Fill(x, y, z, 1.0);
   }

   for ( Int_t e = 0; e < nEvents * nEvents; ++e ) {
      Double_t x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t z = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      p2->Fill(x, y, z, 1.0);
      p4->Fill(x, y, z, 1.0);
   }

   for ( Int_t e = 0; e < nEvents * nEvents; ++e ) {
      Double_t x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t z = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      p3->Fill(x, y, z, 1.0);
      p4->Fill(x, y, z, 1.0);
   }

   TList *list = new TList;
   list->Add(p2);
   list->Add(p3);
   
   p1->Merge(list);

   bool ret = equals("MergeLabelSame2DProf", p1, p4, cmpOptStats, 1E-10);
   delete p1;
   delete p2;
   delete p3;
   return ret;
}

bool testMergeProf3DLabelSame()
{
   // Tests the merge with some equal labels method for 3D Profiles

   TProfile3D* p1 = new TProfile3D("merge3DLabelSame-p1", "p1-Title",
                                   numberOfBins, minRange, maxRange, 
                                   numberOfBins + 1, minRange, maxRange, 
                                   numberOfBins + 2, minRange, maxRange);
   TProfile3D* p2 = new TProfile3D("merge3DLabelSame-p2", "p2-Title",
                                   numberOfBins, minRange, maxRange,
                                   numberOfBins + 1, minRange, maxRange, 
                                   numberOfBins + 2, minRange, maxRange);
   TProfile3D* p3 = new TProfile3D("merge3DLabelSame-p3", "p3-Title", 
                                   numberOfBins, minRange, maxRange,
                                   numberOfBins + 1, minRange, maxRange, 
                                   numberOfBins + 2, minRange, maxRange);
   TProfile3D* p4 = new TProfile3D("merge3DLabelSame-p4", "p4-Title",
                                   numberOfBins, minRange, maxRange, 
                                   numberOfBins + 1, minRange, maxRange, 
                                   numberOfBins + 2, minRange, maxRange);

   // It does not work properly! Look, the bins with the same labels
   // are different ones and still the tests passes! This is not
   // consistent with TH1::Merge()
   p1->GetXaxis()->SetBinLabel(4, "alpha");
   p2->GetXaxis()->SetBinLabel(6, "alpha");
   p3->GetXaxis()->SetBinLabel(8, "alpha");
   p4->GetXaxis()->SetBinLabel(4, "alpha");

   for ( Int_t e = 0; e < nEvents * nEvents; ++e ) {
      Double_t x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t z = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t t = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      p1->Fill(x, y, z, t, 1.0);
      p4->Fill(x, y, z, t, 1.0);
   }

   for ( Int_t e = 0; e < nEvents * nEvents; ++e ) {
      Double_t x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t z = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t t = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      p2->Fill(x, y, z, t, 1.0);
      p4->Fill(x, y, z, t, 1.0);
   }

   for ( Int_t e = 0; e < nEvents * nEvents; ++e ) {
      Double_t x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t z = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t t = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      p3->Fill(x, y, z, t, 1.0);
      p4->Fill(x, y, z, t, 1.0);
   }

   TList *list = new TList;
   list->Add(p2);
   list->Add(p3);
   
   p1->Merge(list);

   bool ret = equals("MergeLabelSame3DProf", p1, p4, cmpOptStats, 1E-10);
   delete p1;
   delete p2;
   delete p3;
   return ret;  
}

bool testMerge1DLabelDiff()
{
   // Tests the merge with some different labels method for 1D Histograms

   // This test fails, as expected! That is why it is not run in the tests suite.

   TH1D* h1 = new TH1D("merge1DLabelDiff-h1", "h1-Title", numberOfBins, minRange, maxRange);
   TH1D* h2 = new TH1D("merge1DLabelDiff-h2", "h2-Title", numberOfBins, minRange, maxRange);
   TH1D* h3 = new TH1D("merge1DLabelDiff-h3", "h3-Title", numberOfBins, minRange, maxRange);
   TH1D* h4 = new TH1D("merge1DLabelDiff-h4", "h4-Title", numberOfBins, minRange, maxRange);

   h1->GetXaxis()->SetBinLabel(2, "gamma");
   h2->GetXaxis()->SetBinLabel(6, "beta");
   h3->GetXaxis()->SetBinLabel(4, "alpha");
   h4->GetXaxis()->SetBinLabel(4, "alpha");

   for ( Int_t e = 0; e < nEvents * nEvents; ++e ) {
      Double_t x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      h1->Fill(x, 1.0);
      h4->Fill(x, 1.0);
   }

   for ( Int_t e = 0; e < nEvents * nEvents; ++e ) {
      Double_t x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      h2->Fill(x, 1.0);
      h4->Fill(x, 1.0);
   }

   for ( Int_t e = 0; e < nEvents * nEvents; ++e ) {
      Double_t x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      h3->Fill(x, 1.0);
      h4->Fill(x, 1.0);
   }

   TList *list = new TList;
   list->Add(h2);
   list->Add(h3);
   
   h1->Merge(list);

   bool ret = equals("MergeLabelDiff1D", h1, h4, cmpOptStats, 1E-10);
   delete h1;
   delete h2;
   delete h3;
   return ret;
}

bool testMerge2DLabelDiff()
{
   // Tests the merge with some different labels method for 2D Histograms

   // It does not work properly! Look, the bins with the same labels
   // are different ones and still the tests passes! This is not
   // consistent with TH1::Merge()

   TH2D* h1 = new TH2D("merge2DLabelDiff-h1", "h1-Title", 
                       numberOfBins, minRange, maxRange,
                       numberOfBins + 2, minRange, maxRange);
   TH2D* h2 = new TH2D("merge2DLabelDiff-h2", "h2-Title",
                       numberOfBins, minRange, maxRange,
                       numberOfBins + 2, minRange, maxRange);
   TH2D* h3 = new TH2D("merge2DLabelDiff-h3", "h3-Title",
                       numberOfBins, minRange, maxRange,
                       numberOfBins + 2, minRange, maxRange);
   TH2D* h4 = new TH2D("merge2DLabelDiff-h4", "h4-Title",
                       numberOfBins, minRange, maxRange,
                       numberOfBins + 2, minRange, maxRange);

   h1->GetXaxis()->SetBinLabel(2, "gamma");
   h2->GetXaxis()->SetBinLabel(6, "beta");
   h3->GetXaxis()->SetBinLabel(4, "alpha");
   h4->GetXaxis()->SetBinLabel(4, "alpha");

   for ( Int_t e = 0; e < nEvents * nEvents; ++e ) {
      Double_t x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      h1->Fill(x, y, 1.0);
      h4->Fill(x, y, 1.0);
   }

   for ( Int_t e = 0; e < nEvents * nEvents; ++e ) {
      Double_t x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      h2->Fill(x, y, 1.0);
      h4->Fill(x, y, 1.0);
   }

   for ( Int_t e = 0; e < nEvents * nEvents; ++e ) {
      Double_t x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      h3->Fill(x, y, 1.0);
      h4->Fill(x, y, 1.0);
   }

   TList *list = new TList;
   list->Add(h2);
   list->Add(h3);
   
   h1->Merge(list);

   bool ret = equals("MergeLabelDiff2D", h1, h4, cmpOptStats, 1E-10);
   delete h1;
   delete h2;
   delete h3;
   return ret;
}

bool testMerge3DLabelDiff()
{
   // Tests the merge with some different labels method for 3D Histograms

   // It does not work properly! Look, the bins with the same labels
   // are different ones and still the tests passes! This is not
   // consistent with TH1::Merge()

   TH3D* h1 = new TH3D("merge3DLabelDiff-h1", "h1-Title", 
                       numberOfBins, minRange, maxRange,
                       numberOfBins + 1, minRange, maxRange,
                       numberOfBins + 2, minRange, maxRange);
   TH3D* h2 = new TH3D("merge3DLabelDiff-h2", "h2-Title",
                       numberOfBins, minRange, maxRange,
                       numberOfBins + 1, minRange, maxRange,
                       numberOfBins + 2, minRange, maxRange);
   TH3D* h3 = new TH3D("merge3DLabelDiff-h3", "h3-Title",
                       numberOfBins, minRange, maxRange,
                       numberOfBins + 1, minRange, maxRange,
                       numberOfBins + 2, minRange, maxRange);
   TH3D* h4 = new TH3D("merge3DLabelDiff-h4", "h4-Title",
                       numberOfBins, minRange, maxRange,
                       numberOfBins + 1, minRange, maxRange,
                       numberOfBins + 2, minRange, maxRange);

   h1->GetXaxis()->SetBinLabel(2, "gamma");
   h2->GetXaxis()->SetBinLabel(6, "beta");
   h3->GetXaxis()->SetBinLabel(4, "alpha");
   h4->GetXaxis()->SetBinLabel(4, "alpha");

   for ( Int_t e = 0; e < nEvents * nEvents; ++e ) {
      Double_t x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t z = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      h1->Fill(x, y, z, 1.0);
      h4->Fill(x, y, z, 1.0);
   }

   for ( Int_t e = 0; e < nEvents * nEvents; ++e ) {
      Double_t x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t z = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      h2->Fill(x, y, z, 1.0);
      h4->Fill(x, y, z, 1.0);
   }

   for ( Int_t e = 0; e < nEvents * nEvents; ++e ) {
      Double_t x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t z = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      h3->Fill(x, y, z, 1.0);
      h4->Fill(x, y, z, 1.0);
   }

   TList *list = new TList;
   list->Add(h2);
   list->Add(h3);
   
   h1->Merge(list);

   bool ret = equals("MergeLabelDiff3D", h1, h4, cmpOptStats, 1E-10);
   delete h1;
   delete h2;
   delete h3;
   return ret;
}

bool testMergeProf1DLabelDiff()
{
   // Tests the merge with some different labels method for 1D Profiles

   TProfile* p1 = new TProfile("merge1DLabelDiff-p1", "p1-Title", numberOfBins, minRange, maxRange);
   TProfile* p2 = new TProfile("merge1DLabelDiff-p2", "p2-Title", numberOfBins, minRange, maxRange);
   TProfile* p3 = new TProfile("merge1DLabelDiff-p3", "p3-Title", numberOfBins, minRange, maxRange);
   TProfile* p4 = new TProfile("merge1DLabelDiff-p4", "p4-Title", numberOfBins, minRange, maxRange);

   // It does not work properly! Look, the bins with the same labels
   // are different ones and still the tests passes! This is not
   // consistent with TH1::Merge()
   p1->GetXaxis()->SetBinLabel(2, "gamma");
   p2->GetXaxis()->SetBinLabel(6, "beta");
   p3->GetXaxis()->SetBinLabel(4, "alpha");
   p4->GetXaxis()->SetBinLabel(4, "alpha");

   for ( Int_t e = 0; e < nEvents * nEvents; ++e ) {
      Double_t x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      p1->Fill(x, y, 1.0);
      p4->Fill(x, y, 1.0);
   }

   for ( Int_t e = 0; e < nEvents * nEvents; ++e ) {
      Double_t x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      p2->Fill(x, y, 1.0);
      p4->Fill(x, y, 1.0);
   }

   for ( Int_t e = 0; e < nEvents * nEvents; ++e ) {
      Double_t x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      p3->Fill(x, y, 1.0);
      p4->Fill(x, y, 1.0);
   }

   TList *list = new TList;
   list->Add(p2);
   list->Add(p3);
   
   p1->Merge(list);

   bool ret = equals("MergeLabelDiff1DProf", p1, p4, cmpOptStats, 1E-10);
   delete p1;
   delete p2;
   delete p3;
   return ret;
}

bool testMergeProf2DLabelDiff()
{
   // Tests the merge with some different labels method for 2D Profiles

   TProfile2D* p1 = new TProfile2D("merge2DLabelDiff-p1", "p1-Title", 
                                   numberOfBins, minRange, maxRange,
                                   numberOfBins + 2, minRange, maxRange);
   TProfile2D* p2 = new TProfile2D("merge2DLabelDiff-p2", "p2-Title", 
                                   numberOfBins, minRange, maxRange,
                                   numberOfBins + 2, minRange, maxRange);
   TProfile2D* p3 = new TProfile2D("merge2DLabelDiff-p3", "p3-Title",
                                   numberOfBins, minRange, maxRange,
                                   numberOfBins + 2, minRange, maxRange);
   TProfile2D* p4 = new TProfile2D("merge2DLabelDiff-p4", "p4-Title",
                                   numberOfBins, minRange, maxRange,
                                   numberOfBins + 2, minRange, maxRange);

   // It does not work properly! Look, the bins with the same labels
   // are different ones and still the tests passes! This is not
   // consistent with TH1::Merge()
   p1->GetXaxis()->SetBinLabel(2, "gamma");
   p2->GetXaxis()->SetBinLabel(6, "beta");
   p3->GetXaxis()->SetBinLabel(4, "alpha");
   p4->GetXaxis()->SetBinLabel(4, "alpha");

   for ( Int_t e = 0; e < nEvents * nEvents; ++e ) {
      Double_t x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t z = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      p1->Fill(x, y, z, 1.0);
      p4->Fill(x, y, z, 1.0);
   }

   for ( Int_t e = 0; e < nEvents * nEvents; ++e ) {
      Double_t x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t z = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      p2->Fill(x, y, z, 1.0);
      p4->Fill(x, y, z, 1.0);
   }

   for ( Int_t e = 0; e < nEvents * nEvents; ++e ) {
      Double_t x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t z = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      p3->Fill(x, y, z, 1.0);
      p4->Fill(x, y, z, 1.0);
   }

   TList *list = new TList;
   list->Add(p2);
   list->Add(p3);
   
   p1->Merge(list);

   bool ret = equals("MergeLabelDiff2DProf", p1, p4, cmpOptStats, 1E-10);
   delete p1;
   delete p2;
   delete p3;
   return ret;
}

bool testMergeProf3DLabelDiff()
{
   // Tests the merge with some different labels method for 3D Profiles

   TProfile3D* p1 = new TProfile3D("merge3DLabelDiff-p1", "p1-Title", 
                                   numberOfBins, minRange, maxRange,
                                   numberOfBins + 1, minRange, maxRange,
                                   numberOfBins + 2, minRange, maxRange);
   TProfile3D* p2 = new TProfile3D("merge3DLabelDiff-p2", "p2-Title", 
                                   numberOfBins, minRange, maxRange,
                                   numberOfBins + 1, minRange, maxRange,
                                   numberOfBins + 2, minRange, maxRange);
   TProfile3D* p3 = new TProfile3D("merge3DLabelDiff-p3", "p3-Title",
                                   numberOfBins, minRange, maxRange,
                                   numberOfBins + 1, minRange, maxRange,
                                   numberOfBins + 2, minRange, maxRange);
   TProfile3D* p4 = new TProfile3D("merge3DLabelDiff-p4", "p4-Title",
                                   numberOfBins, minRange, maxRange,
                                   numberOfBins + 1, minRange, maxRange,
                                   numberOfBins + 2, minRange, maxRange);

   // It does not work properly! Look, the bins with the same labels
   // are different ones and still the tests passes! This is not
   // consistent with TH1::Merge()
   p1->GetXaxis()->SetBinLabel(2, "gamma");
   p2->GetXaxis()->SetBinLabel(6, "beta");
   p3->GetXaxis()->SetBinLabel(4, "alpha");
   p4->GetXaxis()->SetBinLabel(4, "alpha");

   for ( Int_t e = 0; e < nEvents * nEvents; ++e ) {
      Double_t x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t z = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t t = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      p1->Fill(x, y, z, t, 1.0);
      p4->Fill(x, y, z, t, 1.0);
   }

   for ( Int_t e = 0; e < nEvents * nEvents; ++e ) {
      Double_t x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t z = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t t = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      p2->Fill(x, y, z, t, 1.0);
      p4->Fill(x, y, z, t, 1.0);
   }

   for ( Int_t e = 0; e < nEvents * nEvents; ++e ) {
      Double_t x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t z = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t t = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      p3->Fill(x, y, z, t, 1.0);
      p4->Fill(x, y, z, t, 1.0);
   }

   TList *list = new TList;
   list->Add(p2);
   list->Add(p3);
   
   p1->Merge(list);

   bool ret = equals("MergeLabelDiff3DProf", p1, p4, cmpOptStats, 1E-10);
   delete p1;
   delete p2;
   delete p3;
   return ret;
}

bool testMerge1DLabelAll()
{
   // Tests the merge method with fully equally labelled 1D Histograms

   TH1D* h1 = new TH1D("merge1DLabelAll-h1", "h1-Title", numberOfBins, minRange, maxRange);
   TH1D* h2 = new TH1D("merge1DLabelAll-h2", "h2-Title", numberOfBins, minRange, maxRange);
   TH1D* h3 = new TH1D("merge1DLabelAll-h3", "h3-Title", numberOfBins, minRange, maxRange);
   TH1D* h4 = new TH1D("merge1DLabelAll-h4", "h4-Title", numberOfBins, minRange, maxRange);

   for ( Int_t i = 1; i <= numberOfBins; ++ i) {
      ostringstream name;
      name << (char) ((int) 'a' + i - 1);
      h1->GetXaxis()->SetBinLabel(i, name.str().c_str());
      h2->GetXaxis()->SetBinLabel(i, name.str().c_str());
      h3->GetXaxis()->SetBinLabel(i, name.str().c_str());
      h4->GetXaxis()->SetBinLabel(i, name.str().c_str());
   }

   for ( Int_t e = 0; e < nEvents * nEvents; ++e ) {
      Double_t x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      h1->Fill(x, 1.0);
      h4->Fill(x, 1.0);
   }

   for ( Int_t e = 0; e < nEvents * nEvents; ++e ) {
      Double_t x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      h2->Fill(x, 1.0);
      h4->Fill(x, 1.0);
   }

   for ( Int_t e = 0; e < nEvents * nEvents; ++e ) {
      Double_t x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      h3->Fill(x, 1.0);
      h4->Fill(x, 1.0);
   }

   TList *list = new TList;
   list->Add(h2);
   list->Add(h3);
   
   h1->Merge(list);

   bool ret = equals("MergeLabelAll1D", h1, h4, cmpOptStats, 1E-10);
   delete h1;
   delete h2;
   delete h3;
   return ret;
}

bool testMerge2DLabelAll()
{
   // Tests the merge method with fully equally labelled 2D Histograms

   TH2D* h1 = new TH2D("merge2DLabelAll-h1", "h1-Title", 
                       numberOfBins, minRange, maxRange,
                       numberOfBins + 2, minRange, maxRange);
   TH2D* h2 = new TH2D("merge2DLabelAll-h2", "h2-Title",
                       numberOfBins, minRange, maxRange,
                       numberOfBins + 2, minRange, maxRange);
   TH2D* h3 = new TH2D("merge2DLabelAll-h3", "h3-Title", 
                       numberOfBins, minRange, maxRange,
                       numberOfBins + 2, minRange, maxRange);
   TH2D* h4 = new TH2D("merge2DLabelAll-h4", "h4-Title",
                       numberOfBins, minRange, maxRange,
                       numberOfBins + 2, minRange, maxRange);

   for ( Int_t i = 1; i <= numberOfBins; ++ i) {
      ostringstream name;
      name << (char) ((int) 'a' + i - 1);
      h1->GetXaxis()->SetBinLabel(i, name.str().c_str());
      h2->GetXaxis()->SetBinLabel(i, name.str().c_str());
      h3->GetXaxis()->SetBinLabel(i, name.str().c_str());
      h4->GetXaxis()->SetBinLabel(i, name.str().c_str());
   }

   for ( Int_t e = 0; e < nEvents * nEvents; ++e ) {
      Double_t x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      h1->Fill(x, y, 1.0);
      h4->Fill(x, y, 1.0);
   }

   for ( Int_t e = 0; e < nEvents * nEvents; ++e ) {
      Double_t x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      h2->Fill(x, y, 1.0);
      h4->Fill(x, y, 1.0);
   }

   for ( Int_t e = 0; e < nEvents * nEvents; ++e ) {
      Double_t x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      h3->Fill(x, y, 1.0);
      h4->Fill(x, y, 1.0);
   }

   TList *list = new TList;
   list->Add(h2);
   list->Add(h3);
   
   h1->Merge(list);

   bool ret = equals("MergeLabelAll2D", h1, h4, cmpOptStats, 1E-10);
   delete h1;
   delete h2;
   delete h3;
   return ret;
}

bool testMerge3DLabelAll()
{
   // Tests the merge method with fully equally labelled 3D Histograms

   TH3D* h1 = new TH3D("merge3DLabelAll-h1", "h1-Title", 
                       numberOfBins, minRange, maxRange,
                       numberOfBins + 1, minRange, maxRange,
                       numberOfBins + 2, minRange, maxRange);
   TH3D* h2 = new TH3D("merge3DLabelAll-h2", "h2-Title",
                       numberOfBins, minRange, maxRange,
                       numberOfBins + 1, minRange, maxRange,
                       numberOfBins + 2, minRange, maxRange);
   TH3D* h3 = new TH3D("merge3DLabelAll-h3", "h3-Title", 
                       numberOfBins, minRange, maxRange,
                       numberOfBins + 1, minRange, maxRange,
                       numberOfBins + 2, minRange, maxRange);
   TH3D* h4 = new TH3D("merge3DLabelAll-h4", "h4-Title",
                       numberOfBins, minRange, maxRange,
                       numberOfBins + 1, minRange, maxRange,
                       numberOfBins + 2, minRange, maxRange);

   for ( Int_t i = 1; i <= numberOfBins; ++ i) {
      ostringstream name;
      name << (char) ((int) 'a' + i - 1);
      h1->GetXaxis()->SetBinLabel(i, name.str().c_str());
      h2->GetXaxis()->SetBinLabel(i, name.str().c_str());
      h3->GetXaxis()->SetBinLabel(i, name.str().c_str());
      h4->GetXaxis()->SetBinLabel(i, name.str().c_str());
   }

   for ( Int_t e = 0; e < nEvents * nEvents; ++e ) {
      Double_t x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t z = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      h1->Fill(x, y, z, 1.0);
      h4->Fill(x, y, z, 1.0);
   }

   for ( Int_t e = 0; e < nEvents * nEvents; ++e ) {
      Double_t x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t z = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      h2->Fill(x, y, z, 1.0);
      h4->Fill(x, y, z, 1.0);
   }

   for ( Int_t e = 0; e < nEvents * nEvents; ++e ) {
      Double_t x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t z = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      h3->Fill(x, y, z, 1.0);
      h4->Fill(x, y, z, 1.0);
   }

   TList *list = new TList;
   list->Add(h2);
   list->Add(h3);
   
   h1->Merge(list);

   bool ret = equals("MergeLabelAll3D", h1, h4, cmpOptStats, 1E-10);
   delete h1;
   delete h2;
   delete h3;
   return ret;
}

bool testMergeProf1DLabelAll()
{
   // Tests the merge method with fully equally labelled 1D Profiles

   TProfile* p1 = new TProfile("merge1DLabelAll-p1", "p1-Title", numberOfBins, minRange, maxRange);
   TProfile* p2 = new TProfile("merge1DLabelAll-p2", "p2-Title", numberOfBins, minRange, maxRange);
   TProfile* p3 = new TProfile("merge1DLabelAll-p3", "p3-Title", numberOfBins, minRange, maxRange);
   TProfile* p4 = new TProfile("merge1DLabelAll-p4", "p4-Title", numberOfBins, minRange, maxRange);

   for ( Int_t i = 1; i <= numberOfBins; ++ i) {
      ostringstream name;
      name << (char) ((int) 'a' + i - 1);
      p1->GetXaxis()->SetBinLabel(i, name.str().c_str());
      p2->GetXaxis()->SetBinLabel(i, name.str().c_str());
      p3->GetXaxis()->SetBinLabel(i, name.str().c_str());
      p4->GetXaxis()->SetBinLabel(i, name.str().c_str());
   }

   for ( Int_t e = 0; e < nEvents * nEvents; ++e ) {
      Double_t x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      p1->Fill(x, y, 1.0);
      p4->Fill(x, y, 1.0);
   }

   for ( Int_t e = 0; e < nEvents * nEvents; ++e ) {
      Double_t x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      p2->Fill(x, y, 1.0);
      p4->Fill(x, y, 1.0);
   }

   for ( Int_t e = 0; e < nEvents * nEvents; ++e ) {
      Double_t x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      p3->Fill(x, y, 1.0);
      p4->Fill(x, y, 1.0);
   }

   TList *list = new TList;
   list->Add(p2);
   list->Add(p3);
   
   p1->Merge(list);

   bool ret = equals("MergeLabelAll1DProf", p1, p4, cmpOptStats, 1E-10);
   delete p1;
   delete p2;
   delete p3;
   return ret;
}

bool testMergeProf2DLabelAll()
{
   // Tests the merge method with fully equally labelled 2D Profiles

   TProfile2D* p1 = new TProfile2D("merge2DLabelAll-p1", "p1-Title", 
                                   numberOfBins, minRange, maxRange,
                                   numberOfBins + 2, minRange, maxRange);
   TProfile2D* p2 = new TProfile2D("merge2DLabelAll-p2", "p2-Title",
                                   numberOfBins, minRange, maxRange,
                                   numberOfBins + 2, minRange, maxRange);
   TProfile2D* p3 = new TProfile2D("merge2DLabelAll-p3", "p3-Title",
                                   numberOfBins, minRange, maxRange,
                                   numberOfBins + 2, minRange, maxRange);
   TProfile2D* p4 = new TProfile2D("merge2DLabelAll-p4", "p4-Title",
                                   numberOfBins, minRange, maxRange,
                                   numberOfBins + 2, minRange, maxRange);

   for ( Int_t i = 1; i <= numberOfBins; ++ i) {
      ostringstream name;
      name << (char) ((int) 'a' + i - 1);
      p1->GetXaxis()->SetBinLabel(i, name.str().c_str());
      p2->GetXaxis()->SetBinLabel(i, name.str().c_str());
      p3->GetXaxis()->SetBinLabel(i, name.str().c_str());
      p4->GetXaxis()->SetBinLabel(i, name.str().c_str());
   }

   for ( Int_t e = 0; e < nEvents * nEvents; ++e ) {
      Double_t x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t z = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      p1->Fill(x, y, z, 1.0);
      p4->Fill(x, y, z, 1.0);
   }

   for ( Int_t e = 0; e < nEvents * nEvents; ++e ) {
      Double_t x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t z = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      p2->Fill(x, y, z, 1.0);
      p4->Fill(x, y, z, 1.0);
   }

   for ( Int_t e = 0; e < nEvents * nEvents; ++e ) {
      Double_t x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t z = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      p3->Fill(x, y, z, 1.0);
      p4->Fill(x, y, z, 1.0);
   }

   TList *list = new TList;
   list->Add(p2);
   list->Add(p3);
   
   p1->Merge(list);

   bool ret = equals("MergeLabelAll2DProf", p1, p4, cmpOptStats, 1E-10);
   delete p1;
   delete p2;
   delete p3;
   return ret;
}

bool testMergeProf3DLabelAll()
{
   // Tests the merge method with fully equally labelled 3D Profiles

   TProfile3D* p1 = new TProfile3D("merge3DLabelAll-p1", "p1-Title", 
                                   numberOfBins, minRange, maxRange,
                                   numberOfBins + 1, minRange, maxRange,
                                   numberOfBins + 2, minRange, maxRange);
   TProfile3D* p2 = new TProfile3D("merge3DLabelAll-p2", "p2-Title",
                                   numberOfBins, minRange, maxRange,
                                   numberOfBins + 1, minRange, maxRange,
                                   numberOfBins + 2, minRange, maxRange);
   TProfile3D* p3 = new TProfile3D("merge3DLabelAll-p3", "p3-Title",
                                   numberOfBins, minRange, maxRange,
                                   numberOfBins + 1, minRange, maxRange,
                                   numberOfBins + 2, minRange, maxRange);
   TProfile3D* p4 = new TProfile3D("merge3DLabelAll-p4", "p4-Title",
                                   numberOfBins, minRange, maxRange,
                                   numberOfBins + 1, minRange, maxRange,
                                   numberOfBins + 2, minRange, maxRange);

   for ( Int_t i = 1; i <= numberOfBins; ++ i) {
      ostringstream name;
      name << (char) ((int) 'a' + i - 1);
      p1->GetXaxis()->SetBinLabel(i, name.str().c_str());
      p2->GetXaxis()->SetBinLabel(i, name.str().c_str());
      p3->GetXaxis()->SetBinLabel(i, name.str().c_str());
      p4->GetXaxis()->SetBinLabel(i, name.str().c_str());
   }

   for ( Int_t e = 0; e < nEvents * nEvents; ++e ) {
      Double_t x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t z = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t t = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      p1->Fill(x, y, z, t, 1.0);
      p4->Fill(x, y, z, t, 1.0);
   }

   for ( Int_t e = 0; e < nEvents * nEvents; ++e ) {
      Double_t x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t z = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t t = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      p2->Fill(x, y, z, t, 1.0);
      p4->Fill(x, y, z, t, 1.0);
   }

   for ( Int_t e = 0; e < nEvents * nEvents; ++e ) {
      Double_t x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t z = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t t = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      p3->Fill(x, y, z, t, 1.0);
      p4->Fill(x, y, z, t, 1.0);
   }

   TList *list = new TList;
   list->Add(p2);
   list->Add(p3);
   
   p1->Merge(list);

   bool ret = equals("MergeLabelAll3DProf", p1, p4, cmpOptStats, 1E-10);
   delete p1;
   delete p2;
   delete p3;
   return ret;
}

bool testMerge1DLabelAllDiff()
{
   // Tests the merge method with fully differently labelled 1D Histograms

   // This test fails, as expected! That is why it is not run in the tests suite.

   TH1D* h1 = new TH1D("merge1DLabelAllDiff-h1", "h1-Title", numberOfBins, minRange, maxRange);
   TH1D* h2 = new TH1D("merge1DLabelAllDiff-h2", "h2-Title", numberOfBins, minRange, maxRange);
   TH1D* h3 = new TH1D("merge1DLabelAllDiff-h3", "h3-Title", numberOfBins, minRange, maxRange);
   TH1D* h4 = new TH1D("merge1DLabelAllDiff-h4", "h4-Title", numberOfBins, minRange, maxRange);

   for ( Int_t i = 1; i <= numberOfBins; ++ i) {
      ostringstream name;
      name << (char) ((int) 'a' + i - 1);
      h1->GetXaxis()->SetBinLabel(i, name.str().c_str());
      name << 1;
      h2->GetXaxis()->SetBinLabel(i, name.str().c_str());
      name << 2;
      h3->GetXaxis()->SetBinLabel(i, name.str().c_str());
      name << 3;
      h4->GetXaxis()->SetBinLabel(i, name.str().c_str());
   }

   for ( Int_t e = 0; e < nEvents * nEvents; ++e ) {
      Double_t x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      h1->Fill(x, 1.0);
      h4->Fill(x, 1.0);
   }

   for ( Int_t e = 0; e < nEvents * nEvents; ++e ) {
      Double_t x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      h2->Fill(x, 1.0);
      h4->Fill(x, 1.0);
   }

   for ( Int_t e = 0; e < nEvents * nEvents; ++e ) {
      Double_t x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      h3->Fill(x, 1.0);
      h4->Fill(x, 1.0);
   }

   TList *list = new TList;
   list->Add(h2);
   list->Add(h3);
   
   h1->Merge(list);
   
   bool ret = equals("MergeLabelAllDiff1D", h1, h4, cmpOptStats, 1E-10);
   delete h1;
   delete h2;
   delete h3;
   return ret;
}

bool testMerge2DLabelAllDiff()
{
   // Tests the merge method with fully differently labelled 2D Histograms

   // It does not work properly! Look, the bins with the same labels
   // are different ones and still the tests passes! This is not
   // consistent with TH1::Merge()

   TH2D* h1 = new TH2D("merge2DLabelAllDiff-h1", "h1-Title", 
                       numberOfBins, minRange, maxRange,
                       numberOfBins + 2, minRange, maxRange);
   TH2D* h2 = new TH2D("merge2DLabelAllDiff-h2", "h2-Title",
                       numberOfBins, minRange, maxRange,
                       numberOfBins + 2, minRange, maxRange);
   TH2D* h3 = new TH2D("merge2DLabelAllDiff-h3", "h3-Title",
                       numberOfBins, minRange, maxRange,
                       numberOfBins + 2, minRange, maxRange);
   TH2D* h4 = new TH2D("merge2DLabelAllDiff-h4", "h4-Title",
                       numberOfBins, minRange, maxRange, 
                       numberOfBins + 2, minRange, maxRange);

   for ( Int_t i = 1; i <= numberOfBins; ++ i) {
      ostringstream name;
      name << (char) ((int) 'a' + i - 1);
      h1->GetXaxis()->SetBinLabel(i, name.str().c_str());
      h1->GetYaxis()->SetBinLabel(i, name.str().c_str());
      name << 1;
      h2->GetXaxis()->SetBinLabel(i, name.str().c_str());
      h2->GetYaxis()->SetBinLabel(i, name.str().c_str());
      name << 2;
      h3->GetXaxis()->SetBinLabel(i, name.str().c_str());
      h3->GetYaxis()->SetBinLabel(i, name.str().c_str());
      name << 3;
      h4->GetXaxis()->SetBinLabel(i, name.str().c_str());
   }

   for ( Int_t e = 0; e < nEvents * nEvents; ++e ) {
      Double_t x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      h1->Fill(x, y, 1.0);
      h4->Fill(x, y, 1.0);
   }

   for ( Int_t e = 0; e < nEvents * nEvents; ++e ) {
      Double_t x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      h2->Fill(x, y, 1.0);
      h4->Fill(x, y, 1.0);
   }

   for ( Int_t e = 0; e < nEvents * nEvents; ++e ) {
      Double_t x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      h3->Fill(x, y, 1.0);
      h4->Fill(x, y, 1.0);
   }

   TList *list = new TList;
   list->Add(h2);
   list->Add(h3);
   
   h1->Merge(list);

   bool ret = equals("MergeLabelAllDiff2D", h1, h4, cmpOptStats, 1E-10);
   delete h1;
   delete h2;
   delete h3;
   return ret;
}

bool testMerge3DLabelAllDiff()
{
   // Tests the merge method with fully differently labelled 3D Histograms

   // It does not work properly! Look, the bins with the same labels
   // are different ones and still the tests passes! This is not
   // consistent with TH1::Merge()

   TH3D* h1 = new TH3D("merge3DLabelAllDiff-h1", "h1-Title", 
                       numberOfBins, minRange, maxRange,
                       numberOfBins + 1, minRange, maxRange,
                       numberOfBins + 2, minRange, maxRange);
   TH3D* h2 = new TH3D("merge3DLabelAllDiff-h2", "h2-Title",
                       numberOfBins, minRange, maxRange,
                       numberOfBins + 1, minRange, maxRange,
                       numberOfBins + 2, minRange, maxRange);
   TH3D* h3 = new TH3D("merge3DLabelAllDiff-h3", "h3-Title",
                       numberOfBins, minRange, maxRange,
                       numberOfBins + 1, minRange, maxRange,
                       numberOfBins + 2, minRange, maxRange);
   TH3D* h4 = new TH3D("merge3DLabelAllDiff-h4", "h4-Title",
                       numberOfBins, minRange, maxRange,
                       numberOfBins + 1, minRange, maxRange, 
                       numberOfBins + 2, minRange, maxRange);

   for ( Int_t i = 1; i <= numberOfBins; ++ i) {
      ostringstream name;
      name << (char) ((int) 'a' + i - 1);
      h1->GetXaxis()->SetBinLabel(i, name.str().c_str());
      h1->GetYaxis()->SetBinLabel(i, name.str().c_str());
      h1->GetZaxis()->SetBinLabel(i, name.str().c_str());
      name << 1;
      h2->GetXaxis()->SetBinLabel(i, name.str().c_str());
      h2->GetYaxis()->SetBinLabel(i, name.str().c_str());
      h2->GetZaxis()->SetBinLabel(i, name.str().c_str());
      name << 2;
      h3->GetXaxis()->SetBinLabel(i, name.str().c_str());
      h3->GetYaxis()->SetBinLabel(i, name.str().c_str());
      h3->GetZaxis()->SetBinLabel(i, name.str().c_str());
      name << 3;
      h4->GetXaxis()->SetBinLabel(i, name.str().c_str());
      h4->GetYaxis()->SetBinLabel(i, name.str().c_str());
      h4->GetZaxis()->SetBinLabel(i, name.str().c_str());
   }

   for ( Int_t e = 0; e < nEvents * nEvents; ++e ) {
      Double_t x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t z = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      h1->Fill(x, y, z, 1.0);
      h4->Fill(x, y, z, 1.0);
   }

   for ( Int_t e = 0; e < nEvents * nEvents; ++e ) {
      Double_t x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t z = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      h2->Fill(x, y, z, 1.0);
      h4->Fill(x, y, z, 1.0);
   }

   for ( Int_t e = 0; e < nEvents * nEvents; ++e ) {
      Double_t x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t z = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      h3->Fill(x, y, z, 1.0);
      h4->Fill(x, y, z, 1.0);
   }

   TList *list = new TList;
   list->Add(h2);
   list->Add(h3);
   
   h1->Merge(list);

   bool ret = equals("MergeLabelAllDiff3D", h1, h4, cmpOptStats, 1E-10);
   delete h1;
   delete h2;
   delete h3;
   return ret;  
}

bool testMergeProf1DLabelAllDiff()
{
   // Tests the merge method with fully differently labelled 1D Profiles

   TProfile* p1 = new TProfile("merge1DLabelAllDiff-p1", "p1-Title", numberOfBins, minRange, maxRange);
   TProfile* p2 = new TProfile("merge1DLabelAllDiff-p2", "p2-Title", numberOfBins, minRange, maxRange);
   TProfile* p3 = new TProfile("merge1DLabelAllDiff-p3", "p3-Title", numberOfBins, minRange, maxRange);
   TProfile* p4 = new TProfile("merge1DLabelAllDiff-p4", "p4-Title", numberOfBins, minRange, maxRange);

   // It does not work properly! Look, the bins with the same labels
   // are different ones and still the tests passes! This is not
   // consistent with TH1::Merge()
   for ( Int_t i = 1; i <= numberOfBins; ++ i) {
      ostringstream name;
      name << (char) ((int) 'a' + i - 1);
      p1->GetXaxis()->SetBinLabel(i, name.str().c_str());
      name << 1;
      p2->GetXaxis()->SetBinLabel(i, name.str().c_str());
      name << 2;
      p3->GetXaxis()->SetBinLabel(i, name.str().c_str());
      name << 3;
      p4->GetXaxis()->SetBinLabel(i, name.str().c_str());
   }

   for ( Int_t e = 0; e < nEvents * nEvents; ++e ) {
      Double_t x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      p1->Fill(x, y, 1.0);
      p4->Fill(x, y, 1.0);
   }

   for ( Int_t e = 0; e < nEvents * nEvents; ++e ) {
      Double_t x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      p2->Fill(x, y, 1.0);
      p4->Fill(x, y, 1.0);
   }

   for ( Int_t e = 0; e < nEvents * nEvents; ++e ) {
      Double_t x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      p3->Fill(x, y, 1.0);
      p4->Fill(x, y, 1.0);
   }

   TList *list = new TList;
   list->Add(p2);
   list->Add(p3);
   
   p1->Merge(list);
   
   bool ret = equals("MergeLabelAllDiff1DProf", p1, p4, cmpOptStats, 1E-10);
   delete p1;
   delete p2;
   delete p3;
   return ret;
}

bool testMergeProf2DLabelAllDiff()
{
   // Tests the merge method with fully differently labelled 2D Profiles

   TProfile2D* p1 = new TProfile2D("merge2DLabelAllDiff-p1", "p1-Title", 
                                   numberOfBins, minRange, maxRange,
                                   numberOfBins + 2, minRange, maxRange);
   TProfile2D* p2 = new TProfile2D("merge2DLabelAllDiff-p2", "p2-Title",
                                   numberOfBins, minRange, maxRange,
                                   numberOfBins + 2, minRange, maxRange);
   TProfile2D* p3 = new TProfile2D("merge2DLabelAllDiff-p3", "p3-Title",
                                   numberOfBins, minRange, maxRange,
                                   numberOfBins + 2, minRange, maxRange);
   TProfile2D* p4 = new TProfile2D("merge2DLabelAllDiff-p4", "p4-Title",
                                   numberOfBins, minRange, maxRange,
                                   numberOfBins + 2, minRange, maxRange);

   // It does not work properly! Look, the bins with the same labels
   // are different ones and still the tests passes! This is not
   // consistent with TH1::Merge()
   for ( Int_t i = 1; i <= numberOfBins; ++ i) {
      ostringstream name;
      name << (char) ((int) 'a' + i - 1);
      p1->GetXaxis()->SetBinLabel(i, name.str().c_str());
      p1->GetYaxis()->SetBinLabel(i, name.str().c_str());
      name << 1;
      p2->GetXaxis()->SetBinLabel(i, name.str().c_str());
      p2->GetYaxis()->SetBinLabel(i, name.str().c_str());
      name << 2;
      p3->GetXaxis()->SetBinLabel(i, name.str().c_str());
      p3->GetYaxis()->SetBinLabel(i, name.str().c_str());
      name << 3;
      p4->GetXaxis()->SetBinLabel(i, name.str().c_str());
      p4->GetYaxis()->SetBinLabel(i, name.str().c_str());
   }

   for ( Int_t e = 0; e < nEvents * nEvents; ++e ) {
      Double_t x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t z = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      p1->Fill(x, y, z, 1.0);
      p4->Fill(x, y, z, 1.0);
   }

   for ( Int_t e = 0; e < nEvents * nEvents; ++e ) {
      Double_t x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t z = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      p2->Fill(x, y, z, 1.0);
      p4->Fill(x, y, z, 1.0);
   }

   for ( Int_t e = 0; e < nEvents * nEvents; ++e ) {
      Double_t x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t z = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      p3->Fill(x, y, z, 1.0);
      p4->Fill(x, y, z, 1.0);
   }

   TList *list = new TList;
   list->Add(p2);
   list->Add(p3);
   
   p1->Merge(list);
   
   bool ret = equals("MergeLabelAllDiff2DProf", p1, p4, cmpOptStats, 1E-10);
   delete p1;
   delete p2;
   delete p3;
   return ret;
}

bool testMergeProf3DLabelAllDiff()
{
   // Tests the merge method with fully differently labelled 3D Profiles

   TProfile3D* p1 = new TProfile3D("merge3DLabelAllDiff-p1", "p1-Title", 
                                   numberOfBins, minRange, maxRange,
                                   numberOfBins + 1, minRange, maxRange,
                                   numberOfBins + 2, minRange, maxRange);
   TProfile3D* p2 = new TProfile3D("merge3DLabelAllDiff-p2", "p2-Title",
                                   numberOfBins, minRange, maxRange,
                                   numberOfBins + 1, minRange, maxRange,
                                   numberOfBins + 2, minRange, maxRange);
   TProfile3D* p3 = new TProfile3D("merge3DLabelAllDiff-p3", "p3-Title",
                                   numberOfBins, minRange, maxRange,
                                   numberOfBins + 1, minRange, maxRange,
                                   numberOfBins + 2, minRange, maxRange);
   TProfile3D* p4 = new TProfile3D("merge3DLabelAllDiff-p4", "p4-Title",
                                   numberOfBins, minRange, maxRange,
                                   numberOfBins + 1, minRange, maxRange,
                                   numberOfBins + 2, minRange, maxRange);

   // It does not work properly! Look, the bins with the same labels
   // are different ones and still the tests passes! This is not
   // consistent with TH1::Merge()
   for ( Int_t i = 1; i <= numberOfBins; ++ i) {
      ostringstream name;
      name << (char) ((int) 'a' + i - 1);
      p1->GetXaxis()->SetBinLabel(i, name.str().c_str());
      p1->GetYaxis()->SetBinLabel(i, name.str().c_str());
      p1->GetZaxis()->SetBinLabel(i, name.str().c_str());
      name << 1;
      p2->GetXaxis()->SetBinLabel(i, name.str().c_str());
      p2->GetYaxis()->SetBinLabel(i, name.str().c_str());
      p2->GetZaxis()->SetBinLabel(i, name.str().c_str());
      name << 2;
      p3->GetXaxis()->SetBinLabel(i, name.str().c_str());
      p3->GetYaxis()->SetBinLabel(i, name.str().c_str());
      p3->GetZaxis()->SetBinLabel(i, name.str().c_str());
      name << 3;
      p4->GetXaxis()->SetBinLabel(i, name.str().c_str());
      p4->GetYaxis()->SetBinLabel(i, name.str().c_str());
      p4->GetZaxis()->SetBinLabel(i, name.str().c_str());
   }

   for ( Int_t e = 0; e < nEvents * nEvents; ++e ) {
      Double_t x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t z = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t t = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      p1->Fill(x, y, z, t, 1.0);
      p4->Fill(x, y, z, t, 1.0);
   }

   for ( Int_t e = 0; e < nEvents * nEvents; ++e ) {
      Double_t x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t z = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t t = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      p2->Fill(x, y, z, t, 1.0);
      p4->Fill(x, y, z, t, 1.0);
   }

   for ( Int_t e = 0; e < nEvents * nEvents; ++e ) {
      Double_t x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t z = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t t = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      p3->Fill(x, y, z, t, 1.0);
      p4->Fill(x, y, z, t, 1.0);
   }

   TList *list = new TList;
   list->Add(p2);
   list->Add(p3);
   
   p1->Merge(list);
   
   bool ret = equals("MergeLabelAllDiff3DProf", p1, p4, cmpOptStats, 1E-10);
   delete p1;
   delete p2;
   delete p3;
   return ret;
}

bool testMerge1DDiff() 
{
   // Tests the merge method with different binned 1D Histograms

   TH1D *h1 = new TH1D("merge1DDiff-h1","h1-Title",110,-110,0);
   TH1D *h2 = new TH1D("merge1DDiff-h2","h2-Title",220,0,110);
   TH1D *h3 = new TH1D("merge1DDiff-h3","h3-Title",330,-55,55);
   TH1D *h4 = new TH1D("merge1DDiff-h4","h4-Title",220,-110,110);

   h1->Sumw2();h2->Sumw2();h3->Sumw2();h4->Sumw2();

   for ( Int_t e = 0; e < nEvents; ++e ) {
      Double_t value = r.Gaus(-55,10);
      h1->Fill(value, 1.0);
      h4->Fill(value, 1.0);
   }

   for ( Int_t e = 0; e < nEvents; ++e ) {
      Double_t value = r.Gaus(55,10);
      h2->Fill(value, 1.0);
      h4->Fill(value, 1.0);
   }

   for ( Int_t e = 0; e < nEvents; ++e ) {
      Double_t value = r.Gaus(0,10);
      h3->Fill(value, 1.0);
      h4->Fill(value, 1.0);
   }

   TList *list = new TList;
   list->Add(h2);
   list->Add(h3);

   h1->Merge(list);

   bool ret = equals("MergeDiff1D", h1, h4, cmpOptStats, 1E-10);
   delete h1;
   delete h2;
   delete h3;
   return ret;
}

bool testMerge2DDiff() 
{
   // Tests the merge method with different binned 2D Histograms

   TH2D *h1 = new TH2D("merge2DDiff-h1","h1-Title",
                       110,-110,0,
                       110,-110,0);
   TH2D *h2 = new TH2D("merge2DDiff-h2","h2-Title",
                       220,0,110,
                       220,0,110);
   TH2D *h3 = new TH2D("merge2DDiff-h3","h3-Title",
                       330,-55,55,
                       330,-55,55);
   TH2D *h4 = new TH2D("merge2DDiff-h4","h4-Title",
                       220,-110,110,
                       220,-110,110);

   h1->Sumw2();h2->Sumw2();h3->Sumw2();h4->Sumw2();

   for ( Int_t e = 0; e < nEvents; ++e ) {
      Double_t x = r.Gaus(-55,10);
      Double_t y = r.Gaus(-55,10);
      h1->Fill(x, y, 1.0);
      h4->Fill(x, y, 1.0);
   }

   for ( Int_t e = 0; e < nEvents; ++e ) {
      Double_t x = r.Gaus(55,10);
      Double_t y = r.Gaus(55,10);
      h2->Fill(x, y, 1.0);
      h4->Fill(x, y, 1.0);
   }

   for ( Int_t e = 0; e < nEvents; ++e ) {
      Double_t x = r.Gaus(0,10);
      Double_t y = r.Gaus(0,10);
      h3->Fill(x, y, 1.0);
      h4->Fill(x, y, 1.0);
   }

   TList *list = new TList;
   list->Add(h2);
   list->Add(h3);

   h1->Merge(list);

   bool ret = equals("MergeDiff2D", h1, h4, cmpOptStats, 1E-10);
   delete h1;
   delete h2;
   delete h3;
   return ret;
}

bool testMerge3DDiff() 
{
   // Tests the merge method with different binned 3D Histograms

   // This tests fails! It should not!
   TH3D *h1 = new TH3D("merge3DDiff-h1","h1-Title",
                       110,-110,0,
                       110,-110,0,
                       110,-110,0);
   TH3D *h2 = new TH3D("merge3DDiff-h2","h2-Title",
                       220,0,110,
                       220,0,110,
                       220,0,110);
   TH3D *h3 = new TH3D("merge3DDiff-h3","h3-Title",
                       330,-55,55,
                       330,-55,55,
                       330,-55,55);
   TH3D *h4 = new TH3D("merge3DDiff-h4","h4-Title",
                       220,-110,110,
                       220,-110,110,
                       220,-110,110);

   h1->Sumw2();h2->Sumw2();h3->Sumw2();h4->Sumw2();

   for ( Int_t e = 0; e < nEvents * nEvents; ++e ) {
      Double_t x = r.Gaus(-55,10);
      Double_t y = r.Gaus(-55,10);
      Double_t z = r.Gaus(-55,10);
      h1->Fill(x, y, z, 1.0);
      h4->Fill(x, y, z, 1.0);
   }

   for ( Int_t e = 0; e < nEvents * nEvents; ++e ) {
      Double_t x = r.Gaus(55,10);
      Double_t y = r.Gaus(55,10);
      Double_t z = r.Gaus(55,10);
      h2->Fill(x, y, z, 1.0);
      h4->Fill(x, y, z, 1.0);
   }

   for ( Int_t e = 0; e < nEvents * nEvents; ++e ) {
      Double_t x = r.Gaus(0,10);
      Double_t y = r.Gaus(55,10);
      Double_t z = r.Gaus(0,10);
      h3->Fill(x, y, z, 1.0);
      h4->Fill(x, y, z, 1.0);
   }

   TList *list = new TList;
   list->Add(h2);
   list->Add(h3);

   h1->Merge(list);

   bool ret = equals("MergeDiff3D", h1, h4, cmpOptStats, 1E-10);
   delete h1;
   delete h2;
   delete h3;
   return ret;
}

bool testMergeProf1DDiff() 
{
   // Tests the merge method with different binned 1D Profile

   // Stats fail, for a reason I do not know :S

   TProfile *p1 = new TProfile("merge1DDiff-p1","p1-Title",110,-110,0);
   TProfile *p2 = new TProfile("merge1DDiff-p2","p2-Title",220,0,110);
   TProfile *p3 = new TProfile("merge1DDiff-p3","p3-Title",330,-55,55);
   TProfile *p4 = new TProfile("merge1DDiff-p4","p4-Title",220,-110,110);

   for ( Int_t e = 0; e < nEvents; ++e ) {
      Double_t x = r.Gaus(-55,10);
      Double_t y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      p1->Fill(x, y, 1.0);
      p4->Fill(x, y, 1.0);
   }

   for ( Int_t e = 0; e < nEvents; ++e ) {
      Double_t x = r.Gaus(55,10);
      Double_t y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      p2->Fill(x, y, 1.0);
      p4->Fill(x, y, 1.0);
   }

   for ( Int_t e = 0; e < nEvents; ++e ) {
      Double_t x = r.Gaus(0,10);
      Double_t y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      p3->Fill(x, y, 1.0);
      p4->Fill(x, y, 1.0);
   }

   TList *list = new TList;
   list->Add(p2);
   list->Add(p3);

   p1->Merge(list);

   bool ret = equals("MergeDiff1DProf", p1, p4, cmpOptNone , 1E-10);
   delete p1;
   delete p2;
   delete p3;
   return ret;
}

bool testMergeProf2DDiff() 
{
   // Tests the merge method with different binned 2D Profile

   // This tests fails! It should not!
   TProfile2D *p1 = new TProfile2D("merge2DDiff-p1","p1-Title",
                                   110,-110,0,
                                   110,-110,0);
   TProfile2D *p2 = new TProfile2D("merge2DDiff-p2","p2-Title",
                                   220,0,110,
                                   220,0,110);
   TProfile2D *p3 = new TProfile2D("merge2DDiff-p3","p3-Title",
                                   330,-55,55,
                                   330,-55,55);
   TProfile2D *p4 = new TProfile2D("merge2DDiff-p4","p4-Title",
                                   220,-110,110,
                                   220,-110,110);

   for ( Int_t e = 0; e < nEvents; ++e ) {
      Double_t x = r.Gaus(-55,10);
      Double_t y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t z = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      p1->Fill(x, y, z, 1.0);
      p4->Fill(x, y, z, 1.0);
   }

   for ( Int_t e = 0; e < nEvents; ++e ) {
      Double_t x = r.Gaus(55,10);
      Double_t y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t z = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      p2->Fill(x, y, z, 1.0);
      p4->Fill(x, y, z, 1.0);
   }

   for ( Int_t e = 0; e < nEvents; ++e ) {
      Double_t x = r.Gaus(0,10);
      Double_t y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t z = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      p3->Fill(x, y, z, 1.0);
      p4->Fill(x, y, z, 1.0);
   }

   TList *list = new TList;
   list->Add(p2);
   list->Add(p3);

   p1->Merge(list);

   bool ret = equals("MergeDiff2DProf", p1, p4, cmpOptStats, 1E-10);
   delete p1;
   delete p2;
   delete p3;
   return ret;
}

bool testMergeProf3DDiff() 
{
   // Tests the merge method with different binned 3D Profile

   // This tests fails! Segmentation Fault!!It should not!
   TProfile3D *p1 = new TProfile3D("merge3DDiff-p1","p1-Title",
                                   110,-110,0,
                                   110,-110,0,
                                   110,-110,0);
   TProfile3D *p2 = new TProfile3D("merge3DDiff-p2","p2-Title",
                                   220,0,110,
                                   220,0,110,
                                   220,0,110);
   TProfile3D *p3 = new TProfile3D("merge3DDiff-p3","p3-Title",
                                   330,-55,55,
                                   330,-55,55,
                                   330,-55,55);
   TProfile3D *p4 = new TProfile3D("merge3DDiff-p4","p4-Title",
                                   220,-110,110,
                                   220,-110,110,
                                   220,-110,110);

   for ( Int_t e = 0; e < nEvents; ++e ) {
      Double_t x = r.Gaus(-55,10);
      Double_t y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t z = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t t = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      p1->Fill(x, y, z, t, 1.0);
      p4->Fill(x, y, z, t, 1.0);
   }

   for ( Int_t e = 0; e < nEvents; ++e ) {
      Double_t x = r.Gaus(55,10);
      Double_t y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t z = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t t = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      p2->Fill(x, y, z, t, 1.0);
      p4->Fill(x, y, z, t, 1.0);
   }

   for ( Int_t e = 0; e < nEvents; ++e ) {
      Double_t x = r.Gaus(0,10);
      Double_t y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t z = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t t = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      p3->Fill(x, y, z, t, 1.0);
      p4->Fill(x, y, z, t, 1.0);
   }

   TList *list = new TList;
   list->Add(p2);
   list->Add(p3);

   p1->Merge(list);

   bool ret = equals("MergeDiff3DProf", p1, p4, cmpOptStats, 1E-10);
   delete p1;
   delete p2;
   delete p3;
   return ret;
}

bool testLabel()
{
   // Tests labelling a 1D Histogram

   TH1D* h1 = new TH1D("lD1-h1", "h1-Title", numberOfBins, minRange, maxRange);
   TH1D* h2 = new TH1D("lD1-h2", "h2-Title", numberOfBins, minRange, maxRange);

   for ( Int_t bin = 1; bin <= h1->GetNbinsX() ; ++bin ) {
      ostringstream label;
      label << bin;
      h2->GetXaxis()->SetBinLabel(bin, label.str().c_str());
   }

   for ( Int_t e = 0; e < nEvents; ++e ) {
      Double_t value = r.Uniform(minRange, maxRange);
      Int_t bin = h1->GetXaxis()->FindBin(value);
      h1->Fill(h1->GetXaxis()->GetBinCenter(bin), 1.0);

      ostringstream label;
      label << bin;
      h2->Fill(label.str().c_str(), 1.0);
   }

   bool status = equals("Fill(char*)", h1, h2, cmpOptStats, 1E-13);
   delete h1;
   return status;
}

bool testLabelsInflateProf1D()
{
   // Tests labelling a 1D Profile

   Int_t numberOfInflates=4;
   Int_t numberOfFills = numberOfBins;
   Double_t maxRangeInflate = maxRange;
   for ( Int_t i = 0; i < numberOfInflates; ++i )
   {
      numberOfFills *= 2;
      maxRangeInflate = 2*maxRangeInflate - 1;
   }
      

   TProfile* p1 = new TProfile("tLI1D-p1", "p1-Title", numberOfBins, minRange, maxRange);
   TProfile* p2 = new TProfile("tLI1D-p2", "p2-Title", numberOfFills, minRange, maxRangeInflate);

   p1->GetXaxis()->SetTimeDisplay(1);

   for ( Int_t e = 0; e < numberOfFills; ++e ) {
      Double_t x = e;
      Double_t y = sin(x/10);

      p1->SetBinContent(int(x+0.5)+1, y );
      p1->SetBinEntries(int(x+0.5)+1, 10.0);

      p2->SetBinContent(int(x+0.5)+1, y );
      p2->SetBinEntries(int(x+0.5)+1, 10.0);
   }

   bool ret = equals("LabelsInflateProf1D", p1, p2);
   delete p1;
   return ret;
}

Double_t function1D(Double_t x)
{
   Double_t a = -1.8;

   return a * x;
}

bool testInterpolation1D() 
{
   // Tests interpolation method for 1D Histogram

   bool status = false;

   TH1D* h1 = new TH1D("h1", "h1", 
                       numberOfBins, minRange, maxRange);
   
   h1->Reset();

   for ( Int_t nbinsx = 1; nbinsx <= h1->GetXaxis()->GetNbins(); ++nbinsx ) {
      Double_t x = h1->GetXaxis()->GetBinCenter(nbinsx);
      h1->Fill(x, function1D(x));
   }
   
   int itest = 0;
   for (itest = 0; itest < 1000; ++itest) { 
      double xp = r.Uniform( h1->GetXaxis()->GetBinCenter(1), h1->GetXaxis()->GetBinCenter(numberOfBins) ); 
      
      double ip = h1->Interpolate(xp); 
     
      if (  fabs(ip  - function1D(xp) ) > 1.E-13*fabs(ip) ) {
         status = true;
         cout << "x: " << xp 
              << " h3->Inter: " << ip
              << " functionD: " << function1D(xp)
              << " diff: " << fabs(ip  - function1D(xp))
              << endl;
      }
   }

   delete h1;
   if ( defaultEqualOptions & cmpOptPrint ) cout << "testInterpolation1D: \t" << (status?"FAILED":"OK") << endl;
   return status; 
}

bool testInterpolationVar1D() 
{
   // Tests interpolation method for 1D Histogram with variable bin size

   Double_t v[numberOfBins+1];
   FillVariableRange(v);

   bool status = false;

   TH1D* h1 = new TH1D("h1", "h1", numberOfBins, v);
   
   h1->Reset();

   for ( Int_t nbinsx = 1; nbinsx <= h1->GetXaxis()->GetNbins(); ++nbinsx ) {
      Double_t x = h1->GetXaxis()->GetBinCenter(nbinsx);
      h1->Fill(x, function1D(x));
   }
   
   int itest = 0;
   for (itest = 0; itest < 1000; ++itest) { 
      double xp = r.Uniform( h1->GetXaxis()->GetBinCenter(1), h1->GetXaxis()->GetBinCenter(numberOfBins) ); 
      
      double ip = h1->Interpolate(xp); 
     
      if (  fabs(ip  - function1D(xp) ) > 1.E-13*fabs(ip) ) {
         status = true;
         cout << "x: " << xp 
              << " h3->Inter: " << ip
              << " functionD: " << function1D(xp)
              << " diff: " << fabs(ip  - function1D(xp))
              << endl;
      }
   }

   delete h1;
   if ( defaultEqualOptions & cmpOptPrint ) cout << "testInterpolaVar1D: \t" << (status?"FAILED":"OK") << endl;
   return status; 
}

Double_t function2D(Double_t x, Double_t y)
{
   Double_t a = -2.1;
   Double_t b = 0.6;

   return a * x + b * y;
}

bool testInterpolation2D()
{
   // Tests interpolation method for 2D Histogram

   bool status = false;

   TH2D* h1 = new TH2D("h1", "h1", 
                       numberOfBins, minRange, maxRange,
                       2*numberOfBins, minRange, maxRange);
   
   h1->Reset();

   for ( Int_t nbinsx = 1; nbinsx <= h1->GetXaxis()->GetNbins(); ++nbinsx )
      for ( Int_t nbinsy = 1; nbinsy <= h1->GetYaxis()->GetNbins(); ++nbinsy ) {
            Double_t x = h1->GetXaxis()->GetBinCenter(nbinsx);
            Double_t y = h1->GetYaxis()->GetBinCenter(nbinsy);
            h1->Fill(x, y, function2D(x, y));
         }
   
   int itest = 0;
   for (itest = 0; itest < 1000; ++itest) { 

      double xp = r.Uniform( h1->GetXaxis()->GetBinCenter(1), h1->GetXaxis()->GetBinCenter(numberOfBins) ); 
      double yp = r.Uniform( h1->GetYaxis()->GetBinCenter(1), h1->GetYaxis()->GetBinCenter(numberOfBins) );
      
      double ip = h1->Interpolate(xp, yp); 
     
      if (  fabs(ip  - function2D(xp, yp) ) > 1.E-13*fabs(ip) ) {
         status = true;
         cout << "x: " << xp << " y: " << yp
              << " h3->Inter: " << ip
              << " function: " << function2D(xp, yp)
              << " diff: " << fabs(ip  - function2D(xp, yp))
              << endl;
      }
   }

   delete h1;
   if ( defaultEqualOptions & cmpOptPrint ) cout << "testInterpolation2D: \t" << (status?"FAILED":"OK") << endl;
   return status; 
}

Double_t function3D(Double_t x, Double_t y, Double_t z)
{

   Double_t a = 0.3;
   Double_t b = 6;
   Double_t c = -2;

   return a * x + b * y + c * z;
}

bool testInterpolation3D()
{
   // Tests interpolation method for 3D Histogram

   bool status = false;
   TH3D* h1 = new TH3D("h1", "h1", 
                       numberOfBins, minRange, maxRange,
                       2*numberOfBins, minRange, maxRange,
                       4*numberOfBins, minRange, maxRange);
   
   h1->Reset();

   for ( Int_t nbinsx = 1; nbinsx <= h1->GetXaxis()->GetNbins(); ++nbinsx )
      for ( Int_t nbinsy = 1; nbinsy <= h1->GetYaxis()->GetNbins(); ++nbinsy )
         for ( Int_t nbinsz = 1; nbinsz <= h1->GetZaxis()->GetNbins(); ++nbinsz ) {
            Double_t x = h1->GetXaxis()->GetBinCenter(nbinsx);
            Double_t y = h1->GetYaxis()->GetBinCenter(nbinsy);
            Double_t z = h1->GetZaxis()->GetBinCenter(nbinsz);
            h1->Fill(x, y, z, function3D(x, y, z));
         }

   
   int itest = 0;
   for (itest = 0; itest < 1000; ++itest) { 
      double xp = r.Uniform( h1->GetXaxis()->GetBinCenter(1), h1->GetXaxis()->GetBinCenter(numberOfBins) ); 
      double yp = r.Uniform( h1->GetYaxis()->GetBinCenter(1), h1->GetYaxis()->GetBinCenter(numberOfBins) );
      double zp = r.Uniform( h1->GetZaxis()->GetBinCenter(1), h1->GetZaxis()->GetBinCenter(numberOfBins) );
      
      double ip = h1->Interpolate(xp, yp, zp); 
      
      if (  fabs(ip  - function3D(xp, yp, zp) ) > 1.E-15*fabs(ip) ) 
         status = true;
   }

   delete h1;

   if ( defaultEqualOptions & cmpOptPrint ) cout << "testInterpolation3D: \t" << (status?"FAILED":"OK") << endl;

   return status; 
}

bool testScale1DProf()
{
   TProfile* p1 = new TProfile("scD1-p1", "p1-Title", numberOfBins, minRange, maxRange);
   TProfile* p2 = new TProfile("scD1-p2", "p2=c1*p1", numberOfBins, minRange, maxRange);

   Double_t c1 = r.Rndm();

   for ( Int_t e = 0; e < nEvents; ++e ) {
      Double_t x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      p1->Fill(x,      y, 1.0);
      p2->Fill(x, c1 * y, 1.0);
   }

   p1->Scale(c1);

   int status = equals("testScale Prof 1D", p1, p2, cmpOptStats);
   delete p1;
   return status;
}

bool testScale2DProf()
{
   TProfile2D* p1 = new TProfile2D("scD2-p1", "p1", 
                                   numberOfBins, minRange, maxRange,
                                   numberOfBins + 2, minRange, maxRange);

   TProfile2D* p2 = new TProfile2D("scD2-p2", "p2=c1*p1", 
                                   numberOfBins, minRange, maxRange,
                                   numberOfBins + 2, minRange, maxRange);
   Double_t c1 = r.Rndm();

   for ( Int_t e = 0; e < nEvents * nEvents; ++e ) {
      Double_t x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t z = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      p1->Fill(x, y, z     , 1.0);
      p2->Fill(x, y, c1 * z, 1.0);
   }

   p1->Scale(c1);

   int status = equals("testScale Prof 2D", p1, p2, cmpOptStats);
   delete p1;
   return status;
}

bool testScale3DProf()
{
   TProfile3D* p1 = new TProfile3D("scD3-p1", "p1", 
                                   numberOfBins, minRange, maxRange,
                                   numberOfBins + 1, minRange, maxRange,
                                   numberOfBins + 2, minRange, maxRange);

   TProfile3D* p2 = new TProfile3D("scD3-p2", "p2=c1*p1", 
                                   numberOfBins, minRange, maxRange,
                                   numberOfBins + 1, minRange, maxRange,
                                   numberOfBins + 2, minRange, maxRange);
   Double_t c1 = r.Rndm();

   for ( Int_t e = 0; e < nEvents * nEvents; ++e ) {
      Double_t x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t z = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t t = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      p1->Fill(x, y, z, t     , 1.0);
      p2->Fill(x, y, z, c1 * t, 1.0);
   }

   p1->Scale(c1);

   int status = equals("testScale Prof 3D", p1, p2, cmpOptStats);
   delete p1;
   return status;
}

bool testRefRead1D()
{
   // Tests consistency with a reference file for 1D Histogram

   TH1D* h1 = 0;
   bool ret = 0;
   if ( refFileOption == refFileWrite ) {
      h1 = new TH1D("rr1D-h1", "h1-Title", numberOfBins, minRange, maxRange);
      h1->Sumw2();
   
      for ( Int_t e = 0; e < nEvents; ++e ) {
         Double_t value = r.Uniform(0.9 * minRange, 1.1 * maxRange);
         h1->Fill(value, 1.0);
      }  
      h1->Write();
   } else {
      h1 = static_cast<TH1D*> ( refFile->Get("rr1D-h1") );
      TH1D* h2 = new TH1D("rr1D-h2", "h2-Title", numberOfBins, minRange, maxRange);
      h2->Sumw2();
   
      for ( Int_t e = 0; e < nEvents; ++e ) {
         Double_t value = r.Uniform(0.9 * minRange, 1.1 * maxRange);
         h2->Fill(value, 1.0);
      }  

      ret = equals("Ref Read Hist 1D", h1, h2, cmpOptStats);
   }
   if ( h1 ) delete h1;
   return ret;

}

bool testRefReadProf1D()
{
   // Tests consistency with a reference file for 1D Profile

   bool ret = 0;
   TProfile* p1 = 0;
   if ( refFileOption == refFileWrite ) {
      p1 = new TProfile("rr1D-p1", "p1-Title", numberOfBins, minRange, maxRange);
//      p1->Sumw2();
   
      for ( Int_t e = 0; e < nEvents; ++e ) {
         Double_t x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
         Double_t y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
         p1->Fill(x, y, 1.0);
      }  
      p1->Write();
   } else {
      TH1::SetDefaultSumw2(false);
      p1 = static_cast<TProfile*> ( refFile->Get("rr1D-p1") );
      TProfile* p2 = new TProfile("rr1D-p2", "p2-Title", numberOfBins, minRange, maxRange);
//      p2->Sumw2();
   
      for ( Int_t e = 0; e < nEvents; ++e ) {
         Double_t x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
         Double_t y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
         p2->Fill(x, y, 1.0);
      }  

      ret = equals("Ref Read Prof 1D", p1, p2, cmpOptStats);
      TH1::SetDefaultSumw2(true);
   }
   if (p1) delete p1;
   return ret;

}

bool testRefRead2D()
{
   // Tests consistency with a reference file for 2D Histogram

   TH2D* h1 = 0;
   bool ret = 0;
   if ( refFileOption == refFileWrite ) {
      h1 = new TH2D("rr2D-h1", "h1-Title", 
                          numberOfBins, minRange, maxRange,
                          numberOfBins, minRange, maxRange);
      h1->Sumw2();
   
      for ( Int_t e = 0; e < nEvents * nEvents; ++e ) {
         Double_t x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
         Double_t y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
         h1->Fill(x, y, 1.0);
      }  
      h1->Write();
   } else {
      h1 = static_cast<TH2D*> ( refFile->Get("rr2D-h1") );
      TH2D* h2 = new TH2D("rr2D-h2", "h2-Title", 
                          numberOfBins, minRange, maxRange,
                          numberOfBins, minRange, maxRange);
      h2->Sumw2();
   
      for ( Int_t e = 0; e < nEvents * nEvents; ++e ) {
         Double_t x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
         Double_t y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
         h2->Fill(x, y, 1.0);
      }  

      ret = equals("Ref Read Hist 2D", h1, h2, cmpOptStats);
   }
   if ( h1 ) delete h1;
   return ret;
}

bool testRefReadProf2D()
{
   // Tests consistency with a reference file for 2D Profile

   TProfile2D* p1 = 0;
   bool ret = 0;
   if ( refFileOption == refFileWrite ) {
      p1 = new TProfile2D("rr2D-p1", "p1-Title", 
                                      numberOfBins, minRange, maxRange,
                                      numberOfBins, minRange, maxRange);
   
      for ( Int_t e = 0; e < nEvents * nEvents; ++e ) {
         Double_t x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
         Double_t y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
         Double_t z = r.Uniform(0.9 * minRange, 1.1 * maxRange);
         p1->Fill(x, y, z, 1.0);
      }  
      p1->Write();
   } else {
      p1 = static_cast<TProfile2D*> ( refFile->Get("rr2D-p1") );
      TProfile2D* p2 = new TProfile2D("rr2D-p2", "p2-Title", 
                                      numberOfBins, minRange, maxRange,
                                      numberOfBins, minRange, maxRange);
   
      for ( Int_t e = 0; e < nEvents * nEvents; ++e ) {
         Double_t x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
         Double_t y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
         Double_t z = r.Uniform(0.9 * minRange, 1.1 * maxRange);
         p2->Fill(x, y, z, 1.0);
      }  

      ret = equals("Ref Read Prof 2D", p1, p2, cmpOptStats );
   }
   if ( p1 ) delete p1;
   return ret;
}

bool testRefRead3D()
{
   // Tests consistency with a reference file for 3D Histogram

   TH3D* h1 = 0;
   bool ret = 0;
   if ( refFileOption == refFileWrite ) {
      h1 = new TH3D("rr3D-h1", "h1-Title", 
                          numberOfBins, minRange, maxRange,
                          numberOfBins, minRange, maxRange,
                          numberOfBins, minRange, maxRange);
      h1->Sumw2();
   
      for ( Int_t e = 0; e < nEvents * nEvents; ++e ) {
         Double_t x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
         Double_t y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
         Double_t z = r.Uniform(0.9 * minRange, 1.1 * maxRange);
         h1->Fill(x, y, z, 1.0);
      }  
      h1->Write();
   } else {
      h1 = static_cast<TH3D*> ( refFile->Get("rr3D-h1") );
      TH3D* h2 = new TH3D("rr3D-h2", "h2-Title", 
                          numberOfBins, minRange, maxRange,
                          numberOfBins, minRange, maxRange,
                          numberOfBins, minRange, maxRange);
      h2->Sumw2();
   
      for ( Int_t e = 0; e < nEvents * nEvents; ++e ) {
         Double_t x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
         Double_t y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
         Double_t z = r.Uniform(0.9 * minRange, 1.1 * maxRange);
         h2->Fill(x, y, z, 1.0);
      }  

      ret = equals("Ref Read Hist 3D", h1, h2, cmpOptStats);
   }
   if ( h1 ) delete h1;
   return ret;
}

bool testRefReadProf3D()
{
   // Tests consistency with a reference file for 3D Profile

   TProfile3D* p1 = 0;
   bool ret = 0;
   if ( refFileOption == refFileWrite ) {
      p1 = new TProfile3D("rr3D-p1", "p1-Title", 
                          numberOfBins, minRange, maxRange,
                          numberOfBins, minRange, maxRange,
                          numberOfBins, minRange, maxRange);

      for ( Int_t e = 0; e < nEvents * nEvents; ++e ) {
         Double_t x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
         Double_t y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
         Double_t z = r.Uniform(0.9 * minRange, 1.1 * maxRange);
         Double_t t = r.Uniform(0.9 * minRange, 1.1 * maxRange);
         p1->Fill(x, y, z, t, 1.0);
      }  
      p1->Write();
   } else {
      p1 = static_cast<TProfile3D*> ( refFile->Get("rr3D-p1") );
      TProfile3D* p2 = new TProfile3D("rr3D-p2", "p2-Title", 
                          numberOfBins, minRange, maxRange,
                          numberOfBins, minRange, maxRange,
                          numberOfBins, minRange, maxRange);

      for ( Int_t e = 0; e < nEvents * nEvents; ++e ) {
         Double_t x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
         Double_t y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
         Double_t z = r.Uniform(0.9 * minRange, 1.1 * maxRange);
         Double_t t = r.Uniform(0.9 * minRange, 1.1 * maxRange);
         p2->Fill(x, y, z, t, 1.0);
      }  

      ret = equals("Ref Read Prof 3D", p1, p2, cmpOptStats);
   }
   if ( p1 ) delete p1;
   return ret;
}

bool testRefReadSparse()
{
   // Tests consistency with a reference file for Sparse Histogram

   Int_t bsize[] = { TMath::Nint( r.Uniform(1, 5) ),
                     TMath::Nint( r.Uniform(1, 5) ),
                     TMath::Nint( r.Uniform(1, 5) )};
   Double_t xmin[] = {minRange, minRange, minRange};
   Double_t xmax[] = {maxRange, maxRange, maxRange};

   THnSparseD* s1 = 0;
   bool ret = 0;

   if ( refFileOption == refFileWrite ) {
      s1 = new THnSparseD("rr-s1", "s1-Title", 3, bsize, xmin, xmax);
      s1->Sumw2();

      for ( Int_t e = 0; e < nEvents * nEvents; ++e ) {
         Double_t points[3];
         points[0] = r.Uniform( minRange * .9 , maxRange * 1.1 );
         points[1] = r.Uniform( minRange * .9 , maxRange * 1.1 );
         points[2] = r.Uniform( minRange * .9 , maxRange * 1.1 );
         s1->Fill(points);
      }  
      s1->Write();
   } else {
      s1 = static_cast<THnSparseD*> ( refFile->Get("rr-s1") );
      THnSparseD* s2 = new THnSparseD("rr-s1", "s1-Title", 3, bsize, xmin, xmax);
      s2->Sumw2();

      for ( Int_t e = 0; e < nEvents * nEvents; ++e ) {
         Double_t points[3];
         points[0] = r.Uniform( minRange * .9 , maxRange * 1.1 );
         points[1] = r.Uniform( minRange * .9 , maxRange * 1.1 );
         points[2] = r.Uniform( minRange * .9 , maxRange * 1.1 );
         s2->Fill(points);
      }  

      ret = equals("Ref Read Sparse", s1, s2, cmpOptStats);
   }
   if ( s1 ) delete s1;
   return ret;
}

bool testIntegerRebin()
{
   // Tests rebin method with an integer as input for 1D Histogram

   const int rebin = TMath::Nint( r.Uniform(minRebin, maxRebin) );
   UInt_t seed = r.GetSeed();
   TH1D* h1 = new TH1D("h1","Original Histogram", TMath::Nint( r.Uniform(1, 5) ) * rebin, minRange, maxRange);
   r.SetSeed(seed);
   for ( Int_t i = 0; i < nEvents; ++i )
      h1->Fill( r.Uniform( minRange * .9 , maxRange * 1.1 ) );

   TH1D* h2 = static_cast<TH1D*>( h1->Rebin(rebin, "testIntegerRebin") );

   TH1D* h3 = new TH1D("testIntegerRebin2", "testIntegerRebin2", 
                       h1->GetNbinsX() / rebin, minRange, maxRange);
   r.SetSeed(seed);
   for ( Int_t i = 0; i < nEvents; ++i )
      h3->Fill( r.Uniform( minRange * .9 , maxRange * 1.1 ) );

   bool ret = equals("TestIntegerRebinHist", h2, h3, cmpOptStats  );
   delete h1;
   delete h2;
   return ret;
}

bool testIntegerRebinProfile()
{
   // Tests rebin method with an integer as input for 1D Profile

   const int rebin = TMath::Nint( r.Uniform(minRebin, maxRebin) );
   TProfile* p1 = new TProfile("p1","p1-Title", TMath::Nint( r.Uniform(1, 5) ) * rebin, minRange, maxRange);
   TProfile* p3 = new TProfile("testIntRebProf", "testIntRebProf", p1->GetNbinsX() / rebin, minRange, maxRange);

   for ( Int_t i = 0; i < nEvents; ++i ) {
      Double_t x = r.Uniform( minRange * .9 , maxRange * 1.1 );
      Double_t y = r.Uniform( minRange * .9 , maxRange * 1.1 );
      p1->Fill( x, y );
      p3->Fill( x, y );
   }

   TProfile* p2 = static_cast<TProfile*>( p1->Rebin(rebin, "testIntegerRebin") );

   bool ret = equals("TestIntegerRebinProf", p2, p3, cmpOptStats );
   delete p1;
   delete p2;
   return ret;
}

bool testIntegerRebinNoName()
{
   // Tests rebin method with an integer as input and without name for 1D Histogram

   const int rebin = TMath::Nint( r.Uniform(minRebin, maxRebin) );
   UInt_t seed = r.GetSeed();
   TH1D* h1 = new TH1D("h2","Original Histogram", TMath::Nint( r.Uniform(1, 5) ) * rebin, minRange, maxRange);
   r.SetSeed(seed);
   for ( Int_t i = 0; i < nEvents; ++i )
      h1->Fill( r.Uniform( minRange * .9 , maxRange * 1.1 ) );

   TH1D* h2 = dynamic_cast<TH1D*>( h1->Clone() );
   h2->Rebin(rebin);

   TH1D* h3 = new TH1D("testIntegerRebinNoName", "testIntegerRebinNoName", 
                       int(h1->GetNbinsX() / rebin + 0.1), minRange, maxRange);
   r.SetSeed(seed);
   for ( Int_t i = 0; i < nEvents; ++i )
      h3->Fill( r.Uniform( minRange * .9 , maxRange * 1.1 ) );

   bool ret = equals("TestIntRebinNoName", h2, h3, cmpOptStats );
   delete h1;
   delete h2;
   return ret;
}

bool testIntegerRebinNoNameProfile()
{
   // Tests rebin method with an integer as input and without name for 1D Profile

   const int rebin = TMath::Nint( r.Uniform(minRebin, maxRebin) );
   TProfile* p1 = new TProfile("p1","p1-Title", TMath::Nint( r.Uniform(1, 5) ) * rebin, minRange, maxRange);
   TProfile* p3 = new TProfile("testIntRebNNProf", "testIntRebNNProf", int(p1->GetNbinsX() / rebin + 0.1), minRange, maxRange);

   for ( Int_t i = 0; i < nEvents; ++i ) {
      Double_t x = r.Uniform( minRange * .9 , maxRange * 1.1 );
      Double_t y = r.Uniform( minRange * .9 , maxRange * 1.1 );
      p1->Fill( x, y );
      p3->Fill( x, y );
   }

   TProfile* p2 = dynamic_cast<TProfile*>( p1->Clone() );
   p2->Rebin(rebin);
   bool ret = equals("TestIntRebNoNamProf", p2, p3, cmpOptStats);
   delete p1;
   delete p2;
   return ret;
}

bool testArrayRebin()
{
   // Tests rebin method with an array as input for 1D Histogram

   const int rebin = TMath::Nint( r.Uniform(minRebin, maxRebin) ) + 1;
   UInt_t seed = r.GetSeed();
   TH1D* h1 = new TH1D("h3","Original Histogram", TMath::Nint( r.Uniform(1, 5) ) * rebin * 2, minRange, maxRange);
   r.SetSeed(seed);
   for ( Int_t i = 0; i < nEvents; ++i )
      h1->Fill( r.Uniform( minRange * .9 , maxRange * 1.1 ) );

   // Create vector 
   Double_t * rebinArray = new Double_t[rebin];
   r.RndmArray(rebin, rebinArray);
   std::sort(rebinArray, rebinArray + rebin);
   for ( Int_t i = 0; i < rebin; ++i ) {
      rebinArray[i] = TMath::Nint( rebinArray[i] * ( h1->GetNbinsX() - 2 ) + 2 );
      rebinArray[i] = h1->GetBinLowEdge( h1->GetXaxis()->FindBin( rebinArray[i] ) );
   }
   

   rebinArray[0] = minRange;
   rebinArray[rebin-1] = maxRange;

   #ifdef __DEBUG__
   for ( Int_t i = 0; i < rebin; ++i ) 
      cout << rebinArray[i] << endl;
   cout << "rebin: " << rebin << endl;
   #endif

   TH1D* h2 = static_cast<TH1D*>( h1->Rebin(rebin - 1, "testArrayRebin", rebinArray) );

   TH1D* h3 = new TH1D("testArrayRebin2", "testArrayRebin2", rebin - 1, rebinArray );
   r.SetSeed(seed);
   for ( Int_t i = 0; i < nEvents; ++i )
      h3->Fill( r.Uniform( minRange * .9 , maxRange * 1.1 ) );

   delete [] rebinArray;
      
   bool ret = equals("TestArrayRebin", h2, h3, cmpOptStats);
   delete h1;
   delete h2;
   return ret;
}

bool testArrayRebinProfile()
{
   // Tests rebin method with an array as input for 1D Profile

   const int rebin = TMath::Nint( r.Uniform(minRebin, maxRebin) ) + 1;
   UInt_t seed = r.GetSeed();
   TProfile* p1 = new TProfile("p3","Original Histogram", TMath::Nint( r.Uniform(1, 5) ) * rebin * 2, minRange, maxRange);
   r.SetSeed(seed);
   for ( Int_t i = 0; i < nEvents; ++i ) {
      Double_t x = r.Uniform( minRange * .9 , maxRange * 1.1 );
      Double_t y = r.Uniform( minRange * .9 , maxRange * 1.1 ); 
      p1->Fill( x, y );
   }

   // Create vector 
   Double_t * rebinArray = new Double_t[rebin];
   r.RndmArray(rebin, rebinArray);
   std::sort(rebinArray, rebinArray + rebin);
   for ( Int_t i = 0; i < rebin; ++i ) {
      rebinArray[i] = TMath::Nint( rebinArray[i] * ( p1->GetNbinsX() - 2 ) + 2 );
      rebinArray[i] = p1->GetBinLowEdge( p1->GetXaxis()->FindBin( rebinArray[i] ) );
   }

   rebinArray[0] = minRange;
   rebinArray[rebin-1] = maxRange;

   #ifdef __DEBUG__
   for ( Int_t i = 0; i < rebin; ++i ) 
      cout << rebinArray[i] << endl;
   cout << "rebin: " << rebin << endl;
   #endif

   TProfile* p2 = static_cast<TProfile*>( p1->Rebin(rebin - 1, "testArrayRebinProf", rebinArray) );

   TProfile* p3 = new TProfile("testArrayRebinProf2", "testArrayRebinProf2", rebin - 1, rebinArray );
   r.SetSeed(seed);
   for ( Int_t i = 0; i < nEvents; ++i ) {
      Double_t x = r.Uniform( minRange * .9 , maxRange * 1.1 );
      Double_t y = r.Uniform( minRange * .9 , maxRange * 1.1 );
      p3->Fill( x, y );
   }

   delete [] rebinArray;
      
   bool ret = equals("TestArrayRebinProf", p2, p3, cmpOptStats );
   delete p1;
   delete p2;
   return ret;
}

bool test2DRebin()
{
   // Tests rebin method for 1D Histogram

   Int_t xrebin = TMath::Nint( r.Uniform(minRebin, maxRebin) );
   Int_t yrebin = TMath::Nint( r.Uniform(minRebin, maxRebin) );
   TH2D* h2d = new TH2D("h2d","Original Histogram", 
                       xrebin * TMath::Nint( r.Uniform(1, 5) ), minRange, maxRange, 
                       yrebin * TMath::Nint( r.Uniform(1, 5) ), minRange, maxRange);

   UInt_t seed = r.GetSeed();
   r.SetSeed(seed);
   for ( Int_t i = 0; i < nEvents; ++i )
      h2d->Fill( r.Uniform( minRange * .9 , maxRange * 1.1 ), r.Uniform( minRange * .9 , maxRange * 1.1 ) );

   TH2D* h2d2 = (TH2D*) h2d->Rebin2D(xrebin,yrebin, "h2d2");

   TH2D* h3 = new TH2D("test2DRebin", "test2DRebin", 
                       h2d->GetNbinsX() / xrebin, minRange, maxRange,
                       h2d->GetNbinsY() / yrebin, minRange, maxRange );
   r.SetSeed(seed);
   for ( Int_t i = 0; i < nEvents; ++i )
      h3->Fill( r.Uniform( minRange * .9 , maxRange * 1.1 ), r.Uniform( minRange * .9 , maxRange * 1.1 ) );

   bool ret = equals("TestIntRebin2D", h2d2, h3, cmpOptStats);
   delete h2d;
   delete h2d2;
   return ret;
}

bool testSparseRebin1() 
{
   // Tests rebin method for Sparse Histogram

   const int rebin = TMath::Nint( r.Uniform(minRebin, maxRebin) );

   Int_t bsizeRebin[] = { TMath::Nint( r.Uniform(1, 5) ),
                          TMath::Nint( r.Uniform(1, 5) ),
                          TMath::Nint( r.Uniform(1, 5) )};

   Int_t bsize[] = { bsizeRebin[0] * rebin,
                     bsizeRebin[1] * rebin,
                     bsizeRebin[2] * rebin};
                    
   Double_t xmin[] = {minRange, minRange, minRange};
   Double_t xmax[] = {maxRange, maxRange, maxRange};
   THnSparseD* s1 = new THnSparseD("rebin1-s1","s1-Title", 3, bsize, xmin, xmax);
   THnSparseD* s2 = new THnSparseD("rebin1-s2","s2-Title", 3, bsizeRebin, xmin, xmax);

   for ( Int_t i = 0; i < nEvents; ++i ) {
      Double_t points[3];
      points[0] = r.Uniform( minRange * .9 , maxRange * 1.1 );
      points[1] = r.Uniform( minRange * .9 , maxRange * 1.1 );
      points[2] = r.Uniform( minRange * .9 , maxRange * 1.1 );
      s1->Fill(points);
      s2->Fill(points);
   }

   THnSparse* s3 = s1->Rebin(rebin);

   bool ret = equals("THnSparse Rebin 1", s2, s3);
   delete s1;
   delete s2;
   return ret;
}

bool testTH2toTH1()
{
   const double centre_deviation = 0.3;

   const unsigned int binsizeX =  10;
   const unsigned int binsizeY =  11;
   static const unsigned int minbinX = 2;
   static const unsigned int maxbinX = 5;
   static const unsigned int minbinY = 3;
   static const unsigned int maxbinY = 8;
   const int lower_limit = 0;
   const int upper_limit = 10;
   
   r.SetSeed(10);

   TH2D* h2XY = new TH2D("h2XY", "h2XY", binsizeX, lower_limit, upper_limit, 
                                         binsizeY, lower_limit, upper_limit);

   TH1::StatOverflows(kTRUE);

   TH1D* h1X = new TH1D("h1X", "h1X", binsizeX, lower_limit, upper_limit);
   TH1D* h1Y = new TH1D("h1Y", "h1Y", binsizeY, lower_limit, upper_limit);

   TH1D* h1XOR = new TH1D("h1XOR", "h1XOR", binsizeX, lower_limit, upper_limit);
   TH1D* h1YOR = new TH1D("h1YOR", "h1YOR", binsizeY, lower_limit, upper_limit);

   TH1D* h1XR = new TH1D("h1XR", "h1XR",
                         maxbinX - minbinX + 1, 
                         h1X->GetXaxis()->GetBinLowEdge(minbinX), 
                         h1X->GetXaxis()->GetBinUpEdge(maxbinX) );
   TH1D* h1YR = new TH1D("h1YR", "h1YR",
                         maxbinY - minbinY + 1, 
                         h1Y->GetXaxis()->GetBinLowEdge(minbinY),
                         h1Y->GetXaxis()->GetBinUpEdge(maxbinY) );

   TProfile* pe1XY  = new TProfile("pe1XY",  "pe1XY",  binsizeX, lower_limit, upper_limit);
   TProfile* pe1XYOR  = new TProfile("pe1XYOR",  "pe1XYOR",  binsizeX, lower_limit, upper_limit);
   TProfile* pe1XYR = new TProfile("pe1XYR", "pe1XYR", 
                                   maxbinX - minbinX + 1, 
                                   h1X->GetXaxis()->GetBinLowEdge(minbinX), 
                                   h1X->GetXaxis()->GetBinUpEdge(maxbinX) );

   TProfile* pe1YX  = new TProfile("pe1YX",  "pe1YX", binsizeY, lower_limit, upper_limit);
   TProfile* pe1YXOR  = new TProfile("pe1YXOR",  "pe1YXOR", binsizeY, lower_limit, upper_limit);
   TProfile* pe1YXR = new TProfile("pe1YXR", "pe1YXR", 
                                  maxbinY - minbinY + 1, 
                                  h1Y->GetXaxis()->GetBinLowEdge(minbinY), 
                                  h1Y->GetXaxis()->GetBinUpEdge(maxbinY));
   
   for ( int ix = 0; ix <= h2XY->GetXaxis()->GetNbins() + 1; ++ix ) {
      double xc = h2XY->GetXaxis()->GetBinCenter(ix);
      double x = xc + centre_deviation * h2XY->GetXaxis()->GetBinWidth(ix); 
      for ( int iy = 0; iy <= h2XY->GetYaxis()->GetNbins() + 1; ++iy ) {
         double yc = h2XY->GetYaxis()->GetBinCenter(iy);
         double y = yc + centre_deviation * h2XY->GetYaxis()->GetBinWidth(iy);

         Double_t w = (Double_t) r.Uniform(1,3);
         
         h2XY->Fill(x,y,w);
               
         h1X->Fill(x,w);
         h1Y->Fill(y,w);

         pe1XY->Fill(xc,yc,w);
         pe1YX->Fill(yc,xc,w);
         if ( x >= h1X->GetXaxis()->GetBinLowEdge(minbinX) &&
              x <= h1X->GetXaxis()->GetBinUpEdge(maxbinX)  && 
              y >= h1Y->GetXaxis()->GetBinLowEdge(minbinY) &&
              y <= h1Y->GetXaxis()->GetBinUpEdge(maxbinY) ) 
         {
            h1XOR->Fill(x,w);
            h1YOR->Fill(y,w);
            h1XR->Fill(x,w);
            h1YR->Fill(y,w);
            pe1XYR->Fill(xc,yc,w);
            pe1YXR->Fill(yc,xc,w);
            pe1XYOR->Fill(xc,yc,w);
            pe1YXOR->Fill(yc,xc,w);
         }

      }
   }
   
   int status = 0;
   int options = cmpOptStats;

   // TH1 derived from h2XY
   status += equals("TH2XY    -> X", h1X, (TH1D*) h2XY->ProjectionX("x"), options);
   status += equals("TH2XY    -> Y", h1Y, (TH1D*) h2XY->ProjectionY("y"), options);

   status += equals("TH2XYO  -> X", h1X, (TH1D*) h2XY->ProjectionX("ox", 0, -1, "o"), options);
   status += equals("TH2XYO  -> Y", h1Y, (TH1D*) h2XY->ProjectionY("oy", 0, -1, "o"), options);

   status += equals("TH2XY -> PX", pe1XY, (TH1D*) h2XY->ProfileX("PX", 0,h2XY->GetYaxis()->GetNbins()+1), options);
   status += equals("TH2XY -> PY", pe1YX, (TH1D*) h2XY->ProfileY("PY", 0,h2XY->GetXaxis()->GetNbins()+1), options);

   status += equals("TH2XYO -> PX", pe1XY, (TH1D*) h2XY->ProfileX("OPX", 0,h2XY->GetYaxis()->GetNbins()+1,"o"), options);
   status += equals("TH2XYO -> PY", pe1YX, (TH1D*) h2XY->ProfileY("OPY", 0,h2XY->GetXaxis()->GetNbins()+1,"o"), options);

   h2XY->GetXaxis()->SetRange(minbinX, maxbinX);
   h2XY->GetYaxis()->SetRange(minbinY, maxbinY);

   h1X->GetXaxis()->SetRange(minbinX, maxbinX);
   h1Y->GetXaxis()->SetRange(minbinY, maxbinY);

   pe1XY->GetXaxis()->SetRange(minbinX, maxbinX);
   pe1YX->GetXaxis()->SetRange(minbinY, maxbinY);

   // This two, the statistics should work!
   options = 0;

   status += equals("TH2XYR  -> X", h1XR, (TH1D*) h2XY->ProjectionX("x"), options);
   status += equals("TH2XYR  -> Y", h1YR, (TH1D*) h2XY->ProjectionY("y"), options);

   status += equals("TH2XYRO -> X", h1XOR, (TH1D*) h2XY->ProjectionX("ox", 0, -1, "o"), options);
   status += equals("TH2XYRO -> Y", h1YOR, (TH1D*) h2XY->ProjectionY("oy", 0, -1, "o"), options);

   status += equals("TH2XYR -> PX", pe1XYR, (TH1D*) h2XY->ProfileX("PX"), options);
   status += equals("TH2XYR -> PY", pe1YXR, (TH1D*) h2XY->ProfileY("PY"), options);
   
   status += equals("TH2XYRO -> PX", pe1XYOR, (TH1D*) h2XY->ProfileX("OPX", 0,-1,"o"), options);
   status += equals("TH2XYRO -> PY", pe1YXOR, (TH1D*) h2XY->ProfileY("OPY", 0,-1,"o"), options);

   options = 0;

   delete h2XY;
   delete h1X;
   delete h1Y;
   delete h1XOR;
   delete h1YOR;

   delete h1XR;
   delete h1YR;

   delete pe1XY;
   delete pe1XYOR;
   delete pe1XYR;

   delete pe1YX;
   delete pe1YXOR;
   delete pe1YXR;

   return static_cast<bool>(status);
}

bool testTH3toTH1()
{
   const double centre_deviation = 0.3;

   const unsigned int binsizeX =  10;
   const unsigned int binsizeY =  11;
   const unsigned int binsizeZ =  12;
   static const unsigned int minbinX = 2;
   static const unsigned int maxbinX = 5;
   static const unsigned int minbinY = 3;
   static const unsigned int maxbinY = 8;
   static const unsigned int minbinZ = 4;
   static const unsigned int maxbinZ = 10;
   const int lower_limit = 0;
   const int upper_limit = 10;

   r.SetSeed(10);

   TH3D* h3 = new TH3D("h3","h3", binsizeX, lower_limit, upper_limit, 
                                  binsizeY, lower_limit, upper_limit, 
                                  binsizeZ, lower_limit, upper_limit);


   TH1::StatOverflows(kTRUE);

   TH1D* h1X = new TH1D("h1X", "h1X", binsizeX, lower_limit, upper_limit);
   TH1D* h1Y = new TH1D("h1Y", "h1Y", binsizeY, lower_limit, upper_limit);
   TH1D* h1Z = new TH1D("h1Z", "h1Z", binsizeZ, lower_limit, upper_limit);

   TH1D* h1XR = new TH1D("h1XR", "h1XR",
                         maxbinX - minbinX + 1, 
                         h1X->GetXaxis()->GetBinLowEdge(minbinX), 
                         h1X->GetXaxis()->GetBinUpEdge(maxbinX) );
   TH1D* h1YR = new TH1D("h1YR", "h1YR",
                         maxbinY - minbinY + 1, 
                         h1Y->GetXaxis()->GetBinLowEdge(minbinY),
                         h1Y->GetXaxis()->GetBinUpEdge(maxbinY) );
   TH1D* h1ZR = new TH1D("h1ZR", "h1ZR",
                         maxbinZ - minbinZ + 1, 
                         h1Z->GetXaxis()->GetBinLowEdge(minbinZ),
                         h1Z->GetXaxis()->GetBinUpEdge(maxbinZ) );

   TH1D* h1XOR = new TH1D("h1XOR", "h1XOR", binsizeX, lower_limit, upper_limit);
   TH1D* h1YOR = new TH1D("h1YOR", "h1YOR", binsizeY, lower_limit, upper_limit);
   TH1D* h1ZOR = new TH1D("h1ZOR", "h1ZOR", binsizeZ, lower_limit, upper_limit);

   h3->Sumw2();

   for ( int ix = 0; ix <= h3->GetXaxis()->GetNbins() + 1; ++ix ) {
         double x = centre_deviation * h3->GetXaxis()->GetBinWidth(ix) + h3->GetXaxis()->GetBinCenter(ix);
         for ( int iy = 0; iy <= h3->GetYaxis()->GetNbins() + 1; ++iy ) {
            double y = centre_deviation * h3->GetYaxis()->GetBinWidth(iy) + h3->GetYaxis()->GetBinCenter(iy);
            for ( int iz = 0; iz <= h3->GetZaxis()->GetNbins() + 1; ++iz ) {
               double z = centre_deviation * h3->GetZaxis()->GetBinWidth(iz) + h3->GetZaxis()->GetBinCenter(iz);
               Double_t w = (Double_t) r.Uniform(1,3);
         
               h3->Fill(x,y,z,w);
               
               h1X->Fill(x,w);
               h1Y->Fill(y,w);
               h1Z->Fill(z,w);
               
               if ( x >= h1X->GetXaxis()->GetBinLowEdge(minbinX) &&
                    x <= h1X->GetXaxis()->GetBinUpEdge(maxbinX)  && 
                    y >= h1Y->GetXaxis()->GetBinLowEdge(minbinY) &&
                    y <= h1Y->GetXaxis()->GetBinUpEdge(maxbinY)  &&
                    z >= h1Z->GetXaxis()->GetBinLowEdge(minbinZ) &&
                    z <= h1Z->GetXaxis()->GetBinUpEdge(maxbinZ) ) 
               {
                  h1XR->Fill(x,w);
                  h1YR->Fill(y,w);
                  h1ZR->Fill(z,w);
                  h1XOR->Fill(x,w);
                  h1YOR->Fill(y,w);
                  h1ZOR->Fill(z,w);
               }
               
            }
         }
   }

   int status = 0;
   int options = cmpOptStats;

   TH1D* tmp1 = 0;

   options = cmpOptStats;
   status += equals("TH3 -> X", h1X, (TH1D*) h3->Project3D("x"), options);
   tmp1 = h3->ProjectionX("x335");
   status += equals("TH3 -> X(x2)", tmp1, (TH1D*) h3->Project3D("x2"), options);
   delete tmp1; tmp1 = 0;
   status += equals("TH3 -> Y", h1Y, (TH1D*) h3->Project3D("y"), options);
   tmp1 = h3->ProjectionY("y335");
   status += equals("TH3 -> Y(x2)", tmp1, (TH1D*) h3->Project3D("y2"), options);
   delete tmp1; tmp1 = 0;
   status += equals("TH3 -> Z", h1Z, (TH1D*) h3->Project3D("z"), options);
   tmp1 = h3->ProjectionZ("z335");
   status += equals("TH3 -> Z(x2)", tmp1, (TH1D*) h3->Project3D("z2"), options);
   delete tmp1; tmp1 = 0;


   options = cmpOptStats;
   status += equals("TH3O -> X", h1X, (TH1D*) h3->Project3D("ox"), options);
   tmp1 = h3->ProjectionX("x1335");
   status += equals("TH3O -> X(x2)", tmp1, (TH1D*) h3->Project3D("ox2"), options);
   delete tmp1; tmp1 = 0;
   status += equals("TH3O -> Y", h1Y, (TH1D*) h3->Project3D("oy"), options);
   tmp1 = h3->ProjectionY("y1335");
   status += equals("TH3O -> Y(x2)", tmp1, (TH1D*) h3->Project3D("oy2"), options);
   delete tmp1; tmp1 = 0;
   status += equals("TH3O -> Z", h1Z, (TH1D*) h3->Project3D("oz"), options);
   tmp1 = h3->ProjectionZ("z1335");
   status += equals("TH3O -> Z(x2)", tmp1, (TH1D*) h3->Project3D("oz2"), options);
   delete tmp1; tmp1 = 0;

   h3->GetXaxis()->SetRange(minbinX, maxbinX);
   h3->GetYaxis()->SetRange(minbinY, maxbinY);
   h3->GetZaxis()->SetRange(minbinZ, maxbinZ);

   h1X->GetXaxis()->SetRange(minbinX, maxbinX);
   h1Y->GetXaxis()->SetRange(minbinY, maxbinY);
   h1Z->GetXaxis()->SetRange(minbinZ, maxbinZ);
   
   //Statistics are no longer conserved if the center_deviation != 0.0
   options = 0;
   status += equals("TH3R -> X", h1XR, (TH1D*) h3->Project3D("x34"), options );
   tmp1 = h3->ProjectionX("x3335", minbinY, maxbinY, minbinZ, maxbinZ);
   status += equals("TH3R -> X(x2)", tmp1, (TH1D*) h3->Project3D("x22"), options);
   delete tmp1; tmp1 = 0;
   status += equals("TH3R -> Y", h1YR, (TH1D*) h3->Project3D("y34"), options);
   tmp1 = h3->ProjectionY("y3335", minbinX, maxbinX, minbinZ, maxbinZ);
   status += equals("TH3R -> Y(x2)", tmp1, (TH1D*) h3->Project3D("y22"), options);
   delete tmp1; tmp1 = 0;
   status += equals("TH3R -> Z", h1ZR, (TH1D*) h3->Project3D("z34"), options);
   tmp1 = h3->ProjectionZ("z3335", minbinX, maxbinX, minbinY, maxbinY);
   status += equals("TH3R -> Z(x2)", tmp1, (TH1D*) h3->Project3D("z22"), options);
   delete tmp1; tmp1 = 0;

   options = 0;
   status += equals("TH3RO -> X", h1XOR, (TH1D*) h3->Project3D("ox"), options);
   tmp1 = h3->ProjectionX("x1335", minbinY, maxbinY, minbinZ, maxbinZ,"o");
   status += equals("TH3RO-> X(x2)", tmp1, (TH1D*) h3->Project3D("ox2"), options );
   delete tmp1; tmp1 = 0;
   status += equals("TH3RO -> Y", h1YOR, (TH1D*) h3->Project3D("oy"), options);
   tmp1 = h3->ProjectionY("y1335", minbinX, maxbinX, minbinZ, maxbinZ,"o");
   status += equals("TH3RO-> Y(x2)", tmp1, (TH1D*) h3->Project3D("oy2"), options);
   delete tmp1; tmp1 = 0;
   status += equals("TH3RO-> Z", h1ZOR, (TH1D*) h3->Project3D("oz"), options);
   tmp1 = h3->ProjectionZ("z1335", minbinX, maxbinX, minbinY, maxbinY,"o");
   status += equals("TH3RO-> Z(x2)", tmp1, (TH1D*) h3->Project3D("oz2"), options);
   delete tmp1; tmp1 = 0;

   options = 0;

   delete h3;

   delete h1X;
   delete h1Y;
   delete h1Z;

   delete h1XR;
   delete h1YR;
   delete h1ZR;

   delete h1XOR;
   delete h1YOR;
   delete h1ZOR;

   return status;
}

bool testTH3toTH2()
{
   const double centre_deviation = 0.3;

   const unsigned int binsizeX =  10;
   const unsigned int binsizeY =  11;
   const unsigned int binsizeZ =  12;
   static const unsigned int minbinX = 2;
   static const unsigned int maxbinX = 5;
   static const unsigned int minbinY = 3;
   static const unsigned int maxbinY = 8;
   static const unsigned int minbinZ = 4;
   static const unsigned int maxbinZ = 10;
   const int lower_limit = 0;
   const int upper_limit = 10;

   r.SetSeed(10);

   TH3D* h3 = new TH3D("h3","h3", binsizeX, lower_limit, upper_limit, 
                                  binsizeY, lower_limit, upper_limit, 
                                  binsizeZ, lower_limit, upper_limit);


   TH1::StatOverflows(kTRUE);

   TH2D* h2XY = new TH2D("h2XY", "h2XY", binsizeX, lower_limit, upper_limit, 
                                         binsizeY, lower_limit, upper_limit);
   TH2D* h2XZ = new TH2D("h2XZ", "h2XZ", binsizeX, lower_limit, upper_limit, 
                                         binsizeZ, lower_limit, upper_limit);
   TH2D* h2YX = new TH2D("h2YX", "h2YX", binsizeY, lower_limit, upper_limit, 
                                         binsizeX, lower_limit, upper_limit);
   TH2D* h2YZ = new TH2D("h2YZ", "h2YZ", binsizeY, lower_limit, upper_limit, 
                                         binsizeZ, lower_limit, upper_limit);
   TH2D* h2ZX = new TH2D("h2ZX", "h2ZX", binsizeZ, lower_limit, upper_limit, 
                                         binsizeX, lower_limit, upper_limit);
   TH2D* h2ZY = new TH2D("h2ZY", "h2ZY", binsizeZ, lower_limit, upper_limit, 
                                         binsizeY, lower_limit, upper_limit);

   TH2D* h2XYR = new TH2D("h2XYR", "h2XYR", 
                          maxbinX - minbinX + 1, h3->GetXaxis()->GetBinLowEdge(minbinX), h3->GetXaxis()->GetBinUpEdge(maxbinX), 
                          maxbinY - minbinY + 1, h3->GetYaxis()->GetBinLowEdge(minbinY), h3->GetYaxis()->GetBinUpEdge(maxbinY) );
   TH2D* h2XZR = new TH2D("h2XZR", "h2XZR",
                          maxbinX - minbinX + 1, h3->GetXaxis()->GetBinLowEdge(minbinX), h3->GetXaxis()->GetBinUpEdge(maxbinX), 
                          maxbinZ - minbinZ + 1, h3->GetZaxis()->GetBinLowEdge(minbinZ), h3->GetZaxis()->GetBinUpEdge(maxbinZ) );
   TH2D* h2YXR = new TH2D("h2YXR", "h2YXR",
                          maxbinY - minbinY + 1, h3->GetYaxis()->GetBinLowEdge(minbinY), h3->GetYaxis()->GetBinUpEdge(maxbinY),
                          maxbinX - minbinX + 1, h3->GetXaxis()->GetBinLowEdge(minbinX), h3->GetXaxis()->GetBinUpEdge(maxbinX) );
   TH2D* h2YZR = new TH2D("h2YZR", "h2YZR",
                          maxbinY - minbinY + 1, h3->GetYaxis()->GetBinLowEdge(minbinY), h3->GetYaxis()->GetBinUpEdge(maxbinY),
                          maxbinZ - minbinZ + 1, h3->GetZaxis()->GetBinLowEdge(minbinZ), h3->GetZaxis()->GetBinUpEdge(maxbinZ) );
   TH2D* h2ZXR = new TH2D("h2ZXR", "h2ZXR", 
                          maxbinZ - minbinZ + 1, h3->GetZaxis()->GetBinLowEdge(minbinZ), h3->GetZaxis()->GetBinUpEdge(maxbinZ),
                          maxbinX - minbinX + 1, h3->GetXaxis()->GetBinLowEdge(minbinX), h3->GetXaxis()->GetBinUpEdge(maxbinX) );
   TH2D* h2ZYR = new TH2D("h2ZYR", "h2ZYR", 
                          maxbinZ - minbinZ + 1, h3->GetZaxis()->GetBinLowEdge(minbinZ), h3->GetZaxis()->GetBinUpEdge(maxbinZ),
                          maxbinY - minbinY + 1, h3->GetYaxis()->GetBinLowEdge(minbinY), h3->GetYaxis()->GetBinUpEdge(maxbinY) );

   TH2D* h2XYOR = new TH2D("h2XYOR", "h2XYOR", binsizeX, lower_limit, upper_limit, 
                                               binsizeY, lower_limit, upper_limit);
   TH2D* h2XZOR = new TH2D("h2XZOR", "h2XZOR", binsizeX, lower_limit, upper_limit, 
                                               binsizeZ, lower_limit, upper_limit);
   TH2D* h2YXOR = new TH2D("h2YXOR", "h2YXOR", binsizeY, lower_limit, upper_limit, 
                                               binsizeX, lower_limit, upper_limit);
   TH2D* h2YZOR = new TH2D("h2YZOR", "h2YZOR", binsizeY, lower_limit, upper_limit, 
                                               binsizeZ, lower_limit, upper_limit);
   TH2D* h2ZXOR = new TH2D("h2ZXOR", "h2ZXOR", binsizeZ, lower_limit, upper_limit, 
                                               binsizeX, lower_limit, upper_limit);
   TH2D* h2ZYOR = new TH2D("h2ZYOR", "h2ZYOR", binsizeZ, lower_limit, upper_limit, 
                                               binsizeY, lower_limit, upper_limit);

   TProfile2D* pe2XY = new TProfile2D("pe2XY", "pe2XY", binsizeX, lower_limit, upper_limit, 
                                                        binsizeY, lower_limit, upper_limit);
   TProfile2D* pe2XZ = new TProfile2D("pe2XZ", "pe2XZ", binsizeX, lower_limit, upper_limit, 
                                                        binsizeZ, lower_limit, upper_limit);
   TProfile2D* pe2YX = new TProfile2D("pe2YX", "pe2YX", binsizeY, lower_limit, upper_limit, 
                                                        binsizeX, lower_limit, upper_limit);
   TProfile2D* pe2YZ = new TProfile2D("pe2YZ", "pe2YZ", binsizeY, lower_limit, upper_limit, 
                                                        binsizeZ, lower_limit, upper_limit);
   TProfile2D* pe2ZX = new TProfile2D("pe2ZX", "pe2ZX", binsizeZ, lower_limit, upper_limit, 
                                                        binsizeX, lower_limit, upper_limit);
   TProfile2D* pe2ZY = new TProfile2D("pe2ZY", "pe2ZY", binsizeZ, lower_limit, upper_limit, 
                                                        binsizeY, lower_limit, upper_limit);

   TProfile2D* pe2XYR = new TProfile2D("pe2XYR", "pe2XYR", 
                            maxbinX - minbinX + 1, h3->GetXaxis()->GetBinLowEdge(minbinX), h3->GetXaxis()->GetBinUpEdge(maxbinX), 
                            maxbinY - minbinY + 1, h3->GetYaxis()->GetBinLowEdge(minbinY), h3->GetYaxis()->GetBinUpEdge(maxbinY) );
   TProfile2D* pe2XZR = new TProfile2D("pe2XZR", "pe2XZR", 
                            maxbinX - minbinX + 1, h3->GetXaxis()->GetBinLowEdge(minbinX), h3->GetXaxis()->GetBinUpEdge(maxbinX), 
                            maxbinZ - minbinZ + 1, h3->GetZaxis()->GetBinLowEdge(minbinZ), h3->GetZaxis()->GetBinUpEdge(maxbinZ) );
   TProfile2D* pe2YXR = new TProfile2D("pe2YXR", "pe2YXR", 
                            maxbinY - minbinY + 1, h3->GetYaxis()->GetBinLowEdge(minbinY), h3->GetYaxis()->GetBinUpEdge(maxbinY),
                            maxbinX - minbinX + 1, h3->GetXaxis()->GetBinLowEdge(minbinX), h3->GetXaxis()->GetBinUpEdge(maxbinX) );
   TProfile2D* pe2YZR = new TProfile2D("pe2YZR", "pe2YZR",
                            maxbinY - minbinY + 1, h3->GetYaxis()->GetBinLowEdge(minbinY), h3->GetYaxis()->GetBinUpEdge(maxbinY),
                            maxbinZ - minbinZ + 1, h3->GetZaxis()->GetBinLowEdge(minbinZ), h3->GetZaxis()->GetBinUpEdge(maxbinZ) );
   TProfile2D* pe2ZXR = new TProfile2D("pe2ZXR", "pe2ZXR",
                            maxbinZ - minbinZ + 1, h3->GetZaxis()->GetBinLowEdge(minbinZ), h3->GetZaxis()->GetBinUpEdge(maxbinZ),
                            maxbinX - minbinX + 1, h3->GetXaxis()->GetBinLowEdge(minbinX), h3->GetXaxis()->GetBinUpEdge(maxbinX) );
   TProfile2D* pe2ZYR = new TProfile2D("pe2ZYR", "pe2ZYR", 
                            maxbinZ - minbinZ + 1, h3->GetZaxis()->GetBinLowEdge(minbinZ), h3->GetZaxis()->GetBinUpEdge(maxbinZ),
                            maxbinY - minbinY + 1, h3->GetYaxis()->GetBinLowEdge(minbinY), h3->GetYaxis()->GetBinUpEdge(maxbinY) );

   TProfile2D* pe2XYOR = new TProfile2D("pe2XYOR", "pe2XYOR", binsizeX, lower_limit, upper_limit, 
                                                              binsizeY, lower_limit, upper_limit);
   TProfile2D* pe2XZOR = new TProfile2D("pe2XZOR", "pe2XZOR", binsizeX, lower_limit, upper_limit, 
                                                              binsizeZ, lower_limit, upper_limit);
   TProfile2D* pe2YXOR = new TProfile2D("pe2YXOR", "pe2YXOR", binsizeY, lower_limit, upper_limit, 
                                                              binsizeX, lower_limit, upper_limit);
   TProfile2D* pe2YZOR = new TProfile2D("pe2YZOR", "pe2YZOR", binsizeY, lower_limit, upper_limit, 
                                                              binsizeZ, lower_limit, upper_limit);
   TProfile2D* pe2ZXOR = new TProfile2D("pe2ZXOR", "pe2ZXOR", binsizeZ, lower_limit, upper_limit, 
                                                              binsizeX, lower_limit, upper_limit);
   TProfile2D* pe2ZYOR = new TProfile2D("pe2ZYOR", "pe2ZYOR", binsizeZ, lower_limit, upper_limit, 
                                                              binsizeY, lower_limit, upper_limit);

   for ( int ix = 0; ix <= h3->GetXaxis()->GetNbins() + 1; ++ix ) {
      double xc = h3->GetXaxis()->GetBinCenter(ix);
      double x = xc + centre_deviation * h3->GetXaxis()->GetBinWidth(ix); 
      for ( int iy = 0; iy <= h3->GetYaxis()->GetNbins() + 1; ++iy ) {
         double yc = h3->GetYaxis()->GetBinCenter(iy);
         double y = yc + centre_deviation * h3->GetYaxis()->GetBinWidth(iy);
         for ( int iz = 0; iz <= h3->GetZaxis()->GetNbins() + 1; ++iz ) {
            double zc =  h3->GetZaxis()->GetBinCenter(iz);
            double z  = zc + centre_deviation * h3->GetZaxis()->GetBinWidth(iz);
            
//    for ( int ix = 0; ix <= h3->GetXaxis()->GetNbins() + 1; ++ix ) {
//       double x = centre_deviation * h3->GetXaxis()->GetBinWidth(ix) + h3->GetXaxis()->GetBinCenter(ix);
//       for ( int iy = 0; iy <= h3->GetYaxis()->GetNbins() + 1; ++iy ) {
//          double y = centre_deviation * h3->GetYaxis()->GetBinWidth(iy) + h3->GetYaxis()->GetBinCenter(iy);
//          for ( int iz = 0; iz <= h3->GetZaxis()->GetNbins() + 1; ++iz ) {
//             double z = centre_deviation * h3->GetZaxis()->GetBinWidth(iz) + h3->GetZaxis()->GetBinCenter(iz);
            Double_t w = (Double_t) r.Uniform(1,3);
         
            h3->Fill(x,y,z,w);

            h2XY->Fill(x,y,w);
            h2XZ->Fill(x,z,w);
            h2YX->Fill(y,x,w);
            h2YZ->Fill(y,z,w);
            h2ZX->Fill(z,x,w);
            h2ZY->Fill(z,y,w);
               
            pe2XY->Fill(xc,yc,zc,w);
            pe2XZ->Fill(xc,zc,yc,w);
            pe2YX->Fill(yc,xc,zc,w);
            pe2YZ->Fill(yc,zc,xc,w);
            pe2ZX->Fill(zc,xc,yc,w);
            pe2ZY->Fill(zc,yc,xc,w);

               if ( x >= h3->GetXaxis()->GetBinLowEdge(minbinX) &&
                    x <= h3->GetXaxis()->GetBinUpEdge(maxbinX)  && 
                    y >= h3->GetYaxis()->GetBinLowEdge(minbinY) &&
                    y <= h3->GetYaxis()->GetBinUpEdge(maxbinY)  &&
                    z >= h3->GetZaxis()->GetBinLowEdge(minbinZ) &&
                    z <= h3->GetZaxis()->GetBinUpEdge(maxbinZ) ) 
               {
                  h2XYR->Fill(x,y,w);
                  h2XZR->Fill(x,z,w);
                  h2YXR->Fill(y,x,w);
                  h2YZR->Fill(y,z,w);
                  h2ZXR->Fill(z,x,w);
                  h2ZYR->Fill(z,y,w);

                  h2XYOR->Fill(x,y,w);
                  h2XZOR->Fill(x,z,w);
                  h2YXOR->Fill(y,x,w);
                  h2YZOR->Fill(y,z,w);
                  h2ZXOR->Fill(z,x,w);
                  h2ZYOR->Fill(z,y,w);

                  pe2XYR->Fill(xc,yc,zc,w);
                  pe2XZR->Fill(xc,zc,yc,w);
                  pe2YXR->Fill(yc,xc,zc,w);
                  pe2YZR->Fill(yc,zc,xc,w);
                  pe2ZXR->Fill(zc,xc,yc,w);
                  pe2ZYR->Fill(zc,yc,xc,w);

                  pe2XYOR->Fill(xc,yc,zc,w);
                  pe2XZOR->Fill(xc,zc,yc,w);
                  pe2YXOR->Fill(yc,xc,zc,w);
                  pe2YZOR->Fill(yc,zc,xc,w);
                  pe2ZXOR->Fill(zc,xc,yc,w);
                  pe2ZYOR->Fill(zc,yc,xc,w);
               }
         }
      }
   }
   
   int status = 0;
   int options = cmpOptStats;

   options = cmpOptStats;
   status += equals("TH3 -> XY", h2XY, (TH2D*) h3->Project3D("yx"), options);
   status += equals("TH3 -> XZ", h2XZ, (TH2D*) h3->Project3D("zx"), options);
   status += equals("TH3 -> YX", h2YX, (TH2D*) h3->Project3D("XY"), options);
   status += equals("TH3 -> YZ", h2YZ, (TH2D*) h3->Project3D("ZY"), options);
   status += equals("TH3 -> ZX", h2ZX, (TH2D*) h3->Project3D("XZ"), options);
   status += equals("TH3 -> ZY", h2ZY, (TH2D*) h3->Project3D("YZ"), options);
   options = 0;

   options = cmpOptStats;
   status += equals("TH3O -> XY", h2XY, (TH2D*) h3->Project3D("oyx"), options);
   status += equals("TH3O -> XZ", h2XZ, (TH2D*) h3->Project3D("ozx"), options);
   status += equals("TH3O -> YX", h2YX, (TH2D*) h3->Project3D("oXY"), options);
   status += equals("TH3O -> YZ", h2YZ, (TH2D*) h3->Project3D("oZY"), options);
   status += equals("TH3O -> ZX", h2ZX, (TH2D*) h3->Project3D("oXZ"), options);
   status += equals("TH3O -> ZY", h2ZY, (TH2D*) h3->Project3D("oYZ"), options);
   options = 0; 

   options = cmpOptStats;
   status += equals("TH3 -> PXY", (TH2D*) pe2XY, (TH2D*) h3->Project3DProfile("yx  UF OF"), options);
   status += equals("TH3 -> PXZ", (TH2D*) pe2XZ, (TH2D*) h3->Project3DProfile("zx  UF OF"), options);
   status += equals("TH3 -> PYX", (TH2D*) pe2YX, (TH2D*) h3->Project3DProfile("xy  UF OF"), options);
   status += equals("TH3 -> PYZ", (TH2D*) pe2YZ, (TH2D*) h3->Project3DProfile("zy  UF OF"), options);
   status += equals("TH3 -> PZX", (TH2D*) pe2ZX, (TH2D*) h3->Project3DProfile("xz  UF OF"), options);
   status += equals("TH3 -> PZY", (TH2D*) pe2ZY, (TH2D*) h3->Project3DProfile("yz  UF OF"), options);
   options = 0;

   options = cmpOptStats;
   status += equals("TH3O -> PXY", (TH2D*) pe2XY, (TH2D*) h3->Project3DProfile("oyx  UF OF"), options);
   status += equals("TH3O -> PXZ", (TH2D*) pe2XZ, (TH2D*) h3->Project3DProfile("ozx  UF OF"), options);
   status += equals("TH3O -> PYX", (TH2D*) pe2YX, (TH2D*) h3->Project3DProfile("oxy  UF OF"), options);
   status += equals("TH3O -> PYZ", (TH2D*) pe2YZ, (TH2D*) h3->Project3DProfile("ozy  UF OF"), options);
   status += equals("TH3O -> PZX", (TH2D*) pe2ZX, (TH2D*) h3->Project3DProfile("oxz  UF OF"), options);
   status += equals("TH3O -> PZY", (TH2D*) pe2ZY, (TH2D*) h3->Project3DProfile("oyz  UF OF"), options);
   options = 0;   

   h3->GetXaxis()->SetRange(minbinX, maxbinX);
   h3->GetYaxis()->SetRange(minbinY, maxbinY);
   h3->GetZaxis()->SetRange(minbinZ, maxbinZ);

   // Stats won't work here, unless centre_deviation == 0.0
   options = 0;
   status += equals("TH3R -> XY", h2XYR, (TH2D*) h3->Project3D("yx"), options);
   status += equals("TH3R -> XZ", h2XZR, (TH2D*) h3->Project3D("zx"), options);
   status += equals("TH3R -> YX", h2YXR, (TH2D*) h3->Project3D("XY"), options);
   status += equals("TH3R -> YZ", h2YZR, (TH2D*) h3->Project3D("ZY"), options);
   status += equals("TH3R -> ZX", h2ZXR, (TH2D*) h3->Project3D("XZ"), options);
   status += equals("TH3R -> ZY", h2ZYR, (TH2D*) h3->Project3D("YZ"), options);
   options = 0;

   // Stats won't work here, unless centre_deviation == 0.0
   options = 0;
   status += equals("TH3OR -> XY", h2XYOR, (TH2D*) h3->Project3D("oyx"), options );
   status += equals("TH3OR -> XZ", h2XZOR, (TH2D*) h3->Project3D("ozx"), options);
   status += equals("TH3OR -> YX", h2YXOR, (TH2D*) h3->Project3D("oXY"), options);
   status += equals("TH3OR -> YZ", h2YZOR, (TH2D*) h3->Project3D("oZY"), options);
   status += equals("TH3OR -> ZX", h2ZXOR, (TH2D*) h3->Project3D("oXZ"), options);
   status += equals("TH3OR -> ZY", h2ZYOR, (TH2D*) h3->Project3D("oYZ"), options);
   options = 0;

   options = cmpOptStats;
   status += equals("TH3R -> PXY", (TH2D*) pe2XYR, (TH2D*) h3->Project3DProfile("yx  UF OF"), options);
   status += equals("TH3R -> PXZ", (TH2D*) pe2XZR, (TH2D*) h3->Project3DProfile("zx  UF OF"), options);
   status += equals("TH3R -> PYX", (TH2D*) pe2YXR, (TH2D*) h3->Project3DProfile("xy  UF OF"), options);
   status += equals("TH3R -> PYZ", (TH2D*) pe2YZR, (TH2D*) h3->Project3DProfile("zy  UF OF"), options);
   status += equals("TH3R -> PZX", (TH2D*) pe2ZXR, (TH2D*) h3->Project3DProfile("xz  UF OF"), options);
   status += equals("TH3R -> PZY", (TH2D*) pe2ZYR, (TH2D*) h3->Project3DProfile("yz  UF OF"), options);
   options = 0;

   options = cmpOptStats;
   status += equals("TH3OR -> PXY", (TH2D*) pe2XYOR, (TH2D*) h3->Project3DProfile("oyx  UF OF"), options);
   status += equals("TH3OR -> PXZ", (TH2D*) pe2XZOR, (TH2D*) h3->Project3DProfile("ozx  UF OF"), options);
   status += equals("TH3OR -> PYX", (TH2D*) pe2YXOR, (TH2D*) h3->Project3DProfile("oxy  UF OF"), options);
   status += equals("TH3OR -> PYZ", (TH2D*) pe2YZOR, (TH2D*) h3->Project3DProfile("ozy  UF OF"), options);
   status += equals("TH3OR -> PZX", (TH2D*) pe2ZXOR, (TH2D*) h3->Project3DProfile("oxz  UF OF"), options);
   status += equals("TH3OR -> PZY", (TH2D*) pe2ZYOR, (TH2D*) h3->Project3DProfile("oyz  UF OF"), options);
   options = 0;

   options = 0;

   delete h3;

   delete h2XY;
   delete h2XZ;
   delete h2YX;
   delete h2YZ;
   delete h2ZX;
   delete h2ZY;

   delete h2XYR;
   delete h2XZR;
   delete h2YXR;
   delete h2YZR;
   delete h2ZXR;
   delete h2ZYR;

   delete h2XYOR;
   delete h2XZOR;
   delete h2YXOR;
   delete h2YZOR;
   delete h2ZXOR;
   delete h2ZYOR;

   delete pe2XY;
   delete pe2XZ;
   delete pe2YX;
   delete pe2YZ;
   delete pe2ZX;
   delete pe2ZY;

   delete pe2XYR;
   delete pe2XZR;
   delete pe2YXR;
   delete pe2YZR;
   delete pe2ZXR;
   delete pe2ZYR;

   delete pe2XYOR;
   delete pe2XZOR;
   delete pe2YXOR;
   delete pe2YZOR;
   delete pe2ZXOR;
   delete pe2ZYOR;

   return status;
}


// In case of deviation, the profiles' content will not work anymore
// try only for testing the statistics
static const double centre_deviation = 0.3;


class ProjectionTester {
   // This class implements the tests for all types of projections of
   // all the classes tested in this file.


private:
   static const unsigned int binsizeX =  8;
   static const unsigned int binsizeY = 10;
   static const unsigned int binsizeZ = 12;
   static const int lower_limit = 0;
   static const int upper_limit = 10;


   TH3D* h3;
   TH2D* h2XY;
   TH2D* h2XZ;
   TH2D* h2YX;
   TH2D* h2YZ;
   TH2D* h2ZX;
   TH2D* h2ZY;
   TH1D* h1X;
   TH1D* h1Y;
   TH1D* h1Z;

   TH1D* h1XStats;
   TH1D* h1YStats;
   TH1D* h1ZStats;
   
   TProfile2D* pe2XY;
   TProfile2D* pe2XZ;
   TProfile2D* pe2YX;
   TProfile2D* pe2YZ;
   TProfile2D* pe2ZX;
   TProfile2D* pe2ZY;
   
   TH2D* h2wXY;
   TH2D* h2wXZ;
   TH2D* h2wYX;
   TH2D* h2wYZ;
   TH2D* h2wZX;
   TH2D* h2wZY;
   
   TProfile* pe1XY;
   TProfile* pe1XZ;
   TProfile* pe1YX;
   TProfile* pe1YZ;
   TProfile* pe1ZX;
   TProfile* pe1ZY;

   TH1D* hw1XZ;
   TH1D* hw1XY;
   TH1D* hw1YX;
   TH1D* hw1YZ;
   TH1D* hw1ZX;
   TH1D* hw1ZY;
   
   THnSparseD* s3;

   bool buildWithWeights;

   
public:
   ProjectionTester()
   {
      CreateHistograms();
      buildWithWeights = false;
   }
   
   void CreateHistograms()
   {
      h3 = new TH3D("h3","h3", binsizeX, lower_limit, upper_limit, 
                               binsizeY, lower_limit, upper_limit, 
                               binsizeZ, lower_limit, upper_limit);

      h2XY = new TH2D("h2XY", "h2XY", binsizeX, lower_limit, upper_limit, 
                                      binsizeY, lower_limit, upper_limit);
      h2XZ = new TH2D("h2XZ", "h2XZ", binsizeX, lower_limit, upper_limit, 
                                      binsizeZ, lower_limit, upper_limit);
      h2YX = new TH2D("h2YX", "h2YX", binsizeY, lower_limit, upper_limit, 
                                      binsizeX, lower_limit, upper_limit);
      h2YZ = new TH2D("h2YZ", "h2YZ", binsizeY, lower_limit, upper_limit, 
                                      binsizeZ, lower_limit, upper_limit);
      h2ZX = new TH2D("h2ZX", "h2ZX", binsizeZ, lower_limit, upper_limit, 
                                      binsizeX, lower_limit, upper_limit);
      h2ZY = new TH2D("h2ZY", "h2ZY", binsizeZ, lower_limit, upper_limit, 
                                      binsizeY, lower_limit, upper_limit);

      // The bit is set for all the histograms (It's a statistic variable)
      TH1::StatOverflows(kTRUE);

      h1X = new TH1D("h1X", "h1X", binsizeX, lower_limit, upper_limit);
      h1Y = new TH1D("h1Y", "h1Y", binsizeY, lower_limit, upper_limit);
      h1Z = new TH1D("h1Z", "h1Z", binsizeZ, lower_limit, upper_limit);

      h1XStats = new TH1D("h1XStats", "h1XStats", binsizeX, lower_limit, upper_limit);
      h1YStats = new TH1D("h1YStats", "h1YStats", binsizeY, lower_limit, upper_limit);
      h1ZStats = new TH1D("h1ZStats", "h1ZStats", binsizeZ, lower_limit, upper_limit);

      pe2XY = new TProfile2D("pe2XY", "pe2XY", binsizeX, lower_limit, upper_limit, 
                                               binsizeY, lower_limit, upper_limit);
      pe2XZ = new TProfile2D("pe2XZ", "pe2XZ", binsizeX, lower_limit, upper_limit, 
                                               binsizeZ, lower_limit, upper_limit);
      pe2YX = new TProfile2D("pe2YX", "pe2YX", binsizeY, lower_limit, upper_limit, 
                                               binsizeX, lower_limit, upper_limit);
      pe2YZ = new TProfile2D("pe2YZ", "pe2YZ", binsizeY, lower_limit, upper_limit, 
                                               binsizeZ, lower_limit, upper_limit);
      pe2ZX = new TProfile2D("pe2ZX", "pe2ZX", binsizeZ, lower_limit, upper_limit, 
                                               binsizeX, lower_limit, upper_limit);
      pe2ZY = new TProfile2D("pe2ZY", "pe2ZY", binsizeZ, lower_limit, upper_limit, 
                                               binsizeY, lower_limit, upper_limit);
      
      h2wXY = new TH2D("h2wXY", "h2wXY", binsizeX, lower_limit, upper_limit, 
                                         binsizeY, lower_limit, upper_limit);
      h2wXZ = new TH2D("h2wXZ", "h2wXZ", binsizeX, lower_limit, upper_limit, 
                                         binsizeZ, lower_limit, upper_limit);
      h2wYX = new TH2D("h2wYX", "h2wYX", binsizeY, lower_limit, upper_limit, 
                                         binsizeX, lower_limit, upper_limit);
      h2wYZ = new TH2D("h2wYZ", "h2wYZ", binsizeY, lower_limit, upper_limit, 
                                         binsizeZ, lower_limit, upper_limit);
      h2wZX = new TH2D("h2wZX", "h2wZX", binsizeZ, lower_limit, upper_limit, 
                                         binsizeX, lower_limit, upper_limit);
      h2wZY = new TH2D("h2wZY", "h2wZY", binsizeZ, lower_limit, upper_limit, 
                                         binsizeY, lower_limit, upper_limit);

      h2wXY->Sumw2();
      h2wXZ->Sumw2();
      h2wYX->Sumw2();
      h2wYZ->Sumw2();
      h2wZX->Sumw2();
      h2wZY->Sumw2();

      pe1XY = new TProfile("pe1XY", "pe1XY", binsizeX, lower_limit, upper_limit);
      pe1XZ = new TProfile("pe1XZ", "pe1XZ", binsizeX, lower_limit, upper_limit);
      pe1YX = new TProfile("pe1YX", "pe1YX", binsizeY, lower_limit, upper_limit);
      pe1YZ = new TProfile("pe1YZ", "pe1YZ", binsizeY, lower_limit, upper_limit);
      pe1ZX = new TProfile("pe1ZX", "pe1ZX", binsizeZ, lower_limit, upper_limit);
      pe1ZY = new TProfile("pe1ZY", "pe1ZY", binsizeZ, lower_limit, upper_limit);

      hw1XY = new TH1D("hw1XY", "hw1XY", binsizeX, lower_limit, upper_limit);
      hw1XZ = new TH1D("hw1XZ", "hw1XZ", binsizeX, lower_limit, upper_limit);
      hw1YX = new TH1D("hw1YX", "hw1YX", binsizeY, lower_limit, upper_limit);
      hw1YZ = new TH1D("hw1YZ", "hw1YZ", binsizeY, lower_limit, upper_limit);
      hw1ZX = new TH1D("hw1ZX", "hw1ZX", binsizeZ, lower_limit, upper_limit);
      hw1ZY = new TH1D("hw1ZY", "hw1ZY", binsizeZ, lower_limit, upper_limit);

      hw1XZ->Sumw2();
      hw1XY->Sumw2();
      hw1YX->Sumw2();
      hw1YZ->Sumw2();
      hw1ZX->Sumw2();
      hw1ZY->Sumw2();

      Int_t bsize[] = {binsizeX, binsizeY, binsizeZ};
      Double_t xmin[] = {lower_limit, lower_limit, lower_limit};
      Double_t xmax[] = {upper_limit, upper_limit, upper_limit};
      s3 = new THnSparseD("s3","s3", 3, bsize, xmin, xmax);

   }
   
   void DeleteHistograms()
   {
      delete h3;
      
      delete h2XY;
      delete h2XZ;
      delete h2YX;
      delete h2YZ;
      delete h2ZX;
      delete h2ZY;

      delete h1X;
      delete h1Y;
      delete h1Z;
      
      delete h1XStats;
      delete h1YStats;
      delete h1ZStats;

      delete pe2XY;
      delete pe2XZ;
      delete pe2YX;
      delete pe2YZ;
      delete pe2ZX;
      delete pe2ZY;
      
      delete h2wXY;
      delete h2wXZ;
      delete h2wYX;
      delete h2wYZ;
      delete h2wZX;
      delete h2wZY;

      delete pe1XY;
      delete pe1XZ;
      delete pe1YX;
      delete pe1YZ;
      delete pe1ZY;
      delete pe1ZX;

      delete hw1XY;
      delete hw1XZ;
      delete hw1YX;
      delete hw1YZ;
      delete hw1ZX;
      delete hw1ZY;

      delete s3;
   }
   
   virtual ~ProjectionTester()
   {
      DeleteHistograms();
   }
   
   
   void buildHistograms()
   {
      if (h3->GetSumw2N() ) s3->Sumw2();

      for ( int ix = 0; ix <= h3->GetXaxis()->GetNbins() + 1; ++ix ) {
         double xc = h3->GetXaxis()->GetBinCenter(ix);
         double x = xc + centre_deviation * h3->GetXaxis()->GetBinWidth(ix); 
         for ( int iy = 0; iy <= h3->GetYaxis()->GetNbins() + 1; ++iy ) {
            double yc = h3->GetYaxis()->GetBinCenter(iy);
            double y = yc + centre_deviation * h3->GetYaxis()->GetBinWidth(iy);
            for ( int iz = 0; iz <= h3->GetZaxis()->GetNbins() + 1; ++iz ) {
               double zc =  h3->GetZaxis()->GetBinCenter(iz);
               double z  = zc + centre_deviation * h3->GetZaxis()->GetBinWidth(iz);
               for ( int i = 0; i < (int) r.Uniform(1,3); ++i )
               {
                  h3->Fill(x,y,z);

                  Double_t points[] = {x,y,z};
                  s3->Fill(points);
                  
                  h2XY->Fill(x,y);
                  h2XZ->Fill(x,z);
                  h2YX->Fill(y,x);
                  h2YZ->Fill(y,z);
                  h2ZX->Fill(z,x);
                  h2ZY->Fill(z,y);
                  
                  h1X->Fill(x);
                  h1Y->Fill(y);
                  h1Z->Fill(z);

                  if ( ix > 0 && ix < h3->GetXaxis()->GetNbins() + 1 &&
                       iy > 0 && iy < h3->GetYaxis()->GetNbins() + 1 &&
                       iz > 0 && iz < h3->GetZaxis()->GetNbins() + 1 )
                  {
                     h1XStats->Fill(x);
                     h1YStats->Fill(y);
                     h1ZStats->Fill(z);
                  }

                  // for filling reference profile need to use bin center 
                  // because projection from histogram can use only bin center 
                  pe2XY->Fill(xc,yc,zc);
                  pe2XZ->Fill(xc,zc,yc);
                  pe2YX->Fill(yc,xc,zc);
                  pe2YZ->Fill(yc,zc,xc);
                  pe2ZX->Fill(zc,xc,yc);
                  pe2ZY->Fill(zc,yc,xc);
                  
                  // reference histogram to test with option W. 
                  // need to use bin center for the weight
                  h2wXY->Fill(x,y,zc);
                  h2wXZ->Fill(x,z,yc);
                  h2wYX->Fill(y,x,zc);
                  h2wYZ->Fill(y,z,xc);
                  h2wZX->Fill(z,x,yc);
                  h2wZY->Fill(z,y,xc);

                  pe1XY->Fill(xc,yc);
                  pe1XZ->Fill(xc,zc);
                  pe1YX->Fill(yc,xc);
                  pe1YZ->Fill(yc,zc);
                  pe1ZX->Fill(zc,xc);
                  pe1ZY->Fill(zc,yc);

                  hw1XY->Fill(x,yc);
                  hw1XZ->Fill(x,zc);
                  hw1YX->Fill(y,xc);
                  hw1YZ->Fill(y,zc);
                  hw1ZX->Fill(z,xc);
                  hw1ZY->Fill(z,yc);
               }
            }
         }
      }

      buildWithWeights = false;
   }

   void buildHistogramsWithWeights()
   {
      s3->Sumw2();

      for ( int ix = 0; ix <= h3->GetXaxis()->GetNbins() + 1; ++ix ) {
         double xc = h3->GetXaxis()->GetBinCenter(ix);
         double x = xc + centre_deviation * h3->GetXaxis()->GetBinWidth(ix); 
         for ( int iy = 0; iy <= h3->GetYaxis()->GetNbins() + 1; ++iy ) {
            double yc = h3->GetYaxis()->GetBinCenter(iy);
            double y = yc + centre_deviation * h3->GetYaxis()->GetBinWidth(iy);
            for ( int iz = 0; iz <= h3->GetZaxis()->GetNbins() + 1; ++iz ) {
               double zc =  h3->GetZaxis()->GetBinCenter(iz);
               double z  = zc + centre_deviation * h3->GetZaxis()->GetBinWidth(iz);

               Double_t w = (Double_t) r.Uniform(1,3);

               h3->Fill(x,y,z,w);

               Double_t points[] = {x,y,z};
               s3->Fill(points,w);
               
               h2XY->Fill(x,y,w);
               h2XZ->Fill(x,z,w);
               h2YX->Fill(y,x,w);
               h2YZ->Fill(y,z,w);
               h2ZX->Fill(z,x,w);
               h2ZY->Fill(z,y,w);
               
               h1X->Fill(x,w);
               h1Y->Fill(y,w);
               h1Z->Fill(z,w);
                   
               if ( ix > 0 && ix < h3->GetXaxis()->GetNbins() + 1 &&
                    iy > 0 && iy < h3->GetYaxis()->GetNbins() + 1 &&
                    iz > 0 && iz < h3->GetZaxis()->GetNbins() + 1 )
               {
                  h1XStats->Fill(x,w);
                  h1YStats->Fill(y,w);
                  h1ZStats->Fill(z,w);
               }              

               pe2XY->Fill(xc,yc,zc,w);
               pe2XZ->Fill(xc,zc,yc,w);
               pe2YX->Fill(yc,xc,zc,w);
               pe2YZ->Fill(yc,zc,xc,w);
               pe2ZX->Fill(zc,xc,yc,w);
               pe2ZY->Fill(zc,yc,xc,w);
               
               h2wXY->Fill(x,y,zc*w);
               h2wXZ->Fill(x,z,yc*w);
               h2wYX->Fill(y,x,zc*w);
               h2wYZ->Fill(y,z,xc*w);
               h2wZX->Fill(z,x,yc*w);
               h2wZY->Fill(z,y,xc*w);
               
               pe1XY->Fill(xc,yc,w);
               pe1XZ->Fill(xc,zc,w);
               pe1YX->Fill(yc,xc,w);
               pe1YZ->Fill(yc,zc,w);
               pe1ZX->Fill(zc,xc,w);
               pe1ZY->Fill(zc,yc,w);
               
               hw1XY->Fill(x,yc*w);
               hw1XZ->Fill(x,zc*w);
               hw1YX->Fill(y,xc*w);
               hw1YZ->Fill(y,zc*w);
               hw1ZX->Fill(z,xc*w);
               hw1ZY->Fill(z,yc*w);
            }
         }
      }

      buildWithWeights = true;
   }
   
   void buildHistograms(int xmin, int xmax,
                        int ymin, int ymax,
                        int zmin, int zmax)
   {
      for ( int ix = 0; ix <= h3->GetXaxis()->GetNbins() + 1; ++ix ) {
         double xc = h3->GetXaxis()->GetBinCenter(ix);
         double x = xc + centre_deviation * h3->GetXaxis()->GetBinWidth(ix); 
         for ( int iy = 0; iy <= h3->GetYaxis()->GetNbins() + 1; ++iy ) {
            double yc = h3->GetYaxis()->GetBinCenter(iy);
            double y = yc + centre_deviation * h3->GetYaxis()->GetBinWidth(iy);
            for ( int iz = 0; iz <= h3->GetZaxis()->GetNbins() + 1; ++iz ) {
               double zc =  h3->GetZaxis()->GetBinCenter(iz);
               double z  = zc + centre_deviation * h3->GetZaxis()->GetBinWidth(iz);

               for ( int i = 0; i < (int) r.Uniform(1,3); ++i )
               {
                  h3->Fill(x,y,z);

                  Double_t points[] = {x,y,z};
                  s3->Fill(points);
                  
                  if ( h3->GetXaxis()->FindBin(x) >= xmin && h3->GetXaxis()->FindBin(x) <= xmax &&
                       h3->GetYaxis()->FindBin(y) >= ymin && h3->GetYaxis()->FindBin(y) <= ymax &&
                       h3->GetZaxis()->FindBin(z) >= zmin && h3->GetZaxis()->FindBin(z) <= zmax )
                  {
                     if ( defaultEqualOptions & cmpOptPrint )
                        cout << "Filling (" << x << "," << y << "," << z << ")!" << endl;
                     
                     h2XY->Fill(x,y);
                     h2XZ->Fill(x,z);
                     h2YX->Fill(y,x);
                     h2YZ->Fill(y,z);
                     h2ZX->Fill(z,x);
                     h2ZY->Fill(z,y);
                     
                     h1X->Fill(x);
                     h1Y->Fill(y);
                     h1Z->Fill(z);
                     
                     pe2XY->Fill(xc,yc,zc);
                     pe2XZ->Fill(xc,zc,yc);
                     pe2YX->Fill(yc,xc,zc);
                     pe2YZ->Fill(yc,zc,xc);
                     pe2ZX->Fill(zc,xc,yc);
                     pe2ZY->Fill(zc,yc,xc);
                     
                     h2wXY->Fill(x,y,z);
                     h2wXZ->Fill(x,z,y);
                     h2wYX->Fill(y,x,z);
                     h2wYZ->Fill(y,z,x);
                     h2wZX->Fill(z,x,y);
                     h2wZY->Fill(z,y,x);

                     pe1XY->Fill(xc,yc);
                     pe1XZ->Fill(xc,zc);
                     pe1YX->Fill(yc,xc);
                     pe1YZ->Fill(yc,zc);
                     pe1ZX->Fill(zc,xc);
                     pe1ZY->Fill(zc,yc);
                     
                     hw1XY->Fill(x,y);
                     hw1XZ->Fill(x,z);
                     hw1YX->Fill(y,x);
                     hw1YZ->Fill(y,z);
                     hw1ZX->Fill(z,x);
                     hw1ZY->Fill(z,y);
                  }
               }
            }
         }
      }
      
      h3->GetXaxis()->SetRange(xmin, xmax);
      h3->GetYaxis()->SetRange(ymin, ymax);
      h3->GetZaxis()->SetRange(zmin, zmax);
      
      h2XY->GetXaxis()->SetRange(xmin, xmax);
      h2XY->GetYaxis()->SetRange(ymin, ymax);
      
      h2XZ->GetXaxis()->SetRange(xmin, xmax);
      h2XZ->GetZaxis()->SetRange(zmin, zmax);
      
      h2YX->GetYaxis()->SetRange(ymin, ymax);
      h2YX->GetXaxis()->SetRange(xmin, xmax);
      
      h2YZ->GetYaxis()->SetRange(ymin, ymax);
      h2YZ->GetZaxis()->SetRange(zmin, zmax);
      
      h2ZX->GetZaxis()->SetRange(zmin, zmax);
      h2ZX->GetXaxis()->SetRange(xmin, xmax);
      
      h2ZY->GetZaxis()->SetRange(zmin, zmax);
      h2ZY->GetYaxis()->SetRange(ymin, ymax);
      
      h1X->GetXaxis()->SetRange(xmin, xmax);
      h1Y->GetXaxis()->SetRange(ymin, ymax);
      h1Z->GetXaxis()->SetRange(zmin, zmax);

      // Neet to set up the rest of the ranges!

      s3->GetAxis(1)->SetRange(xmin, xmax);
      s3->GetAxis(2)->SetRange(ymin, ymax);
      s3->GetAxis(3)->SetRange(zmin, zmax);

      buildWithWeights = false;
   }
   
   int compareHistograms()
   {
      int status = 0;
      int options = 0;
      
      // TH2 derived from TH3
      options = cmpOptStats;
      status += equals("TH3 -> XY", h2XY, (TH2D*) h3->Project3D("yx"), options);
      status += equals("TH3 -> XZ", h2XZ, (TH2D*) h3->Project3D("zx"), options);
      status += equals("TH3 -> YX", h2YX, (TH2D*) h3->Project3D("XY"), options);
      status += equals("TH3 -> YZ", h2YZ, (TH2D*) h3->Project3D("ZY"), options);
      status += equals("TH3 -> ZX", h2ZX, (TH2D*) h3->Project3D("XZ"), options);
      status += equals("TH3 -> ZY", h2ZY, (TH2D*) h3->Project3D("YZ"), options);
      options = 0;
      if ( defaultEqualOptions & cmpOptPrint )
         cout << "----------------------------------------------" << endl;
      
      // TH1 derived from TH3
      options = cmpOptStats;
      TH1D* tmp1 = 0;
      status += equals("TH3 -> X", h1X, (TH1D*) h3->Project3D("x"), options);
      tmp1 = h3->ProjectionX("x335");
      status += equals("TH3 -> X(x2)", tmp1, (TH1D*) h3->Project3D("x2"), options);
      delete tmp1; tmp1 = 0;
      status += equals("TH3 -> Y", h1Y, (TH1D*) h3->Project3D("y"), options);
      tmp1 = h3->ProjectionY("y335");
      status += equals("TH3 -> Y(x2)", tmp1, (TH1D*) h3->Project3D("y2"), options);
      delete tmp1; tmp1 = 0;
      status += equals("TH3 -> Z", h1Z, (TH1D*) h3->Project3D("z"), options);
      tmp1 = h3->ProjectionZ("z335");
      status += equals("TH3 -> Z(x2)", tmp1, (TH1D*) h3->Project3D("z2"), options);
      delete tmp1; tmp1 = 0;

      options = 0;
      if ( defaultEqualOptions & cmpOptPrint )
         cout << "----------------------------------------------" << endl;
      
      // TH1 derived from h2XY
      options = cmpOptStats;
      status += equals("TH2XY -> X", h1X, (TH1D*) h2XY->ProjectionX("x"), options);
      status += equals("TH2XY -> Y", h1Y, (TH1D*) h2XY->ProjectionY("y"), options);
      // TH1 derived from h2XZ
      status += equals("TH2XZ -> X", h1X, (TH1D*) h2XZ->ProjectionX("x"), options);
      status += equals("TH2XZ -> Z", h1Z, (TH1D*) h2XZ->ProjectionY("z"), options);
      // TH1 derived from h2YX
      status += equals("TH2YX -> Y", h1Y, (TH1D*) h2YX->ProjectionX("y"), options);
      status += equals("TH2YX -> X", h1X, (TH1D*) h2YX->ProjectionY("x"), options);
      // TH1 derived from h2YZ
      status += equals("TH2YZ -> Y", h1Y, (TH1D*) h2YZ->ProjectionX("y"), options);
      status += equals("TH2YZ -> Z", h1Z, (TH1D*) h2YZ->ProjectionY("z"), options);
      // TH1 derived from h2ZX
      status += equals("TH2ZX -> Z", h1Z, (TH1D*) h2ZX->ProjectionX("z"), options);
      status += equals("TH2ZX -> X", h1X, (TH1D*) h2ZX->ProjectionY("x"), options);
      // TH1 derived from h2ZY
      status += equals("TH2ZY -> Z", h1Z, (TH1D*) h2ZY->ProjectionX("z"), options);
      status += equals("TH2ZY -> Y", h1Y, (TH1D*) h2ZY->ProjectionY("y"), options);
      options = 0;
      if ( defaultEqualOptions & cmpOptPrint )
         cout << "----------------------------------------------" << endl;

      // in the following comparison with profiles we need to re-calculate statistics using bin centers 
      // on the reference histograms
      if (centre_deviation != 0) { 
         h2XY->ResetStats();      
         h2YX->ResetStats();
         h2XZ->ResetStats();      
         h2ZX->ResetStats();
         h2YZ->ResetStats();      
         h2ZY->ResetStats();

         h1X->ResetStats(); 
         h1Y->ResetStats(); 
         h1Z->ResetStats(); 
      }
      
      // Now the histograms comming from the Profiles!
      options = cmpOptStats;
      status += equals("TH3 -> PBXY", h2XY, (TH2D*) h3->Project3DProfile("yx UF OF")->ProjectionXY("1", "B"), options  );
      status += equals("TH3 -> PBXZ", h2XZ, (TH2D*) h3->Project3DProfile("zx UF OF")->ProjectionXY("2", "B"), options);
      status += equals("TH3 -> PBYX", h2YX, (TH2D*) h3->Project3DProfile("xy UF OF")->ProjectionXY("3", "B"), options);
      status += equals("TH3 -> PBYZ", h2YZ, (TH2D*) h3->Project3DProfile("zy UF OF")->ProjectionXY("4", "B"), options);
      status += equals("TH3 -> PBZX", h2ZX, (TH2D*) h3->Project3DProfile("xz UF OF")->ProjectionXY("5", "B"), options);
      status += equals("TH3 -> PBZY", h2ZY, (TH2D*) h3->Project3DProfile("yz UF OF")->ProjectionXY("6", "B"), options);
      options = 0;
      if ( defaultEqualOptions & cmpOptPrint )
         cout << "----------------------------------------------" << endl;
      
      // test directly project3dprofile
      options = cmpOptStats;
      status += equals("TH3 -> PXY", (TH2D*) pe2XY, (TH2D*) h3->Project3DProfile("yx  UF OF"), options);
      status += equals("TH3 -> PXZ", (TH2D*) pe2XZ, (TH2D*) h3->Project3DProfile("zx  UF OF"), options);
      status += equals("TH3 -> PYX", (TH2D*) pe2YX, (TH2D*) h3->Project3DProfile("xy  UF OF"), options);
      status += equals("TH3 -> PYZ", (TH2D*) pe2YZ, (TH2D*) h3->Project3DProfile("zy  UF OF"), options);
      status += equals("TH3 -> PZX", (TH2D*) pe2ZX, (TH2D*) h3->Project3DProfile("xz  UF OF"), options);
      status += equals("TH3 -> PZY", (TH2D*) pe2ZY, (TH2D*) h3->Project3DProfile("yz  UF OF"), options);
      options = 0;
      if ( defaultEqualOptions & cmpOptPrint )
         cout << "----------------------------------------------" << endl;
      
      // test option E of ProjectionXY
      options = 0;
      status += equals("TH3 -> PEXY", (TH2D*) pe2XY, (TH2D*) h3->Project3DProfile("yx  UF OF")->ProjectionXY("1", "E"), options);
      status += equals("TH3 -> PEXZ", (TH2D*) pe2XZ, (TH2D*) h3->Project3DProfile("zx  UF OF")->ProjectionXY("2", "E"), options);
      status += equals("TH3 -> PEYX", (TH2D*) pe2YX, (TH2D*) h3->Project3DProfile("xy  UF OF")->ProjectionXY("3", "E"), options);
      status += equals("TH3 -> PEYZ", (TH2D*) pe2YZ, (TH2D*) h3->Project3DProfile("zy  UF OF")->ProjectionXY("4", "E"), options);
      status += equals("TH3 -> PEZX", (TH2D*) pe2ZX, (TH2D*) h3->Project3DProfile("xz  UF OF")->ProjectionXY("5", "E"), options);
      status += equals("TH3 -> PEZY", (TH2D*) pe2ZY, (TH2D*) h3->Project3DProfile("yz  UF OF")->ProjectionXY("6", "E"), options);
      options = 0;
      if ( defaultEqualOptions & cmpOptPrint )
         cout << "----------------------------------------------" << endl;
      
      // test option W of ProjectionXY
      
      // The error fails when built with weights. It is not properly calculated
      if ( buildWithWeights ) options = cmpOptNoError;
      status += equals("TH3 -> PWXY", (TH2D*) h2wXY, (TH2D*) h3->Project3DProfile("yx  UF OF")->ProjectionXY("1", "W"), options);
      status += equals("TH3 -> PWXZ", (TH2D*) h2wXZ, (TH2D*) h3->Project3DProfile("zx  UF OF")->ProjectionXY("2", "W"), options);
      status += equals("TH3 -> PWYX", (TH2D*) h2wYX, (TH2D*) h3->Project3DProfile("xy  UF OF")->ProjectionXY("3", "W"), options);
      status += equals("TH3 -> PWYZ", (TH2D*) h2wYZ, (TH2D*) h3->Project3DProfile("zy  UF OF")->ProjectionXY("4", "W"), options);
      status += equals("TH3 -> PWZX", (TH2D*) h2wZX, (TH2D*) h3->Project3DProfile("xz  UF OF")->ProjectionXY("5", "W"), options);
      status += equals("TH3 -> PWZY", (TH2D*) h2wZY, (TH2D*) h3->Project3DProfile("yz  UF OF")->ProjectionXY("6", "W"), options);
      options = 0;
      if ( defaultEqualOptions & cmpOptPrint )
         cout << "----------------------------------------------" << endl;
      
      // test 1D histograms
      options = cmpOptStats;
      // ProfileX re-use the same histo if sme name is given. 
      // need to give a diffrent name for each projectino (x,y,Z) otherwise we end-up in different bins
      // t.b.d: ProfileX make a new histo if non compatible
      status += equals("TH2XY -> PBX", h1X, (TH1D*) h2XY->ProfileX("PBX", 0,h2XY->GetYaxis()->GetNbins()+1)->ProjectionX("1", "B"),options  );
      status += equals("TH2XY -> PBY", h1Y, (TH1D*) h2XY->ProfileY("PBY", 0,h2XY->GetXaxis()->GetNbins()+1)->ProjectionX("1", "B"),options);
      status += equals("TH2XZ -> PBX", h1X, (TH1D*) h2XZ->ProfileX("PBX", 0,h2XZ->GetYaxis()->GetNbins()+1)->ProjectionX("1", "B"),options);
      status += equals("TH2XZ -> PBZ", h1Z, (TH1D*) h2XZ->ProfileY("PBZ", 0,h2XZ->GetXaxis()->GetNbins()+1)->ProjectionX("1", "B"),options,1E-12);
      status += equals("TH2YX -> PBY", h1Y, (TH1D*) h2YX->ProfileX("PBY", 0,h2YX->GetYaxis()->GetNbins()+1)->ProjectionX("1", "B"),options);
      status += equals("TH2YX -> PBX", h1X, (TH1D*) h2YX->ProfileY("PBX", 0,h2YX->GetXaxis()->GetNbins()+1)->ProjectionX("1", "B"),options);
      status += equals("TH2YZ -> PBY", h1Y, (TH1D*) h2YZ->ProfileX("PBY", 0,h2YZ->GetYaxis()->GetNbins()+1)->ProjectionX("1", "B"),options);
      status += equals("TH2YZ -> PBZ", h1Z, (TH1D*) h2YZ->ProfileY("PBZ", 0,h2YZ->GetXaxis()->GetNbins()+1)->ProjectionX("1", "B"),options,1E-12);
      status += equals("TH2ZX -> PBZ", h1Z, (TH1D*) h2ZX->ProfileX("PBZ", 0,h2ZX->GetYaxis()->GetNbins()+1)->ProjectionX("1", "B"),options,1E-12);
      status += equals("TH2ZX -> PBX", h1X, (TH1D*) h2ZX->ProfileY("PBX", 0,h2ZX->GetXaxis()->GetNbins()+1)->ProjectionX("1", "B"),options);
      status += equals("TH2ZY -> PBZ", h1Z, (TH1D*) h2ZY->ProfileX("PBZ", 0,h2ZY->GetYaxis()->GetNbins()+1)->ProjectionX("1", "B"),options,1E-12);
      status += equals("TH2ZY -> PBY", h1Y, (TH1D*) h2ZY->ProfileY("PBY", 0,h2ZY->GetXaxis()->GetNbins()+1)->ProjectionX("1", "B"),options);
      options = 0;
      if ( defaultEqualOptions & cmpOptPrint )
         cout << "----------------------------------------------" << endl;

      // 1D testing direct profiles 
      options = cmpOptStats;
      status += equals("TH2XY -> PX", pe1XY, (TH1D*) h2XY->ProfileX("PX", 0,h2XY->GetYaxis()->GetNbins()+1), options);
      status += equals("TH2XY -> PY", pe1YX, (TH1D*) h2XY->ProfileY("PY", 0,h2XY->GetXaxis()->GetNbins()+1), options);
      status += equals("TH2XZ -> PX", pe1XZ, (TH1D*) h2XZ->ProfileX("PX", 0,h2XZ->GetYaxis()->GetNbins()+1), options);
      status += equals("TH2XZ -> PZ", pe1ZX, (TH1D*) h2XZ->ProfileY("PZ", 0,h2XZ->GetXaxis()->GetNbins()+1), options);
      status += equals("TH2YX -> PY", pe1YX, (TH1D*) h2YX->ProfileX("PY", 0,h2YX->GetYaxis()->GetNbins()+1), options);
      status += equals("TH2YX -> PX", pe1XY, (TH1D*) h2YX->ProfileY("PX", 0,h2YX->GetXaxis()->GetNbins()+1), options);
      status += equals("TH2YZ -> PY", pe1YZ, (TH1D*) h2YZ->ProfileX("PY", 0,h2YZ->GetYaxis()->GetNbins()+1), options);
      status += equals("TH2YZ -> PZ", pe1ZY, (TH1D*) h2YZ->ProfileY("PZ", 0,h2YZ->GetXaxis()->GetNbins()+1), options);
      status += equals("TH2ZX -> PZ", pe1ZX, (TH1D*) h2ZX->ProfileX("PZ", 0,h2ZX->GetYaxis()->GetNbins()+1), options);
      status += equals("TH2ZX -> PX", pe1XZ, (TH1D*) h2ZX->ProfileY("PX", 0,h2ZX->GetXaxis()->GetNbins()+1), options);
      status += equals("TH2ZY -> PZ", pe1ZY, (TH1D*) h2ZY->ProfileX("PZ", 0,h2ZY->GetYaxis()->GetNbins()+1), options);
      status += equals("TH2ZY -> PY", pe1YZ, (TH1D*) h2ZY->ProfileY("PY", 0,h2ZY->GetXaxis()->GetNbins()+1), options);
      options = 0;
      if ( defaultEqualOptions & cmpOptPrint )
         cout << "----------------------------------------------" << endl;

      // 1D testing e profiles
      options = 0;
      status += equals("TH2XY -> PEX", pe1XY, 
                       (TH1D*) h2XY->ProfileX("PEX", 0,h2XY->GetYaxis()->GetNbins()+1)->ProjectionX("1", "E"), options);
      status += equals("TH2XY -> PEY", pe1YX, 
                       (TH1D*) h2XY->ProfileY("PEY", 0,h2XY->GetXaxis()->GetNbins()+1)->ProjectionX("1", "E"), options);
      status += equals("TH2XZ -> PEX", pe1XZ, 
                       (TH1D*) h2XZ->ProfileX("PEX", 0,h2XZ->GetYaxis()->GetNbins()+1)->ProjectionX("1", "E"), options);
      status += equals("TH2XZ -> PEZ", pe1ZX, 
                       (TH1D*) h2XZ->ProfileY("PEZ", 0,h2XZ->GetXaxis()->GetNbins()+1)->ProjectionX("1", "E"), options);
      status += equals("TH2YX -> PEY", pe1YX, 
                       (TH1D*) h2YX->ProfileX("PEY", 0,h2YX->GetYaxis()->GetNbins()+1)->ProjectionX("1", "E"), options);
      status += equals("TH2YX -> PEX", pe1XY, 
                       (TH1D*) h2YX->ProfileY("PEX", 0,h2YX->GetXaxis()->GetNbins()+1)->ProjectionX("1", "E"), options);
      status += equals("TH2YZ -> PEY", pe1YZ, 
                       (TH1D*) h2YZ->ProfileX("PEY", 0,h2YZ->GetYaxis()->GetNbins()+1)->ProjectionX("1", "E"), options);
      status += equals("TH2YZ -> PEZ", pe1ZY, 
                       (TH1D*) h2YZ->ProfileY("PEZ", 0,h2YZ->GetXaxis()->GetNbins()+1)->ProjectionX("1", "E"), options);
      status += equals("TH2ZX -> PEZ", pe1ZX, 
                       (TH1D*) h2ZX->ProfileX("PEZ", 0,h2ZX->GetYaxis()->GetNbins()+1)->ProjectionX("1", "E"), options);
      status += equals("TH2ZX -> PEX", pe1XZ, 
                       (TH1D*) h2ZX->ProfileY("PEX", 0,h2ZX->GetXaxis()->GetNbins()+1)->ProjectionX("1", "E"), options);
      status += equals("TH2ZY -> PEZ", pe1ZY, 
                       (TH1D*) h2ZY->ProfileX("PEZ", 0,h2ZY->GetYaxis()->GetNbins()+1)->ProjectionX("1", "E"), options);
      status += equals("TH2ZY -> PEY", pe1YZ, 
                       (TH1D*) h2ZY->ProfileY("PEY", 0,h2ZY->GetXaxis()->GetNbins()+1)->ProjectionX("1", "E"), options);
      options = 0;
      if ( defaultEqualOptions & cmpOptPrint )
         cout << "----------------------------------------------" << endl;

      // 1D testing w profiles
      // The error is not properly propagated when build with weights :S
      if ( buildWithWeights ) options = cmpOptNoError;
      status += equals("TH2XY -> PWX", hw1XY, 
                       (TH1D*) h2XY->ProfileX("PWX", 0,h2XY->GetYaxis()->GetNbins()+1)->ProjectionX("1", "W"), options);
      status += equals("TH2XY -> PWY", hw1YX, 
                       (TH1D*) h2XY->ProfileY("PWY", 0,h2XY->GetXaxis()->GetNbins()+1)->ProjectionX("1", "W"), options);
      status += equals("TH2XZ -> PWX", hw1XZ, 
                       (TH1D*) h2XZ->ProfileX("PWX", 0,h2XZ->GetYaxis()->GetNbins()+1)->ProjectionX("1", "W"), options);
      status += equals("TH2XZ -> PWZ", hw1ZX, 
                       (TH1D*) h2XZ->ProfileY("PWZ", 0,h2XZ->GetXaxis()->GetNbins()+1)->ProjectionX("1", "W"), options);
      status += equals("TH2YX -> PWY", hw1YX, 
                       (TH1D*) h2YX->ProfileX("PWY", 0,h2YX->GetYaxis()->GetNbins()+1)->ProjectionX("1", "W"), options);
      status += equals("TH2YX -> PWX", hw1XY, 
                       (TH1D*) h2YX->ProfileY("PWX", 0,h2YX->GetXaxis()->GetNbins()+1)->ProjectionX("1", "W"), options);
      status += equals("TH2YZ -> PWY", hw1YZ, 
                       (TH1D*) h2YZ->ProfileX("PWY", 0,h2YZ->GetYaxis()->GetNbins()+1)->ProjectionX("1", "W"), options);
      status += equals("TH2YZ -> PWZ", hw1ZY, 
                       (TH1D*) h2YZ->ProfileY("PWZ", 0,h2YZ->GetXaxis()->GetNbins()+1)->ProjectionX("1", "W"), options);
      status += equals("TH2ZX -> PWZ", hw1ZX, 
                       (TH1D*) h2ZX->ProfileX("PWZ", 0,h2ZX->GetYaxis()->GetNbins()+1)->ProjectionX("1", "W"), options);
      status += equals("TH2ZX -> PWX", hw1XZ, 
                       (TH1D*) h2ZX->ProfileY("PWX", 0,h2ZX->GetXaxis()->GetNbins()+1)->ProjectionX("1", "W"), options);
      status += equals("TH2ZY -> PWZ", hw1ZY, 
                       (TH1D*) h2ZY->ProfileX("PWZ", 0,h2ZY->GetYaxis()->GetNbins()+1)->ProjectionX("1", "W"), options);
      status += equals("TH2ZY -> PWY", hw1YZ, 
                       (TH1D*) h2ZY->ProfileY("PWY", 0,h2ZY->GetXaxis()->GetNbins()+1)->ProjectionX("1", "W"), options);

      options = 0;
      if ( defaultEqualOptions & cmpOptPrint )
         cout << "----------------------------------------------" << endl;
      
      // TH2 derived from STH3
      options = cmpOptStats;
      status += equals("STH3 -> XY", h2XY, (TH2D*) s3->Projection(1,0), options);
      status += equals("STH3 -> XZ", h2XZ, (TH2D*) s3->Projection(2,0), options);
      status += equals("STH3 -> YX", h2YX, (TH2D*) s3->Projection(0,1), options);
      status += equals("STH3 -> YZ", h2YZ, (TH2D*) s3->Projection(2,1), options);
      status += equals("STH3 -> ZX", h2ZX, (TH2D*) s3->Projection(0,2), options); 
      status += equals("STH3 -> ZY", h2ZY, (TH2D*) s3->Projection(1,2), options); 
      options = 0;
      if ( defaultEqualOptions & cmpOptPrint )
         cout << "----------------------------------------------" << endl;

      // TH1 derived from STH3
      options = cmpOptStats;
      status += equals("STH3 -> X", h1X, (TH1D*) s3->Projection(0), options);
      status += equals("STH3 -> Y", h1Y, (TH1D*) s3->Projection(1), options);
      status += equals("STH3 -> Z", h1Z, (TH1D*) s3->Projection(2), options);
      options = 0;
      if ( defaultEqualOptions & cmpOptPrint )
         cout << "----------------------------------------------" << endl;

      return status;
   }
   
};

int stressHistogram()
{
#ifdef R__WIN32
   // On windows there is an order of initialization problem that lead to 
   // 'Int_t not being in the list of types when TProfile's TClass is 
   // initialized (via a call to IsA()->InheritsFrom(); on linux this is
   // not a problem because G__Base1 is initialized early; on windows with
   // root.exe this is not a problem because GetListOfType(kTRUE) is called
   // via a call to TClass::GetClass induces by the initialization of the 
   // plugin manager.
   gROOT->GetListOfTypes(kTRUE);
#endif
   r.SetSeed(initialSeed);

   int GlobalStatus = false;
   int status = false;

   TBenchmark bm;
   bm.Start("stressHistogram");

   cout << "****************************************************************************" <<endl;
   cout << "*  Starting  stress  H I S T O G R A M                                     *" <<endl;
   cout << "****************************************************************************" <<endl;

   // Test 1
   if ( defaultEqualOptions & cmpOptPrint )
      cout << "**********************************\n"
           << "       Test without weights       \n" 
           << "**********************************\n"
           << endl;


   TH1::SetDefaultSumw2(); 
   // to avoid cases in chi2-test of profiles when error is zero  
   TProfile::Approximate();
   TProfile2D::Approximate();
   TProfile3D::Approximate();


   
   ProjectionTester* ht = new ProjectionTester();
   ht->buildHistograms();
   //Ht->buildHistograms(2,4,5,6,8,10);
   status = ht->compareHistograms();
   GlobalStatus += status;
   printResult("Testing Projections without weights..............................", status);
   delete ht;

   // Test 2
   if ( defaultEqualOptions & cmpOptPrint )
      cout << "**********************************\n"
           << "        Test with weights         \n" 
           << "**********************************\n"
           << endl;
   
   ProjectionTester* ht2 = new ProjectionTester();
   ht2->buildHistogramsWithWeights();
   status = ht2->compareHistograms();
   GlobalStatus += status;
   printResult("Testing Projections with weights.................................", status);
   delete ht2;
   
   // Test 3
   // Range Tests
   const unsigned int numberOfRange = 3;
   pointer2Test rangeTestPointer[numberOfRange] = { testTH2toTH1,
                                                    testTH3toTH1,
                                                    testTH3toTH2
   };
   struct TTestSuite rangeTestSuite = { numberOfRange, 
                                        "Projection with Range for Histograms and Profiles................",
                                        rangeTestPointer };

  // Test 4
   const unsigned int numberOfRebin = 8;
   pointer2Test rebinTestPointer[numberOfRebin] = { testIntegerRebin,       testIntegerRebinProfile,
                                                    testIntegerRebinNoName, testIntegerRebinNoNameProfile,
                                                    testArrayRebin,         testArrayRebinProfile,
                                                    test2DRebin,
                                                    testSparseRebin1};
   struct TTestSuite rebinTestSuite = { numberOfRebin, 
                                        "Histogram Rebinning..............................................",
                                        rebinTestPointer };

   // Test 5
   // Add Tests
   const unsigned int numberOfAdds = 20;
   pointer2Test addTestPointer[numberOfAdds] = { testAdd1,    testAddProfile1, 
                                                 testAdd2,    testAddProfile2,
                                                 testAdd3,   
                                                 testAddVar1, testAddVarProf1, 
                                                 testAddVar2, testAddVarProf2,
                                                 testAdd2D3,
                                                 testAdd3D3,
                                                 testAdd2D1,  testAdd2DProfile1,
                                                 testAdd2D2,  testAdd2DProfile2,
                                                 testAdd3D1,  testAdd3DProfile1,
                                                 testAdd3D2,  testAdd3DProfile2,
                                                 testAddSparse
   };
   struct TTestSuite addTestSuite = { numberOfAdds, 
                                      "Add tests for 1D, 2D and 3D Histograms and Profiles..............",
                                      addTestPointer };

   // Test 6
   // Multiply Tests
   const unsigned int numberOfMultiply = 9;
   pointer2Test multiplyTestPointer[numberOfMultiply] = { testMul1,    testMul2,
                                                          testMulVar1, testMulVar2,
                                                          testMul2D1,  testMul2D2,
                                                          testMul3D1,  testMul3D2,
                                                          testMulSparse
   };
   struct TTestSuite multiplyTestSuite = { numberOfMultiply, 
                                           "Multiply tests for 1D, 2D and 3D Histograms......................",
                                           multiplyTestPointer };

   // Test 7
   // Divide Tests
   const unsigned int numberOfDivide = 8;
   pointer2Test divideTestPointer[numberOfDivide] = { testDivide1,     testDivide2,
                                                      testDivideVar1,  testDivideVar2,
                                                      testDivide2D1,   testDivide2D2,
                                                      testDivide3D1,   testDivide3D2
   };
   struct TTestSuite divideTestSuite = { numberOfDivide, 
                                         "Divide tests for 1D, 2D and 3D Histograms........................",
                                         divideTestPointer };

   // Still to do: Division for profiles

   // The division methods for the profiles have to be changed to
   // calculate the errors correctly.

   // Test 8
   // Copy Tests
   const unsigned int numberOfCopy = 25;
   pointer2Test copyTestPointer[numberOfCopy] = { testAssign1D,             testAssignProfile1D, 
                                                  testAssignVar1D,          testAssignProfileVar1D, 
                                                  testCopyConstructor1D,    testCopyConstructorProfile1D, 
                                                  testCopyConstructorVar1D, testCopyConstructorProfileVar1D, 
                                                  testClone1D,              testCloneProfile1D,
                                                  testCloneVar1D,           testCloneProfileVar1D,
                                                  testAssign2D,             testAssignProfile2D,
                                                  testCopyConstructor2D,    testCopyConstructorProfile2D,
                                                  testClone2D,              testCloneProfile2D,
                                                  testAssign3D,             testAssignProfile3D,
                                                  testCopyConstructor3D,    testCopyConstructorProfile3D,
                                                  testClone3D,              testCloneProfile3D,
                                                  testCloneSparse
   };
   struct TTestSuite copyTestSuite = { numberOfCopy, 
                                       "Copy tests for 1D, 2D and 3D Histograms and Profiles.............",
                                       copyTestPointer };

   // Test 9
   // WriteRead Tests
   const unsigned int numberOfReadwrite = 9;
   pointer2Test readwriteTestPointer[numberOfReadwrite] = { testWriteRead1D,     testWriteReadProfile1D,
                                                            testWriteReadVar1D,  testWriteReadProfileVar1D,
                                                            testWriteRead2D,     testWriteReadProfile2D,
                                                            testWriteRead3D,     testWriteReadProfile3D, 
                                                            testWriteReadSparse
   };
   struct TTestSuite readwriteTestSuite = { numberOfReadwrite, 
                                            "Read/Write tests for 1D, 2D and 3D Histograms and Profiles.......",
                                            readwriteTestPointer };

   // Test 10
   // Merge Tests
   const unsigned int numberOfMerge = 34;
   pointer2Test mergeTestPointer[numberOfMerge] = { testMerge1D,                 testMergeProf1D,
                                                    testMergeVar1D,              testMergeProfVar1D,
                                                    testMerge2D,                 testMergeProf2D,
                                                    testMerge3D,                 testMergeProf3D,
                                                    testMergeSparse,
                                                    testMerge1DLabelSame,        testMergeProf1DLabelSame,
                                                    testMerge2DLabelSame,        testMergeProf2DLabelSame,
                                                    testMerge3DLabelSame,        testMergeProf3DLabelSame,

                                                    /*testMerge1DLabelDiff,*/    testMergeProf1DLabelDiff,
                                                    testMerge2DLabelDiff,        testMergeProf2DLabelDiff,
                                                    testMerge3DLabelDiff,        testMergeProf3DLabelDiff,
                                                    testMerge1DLabelAll,         testMergeProf1DLabelAll,
                                                    testMerge2DLabelAll,         testMergeProf2DLabelAll,
                                                    testMerge3DLabelAll,         testMergeProf3DLabelAll,
                                                    /*testMerge1DLabelAllDiff*/  testMergeProf1DLabelAllDiff,
                                                    testMerge2DLabelAllDiff,     testMergeProf2DLabelAllDiff,
                                                    testMerge3DLabelAllDiff,     testMergeProf3DLabelAllDiff,
                                                    testMerge1DDiff,             testMergeProf1DDiff,
                                                    testMerge2DDiff/*,             testMergeProf2DDiff,*/
                                                    /*testMerge3DDiff,*/         /*testMergeProf3DDiff*/
   };
   struct TTestSuite mergeTestSuite = { numberOfMerge, 
                                        "Merge tests for 1D, 2D and 3D Histograms and Profiles............",
                                        mergeTestPointer };
   // Test 11
   // Label Tests
   const unsigned int numberOfLabel = 2;
   pointer2Test labelTestPointer[numberOfLabel] = { testLabel,
                                                    testLabelsInflateProf1D
   };
   struct TTestSuite labelTestSuite = { numberOfLabel, 
                                        "Label tests for 1D Histograms (TAxis)............................",
                                        labelTestPointer };

   // Test 12
   // Interpolation Tests
   const unsigned int numberOfInterpolation = 4;
   pointer2Test interpolationTestPointer[numberOfInterpolation] = { testInterpolation1D,
                                                                    testInterpolationVar1D,
                                                                    testInterpolation2D, 
                                                                    testInterpolation3D
   };
   struct TTestSuite interpolationTestSuite = { numberOfInterpolation, 
                                                "Interpolation tests for Histograms...............................",
                                                interpolationTestPointer };

   // Test 13
   // Scale Tests
   const unsigned int numberOfScale = 3;
   pointer2Test scaleTestPointer[numberOfScale] = { testScale1DProf,
                                                    testScale2DProf,
                                                    testScale3DProf
   };
   struct TTestSuite scaleTestSuite = { numberOfScale, 
                                        "Scale tests for Profiles.........................................",
                                        scaleTestPointer };

   // Combination of tests
   const unsigned int numberOfSuits = 11;
   struct TTestSuite* testSuite[numberOfSuits];
   testSuite[ 0] = &rangeTestSuite;
   testSuite[ 1] = &rebinTestSuite;
   testSuite[ 2] = &addTestSuite;
   testSuite[ 3] = &multiplyTestSuite;
   testSuite[ 4] = &divideTestSuite;
   testSuite[ 5] = &copyTestSuite;
   testSuite[ 6] = &readwriteTestSuite;
   testSuite[ 7] = &mergeTestSuite;
   testSuite[ 8] = &labelTestSuite;
   testSuite[ 9] = &interpolationTestSuite;
   testSuite[10] = &scaleTestSuite;

   status = 0;
   for ( unsigned int i = 0; i < numberOfSuits; ++i ) {
      bool internalStatus = false;
//       #pragma omp parallel
//       #pragma omp for reduction(|: internalStatus)
      for ( unsigned int j = 0; j < testSuite[i]->nTests; ++j ) {
         internalStatus |= testSuite[i]->tests[j]();
      }
      printResult( testSuite[i]->suiteName, internalStatus);
      status += internalStatus;
   }
   GlobalStatus += status;

   // Test 14
   // Reference Tests
   const unsigned int numberOfRefRead = 7;
   pointer2Test refReadTestPointer[numberOfRefRead] = { testRefRead1D,  testRefReadProf1D,
                                                        testRefRead2D,  testRefReadProf2D,
                                                        testRefRead3D,  testRefReadProf3D,
                                                        testRefReadSparse
   };
   struct TTestSuite refReadTestSuite = { numberOfRefRead, 
                                          "Reference File Read for Histograms and Profiles..................",
                                          refReadTestPointer };
   

   if ( refFileOption == refFileWrite ) {
      refFile = TFile::Open(refFileName, "RECREATE");
   }
   else {
      refFile = TFile::Open(refFileName);
   }

   if ( refFile != 0 ) {
      r.SetSeed(8652);
      status = 0;
      for ( unsigned int j = 0; j < refReadTestSuite.nTests; ++j ) {
         status += refReadTestSuite.tests[j]();
      }
      printResult( refReadTestSuite.suiteName, status);
      GlobalStatus += status;
   } else {
      Warning("stressHistogram", "No reference file found");
   }

   bm.Stop("stressHistogram");
   std::cout <<"****************************************************************************\n";
   bm.Print("stressHistogram");
   const double reftime = 32; // needs to be updated // ref time on  pcbrun4
   double rootmarks = 860 * reftime / bm.GetCpuTime("stressHistogram");
   std::cout << " ROOTMARKS = " << rootmarks << " ROOT version: " << gROOT->GetVersion() << "\t" 
             << gROOT->GetSvnBranch() << "@" << gROOT->GetSvnRevision() << std::endl;
   std::cout <<"****************************************************************************\n";

   return GlobalStatus;
}

ostream& operator<<(ostream& out, TH1D* h)
{
   out << h->GetName() << ": [" << h->GetBinContent(1);
   for ( Int_t i = 1; i < h->GetNbinsX(); ++i )
      out << ", " << h->GetBinContent(i);
   out << "] ";

   return out;
}

void printResult(const char* msg, bool status)
{
   static int counter = 1;
   cout << "Test ";
   cout.width(2);
   cout<< counter << ": "
       << msg
       << (status?"FAILED":"OK") << endl;
   counter += 1;
}

void FillVariableRange(Double_t v[numberOfBins+1])
{
   //Double_t v[numberOfBins+1];
   Double_t minLimit = (maxRange-minRange)  / (numberOfBins*2);
   Double_t maxLimit = (maxRange-minRange)*4/ (numberOfBins);   
   v[0] = 0;
   for ( Int_t i = 1; i < numberOfBins + 1; ++i )
   {
      Double_t limit = r.Uniform(minLimit, maxLimit);
      v[i] = v[i-1] + limit;
   }
   
   Double_t k = (maxRange-minRange)/v[numberOfBins];
   for ( Int_t i = 0; i < numberOfBins + 1; ++i )
   {
      v[i] = v[i] * k + minRange;
   }
}

void FillHistograms(TH1D* h1, TH1D* h2, Double_t c1, Double_t c2)
{
   for ( Int_t e = 0; e < nEvents; ++e ) {
      Double_t value = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      h1->Fill(value, c1);
      h2->Fill(value, c2);
   }
}

void FillProfiles(TProfile* p1, TProfile* p2, Double_t c1, Double_t c2)
{
   for ( Int_t e = 0; e < nEvents; ++e ) {
      Double_t x = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      Double_t y = r.Uniform(0.9 * minRange, 1.1 * maxRange);
      p1->Fill(x, y, c1);
      p2->Fill(x, y, c2);
   }
}

// Methods for histogram comparisions

int equals(const char* msg, THnSparse* h1, THnSparse* h2, int options, double ERRORLIMIT)
{
   options = options | defaultEqualOptions;
   bool print = options & cmpOptPrint;
   bool debug = options & cmpOptDebug;
   bool compareError = ! (options & cmpOptNoError);
   
   int differents = 0;
   
   for ( int i = 0; i <= h1->GetAxis(0)->GetNbins() + 1; ++i )
      for ( int j = 0; j <= h1->GetAxis(1)->GetNbins() + 1; ++j )
         for ( int h = 0; h <= h1->GetAxis(2)->GetNbins() + 1; ++h )
         {
            Double_t x = h1->GetAxis(0)->GetBinCenter(i);
            Double_t y = h1->GetAxis(1)->GetBinCenter(j);
            Double_t z = h1->GetAxis(2)->GetBinCenter(h);
            
            Int_t bin[3] = {i, j, h};
            
            if (debug) {
               cout << equals(x, h2->GetAxis(0)->GetBinCenter(i), ERRORLIMIT) << " "
                    << equals(y, h2->GetAxis(1)->GetBinCenter(j), ERRORLIMIT) << " "
                    << equals(z, h2->GetAxis(2)->GetBinCenter(h), ERRORLIMIT) << " "
                    << "[" << x << "," << y << "," << z << "]: " 
                    << h1->GetBinContent(bin) << " +/- " << h1->GetBinError(bin) << " | "
                    << h2->GetBinContent(bin) << " +/- " << h2->GetBinError(bin)
                    << " | " << equals(h1->GetBinContent(bin), h2->GetBinContent(bin), ERRORLIMIT)
                    << " "   << equals(h1->GetBinError(bin)  , h2->GetBinError(bin),   ERRORLIMIT)
                    << " "   << differents
                    << " "   << (fabs(h1->GetBinContent(bin) - h2->GetBinContent(bin)))
                    << endl;
            }
            differents += equals(x, h2->GetAxis(0)->GetBinCenter(i), ERRORLIMIT);
            differents += equals(y, h2->GetAxis(1)->GetBinCenter(j), ERRORLIMIT);
            differents += equals(z, h2->GetAxis(2)->GetBinCenter(h), ERRORLIMIT);
            differents += equals(h1->GetBinContent(bin), h2->GetBinContent(bin), ERRORLIMIT);
            if ( compareError )
               differents += equals(h1->GetBinError(bin)  , h2->GetBinError(bin), ERRORLIMIT);
         }
   
   // Statistical tests:
   // No statistical tests possible for THnSparse so far...
//    if ( compareStats )
//       differents += compareStatistics( h1, h2, debug, ERRORLIMIT);
   
   if ( print || debug ) cout << msg << ": \t" << (differents?"FAILED":"OK") << endl;
   
   delete h2;
   
   return differents;
}

int equals(const char* msg, TH3D* h1, TH3D* h2, int options, double ERRORLIMIT)
{
   options = options | defaultEqualOptions;
   bool print = options & cmpOptPrint;
   bool debug = options & cmpOptDebug;
   bool compareError = ! (options & cmpOptNoError);
   bool compareStats = options & cmpOptStats;
   
   int differents = ( h1 == h2 ); // Check they are not the same histogram!
   if (debug) {
      cout << static_cast<void*>(h1) << " " << static_cast<void*>(h2) << " "
           << (h1 == h2 ) << " " << differents << endl;
   }
   
   for ( int i = 0; i <= h1->GetNbinsX() + 1; ++i )
      for ( int j = 0; j <= h1->GetNbinsY() + 1; ++j )
         for ( int h = 0; h <= h1->GetNbinsY() + 1; ++h )
      {
         Double_t x = h1->GetXaxis()->GetBinCenter(i);
         Double_t y = h1->GetYaxis()->GetBinCenter(j);
         Double_t z = h1->GetZaxis()->GetBinCenter(h);
         
         if (debug)
         {
            cout << equals(x, h2->GetXaxis()->GetBinCenter(i), ERRORLIMIT) << " "
                 << equals(y, h2->GetYaxis()->GetBinCenter(j), ERRORLIMIT) << " "
                 << equals(z, h2->GetZaxis()->GetBinCenter(h), ERRORLIMIT) << " "
                 << "[" << x << "," << y << "," << z << "]: " 
                 << h1->GetBinContent(i,j,h) << " +/- " << h1->GetBinError(i,j,h) << " | "
                 << h2->GetBinContent(i,j,h) << " +/- " << h2->GetBinError(i,j,h)
                 << " | " << equals(h1->GetBinContent(i,j,h), h2->GetBinContent(i,j,h), ERRORLIMIT)
                 << " "   << equals(h1->GetBinError(i,j,h)  , h2->GetBinError(i,j,h),   ERRORLIMIT)
                 << " "   << differents
                 << " "   << (fabs(h1->GetBinContent(i,j,h) - h2->GetBinContent(i,j,h)))
                 << endl;
         }
         differents += (bool) equals(x, h2->GetXaxis()->GetBinCenter(i), ERRORLIMIT);
         differents += (bool) equals(y, h2->GetYaxis()->GetBinCenter(j), ERRORLIMIT);
         differents += (bool) equals(z, h2->GetZaxis()->GetBinCenter(h), ERRORLIMIT);
         differents += (bool) equals(h1->GetBinContent(i,j,h), h2->GetBinContent(i,j,h), ERRORLIMIT);
         if ( compareError )
            differents += (bool) equals(h1->GetBinError(i,j,h)  , h2->GetBinError(i,j,h), ERRORLIMIT);
      }
   
   // Statistical tests:
   if ( compareStats )
      differents += compareStatistics( h1, h2, debug, ERRORLIMIT);
   
   if ( print || debug ) cout << msg << ": \t" << (differents?"FAILED":"OK") << endl;
   
   delete h2;
   
   return differents;
}

int equals(const char* msg, TH2D* h1, TH2D* h2, int options, double ERRORLIMIT)
{
   options = options | defaultEqualOptions;
   bool print = options & cmpOptPrint;
   bool debug = options & cmpOptDebug;
   bool compareError = ! (options & cmpOptNoError);
   bool compareStats = options & cmpOptStats;
   
   int differents = ( h1 == h2 ); // Check they are not the same histogram!
   if (debug) {
      cout << static_cast<void*>(h1) << " " << static_cast<void*>(h2) << " "
           << (h1 == h2 ) << " " << differents << endl;
   }

   for ( int i = 0; i <= h1->GetNbinsX() + 1; ++i )
      for ( int j = 0; j <= h1->GetNbinsY() + 1; ++j )
      {
         Double_t x = h1->GetXaxis()->GetBinCenter(i);
         Double_t y = h1->GetYaxis()->GetBinCenter(j);
         
         if (debug)
         {
            cout << equals(x, h2->GetXaxis()->GetBinCenter(i), ERRORLIMIT) << " "
                 << equals(y, h2->GetYaxis()->GetBinCenter(j), ERRORLIMIT) << " "
                 << "[" << x << "," << y << "]: " 
                 << h1->GetBinContent(i,j) << " +/- " << h1->GetBinError(i,j) << " | "
                 << h2->GetBinContent(i,j) << " +/- " << h2->GetBinError(i,j)
                 << " | " << equals(h1->GetBinContent(i,j), h2->GetBinContent(i,j), ERRORLIMIT)
                 << " "   << equals(h1->GetBinError(i,j)  , h2->GetBinError(i,j),   ERRORLIMIT)
                 << " "   << differents
                 << " "   << (fabs(h1->GetBinContent(i,j) - h2->GetBinContent(i,j)))
                 << endl;
         }
         differents += (bool) equals(x, h2->GetXaxis()->GetBinCenter(i), ERRORLIMIT);
         differents += (bool) equals(y, h2->GetYaxis()->GetBinCenter(j), ERRORLIMIT);
         differents += (bool) equals(h1->GetBinContent(i,j), h2->GetBinContent(i,j), ERRORLIMIT);
         if ( compareError )
            differents += (bool) equals(h1->GetBinError(i,j)  , h2->GetBinError(i,j), ERRORLIMIT);
      }
   
   // Statistical tests:
   if ( compareStats )
      differents += compareStatistics( h1, h2, debug, ERRORLIMIT);
   
   if ( print || debug ) cout << msg << ": \t" << (differents?"FAILED":"OK") << endl;
   
   delete h2;
   
   return differents;
}

int equals(const char* msg, TH1D* h1, TH1D* h2, int options, double ERRORLIMIT)
{
   options = options | defaultEqualOptions;
   bool print = options & cmpOptPrint;
   bool debug = options & cmpOptDebug;
   bool compareError = ! (options & cmpOptNoError);
   bool compareStats = options & cmpOptStats;
   

   if (debug) { 
      cout << "Nbins  = " << h1->GetXaxis()->GetNbins() << " ,  " <<  h2->GetXaxis()->GetNbins() << endl;
   }

   int differents = ( h1 == h2 ); // Check they are not the same histogram!
   if (debug) {
      cout << static_cast<void*>(h1) << " " << static_cast<void*>(h2) << " "
           << (h1 == h2 ) << " " << differents << endl;
   }

   for ( int i = 0; i <= h1->GetNbinsX() + 1; ++i )
   {
      Double_t x = h1->GetXaxis()->GetBinCenter(i);
      if ( debug )
      {
         cout << equals(x, h2->GetXaxis()->GetBinCenter(i), ERRORLIMIT)
              << " [" << x << "]: " 
              << h1->GetBinContent(i) << " +/- " << h1->GetBinError(i) << " | "
              << h2->GetBinContent(i) << " +/- " << h2->GetBinError(i)
              << " | " << equals(h1->GetBinContent(i), h2->GetBinContent(i), ERRORLIMIT)
              << " "   << equals(h1->GetBinError(i),   h2->GetBinError(i),   ERRORLIMIT)
              << " "   << differents
              << endl;
      }
      differents += (bool) equals(x, h2->GetXaxis()->GetBinCenter(i), ERRORLIMIT);
      differents += (bool) equals(h1->GetBinContent(i), h2->GetBinContent(i), ERRORLIMIT);
      
      if ( compareError )
         differents += (bool) equals(h1->GetBinError(i),   h2->GetBinError(i), ERRORLIMIT);
   }
   
   // Statistical tests:
   if ( compareStats )
      differents += compareStatistics( h1, h2, debug, ERRORLIMIT);
   
   if ( print || debug ) cout << msg << ": \t" << (differents?"FAILED":"OK") << endl;
   
   delete h2;
   
   return differents;      
}

int equals(Double_t n1, Double_t n2, double ERRORLIMIT)
{
   return fabs( n1 - n2 ) > ERRORLIMIT * fabs(n1);
}

int compareStatistics( TH1* h1, TH1* h2, bool debug, double ERRORLIMIT)
{
   int differents = 0;

   int pr = std::cout.precision(12);


   int precLevel = gErrorIgnoreLevel; 
   // switch off Info mesaage from chi2 test
   if (!debug) gErrorIgnoreLevel = 1001; 
            
   if (debug) h2->Print(); 
   
   std::string option = "WW OF UF";
   const char * opt = option.c_str(); 

   double chi_12 = h1->Chi2Test(h2, opt);
   double chi_21 = h2->Chi2Test(h1, opt);

   differents += (bool) equals(chi_12, 1, ERRORLIMIT);
   differents += (bool) equals(chi_21, 1, ERRORLIMIT);  
   differents += (bool) equals(chi_12, chi_21, ERRORLIMIT);
   if ( debug )
      cout << "Chi2Test " << chi_12 << " " <<  chi_21 
           << " | " << differents
           << endl;

   if (!debug) gErrorIgnoreLevel = precLevel; 

   // Mean
   differents += (bool) equals(h1->GetMean(1), h2->GetMean(1), ERRORLIMIT);
   if ( debug )
      cout << "Mean: " << h1->GetMean(1) << " " << h2->GetMean(1) 
           << " | " << fabs( h1->GetMean(1) - h2->GetMean(1) ) 
           << " " << differents
           << endl;
   
   // RMS
   differents += (bool) equals( h1->GetRMS(1), h2->GetRMS(1), ERRORLIMIT);
   if ( debug )
      cout << "RMS: " << h1->GetRMS(1) << " " << h2->GetRMS(1) 
           << " | " << fabs( h1->GetRMS(1) - h2->GetRMS(1) ) 
           << " " << differents
           << endl;  

   // Number of Entries
   // check if is an unweighted histogram compare entries otherwise only effective entries
   if (h1->GetEntries() == h1->GetEffectiveEntries() ) { 

      differents += (bool) equals( h1->GetEntries(), h2->GetEntries(), 100*ERRORLIMIT);
      if ( debug )
         cout << "Entries: " << h1->GetEntries() << " " << h2->GetEntries() 
              << " | " << fabs( h1->GetEntries() - h2->GetEntries() ) 
              << " " << differents
              << endl;  
   }

   // Number of Effective Entries
   differents += (bool) equals( h1->GetEffectiveEntries(), h2->GetEffectiveEntries(), 100*ERRORLIMIT);
   if ( debug )
      cout << "Eff Entries: " << h1->GetEffectiveEntries() << " " << h2->GetEffectiveEntries() 
           << " | " << fabs( h1->GetEffectiveEntries() - h2->GetEffectiveEntries() ) 
           << " " << differents
           << endl;  

   std::cout.precision(pr);   

   return differents;
}

int main(int argc, char** argv)
{
   TApplication* theApp = 0;

   TH1::SetDefaultSumw2();

   if ( __DRAW__ )
      theApp = new TApplication("App",&argc,argv);

   int ret = stressHistogram();

   if ( __DRAW__ ) {
      theApp->Run();
      delete theApp;
      theApp = 0;
   }

   return ret;
}
