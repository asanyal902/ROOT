// @(#)root/gui:$Id$
// Author: Bertrand Bellenot   26/09/2007

/*************************************************************************
 * Copyright (C) 1995-2007, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#include "TROOT.h"
#include "TSystem.h"
#include "TApplication.h"
#include "TGClient.h"
#include "TGListTree.h"
#include "TGLayout.h"
#include "TGComboBox.h"
#include "TContextMenu.h"
#include "TGTextEntry.h"
#include "TGTab.h"
#include "TGLabel.h"
#include "TSystemDirectory.h"
#include "TGMimeTypes.h"
#include "TClass.h"
#include "TQClass.h"
#include "TDataMember.h"
#include "TMethod.h"
#include "TMethodArg.h"
#include "TRealData.h"
#include "TInterpreter.h"
#include "TRegexp.h"
#include "TEnv.h"
#include "TImage.h"
#include "TBrowser.h"
#include "TRemoteObject.h"
#include "Getline.h"
#include <time.h>
#include <string.h>

#include "TGFileBrowser.h"
#include "TRootBrowser.h"

#ifdef WIN32
const char rootdir[] = "\\";
#else
const char rootdir[] = "/";
#endif

const char *filters[] = {
   "",
   "*.*",
   "*.[C|c|h]*",
   "*.root",
   "*.txt"
};

//_____________________________________________________________________________
//
// TCursorSwitcher
//
// Helper class used to change the cursor in a method and restore the 
// original one when going out of the method scope.
//_____________________________________________________________________________

///////////////////////////////////////////////////////////////////////////////
class TCursorSwitcher {
private:
   TGWindow *fW1;
   TGWindow *fW2;
public:
   TCursorSwitcher(TGWindow *w1, TGWindow *w2) : fW1(w1), fW2(w2) {
      if (w1) gVirtualX->SetCursor(w1->GetId(), gVirtualX->CreateCursor(kWatch));
      if (w2) gVirtualX->SetCursor(w2->GetId(), gVirtualX->CreateCursor(kWatch));
   }
   ~TCursorSwitcher() {
      if (fW1) gVirtualX->SetCursor(fW1->GetId(), gVirtualX->CreateCursor(kPointer));
      if (fW2) gVirtualX->SetCursor(fW2->GetId(), gVirtualX->CreateCursor(kPointer));
   }
};

//_____________________________________________________________________________
//
// TGFileBrowser
//
// System file browser, used as TRootBrowser plug-in.
// This class is the real core of the ROOT browser.
//_____________________________________________________________________________

ClassImp(TGFileBrowser)

//______________________________________________________________________________
TGFileBrowser::TGFileBrowser(const TGWindow *p, TBrowser* b, UInt_t w, UInt_t h)
   : TGMainFrame(p, w, h), TBrowserImp(b), fNewBrowser(0)
{
   // TGFileBrowser constructor.

   if (p && p != gClient->GetDefaultRoot())
      fNewBrowser = (TRootBrowser *)p->GetMainFrame();
   if (fNewBrowser)
      fNewBrowser->SetActBrowser(this);
   CreateBrowser();
   Resize(w, h);
   if (fBrowser) Show();
}

//______________________________________________________________________________
void TGFileBrowser::CreateBrowser()
{
   // Create the actual file browser.

   fCachedPic  = 0;
   SetCleanup(kDeepCleanup);

   fTopFrame = new TGHorizontalFrame(this, 100, 30);
   fDrawOption = new TGComboBox(fTopFrame, "");
   TGTextEntry *dropt_entry = fDrawOption->GetTextEntry();
   dropt_entry->SetToolTipText("Object Draw Option", 300);
   fDrawOption->Resize(80, 20);
   TGListBox *lb = fDrawOption->GetListBox();
   lb->Resize(lb->GetWidth(), 120);
   Int_t dropt = 1;
   fDrawOption->AddEntry("", dropt++);
   fDrawOption->AddEntry(" alp", dropt++);
   fDrawOption->AddEntry(" box", dropt++);
   fDrawOption->AddEntry(" colz", dropt++);
   fDrawOption->AddEntry(" lego", dropt++);
   fDrawOption->AddEntry(" lego1", dropt++);
   fDrawOption->AddEntry(" lego2", dropt++);
   fDrawOption->AddEntry(" same", dropt++);
   fDrawOption->AddEntry(" surf", dropt++);
   fDrawOption->AddEntry(" surf1", dropt++);
   fDrawOption->AddEntry(" surf2", dropt++);
   fDrawOption->AddEntry(" surf3", dropt++);
   fDrawOption->AddEntry(" surf4", dropt++);
   fDrawOption->AddEntry(" surf5", dropt++);
   fDrawOption->AddEntry(" text", dropt++);
   fTopFrame->AddFrame(fDrawOption, new TGLayoutHints(kLHintsCenterY |
                       kLHintsRight, 2, 2, 2, 2));
   fTopFrame->AddFrame(new TGLabel(fTopFrame, "Draw Option: "),
                       new TGLayoutHints(kLHintsCenterY | kLHintsRight,
                       2, 2, 2, 2));
   AddFrame(fTopFrame, new TGLayoutHints(kLHintsLeft | kLHintsTop |
            kLHintsExpandX, 2, 2, 2, 2));
   fCanvas   = new TGCanvas(this, 100, 100);
   fListTree = new TGListTree(fCanvas, kHorizontalFrame);
   AddFrame(fCanvas, new TGLayoutHints(kLHintsLeft | kLHintsTop |
                kLHintsExpandX | kLHintsExpandY));
   fListTree->Connect("DoubleClicked(TGListTreeItem *, Int_t)",
      "TGFileBrowser", this, "DoubleClicked(TGListTreeItem *, Int_t)");
   fListTree->Connect("Clicked(TGListTreeItem *, Int_t, Int_t, Int_t)",
      "TGFileBrowser", this, "Clicked(TGListTreeItem *, Int_t, Int_t, Int_t)");
   fListTree->Connect("Checked(TObject*, Bool_t)", "TGFileBrowser",
      this, "Checked(TObject*, Bool_t)");

   fRootIcon = gClient->GetPicture("rootdb_t.xpm");
   fFileIcon = gClient->GetPicture("doc_t.xpm");

   fBotFrame = new TGHorizontalFrame(this, 100, 30);
   fBotFrame->AddFrame(new TGLabel(fBotFrame, "Filter: "),
                       new TGLayoutHints(kLHintsCenterY | kLHintsLeft,
                       2, 2, 2, 2));
   fFileType = new TGComboBox(fBotFrame, " All Files (*.*)");
   Int_t ftype = 1;
   fFileType->AddEntry(" All Files (*.*)", ftype++);
   fFileType->AddEntry(" C/C++ Files (*.c;*.cxx;*.h;...)", ftype++);
   fFileType->AddEntry(" ROOT Files (*.root)", ftype++);
   fFileType->AddEntry(" Text Files (*.txt)", ftype++);
   fFileType->Resize(200, fFileType->GetTextEntry()->GetDefaultHeight());
   fBotFrame->AddFrame(fFileType, new TGLayoutHints(kLHintsLeft | kLHintsTop |
                kLHintsExpandX, 2, 2, 2, 2));
   fFileType->Connect("Selected(Int_t)", "TGFileBrowser", this, "ApplyFilter(Int_t)");
   AddFrame(fBotFrame, new TGLayoutHints(kLHintsLeft | kLHintsTop |
            kLHintsExpandX, 2, 2, 2, 2));

   fContextMenu = new TContextMenu("FileBrowserContextMenu") ;
   fFilter      = 0;
   fGroupSize   = 1000;
   fListLevel   = 0;
   fCurrentDir  = 0;
   fRootDir     = 0;
   fDir         = 0;
   fFile        = 0;
   TString gv = gEnv->GetValue("Browser.GroupView", "1000");
   Int_t igv = atoi(gv.Data());
   if (igv > 10)
      fGroupSize = igv;

   if (gEnv->GetValue("Browser.ShowHidden", 0))
      fShowHidden = kTRUE;
   else
      fShowHidden = kFALSE;

   TQObject::Connect("TGHtmlBrowser", "Clicked(char*)",
                     "TGFileBrowser", this, "Selected(char*)");
   fListLevel = 0;
   MapSubwindows();
   Resize(GetDefaultSize());
   MapWindow();
}

//______________________________________________________________________________
TGFileBrowser::~TGFileBrowser()
{
   // Destructor.

   delete fContextMenu;
   delete fListTree;
   if (fRootIcon) fClient->FreePicture(fRootIcon);
   if (fFileIcon) fClient->FreePicture(fFileIcon);
   if (fCachedPic) fClient->FreePicture(fCachedPic);
   Cleanup();
}


/**************************************************************************/
// TBrowserImp virtuals
/**************************************************************************/

