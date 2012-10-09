// @(#)root/cont:$Id$
// Author: Fons Rademakers   21/10/97

/*************************************************************************
 * Copyright (C) 1995-2000, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// TArray                                                               //
//                                                                      //
// Abstract array base class.                                           //
// Data member is public for historical reasons.                        //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

#include "TArray.h"
#include "TError.h"
#include "TClass.h"
#include "TBuffer.h"


ClassImp(TArray)

//______________________________________________________________________________
Bool_t TArray::OutOfBoundsError(const char *where, Int_t i) const
{
   // Generate an out-of-bounds error. Always returns false.

   ::Error(where, "index %d out of bounds (size: %d, this: 0x%lx)", i, fN, (Long_t)this);
   return kFALSE;
}

//______________________________________________________________________________
TArray *TArray::ReadArray(TBuffer &b, const TClass *clReq)
{
   // Read TArray object from buffer. Simplified version of
   // TBuffer::ReadObject (does not keep track of multiple
   // references to same array).

   R__ASSERT(b.IsReading());

   // Make sure ReadArray is initialized
   b.InitMap();

   // Before reading object save start position
   UInt_t startpos = UInt_t(b.Length());

   UInt_t tag;
   TClass *clRef = b.ReadClass(clReq, &tag);

   TArray *a;
   if (!clRef) {

      a = 0;

   } else {

      a = (TArray *) clRef->New();
      if (!a) {
         ::Error("TArray::ReadArray", "could not create object of class %s",
                 clRef->GetName());
         // Exception
         return 0;
      }

      a->Streamer(b);

      b.CheckByteCount(startpos, tag, clRef);
   }

   return a;
}

//______________________________________________________________________________
void TArray::WriteArray(TBuffer &b, const TArray *a)
{
   // Write TArray object to buffer. Simplified version of
   // TBuffer::WriteObject (does not keep track of multiple
   // references to the same array).

   R__ASSERT(b.IsWriting());

   // Make sure WriteMap is initialized
   b.InitMap();

   if (!a) {

      b << (UInt_t) 0;

   } else {

      // Reserve space for leading byte count
      UInt_t cntpos = UInt_t(b.Length());
      b.SetBufferOffset(Int_t(cntpos+sizeof(UInt_t)));

      TClass *cl = a->IsA();
      b.WriteClass(cl);

      ((TArray *)a)->Streamer(b);

      // Write byte count
      b.SetByteCount(cntpos);
   }
}

//______________________________________________________________________________
TBuffer &operator<<(TBuffer &buf, const TArray *obj)
{
   // Write TArray or derived object to buffer.

   TArray::WriteArray(buf, obj);
   return buf;
}




//////////////////////////////////////////////////////////////////////////
//                                                                      //
// TArrayT                                                              //
//                                                                      //
// Templated implementation of TArray.                                  //
// Used by TArray(C|S|I|L|L64|F|D).                                     //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

template class TArrayT<Char_t>;
template class TArrayT<Double_t>;
template class TArrayT<Float_t>;
template class TArrayT<Int_t>;
template class TArrayT<Long_t>;
template class TArrayT<Short_t>;
template class TArrayT<Long64_t>;

//_______________________________________________________________________
template <typename T>
void TArrayT<T>::Streamer(TBuffer &b)
{
   // Stream a TArrayT object
   if (b.IsReading()) {
      Int_t n;
      b >> n;
      Set(n);
      b.ReadFastArray(fArray,n);
   } else {
      b << fN;
      b.WriteFastArray(fArray, fN);
   }
}


