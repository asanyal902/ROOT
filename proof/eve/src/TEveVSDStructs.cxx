// @(#)root/eve:$Id$
// Authors: Matevz Tadel & Alja Mrak-Tadel: 2006, 2007

/*************************************************************************
 * Copyright (C) 1995-2007, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#include "TEveVSDStructs.h"

//______________________________________________________________________________
// TEveVector
//
// Float three-vector; a inimal Float_t copy of TVector3 used to
// represent points and momenta (also used in VSD).

ClassImp(TEveVector)

//______________________________________________________________________________
Float_t TEveVector::Eta() const
{
   Float_t cosTheta = CosTheta();
   if (cosTheta*cosTheta < 1) return -0.5* TMath::Log( (1.0-cosTheta)/(1.0+cosTheta) );
   Warning("Eta","transverse momentum = 0! return +/- 10e10");
   return (fZ >= 0) ? 10e10 : -10e10;
}

//______________________________________________________________________________
TEveVector TEveVector::operator + (const TEveVector & b)
{
   return TEveVector(fX + b.fX, fY + b.fY, fZ + b.fZ);
}

//______________________________________________________________________________
TEveVector TEveVector::operator - (const TEveVector & b)
{
   return TEveVector(fX - b.fX, fY - b.fY, fZ - b.fZ);
}

//______________________________________________________________________________
TEveVector TEveVector::operator * (Float_t a)
{
   return TEveVector(a*fX, a*fY, a*fZ);
}


//______________________________________________________________________________
// TEvePathMark
//
// Special-point on track: position/momentum reference, daughter
// creation or decay (also used in VSD).

ClassImp(TEvePathMark)

//______________________________________________________________________________
const char* TEvePathMark::TypeName()
{
   switch (fType)
   {
      case kDaughter:  return "Daughter";
      case kReference: return "Reference";
      case kDecay:     return "Decay";
      default:         return "Unknown";
   }
}


//______________________________________________________________________________
//
// Not documented.
//

ClassImp(TEveMCTrack)
ClassImp(TEveHit)
ClassImp(TEveCluster)
ClassImp(TEveRecTrack)
ClassImp(TEveRecKink)
ClassImp(TEveRecV0)
ClassImp(TEveMCRecCrossRef)