//______________________________________________________________________________
void TGFileBrowser::Add(TObject *obj, const char *name, Int_t check)
{
   // Add items to the browser. This function has to be called
   // by the Browse() member function of objects when they are
   // called by a browser. If check < 0 (default) no check box is drawn,
   // if 0 then unchecked checkbox is added, if 1 checked checkbox is added.

   if (obj && obj->InheritsFrom("TApplication"))
      fListLevel = 0;
   if (obj && obj->InheritsFrom("TSystemDirectory"))
      return;
   const TGPicture *pic=0;
   if (obj && obj->InheritsFrom("TKey") && (obj->IsA() != TClass::Class()))
      AddKey(fListLevel, obj, name);
   else if (obj) {
      GetObjPicture(&pic, obj);
      if (!name) name = obj->GetName();
      if(check > -1) {
         if (!fListTree->FindChildByName(fListLevel, name)) {
            TGListTreeItem *item = fListTree->AddItem(fListLevel, name, obj, pic, pic, kTRUE);
            if ((pic != fFileIcon) && (pic != fCachedPic))
               fClient->FreePicture(pic);
            fListTree->CheckItem(item, (Bool_t)check);
            TString tip(obj->ClassName());
            if (obj->GetTitle()) {
               tip += " ";
               tip += obj->GetTitle();
            }
            fListTree->SetToolTipItem(item, tip.Data());
         }
      }
      else {
         // special case for remote object
         Bool_t isRemote = kFALSE;
         if (obj->InheritsFrom("TRemoteObject"))
            isRemote = kTRUE;
         else if (fListLevel) {
            // check also if one of its parents is a remote object
            TGListTreeItem *top = fListLevel;
            while (top->GetParent()) {
               TObject *tobj = (TObject *) top->GetUserData();
               if (tobj && (tobj->InheritsFrom("TRemoteObject") ||
                  tobj->InheritsFrom("TApplicationRemote"))) {
                  isRemote = kTRUE;
                  break;
               }
               top = top->GetParent();
            }
         }
         if (isRemote) {
            TRemoteObject *robj = (TRemoteObject *)obj;
            if (!strcmp(robj->GetClassName(), "TKey")) {
               AddKey(fListLevel, obj, name);
            }
            else {
               TString fname = name;
               // add the remote object only if not already in the list
               if (!fShowHidden && fname.BeginsWith("."))
                  return;
               AddRemoteFile(obj);
            }
         }
         else {
            if (!fListTree->FindChildByName(fListLevel, name)) {
               TGListTreeItem *item = fListTree->AddItem(fListLevel, name, obj, pic, pic);
               if ((pic != fFileIcon) && (pic != fCachedPic))
                  fClient->FreePicture(pic);
               item->SetDNDSource(kTRUE);
            }
         }
      }
   }
}

