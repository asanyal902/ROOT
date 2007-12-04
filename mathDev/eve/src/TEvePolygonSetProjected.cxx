// @(#)root/eve:$Id$
// Authors: Matevz Tadel & Alja Mrak-Tadel: 2006, 2007

/*************************************************************************
 * Copyright (C) 1995-2007, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#include "TEvePolygonSetProjected.h"
#include "TEveVSDStructs.h"
#include "TEveGeoNode.h"
#include "TEveProjectionManager.h"

#include "TBuffer3D.h"
#include "TBuffer3DTypes.h"
#include "TVirtualPad.h"
#include "TVirtualViewer3D.h"

namespace
{
struct Seg_t
{
   // Helper class for building 2D polygons from TBuffer3D.

   Int_t v1;
   Int_t v2;

   Seg_t(Int_t i1=-1, Int_t i2=-1):v1(i1), v2(i2){};
};
typedef std::list<Seg_t>::iterator It_t;
}


//______________________________________________________________________________
// TEvePolygonSetProjected
//
// A set of projected polygons.
// Used for storage of projected geometrical shapes.
//
// Internal struct Polygon_t holds only indices into the master vertex
// array in TEvePolygonSetProjected.

ClassImp(TEvePolygonSetProjected)

//______________________________________________________________________________
TEvePolygonSetProjected::TEvePolygonSetProjected(const Text_t* n, const Text_t* t) :
   TEveElementList(n, t),

   fBuff(0),
   fIdxMap(0),

   fSurf(0),

   fNPnts(0),
   fPnts(0),

   fFillColor(5),
   fLineColor(3),
   fLineWidth(1),
   fTransparency (0)
{
   // Constructor.

   SetMainColorPtr(&fFillColor);
}

//______________________________________________________________________________
TEvePolygonSetProjected::~TEvePolygonSetProjected()
{
   // Destructor.

   ClearPolygonSet();
}

/******************************************************************************/

//______________________________________________________________________________
void TEvePolygonSetProjected::ClearPolygonSet()
{
   // Clears list of points and polygons.

   Int_t* p;
   for (vpPolygon_i i = fPols.begin(); i!= fPols.end(); i++)
   {
      p =  (*i).fPnts; delete [] p;
   }
   fPols.clear();

   // delete reduced points
   delete [] fPnts;
}

//______________________________________________________________________________
void TEvePolygonSetProjected::SetProjection(TEveProjectionManager* proj, TEveProjectable* model)
{
   // This is virtual method from base-class TEveProjected.

   TEveProjected::SetProjection(proj, model);
   TEveGeoShape* gre = dynamic_cast<TEveGeoShape*>(model);

   fBuff = gre->MakeBuffer3D();
   if(fBuff)
   {
      Color_t color = gre->GetMainColor();
      SetMainColor(color);
      SetLineColor((Color_t)TColor::GetColorBright(color));
      SetMainTransparency(gre->GetMainTransparency());
   }
}

//______________________________________________________________________________
void TEvePolygonSetProjected::UpdateProjection()
{
   // This is virtual method from base-class TEveProjected.

   if(fBuff == 0) return;

   // drop polygons, and projected/reduced points
   ClearPolygonSet(); 
   fPnts = 0; 
   fNPnts = 0;
   fSurf = 0;
   ProjectBuffer3D();
}

//______________________________________________________________________________
Bool_t TEvePolygonSetProjected::IsFirstIdxHead(Int_t s0, Int_t s1)
{
   // Compare the two segments and check if the first index of first segment is starting.

   Int_t v0 = fBuff->fSegs[3*s0 + 1];
   Int_t v2 = fBuff->fSegs[3*s1 + 1];
   Int_t v3 = fBuff->fSegs[3*s1 + 2];
   if(v0 != v2 && v0 != v3 )
      return kTRUE;
   else
      return kFALSE;
}

