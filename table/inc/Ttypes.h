/* @(#)root/star:$Name:  $:$Id: Ttypes.h,v 1.12 2002/05/16 06:24:17 brun Exp $ */

/*************************************************************************
 * Copyright (C) 1995-2000, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#ifndef ROOT_Ttypes
#define ROOT_Ttypes

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// Stypes                                                               //
// $Id: Ttypes.h,v 1.12 2002/05/16 06:24:17 brun Exp $
// Basic types used by STAF - ROOT interface.                           //
//                                                                      //
// This header file contains the set of the macro definitions           //
// to generate a ROOT dictionary for "pure" C-strucutre the way ROOT    //
// does it for the "normal" C++ classes                                 //
//                                                                      //
//////////////////////////////////////////////////////////////////////////


#include "Rtypes.h"

#ifdef ANSICPP
#   define _QUOTE2_(name1,name2) _QUOTE_(name1##name2)
#else
#   define _QUOTE2_(name1,name2) _QUOTE_(_NAME1_(name1)name2)
#endif

//___________________________________________________________________
#define _TableClassInit_(className,structName)                      \
   extern void AddClass(const char *cname, Version_t id, VoidFuncPtr_t dict, Int_t); \
   extern void RemoveClass(const char *cname);                      \
   class _NAME2_(R__Init,className) {                               \
      public:                                                       \
         _NAME2_(R__Init,className)() {                             \
            AddClass(_QUOTE_(className), className::Class_Version(),\
                     &className::Dictionary, 0);                       \
         }                                                          \
         ~_NAME2_(R__Init,className)() {                            \
            RemoveClass(_QUOTE_(className));                        \
            RemoveClass(_QUOTE_(structName));                       \
         }                                                          \
   };

//___________________________________________________________________
#define _TableClassImp_(className,structName)                       \


//   _TableClassInit_(className,structName)

//___________________________________________________________________
#define TableClassStreamerImp(className)                            \
void className::Streamer(TBuffer &R__b) {                           \
   TTable::Streamer(R__b); }

#if 0
   void className::Dictionary()                                     \
   {                                                                \
      TClass *c = CreateClass(_QUOTE_(className), Class_Version(),  \
                              DeclFileName(), ImplFileName(),       \
                              DeclFileLine(), ImplFileLine());      \
      className::TableDictionary();                                 \
   }                                                                \

#endif

//___________________________________________________________________
#define TableClassImp(className,structName)                         \
   const char* className::TableDictionary() {                       \
      char *structBuf = new char[strlen(_QUOTE2_(structName,.h))+2];\
      strcpy(structBuf,_QUOTE2_(structName,.h));                    \
      char *s = strstr(structBuf,"_st.h");                          \
      if (s) { *s = 0;  strcat(structBuf,".h"); }                   \
      TClass *r = ROOT::CreateClass(_QUOTE_(structName),            \
                                    Class_Version(), structBuf,     \
                                    structBuf, 1,  1 );             \
      fgColDescriptors = new TTableDescriptor(r);                   \
      return _QUOTE_(structName);                                   \
   }                                                                \
   _TableClassImp_(className,structName)

//___________________________________________________________________
#define TableClassImpl(className,structName)                        \
  TTableDescriptor *className::fgColDescriptors = 0;                \
  TableClassImp(className,structName)                               \
  TableClassStreamerImp(className)


#define TableImpl(name)                                            \
  TTableDescriptor *_NAME2_(St_,name)::fgColDescriptors = 0;       \
  TableClassImp(_NAME2_(St_,name), _NAME2_(name,_st))              \
  TableClassStreamerImp(_NAME2_(St_,name))

#define TableImp(name)  TableClassImp(_NAME2_(St_,name),_QUOTE2_(St_,name))

#define ClassDefTable(className,structName)         \
  public:                                           \
     static const char* TableDictionary();          \
  protected:                                        \
     static TTableDescriptor *fgColDescriptors;     \
     virtual TTableDescriptor *GetDescriptorPointer() const { return fgColDescriptors;}                 \
virtual void SetDescriptorPointer(TTableDescriptor *list)  { fgColDescriptors = list;}                  \
  public:                                           \
    typedef structName* iterator;                   \
    className() : TTable(_QUOTE_(className),sizeof(structName))    {SetType(_QUOTE_(structName));}      \
    className(const Text_t *name) : TTable(name,sizeof(structName)) {SetType(_QUOTE_(structName));}     \
    className(Int_t n) : TTable(_QUOTE_(className),n,sizeof(structName)) {SetType(_QUOTE_(structName));}\
    className(const Text_t *name,Int_t n) : TTable(name,n,sizeof(structName)) {SetType(_QUOTE_(structName));}\
    structName *GetTable(Int_t i=0) const { return ((structName *)GetArray())+i;}                       \
    structName &operator[](Int_t i){ assert(i>=0 && i < GetNRows()); return *GetTable(i); }             \
    const structName &operator[](Int_t i) const { assert(i>=0 && i < GetNRows()); return *((const structName *)(GetTable(i))); } \
    structName *begin() const  {                      return GetNRows()? GetTable(0):0;}\
    structName *end()   const  {Long_t i = GetNRows(); return          i? GetTable(i):0;}

// -- The member function "begin()" returns a pointer to the first table row
//   (or just zero if the table is empty).
// -- The member function "end()" returns a pointer to the last+1 table row
//   (or just zero if the table is empty).

//  protected:
//    _NAME2_(className,C)() : TChair() {;}
//  public:
//    _NAME2_(className,C)(className *tableClass) : TChair(tableClass) {;}

#define ClassDefChair(className,structName)             \
  public:                                               \
    typedef structName* iterator;                       \
    structName *GetTable(Int_t i) const  {              \
              if (fLastIndx != UInt_t(i)) {             \
                ((_NAME2_(className,C) *)this)->fLastIndx = i;        \
                ((_NAME2_(className,C) *)this)->fLastRow =            \
                  ((className *)GetThisTable())->GetTable(i);              \
           }; return (structName *)fLastRow; }          \
    structName &operator[](Int_t i){ assert(i>=0 && i < GetNRows()); return *GetTable(i); }    \
    const structName &operator[](Int_t i) const { assert(i>=0 && i < GetNRows()); return *((const structName *)(GetTable(i))); }\
    structName *begin() const  {                      return GetNRows()? GetTable(0):0;}\
    structName *end()   const  {Int_t i = GetNRows(); return          i? GetTable(i):0;}


namespace ROOT {
   template <class T> class TTableInitBehavior: public TDefaultInitBehavior {
   public:
      static const char* fgStructName; // Need to be instantiated
      virtual TClass* CreateClass(const char *cname, Version_t id,
                                  const type_info &info, IsAFunc_t isa,
                                  ShowMembersFunc_t show,
                                  const char *dfil, const char *ifil,
                                  Int_t dl, Int_t il) const {
         TClass * cl = TDefaultInitBehavior::CreateClass(cname, id, info, isa, show, 
                                                              dfil, ifil,dl, il);
         fgStructName = T::TableDictionary();
         return cl;
      }
      virtual void Unregister(const char* classname) const {
         TDefaultInitBehavior::Unregister(classname);
         TDefaultInitBehavior::Unregister(fgStructName);
      }
   };
   template <class T> const char * TTableInitBehavior<T >::fgStructName = 0;
}

class TTable;
namespace ROOT {
   template <class RootClass> 
      const ROOT::TTableInitBehavior<RootClass>* DefineBehavior( TTable*, RootClass*) 
      {
         static ROOT::TTableInitBehavior<RootClass> behave;
         return &behave;
      }
}

#endif