//______________________________________________________________________________
void TGFileBrowser::AddRemoteFile(TObject *obj)
{
   // Add remote file in list tree.

   Bool_t      is_link;
   Int_t       type, uid, gid;
   Long_t      modtime;
   Long64_t    size;
   TString     filename;
   const TGPicture *spic;
   TGPicture *pic;

   FileStat_t sbuf;

   type    = 0;
   size    = 0;
   uid     = 0;
   gid     = 0;
   modtime = 0;
   is_link = kFALSE;

   TRemoteObject *robj = (TRemoteObject *)obj;

   robj->GetFileStat(&sbuf);
   is_link = sbuf.fIsLink;
   type    = sbuf.fMode;
   size    = sbuf.fSize;
   uid     = sbuf.fUid;
   gid     = sbuf.fGid;
   modtime = sbuf.fMtime;
   filename = robj->GetName();
   if (R_ISDIR(type) || fFilter == 0 ||
       (fFilter && filename.Index(*fFilter) != kNPOS)) {

      GetFilePictures(&spic, type, is_link, filename);

      pic = (TGPicture*)spic; pic->AddReference();

      if ((!fListTree->FindChildByName(fListLevel, filename)) &&
         (!fListTree->FindChildByData(fListLevel, obj)))
         fListTree->AddItem(fListLevel, filename, obj, pic, pic);
   }
}

//______________________________________________________________________________
void TGFileBrowser::BrowseObj(TObject *obj)
{
   // Browse object. This, in turn, will trigger the calling of
   // TBrowser::Add() which will fill the IconBox and the tree.
   // Emits signal "BrowseObj(TObject*)".

   if (fNewBrowser)
      fNewBrowser->SetActBrowser(this);
   obj->Browse(fBrowser);
   if (obj == gROOT) {
      AddFSDirectory("/");
      GotoDir(gSystem->WorkingDirectory());
   }
}

//______________________________________________________________________________
void TGFileBrowser::Checked(TObject *obj, Bool_t checked)
{
   // Emits signal when double clicking on icon.

   if (fNewBrowser)
      fNewBrowser->Checked(obj, checked);
}

//______________________________________________________________________________
Option_t *TGFileBrowser::GetDrawOption() const
{
   // returns drawing option

   return fDrawOption->GetTextEntry()->GetText();
}

//______________________________________________________________________________
void TGFileBrowser::GetFilePictures(const TGPicture **pic, Int_t file_type,
                                    Bool_t is_link, const char *name)
{
   // Determine the file picture for the given file type.

   static TString cached_ext;
   static const TGPicture *cached_spic = 0;
   const char *ext = name ? strrchr(name, '.') : 0;
   TString sname = name ? name : " ";
   *pic = 0;

   if (ext && cached_spic && (cached_ext == ext)) {
      *pic = cached_spic;
      return;
   }

   if (R_ISREG(file_type)) {
      *pic = gClient->GetMimeTypeList()->GetIcon(name, kTRUE);

      if (*pic) {
         if (ext) {
            cached_ext = ext;
            cached_spic = *pic;
            return;
         }
      }
   } else {
      *pic = 0;
   }

   if (*pic == 0) {
      *pic = gClient->GetPicture("doc_t.xpm");

      if (R_ISREG(file_type) && (file_type) & kS_IXUSR) {
         *pic = gClient->GetPicture("app_t.xpm");
      }
      if (R_ISDIR(file_type)) {
         *pic = gClient->GetPicture("folder_t.xpm");
      }
      if(sname.EndsWith(".root")) {
         *pic = gClient->GetPicture("rootdb_t.xpm");
      }

   }
   if (is_link) {
      *pic = gClient->GetPicture("slink_t.xpm");
   }

   cached_spic = 0;
   cached_ext = "";
}

