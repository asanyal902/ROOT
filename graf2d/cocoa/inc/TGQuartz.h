// @(#)root/graf2d:$Id$
// Author: Olivier Couet and Timur Pocheptsov 23/01/2012

/*************************************************************************
 * Copyright (C) 1995-2011, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/


#ifndef ROOT_TGQuartz
#define ROOT_TGQuartz

#include <vector>

#ifndef ROOT_TGCocoa
#include "TGCocoa.h"
#endif
#ifndef ROOT_TPoint
#include "TPoint.h"
#endif

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// This is non-GUI part of TVirtualX interface, implemented for         //
// MacOS X, using CoreGraphics (Quartz).                                //
//                                                                      //
//////////////////////////////////////////////////////////////////////////
class TGQuartz : public TGCocoa {
public:
   TGQuartz();
   TGQuartz(const char *name, const char *title);
   
   //Final-overriders for TVirtualX.
   virtual void      DrawBox(Int_t x1, Int_t y1, Int_t x2, Int_t y2,
                             EBoxMode mode);
   virtual void      DrawCellArray(Int_t x1, Int_t y1, Int_t x2, Int_t y2,
                                   Int_t nx, Int_t ny, Int_t *ic);
   virtual void      DrawFillArea(Int_t n, TPoint *xy);
   
   using TGCocoa::DrawLine;//There is a GUI version of DrawLine.
   
   virtual void      DrawLine(Int_t x1, Int_t y1, Int_t x2, Int_t y2);
   virtual void      DrawPolyLine(Int_t n, TPoint *xy);
   virtual void      DrawPolyMarker(Int_t n, TPoint *xy);
   virtual void      DrawText(Int_t x, Int_t y, Float_t angle, Float_t mgn, 
                              const char *text, ETextMode mode);
   
   //I have to override these setters, since they overriden
   //in TVirtualX (originally, they are declared in TAttXXX classes)
   //and do nothing in TVirtualX (though, they are implemented
   //correctly in TAttXXX classes).
   virtual void      SetFillColor(Color_t cindex);
   virtual void      SetFillStyle(Style_t style);
   virtual void      SetLineColor(Color_t cindex);
   virtual void      SetLineStyle(Style_t linestyle);
   virtual void      SetLineWidth(Width_t width);
   virtual void      SetMarkerColor(Color_t cindex);
   virtual void      SetMarkerSize(Float_t markersize);
   virtual void      SetMarkerStyle(Style_t markerstyle);
   virtual void      SetOpacity(Int_t percent);
   virtual void      SetTextAlign(Short_t talign=11);
   virtual void      SetTextColor(Color_t cindex);
   virtual void      SetTextFont(Font_t fontnumber);
   virtual Int_t     SetTextFont(char *fontname, ETextSetMode mode);
   virtual void      SetTextSize(Float_t textsize);
   
   virtual void      GetTextExtent(UInt_t &w, UInt_t &h, char *text);
   virtual Int_t     GetFontAscent() const;
   virtual Int_t     GetFontDescent() const ;
   virtual Float_t   GetTextMagnitude();

private:

   //Unfortunately, I have to convert from
   //top-left to bottom-left corner system.
   std::vector<TPoint> fConvertedPoints;
   
   unsigned fPatternIndex;

   void *GetSelectedDrawableChecked(const char *calledFrom) const;

   TGQuartz(const TGQuartz &rhs);
   TGQuartz &operator = (const TGQuartz &rhs);
      
   ClassDef(TGQuartz, 0);//2D non-GUI graphics for Mac OSX.
};

#endif
