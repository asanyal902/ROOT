// @(#)root/eve:$Id$
// Author: Matevz Tadel 2007

/*************************************************************************
 * Copyright (C) 1995-2007, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#include "TEveArrowGL.h"
#include "TEveArrow.h"

#include "TGLRnrCtx.h"
#include "TGLIncludes.h"
#include "TGLUtil.h"
#include "TGLQuadric.h"

// #include "TAttLine.h"

//______________________________________________________________________________
// OpenGL renderer class for TEveArrow.
//

ClassImp(TEveArrowGL);

//______________________________________________________________________________
TEveArrowGL::TEveArrowGL() :
   TGLObject(), fM(0)
{
   // Constructor.
}

/******************************************************************************/

//______________________________________________________________________________
Bool_t TEveArrowGL::SetModel(TObject* obj, const Option_t* /*opt*/)
{
   // Set model object.

   if (SetModelCheckClass(obj, TEveArrow::Class())) {
      fM = dynamic_cast<TEveArrow*>(obj);
      return kTRUE;
   }
   return kFALSE;
}

//______________________________________________________________________________
void TEveArrowGL::SetBBox()
{
   // Set bounding box.

   // !! This ok if master sub-classed from TAttBBox
   SetAxisAlignedBBox(((TEveArrow*)fExternalObj)->AssertBBox());
}

/******************************************************************************/

//______________________________________________________________________________
void TEveArrowGL::DirectDraw(TGLRnrCtx & rnrCtx) const
{
   // Render with OpenGL.

   // printf("TEveArrowGL::DirectDraw LOD \n");

   static TGLQuadric quad;
   UInt_t drawQuality = 10;
   // Draw 3D line (tube) with optional head shape

   // glPushAttrib(GL_ENABLE_BIT | GL_POLYGON_BIT);
   glDisable(GL_CULL_FACE);

   glPushMatrix();

   TGLVertex3 uo(fM->fOrigin.fX, fM->fOrigin.fY, fM->fOrigin.fZ);
   TGLVector3 uv(fM->fVector.fX, fM->fVector.fY, fM->fVector.fZ);
   TGLMatrix local(uo, uv);
   glMultMatrixd(local.CArr());
   Float_t size = fM->fVector.Mag();

   // Line (tube) component
   Float_t tr = size*fM->fTubeR;
   Float_t hh = size*fM->fConeL;
   gluCylinder(quad.Get(), tr, tr, size - hh, drawQuality, 1);

   // disks
   gluQuadricOrientation(quad.Get(), (GLenum)GLU_INSIDE);
   gluDisk(quad.Get(), 0.0, tr, drawQuality, 1);
   glTranslated(0.0, 0.0, size -hh );
   gluDisk(quad.Get(), 0.0, tr, drawQuality, 1);

   // Arrow cone
   gluQuadricOrientation(quad.Get(), (GLenum)GLU_OUTSIDE);
   gluCylinder(quad.Get(), size*fM->fConeR, 0., hh , drawQuality, 1);
   gluDisk(quad.Get(), 0.0, size*fM->fConeR, drawQuality, 1);

   glPopMatrix();
   // glPopAttrib();
}