//______________________________________________________________________________
void TEvePolygonSetProjected::ProjectAndReducePoints()
{
   // Project and reduce buffer points.

   TEveProjection* projection = fProjector->GetProjection();

   Int_t N = fBuff->NbPnts();
   TEveVector*  pnts  = new TEveVector[N];
   for(Int_t i = 0; i<N; i++)
   {
      pnts[i].Set(fBuff->fPnts[3*i],fBuff->fPnts[3*i+1], fBuff->fPnts[3*i+2]);
      projection->ProjectPoint(pnts[i].x, pnts[i].y, pnts[i].z, TEveProjection::PP_Plane);
   }
   fIdxMap   = new Int_t[N];
   Int_t* ra = new Int_t[N];  // list of reduced vertices
   for(UInt_t v = 0; v < (UInt_t)N; ++v)
   {
      fIdxMap[v] = -1;
      for(Int_t k = 0; k < fNPnts; ++k)
      {
         if(pnts[v].SquareDistance(pnts[ra[k]]) < TEveProjection::fgEps*TEveProjection::fgEps)
         {
            fIdxMap[v] = k;
            break;
         }
      }
      // have not found a point inside epsilon, add new point in scaled array
      if(fIdxMap[v] == -1)
      {
         fIdxMap[v] = fNPnts;
         ra[fNPnts] = v;
         ++fNPnts;
      }
   }

   // create an array of scaled points
   fPnts = new TEveVector[fNPnts];
   for(Int_t idx = 0; idx < fNPnts; ++idx)
   {
      Int_t i = ra[idx];
      projection->ProjectPoint(pnts[i].x, pnts[i].y, pnts[i].z, TEveProjection::PP_Distort);
      fPnts[idx].Set(pnts[i]);
   }
   delete [] ra;
   delete [] pnts;
   // printf("reduced %d points of %d\n", fNPnts, N);
}

//______________________________________________________________________________
void TEvePolygonSetProjected::AddPolygon(std::list<Int_t>& pp, vpPolygon_t& pols)
{
   // Check if polygon has dimensions above TEveProjection::fgEps and add it
   // to a list if it is not a duplicate.

   if (pp.size() <= 2) return;

   // dimension of bbox
   Float_t bbox[] = { 1e6, -1e6, 1e6, -1e6, 1e6, -1e6 };
   for (std::list<Int_t>::iterator u = pp.begin(); u!= pp.end(); u++)
   {
      Int_t idx = *u;
      if (fPnts[idx].x < bbox[0]) bbox[0] = fPnts[idx].x;
      if (fPnts[idx].x > bbox[1]) bbox[1] = fPnts[idx].x;

      if (fPnts[idx].y < bbox[2]) bbox[2] = fPnts[idx].y;
      if (fPnts[idx].y > bbox[3]) bbox[3] = fPnts[idx].y;
   }
   Float_t eps = 2*TEveProjection::fgEps;
   if ((bbox[1]-bbox[0]) < eps || (bbox[3]-bbox[2]) < eps) return;

   // duplication
   for (vpPolygon_i poi = pols.begin(); poi != pols.end(); poi++)
   {
      Polygon_t P = *poi;
      if ((Int_t)pp.size() != P.fNPnts)
         continue;
      std::list<Int_t>::iterator u = pp.begin();
      Int_t pidx = P.FindPoint(*u);
      if (pidx < 0)
         continue;
      while (u != pp.end())
      {
         if ((*u) != P.fPnts[pidx])
            break;
         ++u;
         if (++pidx >= P.fNPnts) pidx = 0;
      }
      if (u == pp.end()) return;
   }

   Int_t* pv = new Int_t[pp.size()];
   Int_t count=0;
   for (std::list<Int_t>::iterator u = pp.begin(); u != pp.end(); u++)
   {
      pv[count] = *u;
      count++;
   }
   pols.push_back(Polygon_t((Int_t)pp.size(), pv));
   fSurf += (bbox[1]-bbox[0]) * (bbox[3]-bbox[2]);
}