//______________________________________________________________________________
void TGFileBrowser::RecursiveRemove(TObject *obj)
{
   // Recursively remove object.

   TGListTreeItem *itm = 0, *item = 0;
   if (obj->InheritsFrom("TFile")) {
      itm = fListTree->FindChildByData(0, gROOT->GetListOfFiles());
      if (itm)
         item = fListTree->FindChildByData(itm, obj);
      if (item)
         fListTree->DeleteItem(item);
      itm = fRootDir ? fRootDir->GetFirstChild() : 0;
      while (itm) {
         item = fListTree->FindItemByObj(itm, obj);
         if (item) {
            fListTree->DeleteChildren(item);
            item->SetUserData(0);
         }
         itm = itm->GetNextSibling();
      }
   }
   if (!obj->InheritsFrom("TFile") && fRootDir)
      fListTree->RecursiveDeleteItem(fRootDir, obj);
   fListTree->ClearViewPort();
}

//______________________________________________________________________________
void TGFileBrowser::Refresh(Bool_t /*force*/)
{
   // Refresh content of the list tree.

   return; // disable refresh for the time being...
   TCursorSwitcher cursorSwitcher(this, fListTree);
   static UInt_t prev = 0;
   UInt_t curr =  gROOT->GetListOfBrowsables()->GetSize();
   if (!prev) prev = curr;

   if (prev != curr) { // refresh gROOT
      TGListTreeItem *sav = fListLevel;
      fListLevel = 0;
      BrowseObj(gROOT);
      fListLevel = sav;
      prev = curr;
   }
}

/**************************************************************************/
// Other
/**************************************************************************/

//______________________________________________________________________________
void TGFileBrowser::AddFSDirectory(const char* /*entry*/, const char* path)
{
   // Add file system directory in the list tree.

   if (path == 0 && fRootDir == 0) {
      fRootDir = fListTree->AddItem(0, rootdir);
   } else {
      // MT: i give up! wanted to place entries for selected
      // directories like home, pwd, alice-macros.
      // TGListTreeItem *lti = fListTree->AddItem(0, entry);
      //
   }
}

//______________________________________________________________________________
void TGFileBrowser::AddKey(TGListTreeItem *itm, TObject *obj, const char *name)
{
   // display content of ROOT file

   // Int_t from, to;
   TGListTreeItem *where;
   static TGListTreeItem *olditem = itm;
   static TGListTreeItem *item = itm;
   const TGPicture *pic;

   if ((fCnt == 0) || (olditem != itm)) {
      olditem = item = itm;
   }
   if (!name) name = obj->GetName();
   if (fNKeys > fGroupSize) {
      where = itm->GetFirstChild();
      while (where) {
         if (fListTree->FindItemByObj(where, obj))
            return;
         where = where->GetNextSibling();
      }
   }
   if ((fNKeys > fGroupSize) && (fCnt % fGroupSize == 0)) {
      if (item != itm) {
         TString newname = Form("%s-%s", item->GetText(), name);
         item->Rename(newname.Data());
      }
      item = fListTree->AddItem(itm, name);
      item->SetDNDSource(kTRUE);
   }
   if ((fCnt > fGroupSize) && (fCnt >= fNKeys-1)) {
      TString newname = Form("%s-%s", item->GetText(), name);
      item->Rename(newname.Data());
   }
   GetObjPicture(&pic, obj);
   if (!pic) pic = gClient->GetPicture("leaf_t.xpm");
   if (!fListTree->FindChildByName(item, name)) {
      TGListTreeItem *it = fListTree->AddItem(item, name, obj, pic, pic);
      if ((pic != fFileIcon) && (pic != fCachedPic))
         fClient->FreePicture(pic);
      it->SetDNDSource(kTRUE);
   }
   fCnt++;
}

//______________________________________________________________________________
void TGFileBrowser::ApplyFilter(Int_t id)
{
   // Apply filter selected in combo box to the file tree view.

   // Long64_t size;
   // Long_t fid, flags, modtime;

   if (fFilter) delete fFilter;
   fFilter = 0;
   if (id > 1)
      fFilter = new TRegexp(filters[id], kTRUE);
   TGListTreeItem *item = fCurrentDir;
   if (!item)
      item = fRootDir;
   fListTree->DeleteChildren(item);
   DoubleClicked(item, 1);
   //fListTree->AdjustPosition(item);
   fListTree->ClearViewPort();
}

//______________________________________________________________________________
void TGFileBrowser::Chdir(TGListTreeItem *item)
{
   // Make object associated with item the current directory.

   if (item) {
      TGListTreeItem *i = item;
      while (i) {
         TObject *obj = (TObject*) i->GetUserData();
         if ((obj) && obj->InheritsFrom("TDirectory")) {
            ((TDirectory *)obj)->cd();
            break;
         }
         i = i->GetParent();
      }
   }
}

