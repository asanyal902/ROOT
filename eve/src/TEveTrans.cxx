// @(#)root/eve:$Id$
// Authors: Matevz Tadel & Alja Mrak-Tadel: 2006, 2007

/*************************************************************************
 * Copyright (C) 1995-2007, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

//______________________________________________________________________________
// TEveTrans
//
// TEveTrans is a 4x4 transformation matrix for homogeneous coordinates
// stored internaly in a column-major order to allow direct usage by
// GL. The element type is Double32_t as statically the floats would
// be precise enough but continuous operations on the matrix must
// retain precision of column vectors.
//
// Cartan angles are stored in fA[1-3] (+z, -y, +x). They are
// recalculated on demand.
//
// Direct  element access (first two should be used with care):
// operator[i]    direct access to elements,   i:0->15
// CM(i,j)        element 4*j + i;           i,j:0->3    { CM ~ c-matrix }
// operator(i,j)  element 4*(j-1) + i - 1    i,j:1->4
//
// Column-vector access:
// USet Get/SetBaseVec(), Get/SetPos() and Arr[XYZT]() methods.
//
// For all methods taking the matrix indices:
// 1->X, 2->Y, 3->Z; 4->Position (if applicable). 0 reserved for time.
//
// Shorthands in method-names:
// LF ~ LocalFrame; PF ~ ParentFrame; IP ~ InPlace

#include "TEveTrans.h"
#include "TEveUtil.h"
#include "TMath.h"
#include "TClass.h"

#include <ctype.h>
#include <iostream>

#define F00  0
#define F01  4
#define F02  8
#define F03 12

#define F10  1
#define F11  5
#define F12  9
#define F13 13

#define F20  2
#define F21  6
#define F22 10
#define F23 14

#define F30  3
#define F31  7
#define F32 11
#define F33 15

ClassImp(TEveTrans)

//______________________________________________________________________________
TEveTrans::TEveTrans() :
   TObject(),
   fA1(0), fA2(0), fA3(0), fAsOK(kFALSE),
   fUseTrans (kTRUE),
   fEditTrans(kFALSE),
   fEditRotation(kTRUE),
   fEditScale(kTRUE)
{
   // Default constructor.

   UnitTrans();
}

//______________________________________________________________________________
TEveTrans::TEveTrans(const TEveTrans& t) :
   TObject(),
   fA1(t.fA1), fA2(t.fA2), fA3(t.fA3), fAsOK(t.fAsOK),
   fUseTrans (t.fUseTrans),
   fEditTrans(t.fEditTrans)
{
   // Constructor.

   SetTrans(t, kFALSE);
}

//______________________________________________________________________________
TEveTrans::TEveTrans(const Double_t arr[16]) :
   TObject(),
   fA1(0), fA2(0), fA3(0), fAsOK(kFALSE),
   fUseTrans (kTRUE),
   fEditTrans(kFALSE)
{
   // Constructor.

   SetFromArray(arr);
}

//______________________________________________________________________________
TEveTrans::TEveTrans(const Float_t arr[16]) :
   TObject(),
   fA1(0), fA2(0), fA3(0), fAsOK(kFALSE),
   fUseTrans (kTRUE),
   fEditTrans(kFALSE)
{
   // Constructor.

   SetFromArray(arr);
}

/******************************************************************************/

//______________________________________________________________________________
void TEveTrans::UnitTrans()
{
   // Reset matrix to unity.

   memset(fM, 0, 16*sizeof(Double_t));
   fM[F00] = fM[F11] = fM[F22] = fM[F33] = 1;
   fA1 = fA2 = fA3 = 0;
   fAsOK = kTRUE;
}

//______________________________________________________________________________
void TEveTrans::UnitRot()
{
   // Reset rotation part of the matrix to unity.

   memset(fM, 0, 12*sizeof(Double_t));
   fM[F00] = fM[F11] = fM[F22] = 1;
   fA1 = fA2 = fA3 = 0;
   fAsOK = kTRUE;
}

//______________________________________________________________________________
void TEveTrans::SetTrans(const TEveTrans& t, Bool_t copyAngles)
{
   // Set matrix from another,

   memcpy(fM, t.fM, sizeof(fM));
   if (copyAngles && t.fAsOK) {
      fAsOK = kTRUE;
      fA1 = t.fA1; fA2 = t.fA2; fA3 = t.fA3;
   } else {
      fAsOK = kFALSE;
   }
}

//______________________________________________________________________________
void TEveTrans::SetFromArray(const Double_t arr[16])
{
   // Set matrix from Double_t array.

   for(Int_t i=0; i<16; ++i) fM[i] = arr[i];
   fAsOK = kFALSE;
}

//______________________________________________________________________________
void TEveTrans::SetFromArray(const Float_t arr[16])
{
   // Set matrix from Float_t array.

   for(Int_t i=0; i<16; ++i) fM[i] = arr[i];
   fAsOK = kFALSE;
}

