// @(#)root/eve:$Id$
// Authors: Matevz Tadel & Alja Mrak-Tadel: 2006, 2007

/*************************************************************************
 * Copyright (C) 1995-2007, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#include "TEveTrackGL.h"
#include "TEveTrack.h"
#include "TEveTrackPropagator.h"

#include "TGLIncludes.h"
#include "TGLRnrCtx.h"
#include "TGLSelectRecord.h"

//______________________________________________________________________________
// TEveTrackGL
//
// GL-renderer for TEveTrack class.

ClassImp(TEveTrackGL)

//______________________________________________________________________________
TEveTrackGL::TEveTrackGL() : TEveLineGL()
{
   // Default constructor.

   // fDLCache = false; // Disable display list.
}

/******************************************************************************/

//______________________________________________________________________________
Bool_t TEveTrackGL::SetModel(TObject* obj, const Option_t* /*opt*/)
{
   // Set model object.

   if (TEveLineGL::SetModel(obj) == kFALSE) return kFALSE;
   if (SetModelCheckClass(obj, TEveTrack::Class())) {
      fTrack = dynamic_cast<TEveTrack*>(obj);
      return kTRUE;
   }
   return kFALSE;
}

//______________________________________________________________________________
Bool_t TEveTrackGL::ShouldDLCache(const TGLRnrCtx& rnrCtx) const
{
   // Override from TGLLogicalShape.
   // To account for large point-sizes we modify the projection matrix
   // during selection and thus we need a direct draw.

   if (rnrCtx.Selection()) return kFALSE;
   return fDLCache;
}

//______________________________________________________________________________
void TEveTrackGL::ProcessSelection(TGLRnrCtx & /*rnrCtx*/, TGLSelectRecord & rec)
{
   // Processes secondary selection from TGLViewer.
   // Calls TPointSet3D::PointSelected(Int_t) with index of selected
   // point as an argument.

   printf("TEveTrackGL::ProcessSelection %d names on the stack (z1=%g, z2=%g).\n",
          rec.GetN(), rec.GetMinZ(), rec.GetMaxZ());
   printf("  Names: ");
   for (Int_t j=0; j<rec.GetN(); ++j) printf ("%d ", rec.GetItem(j));
   printf("\n");

   ((TEveTrack*)fM)->CtrlClicked((TEveTrack*)fM);
}

//______________________________________________________________________________
void TEveTrackGL::DirectDraw(TGLRnrCtx & rnrCtx) const
{
   // GL rendering code.
   // Virtual from TGLLogicalShape.

   // path-marks
   std::vector<TEvePathMark*>& pm = fTrack->fPathMarks;
   TEveTrackPropagator& rTP = *fTrack->GetPropagator();
   if (pm.size())
   {
      Float_t* pnts = new Float_t[3*pm.size()]; // maximum
      Int_t n = 0;
      Bool_t accept;
      for (std::vector<TEvePathMark*>::iterator i=pm.begin(); i!=pm.end(); ++i)
      {
         accept = kFALSE;
         switch ((*i)->fType)
         {
            case TEvePathMark::kDaughter:
               if (rTP.GetRnrDaughters()) accept = kTRUE;
               break;
            case TEvePathMark::kReference:
               if (rTP.GetRnrReferences()) accept = kTRUE;
               break;
            case TEvePathMark::kDecay:
               if (rTP.GetRnrDecay()) accept = kTRUE;
               break;
         }
         if (accept)
         {
            if ((TMath::Abs((*i)->fV.fZ) < rTP.GetMaxZ()) && ((*i)->fV.Perp() < rTP.GetMaxR()))
            {
               pnts[3*n  ] =(*i)->fV.fX;
               pnts[3*n+1] =(*i)->fV.fY;
               pnts[3*n+2] =(*i)->fV.fZ;
               ++n;
            }
         }
      }
      TGLUtil::RenderPolyMarkers(rTP.RefPMAtt(), pnts, n,
                                 rnrCtx.GetPickRadius(),
                                 rnrCtx.Selection());
      delete [] pnts;
   }

   // fist vertex
   if (rTP.GetRnrFV() && fTrack->GetLastPoint())
      TGLUtil::RenderPolyMarkers(rTP.RefFVAtt(), fTrack->GetP(), 1,
                                 rnrCtx.GetPickRadius(),
                                 rnrCtx.Selection());

   TEveLineGL::DirectDraw(rnrCtx);
}
