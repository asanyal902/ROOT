// @(#)root/eve:$Id$
// Authors: Matevz Tadel & Alja Mrak-Tadel: 2006, 2007

/*************************************************************************
 * Copyright (C) 1995-2007, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#include "TEveBoxSetGL.h"
#include "TEveBoxSet.h"

#include "TGLIncludes.h"
#include "TGLRnrCtx.h"
#include "TGLScene.h"
#include "TGLSelectRecord.h"
#include "TGLContext.h"

//______________________________________________________________________________
// TEveBoxSetGL
//
// A GL rendering class for TEveBoxSet.
//

ClassImp(TEveBoxSetGL)

//______________________________________________________________________________
TEveBoxSetGL::TEveBoxSetGL() : fM(0), fBoxDL(0)
{
   // Default constructor.

   // fDLCache = false; // Disable display list.
}

//______________________________________________________________________________
TEveBoxSetGL::~TEveBoxSetGL()
{
   // Destructor. Noop.
}

/******************************************************************************/
// Protected methods
/******************************************************************************/

//______________________________________________________________________________
Int_t TEveBoxSetGL::PrimitiveType() const
{
   // Return GL primitive used to render the boxes, based on the
   // render-mode specified in the model object.

   return (fM->fRenderMode != TEveDigitSet::RM_TEveLine) ? GL_QUADS : GL_LINE_LOOP;
}

//______________________________________________________________________________
inline Bool_t TEveBoxSetGL::SetupColor(const TEveDigitSet::DigitBase& q) const
{
   // Set GL color for given primitive.

   if (fM->fValueIsColor)
   {
      glColor4ubv((UChar_t*) & q.fValue);
      return kTRUE;
   }
   else
   {
      UChar_t c[4];
      Bool_t visible = fM->fPalette->ColorFromValue(q.fValue, fM->fDefaultValue, c);
      if (visible)
         glColor4ubv(c);
      return visible;
   }
}

//______________________________________________________________________________
void TEveBoxSetGL::MakeOriginBox(Float_t p[24], Float_t dx, Float_t dy, Float_t dz) const
{
   // Fill array p to represent a box (0,0,0) - (dx,dy,dz).

   // bottom
   p[0] = 0;  p[1] = dy; p[2] = 0;  p += 3;
   p[0] = dx; p[1] = dy; p[2] = 0;  p += 3;
   p[0] = dx; p[1] = 0;  p[2] = 0;  p += 3;
   p[0] = 0;  p[1] = 0;  p[2] = 0;  p += 3;
   // top
   p[0] = 0;  p[1] = dy; p[2] = dz; p += 3;
   p[0] = dx; p[1] = dy; p[2] = dz; p += 3;
   p[0] = dx; p[1] = 0;  p[2] = dz; p += 3;
   p[0] = 0;  p[1] = 0;  p[2] = dz;
}

//______________________________________________________________________________
inline void TEveBoxSetGL::RenderBox(const Float_t p[24]) const
{
   // Render a box specified by points in array p.

   // bottom: 0123
   glNormal3f(0, 0, -1);
   glVertex3fv(p);      glVertex3fv(p + 3);
   glVertex3fv(p + 6);  glVertex3fv(p + 9);
   // top:    7654
   glNormal3f(0, 0, 1);
   glVertex3fv(p + 21); glVertex3fv(p + 18);
   glVertex3fv(p + 15); glVertex3fv(p + 12);
   // back:  0451
   glNormal3f(0, 1, 0);
   glVertex3fv(p);      glVertex3fv(p + 12);
   glVertex3fv(p + 15); glVertex3fv(p + 3);
   // front:   3267
   glNormal3f(0, -1, 0);
   glVertex3fv(p + 9);   glVertex3fv(p + 6);
   glVertex3fv(p + 18);  glVertex3fv(p + 21);
   // left:    0374
   glNormal3f(-1, 0, 0);
   glVertex3fv(p);       glVertex3fv(p + 9);
   glVertex3fv(p + 21);  glVertex3fv(p + 12);
   // right:   1562
   glNormal3f(1, 0, 0);
   glVertex3fv(p + 3);   glVertex3fv(p + 15);
   glVertex3fv(p + 18);  glVertex3fv(p + 6);
}

