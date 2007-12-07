// @(#)root/eve:$Id$
// Authors: Matevz Tadel & Alja Mrak-Tadel: 2006, 2007

/*************************************************************************
 * Copyright (C) 1995-2007, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#include "TMath.h"

#include "TEveQuadSetGL.h"
#include "TEveFrameBoxGL.h"

#include "TGLRnrCtx.h"
#include "TGLSelectRecord.h"
#include "TGLIncludes.h"

//______________________________________________________________________________
// TEveQuadSetGL
//
// GL-renderer for TEveQuadSet class.

ClassImp(TEveQuadSetGL)

/******************************************************************************/

TEveQuadSetGL::TEveQuadSetGL() : TGLObject(), fM(0)
{
   // fDLCache = false; // Disable DL.
}

//______________________________________________________________________________
TEveQuadSetGL::~TEveQuadSetGL()
{}

/******************************************************************************/

//______________________________________________________________________________
Bool_t TEveQuadSetGL::ShouldDLCache(const TGLRnrCtx & rnrCtx) const
{
   if (rnrCtx.DrawPass() == TGLRnrCtx::kPassOutlineLine)
      return kFALSE;
   return TGLObject::ShouldDLCache(rnrCtx);
}

/******************************************************************************/

//______________________________________________________________________________
Bool_t TEveQuadSetGL::SetModel(TObject* obj, const Option_t* /*opt*/)
{
   Bool_t ok = SetModelCheckClass(obj, TEveQuadSet::Class());
   fM = ok ? dynamic_cast<TEveQuadSet*>(obj) : 0;
   return ok;
}

//______________________________________________________________________________
void TEveQuadSetGL::SetBBox()
{
   SetAxisAlignedBBox(fM->AssertBBox());
}

/******************************************************************************/

//______________________________________________________________________________
inline Bool_t TEveQuadSetGL::SetupColor(const TEveDigitSet::DigitBase_t& q) const
{
   if (fM->fValueIsColor)
   {
      TGLUtil::Color4ubv((UChar_t*) & q.fValue);
      return kTRUE;
   }
   else
   {
      UChar_t c[4];
      Bool_t visible = fM->fPalette->ColorFromValue(q.fValue, fM->fDefaultValue, c);
      if (visible)
         TGLUtil::Color4ubv(c);
      return visible;
   }
}

/******************************************************************************/

//______________________________________________________________________________
void TEveQuadSetGL::DirectDraw(TGLRnrCtx & rnrCtx) const
{
   static const TEveException eH("TEveQuadSetGL::DirectDraw ");

   // printf("QuadSetGLRenderer::DirectDraw Style %d, LOD %d\n", rnrCtx.Style(), rnrCtx.LOD());

   if (rnrCtx.DrawPass() == TGLRnrCtx::kPassOutlineLine)
      return;

   TEveQuadSet& mQ = * fM;

   if (mQ.fFrame != 0 && ! rnrCtx.SecSelection())
      TEveFrameBoxGL::Render(mQ.fFrame);

   if (mQ.fPlex.Size() == 0)
      return;
   if ( ! mQ.fValueIsColor && mQ.fPalette == 0)
   {
      mQ.AssertPalette();
   }

   glPushAttrib(GL_ENABLE_BIT | GL_POLYGON_BIT);
   glColorMaterial(GL_FRONT_AND_BACK, GL_DIFFUSE);
   glEnable(GL_COLOR_MATERIAL);
   glDisable(GL_CULL_FACE);

   if (mQ.fRenderMode == TEveDigitSet::kRM_Fill)
      glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
   else if (mQ.fRenderMode == TEveDigitSet::kRM_TEveLine)
      glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

   if (mQ.fDisableLigting)  glDisable(GL_LIGHTING);

   if (mQ.fQuadType < TEveQuadSet::kQT_Rectangle_End)      RenderQuads(rnrCtx);
   else if (mQ.fQuadType < TEveQuadSet::kQT_Line_End)      RenderLines(rnrCtx);
   else if (mQ.fQuadType < TEveQuadSet::kQT_Hexagon_End)   RenderHexagons(rnrCtx);

   glPopAttrib();

}