//______________________________________________________________________________
void TGFileBrowser::CheckRemote(TGListTreeItem *item)
{
   // Check if the current list tree item points to a remote object.

   TObject *obj = (TObject *) item->GetUserData();
   if (obj) {
      if (obj->InheritsFrom("TApplicationRemote")) {
         if (!gApplication->GetAppRemote()) {
            gROOT->ProcessLine(Form(".R %s", item->GetText()));
            if (gApplication->GetAppRemote()) {
               Getlinem(kInit, Form("\n%s:root [0]",
                        gApplication->GetAppRemote()->ApplicationName()));
            }
         }
      }
      if (item->GetParent() && item->GetParent()->GetUserData() &&
         ((TObject *)item->GetParent()->GetUserData())->InheritsFrom("TApplicationRemote")) {
         // switch to remote session
         if (!gApplication->GetAppRemote()) {
            gROOT->ProcessLine(Form(".R %s", item->GetParent()->GetText()));
            if (gApplication->GetAppRemote()) {
               Getlinem(kInit, Form("\n%s:root [0]",
                        gApplication->GetAppRemote()->ApplicationName()));
            }
         }
         else if (!strcmp(item->GetText(), "ROOT Files")) {
            // update list of files opened in the remote session
            gApplication->SetBit(TApplication::kProcessRemotely);
            gApplication->ProcessLine("((TApplicationServer *)gApplication)->BrowseFile(0);");
         }
      }
      else {
         // check if the listtree item is from a local session or
         // from a remote session, then switch to the session it belongs to
         TGListTreeItem *top = item;
         while (top->GetParent()) {
            top = top->GetParent();
         }
         TObject *topobj = (TObject *) top->GetUserData();
         if (topobj && topobj->InheritsFrom("TApplicationRemote")) {
            // it belongs to a remote session
            if (!gApplication->GetAppRemote()) {
               // switch to remote session if not already in
               gROOT->ProcessLine(Form(".R %s", top->GetText()));
               if (gApplication->GetAppRemote()) {
                  Getlinem(kInit, Form("\n%s:root [0]",
                           gApplication->GetAppRemote()->ApplicationName()));
               }
            }
         }
         else if (gApplication->GetAppRemote()) {
            // switch back to local session if not already in
            gApplication->ProcessLine(".R");
            Getlinem(kInit, "\nroot [0]");
         }
      }
   }
   else if (gApplication->GetAppRemote()) {
      // switch back to local session if not already in
      gApplication->ProcessLine(".R");
      Getlinem(kInit, "\nroot [0]");
   }
}

//______________________________________________________________________________
void TGFileBrowser::Clicked(TGListTreeItem *item, Int_t btn, Int_t x, Int_t y)
{
   // Process mouse clicks in TGListTree.

   char path[1024];
   Long64_t size = 0;
   Long_t id = 0, flags = 0, modtime = 0;
   fListLevel = item;
   CheckRemote(item);
   if (item && btn == kButton3) {
      TObject *obj = (TObject *) item->GetUserData();
      if (obj) {
         if (obj->InheritsFrom("TKey") && (obj->IsA() != TClass::Class())) {
            Chdir(item);
            const char *clname = (const char *)gROOT->ProcessLine(Form("((TKey *)0x%lx)->GetClassName();", obj));
            if (clname) {
               TClass *cl = TClass::GetClass(clname);
               void *add = gROOT->FindObject((char *) obj->GetName());
               if (add && cl->IsTObject()) {
                  obj = (TObject*)add;
                  item->SetUserData(obj);
               }
            }
         }
         if (obj->InheritsFrom("TLeaf") ||
             obj->InheritsFrom("TBranch")) {
            Chdir(item);
         }
         fContextMenu->Popup(x, y, obj);
      }
      else {
         fListTree->GetPathnameFromItem(item, path);
         if (strlen(path) > 3) {
            TString dirname = DirName(item);
            gSystem->GetPathInfo(dirname.Data(), &id, &size, &flags, &modtime);
            if (flags & 2) {
               fCurrentDir = item;
               if (fDir) delete fDir;
               fDir = new TSystemDirectory(item->GetText(), dirname.Data());
               fContextMenu->Popup(x, y, fDir);
            }
            else {
               fCurrentDir = item->GetParent();
               if (fFile) delete fFile;
               fFile = new TSystemFile(item->GetText(), dirname.Data());
               fContextMenu->Popup(x, y, fFile);
            }
         }
      }
      fListTree->ClearViewPort();
   }
   else {
      if (item->GetUserData()) {
         TObject *obj = (TObject *) item->GetUserData();
         if (obj && obj->InheritsFrom("TKey"))
            Chdir(item);
      }
      else {
         fListTree->GetPathnameFromItem(item, path);
         if (strlen(path) > 1) {
            TString dirname = DirName(item);
            gSystem->GetPathInfo(dirname.Data(), &id, &size, &flags, &modtime);
            if (flags & 2)
               fCurrentDir = item;
            else
               fCurrentDir = item->GetParent();
         }
      }
   }
}

//______________________________________________________________________________
TString TGFileBrowser::DirName(TGListTreeItem* item)
{
   // returns an absolute path

   TGListTreeItem* parent;
   TString dirname = item->GetText();

   while ((parent=item->GetParent())) {
      dirname = gSystem->ConcatFileName(parent->GetText(),dirname);
      item = parent;
   }

   return dirname;
}

//______________________________________________________________________________
static Bool_t IsTextFile(const char *candidate)
{
   // Returns true if given a text file
   // Uses the specification given on p86 of the Camel book
   // - Text files have no NULLs in the first block
   // - and less than 30% of characters with high bit set

   Int_t i;
   Int_t nchars;
   Int_t weirdcount = 0;
   char buffer[512];
   FILE *infile;
   FileStat_t buf;

   gSystem->GetPathInfo(candidate, buf);
   if (!(buf.fMode & kS_IFREG))
      return kFALSE;

   infile = fopen(candidate, "r");
   if (infile) {
      // Read a block
      nchars = fread(buffer, 1, 512, infile);
      fclose (infile);
      // Examine the block
      for (i = 0; i < nchars; i++) {
         if (buffer[i] & 128)
            weirdcount++;
         if (buffer[i] == '\0')
            // No NULLs in text files
            return kFALSE;
      }
      if ((nchars > 0) && ((weirdcount * 100 / nchars) > 30))
         return kFALSE;
   } else {
      // Couldn't open it. Not a text file then
      return kFALSE;
   }
   return kTRUE;
}

