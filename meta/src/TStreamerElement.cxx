// @(#)root/meta:$Name:  $:$Id: TStreamerElement.cxx,v 1.8 2000/12/18 07:12:58 brun Exp $
// Author: Rene Brun   12/10/2000

/*************************************************************************
 * Copyright (C) 1995-2000, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

//////////////////////////////////////////////////////////////////////////
//                                                                      //
//                                                                      //
//////////////////////////////////////////////////////////////////////////


#include "TROOT.h"
#include "TStreamerElement.h"
#include "TStreamerInfo.h"
#include "TClass.h"
#include "TDataMember.h"
#include "TDataType.h"
#include "TMethodCall.h"
#include "TRealData.h"

static char includeName[100];

ClassImp(TStreamerElement)

//______________________________________________________________________________
TStreamerElement::TStreamerElement()
{
   // Default ctor.

   fType        = 0;
   fSize        = 0;
   fNewType     = 0;
   fArrayDim    = 0;
   fArrayLength = 0;
   fStreamer    = 0;
   fMethod      = 0;
   for (Int_t i=0;i<5;i++) fMaxIndex[i] = 0;
}

//______________________________________________________________________________
TStreamerElement::TStreamerElement(const char *name, const char *title, Int_t offset, Int_t dtype, const char *typeName)
        : TNamed(name,title)
{
   // Create a TStreamerElement object.

   fOffset      = offset;
   fType        = dtype;
   fSize        = 0;
   fNewType     = fType;
   fArrayDim    = 0;
   fArrayLength = 0;
   fTypeName    = typeName;
   fStreamer    = 0;
   fMethod      = 0;
   for (Int_t i=0;i<5;i++) fMaxIndex[i] = 0;
}

//______________________________________________________________________________
TStreamerElement::~TStreamerElement()
{
   // TStreamerElement dtor.
   delete fMethod;
}


//______________________________________________________________________________
void TStreamerElement::Init(TObject *)
{
}

//______________________________________________________________________________
void TStreamerElement::ls(Option_t *) const
{
   char name[128];
   char cdim[8];
   sprintf(name,GetName());
   for (Int_t i=0;i<fArrayDim;i++) {
      sprintf(cdim,"[%d]",fMaxIndex[i]);
      strcat(name,cdim);
   }
   printf("  %-14s%-15s offset=%3d type=%2d %-20s\n",GetTypeName(),name,fOffset,fType,GetTitle());
}

//______________________________________________________________________________
void TStreamerElement::SetArrayDim(Int_t dim)
{
   // Set number of array dimensions.
   
   fArrayDim = dim;
   if (dim) fType += TStreamerInfo::kOffsetL;
   fNewType = fType;
}

//______________________________________________________________________________
void TStreamerElement::SetMaxIndex(Int_t dim, Int_t max)
{
   //set maximum index for array with dimension dim
   
   if (dim < 0 || dim > 4) return;
   fMaxIndex[dim] = max;
   if (fArrayLength == 0)  fArrayLength  = max;
   else                    fArrayLength *= max;
}

//______________________________________________________________________________
void TStreamerElement::SetStreamer(Streamer_t streamer)
{
   //set pointer to Streamer function for this element

   fStreamer = streamer;
   if (streamer) {
      if (fArrayLength == 0 && fType != kSTL) return;
      //printf("Changing type of %s from %d to kStreamer\n",GetName(),fType);
      fType = TStreamerInfo::kStreamer;
      fNewType = fType;
   }
}

//______________________________________________________________________________

//////////////////////////////////////////////////////////////////////////
//                                                                      //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

ClassImp(TStreamerBase)

//______________________________________________________________________________
TStreamerBase::TStreamerBase()
{
   // Default ctor.
   
}

//______________________________________________________________________________
TStreamerBase::TStreamerBase(const char *name, const char *title, Int_t offset)
        : TStreamerElement(name,title,offset,TStreamerInfo::kBase,"BASE")
{
   // Create a TStreamerBase object.

   if (strcmp(name,"TObject") == 0) fType = TStreamerInfo::kTObject;
   if (strcmp(name,"TNamed")  == 0) fType = TStreamerInfo::kTNamed;
   fNewType = fType;
   Init();
}

//______________________________________________________________________________
TStreamerBase::~TStreamerBase()
{
   // TStreamerBase dtor.
}

//______________________________________________________________________________
void TStreamerBase::Init(TObject *)
{
   if (fType == TStreamerInfo::kTObject || fType == TStreamerInfo::kTNamed) return;
   fMethod = new TMethodCall();
   fMethod->InitWithPrototype(gROOT->GetClass(GetName()),"StreamerNVirtual","TBuffer &");
}

//______________________________________________________________________________
const char *TStreamerBase::GetInclude() const
{
   TClass *cl = gROOT->GetClass(GetTypeName());
   if (cl && cl->GetClassInfo()) sprintf(includeName,"\"%s\"",cl->GetDeclFileName());
   else                          sprintf(includeName,"\"%s.h\"",GetName());
   return includeName;
}


//______________________________________________________________________________

//////////////////////////////////////////////////////////////////////////
//                                                                      //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

ClassImp(TStreamerBasicPointer)

//______________________________________________________________________________
TStreamerBasicPointer::TStreamerBasicPointer()
{
   // Default ctor.

}

//______________________________________________________________________________
TStreamerBasicPointer::TStreamerBasicPointer(const char *name, const char *title, Int_t offset, Int_t dtype, const char *countName, const char *countClass, Int_t countVersion, const char *typeName)
        : TStreamerElement(name,title,offset,dtype,typeName)
{
   // Create a TStreamerBasicPointer object.

   fType += TStreamerInfo::kOffsetP;
   fCountName    = countName;
   fCountClass   = countClass;
   fCountVersion = countVersion;
   Init();
//   printf("BasicPointer Init:%s, countName=%s, countClass=%s, countVersion=%d, fCounter=%x\n",
//      name,countName,countClass,countVersion,fCounter);
}

//______________________________________________________________________________
TStreamerBasicPointer::~TStreamerBasicPointer()
{
   // TStreamerBasicPointer dtor.
}

//______________________________________________________________________________
ULong_t TStreamerBasicPointer::GetMethod() const
{
   // return address of counter
   
//printf("getmethod, counterAddress=%x\n",fCounter->GetCounterAddress());
   return (ULong_t)fCounter->GetMethod();
}

//______________________________________________________________________________
void TStreamerBasicPointer::Init(TObject *)
{
   
   TClass *cl = gROOT->GetClass(fCountClass.Data());
   fCounter = TStreamerInfo::GetElementCounter(fCountName.Data(),cl,fCountVersion);
   //at this point the counter is may be declared to skip
   if (fCounter) {
      if (fCounter->GetType() < TStreamerInfo::kCounter) fCounter->SetType(TStreamerInfo::kCounter); 
   }  
}


//______________________________________________________________________________

//////////////////////////////////////////////////////////////////////////
//                                                                      //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

ClassImp(TStreamerLoop)

//______________________________________________________________________________
TStreamerLoop::TStreamerLoop()
{
   // Default ctor.

}

//______________________________________________________________________________
TStreamerLoop::TStreamerLoop(const char *name, const char *title, Int_t offset, const char *countName, const char *countClass, Int_t countVersion, const char *typeName)
        : TStreamerElement(name,title,offset,TStreamerInfo::kStreamLoop,typeName)
{
   // Create a TStreamerLoop object.

   fCountName    = countName;
   fCountClass   = countClass;
   fCountVersion = countVersion;
   Init();
}

//______________________________________________________________________________
TStreamerLoop::~TStreamerLoop()
{
   // TStreamerLoop dtor.
}

//______________________________________________________________________________
ULong_t TStreamerLoop::GetMethod() const
{
   // return address of counter
   
   return (ULong_t)fCounter->GetMethod();
}

//______________________________________________________________________________
void TStreamerLoop::Init(TObject *)
{   
   TClass *cl = gROOT->GetClass(fCountClass.Data());
   fCounter = TStreamerInfo::GetElementCounter(fCountName.Data(),cl,fCountVersion);
   //at this point the counter is may be declared to skip
   if (fCounter) {
      if (fCounter->GetType() < TStreamerInfo::kCounter) fCounter->SetType(TStreamerInfo::kCounter); 
   }  
}

//______________________________________________________________________________
const char *TStreamerLoop::GetInclude() const
{
   sprintf(includeName,"<%s>","TString.h"); //to be generalized
   return includeName;
}


//______________________________________________________________________________

//////////////////////////////////////////////////////////////////////////
//                                                                      //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

ClassImp(TStreamerBasicType)

//______________________________________________________________________________
TStreamerBasicType::TStreamerBasicType()
{
   // Default ctor.

}

//______________________________________________________________________________
TStreamerBasicType::TStreamerBasicType(const char *name, const char *title, Int_t offset, Int_t dtype, const char *typeName)
        : TStreamerElement(name,title,offset,dtype,typeName)
{
   // Create a TStreamerBasicType object.

}

//______________________________________________________________________________
TStreamerBasicType::~TStreamerBasicType()
{
   // TStreamerBasicType dtor.
}

//______________________________________________________________________________
ULong_t TStreamerBasicType::GetMethod() const
{
   // return address of counter
   
   if (fType ==  TStreamerInfo::kCounter || 
       fType == (TStreamerInfo::kCounter+TStreamerInfo::kSkip)) return (ULong_t)&fCounter;
   return 0;
}



//______________________________________________________________________________

//////////////////////////////////////////////////////////////////////////
//                                                                      //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

ClassImp(TStreamerObject)

//______________________________________________________________________________
TStreamerObject::TStreamerObject()
{
   // Default ctor.

   fClassObject = 0;
}

//______________________________________________________________________________
TStreamerObject::TStreamerObject(const char *name, const char *title, Int_t offset, const char *typeName)
        : TStreamerElement(name,title,offset,0,typeName)
{
   // Create a TStreamerObject object.

   fType = TStreamerInfo::kObject;
   if (strcmp(typeName,"TObject") == 0) fType = TStreamerInfo::kTObject;
   if (strcmp(typeName,"TNamed")  == 0) fType = TStreamerInfo::kTNamed;
   fNewType = fType;
   Init();
}

//______________________________________________________________________________
TStreamerObject::~TStreamerObject()
{
   // TStreamerObject dtor.
}

//______________________________________________________________________________
void TStreamerObject::Init(TObject *)
{
   fClassObject = gROOT->GetClass(fTypeName.Data());
}

//______________________________________________________________________________
const char *TStreamerObject::GetInclude() const
{
   TClass *cl = gROOT->GetClass(GetTypeName());
   if (cl && cl->GetClassInfo()) sprintf(includeName,"\"%s\"",cl->GetDeclFileName());
   else                          sprintf(includeName,"\"%s.h\"",GetTypeName());
   return includeName;
}


//______________________________________________________________________________

//////////////////////////////////////////////////////////////////////////
//                                                                      //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

ClassImp(TStreamerObjectAny)

//______________________________________________________________________________
TStreamerObjectAny::TStreamerObjectAny()
{
   // Default ctor.

   fClassObject = 0;
}

//______________________________________________________________________________
TStreamerObjectAny::TStreamerObjectAny(const char *name, const char *title, Int_t offset, const char *typeName)
        : TStreamerElement(name,title,offset,TStreamerInfo::kAny,typeName)
{
   // Create a TStreamerObjectAny object.

   Init();
}

//______________________________________________________________________________
TStreamerObjectAny::~TStreamerObjectAny()
{
   // TStreamerObjectAny dtor.
}

//______________________________________________________________________________
void TStreamerObjectAny::Init(TObject *)
{
   fClassObject = gROOT->GetClass(fTypeName.Data());
}

//______________________________________________________________________________
const char *TStreamerObjectAny::GetInclude() const
{
   TClass *cl = gROOT->GetClass(GetTypeName());
   if (cl && cl->GetClassInfo()) sprintf(includeName,"\"%s\"",cl->GetDeclFileName());
   else                          sprintf(includeName,"\"%s.h\"",GetTypeName());
   return includeName;
}



//______________________________________________________________________________

//////////////////////////////////////////////////////////////////////////
//                                                                      //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

ClassImp(TStreamerObjectPointer)

//______________________________________________________________________________
TStreamerObjectPointer::TStreamerObjectPointer()
{
   // Default ctor.

   fClassObject = 0;
}

//______________________________________________________________________________
TStreamerObjectPointer::TStreamerObjectPointer(const char *name, const char *title, Int_t offset, const char *typeName)
        : TStreamerElement(name,title,offset,TStreamerInfo::kObjectP,typeName)
{
   // Create a TStreamerObjectPointer object.

   if (strncmp(title,"->",2) == 0) fType = TStreamerInfo::kObjectp;
   fNewType = fType;
   Init();
}

//______________________________________________________________________________
TStreamerObjectPointer::~TStreamerObjectPointer()
{
   // TStreamerObjectPointer dtor.
}

//______________________________________________________________________________
void TStreamerObjectPointer::Init(TObject *)
{
   fClassObject = gROOT->GetClass(fTypeName.Data());
}

//______________________________________________________________________________
const char *TStreamerObjectPointer::GetInclude() const
{
   TClass *cl = gROOT->GetClass(GetTypeName());
   if (cl && cl->GetClassInfo()) sprintf(includeName,"\"%s\"",cl->GetDeclFileName());
   else                          sprintf(includeName,"\"%s.h\"",GetTypeName());
   return includeName;
}


//______________________________________________________________________________

//////////////////////////////////////////////////////////////////////////
//                                                                      //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

ClassImp(TStreamerString)

//______________________________________________________________________________
TStreamerString::TStreamerString()
{
   // Default ctor.

}

//______________________________________________________________________________
TStreamerString::TStreamerString(const char *name, const char *title, Int_t offset)
        : TStreamerElement(name,title,offset,TStreamerInfo::kTString,"TString")
{
   // Create a TStreamerString object.

}

//______________________________________________________________________________
TStreamerString::~TStreamerString()
{
   // TStreamerString dtor.
}

//______________________________________________________________________________

//////////////////////////////////////////////////////////////////////////
//                                                                      //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

ClassImp(TStreamerSTL)

//______________________________________________________________________________
TStreamerSTL::TStreamerSTL()
{
   // Default ctor.

}

//______________________________________________________________________________
TStreamerSTL::TStreamerSTL(const char *name, const char *title, Int_t offset, const char *typeName, Bool_t dmPointer)
        : TStreamerElement(name,title,offset,kSTL,typeName)
{
   // Create a TStreamerSTL object.
   
   Int_t nch = strlen(typeName);
   char *s = new char[nch+1];
   strcpy(s,typeName);
   char *sopen  = strchr(s,'<'); *sopen  = 0; sopen++;
   char *sclose = strchr(sopen+1,'>'); *sclose = 0;
   char *sconst = strstr(sopen,"const");
   if (sconst) sopen = sconst + 5;
   fSTLtype = 0;
   fCtype   = 0;
   if      (strstr(s,"vector"))   fSTLtype = kSTLvector;
   else if (strstr(s,"list"))     fSTLtype = kSTLlist;
   else if (strstr(s,"deque"))    fSTLtype = kSTLdeque;
   else if (strstr(s,"map"))      fSTLtype = kSTLmap;
   else if (strstr(s,"set"))      fSTLtype = kSTLset;
   else if (strstr(s,"multimap")) fSTLtype = kSTLmultimap;
   else if (strstr(s,"multiset")) fSTLtype = kSTLmultiset;
   if (fSTLtype == 0) { delete [] s; return;}
   if (dmPointer) fSTLtype += TStreamerInfo::kOffsetP;
   
   // find STL contained type
    while (*sopen==' ') sopen++;
   Bool_t isPointer = kFALSE;
   char *star = strchr(sopen,'*');
   if (star) {
      isPointer = kTRUE;
      *star = 0;
      sclose = star - 1;
   }
   while (*sclose == ' ') {*sclose = 0; sclose--;}
   
   
   TDataType *dt = (TDataType*)gROOT->GetListOfTypes()->FindObject(sopen);
   if (dt) {
      fCtype = dt->GetType();
      if (isPointer) fCtype += TStreamerInfo::kOffsetP;
   } else {
      TClass *cl = gROOT->GetClass(sopen);
      if (cl) {
         if (isPointer) fCtype = TStreamerInfo::kObjectp;
         else           fCtype = TStreamerInfo::kObject;
      } else {
         printf ("UNKNOW type, sopen=%s\n",sopen);
      }
   }
   delete [] s;
   
}

//______________________________________________________________________________
TStreamerSTL::~TStreamerSTL()
{
   // TStreamerSTL dtor.
}

//______________________________________________________________________________
void TStreamerSTL::ls(Option_t *) const
{
   char name[128];
   char cdim[8];
   sprintf(name,GetName());
   for (Int_t i=0;i<fArrayDim;i++) {
      sprintf(cdim,"[%d]",fMaxIndex[i]);
      strcat(name,cdim);
   }
   printf("  %-14s%-15s offset=%3d type=%2d ,stl=%d, ctype=%d, %-20s\n",GetTypeName(),name,fOffset,fType,fSTLtype,fCtype,GetTitle());
}

//______________________________________________________________________________
const char *TStreamerSTL::GetInclude() const
{
   if      (fSTLtype == kSTLvector)   sprintf(includeName,"<%s>","vector");
   else if (fSTLtype == kSTLlist)     sprintf(includeName,"<%s>","list");
   else if (fSTLtype == kSTLdeque)    sprintf(includeName,"<%s>","deque");
   else if (fSTLtype == kSTLmap)      sprintf(includeName,"<%s>","map");
   else if (fSTLtype == kSTLset)      sprintf(includeName,"<%s>","set");
   else if (fSTLtype == kSTLmultimap) sprintf(includeName,"<%s>","multimap");
   else if (fSTLtype == kSTLmultiset) sprintf(includeName,"<%s>","multiset");
   return includeName;
}

//______________________________________________________________________________

//////////////////////////////////////////////////////////////////////////
//                                                                      //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

ClassImp(TStreamerSTLstring)

//______________________________________________________________________________
TStreamerSTLstring::TStreamerSTLstring()
{
   // Default ctor.

}

//______________________________________________________________________________
TStreamerSTLstring::TStreamerSTLstring(const char *name, const char *title, Int_t offset)
        : TStreamerSTL()
{
   // Create a TStreamerSTLstring object.
   
   SetName(name);
   SetTitle(title);
   fType    = kSTL;
   fNewType = fType;
   fOffset  = offset;
   fSTLtype = kSTLstring;
   fCtype   = kSTLstring;
   fTypeName= "string";
}

//______________________________________________________________________________
TStreamerSTLstring::~TStreamerSTLstring()
{
   // TStreamerSTLstring dtor.
}

//______________________________________________________________________________
const char *TStreamerSTLstring::GetInclude() const
{
   sprintf(includeName,"<string>");
   return includeName;
}