//______________________________________________________________________________
void TEveTrans::SetupRotation(Int_t i, Int_t j, Double_t f)
{
   // Setup the matrix as an elementary rotation.
   // Optimized versions of left/right multiplication with an elementary
   // rotation matrix are implemented in RotatePF/RotateLF.
   // Expects identity matrix.

   if(i == j) return;
   TEveTrans& t = *this;
   t(i,i) = t(j,j) = TMath::Cos(f);
   Double_t s = TMath::Sin(f);
   t(i,j) = -s; t(j,i) = s;
   fAsOK = kFALSE;
}

/******************************************************************************/

// OrtoNorm3 and Invert are near the bottom.

/******************************************************************************/

//______________________________________________________________________________
void TEveTrans::MultLeft(const TEveTrans& t)
{
   // Multiply from left: this = t * this.

   Double_t  buf[4];
   Double_t* col = fM;
   for(int c=0; c<4; ++c, col+=4) {
      const Double_t* row = t.fM;
      for(int r=0; r<4; ++r, ++row)
         buf[r] = row[0]*col[0] + row[4]*col[1] + row[8]*col[2] + row[12]*col[3];
      col[0] = buf[0]; col[1] = buf[1]; col[2] = buf[2]; col[3] = buf[3];
   }
   fAsOK = kFALSE;
}

//______________________________________________________________________________
void TEveTrans::MultRight(const TEveTrans& t)
{
   // Multiply from right: this = this * t.

   Double_t  buf[4];
   Double_t* row = fM;
   for(int r=0; r<4; ++r, ++row) {
      const Double_t* col = t.fM;
      for(int c=0; c<4; ++c, col+=4)
         buf[c] = row[0]*col[0] + row[4]*col[1] + row[8]*col[2] + row[12]*col[3];
      row[0] = buf[0]; row[4] = buf[1]; row[8] = buf[2]; row[12] = buf[3];
   }
   fAsOK = kFALSE;
}

//______________________________________________________________________________
TEveTrans TEveTrans::operator*(const TEveTrans& t)
{
   // Copy, multiply from right and return product.
   // Avoid unless necessary.

   TEveTrans b(*this);
   b.MultRight(t);
   return b;
}

/******************************************************************************/

//______________________________________________________________________________
void TEveTrans::TransposeRotationPart()
{
   // Transpose 3x3 rotation sub-matrix.

   Double_t x;
   x = fM[F01]; fM[F01] = fM[F10]; fM[F10] = x;
   x = fM[F02]; fM[F02] = fM[F20]; fM[F20] = x;
   x = fM[F12]; fM[F12] = fM[F21]; fM[F21] = x;
   fAsOK = kFALSE;
}

/******************************************************************************/
// Move & Rotate
/******************************************************************************/

//______________________________________________________________________________
void TEveTrans::MoveLF(Int_t ai, Double_t amount)
{
   // Move in local-frame along axis with index ai.

   const Double_t *col = fM + 4*--ai;
   fM[F03] += amount*col[0]; fM[F13] += amount*col[1]; fM[F23] += amount*col[2];
}

//______________________________________________________________________________
void TEveTrans::Move3LF(Double_t x, Double_t y, Double_t z)
{
   // General move in local-frame.

   fM[F03] += x*fM[0] + y*fM[4] + z*fM[8];
   fM[F13] += x*fM[1] + y*fM[5] + z*fM[9];
   fM[F23] += x*fM[2] + y*fM[6] + z*fM[10];
}

//______________________________________________________________________________
void TEveTrans::RotateLF(Int_t i1, Int_t i2, Double_t amount)
{
   // Rotate in local frame. Does optimised version of MultRight.

   if(i1 == i2) return;
   // Algorithm: TEveTrans a; a.SetupRotation(i1, i2, amount); MultRight(a);
   // Optimized version:
   const Double_t cos = TMath::Cos(amount), sin = TMath::Sin(amount);
   Double_t  b1, b2;
   Double_t* row = fM;
   --i1 <<= 2; --i2 <<= 2; // column major
   for (int r=0; r<4; ++r, ++row) {
      b1 = cos*row[i1] + sin*row[i2];
      b2 = cos*row[i2] - sin*row[i1];
      row[i1] = b1; row[i2] = b2;
   }
   fAsOK = kFALSE;
}

/******************************************************************************/

//______________________________________________________________________________
void TEveTrans::MovePF(Int_t ai, Double_t amount)
{
   // Move in parent-frame along axis index ai.

   fM[F03 + --ai] += amount;
}

//______________________________________________________________________________
void TEveTrans::Move3PF(Double_t x, Double_t y, Double_t z)
{
   // General move in parent-frame.

   fM[F03] += x;
   fM[F13] += y;
   fM[F23] += z;
}

