// @(#)root/base:$Id$
// Author: Rene Brun   02/09/2000

/*************************************************************************
 * Copyright (C) 1995-2000, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/


//______________________________________________________________________________
//
// A TFolder object is a collection of objects and folders.
// Folders have a name and a title and are identified in the folder hierarchy
// by a "Unix-like" naming mechanism. The root of all folders is //root.
// New folders can be dynamically added or removed to/from a folder.
// The folder hierarchy can be visualized via the TBrowser.
//
// The Root folders hierarchy can be seen as a whiteboard where objects
// are posted. Other classes/tasks can access these objects by specifying
// only a string pathname. This whiteboard facility greatly improves the
// modularity of an application, minimizing the class relationship problem
// that penalizes large applications.
//
// Pointers are efficient to communicate between classes.
// However, one has interest to minimize direct coupling between classes
// in the form of direct pointers. One better uses the naming and search
// service provided by the Root folders hierarchy. This makes the classes
// loosely coupled and also greatly facilitates I/O operations.
// In a client/server environment, this mechanism facilitates the access
// to any kind of object in //root stores running on different processes.
//
// A TFolder is created by invoking the TFolder constructor. It is placed
// inside an existing folder via the TFolder::AddFolder method.
// One can search for a folder or an object in a folder using the FindObject
// method. FindObject analyzes the string passed as its argument and searches
// in the hierarchy until it finds an object or folder matching the name.
//
// When a folder is deleted, its reference from the parent folder and
// possible other folders is deleted.
//
// If a folder has been declared the owner of its objects/folders via
// TFolder::SetOwner, then the contained objects are deleted when the
// folder is deleted. By default, a folder does not own its contained objects.
// NOTE that folder ownership can be set
//   - via TFolder::SetOwner
//   - or via TCollection::SetOwner on the collection specified to TFolder::AddFolder
//
// Standard Root objects are automatically added to the folder hierarchy.
// For example, the following folders exist:
//   //root/Files      with the list of currently connected Root files
//   //root/Classes    with the list of active classes
//   //root/Geometries with active geometries
//   //root/Canvases   with the list of active canvases
//   //root/Styles     with the list of graphics styles
//   //root/Colors     with the list of active colors
//
// For example, if a file "myFile.root" is added to the list of files, one can
// retrieve a pointer to the corresponding TFile object with a statement like:
//   TFile *myFile = (TFile*)gROOT->FindObject("//root/Files/myFile.root");
// The above statement can be abbreviated to:
//   TFile *myFile = (TFile*)gROOT->FindObject("/Files/myFile.root");
// or even to:
//   TFile *myFile = (TFile*)gROOT->FindObjectAny("myFile.root");
// In this last case, the TROOT::FindObjectAny function will scan the folder hierarchy
// starting at //root and will return the first object named "myFile.root".
//
// Because a string-based search mechanism is expensive, it is recommended
// to save the pointer to the object as a class member or local variable
// if this pointer is used frequently or inside loops.
//
//Begin_Html
/*
<img src="gif/folder.gif">
*/
//End_Html

#include "Riostream.h"
#include "Strlen.h"
#include "TFolder.h"
#include "TBrowser.h"
#include "TROOT.h"
#include "TClass.h"
#include "TError.h"
#include "TRegexp.h"

static const char *gFolderD[64];
static Int_t gFolderLevel = -1;
static char  gFolderPath[512];

ClassImp(TFolder)

//______________________________________________________________________________
TFolder::TFolder() : TNamed()
{
   // Default constructor used by the Input functions.
   //
   // This constructor should not be called by a user directly.
   // The normal way to create a folder is by calling TFolder::AddFolder.

   fFolders = 0;
   fIsOwner = kFALSE;
}

//______________________________________________________________________________
TFolder::TFolder(const char *name, const char *title) : TNamed(name,title)
{
   // Create a normal folder.
   // Use Add or AddFolder to add objects or folders to this folder.

   fFolders = new TList();
   fIsOwner = kFALSE;
}

//______________________________________________________________________________
TFolder::TFolder(const TFolder &folder) : TNamed(folder),fFolders(0),fIsOwner(kFALSE)
{
   // Copy constructor.

   ((TFolder&)folder).Copy(*this);
}

//______________________________________________________________________________
TFolder::~TFolder()
{
   // Folder destructor. Remove all objects from its lists and delete
   // all its sub folders.

   TCollection::StartGarbageCollection();

   if (fFolders) {
      fFolders->Clear();
      SafeDelete(fFolders);
   }

   TCollection::EmptyGarbageCollection();

   if (gDebug)
      cerr << "TFolder dtor called for "<< GetName() << endl;
}

//______________________________________________________________________________
void TFolder::Add(TObject *obj)
{
   // Add object to this folder. obj must be a TObject or a TFolder.

   if (obj == 0 || fFolders == 0) return;
   obj->SetBit(kMustCleanup);
   fFolders->Add(obj);
}

