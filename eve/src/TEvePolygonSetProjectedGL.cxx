// @(#)root/eve:$Id$
// Authors: Matevz Tadel & Alja Mrak-Tadel: 2006, 2007

/*************************************************************************
 * Copyright (C) 1995-2007, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#include "TEvePolygonSetProjectedGL.h"
#include "TEvePolygonSetProjected.h"
#include "TEveVSDStructs.h"

#include "TGLRnrCtx.h"
#include "TGLIncludes.h"

//______________________________________________________________________________
// TEvePolygonSetProjectedGL
//
// GL-renderer for TEvePolygonSetProjected class.

ClassImp(TEvePolygonSetProjectedGL)

//______________________________________________________________________________
TEvePolygonSetProjectedGL::TEvePolygonSetProjectedGL() : TGLObject()
{
   // Constructor

   // fDLCache = false; // Disable DL.
}

//______________________________________________________________________________
TEvePolygonSetProjectedGL::~TEvePolygonSetProjectedGL()
{
   // Destructor. Noop.
}

/******************************************************************************/
Bool_t TEvePolygonSetProjectedGL::SetModel(TObject* obj, const Option_t* /*opt*/)
{
   // Set model object.

   return SetModelCheckClass(obj, TEvePolygonSetProjected::Class());
}

/******************************************************************************/

//______________________________________________________________________________
void TEvePolygonSetProjectedGL::SetBBox()
{
   // Setup bounding-box information.

   SetAxisAlignedBBox(((TEvePolygonSetProjected*)fExternalObj)->AssertBBox());
}

/******************************************************************************/

static GLUtriangulatorObj *GetTesselator()
{
   // Return default static GLU triangulator object.

   static struct Init {
      Init()
      {
#if defined(R__WIN32)
         typedef void (CALLBACK *tessfuncptr_t)();
#elif defined(R__AIXGCC)
         typedef void (*tessfuncptr_t)(...);
#else
         typedef void (*tessfuncptr_t)();
#endif
         fTess = gluNewTess();

         if (!fTess) {
            Error("GetTesselator::Init", "could not create tesselation object");
         } else {
            gluTessCallback(fTess, (GLenum)GLU_BEGIN,  (tessfuncptr_t) glBegin);
            gluTessCallback(fTess, (GLenum)GLU_END,    (tessfuncptr_t) glEnd);
            gluTessCallback(fTess, (GLenum)GLU_VERTEX, (tessfuncptr_t) glVertex3fv);
         }
      }
      ~Init()
      {
         if(fTess)
            gluDeleteTess(fTess);
      }
      GLUtriangulatorObj *fTess;
   } singleton;

   return singleton.fTess;
}

/******************************************************************************/

//______________________________________________________________________________
void TEvePolygonSetProjectedGL::DirectDraw(TGLRnrCtx & /*rnrCtx*/) const
{
   // Do GL rendering.

   TEvePolygonSetProjected& PS = * (TEvePolygonSetProjected*) fExternalObj;
   if(PS.fPols.size() == 0) return;

   glPushAttrib(GL_ENABLE_BIT | GL_LINE_BIT | GL_POLYGON_BIT);

   glDisable(GL_LIGHTING);
   glColorMaterial(GL_FRONT_AND_BACK, GL_DIFFUSE);
   glEnable(GL_COLOR_MATERIAL);
   glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
   glDisable(GL_CULL_FACE);

   // polygons
   glEnable(GL_POLYGON_OFFSET_FILL);
   glPolygonOffset(1.,1.);
   GLUtriangulatorObj *tessObj = GetTesselator();

   TEveVector* pnts = PS.fPnts;
   for (TEvePolygonSetProjected::vpPolygon_ci i = PS.fPols.begin(); i!= PS.fPols.end(); i++)
   {
      Int_t vi; //current vertex index of curent polygon
      Int_t N = (*i).fNPnts; // number of points in current polygon
      if(N < 4)
      {
         glBegin(GL_POLYGON);
         for(Int_t k=0; k<N; k++)
         {
            vi = (*i).fPnts[k];
            glVertex3fv(pnts[vi].Arr());
         }
         glEnd();
      }
      else {
         gluBeginPolygon(tessObj);
         gluNextContour(tessObj, (GLenum)GLU_UNKNOWN);
         glNormal3f(0., 0., 1.);
         Double_t coords[3];
         coords[2] = 0.;
         for (Int_t k = 0; k<N; k++)
         {
            vi = (*i).fPnts[k];
            coords[0] = pnts[vi].fX;
            coords[1] = pnts[vi].fY;
            gluTessVertex(tessObj, coords, pnts[vi].Arr());
         }
         gluEndPolygon(tessObj);
      }
   }
   glDisable(GL_POLYGON_OFFSET_FILL);

   // outline
   UChar_t lcol[4];
   TEveUtil::ColorFromIdx(PS.fLineColor, lcol);
   TGLUtil::Color4ubv(lcol);
   glEnable(GL_LINE_SMOOTH);

   glLineWidth(PS.fLineWidth);
   Int_t vi;
   for (TEvePolygonSetProjected::vpPolygon_ci i = PS.fPols.begin(); i!= PS.fPols.end(); i++)
   {
      glBegin(GL_LINE_LOOP);
      for(Int_t k=0; k<(*i).fNPnts; k++)
      {
         vi = (*i).fPnts[k];
         glVertex3fv(PS.fPnts[vi].Arr());
      }
      glEnd();
   }

   glPopAttrib();
}