//______________________________________________________________________________
void TEveQuadSetGL::RenderQuads(TGLRnrCtx & rnrCtx) const
{
   static const TEveException eH("TEveQuadSetGL::RenderQuads ");

   TEveQuadSet& mQ = * fM;

   GLenum primitiveType;
   if (mQ.fRenderMode != TEveDigitSet::kRM_TEveLine)
   {
      primitiveType = GL_QUADS;
      if (mQ.fQuadType == TEveQuadSet::kQT_FreeQuad)
         glEnable(GL_NORMALIZE);
      else
         glNormal3f(0, 0, 1);
   } else {
      primitiveType = GL_LINE_LOOP;
   }

   TEveChunkManager::iterator qi(mQ.fPlex);

   if (rnrCtx.SecSelection()) glPushName(0);

   switch (mQ.fQuadType)
   {

      case TEveQuadSet::kQT_FreeQuad:
      {
         Float_t e1[3], e2[3], normal[3];
         while (qi.next()) {
            TEveQuadSet::QFreeQuad_t& q = * (TEveQuadSet::QFreeQuad_t*) qi();
            if (SetupColor(q))
            {
               Float_t* p = q.fVertices;
               e1[0] = p[3] - p[0]; e1[1] = p[4] - p[1]; e1[2] = p[5] - p[2];
               e2[0] = p[6] - p[0]; e2[1] = p[7] - p[1]; e2[2] = p[8] - p[2];
               TMath::Cross(e1, e2, normal);
               if (rnrCtx.SecSelection()) glLoadName(qi.index());
               glBegin(primitiveType);
               glNormal3fv(normal);
               glVertex3fv(p);
               glVertex3fv(p + 3);
               glVertex3fv(p + 6);
               glVertex3fv(p + 9);
               glEnd();
            }
         }
         break;
      }

      case TEveQuadSet::kQT_RectangleXY:
      {
         while (qi.next()) {
            TEveQuadSet::QRect_t& q = * (TEveQuadSet::QRect_t*) qi();
            if (SetupColor(q))
            {
               if (rnrCtx.SecSelection()) glLoadName(qi.index());
               glBegin(primitiveType);
               glVertex3f(q.fA,        q.fB,        q.fC);
               glVertex3f(q.fA + q.fW, q.fB,        q.fC);
               glVertex3f(q.fA + q.fW, q.fB + q.fH, q.fC);
               glVertex3f(q.fA,        q.fB + q.fH, q.fC);
               glEnd();
            }
         }
         break;
      }

      case TEveQuadSet::kQT_RectangleXZ:
      {
         while (qi.next()) {
            TEveQuadSet::QRect_t& q = * (TEveQuadSet::QRect_t*) qi();
            if (SetupColor(q))
            {
               if (rnrCtx.SecSelection()) glLoadName(qi.index());
               glBegin(primitiveType);
               glVertex3f(q.fA,        q.fC, q.fB);
               glVertex3f(q.fA + q.fW, q.fC, q.fB);
               glVertex3f(q.fA + q.fW, q.fC, q.fB + q.fH);
               glVertex3f(q.fA,        q.fC, q.fB + q.fH);
               glEnd();
            }
         }
         break;
      }

      case TEveQuadSet::kQT_RectangleYZ:
      {
         while (qi.next()) {
            TEveQuadSet::QRect_t& q = * (TEveQuadSet::QRect_t*) qi();
            if (SetupColor(q))
            {
               if (rnrCtx.SecSelection()) glLoadName(qi.index());
               glBegin(primitiveType);
               glVertex3f(q.fC, q.fA,        q.fB);
               glVertex3f(q.fC, q.fA + q.fW, q.fB);
               glVertex3f(q.fC, q.fA + q.fW, q.fB + q.fH);
               glVertex3f(q.fC, q.fA,        q.fB + q.fH);
               glEnd();
            }
         }
         break;
      }

      case TEveQuadSet::kQT_RectangleXYFixedDim:
      {
         const Float_t& w = mQ.fDefWidth;
         const Float_t& h = mQ.fDefHeight;
         while (qi.next()) {
            TEveQuadSet::QRectFixDim_t& q = * (TEveQuadSet::QRectFixDim_t*) qi();
            if (SetupColor(q))
            {
               if (rnrCtx.SecSelection()) glLoadName(qi.index());
               glBegin(primitiveType);
               glVertex3f(q.fA,     q.fB,     q.fC);
               glVertex3f(q.fA + w, q.fB,     q.fC);
               glVertex3f(q.fA + w, q.fB + h, q.fC);
               glVertex3f(q.fA,     q.fB + h, q.fC);
               glEnd();
            }
         }
         break;
      }

      case TEveQuadSet::kQT_RectangleXYFixedZ:
      {
         const Float_t& z = mQ.fDefCoord;
         while (qi.next()) {
            TEveQuadSet::QRectFixC_t& q = * (TEveQuadSet::QRectFixC_t*) qi();
            if (SetupColor(q))
            {
               if (rnrCtx.SecSelection()) glLoadName(qi.index());
               glBegin(primitiveType);
               glVertex3f(q.fA,        q.fB,        z);
               glVertex3f(q.fA + q.fW, q.fB,        z);
               glVertex3f(q.fA + q.fW, q.fB + q.fH, z);
               glVertex3f(q.fA,        q.fB + q.fH, z);
               glEnd();
            }
         }
         break;
      }

      case TEveQuadSet::kQT_RectangleXZFixedY:
      {
         const Float_t& y = mQ.fDefCoord;
         while (qi.next()) {
            TEveQuadSet::QRectFixC_t& q = * (TEveQuadSet::QRectFixC_t*) qi();
            if (SetupColor(q))
            {
               if (rnrCtx.SecSelection()) glLoadName(qi.index());
               glBegin(primitiveType);
               glVertex3f(q.fA,        y, q.fB);
               glVertex3f(q.fA + q.fW, y, q.fB);
               glVertex3f(q.fA + q.fW, y, q.fB + q.fH);
               glVertex3f(q.fA,        y, q.fB + q.fH);
               glEnd();
            }
         }
         break;
      }

      case TEveQuadSet::kQT_RectangleYZFixedX:
      {
         const Float_t& x = mQ.fDefCoord;
         while (qi.next()) {
            TEveQuadSet::QRectFixC_t& q = * (TEveQuadSet::QRectFixC_t*) qi();
            if (SetupColor(q))
            {
               if (rnrCtx.SecSelection()) glLoadName(qi.index());
               glBegin(primitiveType);
               glVertex3f(x, q.fA,        q.fB);
               glVertex3f(x, q.fA + q.fW, q.fB);
               glVertex3f(x, q.fA + q.fW, q.fB + q.fH);
               glVertex3f(x, q.fA,        q.fB + q.fH);
               glEnd();
            }
         }
         break;
      }

      case TEveQuadSet::kQT_RectangleXYFixedDimZ:
      {
         const Float_t& z = mQ.fDefCoord;
         const Float_t& w = mQ.fDefWidth;
         const Float_t& h = mQ.fDefHeight;
         while (qi.next()) {
            TEveQuadSet::QRectFixDimC_t& q = * (TEveQuadSet::QRectFixDimC_t*) qi();
            if (SetupColor(q))
            {
               if (rnrCtx.SecSelection()) glLoadName(qi.index());
               glBegin(primitiveType);
               glVertex3f(q.fA,     q.fB,     z);
               glVertex3f(q.fA + w, q.fB,     z);
               glVertex3f(q.fA + w, q.fB + h, z);
               glVertex3f(q.fA,     q.fB + h, z);
               glEnd();
            }
         }
         break;
      }

      case TEveQuadSet::kQT_RectangleXZFixedDimY:
      {
         const Float_t& y = mQ.fDefCoord;
         const Float_t& w = mQ.fDefWidth;
         const Float_t& h = mQ.fDefHeight;
         while (qi.next()) {
            TEveQuadSet::QRectFixDimC_t& q = * (TEveQuadSet::QRectFixDimC_t*) qi();
            if (SetupColor(q))
            {
               if (rnrCtx.SecSelection()) glLoadName(qi.index());
               glBegin(primitiveType);
               glVertex3f(q.fA,     y, q.fB);
               glVertex3f(q.fA + w, y, q.fB);
               glVertex3f(q.fA + w, y, q.fB + h);
               glVertex3f(q.fA,     y, q.fB + h);
               glEnd();
            }
         }
         break;
      }

      case TEveQuadSet::kQT_RectangleYZFixedDimX:
      {
         const Float_t& x = mQ.fDefCoord;
         const Float_t& w = mQ.fDefWidth;
         const Float_t& h = mQ.fDefHeight;
         while (qi.next()) {
            TEveQuadSet::QRectFixDimC_t& q = * (TEveQuadSet::QRectFixDimC_t*) qi();
            if (SetupColor(q))
            {
               if (rnrCtx.SecSelection()) glLoadName(qi.index());
               glBegin(primitiveType);
               glVertex3f(x, q.fA,     q.fB);
               glVertex3f(x, q.fA + w, q.fB);
               glVertex3f(x, q.fA + w, q.fB + h);
               glVertex3f(x, q.fA,     q.fB + h);
               glEnd();
            }
         }
         break;
      }

      default:
         throw(eH + "unsupported quad-type.");

   } // end switch quad-type

   if (rnrCtx.SecSelection()) glPopName();
}

