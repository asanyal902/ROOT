// @(#)root/eve:$Id$
// Author: Matevz Tadel 2007

/*************************************************************************
 * Copyright (C) 1995-2007, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#ifndef ROOT_TEveCaloData
#define ROOT_TEveCaloData

#include <vector>
#include "Rtypes.h"
#include "TEveUtil.h"

class TH2F;
class TAxis;
class TObjArray;

class TEveCaloData: public TEveRefCnt
{
public:
   struct CellId_t
   {
      Int_t fTower;
      Int_t fSlice;

      CellId_t(Int_t t, Int_t s):fTower(t), fSlice(s){}
   };

   struct CellData_t
   {
      Float_t fValue;
      Float_t fPhiMin;
      Float_t fPhiMax;
      Float_t fThetaMin;
      Float_t fThetaMax;
      Int_t   fZSideSign;

      CellData_t(): fValue(0), fPhiMin(0), fPhiMax(0), fThetaMin(0), fThetaMax(0), fZSideSign(1) {}
      void Configure(Float_t v, Float_t e1, Float_t e2, Float_t p1, Float_t p2);

      Float_t Value()    const { return fValue;    }

      Float_t ThetaMin(Bool_t isSigned = kFALSE) const;
      Float_t ThetaMax(Bool_t isSigned = kFALSE) const;
      Float_t Theta(Bool_t a = kFALSE) const { return (ThetaMax(a)+ThetaMin(a))*0.5f; }
      Float_t ThetaDelta() const { return fThetaMax-fThetaMin; }

      Float_t PhiMin()   const { return fPhiMin; }
      Float_t PhiMax()   const { return fPhiMax; }
      Float_t Phi()      const { return (fPhiMin+fPhiMax)*0.5f; }
      Float_t PhiDelta() const { return fPhiMax-fPhiMax; }

      Float_t ZSideSign()const { return fZSideSign;}

      void Dump() const;
   };

   typedef std::vector<CellId_t>           vCellId_t;
   typedef std::vector<CellId_t>::iterator vCellId_i;

private:
   TEveCaloData(const TEveCaloData&);            // Not implemented
   TEveCaloData& operator=(const TEveCaloData&); // Not implemented

public:
   TEveCaloData();
   virtual ~TEveCaloData(){}

   virtual Int_t GetCellList(Float_t etaMin, Float_t etaMax,
                             Float_t phi, Float_t phiRng,
                             Float_t threshold, vCellId_t &out) = 0;

   virtual void  GetCellData(const CellId_t &id, CellData_t& data) = 0;

   virtual Int_t GetNSlices() const = 0;

   virtual Bool_t SupportsEtaBinning(){ return kFALSE; }
   virtual Bool_t SupportsPhiBinning(){ return kFALSE; }
   virtual const TAxis* GetEtaBins(){ return 0 ;}
   virtual const TAxis* GetPhiBins(){ return 0 ;}

   ClassDef(TEveCaloData, 0); // Manages calorimeter event data.
};

/**************************************************************************/
/**************************************************************************/

class TEveCaloDataHist: public TEveCaloData
{
private:
   TEveCaloDataHist(const TEveCaloDataHist&);            // Not implemented
   TEveCaloDataHist& operator=(const TEveCaloDataHist&); // Not implemented

protected:
   TObjArray*    fHStack;
public:
   TEveCaloDataHist();
   virtual ~TEveCaloDataHist();


   virtual Int_t GetCellList(Float_t etaMin, Float_t etaMax,
                             Float_t phi, Float_t phiRng,
                             Float_t threshold, TEveCaloData::vCellId_t &out);

   virtual void GetCellData(const TEveCaloData::CellId_t &id, TEveCaloData::CellData_t& data);

   virtual Int_t GetNSlices() const;

   virtual Bool_t SupportsEtaBinning(){ return kTRUE; }
   virtual Bool_t SupportsPhiBinning(){ return kTRUE; }

   void   AddHistogram(TH2F* hist);
   const  TH2F*  GetHistogram(Int_t slice);

   const TAxis* GetEtaBins();
   const TAxis* GetPhiBins();

   ClassDef(TEveCaloDataHist, 0); // Manages calorimeter TH2F event data. 
};

#endif
