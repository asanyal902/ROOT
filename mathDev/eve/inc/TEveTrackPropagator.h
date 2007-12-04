// @(#)root/eve:$Id$
// Authors: Matevz Tadel & Alja Mrak-Tadel: 2006, 2007

/*************************************************************************
 * Copyright (C) 1995-2007, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#ifndef ROOT_TEveTrackPropagator
#define ROOT_TEveTrackPropagator

#include "TEveVSDStructs.h"
#include "TEveUtil.h"
#include "TObject.h"
#include "TMarker.h"

#include <vector>

class TEvePointSet;

class TEveTrackPropagator: public TObject,
                           public TEveRefBackPtr
{
   friend class TEveTrackPropagatorSubEditor;

public:
   struct Vertex4D_t
   {
      Float_t x, y, z, t;

      Vertex4D_t() : x(0), y(0), z(0), t(0) {}
      Vertex4D_t(Float_t _x, Float_t _y, Float_t _z, Float_t _t=0) :
         x(_x), y(_y), z(_z), t(_t) {}

      Float_t Mag()  const { return TMath::Sqrt(x*x+y*y+z*z);}
      Float_t Mag2() const { return x*x+y*y+z*z;}

      Float_t Perp()  const { return TMath::Sqrt(x*x+y*y);}
      Float_t Perp2() const { return x*x+y*y;}
      Float_t R()     const { return Perp(); }

      Vertex4D_t operator + (const Vertex4D_t & b)
      { return Vertex4D_t(x + b.x, y + b.y, z + b.z, t + b.t); }

      Vertex4D_t operator - (const Vertex4D_t & b)
      { return Vertex4D_t(x - b.x, y - b.y, z - b.z, t - b.t); }

      Vertex4D_t operator * (Float_t a)
      { return Vertex4D_t(a*x, a*y, a*z, a*t); }

      Vertex4D_t& operator += (const Vertex4D_t & b)
      { x += b.x; y += b.y; z += b.z; t += b.t; return *this; }
   };

   struct Helix_t
   {
      Float_t fA;           // contains charge and magnetic field data
      Float_t fLam;         // momentum ratio pT/pZ
      Float_t fR;           // a/pT
      Float_t fPhiStep;     // step size in xy projection, dependent of RnrMode and momentum
      Float_t fTimeStep;    // time step
      Float_t fSin,  fCos;  // current sin, cos
      Float_t fXoff, fYoff; // offset for fitting daughters

      Helix_t() :
         fLam  (0), fR    (0), fPhiStep (0), fTimeStep (0) ,
         fSin  (0), fCos  (0),
         fXoff (0), fYoff (0)
      {}

      void Step(Vertex4D_t& v, TEveVector& p);
      void StepVertex(Vertex4D_t& v, TEveVector& p, Vertex4D_t& forw);
   };

private:
   TEveTrackPropagator(const TEveTrackPropagator&);            // Not implemented
   TEveTrackPropagator& operator=(const TEveTrackPropagator&); // Not implemented

protected:
   //----------------------------------
   // Track extrapolation configuration
   Float_t                  fMagField;      // Constant magnetic field along z in Tesla.
   // TEveTrack limits
   Float_t                  fMaxR;          // Max radius for track extrapolation
   Float_t                  fMaxZ;          // Max z-coordinate for track extrapolation.
   // Helix limits
   Float_t                  fMaxOrbs;       // Maximal angular path of tracks' orbits (1 ~ 2Pi).
   Float_t                  fMinAng;        // Minimal angular step between two helix points.
   Float_t                  fDelta;         // Maximal error at the mid-point of the line connecting to helix points.

   // Path-mark / first-vertex control
   Bool_t                   fEditPathMarks; // Show widgets for path-mark control in GUI editor.
   Bool_t                   fFitDaughters;  // Pass through daughter creation points when extrapolating a track.
   Bool_t                   fFitReferences; // Pass through given track-references when extrapolating a track.
   Bool_t                   fFitDecay;      // Pass through decay point when extrapolating a track.
   Bool_t                   fRnrDaughters;  // Render daughter path-marks.
   Bool_t                   fRnrReferences; // Render track-reference path-marks.
   Bool_t                   fRnrDecay;      // Render decay path-marks.
   Bool_t                   fRnrFV;         // Render first vertex.
   TMarker                  fPMAtt;         // Marker attributes for rendering of path-marks.
   TMarker                  fFVAtt;         // Marker attributes for fits vertex.

   //------------------------------------
   // propagation, state of current track

   Int_t                    fCharge;        // particle charge
   Float_t                  fVelocity;      // particle velocity 
   std::vector<Vertex4D_t>  fPoints;        // calculated point
   Vertex4D_t               fV;             // current vertex 
   Int_t                    fN;             // current step number;
   Int_t                    fNLast;         // last step
   Int_t                    fNMax;          // max steps
   Helix_t                  fH;             // helix

   void    RebuildTracks();

   void    InitHelix();
   void    SetNumOfSteps();
   Bool_t  HelixToVertex(TEveVector& v, TEveVector& p);
   void    HelixToBounds(TEveVector& p);

   Bool_t  LineToVertex (TEveVector& v);
   void    LineToBounds (TEveVector& p);
   
public:
   TEveTrackPropagator();
   virtual ~TEveTrackPropagator() {}

   // propagation
   void   InitTrack(TEveVector &v, TEveVector &p, Float_t beta, Int_t charge);
   void   ResetTrack();
   void   GoToBounds(TEveVector& p);
   Bool_t GoToVertex(TEveVector& v, TEveVector& p);
   void   FillPointSet(TEvePointSet* ps) const;

   void   SetMagField(Float_t x);
   void   SetMaxR(Float_t x);
   void   SetMaxZ(Float_t x);
   void   SetMaxOrbs(Float_t x);
   void   SetMinAng(Float_t x);
   void   SetDelta(Float_t x);

   void   SetEditPathMarks(Bool_t x) { fEditPathMarks = x; }
   void   SetRnrDaughters(Bool_t x);
   void   SetRnrReferences(Bool_t x);
   void   SetRnrDecay(Bool_t x);
   void   SetFitDaughters(Bool_t x);
   void   SetFitReferences(Bool_t x);
   void   SetFitDecay(Bool_t x);
   void   SetRnrFV(Bool_t x) { fRnrFV = x; }

   Float_t GetMagField() const { return fMagField; }
   Float_t GetMaxR()     const { return fMaxR; }
   Float_t GetMaxZ()     const { return fMaxZ; }
   Float_t GetMaxOrbs()  const { return fMaxOrbs; }
   Float_t GetMinAng()   const { return fMinAng; }
   Float_t GetDelta()    const { return fDelta; }

   Bool_t  GetEditPathMarks() const { return fEditPathMarks; }
   Bool_t  GetRnrDaughters()  const { return fRnrDaughters; }
   Bool_t  GetRnrReferences() const { return fRnrReferences; }
   Bool_t  GetRnrDecay()      const { return fRnrDecay; }
   Bool_t  GetFitDaughters()  const { return fFitDaughters; }
   Bool_t  GetFitReferences() const { return fFitReferences; }
   Bool_t  GetFitDecay()      const { return fFitDecay; }
   Bool_t  GetRnrFV()         const { return fRnrFV; }

   TMarker& RefPMAtt() { return fPMAtt; }
   TMarker& RefFVAtt() { return fFVAtt; }

   static Float_t             fgDefMagField; // Default value for constant solenoid magnetic field.
   static const Float_t       fgkB2C;        // Constant for conversion of momentum to curvature.
   static TEveTrackPropagator fgDefStyle;    // Default track render-style.

   ClassDef(TEveTrackPropagator, 0); // Calculates path of a particle taking into account special path-marks and imposed boundaries.
};

#endif
