// @(#)root/matrix:$Name:  $:$Id: TMatrixFSym.h,v 1.2 2004/01/27 08:12:26 brun Exp $
// Authors: Fons Rademakers, Eddy Offermann   Nov 2003

/*************************************************************************
 * Copyright (C) 1995-2000, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#ifndef ROOT_TMatrixFSym
#define ROOT_TMatrixFSym

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// TMatrixFSym                                                          //
//                                                                      //
// Implementation of a symmetric matrix in the linear algebra package   //
//                                                                      //
// Note that in this implementation both matrix element m[i][j] and     //
// m[j][i] are updated and stored in memory . However, when making the  //                             
// object persistent only the upper right triangle is stored .          //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

#ifndef ROOT_TMatrixFBase
#include "TMatrixFBase.h"
#endif 

class TMatrixF;
class TVectorF;

class TMatrixDSym;
class TMatrixFSym : public TMatrixFBase {

protected:

  Float_t *fElements;  //![fNelems] elements themselves

  virtual void Allocate  (Int_t nrows,Int_t ncols,Int_t row_lwb = 0,Int_t col_lwb = 0,Int_t init = 0);

  // Elementary constructors
  void AtMultA(const TMatrixF    &a,Int_t constr=1);
  void AtMultA(const TMatrixFSym &a,Int_t constr=1);

  void AMultA (const TMatrixFSym &a,Int_t constr=1) { AtMultA(a,constr); }

public:

  TMatrixFSym() { fIsOwner = kTRUE; fElements = 0; Invalidate(); }
  explicit TMatrixFSym(Int_t nrows);
  TMatrixFSym(Int_t row_lwb,Int_t row_upb);
  TMatrixFSym(Int_t nrows,const Float_t *data,Option_t *option="");
  TMatrixFSym(Int_t row_lwb,Int_t row_upb,const Float_t *data,Option_t *option="");
  TMatrixFSym(const TMatrixFSym &another);

  TMatrixFSym(EMatrixCreatorsOp1 op,const TMatrixFSym &prototype);
  TMatrixFSym(EMatrixCreatorsOp1 op,const TMatrixF    &prototype);
  TMatrixFSym(const TMatrixFSymLazy &lazy_constructor);

  virtual ~TMatrixFSym() { Clear(); Invalidate(); }

  virtual const Float_t *GetMatrixArray  () const;
  virtual       Float_t *GetMatrixArray  ();

  virtual void Clear(Option_t * /*option*/ ="") { if (fIsOwner) Delete_m(fNelems,fElements); }

  void        Adopt         (Int_t nrows,Float_t *data);
  void        Adopt         (Int_t row_lwb,Int_t row_upb,Float_t *data);
  TMatrixFSym GetSub        (Int_t row_lwb,Int_t row_upb,Option_t *option="S") const;
  void        SetSub        (Int_t row_lwb,const TMatrixFSym &source);

  virtual  Double_t Determinant() const;
  virtual  void     Determinant(Double_t &d1,Double_t &d2) const;

  TMatrixFSym  &Zero        ();
  TMatrixFSym  &Abs         ();
  TMatrixFSym  &Sqr         ();
  TMatrixFSym  &Sqrt        ();
  TMatrixFSym  &UnitMatrix  ();
  TMatrixFSym  &Transpose   (const TMatrixFSym &source);
  TMatrixFSym  &NormByDiag  (const TVectorF &v,Option_t *option="D");

  // Either access a_ij as a(i,j)
  inline const Float_t &operator()(Int_t rown,Int_t coln) const;
  inline       Float_t &operator()(Int_t rown,Int_t coln)
                                   { return (Float_t&)((*(const TMatrixFSym *)this)(rown,coln)); }

  TMatrixFSym &operator= (const TMatrixFSym     &source);
  TMatrixFSym &operator= (const TMatrixFSymLazy &source);
  TMatrixFSym &operator= (Float_t val);
  TMatrixFSym &operator-=(Float_t val);
  TMatrixFSym &operator+=(Float_t val);
  TMatrixFSym &operator*=(Float_t val);

  TMatrixFSym &operator+=(const TMatrixFSym &source);
  TMatrixFSym &operator-=(const TMatrixFSym &source);

  TMatrixFSym &Apply(const TElementActionF    &action);
  TMatrixFSym &Apply(const TElementPosActionF &action);

  friend Bool_t       operator== (const TMatrixFSym &m1,     const TMatrixFSym &m2);
  friend TMatrixFSym  operator+  (const TMatrixFSym &source1,const TMatrixFSym &source2);
  friend TMatrixFSym  operator-  (const TMatrixFSym &source1,const TMatrixFSym &source2);
  friend TMatrixFSym  operator*  (      Float_t      val,    const TMatrixFSym &source );
  friend TMatrixFSym  operator*  (const TMatrixFSym &source,       Float_t      val    )
                                 { return operator*(val,source); }

  friend TMatrixFSym &Add        (TMatrixFSym &target,      Float_t      scalar,const TMatrixFSym &source);
  friend TMatrixFSym &ElementMult(TMatrixFSym &target,const TMatrixFSym &source);
  friend TMatrixFSym &ElementDiv (TMatrixFSym &target,const TMatrixFSym &source);

  ClassDef(TMatrixFSym,1) // Symmetric Matrix class (single precision)
};

inline const Float_t  *TMatrixFSym::GetMatrixArray  () const { return fElements; }
inline       Float_t  *TMatrixFSym::GetMatrixArray  ()       { return fElements; }
inline const Float_t  &TMatrixFSym::operator()(Int_t rown,Int_t coln) const {
  Assert(IsValid());
  const Int_t arown = rown-fRowLwb;
  const Int_t acoln = coln-fColLwb;
  if (!(arown < fNrows && arown >= 0)) {
    printf("fRowLwb = %d\n",fRowLwb);
    printf("fNrows  = %d\n",fNrows);
    printf("arown   = %d\n",arown);
    printf("acoln   = %d\n",acoln);
  }
  Assert(arown < fNrows && arown >= 0);
  Assert(acoln < fNcols && acoln >= 0);
  return (fElements[arown*fNcols+acoln]);
}

Bool_t       operator== (const TMatrixFSym &m1,     const TMatrixFSym  &m2);
TMatrixFSym  operator+  (const TMatrixFSym &source1,const TMatrixFSym  &source2);
TMatrixFSym  operator-  (const TMatrixFSym &source1,const TMatrixFSym  &source2);
TMatrixFSym  operator*  (      Float_t      val,    const TMatrixFSym  &source );
TMatrixFSym  operator*  (const TMatrixFSym &source,       Float_t       val    );

TMatrixFSym &Add        (TMatrixFSym &target,      Float_t      scalar,const TMatrixFSym &source);
TMatrixFSym &ElementMult(TMatrixFSym &target,const TMatrixFSym &source);
TMatrixFSym &ElementDiv (TMatrixFSym &target,const TMatrixFSym &source);

#endif