//______________________________________________________________________________
void TGFileBrowser::DoubleClicked(TGListTreeItem *item, Int_t /*btn*/)
{
   // Process double clicks in TGListTree.

   const TGPicture *pic=0;
   TString dirname = DirName(item);
   TGListTreeItem *itm;
   FileStat_t sbuf;
   Long64_t size;
   Long_t id, flags, modtime;
   char action[512];
   TString act;

   if (fNewBrowser)
      fNewBrowser->SetActBrowser(this);
   TCursorSwitcher switcher(this, fListTree);
   fListLevel = item;
   CheckRemote(item);
   TGListTreeItem *pitem = item->GetParent();
   TObject *obj = (TObject *) item->GetUserData();
   if (obj && !obj->InheritsFrom("TSystemFile")) {
      TString ext = obj->GetName();
      if (obj->InheritsFrom("TFile")) {
         fNKeys = ((TDirectory *)obj)->GetListOfKeys()->GetEntries();
      }
      else if (obj->InheritsFrom("TKey") && (obj->IsA() != TClass::Class())) {
         Chdir(item);
         const char *clname = (const char *)gROOT->ProcessLine(Form("((TKey *)0x%lx)->GetClassName();", obj));
         if (clname) {
            TClass *cl = TClass::GetClass(clname);
            void *add = gROOT->FindObject((char *) obj->GetName());
            if (add && cl->IsTObject()) {
               obj = (TObject*)add;
               item->SetUserData(obj);
            }
         }
      }
      else if (obj->InheritsFrom("TLeaf") ||
          obj->InheritsFrom("TBranch")) {
         Chdir(item);
      }
      else if (obj->InheritsFrom("TRemoteObject")) {
         // the real object is a TKey
         TRemoteObject *robj = (TRemoteObject *)obj;
         if (!strcmp(robj->GetClassName(), "TKey")) {
            TGListTreeItem *parent = item;
            TRemoteObject *probj = (TRemoteObject *)parent->GetUserData();
            // find the TFile remote object containing the TKey
            while ( probj && strcmp(probj->GetClassName(), "TFile")) {
               parent = parent->GetParent();
               probj = (TRemoteObject *)parent->GetUserData();
            }
            if (probj && !strcmp(probj->GetClassName(), "TFile")) {
               // remotely browse file (remotely call TFile::cd())
               gApplication->SetBit(TApplication::kProcessRemotely);
               gApplication->ProcessLine(
                  Form("((TApplicationServer *)gApplication)->BrowseFile(\"%s\");",
                       probj->GetName()));
               gSystem->Sleep(250);
            }
         }
         if (gClient->GetMimeTypeList()->GetAction(obj->GetName(), action)) {
            act = action;
            act.ReplaceAll("%s", obj->GetName());
            if ((act[0] != '!') && (strcmp(pitem->GetText(), "ROOT Files"))) {
               // special case for remote object: remote process
               gApplication->SetBit(TApplication::kProcessRemotely);
               gApplication->ProcessLine(act.Data());
            }
         }
         if ((ext.EndsWith(".root")) && (strcmp(pitem->GetText(), "ROOT Files"))) {
            gApplication->SetBit(TApplication::kProcessRemotely);
            gApplication->ProcessLine("((TApplicationServer *)gApplication)->BrowseFile(0);");
         }
      }
      if (!obj->InheritsFrom("TObjString")) {
         obj->Browse(fBrowser);
         fNKeys = 0;
         fCnt = 0;
         fListTree->ClearViewPort();
         return;
      }
   }
   flags = id = size = modtime = 0;
   gSystem->GetPathInfo(dirname.Data(), &id, &size, &flags, &modtime);
   Int_t isdir = (Int_t)flags & 2;

   TString savdir = gSystem->WorkingDirectory();
   if (isdir) {
      fCurrentDir = item;
      //fListTree->DeleteChildren(item);
      TSystemDirectory dir(item->GetText(),DirName(item));
      TList *files = dir.GetListOfFiles();
      if (files) {
         files->Sort();
         TIter next(files);
         TSystemFile *file;
         TString fname;
         // directories first
         while ((file=(TSystemFile*)next())) {
            fname = file->GetName();
            if (file->IsDirectory()) {
               if (!fShowHidden && fname.BeginsWith("."))
                  continue;
               if ((fname!="..") && (fname!=".")) { // skip it
                  if (!fListTree->FindChildByName(item, fname)) {
                     itm = fListTree->AddItem(item, fname);
                     // uncomment line below to set directories as
                     // DND targets
                     //itm->SetDNDTarget(kTRUE);
                     itm->SetUserData(0);
                  }
               }
            }
         }
         // then files...
         TIter nextf(files);
         while ((file=(TSystemFile*)nextf())) {
            fname = file->GetName();
            if (!file->IsDirectory() && (fFilter == 0 ||
               (fFilter && fname.Index(*fFilter) != kNPOS))) {
               if (!fShowHidden && fname.BeginsWith("."))
                  continue;
               size = modtime = 0;
               if (gSystem->GetPathInfo(fname, sbuf) == 0) {
                  size    = sbuf.fSize;
                  modtime = sbuf.fMtime;
               }
               pic = gClient->GetMimeTypeList()->GetIcon(fname, kTRUE);
               if (!pic)
                  pic = fFileIcon;
               if (!fListTree->FindChildByName(item, fname)) {
                  itm = fListTree->AddItem(item,fname,pic,pic);
                  if (pic != fFileIcon)
                     fClient->FreePicture(pic);
                  itm->SetUserData(new TObjString(Form("file://%s/%s\r\n",
                                   gSystem->UnixPathName(file->GetTitle()),
                                   file->GetName())), kTRUE);
                  itm->SetDNDSource(kTRUE);
                  if (size && modtime) {
                     char *tiptext = FormatFileInfo(fname.Data(), size, modtime);
                     itm->SetTipText(tiptext);
                     delete [] tiptext;
                  }
               }
            }
         }
         files->Delete();
         delete files;
      }
   }
   else {
      fCurrentDir = item->GetParent();
      TSystemFile f(item->GetText(), dirname.Data());
      TString fname = f.GetName();
      if (fname.EndsWith(".root")) {
         TDirectory *rfile = 0;
         gSystem->ChangeDirectory(gSystem->DirName(dirname.Data()));
         rfile = (TDirectory *)gROOT->GetListOfFiles()->FindObject(obj);
         if (!rfile) {
            rfile = (TDirectory *)gROOT->ProcessLine(Form("new TFile(\"%s\")",fname.Data()));
         }
         if (rfile) {
            // replace actual user data (TObjString) by the TDirectory...
            if (item->GetUserData()) {
               // first delete the data to avoid memory leaks
               TObject *obj = static_cast<TObject *>(item->GetUserData());
               // only delete TObjString as they are the only objects
               // created who have to be deleted
               delete dynamic_cast<TObjString *>(obj);
            }
            item->SetUserData(rfile);
            fNKeys = rfile->GetListOfKeys()->GetEntries();
            fCnt = 0;
            rfile->Browse(fBrowser);
            fNKeys = 0;
            fCnt = 0;
         }
      }
      else if (fname.EndsWith(".png")) {
         gSystem->ChangeDirectory(gSystem->DirName(dirname.Data()));
         XXExecuteDefaultAction(&f);
      }
      else if (IsTextFile(dirname.Data())) {
         gSystem->ChangeDirectory(gSystem->DirName(dirname.Data()));
         if (fNewBrowser) {
            TGFrameElement *fe = 0;
            TGTab *tabRight = fNewBrowser->GetTabRight();
            TGCompositeFrame *frame = tabRight->GetCurrentContainer();
            if (frame)
               fe = (TGFrameElement *)frame->GetList()->First();
            if (fe) {
               TGCompositeFrame *embed = (TGCompositeFrame *)fe->fFrame;
               if (embed->InheritsFrom("TGTextEditor")) {
                  gROOT->ProcessLine(Form("((TGTextEditor *)0x%lx)->LoadFile(\"%s\");",
                                     embed, f.GetName()));
               }
               else if (embed->InheritsFrom("TGTextEdit")) {
                  gROOT->ProcessLine(Form("((TGTextEdit *)0x%lx)->LoadFile(\"%s\");",
                                     embed, f.GetName()));
               }
               else {
                  XXExecuteDefaultAction(&f);
               }
            }
            else {
               XXExecuteDefaultAction(&f);
            }
         }
      }
      else {
         gSystem->ChangeDirectory(gSystem->DirName(dirname.Data()));
         XXExecuteDefaultAction(&f);
      }
   }
   gSystem->ChangeDirectory(savdir.Data());
   fListTree->ClearViewPort();
}

