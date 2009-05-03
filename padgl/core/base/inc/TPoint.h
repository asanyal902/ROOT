/* @(#)root/base:$Id$ */

/*************************************************************************
 * Copyright (C) 1995-2000, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#ifndef ROOT_TPoint
#define ROOT_TPoint


//////////////////////////////////////////////////////////////////////////
//                                                                      //
// TPoint                                                               //
//                                                                      //
// TPoint implements a 2D screen (device) point (see also TPoints).     //
//                                                                      //
// Don't add in dictionary since that will add a virtual table pointer  //
// and that will destroy the data layout of an array of TPoint's which  //
// should match the layout of an array of XPoint's (so no extra copying //
// needs to be done in the X11 drawing routines).                       //
// TPointFD is almost the same, but for Double_t and Float_t.           //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

#ifndef ROOT_Rtypes
#include "Rtypes.h"
#endif


class TPoint {

public:    // for easy access
#if !defined(WIN32) && !defined(G__WIN32)
   SCoord_t    fX;         //X device coordinate
   SCoord_t    fY;         //Y device coordinate
   
   typedef SCoord_t Value_t;
#else
   long        fX;         //X device coordinate
   long        fY;         //Y device coordinate
   
   typedef long Value_t;
#endif

public:
   TPoint() : fX(0), fY(0) { }
   TPoint(SCoord_t xy) : fX(xy), fY(xy) { }
   TPoint(SCoord_t x, SCoord_t y) : fX(x), fY(y) { }
   ~TPoint() { }
   SCoord_t    GetX() const { return (SCoord_t)fX; }
   SCoord_t    GetY() const { return (SCoord_t)fY; }
   void        SetX(SCoord_t x) { fX = x; }
   void        SetY(SCoord_t y) { fY = y; }

   TPoint& operator=(const TPoint& p) { fX = p.fX; fY = p.fY; return *this; }
   friend bool operator==(const TPoint& p1, const TPoint& p2);
   friend bool operator!=(const TPoint& p1, const TPoint& p2);
};

inline bool operator==(const TPoint& p1, const TPoint& p2)
{
   return p1.fX == p2.fX && p1.fY == p2.fY;
}

inline bool operator!=(const TPoint& p1, const TPoint& p2)
{
   return p1.fX != p2.fX || p1.fY != p2.fY;
}

template<class ValueType>
class TPointFD {//F float D double
public:
   typedef ValueType Value_t;
   
   Value_t fX;
   Value_t fY;
   
   TPointFD() : fX(0), fY(0) {}
   explicit TPointFD(Value_t xy) : fX(xy), fY(xy) {}
   TPointFD(Value_t x, Value_t y) : fX(x), fY(y) {}
   //Do not need explicit dtor, copy ctor or copy assignment operator.

private:
   /*
   Declared as private and not implemented, doubles and floats
   cannot be correctly compared with ==, !=. Eps of comparision: abs(a - b) < eps 
   can be different.
   */
   bool operator == (const TPointFD &rhs)const;
   bool operator != (const TPointFD &rhs)const;
};

typedef TPointFD<Float_t>  TPointF;
typedef TPointFD<Double_t> TPointD;

#endif
