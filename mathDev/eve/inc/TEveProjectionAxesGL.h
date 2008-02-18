// @(#)root/eve:$Id$
// Author: Matevz Tadel 2007

/*************************************************************************
 * Copyright (C) 1995-2007, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#ifndef ROOT_TEveProjectionAxesGL
#define ROOT_TEveProjectionAxesGL

#include "TEveTextGL.h"
#include <list>

class FTFont;
class TEveProjectionAxes;
class TEveProjection;

class TEveProjectionAxesGL : public TEveTextGL
{
private:
   TEveProjectionAxesGL(const TEveProjectionAxesGL&);            // Not implemented
   TEveProjectionAxesGL& operator=(const TEveProjectionAxesGL&); // Not implemented

   mutable Float_t    fRange;     // bounding box size in the current axis
   const   Float_t    fLabelSize; // size of labels
   const   Float_t    fLabelOff;  // distance between labels and tick-marks
   const   Float_t    fTMSize;    // tick-mark size

   typedef std::pair<Float_t, Float_t>  TM_t;
   typedef std::list<TM_t>              TMList_t; 

   mutable TMList_t   fTMList;    // list of tick-mark position-value pairs

   void               DrawTickMarks(Float_t tms) const;
   void               DrawHInfo() const;
   void               DrawVInfo() const;
   const char*        GetText(Float_t) const;

   void               SplitInterval(Int_t axis) const;
   void               SplitIntervalByPos(Float_t min, Float_t max, Int_t axis, Int_t level)const;
   void               SplitIntervalByVal(Float_t min, Float_t max, Int_t axis, Int_t level)const;

   void               SetRange(Float_t val, Int_t axis) const;
   void               RenderText(const char* txt, Float_t x, Float_t y) const;

protected:
   TEveProjectionAxes     *fAxesModel;  // model object.
   mutable TEveProjection *fProjection; // cached model projection

public:
   TEveProjectionAxesGL();
   virtual ~TEveProjectionAxesGL() {}

   virtual Bool_t  SetModel(TObject* obj, const Option_t* opt=0);
   virtual void    SetBBox();
   virtual void    DirectDraw(TGLRnrCtx & rnrCtx) const;

   Bool_t IgnoreSizeForOfInterest() const { return kTRUE;}


   ClassDef(TEveProjectionAxesGL, 0); // GL renderer class for TEveProjectionAxes.
};

#endif