//____________________________________________________________________________
Long_t TGFileBrowser::XXExecuteDefaultAction(TObject *obj)
{
   // Execute default action for selected object (action is specified
   // in the $HOME/.root.mimes or $ROOTSYS/etc/root.mimes file.

   char action[512];
   TString act;
   TString ext = obj->GetName();
   fBrowser->SetDrawOption(GetDrawOption());

   if (gClient->GetMimeTypeList()->GetAction(obj->GetName(), action)) {
      act = action;
      act.ReplaceAll("%s", obj->GetName());
      gInterpreter->SaveGlobalsContext();

      if (act[0] == '!') {
         act.Remove(0, 1);
         gSystem->Exec(act.Data());
         return 0;
      } else {
         // special case for remote object: remote process
         if (obj->InheritsFrom("TRemoteObject"))
            gApplication->SetBit(TApplication::kProcessRemotely);
         return gApplication->ProcessLine(act.Data());
      }
   }
   return 0;
}

//______________________________________________________________________________
char *TGFileBrowser::FormatFileInfo(const char *fname, Long64_t size, Long_t modtime)
{
   // Format file information to be displayed in the tooltip.

   Long64_t fsize, bsize;
   TString infos = fname;
   infos += "\n";

   fsize = bsize = size;
   if (fsize > 1024) {
      fsize /= 1024;
      if (fsize > 1024) {
         // 3.7MB is more informative than just 3MB
         infos += Form("Size: %lld.%lldM", fsize/1024, (fsize%1024)/103);
      } else {
         infos += Form("Size: %lld.%lldK", bsize/1024, (bsize%1024)/103);
      }
   } else {
      infos += Form("Size: %lld", bsize);
   }
   struct tm *newtime;
   time_t loctime = (time_t) modtime;
   newtime = localtime(&loctime);
   infos += "\n";
   infos += Form("%d-%02d-%02d %02d:%02d", newtime->tm_year + 1900,
           newtime->tm_mon+1, newtime->tm_mday, newtime->tm_hour,
           newtime->tm_min);
   return StrDup(infos.Data());
}

