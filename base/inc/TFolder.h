// @(#)root/base:$Name:  $:$Id: TFolder.h,v 1.4 2000/09/06 09:29:20 brun Exp $
// Author: Rene Brun   02/09/2000

/*************************************************************************
 * Copyright (C) 1995-2000, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#ifndef ROOT_TFolder
#define ROOT_TFolder


//////////////////////////////////////////////////////////////////////////
//                                                                      //
// TFolder                                                              //
//                                                                      //
// Describe a folder: a list of objects and folders                     //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

#ifndef ROOT_TNamed
#include "TNamed.h"
#endif

class TBrowser;

class TFolder : public TNamed {

protected:
   TCollection       *fFolders;        //pointer to the list of folders
   Bool_t             fIsOwner;        //true if folder own its contained objects
   
private:
   TFolder(const TFolder &folder);  //folders cannot be copied
   void operator=(const TFolder &);

public:

   TFolder();
   virtual ~TFolder();
   virtual void        Add(TObject *obj);
   TFolder            *AddFolder(const char *name, const char *title, TCollection *collection=0);
   virtual void        Browse(TBrowser *b);
   virtual void        Clear(Option_t *option="");
   virtual void        Copy(TObject &) { MayNotUse("Copy(TObject &)"); }
   virtual TObject    *FindObject(const char *name) const;
   virtual TObject    *FindObject(TObject *obj) const;
   virtual TObject    *FindObjectAny(const char *name) const;
   TCollection        *GetListOfFolders() const { return fFolders; }
   virtual const char *GetPath() const;
   Bool_t              IsFolder() const { return kTRUE; }
   Bool_t              IsOwner()  const { return fIsOwner; }
   virtual void        ls(Option_t *option="");  // *MENU*
   virtual void        RecursiveRemove(TObject *obj);
   virtual void        Remove(TObject *obj);
   virtual void        SetOwner(Bool_t owner=kTRUE) {fIsOwner = owner;}
   
   ClassDef(TFolder,1)  //Describe a folder: a list of objects and folders
};

#endif

