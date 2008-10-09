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


//==============================================================================
// TEveVector
//==============================================================================

//______________________________________________________________________________
//
// Float three-vector; a inimal Float_t copy of TVector3 used to
// represent points and momenta (also used in VSD).

ClassImp(TEveVector);

//______________________________________________________________________________
void TEveVector::Dump() const
{
   // Dump to stdout as "(x, y, z)\n".

   printf("(%f, %f, %f)\n", fX, fY, fZ);
}

//______________________________________________________________________________
Float_t TEveVector::Eta() const
{
   // Calculate eta of the point, pretending it's a momentum vector.

   Float_t cosTheta = CosTheta();
   if (cosTheta*cosTheta < 1) return -0.5* TMath::Log( (1.0-cosTheta)/(1.0+cosTheta) );
   Warning("Eta","transverse momentum = 0, returning +/- 1e10");
   return (fZ >= 0) ? 1e10 : -1e10;
}

//______________________________________________________________________________
TEveVector TEveVector::operator + (const TEveVector & b) const
{
   // Vector addition.

   return TEveVector(fX + b.fX, fY + b.fY, fZ + b.fZ);
}

//______________________________________________________________________________
TEveVector TEveVector::operator - (const TEveVector & b) const
{
   // Vector subtraction.

   return TEveVector(fX - b.fX, fY - b.fY, fZ - b.fZ);
}

//______________________________________________________________________________
TEveVector TEveVector::operator * (Float_t a) const
{
   // Multiplication with scalar.

   return TEveVector(a*fX, a*fY, a*fZ);
}


//==============================================================================
// TEveVector4
//==============================================================================

//______________________________________________________________________________
//
// Float four-vector.

ClassImp(TEveVector4);

//______________________________________________________________________________
void TEveVector4::Dump() const
{
   // Dump to stdout as "(x, y, z; t)\n".

   printf("(%f, %f, %f; %f)\n", fX, fY, fZ, fT);
}


//==============================================================================
// TEvePathMark
//==============================================================================

//______________________________________________________________________________
//
// Special-point on track:
//  kDaughter  - daughter creation; fP is momentum of the daughter
//  kReference - position/momentum reference
//  kDecay     - decay point, fP not used
//  kCluster2D - measurement with large error in one direction (like strip detectors):
//               fP - normal to detector plane,
//               fE - large error direction, must be normalized.
//               Track is propagated to plane and correction in fE direction is discarded.


ClassImp(TEvePathMark);

//______________________________________________________________________________
const char* TEvePathMark::TypeName()
{
   // Return the name of path-mark type.

   switch (fType)
   {
      case kDaughter:  return "Daughter";
      case kReference: return "Reference";
      case kDecay:     return "Decay";
      case kCluster2D: return "Cluster2D";
      default:         return "Unknown";
   }
}


//______________________________________________________________________________
//
// Not documented.
//

ClassImp(TEveVector4);
ClassImp(TEveMCTrack);
ClassImp(TEveHit);
ClassImp(TEveCluster);
ClassImp(TEveRecTrack);
ClassImp(TEveRecKink);
ClassImp(TEveRecV0);
ClassImp(TEveRecCascade);
ClassImp(TEveMCRecCrossRef);
