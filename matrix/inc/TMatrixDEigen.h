// @(#)root/matrix:$Name:  $:$Id: TMatrixDEigen.h,v 1.25 2003/09/05 09:21:54 brun Exp $
// Authors: Fons Rademakers, Eddy Offermann   Dec 2003

/*************************************************************************
 * Copyright (C) 1995-2000, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#ifndef ROOT_TMatrixDEigen
#define ROOT_TMatrixDEigen

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// TMatrixDEigen                                                        //
//                                                                      //
// Eigenvalues and eigenvectors of a real symmetric matrix.             //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

#ifndef ROOT_TMatrixD
#include "TMatrixD.h"
#endif

class TMatrixDEigen
{
protected :

  static void MakeHessenBerg  (TMatrixD &v,TVectorD &ortho,TMatrixD &H);
  static void MakeSchurr      (TMatrixD &v,TVectorD &d,    TVectorD &e,TMatrixD &H);

  TMatrixD fEigenVectors;   // Eigen-vectors of matrix
  TVectorD     fEigenValuesRe;  // Eigen-values
  TVectorD     fEigenValuesIm;  // Eigen-values

public :

  TMatrixDEigen() {};
  TMatrixDEigen(const TMatrixD &a);
  TMatrixDEigen(const TMatrixDEigen &another);
  virtual ~TMatrixDEigen() {}

  const TMatrixD &GetEigenVectors () const { return fEigenVectors;  }
  const TVectorD &GetEigenValuesRe() const { return fEigenValuesRe; }
  const TVectorD &GetEigenValuesIm() const { return fEigenValuesIm; }
  const TMatrixD  GetEigenValues  () const;

  TMatrixDEigen &operator= (const TMatrixDEigen &source);

  ClassDef(TMatrixDEigen,1) // Eigen-Vectors/Values of a Matrix
};
#endif
