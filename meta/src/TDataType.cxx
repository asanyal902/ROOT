// @(#)root/meta:$Name:  $:$Id: TDataType.cxx,v 1.9 2004/01/29 11:20:12 brun Exp $
// Author: Rene Brun   04/02/95

/*************************************************************************
 * Copyright (C) 1995-2000, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

//////////////////////////////////////////////////////////////////////////
//                                                                      //                                                                      //
// Basic data type descriptor (datatype information is obtained from    //
// CINT). This class describes the attributes of type definitions       //
// (typedef's). The TROOT class contains a list of all currently        //
// defined types (accessible via TROOT::GetListOfTypes()).              //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

#include "TDataType.h"
#include "TInterpreter.h"
#include "Api.h"

ClassImp(TDataType)

//______________________________________________________________________________
TDataType::TDataType(G__TypedefInfo *info) : TDictionary()
{
   // Default TDataType ctor. TDataTypes are constructed in TROOT via
   // a call to TCint::UpdateListOfTypes().

   fInfo = info;

   if (fInfo) {
      SetName(info->Name());
      SetTitle(info->Title());
      SetType(fInfo->TrueName());
   } else {
      SetTitle("Builtin basic type");
   }
}

//______________________________________________________________________________
TDataType::TDataType(const char *typenam)
{
   // Constructor for basic data types, like "char", "unsigned char", etc.

   fInfo = 0;
   SetName(typenam);
   SetTitle("Builtin basic type");

   SetType(fName.Data());
}

//______________________________________________________________________________
TDataType::~TDataType()
{
   // TDataType dtor deletes adopted G__TypedefInfo object.

   delete fInfo;
}

//______________________________________________________________________________
const char *TDataType::GetTypeName() const
{
   // Get basic type of typedef, e,g.: "class TDirectory*" -> "TDirectory".
   // Result needs to be used or copied immediately.

   if (fInfo)
      return gInterpreter->TypeName(fInfo->TrueName());
  else
      return fName.Data();
}

//______________________________________________________________________________
const char *TDataType::GetFullTypeName() const
{
   // Get full type description of typedef, e,g.: "class TDirectory*".

   if (fInfo)
      return fInfo->TrueName();
   else
      return fName.Data();
}

//______________________________________________________________________________
const char *TDataType::AsString(void *buf) const
{
   // Return string containing value in buffer formatted according to
   // the basic data type. The result needs to be used or copied immediately.

   static char line[81];
   const char *name;

   if (fInfo)
      name = fInfo->TrueName();
   else
      name = fName.Data();

   line[0] = 0;
   if (!strcmp("unsigned int", name))
      sprintf(line, "%u", *(unsigned int *)buf);
   else if (!strcmp("int", name))
      sprintf(line, "%d", *(int *)buf);
   else if (!strcmp("unsigned long", name))
      sprintf(line, "%lu", *(unsigned long *)buf);
   else if (!strcmp("long", name))
      sprintf(line, "%ld", *(long *)buf);
   else if (!strcmp("unsigned long long", name))
      sprintf(line, "%llu", *(ULong64_t *)buf);
   else if (!strcmp("long long", name))
      sprintf(line, "%lld", *(Long64_t *)buf);
   else if (!strcmp("unsigned short", name))
      sprintf(line, "%hu", *(unsigned short *)buf);
   else if (!strcmp("short", name))
      sprintf(line, "%hd", *(short *)buf);
   else if (!strcmp("unsigned char", name))
      strncpy(line, (char *)buf,80);
   else if (!strcmp("bool", name))
      strncpy(line, (char *)buf,80);
   else if (!strcmp("char", name))
      strncpy(line, (char *)buf,80);
   else if (!strcmp("float", name))
      sprintf(line, "%g", *(float *)buf);
   else if (!strcmp("double", name))
      sprintf(line, "%g", *(double *)buf);

   return line;
}

//______________________________________________________________________________
Long_t TDataType::Property() const
{
   // Get property description word. For meaning of bits see EProperty.

   if (fInfo)
      return fInfo->Property();
   else
      return (Long_t) kIsFundamental;
}

//______________________________________________________________________________
void TDataType::SetType(const char *name)
{
   // Set type id depending on name.

   fType = kOther_t;
   fSize = 0;

   if (!strcmp("unsigned int", name)) {
      fType = kUInt_t;
      fSize = sizeof(UInt_t);
   } else if (!strcmp("int", name)) {
      fType = kInt_t;
      fSize = sizeof(Int_t);
   } else if (!strcmp("unsigned long", name)) {
      fType = kULong_t;
      fSize = sizeof(ULong_t);
   } else if (!strcmp("long", name)) {
      fType = kLong_t;
      fSize = sizeof(Long_t);
   } else if (!strcmp("unsigned long long", name)) {
      fType = kULong64_t;
      fSize = sizeof(ULong64_t);
   } else if (!strcmp("long long", name)) {
      fType = kLong64_t;
      fSize = sizeof(Long64_t);
   } else if (!strcmp("unsigned short", name)) {
      fType = kUShort_t;
      fSize = sizeof(UShort_t);
   } else if (!strcmp("short", name)) {
      fType = kShort_t;
      fSize = sizeof(Short_t);
   } else if (!strcmp("unsigned char", name)) {
      fType = kUChar_t;
      fSize = sizeof(UChar_t);
   } else if (!strcmp("char", name)) {
      fType = kChar_t;
      fSize = sizeof(Char_t);
   } else if (!strcmp("bool", name)) {
      fType = kUChar_t;
      fSize = sizeof(UChar_t);
   } else if (!strcmp("float", name)) {
      fType = kFloat_t;
      fSize = sizeof(Float_t);
   } else if (!strcmp("double", name)) {
      fType = kDouble_t;
      fSize = sizeof(Double_t);
   } 
   
   if (!strcmp("Double32_t", fName.Data())) {
      fType = kDouble32_t;
   }
}

//______________________________________________________________________________
Int_t TDataType::Size() const
{
   // Get size of basic typedef'ed type.

   if (fInfo)
      return fInfo->Size();
   else
      return fSize;
}
