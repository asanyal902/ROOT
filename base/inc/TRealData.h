// @(#)root/base:$Name:  $:$Id: TRealData.h,v 1.5 2000/12/02 16:26:48 brun Exp $
// Author: Rene Brun   05/03/95

/*************************************************************************
 * Copyright (C) 1995-2000, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#ifndef ROOT_TRealData
#define ROOT_TRealData


//////////////////////////////////////////////////////////////////////////
//                                                                      //
// TRealData                                                            //
//                                                                      //
// Description of persistent data members.                              //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

#ifndef ROOT_TObject
#include "TObject.h"
#endif
#ifndef ROOT_TString
#include "TString.h"
#endif

class TDataMember;


class TRealData : public TObject {

private:
   TDataMember *fDataMember;         //pointer to data member descriptor
   Int_t        fThisOffset;         //offset with the THIS object pointer
   TString      fName;               //Concatenated names of this realdata
   Streamer_t   fStreamer;           //!pointer to STL Streamer function
   Bool_t       fIsObject;           //!true if member is an object
      
public:
   TRealData();
   TRealData(const char *name, Int_t offset, TDataMember *datamember);
   virtual     ~TRealData();
   virtual const char *GetName() const {return fName.Data();}
   TDataMember *GetDataMember() const {return fDataMember;}
   Streamer_t   GetStreamer() const {return fStreamer;}
   Int_t        GetThisOffset() const {return fThisOffset;}
   Bool_t       IsObject() const {return fIsObject;}
   void         SetIsObject(Bool_t isObject) {fIsObject=isObject;}
   void         SetStreamer(Streamer_t p) {fStreamer = p;}
   void         WriteRealData(void *pointer, char *&buffer);

   ClassDef(TRealData,0)  //Description of persistent data members
};

#endif

