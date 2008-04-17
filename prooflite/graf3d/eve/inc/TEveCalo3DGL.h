// @(#)root/eve:$Id$
// Author: Matevz Tadel 2007

/*************************************************************************
 * Copyright (C) 1995-2007, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#ifndef ROOT_TEveCalo3DGL
#define ROOT_TEveCalo3DGL

#include "TGLObject.h"
#include "TEveCaloData.h"

class TEveCalo3D;

class TEveCalo3DGL : public TGLObject
{
private:
   TEveCalo3DGL(const TEveCalo3DGL&);            // Not implemented
   TEveCalo3DGL& operator=(const TEveCalo3DGL&); // Not implemented

   void    CrossProduct(const Float_t a[3], const Float_t b[3], const Float_t c[3], Float_t out[3]) const;

   void    RenderBox(const Float_t pnts[8]) const;
   Float_t RenderBarrelCell(const TEveCaloData::CellData_t &cell, Float_t towerH, Float_t offset) const;
   Float_t RenderEndCapCell(const TEveCaloData::CellData_t &cell, Float_t towerH, Float_t offset) const;

protected:
   TEveCalo3D     *fM;  // Model object.

public:
   TEveCalo3DGL();
   virtual ~TEveCalo3DGL() {}

   virtual Bool_t SetModel(TObject* obj, const Option_t* opt=0);
   virtual void   SetBBox();

   virtual Bool_t ShouldDLCache(const TGLRnrCtx & rnrCtx) const;

   virtual void   DirectDraw(TGLRnrCtx & rnrCtx) const;

   virtual Bool_t SupportsSecondarySelect() const { return kTRUE; }
   virtual void   ProcessSelection(TGLRnrCtx & rnrCtx, TGLSelectRecord & rec);

   ClassDef(TEveCalo3DGL, 0); // GL renderer class for TEveCalo.
};

#endif
