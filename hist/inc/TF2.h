// @(#)root/hist:$Name:  $:$Id: TF2.h,v 1.9 2001/10/27 10:38:50 brun Exp $
// Author: Rene Brun   23/08/95

/*************************************************************************
 * Copyright (C) 1995-2000, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/
// ---------------------------------- F2.h

#ifndef ROOT_TF2
#define ROOT_TF2



//////////////////////////////////////////////////////////////////////////
//                                                                      //
// TF2                                                                  //
//                                                                      //
// The Parametric 2-D function                                          //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

#ifndef ROOT_TF1
#include "TF1.h"
#endif
#ifndef ROOT_TArrayD
#include "TArrayD.h"
#endif

class TF2 : public TF1 {

protected:
   Double_t  fYmin;        //Lower bound for the range in y
   Double_t  fYmax;        //Upper bound for the range in y
   Int_t     fNpy;         //Number of points along y used for the graphical representation
   TArrayD   fContour;     //Array to display contour levels

public:
   TF2();
   TF2(const char *name, const char *formula, Double_t xmin=0, Double_t xmax=1, Double_t ymin=0, Double_t ymax=1);
   TF2(const char *name, void *fcn, Double_t xmin=0, Double_t xmax=1, Double_t ymin=0, Double_t ymax=1, Int_t npar=0);
   TF2(const char *name, Double_t (*fcn)(Double_t *, Double_t *), Double_t xmin=0, Double_t xmax=1, Double_t ymin=0, Double_t ymax=1, Int_t npar=0);
   TF2(const TF2 &f2);
   virtual   ~TF2();
   virtual void     Copy(TObject &f2);
   virtual Int_t    DistancetoPrimitive(Int_t px, Int_t py);
   virtual void     Draw(Option_t *option="");
   virtual TF1     *DrawCopy(Option_t *option="");
   virtual void     DrawDerivative(Option_t *option="al") {;}
   virtual void     DrawIntegral(Option_t *option="al") {;}
   virtual void     DrawF2(const char *formula, Double_t xmin, Double_t xmax, Double_t ymin, Double_t ymax, Option_t *option="");
   virtual void     ExecuteEvent(Int_t event, Int_t px, Int_t py);
   virtual Int_t    GetContour(Double_t *levels=0);
   virtual Double_t GetContourLevel(Int_t level) const;
          Int_t     GetNpy() const {return fNpy;}
   virtual char    *GetObjectInfo(Int_t px, Int_t py) const;
       Double_t     GetRandom();
   virtual void     GetRandom2(Double_t &xrandom, Double_t &yrandom);
   virtual void     GetRange(Double_t &xmin, Double_t &xmax) { TF1::GetRange(xmin, xmax); }
   virtual void     GetRange(Double_t &xmin, Double_t &ymin, Double_t &xmax, Double_t &ymax);
   virtual void     GetRange(Double_t &xmin, Double_t &ymin, Double_t &zmin, Double_t &xmax, Double_t &ymax, Double_t &zmax);
   virtual Double_t GetSave(const Double_t *x);
   virtual Double_t GetYmin() const {return fYmin;}
   virtual Double_t GetYmax() const {return fYmax;}
   virtual Double_t Integral(Double_t a, Double_t b, const Double_t *params=0, Double_t epsil=0.000001) {return TF1::Integral(a,b,params,epsil);}
   virtual Double_t Integral(Double_t ax, Double_t bx, Double_t ay, Double_t by, Double_t epsil=0.000001);
   virtual Double_t Integral(Double_t ax, Double_t bx, Double_t ay, Double_t by, Double_t az, Double_t bz, Double_t epsil=0.000001)
                            {return TF1::Integral(ax,bx,ay,by,az,bz,epsil);}
   virtual Bool_t   IsInside(const Double_t *x) const;
   virtual void     Paint(Option_t *option="");
   virtual void     Save(Double_t xmin, Double_t xmax, Double_t ymin, Double_t ymax, Double_t zmin, Double_t zmax);
   virtual void     SavePrimitive(ofstream &out, Option_t *option);
   virtual void     SetNpy(Int_t npy=100); // *MENU*
   virtual void     SetContour(Int_t nlevels=20, const Double_t *levels=0);
   virtual void     SetContourLevel(Int_t level, Double_t value);
   virtual void     SetRange(Double_t xmin, Double_t xmax);
   virtual void     SetRange(Double_t xmin, Double_t ymin, Double_t xmax, Double_t ymax); // *MENU*
   virtual void     SetRange(Double_t xmin, Double_t ymin, Double_t zmin, Double_t xmax, Double_t ymax, Double_t zmax);

   ClassDef(TF2,4)  //The Parametric 2-D function
};

inline void TF2::SetRange(Double_t xmin, Double_t xmax)
   { TF1::SetRange(xmin, xmax); }
inline void TF2::SetRange(Double_t xmin, Double_t ymin, Double_t, Double_t xmax, Double_t ymax, Double_t)
   { SetRange(xmin, ymin, xmax, ymax); }

#endif