//______________________________________________________________________________
void TEveBoxSetGL::MakeDisplayList() const
{
   // Create a display-list for rendering a single box, based on the
   // current box-type.
   // Some box-types don't benefit from the display-list rendering and
   // so display-list is not created.

   if (fM->fBoxType == TEveBoxSet::BT_AABox ||
       fM->fBoxType == TEveBoxSet::BT_AABoxFixedDim)
   {
      if (fBoxDL == 0)
         fBoxDL = glGenLists(1);

      Float_t p[24];
      if (fM->fBoxType == TEveBoxSet::BT_AABox)
         MakeOriginBox(p, 1.0f, 1.0f, 1.0f);
      else
         MakeOriginBox(p, fM->fDefWidth, fM->fDefHeight, fM->fDefDepth);

      glNewList(fBoxDL, GL_COMPILE);
      glBegin(PrimitiveType());
      RenderBox(p);
      glEnd();
      glEndList();
   }
}

/******************************************************************************/
// Virtuals from base-classes
/******************************************************************************/

//______________________________________________________________________________
Bool_t TEveBoxSetGL::ShouldDLCache(const TGLRnrCtx & rnrCtx) const
{
   // Determines if display-list will be used for rendering.
   // Virtual from TGLLogicalShape.

   MakeDisplayList();

   if (rnrCtx.DrawPass() == TGLRnrCtx::kPassOutlineLine)
      return kFALSE;
   return TGLObject::ShouldDLCache(rnrCtx);
}

//______________________________________________________________________________
void TEveBoxSetGL::DLCacheDrop()
{
   // Called when display lists have been destroyed externally and the
   // internal display-list data needs to be cleare.
   // Virtual from TGLLogicalShape.

   fBoxDL = 0;
   TGLObject::DLCacheDrop();
}

//______________________________________________________________________________
void TEveBoxSetGL::DLCachePurge()
{
   // Called when display-lists need to be returned to the system.
   // Virtual from TGLLogicalShape.

   static const TEveException eH("TEveBoxSetGL::DLCachePurge ");

   if (fBoxDL == 0) return;
   if (fScene)
   {
      fScene->GetGLCtxIdentity()->RegisterDLNameRangeToWipe(fBoxDL, 1);
   }
   else
   {
      Warning(eH, "TEveScene unknown, attempting direct deletion.");
      glDeleteLists(fBoxDL, 1);
   }
   TGLObject::DLCachePurge();
}

/******************************************************************************/

//______________________________________________________________________________
Bool_t TEveBoxSetGL::SetModel(TObject* obj, const Option_t* /*opt*/)
{
   // Set model object.
   // Virtual from TGLObject.

   Bool_t isok = SetModelCheckClass(obj, TEveBoxSet::Class());
   fM = isok ? dynamic_cast<TEveBoxSet*>(obj) : 0;
   return isok;
}

//______________________________________________________________________________
void TEveBoxSetGL::SetBBox()
{
   // Fill the bounding-box data of the logical-shape.
   // Virtual from TGLObject.

   SetAxisAlignedBBox(fM->AssertBBox());
}

