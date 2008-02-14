// @(#)root/physics:$Id$
// Author: Pasha Murat   12/02/99

/*************************************************************************
 * Copyright (C) 1995-2000, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#include "TROOT.h"
#include "TVector2.h"
#include "TClass.h"

Double_t const  kPI        = TMath::Pi();
Double_t const  kTWOPI     = 2.*kPI;

ClassImp(TVector2)

//______________________________________________________________________________
TVector2::TVector2()
{
   //constructor
   fX = 0.;
   fY = 0.;
}

//______________________________________________________________________________
TVector2::TVector2(Double_t *v)
{
   //constructor
   fX = v[0];
   fY = v[1];
}

//______________________________________________________________________________
TVector2::TVector2(Double_t x0, Double_t y0)
{
   //constructor
   fX = x0;
   fY = y0;
}

//______________________________________________________________________________
TVector2::~TVector2()
{
}


//______________________________________________________________________________
Double_t TVector2::Phi_0_2pi(Double_t x) {
   // (static function) returns phi angle in the interval [0,2*PI)
   if(TMath::IsNaN(x)){
      gROOT->Error("TVector2::Phi_0_2pi","function called with NaN");
      return x;
   }
   while (x >= kTWOPI) x -= kTWOPI;
   while (x <     0.)  x += kTWOPI;
   return x;
}

//______________________________________________________________________________
Double_t TVector2::Phi_mpi_pi(Double_t x) {
   // (static function) returns phi angle in the interval [-PI,PI)
   if(TMath::IsNaN(x)){
      gROOT->Error("TVector2::Phi_mpi_pi","function called with NaN");
      return x;
   }
   while (x >= kPI) x -= kTWOPI;
   while (x < -kPI) x += kTWOPI;
   return x;
}

//______________________________________________________________________________
TVector2 TVector2::Rotate (Double_t phi) const
{
   //rotation by phi
   return TVector2( fX*TMath::Cos(phi)-fY*TMath::Sin(phi), fX*TMath::Sin(phi)+fY*TMath::Cos(phi) );
}


//______________________________________________________________________________
void TVector2::Streamer(TBuffer &R__b)
{
   // Stream an object of class TVector2.

   if (R__b.IsReading()) {
      UInt_t R__s, R__c;
      Version_t R__v = R__b.ReadVersion(&R__s, &R__c);
      if (R__v > 2) {
         TVector2::Class()->ReadBuffer(R__b, this, R__v, R__s, R__c);
         return;
      }
      //====process old versions before automatic schema evolution
      if (R__v < 2) TObject::Streamer(R__b);
      R__b >> fX;
      R__b >> fY;
      R__b.CheckByteCount(R__s, R__c, TVector2::IsA());
      //====end of old versions

   } else {
      TVector2::Class()->WriteBuffer(R__b,this);
   }
}

void TVector2::Print(Option_t*)const
{
   //print vector parameters
   Printf("%s %s (x,y)=(%f,%f) (rho,phi)=(%f,%f)",GetName(),GetTitle(),X(),Y(),
                                          Mod(),Phi()*TMath::RadToDeg());
}