//______________________________________________________________________________
void TEvePolygonSetProjected::MakePolygonsFromBP()
{
   // Build polygons from list of buffer polygons.

   TEveProjection* projection = fProjector->GetProjection();
   Int_t* bpols = fBuff->fPols;
   for(UInt_t pi = 0; pi< fBuff->NbPols(); pi++)
   {
      std::list<Int_t>  pp; // points in current polygon
      UInt_t Nseg = bpols[1];
      Int_t* seg =  &bpols[2];
      // start idx in the fist segment depends of second segment
      Int_t  tail, head;
      Bool_t h = IsFirstIdxHead(seg[0], seg[1]);
      if(h) {
         head = fIdxMap[fBuff->fSegs[3*seg[0] + 1]];
         tail = fIdxMap[fBuff->fSegs[3*seg[0] + 2]];
      }
      else {
         head = fIdxMap[fBuff->fSegs[3*seg[0] + 2]];
         tail = fIdxMap[fBuff->fSegs[3*seg[0] + 1]];
      }
      pp.push_back(head);
      // printf("start idx head %d, tail %d\n", head, tail);
      std::list<Seg_t> segs;
      for(UInt_t s = 1; s < Nseg; ++s)
         segs.push_back(Seg_t(fBuff->fSegs[3*seg[s] + 1],fBuff->fSegs[3*seg[s] + 2]));


      Bool_t accepted = kFALSE;
      for(std::list<Seg_t>::iterator it = segs.begin(); it != segs.end(); it++ )
      {
         Int_t mv1 = fIdxMap[(*it).v1];
         Int_t mv2 = fIdxMap[(*it).v2];
         accepted = projection->AcceptSegment(fPnts[mv1], fPnts[mv2], TEveProjection::fgEps);

         if(accepted == kFALSE)
         {
            pp.clear();
            break;
         }
         if(tail != pp.back()) pp.push_back(tail);
         tail = (mv1 == tail) ? mv2 :mv1;
      }
      // DirectDraw() implementation: last and first vertices should not be equal
      if(pp.empty() == kFALSE)
      {
         if(pp.front() == pp.back()) pp.pop_front();
         AddPolygon(pp, fPolsBP);
      }
      bpols += (Nseg+2);
   }
}

//______________________________________________________________________________
void TEvePolygonSetProjected::MakePolygonsFromBS()
{
   // Build polygons from the set of buffer segments.
   // First creates a segment pool according to reduced and projected points
   // and then build polygons from the pool.

   std::list<Seg_t> segs;
   std::list<Seg_t>::iterator it;
   TEveProjection* projection = fProjector->GetProjection();
   for(UInt_t s = 0; s < fBuff->NbSegs(); ++s)
   {
      Bool_t duplicate = kFALSE;
      Int_t vo1, vo2;   // idx from fBuff segment
      Int_t vor1, vor2; // mapped idx
      vo1 =  fBuff->fSegs[3*s + 1];
      vo2 =  fBuff->fSegs[3*s + 2]; //... skip color info
      vor1 = fIdxMap[vo1];
      vor2 = fIdxMap[vo2];
      if(vor1 == vor2) continue;
      // check duplicate
      for(it = segs.begin(); it != segs.end(); it++ ){
         Int_t vv1 = (*it).v1;
         Int_t vv2 = (*it).v2;
         if((vv1 == vor1 && vv2 == vor2 )||(vv1 == vor2 && vv2 == vor1 )){
            duplicate = kTRUE;
            continue;
         }
      }
      if(duplicate == kFALSE && projection->AcceptSegment(fPnts[vor1], fPnts[vor2], TEveProjection::fgEps))
      {
         segs.push_back(Seg_t(vor1, vor2));
      }
   }

   while(segs.empty() == kFALSE)
   {
      std::list<Int_t> pp; // points in current polygon
      pp.push_back(segs.front().v1);
      Int_t tail = segs.front().v2;
      segs.pop_front();
      Bool_t match = kTRUE;
      while(match && segs.empty() == kFALSE)
      {
         for(It_t k=segs.begin(); k!=segs.end(); ++k){
            Int_t cv1 = (*k).v1;
            Int_t cv2 = (*k).v2;
            if( cv1 == tail || cv2 == tail){
               pp.push_back(tail);
               tail = (cv1 == tail)? cv2:cv1;
               It_t to_erase = k--;
               segs.erase(to_erase);
               match = kTRUE;
               break;
            }
            else
            {
               match = kFALSE;
            }
         } // end for loop in the segment pool
         if(tail == pp.front())
            break;
      };
      AddPolygon(pp, fPolsBS);
   }
}