//______________________________________________________________________________
void TEveQuadSetGL::RenderLines(TGLRnrCtx & rnrCtx) const
{
   static const TEveException eH("TEveQuadSetGL::RenderLines ");

   TEveQuadSet& mQ = * fM;

   TEveChunkManager::iterator qi(mQ.fPlex);

   if (rnrCtx.SecSelection()) glPushName(0);

   switch (mQ.fQuadType)
   {

      case TEveQuadSet::kQT_LineXYFixedZ:
      {
         const Float_t& z = mQ.fDefCoord;
         while (qi.next()) {
            TEveQuadSet::QLineFixC_t& q = * (TEveQuadSet::QLineFixC_t*) qi();
            if (SetupColor(q))
            {
               if (rnrCtx.SecSelection()) glLoadName(qi.index());
               glBegin(GL_LINES);
               glVertex3f(q.fA,         q.fB,         z);
               glVertex3f(q.fA + q.fDx, q.fB + q.fDy, z);
               glEnd();
            }
         }
         break;
      }

      case TEveQuadSet::kQT_LineXZFixedY:
      {
         const Float_t& z = mQ.fDefCoord;
         while (qi.next()) {
            TEveQuadSet::QLineFixC_t& q = * (TEveQuadSet::QLineFixC_t*) qi();
            if (SetupColor(q))
            {
               if (rnrCtx.SecSelection()) glLoadName(qi.index());
               glBegin(GL_LINES);
               glVertex3f(q.fA,         z, q.fB);
               glVertex3f(q.fA + q.fDx, z, q.fB + q.fDy);
               glEnd();
            }
         }
         break;
      }

      default:
         throw(eH + "unsupported quad-type.");

   }

   if (rnrCtx.SecSelection()) glPopName();
}

