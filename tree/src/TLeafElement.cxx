// @(#)root/tree:$Name:  $:$Id: TLeafElement.cxx,v 1.4 2001/01/18 10:37:33 brun Exp $
// Author: Rene Brun   14/01/2001

/*************************************************************************
 * Copyright (C) 1995-2000, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// A TLeaf for a general object derived from TObject.                   //
//////////////////////////////////////////////////////////////////////////

#include "TROOT.h"
#include "TLeafElement.h"
#include "TStreamerInfo.h"
#include "TStreamerElement.h"
#include "TBranchElement.h"
#include "TClass.h"
#include "TMethodCall.h"
#include "TDataType.h"


ClassImp(TLeafElement)

//______________________________________________________________________________
TLeafElement::TLeafElement(): TLeaf()
{
//*-*-*-*-*-*Default constructor for LeafObject*-*-*-*-*-*-*-*-*-*-*-*-*-*
//*-*        =================================
   fAbsAddress = 0;
}

//______________________________________________________________________________
TLeafElement::TLeafElement(const char *name, Int_t id, Int_t type)
       :TLeaf(name,name)
{
//*-*-*-*-*-*-*-*-*-*-*-*-*Create a LeafObject*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
//*-*                      ==================
//*-*

  fAbsAddress = 0;
  fID         = id;
  fType       = type;
}

//______________________________________________________________________________
TLeafElement::~TLeafElement()
{
//*-*-*-*-*-*Default destructor for a LeafObject*-*-*-*-*-*-*-*-*-*-*-*
//*-*        ==================================

}

//______________________________________________________________________________
TMethodCall *TLeafElement::GetMethodCall(const char *name)
{
//*-*-*-*-*-*-*-*Returns pointer to method corresponding to name*-*-*-*-*-*-*
//*-*            ============================================
//*-*
//*-*    name is a string with the general form  "method(list of params)"
//*-*   If list of params is omitted, () is assumed;
//*-*
   return 0;
}

//______________________________________________________________________________
Double_t TLeafElement::GetValue(Int_t) const
{
// Returns leaf value

   return Double_t(fType);
}

//______________________________________________________________________________
void TLeafElement::PrintValue(Int_t) const
{
// Prints leaf value

   // basic types
   switch (fType) {
      case TStreamerInfo::kChar:   {printf("should print Char"); break;}
      case TStreamerInfo::kShort:  {Short_t *val = (Short_t*)fAbsAddress; printf("%d",*val); break;}
      case TStreamerInfo::kInt:    {Int_t *val = (Int_t*)fAbsAddress; printf("%d",*val); break;}
      case TStreamerInfo::kLong:   {printf("should print Long"); break;}
      case TStreamerInfo::kFloat:  {Float_t *val = (Float_t*)fAbsAddress; printf("%f",*val); break;}
      case TStreamerInfo::kDouble: {Double_t *val = (Double_t*)fAbsAddress; printf("%g",*val); break;}
      case TStreamerInfo::kUChar:  {printf("should print UChar"); break;}
      case TStreamerInfo::kUShort: {UShort_t *val = (UShort_t*)fAbsAddress; printf("%d",*val); break;}
      case TStreamerInfo::kUInt:   {UInt_t *val = (UInt_t*)fAbsAddress; printf("%d",*val); break;}
      case TStreamerInfo::kULong:  {ULong_t *val = (ULong_t*)fAbsAddress; printf("%ld",*val); break;}
   }
}