//______________________________________________________________________________
void TEveBoxSetGL::DirectDraw(TGLRnrCtx & rnrCtx) const
{
   // Actual rendering code.
   // Virtual from TGLLogicalShape.

   static const TEveException eH("TEveBoxSetGL::DirectDraw ");

   if (rnrCtx.DrawPass() == TGLRnrCtx::kPassOutlineLine)
      return;

   TEveBoxSet& mB = * fM;
   // printf("TEveBoxSetGL::DirectDraw N boxes %d\n", mB.fPlex.Size());
   if(mB.fPlex.Size() == 0)
      return;

   glPushAttrib(GL_ENABLE_BIT | GL_POLYGON_BIT);
   glColorMaterial(GL_FRONT_AND_BACK, GL_DIFFUSE);
   glEnable(GL_COLOR_MATERIAL);

   if (mB.fRenderMode == TEveDigitSet::RM_Fill)
      glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
   else if (mB.fRenderMode == TEveDigitSet::RM_TEveLine)
      glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

   if (mB.fDisableLigting) glDisable(GL_LIGHTING);

   if (rnrCtx.SecSelection()) glPushName(0);

   Int_t boxSkip = 0;
   if (rnrCtx.ShapeLOD() < 50)
      boxSkip = 6 - (rnrCtx.ShapeLOD()+1)/10;

   TEveChunkManager::iterator bi(mB.fPlex);

   switch (mB.fBoxType)
   {

      case TEveBoxSet::BT_FreeBox:
      {
         GLenum primitiveType = PrimitiveType();
         while (bi.next())
         {
            TEveBoxSet::BFreeBox& b = * (TEveBoxSet::BFreeBox*) bi();
            if (SetupColor(b))
            {
               if (rnrCtx.SecSelection()) glLoadName(bi.index());
               glBegin(primitiveType);
               RenderBox(b.fVertices);
               glEnd();
            }
            if (boxSkip) { Int_t s = boxSkip; while (s--) bi.next(); }
         }
         break;
      } // end case free-box

      case TEveBoxSet::BT_AABox:
      {
         glEnable(GL_NORMALIZE);
         while (bi.next())
         {
            TEveBoxSet::BAABox& b = * (TEveBoxSet::BAABox*) bi();
            if (SetupColor(b))
            {
               if (rnrCtx.SecSelection()) glLoadName(bi.index());
               glPushMatrix();
               glTranslatef(b.fA, b.fB, b.fC);
               glScalef    (b.fW, b.fH, b.fD);
               glCallList(fBoxDL);
               glPopMatrix();
            }
            if (boxSkip) { Int_t s = boxSkip; while (s--) bi.next(); }
         }
         break;
      }

      case TEveBoxSet::BT_AABoxFixedDim:
      {
         while (bi.next())
         {
            TEveBoxSet::BAABoxFixedDim& b = * (TEveBoxSet::BAABoxFixedDim*) bi();
            if (SetupColor(b))
            {
               if (rnrCtx.SecSelection()) glLoadName(bi.index());
               glTranslatef(b.fA, b.fB, b.fC);
               glCallList(fBoxDL);
               glTranslatef(-b.fA, -b.fB, -b.fC);
            }
            if (boxSkip) { Int_t s = boxSkip; while (s--) bi.next(); }
         }
         break;
      }

      default:
      {
         throw(eH + "unsupported box-type.");
      }

   } // end switch box-type

   if (rnrCtx.SecSelection()) glPopName();

   glPopAttrib();
}

/******************************************************************************/

//______________________________________________________________________________
void TEveBoxSetGL::ProcessSelection(TGLRnrCtx & /*rnrCtx*/, TGLSelectRecord & rec)
{
   // Processes secondary selection from TGLViewer.
   // Calls TPointSet3D::PointSelected(Int_t) with index of selected
   // point as an argument.

   if (rec.GetN() < 2) return;
   fM->DigitSelected(rec.GetItem(1));
}

//______________________________________________________________________________
void TEveBoxSetGL::Render(TGLRnrCtx & rnrCtx)
{
   // Interface for direct rendering from classes that include TEveBoxSet
   // as a member.

   MakeDisplayList();
   DirectDraw(rnrCtx);
   glDeleteLists(fBoxDL, 1);
   fBoxDL = 0;
}
