// @(#)root/eve:$Id$
// Authors: Matevz Tadel & Alja Mrak-Tadel: 2006, 2007

/*************************************************************************
 * Copyright (C) 1995-2007, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#include "TEveManager.h"

#include "TEveViewer.h"
#include "TEveScene.h"
#include "TEvePad.h"
#include "TEveEventManager.h"

#include "TEveBrowser.h"
#include "TEveGedEditor.h"

#include "TGMenu.h"
#include "TGTab.h"
#include "TGToolBar.h"
#include "TGLabel.h"
#include "TGTextEntry.h"
#include "TGSplitter.h"
#include "TRootEmbeddedCanvas.h"

#include "TGStatusBar.h"

#include "TGLSAViewer.h"

#include "TROOT.h"
#include "TFile.h"
#include "TMacro.h"
#include "TFolder.h"
#include "TStyle.h"
#include "TBrowser.h"
#include "TPad.h"
#include "TCanvas.h"
#include "TSystem.h"
#include "TRint.h"
#include "TVirtualX.h"
#include "TEnv.h"
#include "TStyle.h"
#include "TColor.h"
#include "TGeoShape.h"
#include <KeySymbols.h>
#include "TVirtualGL.h"
#include "TPluginManager.h"

#include <iostream>

TEveManager* gEve = 0;

//______________________________________________________________________________
// TEveManager
//
// Central aplicat manager for Reve.
// Manages elements, GUI, GL scenes and GL viewers.

ClassImp(TEveManager)

//______________________________________________________________________________
TEveManager::TEveManager(UInt_t w, UInt_t h) :
   fExcHandler  (0),
   fBrowser     (0),
   fEditor      (0),
   fStatusBar   (0),

   fMacroFolder (0),

   fViewers        (0),
   fScenes         (0),
   fViewer         (0),
   fGlobalScene    (0),
   fEventScene     (0),
   fCurrentEvent   (0),

   fRedrawDisabled (0),
   fResetCameras   (kFALSE),
   fDropLogicals   (kFALSE),
   fKeepEmptyCont  (kFALSE),
   fTimerActive    (kFALSE),
   fRedrawTimer    (),

   fGeometries     ()
{
   // Constructor.

   static const TEveException eh("TEveManager::TEveManager ");

   if (gEve != 0)
      throw(eh + "There can be only one!");

   gEve = this;

   fExcHandler = new TExceptionHandler;

   fRedrawTimer.Connect("Timeout()", "TEveManager", this, "DoRedraw3D()");
   fMacroFolder = new TFolder("EVE", "Visualization macros");
   gROOT->GetListOfBrowsables()->Add(fMacroFolder);


   // Build GUI
   fBrowser   = new TEveBrowser(w, h);
   fStatusBar = fBrowser->GetStatusBar();

   // ListTreeEditor
   fBrowser->StartEmbedding(0);
   fLTEFrame = new TEveGListTreeEditorFrame("Eve list-tree/editor");
   fBrowser->StopEmbedding();
   fBrowser->SetTabTitle("Eve", 0);
   fEditor   = fLTEFrame->fEditor;

   // GL viewer
   fBrowser->StartEmbedding(1);
   TGLSAViewer* glv = new TGLSAViewer(gClient->GetRoot(), 0, fEditor);
   //glv->GetFrame()->SetCleanup(kNoCleanup);
   glv->ToggleEditObject();
   fBrowser->StopEmbedding();
   fBrowser->SetTabTitle("GLViewer", 1);

   // Finalize it
   fBrowser->InitPlugins();
   fBrowser->MapWindow();

   // --------------------------------

   fViewers = new TEveViewerList("Viewers");
   fViewers->IncDenyDestroy();
   AddToListTree(fViewers, kTRUE);

   fViewer  = new TEveViewer("GLViewer");
   fViewer->SetGLViewer(glv);
   fViewer->IncDenyDestroy();
   AddElement(fViewer, fViewers);

   fScenes  = new TEveSceneList ("Scenes");
   fScenes->IncDenyDestroy();
   AddToListTree(fScenes, kTRUE);

   fGlobalScene = new TEveScene("Geometry scene");
   fGlobalScene->IncDenyDestroy();
   AddElement(fGlobalScene, fScenes);

   fEventScene = new TEveScene("Event scene");
   fEventScene->IncDenyDestroy();
   AddElement(fEventScene, fScenes);

   fViewer->AddScene(fGlobalScene);
   fViewer->AddScene(fEventScene);

   /**************************************************************************/
   /**************************************************************************/

   fEditor->DisplayObject(GetGLViewer());

   gSystem->ProcessEvents();
}

