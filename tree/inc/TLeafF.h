// @(#)root/tree:$Name:  $:$Id: TLeafF.h,v 1.2 2000/06/13 09:27:08 brun Exp $
// Author: Rene Brun   12/01/96

/*************************************************************************
 * Copyright (C) 1995-2000, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#ifndef ROOT_TLeafF
#define ROOT_TLeafF


//////////////////////////////////////////////////////////////////////////
//                                                                      //
// TLeafF                                                               //
//                                                                      //
// A TLeaf for a 32 bit floating point data type.                       //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

#ifndef ROOT_TLeaf
#include "TLeaf.h"
#endif

class TLeafF : public TLeaf {

protected:
    Float_t       fMinimum;         //Minimum value if leaf range is specified
    Float_t       fMaximum;         //Maximum value if leaf range is specified
    Float_t       *fValue;          //!Pointer to data buffer
    Float_t       **fPointer;       //!Addresss of pointer to data buffer!

public:
    TLeafF();
    TLeafF(const char *name, const char *type);
    virtual ~TLeafF();

    virtual void    Export(TClonesArray *list, Int_t n);
    virtual void    FillBasket(TBuffer &b);
    const char     *GetTypeName() const {return "Float_t";}
    Double_t        GetValue(Int_t i=0);
    virtual void   *GetValuePointer() {return fValue;}
    virtual void    Import(TClonesArray *list, Int_t n);
    virtual void    Print(Option_t *option="");
    virtual void    ReadBasket(TBuffer &b);
    virtual void    ReadBasketExport(TBuffer &b, TClonesArray *list, Int_t n);
    virtual void    SetAddress(void *add=0);

    ClassDef(TLeafF,1)  //A TLeaf for a 32 bit floating point data type.
};

inline Double_t TLeafF::GetValue(Int_t i) { return fValue[i]; }

#endif