//______________________________________________________________________________
void TEveTrans::RotatePF(Int_t i1, Int_t i2, Double_t amount)
{
   // Rotate in parent frame. Does optimised version of MultLeft.

   if(i1 == i2) return;
   // Algorithm: TEveTrans a; a.SetupRotation(i1, i2, amount); MultLeft(a);

   // Optimized version:
   const Double_t cos = TMath::Cos(amount), sin = TMath::Sin(amount);
   Double_t  b1, b2;
   Double_t* col = fM;
   --i1; --i2;
   for(int c=0; c<4; ++c, col+=4) {
      b1 = cos*col[i1] - sin*col[i2];
      b2 = cos*col[i2] + sin*col[i1];
      col[i1] = b1; col[i2] = b2;
   }
   fAsOK = kFALSE;
}

/******************************************************************************/

//______________________________________________________________________________
void TEveTrans::Move(const TEveTrans& a, Int_t ai, Double_t amount)
{
   // Move in a's coord-system along axis-index ai.

   const Double_t* vec = a.fM + 4*--ai;
   fM[F03] += amount*vec[0];
   fM[F13] += amount*vec[1];
   fM[F23] += amount*vec[2];
}

//______________________________________________________________________________
void TEveTrans::Move3(const TEveTrans& a, Double_t x, Double_t y, Double_t z)
{
   // General move in a's coord-system.

   const Double_t* m = a.fM;
   fM[F03] += x*m[F00] + y*m[F01] + z*m[F02];
   fM[F13] += x*m[F10] + y*m[F11] + z*m[F12];
   fM[F23] += x*m[F20] + y*m[F21] + z*m[F22];
}

//______________________________________________________________________________
void TEveTrans::Rotate(const TEveTrans& a, Int_t i1, Int_t i2, Double_t amount)
{
   // Rotate in a's coord-system, rotating base vector with index i1
   // into i2.

   if(i1 == i2) return;
   TEveTrans x(a);
   x.Invert();
   MultLeft(x);
   RotatePF(i1, i2, amount);
   MultLeft(a);
   fAsOK = kFALSE;
}

/******************************************************************************/
// Base-vector interface
/******************************************************************************/

//______________________________________________________________________________
void TEveTrans::SetBaseVec(Int_t b, Double_t x, Double_t y, Double_t z)
{
   // Set base-vector with index b.

   Double_t* col = fM + 4*--b;
   col[0] = x; col[1] = y; col[2] = z;
   fAsOK = kFALSE;
}

//______________________________________________________________________________
void TEveTrans::SetBaseVec(Int_t b, const TVector3& v)
{
   // Set base-vector with index b.

   Double_t* col = fM + 4*--b;
   v.GetXYZ(col);
   fAsOK = kFALSE;
}

//______________________________________________________________________________
TVector3 TEveTrans::GetBaseVec(Int_t b) const
{
   // Get base-vector with index b.

   return TVector3(&fM[4*--b]);
}

void TEveTrans::GetBaseVec(Int_t b, TVector3& v) const
{
   // Get base-vector with index b.

   const Double_t* col = fM + 4*--b;
   v.SetXYZ(col[0], col[1], col[2]);
}

/******************************************************************************/
// Position interface
/******************************************************************************/

//______________________________________________________________________________
void TEveTrans::SetPos(Double_t x, Double_t y, Double_t z)
{
   // Set position (base-vec 4).
   fM[F03] = x; fM[F13] = y; fM[F23] = z;
}

void TEveTrans::SetPos(Double_t* x)
{
   // Set position (base-vec 4).
   fM[F03] = x[0]; fM[F13] = x[1]; fM[F23] = x[2];
}

void TEveTrans::SetPos(Float_t* x)
{
   // Set position (base-vec 4).
   fM[F03] = x[0]; fM[F13] = x[1]; fM[F23] = x[2];
}

void TEveTrans::SetPos(const TEveTrans& t)
{
   // Set position (base-vec 4).
   const Double_t* m = t.fM;
   fM[F03] = m[F03]; fM[F13] = m[F13]; fM[F23] = m[F23];
}

//______________________________________________________________________________
void TEveTrans::GetPos(Double_t& x, Double_t& y, Double_t& z) const
{
   // Get position (base-vec 4).
   x = fM[F03]; y = fM[F13]; z = fM[F23];
}

void TEveTrans::GetPos(Double_t* x) const
{
   // Get position (base-vec 4).
   x[0] = fM[F03]; x[1] = fM[F13]; x[2] = fM[F23];
}

void TEveTrans::GetPos(Float_t* x) const
{
   // Get position (base-vec 4).
   x[0] = fM[F03]; x[1] = fM[F13]; x[2] = fM[F23];
}

void TEveTrans::GetPos(TVector3& v) const
{
   // Get position (base-vec 4).
   v.SetXYZ(fM[F03], fM[F13], fM[F23]);
}

TVector3 TEveTrans::GetPos() const
{
   // Get position (base-vec 4).
   return TVector3(fM[F03], fM[F13], fM[F23]);
}

/******************************************************************************/
// Cardan angle interface
/******************************************************************************/