//______________________________________________________________________________
TEveManager::~TEveManager()
{
   // Destructor.

   delete fExcHandler;
}

/******************************************************************************/

//______________________________________________________________________________
TCanvas* TEveManager::AddCanvasTab(const char* name)
{
   // Add a new canvas tab.

   fBrowser->StartEmbedding(1, -1);
   TCanvas* c = new TCanvas;
   fBrowser->StopEmbedding();
   fBrowser->SetTabTitle(name, 1, -1);

   return c;
}

//______________________________________________________________________________
TGWindow* TEveManager::GetMainWindow() const
{
   // Get the main window, i.e. the first created reve-browser.

   return fBrowser;
}

//______________________________________________________________________________
TGLViewer* TEveManager::GetGLViewer() const
{
   // Get default TGLViewer.

   return fViewer->GetGLViewer();
}

//______________________________________________________________________________
TEveViewer* TEveManager::SpawnNewViewer(const Text_t* name, const Text_t* title,
                                        Bool_t embed)
{
   // Create a new GL viewer.

   TEveViewer* v = new TEveViewer(name, title);

   if (embed)  fBrowser->StartEmbedding(1);
   v->SpawnGLViewer(gClient->GetRoot(), embed ? fEditor : 0);
   v->IncDenyDestroy();
   if (embed)  fBrowser->StopEmbedding(), fBrowser->SetTabTitle(name, 1);
   AddElement(v, fViewers);
   return v;
}

//______________________________________________________________________________
TEveScene* TEveManager::SpawnNewScene(const Text_t* name, const Text_t* title)
{
   // Create a new scene.

   TEveScene* s = new TEveScene(name, title);
   AddElement(s, fScenes);
   return s;
}

/******************************************************************************/
// Macro management
/******************************************************************************/

//______________________________________________________________________________
TMacro* TEveManager::GetMacro(const Text_t* name) const
{
   // Find macro in fMacroFolder by name.

   return dynamic_cast<TMacro*>(fMacroFolder->FindObject(name));
}

/******************************************************************************/
// Editor
/******************************************************************************/

//______________________________________________________________________________
void TEveManager::EditElement(TEveElement* element)
{
   // Show element in default editor.

   static const TEveException eh("TEveManager::EditElement ");

   fEditor->DisplayElement(element);
}

/******************************************************************************/
// 3D TEvePad management
/******************************************************************************/

//______________________________________________________________________________
void TEveManager::RegisterRedraw3D()
{
   // Register a request for 3D redraw.

   fRedrawTimer.Start(0, kTRUE);
   fTimerActive = true;
}

//______________________________________________________________________________
void TEveManager::DoRedraw3D()
{
   // Perform 3D redraw of scenes and viewers whose contents has
   // changed.

   // printf("TEveManager::DoRedraw3D redraw triggered\n");

   fScenes ->RepaintChangedScenes();
   fViewers->RepaintChangedViewers(fResetCameras, fDropLogicals);

   fResetCameras = kFALSE;
   fDropLogicals = kFALSE;

   fTimerActive = kFALSE;
}

//______________________________________________________________________________
void TEveManager::FullRedraw3D(Bool_t resetCameras, Bool_t dropLogicals)
{
   // Perform 3D redraw of all scenes and viewers.

   fScenes ->RepaintAllScenes();
   fViewers->RepaintAllViewers(resetCameras, dropLogicals);
}

/******************************************************************************/

