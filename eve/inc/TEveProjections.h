// @(#)root/eve:$Id$
// Authors: Matevz Tadel & Alja Mrak-Tadel: 2006, 2007

/*************************************************************************
 * Copyright (C) 1995-2007, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#ifndef ROOT_TEveProjections
#define ROOT_TEveProjections

#include "TEveVSDStructs.h"

////////////////////////////////////////////////////////////////
//                                                            //
// TEveProjection                                              //
//                                                            //
////////////////////////////////////////////////////////////////

class TEveProjection
{
public:
   enum PType_e   { PT_Unknown, PT_CFishEye, PT_RhoZ };     // type
   enum PProc_e   { PP_Plane, PP_Distort, PP_Full };        // procedure
   enum GeoMode_e { GM_Unknown, GM_Polygons, GM_Segments }; // reconstruction of geometry

protected:
   PType_e             fType;          // type
   GeoMode_e           fGeoMode;       // way of polygon reconstruction
   const char*         fName;          // name

   TEveVector          fCenter;        // center of distortion
   TEveVector          fZeroPosVal;    // projected origin (0, 0, 0)

   Float_t             fDistortion;    // distortion
   Float_t             fFixedRadius;   // projected radius independent of distortion
   Float_t             fScale;         // scale factor to keep projected radius fixed
   TEveVector          fUpLimit;       // convergence of point +infinity
   TEveVector          fLowLimit;      // convergence of point -infinity

public:
   TEveProjection(TEveVector& center);
   virtual ~TEveProjection(){}

   virtual   void      ProjectPoint(Float_t&, Float_t&, Float_t&, PProc_e p = PP_Full ) = 0;
   virtual   void      ProjectPointFv(Float_t* v){ ProjectPoint(v[0], v[1], v[2]); }
   virtual   void      ProjectVector(TEveVector& v);

   const     char*     GetName(){return fName;}
   void                SetName(const char* txt){ fName = txt; }

   virtual void        SetCenter(TEveVector& v){ fCenter = v; UpdateLimit();}
   virtual Float_t*    GetProjectedCenter() { return fCenter.c_vec(); }

   void                SetType(PType_e t){fType = t;}
   PType_e             GetType(){return fType;}

   void                SetGeoMode(GeoMode_e m){fGeoMode = m;}
   GeoMode_e           GetGeoMode(){return fGeoMode;}

   void                UpdateLimit();
   void                SetDistortion(Float_t d);
   Float_t             GetDistortion(){return fDistortion;}
   void                SetFixedRadius(Float_t x);
   Float_t             GetFixedRadius(){return fFixedRadius;}

   virtual   Bool_t    AcceptSegment(TEveVector&, TEveVector&, Float_t /*tolerance*/) { return kTRUE; }
   virtual   void      SetDirectionalVector(Int_t screenAxis, TEveVector& vec);

   // utils to draw axis
   virtual Float_t     GetValForScreenPos(Int_t ax, Float_t value);
   virtual Float_t     GetScreenVal(Int_t ax, Float_t value);
   Float_t             GetLimit(Int_t i, Bool_t pos) { return pos ? fUpLimit[i] : fLowLimit[i]; }

   static   Float_t    fgEps;  // resolution of projected points

   ClassDef(TEveProjection, 0); // Base for specific classes that implement non-linear projections.
};


////////////////////////////////////////////////////////////////
//                                                            //
// TEveRhoZProjection                                         //
//                                                            //
////////////////////////////////////////////////////////////////

class TEveRhoZProjection: public TEveProjection
{
private:
   TEveVector   fProjectedCenter; // projected center of distortion.

public:
   TEveRhoZProjection(TEveVector& center) : TEveProjection(center) { fType = PT_RhoZ; fName="RhoZ"; }
   virtual ~TEveRhoZProjection() {}

   virtual   Bool_t    AcceptSegment(TEveVector& v1, TEveVector& v2, Float_t tolerance);
   virtual   void      ProjectPoint(Float_t& x, Float_t& y, Float_t& z, PProc_e proc = PP_Full);
   virtual   void      SetDirectionalVector(Int_t screenAxis, TEveVector& vec);

   virtual   void      SetCenter(TEveVector& center);
   virtual Float_t*    GetProjectedCenter() { return fProjectedCenter.c_vec(); }

   ClassDef(TEveRhoZProjection, 0); // Rho/Z non-linear projection.
};


////////////////////////////////////////////////////////////////
//                                                            //
// TEveCircularFishEyeProjection                              //
//                                                            //
////////////////////////////////////////////////////////////////

class TEveCircularFishEyeProjection : public TEveProjection
{
public:
   TEveCircularFishEyeProjection(TEveVector& center):TEveProjection(center) { fType = PT_CFishEye; fGeoMode = GM_Polygons; fName="CircularFishEye"; }
   virtual ~TEveCircularFishEyeProjection() {}

   virtual void ProjectPoint(Float_t& x, Float_t& y, Float_t& z, PProc_e proc = PP_Full);

   ClassDef(TEveCircularFishEyeProjection, 0); // XY non-linear projection.
};

#endif