namespace
{
inline void clamp_angle(Float_t& a)
{
   while(a < -TMath::TwoPi()) a += TMath::TwoPi();
   while(a >  TMath::TwoPi()) a -= TMath::TwoPi();
}
}

void TEveTrans::SetRotByAngles(Float_t a1, Float_t a2, Float_t a3)
{
   // Sets Rotation part as given by angles:
   // a1 around z, -a2 around y, a3 around x.

   clamp_angle(a1); clamp_angle(a2); clamp_angle(a3);

   Double_t a, b, c, d, e, f;
   a = TMath::Cos(a3); b = TMath::Sin(a3);
   c = TMath::Cos(a2); d = TMath::Sin(a2); // should be -sin(a2) for positive direction
   e = TMath::Cos(a1); f = TMath::Sin(a1);
   Double_t ad = a*d, bd = b*d;

   fM[F00] = c*e; fM[F01] = -bd*e - a*f; fM[F02] = -ad*e + b*f;
   fM[F10] = c*f; fM[F11] = -bd*f + a*e; fM[F12] = -ad*f - b*e;
   fM[F20] = d;   fM[F21] =  b*c;        fM[F22] =  a*c;

   fA1 = a1; fA2 = a2; fA3 = a3;
   fAsOK = kTRUE;
}

//______________________________________________________________________________
void TEveTrans::SetRotByAnyAngles(Float_t a1, Float_t a2, Float_t a3,
                                  const Text_t* pat)
{
   // Sets Rotation part as given by angles a1, a1, a3 and pattern pat.
   // Pattern consists of "XxYyZz" characters.
   // eg: x means rotate about x axis, X means rotate in negative direction
   // xYz -> R_x(a3) * R_y(-a2) * R_z(a1); (standard Gled representation)
   // Note that angles and pattern elements have inversed order!
   //
   // Implements Eulerian/Cardanian angles in a uniform way.

   int n = strspn(pat, "XxYyZz"); if(n > 3) n = 3;
   // Build Trans ... assign ...
   Float_t a[] = { a3, a2, a1 };
   UnitRot();
   for(int i=0; i<n; i++) {
      if(isupper(pat[i])) a[i] = -a[i];
      switch(pat[i]) {
         case 'x': case 'X': RotateLF(2, 3, a[i]); break;
         case 'y': case 'Y': RotateLF(3, 1, a[i]); break;
         case 'z': case 'Z': RotateLF(1, 2, a[i]); break;
      }
   }
   fAsOK = kFALSE;
}

//______________________________________________________________________________
void TEveTrans::GetRotAngles(Float_t* x) const
{
   // Get Cardan rotation angles (pattern xYz above).

   if(!fAsOK) {
      Double_t sx, sy, sz;
      GetScale(sx, sy, sz);
      Double_t d = fM[F20]/sx;
      if(d>1) d=1; else if(d<-1) d=-1; // Fix numerical errors
      fA2 = TMath::ASin(d);
      Double_t cos = TMath::Cos(fA2);
      if(TMath::Abs(cos) > 8.7e-6) {
         fA1 = TMath::ATan2(fM[F10], fM[F00]);
         fA3 = TMath::ATan2(fM[F21]/sy, fM[F22]/sz);
      } else {
         fA1 = TMath::ATan2(fM[F10]/sx, fM[F11]/sy);
         fA3 = 0;
      }
      fAsOK = kTRUE;
   }
   x[0] = fA1; x[1] = fA2; x[2] = fA3;
}

/******************************************************************************/
// Scaling
/******************************************************************************/

//______________________________________________________________________________
void TEveTrans::Scale(Double_t sx, Double_t sy, Double_t sz)
{
   // Scale matrix. Translation part untouched.

   fM[F00] *= sx; fM[F10] *= sx; fM[F20] *= sx;
   fM[F01] *= sy; fM[F11] *= sy; fM[F21] *= sy;
   fM[F02] *= sz; fM[F12] *= sz; fM[F22] *= sz;
}

//______________________________________________________________________________
Double_t TEveTrans::Unscale()
{
   // Remove scaling, make all base vectors of unit length.

   Double_t sx, sy, sz;
   Unscale(sx, sy, sz);
   return (sx + sy + sz)/3;
}

//______________________________________________________________________________
void TEveTrans::Unscale(Double_t& sx, Double_t& sy, Double_t& sz)
{
   // Remove scaling, make all base vectors of unit length.

   GetScale(sx, sy, sz);
   fM[F00] /= sx; fM[F10] /= sx; fM[F20] /= sx;
   fM[F01] /= sy; fM[F11] /= sy; fM[F21] /= sy;
   fM[F02] /= sz; fM[F12] /= sz; fM[F22] /= sz;
}