//______________________________________________________________________________
void TEveManager::ElementChanged(TEveElement* element, Bool_t update_scenes, Bool_t redraw)
{
   // Element was changed, perform framework side action.
   // Called from TEveElement::ElementChanged().

   static const TEveException eh("TEveElement::ElementChanged ");

   if (fEditor->GetModel() == element->GetEditorObject(eh))
      fEditor->DisplayElement(element);

   if (update_scenes) {
      std::list<TEveElement*> scenes;
      element->CollectSceneParents(scenes);
      ScenesChanged(scenes);
   }

   if (redraw)
      Redraw3D();
}

//______________________________________________________________________________
void TEveManager::ScenesChanged(std::list<TEveElement*>& scenes)
{
   // Mark all scenes from the given list as changed.

   for (TEveElement::List_i s=scenes.begin(); s!=scenes.end(); ++s)
      ((TEveScene*)*s)->Changed();
}


/******************************************************************************/
// GUI interface
/******************************************************************************/

//______________________________________________________________________________
TGListTree* TEveManager::GetListTree() const
{
   // Get default list-tree widget.

   return fLTEFrame->fListTree;
}

TGListTreeItem*
TEveManager::AddToListTree(TEveElement* re, Bool_t open, TGListTree* lt)
{
   // Add element as a top-level to a list-tree.
   // Only add a single copy of a render-element as a top level.

   if (lt == 0) lt = GetListTree();
   TGListTreeItem* lti = re->AddIntoListTree(lt, (TGListTreeItem*)0);
   if (open) lt->OpenItem(lti);
   return lti;
}

//______________________________________________________________________________
void TEveManager::RemoveFromListTree(TEveElement* element,
                                     TGListTree* lt, TGListTreeItem* lti)
{
   // Remove top-level element from list-tree with specified tree-item.

   static const TEveException eh("TEveManager::RemoveFromListTree ");

   if (lti->GetParent())
      throw(eh + "not a top-level item.");

   element->RemoveFromListTree(lt, 0);
}

/******************************************************************************/

//______________________________________________________________________________
TGListTreeItem* TEveManager::AddEvent(TEveEventManager* event)
{
   // Add a new event and make it the current event.
   // It is added into the event-scene and as a top-level list-tree
   // item.

   fCurrentEvent = event;
   fCurrentEvent->IncDenyDestroy();
   AddElement(fCurrentEvent, fEventScene);
   return AddToListTree(event, kTRUE);
}

//______________________________________________________________________________
TGListTreeItem* TEveManager::AddElement(TEveElement* element,
                                        TEveElement* parent)
{
   // Add an element. If parent is not specified it is added into
   // current event (which is created if does not exist).

   if (parent == 0) {
      if (fCurrentEvent == 0)
         AddEvent(new TEveEventManager("Event", "Auto-created event directory"));
      parent = fCurrentEvent;
   }

   return parent->AddElement(element);
}

//______________________________________________________________________________
TGListTreeItem* TEveManager::AddGlobalElement(TEveElement* element,
                                              TEveElement* parent)
{
   // Add a global element, i.e. one that does not change on each
   // event, like geometry or projection manager.
   // If parent is not specified it is added to a global scene.

   if (parent == 0)
      parent = fGlobalScene;

   return parent->AddElement(element);
}

/******************************************************************************/

//______________________________________________________________________________
void TEveManager::RemoveElement(TEveElement* element,
                                TEveElement* parent)
{
   // Remove element from parent.

   parent->RemoveElement(element);
}

//______________________________________________________________________________
void TEveManager::PreDeleteElement(TEveElement* element)
{
   // Called from TEveElement prior to its destruction so the
   // framework components (like object editor) can unreference it.

   if (fEditor->GetEveElement() == element)
      fEditor->DisplayObject(0);

   std::list<TEveElement*> scenes;
   element->CollectSceneParents(scenes);
   ScenesChanged(scenes);
}

/******************************************************************************/

//______________________________________________________________________________
void TEveManager::ElementSelect(TEveElement* element)
{
   // Select an element.

   EditElement(element);
}