//______________________________________________________________________________
void TGFileBrowser::GetObjPicture(const TGPicture **pic, TObject *obj)
{
   // Retrieve icons associated with class "name". Association is made
   // via the user's ~/.root.mimes file or via $ROOTSYS/etc/root.mimes.

   TClass *objClass = 0;
   static TImage *im = 0;
   if (!im) {
      im = TImage::Create();
   }

   if (obj->IsA() == TClass::Class()) {
      objClass = obj->IsA();
   }
   else if (obj->InheritsFrom("TKey")) {
      const char *clname = (const char *)gROOT->ProcessLine(Form("((TKey *)0x%lx)->GetClassName();", obj));
      if (clname)
         objClass = TClass::GetClass(clname);
   }
   else if (obj->InheritsFrom("TKeyMapFile")) {
      const char *title = (const char *)gROOT->ProcessLine(Form("((TKeyMapFile *)0x%lx)->GetTitle();", obj));
      if (title)
         objClass = TClass::GetClass(title);
   }
   else if (obj->InheritsFrom("TRemoteObject")) {
      // special case for remote object: get real object class
      TRemoteObject *robj = (TRemoteObject *)obj;
      if (!strcmp(robj->GetClassName(), "TKey"))
         objClass = TClass::GetClass(robj->GetKeyClassName());
      else
         objClass = TClass::GetClass(robj->GetClassName());
   }
   else
      objClass = obj->IsA();
   const char *name = obj->GetIconName() ? obj->GetIconName() : objClass->GetName();
   TString xpm_magic(name, 3);
   Bool_t xpm = xpm_magic == "/* ";
   const char *iconname = xpm ? obj->GetName() : name;

   if (obj->IsA()->InheritsFrom("TGeoVolume")) {
      iconname = obj->GetIconName() ? obj->GetIconName() : obj->IsA()->GetName();
   }

   if (fCachedPicName == iconname) {
      *pic = fCachedPic;
      return;
   }
   *pic = gClient->GetMimeTypeList()->GetIcon(iconname, kTRUE);
   if (!(*pic) && xpm) {
      if (im && im->SetImageBuffer((char**)&name, TImage::kXpm)) {
         im->Scale(im->GetWidth()/4, im->GetHeight()/4);
         *pic = gClient->GetPicturePool()->GetPicture(iconname, im->GetPixmap(),
                                                      im->GetMask());
      }
      gClient->GetMimeTypeList()->AddType("[thumbnail]", iconname, iconname, iconname, "->Browse()");
      return;
   }
   if (fCachedPic && (fCachedPic != fFileIcon))
      fClient->FreePicture(fCachedPic);
   if (*pic == 0) {
      if (!obj->IsFolder())
         *pic = fFileIcon;
   }
   fCachedPic = *pic;
   fCachedPicName = iconname;
}

//______________________________________________________________________________
void TGFileBrowser::GotoDir(const char *path)
{
   // Go to the directory "path" and open all the parent list tree items.

   TGListTreeItem *item, *itm;
   TString sPath(gSystem->UnixPathName(path));
   item = fRootDir;
   if (item == 0) return;
   fListTree->HighlightItem(item);
   fListTree->OpenItem(item);
   DoubleClicked(item, 1);
   TObjArray *tokens = sPath.Tokenize("/");
   for (Int_t i = 0; i < tokens->GetEntriesFast(); ++i) {
      const char *token = ((TObjString*)tokens->At(i))->GetName();
      itm = fListTree->FindChildByName(item, token);
      if (itm) {
         item = itm;
         fListTree->HighlightItem(item);
         fListTree->OpenItem(item);
         DoubleClicked(item, 1);
      }
   }
   delete tokens;
   fListTree->ClearViewPort();
   fListTree->AdjustPosition(item);
}

//______________________________________________________________________________
void TGFileBrowser::Selected(char *)
{
   // A ROOT File has been selected in TGHtmlBrowser.

   TGListTreeItem *itm = fListTree->FindChildByData(0, gROOT->GetListOfFiles());
   if (itm) {
      fListTree->ClearHighlighted();
      fListLevel = itm;
      fListTree->HighlightItem(fListLevel);
      fListTree->OpenItem(fListLevel);
      BrowseObj(gROOT->GetListOfFiles());
      fListTree->ClearViewPort();
      fListTree->AdjustPosition(fListLevel);
   }
}