//______________________________________________________________________________
void TEveTrans::GetScale(Double_t& sx, Double_t& sy, Double_t& sz) const
{
   // Deduce scales from sizes of base vectors.

   sx = TMath::Sqrt( fM[F00]*fM[F00] + fM[F10]*fM[F10] + fM[F20]*fM[F20] );
   sy = TMath::Sqrt( fM[F01]*fM[F01] + fM[F11]*fM[F11] + fM[F21]*fM[F21] );
   sz = TMath::Sqrt( fM[F02]*fM[F02] + fM[F12]*fM[F12] + fM[F22]*fM[F22] );
}

//______________________________________________________________________________
void TEveTrans::SetScale(Double_t sx, Double_t sy, Double_t sz)
{
   // Set scaling.

   sx /= TMath::Sqrt( fM[F00]*fM[F00] + fM[F10]*fM[F10] + fM[F20]*fM[F20] );
   sy /= TMath::Sqrt( fM[F01]*fM[F01] + fM[F11]*fM[F11] + fM[F21]*fM[F21] );
   sz /= TMath::Sqrt( fM[F02]*fM[F02] + fM[F12]*fM[F12] + fM[F22]*fM[F22] );

   fM[F00] *= sx; fM[F10] *= sx; fM[F20] *= sx;
   fM[F01] *= sy; fM[F11] *= sy; fM[F21] *= sy;
   fM[F02] *= sz; fM[F12] *= sz; fM[F22] *= sz;
}

//______________________________________________________________________________
void TEveTrans::SetScaleX(Double_t sx)
{
   // Change x scaling.

   sx /= TMath::Sqrt( fM[F00]*fM[F00] + fM[F10]*fM[F10] + fM[F20]*fM[F20] );
   fM[F00] *= sx; fM[F10] *= sx; fM[F20] *= sx;
}

//______________________________________________________________________________
void TEveTrans::SetScaleY(Double_t sy)
{
   // Change y scaling.

   sy /= TMath::Sqrt( fM[F01]*fM[F01] + fM[F11]*fM[F11] + fM[F21]*fM[F21] );
   fM[F01] *= sy; fM[F11] *= sy; fM[F21] *= sy;
}

//______________________________________________________________________________
void TEveTrans::SetScaleZ(Double_t sz)
{
   // Change z scaling.

   sz /= TMath::Sqrt( fM[F02]*fM[F02] + fM[F12]*fM[F12] + fM[F22]*fM[F22] );
   fM[F02] *= sz; fM[F12] *= sz; fM[F22] *= sz;
}


/******************************************************************************/
// Operations on vectors
/******************************************************************************/

//______________________________________________________________________________
void TEveTrans::MultiplyIP(TVector3& v, Double_t w) const
{
   // Multiply vector in-place.

   v.SetXYZ(fM[F00]*v.x() + fM[F01]*v.y() + fM[F02]*v.z() + fM[F03]*w,
            fM[F10]*v.x() + fM[F11]*v.y() + fM[F12]*v.z() + fM[F13]*w,
            fM[F20]*v.x() + fM[F21]*v.y() + fM[F22]*v.z() + fM[F23]*w);
}

//______________________________________________________________________________
void TEveTrans::MultiplyIP(Double_t* v, Double_t w) const
{
   // Multiply vector in-place.

   Double_t r[3] = { v[0], v[1], v[2] };
   v[0] = fM[F00]*r[0] + fM[F01]*r[1] + fM[F02]*r[2] + fM[F03]*w;
   v[1] = fM[F10]*r[0] + fM[F11]*r[1] + fM[F12]*r[2] + fM[F13]*w;
   v[2] = fM[F20]*r[0] + fM[F21]*r[1] + fM[F22]*r[2] + fM[F23]*w;
}

//______________________________________________________________________________
void TEveTrans::MultiplyIP(Float_t* v, Double_t w) const
{
   // Multiply vector in-place.

   Double_t r[3] = { v[0], v[1], v[2] };
   v[0] = fM[F00]*r[0] + fM[F01]*r[1] + fM[F02]*r[2] + fM[F03]*w;
   v[1] = fM[F10]*r[0] + fM[F11]*r[1] + fM[F12]*r[2] + fM[F13]*w;
   v[2] = fM[F20]*r[0] + fM[F21]*r[1] + fM[F22]*r[2] + fM[F23]*w;
}

//______________________________________________________________________________
TVector3 TEveTrans::Multiply(const TVector3& v, Double_t w) const
{
   // Multiply vector and return it.

   return TVector3(fM[F00]*v.x() + fM[F01]*v.y() + fM[F02]*v.z() + fM[F03]*w,
                   fM[F10]*v.x() + fM[F11]*v.y() + fM[F12]*v.z() + fM[F13]*w,
                   fM[F20]*v.x() + fM[F21]*v.y() + fM[F22]*v.z() + fM[F23]*w);
}

//______________________________________________________________________________
void TEveTrans::Multiply(const Double_t *vin, Double_t* vout, Double_t w) const
{
   // Multiply vector and fill output array vout.

   vout[0] = fM[F00]*vin[0] + fM[F01]*vin[1] + fM[F02]*vin[2] + fM[F03]*w;
   vout[1] = fM[F10]*vin[0] + fM[F11]*vin[1] + fM[F12]*vin[1] + fM[F13]*w;
   vout[2] = fM[F20]*vin[0] + fM[F21]*vin[1] + fM[F22]*vin[1] + fM[F23]*w;
}