//______________________________________________________________________________
Bool_t TEveManager::ElementPaste(TEveElement* element)
{
   // Paste has been called.

   // The object to paste is taken from the editor (this is not
   // exactly right) and handed to 'element' for pasting.

   TEveElement* src = fEditor->GetEveElement();
   if (src)
      return element->HandleElementPaste(src);
   return kFALSE;
}

//______________________________________________________________________________
void TEveManager::ElementChecked(TEveElement* element, Bool_t state)
{
   // An element has been checked in the list-tree.

   element->SetRnrState(state);
   element->ElementChanged();
}

/******************************************************************************/
// GeoManager registration
/******************************************************************************/

//______________________________________________________________________________
TGeoManager* TEveManager::GetGeometry(const TString& filename)
{
   // Get geometry with given filename.
   // This is cached internally so the second time this function is
   // called with the same argument the same geo-manager is returned.

   static const TEveException eh("TEveManager::GetGeometry ");

   TString exp_filename = filename;
   gSystem->ExpandPathName(exp_filename);
   printf("%s loading: '%s' -> '%s'.\n", eh.Data(),
          filename.Data(), exp_filename.Data());

   std::map<TString, TGeoManager*>::iterator g = fGeometries.find(filename);
   if (g != fGeometries.end()) {
      return g->second;
   } else {
      if (gSystem->AccessPathName(exp_filename, kReadPermission))
         throw(eh + "file '" + exp_filename + "' not readable.");
      gGeoManager = 0;
      TGeoManager::Import(filename);
      if (gGeoManager == 0)
         throw(eh + "GeoManager import failed.");
      gGeoManager->GetTopVolume()->VisibleDaughters(1);

      // Import colors exported by Gled, if they exist.
      {
         TFile f(exp_filename, "READ");
         TObjArray* collist = (TObjArray*) f.Get("ColorList");
         f.Close();
         if (collist != 0) {
            TIter next(gGeoManager->GetListOfVolumes());
            TGeoVolume* vol;
            while ((vol = (TGeoVolume*) next()) != 0)
            {
               Int_t oldID = vol->GetLineColor();
               TColor* col = (TColor*)collist->At(oldID);
               Float_t r, g, b;
               col->GetRGB(r, g, b);
               Int_t  newID = TColor::GetColor(r,g,b);
               vol->SetLineColor(newID);
            }
         }
      }

      fGeometries[filename] = gGeoManager;
      return gGeoManager;
   }
}

//______________________________________________________________________________
void TEveManager::SetStatusLine(const char* text)
{
   // Set the text in the right side of browser's status bar.

   fBrowser->SetStatusText(text, 1);
}

/******************************************************************************/
// Static initialization.
/******************************************************************************/

//______________________________________________________________________________
TEveManager* TEveManager::Create()
{
   // If global TEveManager* gEve is not set initialize it.
   // Returns gEve.

   if (gEve == 0)
   {
      // Make sure that the GUI system is initialized.
      TApplication::NeedGraphicsLibs();
      gApplication->InitializeGraphics();

      Int_t w = 1024;
      Int_t h =  768;

      TEveUtil::SetupEnvironment();
      TEveUtil::SetupGUI();
      gEve = new TEveManager(w, h);
   }
   return gEve;
}


/******************************************************************************/
// TEveManager::TExceptionHandler
/******************************************************************************/

//______________________________________________________________________________
//
// Exception handler for Eve exceptions.

ClassImp(TEveManager::TExceptionHandler)

//______________________________________________________________________________
TStdExceptionHandler::EStatus
TEveManager::TExceptionHandler::Handle(std::exception& exc)
{
   // Handle exceptions deriving from TEveException.

   TEveException* ex = dynamic_cast<TEveException*>(&exc);
   if (ex) {
      Info("Handle", ex->Data());
      gEve->SetStatusLine(ex->Data());
      gSystem->Beep();
      return kSEHandled;
   } else {
      return kSEProceed;
   }
}
