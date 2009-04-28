// @(#)root/gl:$Id$
// Author:  Matevz Tadel, Jun 2007

/*************************************************************************
 * Copyright (C) 1995-2004, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#include "TF2GL.h"

#include <TF2.h>
#include <TF3.h>
#include <TH2.h>
#include <TVirtualPad.h>

#include "TGLSurfacePainter.h"
#include "TGLTF3Painter.h"
#include "TGLAxisPainter.h"

#include "TGLRnrCtx.h"

#include "TGLIncludes.h"

//______________________________________________________________________
//
// GL renderer for TF2.
// TGLPlotPainter is used internally.

ClassImp(TF2GL);

//______________________________________________________________________________
TF2GL::TF2GL() : TGLObject(), fM(0), fH(0)
{
   // Constructor.

   fDLCache = kFALSE; // Disable display list.
}

//______________________________________________________________________________
TF2GL::~TF2GL()
{
   // Destructor.

   delete fH;
   delete fPlotPainter;
}

/**************************************************************************/

//______________________________________________________________________________
Bool_t TF2GL::SetModel(TObject* obj, const Option_t* opt)
{
   // Set model object.

   TString option(opt);
   option.ToLower();

   if(SetModelCheckClass(obj, TF2::Class()))
   {
      fM = dynamic_cast<TF2*>(obj);
      fH = (TH2*) fM->CreateHistogram();
      fH->GetZaxis()->SetLimits(fH->GetMinimum(), fH->GetMaximum());

      if (dynamic_cast<TF3*>(fM))
         fPlotPainter = new TGLTF3Painter((TF3*)fM, fH, 0, &fCoord);
      else
         fPlotPainter = new TGLSurfacePainter(fH, 0, &fCoord);

      // Coord-system
      fCoord.SetXLog(gPad->GetLogx());
      fCoord.SetYLog(gPad->GetLogy());
      fCoord.SetZLog(gPad->GetLogz());

      if (option.Index("sph") != kNPOS)
         fCoord.SetCoordType(kGLSpherical);
      else if (option.Index("pol") != kNPOS)
         fCoord.SetCoordType(kGLPolar);
      else if (option.Index("cyl") != kNPOS)
         fCoord.SetCoordType(kGLCylindrical);

      fPlotPainter->AddOption(option);

      fPlotPainter->InitGeometry();
      return kTRUE;
   }
   return kFALSE;
}

//______________________________________________________________________________
void TF2GL::SetBBox()
{
   // Setup bounding-box.

   fBoundingBox.Set(fPlotPainter->RefBackBox().Get3DBox());
}

//______________________________________________________________________________
void TF2GL::DirectDraw(TGLRnrCtx & rnrCtx) const
{
   // Render the object.

   fPlotPainter->RefBackBox().FindFrontPoint();

   glPushAttrib(GL_ENABLE_BIT | GL_LIGHTING_BIT);

   glEnable(GL_NORMALIZE);
   glDisable(GL_COLOR_MATERIAL);

   fPlotPainter->InitGL();
   fPlotPainter->DrawPlot();

   glDisable(GL_CULL_FACE);
   glPopAttrib();

   // Axes
   TGLAxisPainterBox axe_painter;
   axe_painter.SetFontMode(TGLFont::kPixmap);
   axe_painter.PlotStandard(rnrCtx, fH, fBoundingBox);
}