//______________________________________________________________________________
void TEveTrans::RotateIP(TVector3& v) const
{
   // Rotate vector in-place. Translation is NOT applied.

   v.SetXYZ(fM[F00]*v.x() + fM[F01]*v.y() + fM[F02]*v.z(),
            fM[F10]*v.x() + fM[F11]*v.y() + fM[F12]*v.z(),
            fM[F20]*v.x() + fM[F21]*v.y() + fM[F22]*v.z());
}

//______________________________________________________________________________
TVector3 TEveTrans::Rotate(const TVector3& v) const
{
   // Rotate vector in-place. Translation is NOT applied.

   return TVector3(fM[F00]*v.x() + fM[F01]*v.y() + fM[F02]*v.z(),
                   fM[F10]*v.x() + fM[F11]*v.y() + fM[F12]*v.z(),
                   fM[F20]*v.x() + fM[F21]*v.y() + fM[F22]*v.z());
}

/******************************************************************************/
// Normalization, ortogonalization
/******************************************************************************/

//______________________________________________________________________________
Double_t TEveTrans::Norm3Column(Int_t col)
{
   // Norm 3-vector in column col.

   Double_t* c = fM + 4*--col;
   const Double_t  l = TMath::Sqrt(c[0]*c[0] + c[1]*c[1] + c[2]*c[2]);
   c[0] /= l; c[1] /= l; c[2] /= l;
   return l;
}

//______________________________________________________________________________
Double_t TEveTrans::Orto3Column(Int_t col, Int_t ref)
{
   // Orto-norm 3-vector in column col with respect to column ref.

   Double_t* c =  fM + 4*--col;
   Double_t* rc = fM + 4*--ref;
   const Double_t dp = c[0]*rc[0] + c[1]*rc[1] + c[2]*rc[2];
   c[0] -= rc[0]*dp; c[1] -= rc[1]*dp; c[2] -= rc[2]*dp;
   return dp;
}

//______________________________________________________________________________
void TEveTrans::OrtoNorm3()
{
   // Orto-norm columns 1 to 3.

   Norm3Column(1);
   Orto3Column(2,1); Norm3Column(2);
   fM[F02] = fM[F10]*fM[F21] - fM[F11]*fM[F20];
   fM[F12] = fM[F20]*fM[F01] - fM[F21]*fM[F00];
   fM[F22] = fM[F00]*fM[F11] - fM[F01]*fM[F10];
   // Cross-product faster than the following.
   // Orto3Column(3,1); Orto3Column(3,2); Norm3Column(3);
}

/******************************************************************************/
// Inversion
/******************************************************************************/

