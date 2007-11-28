// @(#)root/eve:$Id$
// Authors: Matevz Tadel & Alja Mrak-Tadel: 2006, 2007

/*************************************************************************
 * Copyright (C) 1995-2007, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#ifndef ROOT_TEvePolygonSetProjected
#define ROOT_TEvePolygonSetProjected

#include "TEveElement.h"
#include "TEveProjectionBases.h"

#include "TNamed.h"
#include "TAtt3D.h"
#include "TAttBBox.h"
#include "TColor.h"
#include "TEveVSDStructs.h"

class TBuffer3D;

namespace std
{
template<typename _Tp> class allocator;
template<typename _Tp, typename _Alloc > class list;
}

class TEveVector;




class TEvePolygonSetProjected :  public TEveElementList,
                                 public TEveProjected,
                                 public TAtt3D,
                                 public TAttBBox
{
   friend class TEvePolygonSetProjectedGL;
   friend class TEvePolygonSetProjectedEditor;
private:
   TEvePolygonSetProjected(const TEvePolygonSetProjected&);            // Not implemented
   TEvePolygonSetProjected& operator=(const TEvePolygonSetProjected&); // Not implemented

protected:
   struct Polygon_t
   {
      Int_t     fNPnts;
      Int_t*    fPnts;

      Polygon_t() : fNPnts(0), fPnts(0) {}
      Polygon_t(Int_t n, Int_t* p) : fNPnts(n), fPnts(p) {}
      Polygon_t(const Polygon_t& x) : fNPnts(x.fNPnts), fPnts(x.fPnts) {}
      virtual ~Polygon_t() {}

      Polygon_t& operator=(const Polygon_t& x)
      { fNPnts = x.fNPnts; fPnts = x.fPnts; return *this; }

      Int_t FindPoint(Int_t pi)
      { for (Int_t i=0; i<fNPnts; ++i) if (fPnts[i] == pi) return i; return -1; }
   };

   typedef std::list<Polygon_t>                    vpPolygon_t;
   typedef vpPolygon_t::iterator                   vpPolygon_i;
   typedef vpPolygon_t::const_iterator             vpPolygon_ci;

private:
   TBuffer3D*   fBuff;
   Int_t*       fIdxMap; // map from original to projected and reduced point needed oly for geometry

   Bool_t       IsFirstIdxHead(Int_t s0, Int_t s1);
   void         AddPolygon(std::list<Int_t, std::allocator<Int_t> >& pp, std::list<Polygon_t, std::allocator<Polygon_t> >& p);

   void         ProjectAndReducePoints();
   void         MakePolygonsFromBP();
   void         MakePolygonsFromBS();
   void         ClearPolygonSet();

protected:
   vpPolygon_t  fPols;     // NLT polygons
   vpPolygon_t  fPolsBS;   // NLT polygons build freom TBuffer3D segments
   vpPolygon_t  fPolsBP;   // NLT polygons build freom TBuffer3D polygond
   Float_t      fSurf;     // sum of surface of polygons

   Int_t        fNPnts;    // number of reduced and projected points
   TEveVector*  fPnts;     // reduced and projected points

   Color_t      fFillColor;
   Color_t      fLineColor;
   Float_t      fLineWidth;

   UChar_t      fTransparency;

public:
   TEvePolygonSetProjected(const Text_t* n="TEvePolygonSetProjected", const Text_t* t="");
   virtual ~TEvePolygonSetProjected();

   virtual void    SetProjection(TEveProjectionManager* proj, TEveProjectable* model);
   virtual void    UpdateProjection();

   void            ProjectBuffer3D();

   virtual void    ComputeBBox();
   virtual void    Paint(Option_t* option = "");

   virtual void    DumpPolys() const;
   void            DumpBuffer3D();

   //rendering
   virtual Bool_t  CanEditMainColor()   { return kTRUE; }
   virtual Color_t GetLineColor() const { return fLineColor; }

   virtual Bool_t  CanEditMainTransparency()      { return kTRUE; }
   virtual UChar_t GetMainTransparency() const    { return fTransparency; }
   virtual void    SetMainTransparency(UChar_t t) { fTransparency = t; }

   virtual void    SetFillColor(Pixel_t pixel) { fFillColor = Color_t(TColor::GetColor(pixel));}
   virtual void    SetLineColor(Pixel_t pixel) { fLineColor = Color_t(TColor::GetColor(pixel));}

   virtual void    SetFillColor(Color_t c)   { fFillColor = c; }
   virtual void    SetLineColor(Color_t c)   { fLineColor = c; }
   virtual void    SetLineWidth(Double_t lw) {fLineWidth = lw;}

   ClassDef(TEvePolygonSetProjected,0); // Set of projected polygons with outline; typically produced from a TBuffer3D.

};

#endif