//______________________________________________________________________________
TFolder *TFolder::AddFolder(const char *name, const char *title, TCollection *collection)
{
   // Create a new folder and add it to the list of folders of this folder,
   // return a pointer to the created folder.
   // Note that a folder can be added to several folders.
   //
   // If collection is non NULL, the pointer fFolders is set to the existing
   // collection, otherwise a default collection (Tlist) is created.
   // Note that the folder name cannot contain slashes.

   if (strchr(name,'/')) {
      ::Error("TFolder::TFolder","folder name cannot contain a slash: %s", name);
      return 0;
   }
   if (strlen(GetName()) == 0) {
      ::Error("TFolder::TFolder","folder name cannot be \"\"");
      return 0;
   }
   TFolder *folder = new TFolder();
   folder->SetName(name);
   folder->SetTitle(title);
   if (!fFolders) fFolders = new TList(); //only true when gROOT creates its 1st folder
   fFolders->Add(folder);

   if (collection) folder->fFolders = collection;
   else            folder->fFolders = new TList();
   return folder;
}

//______________________________________________________________________________
void TFolder::Browse(TBrowser *b)
{
   // Browse this folder.

   if (fFolders) fFolders->Browse(b);
}

//______________________________________________________________________________
void TFolder::Clear(Option_t *option)
{
   // Delete all objects from a folder list.

   if (fFolders) fFolders->Clear(option);
}

//______________________________________________________________________________
const char *TFolder::FindFullPathName(const char *name) const
{
   // Return the full pathname corresponding to subpath name.
   // The returned path will be re-used by the next call to GetPath().

   TObject *obj = FindObject(name);
   if (obj || !fFolders) {
      gFolderLevel++;
      gFolderD[gFolderLevel] = GetName();
      gFolderPath[0] = '/';
      gFolderPath[1] = 0;
      for (Int_t l=0;l<=gFolderLevel;l++) {
         strlcat(gFolderPath, "/", sizeof(gFolderPath));
         strlcat(gFolderPath, gFolderD[l], sizeof(gFolderPath));
      }
      strlcat(gFolderPath, "/", sizeof(gFolderPath));
      strlcat(gFolderPath,name, sizeof(gFolderPath));
      gFolderLevel = -1;
      return gFolderPath;
   }
   if (name[0] == '/') return 0;
   TIter next(fFolders);
   TFolder *folder;
   const char *found;
   gFolderLevel++;
   gFolderD[gFolderLevel] = GetName();
   while ((obj=next())) {
      if (!obj->InheritsFrom(TFolder::Class())) continue;
      if (obj->InheritsFrom(TClass::Class())) continue;
      folder = (TFolder*)obj;
      found = folder->FindFullPathName(name);
      if (found) return found;
   }
   gFolderLevel--;
   return 0;
}


//______________________________________________________________________________
const char *TFolder::FindFullPathName(const TObject *) const
{
   // Return the full pathname corresponding to subpath name.
   // The returned path will be re-used by the next call to GetPath().

   Error("FindFullPathname","Not yet implemented");
   return 0;
}

//______________________________________________________________________________
TObject *TFolder::FindObject(const TObject *) const
{
   // Find object in an folder.

   Error("FindObject","Not yet implemented");
   return 0;
}

//______________________________________________________________________________
TObject *TFolder::FindObject(const char *name) const
{
   // Search object identified by name in the tree of folders inside
   // this folder.
   // Name may be of the forms:
   //   A, Specify a full pathname starting at the top ROOT folder
   //     //root/xxx/yyy/name
   //
   //   B, Specify a pathname starting with a single slash. //root is assumed
   //     /xxx/yyy/name
   //
   //   C, Specify a pathname relative to this folder
   //     xxx/yyy/name
   //     name

   if (!fFolders) return 0;
   if (name == 0) return 0;
   if (name[0] == '/') {
      if (name[1] == '/') {
         if (!strstr(name,"//root/")) return 0;
         return gROOT->GetRootFolder()->FindObject(name+7);
      } else {
         return gROOT->GetRootFolder()->FindObject(name+1);
      }
   }
   Int_t nch = strlen(name);
   char *cname;
   char csname[128];
   if (nch < (int)sizeof(csname))
      cname = csname;
   else
      cname = new char[nch+1];
   strcpy(cname, name);
   TObject *obj;
   char *slash = strchr(cname,'/');
   if (slash) {
      *slash = 0;
      obj = fFolders->FindObject(cname);
      if (!obj) {
         if (nch >= (int)sizeof(csname)) delete [] cname;
         return 0;
      }
      TObject *ret = obj->FindObject(slash+1);
      if (nch >= (int)sizeof(csname)) delete [] cname;
      return ret;
   } else {
      TObject *ret = fFolders->FindObject(cname);
      if (nch >= (int)sizeof(csname)) delete [] cname;
      return ret;
   }
}