//______________________________________________________________________________
Double_t TEveTrans::Invert()
{
   // Invert matrix.
   // Copied from ROOT's TMatrixFCramerInv.

   static const TEveException eh("TEveTrans::Invert ");

   // Find all NECESSARY 2x2 dets:  (18 of them)
   const Double_t det2_12_01 = fM[F10]*fM[F21] - fM[F11]*fM[F20];
   const Double_t det2_12_02 = fM[F10]*fM[F22] - fM[F12]*fM[F20];
   const Double_t det2_12_03 = fM[F10]*fM[F23] - fM[F13]*fM[F20];
   const Double_t det2_12_13 = fM[F11]*fM[F23] - fM[F13]*fM[F21];
   const Double_t det2_12_23 = fM[F12]*fM[F23] - fM[F13]*fM[F22];
   const Double_t det2_12_12 = fM[F11]*fM[F22] - fM[F12]*fM[F21];
   const Double_t det2_13_01 = fM[F10]*fM[F31] - fM[F11]*fM[F30];
   const Double_t det2_13_02 = fM[F10]*fM[F32] - fM[F12]*fM[F30];
   const Double_t det2_13_03 = fM[F10]*fM[F33] - fM[F13]*fM[F30];
   const Double_t det2_13_12 = fM[F11]*fM[F32] - fM[F12]*fM[F31];
   const Double_t det2_13_13 = fM[F11]*fM[F33] - fM[F13]*fM[F31];
   const Double_t det2_13_23 = fM[F12]*fM[F33] - fM[F13]*fM[F32];
   const Double_t det2_23_01 = fM[F20]*fM[F31] - fM[F21]*fM[F30];
   const Double_t det2_23_02 = fM[F20]*fM[F32] - fM[F22]*fM[F30];
   const Double_t det2_23_03 = fM[F20]*fM[F33] - fM[F23]*fM[F30];
   const Double_t det2_23_12 = fM[F21]*fM[F32] - fM[F22]*fM[F31];
   const Double_t det2_23_13 = fM[F21]*fM[F33] - fM[F23]*fM[F31];
   const Double_t det2_23_23 = fM[F22]*fM[F33] - fM[F23]*fM[F32];

   // Find all NECESSARY 3x3 dets:   (16 of them)
   const Double_t det3_012_012 = fM[F00]*det2_12_12 - fM[F01]*det2_12_02 + fM[F02]*det2_12_01;
   const Double_t det3_012_013 = fM[F00]*det2_12_13 - fM[F01]*det2_12_03 + fM[F03]*det2_12_01;
   const Double_t det3_012_023 = fM[F00]*det2_12_23 - fM[F02]*det2_12_03 + fM[F03]*det2_12_02;
   const Double_t det3_012_123 = fM[F01]*det2_12_23 - fM[F02]*det2_12_13 + fM[F03]*det2_12_12;
   const Double_t det3_013_012 = fM[F00]*det2_13_12 - fM[F01]*det2_13_02 + fM[F02]*det2_13_01;
   const Double_t det3_013_013 = fM[F00]*det2_13_13 - fM[F01]*det2_13_03 + fM[F03]*det2_13_01;
   const Double_t det3_013_023 = fM[F00]*det2_13_23 - fM[F02]*det2_13_03 + fM[F03]*det2_13_02;
   const Double_t det3_013_123 = fM[F01]*det2_13_23 - fM[F02]*det2_13_13 + fM[F03]*det2_13_12;
   const Double_t det3_023_012 = fM[F00]*det2_23_12 - fM[F01]*det2_23_02 + fM[F02]*det2_23_01;
   const Double_t det3_023_013 = fM[F00]*det2_23_13 - fM[F01]*det2_23_03 + fM[F03]*det2_23_01;
   const Double_t det3_023_023 = fM[F00]*det2_23_23 - fM[F02]*det2_23_03 + fM[F03]*det2_23_02;
   const Double_t det3_023_123 = fM[F01]*det2_23_23 - fM[F02]*det2_23_13 + fM[F03]*det2_23_12;
   const Double_t det3_123_012 = fM[F10]*det2_23_12 - fM[F11]*det2_23_02 + fM[F12]*det2_23_01;
   const Double_t det3_123_013 = fM[F10]*det2_23_13 - fM[F11]*det2_23_03 + fM[F13]*det2_23_01;
   const Double_t det3_123_023 = fM[F10]*det2_23_23 - fM[F12]*det2_23_03 + fM[F13]*det2_23_02;
   const Double_t det3_123_123 = fM[F11]*det2_23_23 - fM[F12]*det2_23_13 + fM[F13]*det2_23_12;

   // Find the 4x4 det:
   const Double_t det = fM[F00]*det3_123_123 - fM[F01]*det3_123_023 +
      fM[F02]*det3_123_013 - fM[F03]*det3_123_012;

   if(det == 0) {
      throw(eh + "matrix is singular.");
   }

   const Double_t oneOverDet = 1.0/det;
   const Double_t mn1OverDet = - oneOverDet;

   fM[F00] = det3_123_123 * oneOverDet;
   fM[F01] = det3_023_123 * mn1OverDet;
   fM[F02] = det3_013_123 * oneOverDet;
   fM[F03] = det3_012_123 * mn1OverDet;

   fM[F10] = det3_123_023 * mn1OverDet;
   fM[F11] = det3_023_023 * oneOverDet;
   fM[F12] = det3_013_023 * mn1OverDet;
   fM[F13] = det3_012_023 * oneOverDet;

   fM[F20] = det3_123_013 * oneOverDet;
   fM[F21] = det3_023_013 * mn1OverDet;
   fM[F22] = det3_013_013 * oneOverDet;
   fM[F23] = det3_012_013 * mn1OverDet;

   fM[F30] = det3_123_012 * mn1OverDet;
   fM[F31] = det3_023_012 * oneOverDet;
   fM[F32] = det3_013_012 * mn1OverDet;
   fM[F33] = det3_012_012 * oneOverDet;

   fAsOK = kFALSE;
   return det;
}

/******************************************************************************/

//______________________________________________________________________________
void TEveTrans::Streamer(TBuffer &R__b)
{
   // Stream an object of class TEveTrans.

   if (R__b.IsReading()) {
      TEveTrans::Class()->ReadBuffer(R__b, this);
      fAsOK = kFALSE;
   } else {
      TEveTrans::Class()->WriteBuffer(R__b, this);
   }
}

/******************************************************************************/
/******************************************************************************/

//______________________________________________________________________________
void TEveTrans::Print(Option_t* /*option*/) const
{
   // Print in reasonable format.

   const Double_t* row = fM;
   for(Int_t i=0; i<4; ++i, ++row)
      printf("%8.3f %8.3f %8.3f | %8.3f\n", row[0], row[4], row[8], row[12]);
}

#include <iomanip>