//______________________________________________________________________________
void TEveQuadSetGL::RenderHexagons(TGLRnrCtx & rnrCtx) const
{
   static const TEveException eH("TEveQuadSetGL::RenderHexagons ");

   const Float_t sqr3hf = 0.5*TMath::Sqrt(3);

   TEveQuadSet& mQ = * fM;

   GLenum primitveType = (mQ.fRenderMode != TEveDigitSet::kRM_TEveLine) ?
      GL_POLYGON : GL_LINE_LOOP;

   glNormal3f(0, 0, 1);

   TEveChunkManager::iterator qi(mQ.fPlex);

   if (rnrCtx.SecSelection()) glPushName(0);

   switch (mQ.fQuadType)
   {

      case TEveQuadSet::kQT_HexagonXY:
      {
         while (qi.next()) {
            TEveQuadSet::QHex_t& q = * (TEveQuadSet::QHex_t*) qi();
            if (SetupColor(q))
            {
               const Float_t rh = q.fR * 0.5;
               const Float_t rs = q.fR * sqr3hf;
               if (rnrCtx.SecSelection()) glLoadName(qi.index());
               glBegin(primitveType);
               glVertex3f( q.fR + q.fA,       q.fB, q.fC);
               glVertex3f(   rh + q.fA,  rs + q.fB, q.fC);
               glVertex3f(  -rh + q.fA,  rs + q.fB, q.fC);
               glVertex3f(-q.fR + q.fA,       q.fB, q.fC);
               glVertex3f(  -rh + q.fA, -rs + q.fB, q.fC);
               glVertex3f(   rh + q.fA, -rs + q.fB, q.fC);
               glEnd();
            }
         }
         break;
      }

      case TEveQuadSet::kQT_HexagonYX:
      {
         while (qi.next()) {
            TEveQuadSet::QHex_t& q = * (TEveQuadSet::QHex_t*) qi();
            if (SetupColor(q))
            {
               const Float_t rh = q.fR * 0.5;
               const Float_t rs = q.fR * sqr3hf;
               if (rnrCtx.SecSelection()) glLoadName(qi.index());
               glBegin(primitveType);
               glVertex3f( rs + q.fA,    rh + q.fB, q.fC);
               glVertex3f(      q.fA,  q.fR + q.fB, q.fC);
               glVertex3f(-rs + q.fA,    rh + q.fB, q.fC);
               glVertex3f(-rs + q.fA,   -rh + q.fB, q.fC);
               glVertex3f(      q.fA, -q.fR + q.fB, q.fC);
               glVertex3f( rs + q.fA,   -rh + q.fB, q.fC);
               glEnd();
            }
         }
         break;
      }

      default:
         throw(eH + "unsupported quad-type.");

   } // end switch quad-type

   if (rnrCtx.SecSelection()) glPopName();
}

/******************************************************************************/

//______________________________________________________________________________
void TEveQuadSetGL::ProcessSelection(TGLRnrCtx & /*rnrCtx*/, TGLSelectRecord & rec)
{
   // Processes secondary selection from TGLViewer.
   // Calls TPointSet3D::PointSelected(Int_t) with index of selected
   // point as an argument.

   if (rec.GetN() < 2) return;
   fM->DigitSelected(rec.GetItem(1));
}