//______________________________________________________________________________
TObject *TFolder::FindObjectAny(const char *name) const
{
   // Return a pointer to the first object with name starting at this folder.

   TObject *obj = FindObject(name);
   if (obj || !fFolders) return obj;

   //if (!obj->InheritsFrom(TFolder::Class())) continue;
   if (name[0] == '/') return 0;
   TIter next(fFolders);
   TFolder *folder;
   TObject *found;
   if (gFolderLevel >= 0) gFolderD[gFolderLevel] = GetName();
   while ((obj=next())) {
      if (!obj->InheritsFrom(TFolder::Class())) continue;
      if (obj->IsA() == TClass::Class()) continue;
      folder = (TFolder*)obj;
      found = folder->FindObjectAny(name);
      if (found) return found;
   }
   return 0;
}

//______________________________________________________________________________
Bool_t TFolder::IsOwner()  const
{
   // Folder ownership has been set via
   //   - TFolder::SetOwner
   //   - TCollection::SetOwner on the collection specified to TFolder::AddFolder

   if (!fFolders) return kFALSE;
   return fFolders->IsOwner();
}

//______________________________________________________________________________
void TFolder::ls(Option_t *option) const
{
   // List folder contents
   //   If option contains "dump",  the Dump function of contained objects is called.
   //   If option contains "print", the Print function of contained objects is called.
   //   By default the ls function of contained objects is called.
   // Indentation is used to identify the folder tree.
   //
   // The if option contains a <regexp> it be used to match the name of the objects.

   if (!fFolders) return;
   TROOT::IndentLevel();
   cout <<ClassName()<<"*\t\t"<<GetName()<<"\t"<<GetTitle()<<endl;
   TROOT::IncreaseDirLevel();

   TString opt = option;
   Ssiz_t dump = opt.Index("dump", 0, TString::kIgnoreCase);
   if (dump != kNPOS)
      opt.Remove(dump, 4);
   Ssiz_t print = opt.Index("print", 0, TString::kIgnoreCase);
   if (print != kNPOS)
      opt.Remove(print, 5);
   opt = opt.Strip(TString::kBoth);
   if (opt == "")
      opt = "*";
   TRegexp re(opt, kTRUE);

   TObject *obj;
   TIter nextobj(fFolders);
   while ((obj = (TObject *) nextobj())) {
      TString s = obj->GetName();
      if (s.Index(re) == kNPOS) continue;
      if (dump != kNPOS)
         obj->Dump();
      if (print != kNPOS)
         obj->Print(option);
      obj->ls(option);
   }
   TROOT::DecreaseDirLevel();
}

//______________________________________________________________________________
Int_t TFolder::Occurence(const TObject *object) const
{
   // Return occurence number of object in the list of objects of this folder.
   // The function returns the number of objects with the same name as object
   // found in the list of objects in this folder before object itself.
   // If only one object is found, return 0.

   Int_t n = 0;
   if (!fFolders) return 0;
   TIter next(fFolders);
   TObject *obj;
   while ((obj=next())) {
      if (strcmp(obj->GetName(),object->GetName()) == 0) n++;
   }
   if (n <=1) return n-1;
   n = 0;
   next.Reset();
   while ((obj=next())) {
      if (strcmp(obj->GetName(),object->GetName()) == 0) n++;
      if (obj == object) return n;
   }
   return 0;
}

//______________________________________________________________________________
void TFolder::RecursiveRemove(TObject *obj)
{
   // Recursively remove object from a folder.

   if (fFolders) fFolders->RecursiveRemove(obj);
}

//______________________________________________________________________________
void TFolder::Remove(TObject *obj)
{
   // Remove object from this folder. obj must be a TObject or a TFolder.

   if (obj == 0 || fFolders == 0) return;
   fFolders->Remove(obj);
}

//______________________________________________________________________________
void TFolder::SaveAs(const char *filename, Option_t *option) const
{
   // Save all objects in this folder in filename.
   // Each object in this folder will have a key in the file where the name of
   // the key will be the name of the object.

   if (gDirectory) gDirectory->SaveObjectAs(this,filename,option);
}

//______________________________________________________________________________
void TFolder::SetOwner(Bool_t owner)
{
   // Set ownership.
   // If the folder is declared owner, when the folder is deleted, all
   // the objects added via TFolder::Add are deleted via TObject::Delete,
   // otherwise TObject::Clear is called.
   //
   // NOTE that folder ownership can be set:
   //   - via TFolder::SetOwner
   //   - or via TCollection::SetOwner on the collection specified to TFolder::AddFolder

   if (!fFolders) fFolders = new TList();
   fFolders->SetOwner(owner);
}