//______________________________________________________________________________
ostream& operator<<(ostream& s, const TEveTrans& t)
{
   // Print to ostream.

   s.setf(std::ios::fixed, std::ios::floatfield);
   s.precision(3);
   for(Int_t i=1; i<=4; i++)
      for(Int_t j=1; j<=4; j++)
         s << t(i,j) << ((j==4) ? "\n" : "\t");
   return s;
}

/******************************************************************************/
// TEveUtil stuff
/******************************************************************************/

#include "TGeoMatrix.h"
#include "TBuffer3D.h"

void TEveTrans::SetFrom(Double_t* carr)
{
   // Initialize from array.

   fUseTrans = kTRUE;
   memcpy(fM, carr, 16*sizeof(Double_t));
   fAsOK = kFALSE;
}

//______________________________________________________________________________
void TEveTrans::SetFrom(const TGeoMatrix& mat)
{
   // Initialize from TGeoMatrix.

   fUseTrans = kTRUE;
   const Double_t *r = mat.GetRotationMatrix();
   const Double_t *t = mat.GetTranslation();
   const Double_t *s = mat.GetScale();
   Double_t       *m = fM;
   m[0] = r[0]*s[0]; m[1] = r[3]*s[0]; m[2] = r[6]*s[0]; m[3] = 0; m += 4;
   m[0] = r[1]*s[1]; m[1] = r[4]*s[1]; m[2] = r[7]*s[1]; m[3] = 0; m += 4;
   m[0] = r[2]*s[2]; m[1] = r[5]*s[2]; m[2] = r[8]*s[2]; m[3] = 0; m += 4;
   m[0] = t[0];      m[1] = t[1];      m[2] = t[2];      m[3] = 1;
   fAsOK = kFALSE;
}

//______________________________________________________________________________
void TEveTrans::SetGeoHMatrix(TGeoHMatrix& mat)
{
   // Set TGeoHMatrix mat.

   Double_t *r = mat.GetRotationMatrix();
   Double_t *t = mat.GetTranslation();
   Double_t *s = mat.GetScale();
   if (fUseTrans)
   {
      mat.SetBit(TGeoMatrix::kGeoGenTrans);
      Double_t *m = fM;
      GetScale(s[0], s[1], s[2]);
      r[0] = m[0]/s[0]; r[3] = m[1]/s[0]; r[6] = m[2]/s[0]; m += 4;
      r[1] = m[0]/s[1]; r[4] = m[1]/s[1]; r[7] = m[2]/s[1]; m += 4;
      r[2] = m[0]/s[2]; r[5] = m[1]/s[2]; r[8] = m[2]/s[2]; m += 4;
      t[0] = m[0];      t[1] = m[1];      t[2] = m[2];
   } else {
      mat.ResetBit(TGeoMatrix::kGeoGenTrans);
      r[0] = 1; r[3] = 0; r[6] = 0;
      r[1] = 0; r[4] = 1; r[7] = 0;
      r[2] = 0; r[5] = 0; r[8] = 1;
      s[0] = s[1] = s[2] = 1;
      t[0] = t[1] = t[2] = 0;
   }
}

//______________________________________________________________________________
void TEveTrans::SetBuffer3D(TBuffer3D& buff)
{
   // Fill transformation part TBuffer3D core section.

   buff.fLocalFrame = fUseTrans;
   if (fUseTrans) {
      // In phys-shape ctor the rotation part is transposed, due to
      // TGeo's convention for rotation matrix. So we have to transpose
      // it here, also.
      Double_t *m = buff.fLocalMaster;
      m[0]  = fM[0];  m[1]  = fM[4];  m[2]  = fM[8];  m[3]  = fM[3];
      m[4]  = fM[1];  m[5]  = fM[5];  m[6]  = fM[9];  m[7]  = fM[7];
      m[8]  = fM[2];  m[9]  = fM[6];  m[10] = fM[10]; m[11] = fM[11];
      m[12] = fM[12]; m[13] = fM[13]; m[14] = fM[14]; m[15] = fM[15];
      // Otherwise this would do:
      // memcpy(buff.fLocalMaster, fM, 16*sizeof(Double_t));
   }
}

//______________________________________________________________________________
Bool_t TEveTrans::IsScale(Double_t low, Double_t high) const
{
   // Test if the transformation is a scale.
   // To be used by ROOT TGLObject descendants that potentially need to
   // use GL_NORMALIZE.
   // The low/high limits are expected to be squares of acutal limits.
   //
   // Ideally this should be done by the TGLViewer [but is not].

   if (!fUseTrans) return kFALSE;
   Double_t s;
   s = fM[F00]*fM[F00] + fM[F10]*fM[F10] + fM[F20]*fM[F20];
   if (s < low || s > high) return kTRUE;
   s = fM[F01]*fM[F01] + fM[F11]*fM[F11] + fM[F21]*fM[F21];
   if (s < low || s > high) return kTRUE;
   s = fM[F02]*fM[F02] + fM[F12]*fM[F12] + fM[F22]*fM[F22];
   if (s < low || s > high) return kTRUE;
   return kFALSE;
}