/******************************************************************************/

//______________________________________________________________________________
void  TEvePolygonSetProjected::ProjectBuffer3D()
{
   // Project current buffer.

   ProjectAndReducePoints();
   TEveProjection::GeoMode_e mode = fProjector->GetProjection()->GetGeoMode();

   switch (mode)
   {
      case TEveProjection::GM_Polygons :
      {
         MakePolygonsFromBP();
         fPolsBP.swap(fPols);
         break;
      }
      case TEveProjection::GM_Segments :
      {
         MakePolygonsFromBS();
         fPolsBS.swap(fPols);
         break;
      }
      case TEveProjection::GM_Unknown:
      {
         Float_t BPsurf = fSurf;
         fSurf = 0;
         MakePolygonsFromBS();
         if(fSurf < BPsurf)
         {
            fPolsBP.swap(fPols);
            fPolsBS.clear();
         }
         else
         {
            fPolsBS.swap(fPols);
            fPolsBP.clear();
         }
      }
      default:
         break;
   }

   delete []  fIdxMap;
   ResetBBox();
}

//______________________________________________________________________________
void TEvePolygonSetProjected::ComputeBBox()
{
   // Override of virtual method from TAttBBox.

   BBoxInit();
   for(Int_t pi = 0; pi<fNPnts; pi++)
      BBoxCheckPoint(fPnts[pi].x, fPnts[pi].y, fPnts[pi].z );
   AssertBBoxExtents(0.1);
}

//______________________________________________________________________________
void TEvePolygonSetProjected::Paint(Option_t* )
{
   // Paint this object. Only direct rendering is supported.

   if(fNPnts == 0) return;
   TBuffer3D buffer(TBuffer3DTypes::kGeneric);

   // Section kCore
   buffer.fID           = this;
   buffer.fColor        = GetMainColor();
   buffer.fTransparency = fTransparency;
   buffer.fLocalFrame   = false;

   buffer.SetSectionsValid(TBuffer3D::kCore);

   // We fill kCore on first pass and try with viewer
   Int_t reqSections = gPad->GetViewer3D()->AddObject(buffer);
   if (reqSections == TBuffer3D::kNone) {
      return;
   }
}

/******************************************************************************/

//______________________________________________________________________________
void TEvePolygonSetProjected::DumpPolys() const
{
   // Dump information about built polygons.

   printf("TEvePolygonSetProjected %d polygons\n", (Int_t)fPols.size());
   for (vpPolygon_ci i = fPols.begin(); i!= fPols.end(); i++)
   {
      Int_t N =  (*i).fNPnts;
      printf("Points of polygon %d:\n", N);
      for(Int_t vi = 0; vi<N; vi++) {
         Int_t pi = (*i).fPnts[vi];
         printf("(%f, %f, %f)", fPnts[pi].x, fPnts[pi].y, fPnts[pi].z);
      }
      printf("\n");
   }
}

//______________________________________________________________________________
void TEvePolygonSetProjected::DumpBuffer3D()
{
   // Dump information about currenty projected buffer.

   Int_t* bpols = fBuff->fPols;

   for(UInt_t pi = 0; pi< fBuff->NbPols(); pi++)
   {
      UInt_t Nseg = bpols[1];
      printf("%d polygon of %d has %d segments \n", pi,fBuff->NbPols(),Nseg);

      Int_t* seg =  &bpols[2];
      for(UInt_t a=0; a<Nseg; a++)
      {
         Int_t a1 = fBuff->fSegs[3*seg[a]+ 1];
         Int_t a2 = fBuff->fSegs[3*seg[a]+ 2];
         printf("(%d, %d) \n", a1, a2);
         printf("ORIG points :(%f, %f, %f)  (%f, %f, %f)\n",
                fBuff->fPnts[3*a1],fBuff->fPnts[3*a1+1], fBuff->fPnts[3*a1+2],
                fBuff->fPnts[3*a2],fBuff->fPnts[3*a2+1], fBuff->fPnts[3*a2+2]);
      }
      printf("\n");
      bpols += (Nseg+2);
   }
}
