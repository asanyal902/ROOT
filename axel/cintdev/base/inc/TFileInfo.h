// @(#)root/base:$Id$
// Author: Andreas-Joachim Peters   20/9/2005

/*************************************************************************
 * Copyright (C) 1995-2005, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#ifndef ROOT_TFileInfo
#define ROOT_TFileInfo

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// TFileInfo                                                            //
//                                                                      //
// Class describing a generic file including meta information.          //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

#ifndef ROOT_TNamed
#include "TNamed.h"
#endif

#ifndef ROOT_Turl
#include "TUrl.h"
#endif

#ifndef ROOT_TUUID
#include "TUUID.h"
#endif

#ifndef ROOT_TMD5
#include "TMD5.h"
#endif

#ifndef ROOT_TObjString
#include "TObjString.h"
#endif

#ifndef ROOT_TList
#include "TList.h"
#endif

class TFileInfoMeta;


class TFileInfo : public TNamed {

private:
   TUrl            *fCurrentUrl;     //! current URL to access the file, points to URL in the fUrlList or 0, if the list end is reached
   TList           *fUrlList;        // list of file URLs
   Long64_t         fSize;           // file size
   TUUID           *fUUID;           //-> uuid of the referenced file
   TMD5            *fMD5;            //-> md5 digest of the file
   TList           *fMetaDataList;   // generic list of file meta data object(s)

   TFileInfo(const TFileInfo&);             // not implemented
   TFileInfo& operator=(const TFileInfo&);  // not implemented

public:
   enum EStatusBits {
      kStaged    = BIT(15),
      kCorrupted = BIT(16)
   };

   TFileInfo(const char *url = 0, Long64_t size = -1, const char *uuid = 0,
             const char *md5 = 0, TObject *meta = 0);

   virtual ~TFileInfo();

   void            ResetUrl() { fCurrentUrl = (TUrl*)fUrlList->First(); }
   TUrl           *NextUrl();
   TUrl           *GetCurrentUrl() const;
   TUrl           *GetFirstUrl() const { return (TUrl*)fUrlList->First(); }

   Long64_t        GetSize() const         { return fSize; }
   TUUID          *GetUUID() const         { return fUUID; }
   TMD5           *GetMD5() const          { return fMD5; }
   TList          *GetMetaDataList() const { return fMetaDataList; }
   TFileInfoMeta  *GetMetaData(const char *meta = 0) const;

   void            SetSize(Long64_t size)  { fSize = size; }

   TUrl           *FindByUrl(const char *url);

   Bool_t          AddUrl(const char *url);
   Bool_t          RemoveUrl(const char *url);
   Bool_t          AddMetaData(TObject *meta);
   Bool_t          RemoveMetaData(const char *meta = 0);

   Bool_t          IsSortable() const { return kTRUE; }
   Int_t           Compare(const TObject *obj) const;

   void            Print(Option_t *options="") const;

   ClassDef(TFileInfo,3)   // Describes generic file info including meta data information
};


class TFileInfoMeta : public TNamed {

private:
   Long64_t      fEntries;    // number of entries in tree or number of objects
   Long64_t      fFirst;      // first valid tree entry
   Long64_t      fLast;       // last valid tree entry
   Bool_t        fIsTree;     // true if type is a TTree (or TTree derived)

   TFileInfoMeta(const TFileInfoMeta&);             // not implemented
   TFileInfoMeta& operator=(const TFileInfoMeta&);  // not implemented

public:
   TFileInfoMeta() : fEntries(-1), fFirst(0), fLast(-1), fIsTree(kFALSE) { }
   TFileInfoMeta(const char *objPath, const char *objClass = "TTree",
                 Long64_t entries = -1, Long64_t first = 0, Long64_t last = -1);
   TFileInfoMeta(const char *objPath, const char *objDir,
                 const char *objClass, Long64_t entries = -1,
                 Long64_t first = 0, Long64_t last = -1);

   virtual ~TFileInfoMeta() { }

   const char     *GetObject() const;
   const char     *GetClass() const        { return GetTitle(); }
   const char     *GetDirectory() const;
   Long64_t        GetEntries() const      { return fEntries; }
   Long64_t        GetFirst() const        { return fFirst; }
   Long64_t        GetLast() const         { return fLast; }
   Bool_t          IsTree() const          { return fIsTree; }

   void            SetEntries(Long64_t entries) { fEntries = entries; }
   void            SetFirst(Long64_t first)     { fFirst = first; }
   void            SetLast(Long64_t last)       { fLast = last; }

   void            Print(Option_t *options="") const;

   ClassDef(TFileInfoMeta,1)   // Describes TFileInfo meta data
};

#endif
