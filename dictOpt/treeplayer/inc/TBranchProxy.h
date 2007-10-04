// @(#)root/treeplayer:$Id$
// Author: Philippe Canal 01/06/2004

/*************************************************************************
 * Copyright (C) 1995-2004, Rene Brun and Fons Rademakers and al.        *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#ifndef ROOT_TBranchProxy
#define ROOT_TBranchProxy

#ifndef ROOT_TBranchProxyDirector
#include "TBranchProxyDirector.h"
#endif
#ifndef ROOT_TTree
#include "TTree.h"
#endif
#ifndef ROOT_TBranch
#include "TBranch.h"
#endif
#ifndef ROOT_TClonesArray
#include "TClonesArray.h"
#endif
#ifndef ROOT_TString
#include "TString.h"
#endif
#ifndef ROOT_Riostream
#include "Riostream.h"
#endif
#ifndef ROOT_TError
#include "TError.h"
#endif
#ifndef ROOT_TVirtualCollectionProxy
#include "TVirtualCollectionProxy.h"
#endif

#include <list>
#include <algorithm>

class TBranch;
class TStreamerElement;

// Note we could protect the arrays more by introducing a class TArrayWrapper<class T> which somehow knows
// its internal dimensions and check for them ...
// template <class T> TArrayWrapper {
// public:
//    TArrayWrapper(void *where, int dim1);
//    const T operator[](int i) {
//       if (i>=dim1) return 0;
//       return where[i];
//    };
// };
// 2D array would actually be a wrapper of a wrapper i.e. has a method TArrayWrapper<T> operator[](int i);

namespace ROOT {

   class TBranchProxyHelper {
   public:
      TString fName;
      TBranchProxyHelper(const char *left,const char *right = 0) :
         fName() {
         if (left) {
            fName = left;
            if (strlen(left)&&right && fName[fName.Length()-1]!='.') fName += ".";
         }
         if (right) {
            fName += right;
         }
      }
      operator const char*() { return fName.Data(); };
   };


   class TBranchProxy {
   protected:
      TBranchProxyDirector *fDirector; // contain pointer to TTree and entry to be read

      Bool_t   fInitialized;

      const TString fBranchName;  // name of the branch to read
      TBranchProxy *fParent;      // Proxy to a parent object

      const TString fDataMember;  // name of the (eventual) data member being proxied

      const Bool_t  fIsMember;    // true if we proxy an unsplit data member
      Bool_t        fIsClone;     // true if we proxy the inside of a TClonesArray
      Bool_t        fIsaPointer;  // true if we proxy a data member of pointer type


      TString           fClassName;     // class name of the object pointed to by the branch
      TClass           *fClass;         // class name of the object pointed to by the branch
      TStreamerElement *fElement;
      Int_t             fMemberOffset;
      Int_t             fOffset;        // Offset inside the object

      TBranch *fBranch;       // branch to read
      TBranch *fBranchCount;  // eventual auxiliary branch (for example holding the size)

      TTree   *fLastTree; // TTree containing the last entry read
      Long64_t fRead;     // Last entry read

      void    *fWhere;    // memory location of the data
      TVirtualCollectionProxy *fCollection; // Handle to the collection containing the data chunk.

   public:
      virtual void Print();

      TBranchProxy();
      TBranchProxy(TBranchProxyDirector* boss, const char* top, const char* name = 0);
      TBranchProxy(TBranchProxyDirector* boss, const char *top, const char *name, const char *membername);
      TBranchProxy(TBranchProxyDirector* boss, TBranchProxy *parent, const char* membername, const char* top = 0, const char* name = 0);
      virtual ~TBranchProxy();

      TBranchProxy* GetProxy() { return this; }

      void Reset();

      Bool_t Setup();

      Bool_t IsInitialized() {
         return (fLastTree == fDirector->GetTree()) && (fLastTree);
      }

      Bool_t IsaPointer() const {
         return fIsaPointer;
      }

      Bool_t Read() {
         if (fDirector==0) return false;

         if (fDirector->GetReadEntry()!=fRead) {
            if (!IsInitialized()) {
               if (!Setup()) {
                  Error("Read",Form("Unable to initialize %s\n",fBranchName.Data()));
                  return false;
               }
            }
            if (fParent) fParent->Read();
            else {
               if (fBranchCount) {
                  fBranchCount->GetEntry(fDirector->GetReadEntry());
               }
               fBranch->GetEntry(fDirector->GetReadEntry());
            }
            fRead = fDirector->GetReadEntry();
         }
         return IsInitialized();
      }

      Bool_t ReadEntries() {
         if (fDirector==0) return false;

         if (fDirector->GetReadEntry()!=fRead) {
            if (!IsInitialized()) {
               if (!Setup()) {
                  Error("Read",Form("Unable to initialize %s\n",fBranchName.Data()));
                  return false;
               }
            }
            if (fParent) fParent->ReadEntries();
            else {
               if (fBranchCount) {
                  fBranchCount->TBranch::GetEntry(fDirector->GetReadEntry());
               }
               fBranch->TBranch::GetEntry(fDirector->GetReadEntry());
            }
            // NO - we only read the entries, not the contained objects!
            // fRead = fDirector->GetReadEntry();
         }
         return IsInitialized();
      }

      TClass *GetClass() {
         if (fDirector==0) return 0;
         if (fDirector->GetReadEntry()!=fRead) {
            if (!IsInitialized()) {
               if (!Setup()) {
                  return 0;
               }
            }
         }
         return fClass;
      }

      void* GetWhere() const { return fWhere; } // intentionally non-virtual
      
      TVirtualCollectionProxy *GetCollection() { return fCollection; }

      // protected:
      virtual  void *GetStart(int /*i*/=0) {
         // return the address of the start of the object being proxied. Assumes
         // that Setup() has been called.

         if (fParent) {
            fWhere = ((unsigned char*)fParent->GetStart()) + fMemberOffset;
         }
         if (IsaPointer()) {
            if (fWhere) return *(void**)fWhere;
            else return 0;
         } else {
            return fWhere;
         }
      }

      virtual void *GetClaStart(int i=0) {
         // return the address of the start of the object being proxied. Assumes
         // that Setup() has been called.  Assumes the object containing this data
         // member is held in TClonesArray.

         char *location;

         if (fIsClone) {

            TClonesArray *tca;
            tca = (TClonesArray*)GetStart();

            if (tca->GetLast()<i) return 0;

            location = (char*)tca->At(i);

            return location;

         } else if (fParent) {

            //tcaloc = ((unsigned char*)fParent->GetStart());
            location = (char*)fParent->GetClaStart(i);

         } else {

            void *tcaloc;
            tcaloc = fWhere;
            TClonesArray *tca;
            tca = (TClonesArray*)tcaloc;

            if (tca->GetLast()<i) return 0;

            location = (char*)tca->At(i);
         }

         if (location) location += fOffset;
         else return 0;

         if (IsaPointer()) {
            return *(void**)(location);
         } else {
            return location;
         }

      }

      virtual void *GetStlStart(UInt_t i=0) {
         // return the address of the start of the object being proxied. Assumes
         // that Setup() has been called.  Assumes the object containing this data
         // member is held in TClonesArray.

         char *location=0;

         if (fCollection) {

            if (fCollection->Size()<i) return 0;

            location = (char*)fCollection->At(i);

            // return location;

         } else if (fParent) {

            //tcaloc = ((unsigned char*)fParent->GetStart());
            location = (char*)fParent->GetStlStart(i);

         } else {

            R__ASSERT(0);
            //void *tcaloc;
            //tcaloc = fWhere;
            //TClonesArray *tca;
            //tca = (TClonesArray*)tcaloc;

            //if (tca->GetLast()<i) return 0;

            //location = (char*)tca->At(i);
         }

         if (location) location += fOffset;
         else return 0;

         if (IsaPointer()) {
            return *(void**)(location);
         } else {
            return location;
         }

      }
   };

   class TArrayCharProxy : public TBranchProxy {
   public:
      void Print() {
         TBranchProxy::Print();
         cout << "fWhere " << fWhere << endl;
         if (fWhere) cout << "value? " << *(unsigned char*)GetStart() << endl;
      }

      TArrayCharProxy() : TBranchProxy() {}
      TArrayCharProxy(TBranchProxyDirector *director, const char *name) : TBranchProxy(director,name) {};
      TArrayCharProxy(TBranchProxyDirector *director, const char *top, const char *name) :
         TBranchProxy(director,top,name) {};
      TArrayCharProxy(TBranchProxyDirector *director, const char *top, const char *name, const char *data) :
         TBranchProxy(director,top,name,data) {};
      TArrayCharProxy(TBranchProxyDirector *director, TBranchProxy *parent, const char *name, const char* top = 0, const char* mid = 0) :
         TBranchProxy(director,parent, name, top, mid) {};
      ~TArrayCharProxy() {};

      unsigned char At(int i) {
         static unsigned char default_val;
         if (!Read()) return default_val;
         // should add out-of bound test
         unsigned char* str = (unsigned char*)GetStart();
         return str[i];
      }

      unsigned char operator [](int i) {
         return At(i);
      }

      const char* c_str() {
         if (!Read()) return "";
         return (const char*)GetStart();
      }

      operator std::string() {
         if (!Read()) return "";
         return std::string((const char*)GetStart());
      }

   };

   class TClaProxy : public TBranchProxy {
   public:
      void Print() {
         TBranchProxy::Print();
         cout << "fWhere " << fWhere << endl;
         if (fWhere) {
            if (IsaPointer()) {
               cout << "location " << *(TClonesArray**)fWhere << endl;
            } else {
               cout << "location " << fWhere << endl;
            }
         }
      }

      TClaProxy() : TBranchProxy() {}
      TClaProxy(TBranchProxyDirector *director, const char *name) : TBranchProxy(director,name) {};
      TClaProxy(TBranchProxyDirector *director, const char *top, const char *name) :
         TBranchProxy(director,top,name) {};
      TClaProxy(TBranchProxyDirector *director, const char *top, const char *name, const char *data) :
         TBranchProxy(director,top,name,data) {};
      TClaProxy(TBranchProxyDirector *director, TBranchProxy *parent, const char *name, const char* top = 0, const char* mid = 0) :
         TBranchProxy(director,parent, name, top, mid) {};
      ~TClaProxy() {};

      const TClonesArray* GetPtr() {
         if (!Read()) return 0;
         return (TClonesArray*)GetStart();
      }

      Int_t GetEntries() {
         if (!ReadEntries()) return 0;
         TClonesArray *arr = (TClonesArray*)GetStart();
         if (arr) return arr->GetEntries();
         return 0;
      }

      const TClonesArray* operator->() { return GetPtr(); }

   };

   class TStlProxy : public TBranchProxy {
   public:
      void Print() {
         TBranchProxy::Print();
         cout << "fWhere " << fWhere << endl;
         if (fWhere) {
            if (IsaPointer()) {
               cout << "location " << *(TClonesArray**)fWhere << endl;
            } else {
               cout << "location " << fWhere << endl;
            }
         }
      }

      TStlProxy() : TBranchProxy() {}
      TStlProxy(TBranchProxyDirector *director, const char *name) : TBranchProxy(director,name) {};
      TStlProxy(TBranchProxyDirector *director, const char *top, const char *name) :
         TBranchProxy(director,top,name) {};
      TStlProxy(TBranchProxyDirector *director, const char *top, const char *name, const char *data) :
         TBranchProxy(director,top,name,data) {};
      TStlProxy(TBranchProxyDirector *director, TBranchProxy *parent, const char *name, const char* top = 0, const char* mid = 0) :
         TBranchProxy(director,parent, name, top, mid) {};
      ~TStlProxy() {};

      const TVirtualCollectionProxy* GetPtr() {
         if (!Read()) return 0;
         return GetCollection();;
      }

      Int_t GetEntries() {
         if (!ReadEntries()) return 0;
         return GetPtr()->Size();
      }

      const TVirtualCollectionProxy* operator->() { return GetPtr(); }

   };

   template <class T>
   class TImpProxy : public TBranchProxy {
   public:
      void Print() {
         TBranchProxy::Print();
         cout << "fWhere " << fWhere << endl;
         if (fWhere) cout << "value? " << *(T*)GetStart() << endl;
      }

      TImpProxy() : TBranchProxy() {};
      TImpProxy(TBranchProxyDirector *director, const char *name) : TBranchProxy(director,name) {};
      TImpProxy(TBranchProxyDirector *director, const char *top, const char *name) :
         TBranchProxy(director,top,name) {};
      TImpProxy(TBranchProxyDirector *director, const char *top, const char *name, const char *data) :
         TBranchProxy(director,top,name,data) {};
      TImpProxy(TBranchProxyDirector *director, TBranchProxy *parent, const char *name, const char* top = 0, const char* mid = 0) : 
         TBranchProxy(director,parent, name, top, mid) {};
      ~TImpProxy() {};

      operator T() {
         if (!Read()) return 0;
         return *(T*)GetStart();
      }

      // Make sure that the copy methods are really private
#ifdef private
#undef private
#define private_was_replaced
#endif
      // For now explicitly disable copying into the value (i.e. the proxy is read-only).
   private:
      TImpProxy(T);
      TImpProxy &operator=(T);
#ifdef private_was_replaced
#define private public
#endif

   };

   // Helper template to be able to determine and
   // use array dimentsions.
   template <class T, int d = 0> struct TArrayType {
      typedef T type_t;
      typedef T array_t[d];
   };
   template <class T> struct TArrayType<T,0> {
      typedef T type_t;
      typedef T array_t;
   };
   template <class T, int d> struct TMultiArrayType {
      typedef typename T::type_t type_t;
      typedef typename T::array_t array_t[d];
   };

   template <class T>
   class TArrayProxy : public TBranchProxy {
   public:
      TArrayProxy() : TBranchProxy() {}
      TArrayProxy(TBranchProxyDirector *director, const char *name) : TBranchProxy(director,name) {};
      TArrayProxy(TBranchProxyDirector *director, const char *top, const char *name) :
         TBranchProxy(director,top,name) {};
      TArrayProxy(TBranchProxyDirector *director, const char *top, const char *name, const char *data) :
         TBranchProxy(director,top,name,data) {};
      TArrayProxy(TBranchProxyDirector *director, TBranchProxy *parent, const char *name, const char* top = 0, const char* mid = 0) :
         TBranchProxy(director,parent, name, top, mid) {};
      ~TArrayProxy() {};

      typedef typename T::array_t array_t;
      typedef typename T::type_t type_t;

      void Print() {
         TBranchProxy::Print();
         cout << "fWhere " << GetWhere() << endl;
         if (GetWhere()) cout << "value? " << *(type_t*)GetWhere() << endl;
      }

      const array_t &At(int i) {
         static array_t default_val;
         if (!Read()) return default_val;
         // should add out-of bound test
         array_t *arr = 0;
         arr = (array_t*)((type_t*)(GetStart()));
         if (arr) return arr[i];
         else return default_val;
      }

      const array_t &operator [](int i) { return At(i); }
   };

   template <class T>
   class TClaImpProxy : public TBranchProxy {
   public:

      void Print() {
         TBranchProxy::Print();
         cout << "fWhere " << fWhere << endl;
         if (fWhere) cout << "value? " << *(T*)GetStart() << endl;
      }

      TClaImpProxy() : TBranchProxy() {};
      TClaImpProxy(TBranchProxyDirector *director, const char *name) : TBranchProxy(director,name) {};
      TClaImpProxy(TBranchProxyDirector *director,  const char *top, const char *name) :
         TBranchProxy(director,top,name) {};
      TClaImpProxy(TBranchProxyDirector *director,  const char *top, const char *name, const char *data) :
         TBranchProxy(director,top,name,data) {};
      TClaImpProxy(TBranchProxyDirector *director, TBranchProxy *parent, const char *name, const char* top = 0, const char* mid = 0) : 
         TBranchProxy(director,parent, name, top, mid) {};
      ~TClaImpProxy() {};

      const T& At(int i) {
         static T default_val;
         if (!Read()) return default_val;
         if (fWhere==0) return default_val;

         T *temp = (T*)GetClaStart(i);

         if (temp) return *temp;
         else return default_val;

      }

      const T& operator [](int i) { return At(i); }

      // Make sure that the copy methods are really private
#ifdef private
#undef private
#define private_was_replaced
#endif
      // For now explicitly disable copying into the value (i.e. the proxy is read-only).
   private:
      TClaImpProxy(T);
      TClaImpProxy &operator=(T);
#ifdef private_was_replaced
#define private public
#endif

   };

   template <class T>
   class TStlImpProxy : public TBranchProxy {
   public:

      void Print() {
         TBranchProxy::Print();
         cout << "fWhere " << fWhere << endl;
         if (fWhere) cout << "value? " << *(T*)GetStart() << endl;
      }

      TStlImpProxy() : TBranchProxy() {};
      TStlImpProxy(TBranchProxyDirector *director, const char *name) : TBranchProxy(director,name) {};
      TStlImpProxy(TBranchProxyDirector *director,  const char *top, const char *name) :
         TBranchProxy(director,top,name) {};
      TStlImpProxy(TBranchProxyDirector *director,  const char *top, const char *name, const char *data) :
         TBranchProxy(director,top,name,data) {};
      TStlImpProxy(TBranchProxyDirector *director, TBranchProxy *parent, const char *name, const char* top = 0, const char* mid = 0) : 
         TBranchProxy(director,parent, name, top, mid) {};
      ~TStlImpProxy() {};

      const T& At(int i) {
         static T default_val;
         if (!Read()) return default_val;
         if (fWhere==0) return default_val;

         T *temp = (T*)GetStlStart(i);

         if (temp) return *temp;
         else return default_val;
      }

      const T& operator [](int i) { return At(i); }

      // Make sure that the copy methods are really private
#ifdef private
#undef private
#define private_was_replaced
#endif
      // For now explicitly disable copying into the value (i.e. the proxy is read-only).
   private:
      TStlImpProxy(T);
      TStlImpProxy &operator=(T);
#ifdef private_was_replaced
#define private public
#endif

   };

   template <class T>
   class TClaArrayProxy : public TBranchProxy {
   public:
      typedef typename T::array_t array_t;
      typedef typename T::type_t type_t;

      void Print() {
         TBranchProxy::Print();
         cout << "fWhere " << fWhere << endl;
         if (fWhere) cout << "value? " << *(type_t*)GetStart() << endl;
      }

      TClaArrayProxy() : TBranchProxy() {}
      TClaArrayProxy(TBranchProxyDirector *director, const char *name) : TBranchProxy(director,name) {};
      TClaArrayProxy(TBranchProxyDirector *director, const char *top, const char *name) :
         TBranchProxy(director,top,name) {};
      TClaArrayProxy(TBranchProxyDirector *director, const char *top, const char *name, const char *data) :
         TBranchProxy(director,top,name,data) {};
      TClaArrayProxy(TBranchProxyDirector *director, TBranchProxy *parent, const char *name, const char* top = 0, const char* mid = 0) :
         TBranchProxy(director,parent, name, top, mid) {};
      ~TClaArrayProxy() {};

      /* const */  array_t *At(int i) {
         static array_t default_val;
         if (!Read()) return &default_val;
         if (fWhere==0) return &default_val;

         return (array_t*)GetClaStart(i);
      }

      /* const */ array_t *operator [](int i) { return At(i); }
   };

   template <class T>
   class TStlArrayProxy : public TBranchProxy {
   public:
      typedef typename T::array_t array_t;
      typedef typename T::type_t type_t;

      void Print() {
         TBranchProxy::Print();
         cout << "fWhere " << fWhere << endl;
         if (fWhere) cout << "value? " << *(type_t*)GetStart() << endl;
      }

      TStlArrayProxy() : TBranchProxy() {}
      TStlArrayProxy(TBranchProxyDirector *director, const char *name) : TBranchProxy(director,name) {};
      TStlArrayProxy(TBranchProxyDirector *director, const char *top, const char *name) :
         TBranchProxy(director,top,name) {};
      TStlArrayProxy(TBranchProxyDirector *director, const char *top, const char *name, const char *data) :
         TBranchProxy(director,top,name,data) {};
      TStlArrayProxy(TBranchProxyDirector *director, TBranchProxy *parent, const char *name, const char* top = 0, const char* mid = 0) :
         TBranchProxy(director,parent, name, top, mid) {};
      ~TStlArrayProxy() {};

      /* const */  array_t *At(int i) {
         static array_t default_val;
         if (!Read()) return &default_val;
         if (fWhere==0) return &default_val;

         return (array_t*)GetStlStart(i);
      }

      /* const */ array_t *operator [](int i) { return At(i); }
   };

   //TImpProxy<TObject> d;
   typedef TImpProxy<Double_t>   TDoubleProxy;
   typedef TImpProxy<Double32_t> TDouble32Proxy;
   typedef TImpProxy<Float_t>    TFloatProxy;
   typedef TImpProxy<UInt_t>     TUIntProxy;
   typedef TImpProxy<ULong_t>    TULongProxy;
   typedef TImpProxy<ULong64_t>  TULong64Proxy;
   typedef TImpProxy<UShort_t>   TUShortProxy;
   typedef TImpProxy<UChar_t>    TUCharProxy;
   typedef TImpProxy<Int_t>      TIntProxy;
   typedef TImpProxy<Long_t>     TLongProxy;
   typedef TImpProxy<Long64_t>   TLong64Proxy;
   typedef TImpProxy<Short_t>    TShortProxy;
   typedef TImpProxy<Char_t>     TCharProxy;
   typedef TImpProxy<Bool_t>     TBoolProxy;

   typedef TArrayProxy<TArrayType<Double_t> >   TArrayDoubleProxy;
   typedef TArrayProxy<TArrayType<Double32_t> > TArrayDouble32Proxy;
   typedef TArrayProxy<TArrayType<Float_t> >    TArrayFloatProxy;
   typedef TArrayProxy<TArrayType<UInt_t> >     TArrayUIntProxy;
   typedef TArrayProxy<TArrayType<ULong_t> >    TArrayULongProxy;
   typedef TArrayProxy<TArrayType<ULong64_t> >  TArrayULong64Proxy;
   typedef TArrayProxy<TArrayType<UShort_t> >   TArrayUShortProxy;
   typedef TArrayProxy<TArrayType<UChar_t> >    TArrayUCharProxy;
   typedef TArrayProxy<TArrayType<Int_t> >      TArrayIntProxy;
   typedef TArrayProxy<TArrayType<Long_t> >     TArrayLongProxy;
   typedef TArrayProxy<TArrayType<Long64_t> >   TArrayLong64Proxy;
   typedef TArrayProxy<TArrayType<UShort_t> >   TArrayShortProxy;
   //specialized ! typedef TArrayProxy<TArrayType<Char_t> >  TArrayCharProxy;
   typedef TArrayProxy<TArrayType<Bool_t> >     TArrayBoolProxy;

   typedef TClaImpProxy<Double_t>   TClaDoubleProxy;
   typedef TClaImpProxy<Double32_t> TClaDouble32Proxy;
   typedef TClaImpProxy<Float_t>    TClaFloatProxy;
   typedef TClaImpProxy<UInt_t>     TClaUIntProxy;
   typedef TClaImpProxy<ULong_t>    TClaULongProxy;
   typedef TClaImpProxy<ULong64_t>  TClaULong64Proxy;
   typedef TClaImpProxy<UShort_t>   TClaUShortProxy;
   typedef TClaImpProxy<UChar_t>    TClaUCharProxy;
   typedef TClaImpProxy<Int_t>      TClaIntProxy;
   typedef TClaImpProxy<Long_t>     TClaLongProxy;
   typedef TClaImpProxy<Long64_t>   TClaLong64Proxy;
   typedef TClaImpProxy<Short_t>    TClaShortProxy;
   typedef TClaImpProxy<Char_t>     TClaCharProxy;
   typedef TClaImpProxy<Bool_t>     TClaBoolProxy;

   typedef TClaArrayProxy<TArrayType<Double_t> >    TClaArrayDoubleProxy;
   typedef TClaArrayProxy<TArrayType<Double32_t> >  TClaArrayDouble32Proxy;
   typedef TClaArrayProxy<TArrayType<Float_t> >     TClaArrayFloatProxy;
   typedef TClaArrayProxy<TArrayType<UInt_t> >      TClaArrayUIntProxy;
   typedef TClaArrayProxy<TArrayType<ULong_t> >     TClaArrayULongProxy;
   typedef TClaArrayProxy<TArrayType<ULong64_t> >   TClaArrayULong64Proxy;
   typedef TClaArrayProxy<TArrayType<UShort_t> >    TClaArrayUShortProxy;
   typedef TClaArrayProxy<TArrayType<UChar_t> >     TClaArrayUCharProxy;
   typedef TClaArrayProxy<TArrayType<Int_t> >       TClaArrayIntProxy;
   typedef TClaArrayProxy<TArrayType<Long_t> >      TClaArrayLongProxy;
   typedef TClaArrayProxy<TArrayType<Long64_t> >    TClaArrayLong64Proxy;
   typedef TClaArrayProxy<TArrayType<UShort_t> >    TClaArrayShortProxy;
   typedef TClaArrayProxy<TArrayType<Char_t> >      TClaArrayCharProxy;
   typedef TClaArrayProxy<TArrayType<Bool_t> >      TClaArrayBoolProxy;
   //specialized ! typedef TClaArrayProxy<TArrayType<Char_t> >  TClaArrayCharProxy;

   typedef TStlImpProxy<Double_t>   TStlDoubleProxy;
   typedef TStlImpProxy<Double32_t> TStlDouble32Proxy;
   typedef TStlImpProxy<Float_t>    TStlFloatProxy;
   typedef TStlImpProxy<UInt_t>     TStlUIntProxy;
   typedef TStlImpProxy<ULong_t>    TStlULongProxy;
   typedef TStlImpProxy<ULong64_t>  TStlULong64Proxy;
   typedef TStlImpProxy<UShort_t>   TStlUShortProxy;
   typedef TStlImpProxy<UChar_t>    TStlUCharProxy;
   typedef TStlImpProxy<Int_t>      TStlIntProxy;
   typedef TStlImpProxy<Long_t>     TStlLongProxy;
   typedef TStlImpProxy<Long64_t>   TStlLong64Proxy;
   typedef TStlImpProxy<Short_t>    TStlShortProxy;
   typedef TStlImpProxy<Char_t>     TStlCharProxy;
   typedef TStlImpProxy<Bool_t>     TStlBoolProxy;

   typedef TStlArrayProxy<TArrayType<Double_t> >    TStlArrayDoubleProxy;
   typedef TStlArrayProxy<TArrayType<Double32_t> >  TStlArrayDouble32Proxy;
   typedef TStlArrayProxy<TArrayType<Float_t> >     TStlArrayFloatProxy;
   typedef TStlArrayProxy<TArrayType<UInt_t> >      TStlArrayUIntProxy;
   typedef TStlArrayProxy<TArrayType<ULong_t> >     TStlArrayULongProxy;
   typedef TStlArrayProxy<TArrayType<ULong64_t> >   TStlArrayULong64Proxy;
   typedef TStlArrayProxy<TArrayType<UShort_t> >    TStlArrayUShortProxy;
   typedef TStlArrayProxy<TArrayType<UChar_t> >     TStlArrayUCharProxy;
   typedef TStlArrayProxy<TArrayType<Int_t> >       TStlArrayIntProxy;
   typedef TStlArrayProxy<TArrayType<Long_t> >      TStlArrayLongProxy;
   typedef TStlArrayProxy<TArrayType<Long64_t> >    TStlArrayLong64Proxy;
   typedef TStlArrayProxy<TArrayType<UShort_t> >    TStlArrayShortProxy;
   typedef TStlArrayProxy<TArrayType<Char_t> >      TStlArrayCharProxy;
   typedef TStlArrayProxy<TArrayType<Bool_t> >      TStlArrayBoolProxy;
   //specialized ! typedef TStlArrayProxy<TArrayType<Char_t> >  TStlArrayCharProxy;

} // namespace ROOT

#endif

