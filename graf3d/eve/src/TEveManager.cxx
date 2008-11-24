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

#include "TEveSelection.h"
#include "TEveViewer.h"
#include "TEveScene.h"
#include "TEvePad.h"
#include "TEveEventManager.h"

#include "TEveBrowser.h"
#include "TEveGedEditor.h"

#include "TGStatusBar.h"

#include "TGLSAViewer.h"

#include "TGeoManager.h"
#include "TObjString.h"
#include "TROOT.h"
#include "TFile.h"
#include "TMap.h"
#include "TMacro.h"
#include "TFolder.h"
#include "TBrowser.h"
#include "TPad.h"
#include "TCanvas.h"
#include "TSystem.h"
#include "TRint.h"
#include "TVirtualX.h"
#include "TEnv.h"
#include "TColor.h"
#include "TVirtualGL.h"
#include "TPluginManager.h"
#include "TPRegexp.h"
#include "TClass.h"

#include "Riostream.h"

TEveManager* gEve = 0;

//______________________________________________________________________________
// TEveManager
//
// Central aplication manager for Eve.
// Manages elements, GUI, GL scenes and GL viewers.

ClassImp(TEveManager);

//______________________________________________________________________________
TEveManager::TEveManager(UInt_t w, UInt_t h, Bool_t map_window, Option_t* opt) :
   fExcHandler  (0),
   fVizDB       (0), fVizDBReplace(kTRUE), fVizDBUpdate(kTRUE),
   fGeometries  (0),
   fGeometryAliases (0),
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

   fStampedElements(),
   fSelection      (0),
   fHighlight      (0),

   fOrphanage      (0),
   fUseOrphanage   (kFALSE)
{
   // Constructor.
   // If map_window is true, the TEveBrowser window is mapped.
   // Option string is passed to TEveBrowser for creation of
   // additional plugins. By default file-browser and command-line
   // plugins are created.


   static const TEveException eh("TEveManager::TEveManager ");

   if (gEve != 0)
      throw(eh + "There can be only one!");

   gEve = this;

   fExcHandler = new TExceptionHandler;

   fGeometries      = new TMap; fGeometries->SetOwnerKeyValue();
   fGeometryAliases = new TMap; fGeometryAliases->SetOwnerKeyValue();
   fVizDB           = new TMap; fVizDB->SetOwnerKeyValue();

   fSelection = new TEveSelection("Global Selection");
   fSelection->IncDenyDestroy();
   fHighlight = new TEveSelection("Global Highlight");
   fHighlight->SetHighlightMode();
   fHighlight->IncDenyDestroy();

   fOrphanage = new TEveElementList("Global Orphanage");
   fOrphanage->IncDenyDestroy();

   fRedrawTimer.Connect("Timeout()", "TEveManager", this, "DoRedraw3D()");
   fMacroFolder = new TFolder("EVE", "Visualization macros");
   gROOT->GetListOfBrowsables()->Add(fMacroFolder);


   // Build GUI
   fBrowser   = new TEveBrowser(w, h);
   fStatusBar = fBrowser->GetStatusBar();
   fBrowser->Connect("CloseWindow()", "TEveManager", this, "CloseEveWindow()");

   // ListTreeEditor
   fBrowser->StartEmbedding(0);
   fLTEFrame = new TEveGListTreeEditorFrame;
   fBrowser->StopEmbedding("Eve");
   fLTEFrame->ConnectSignals();
   fEditor = fLTEFrame->fEditor;

   // GL viewer
   fBrowser->ShowCloseTab(kFALSE);
   fBrowser->StartEmbedding(1);
   TGLSAViewer* glv = new TGLSAViewer(gClient->GetRoot(), 0, fEditor);
   //glv->GetFrame()->SetCleanup(kNoCleanup);
   glv->ToggleEditObject();
   fBrowser->StopEmbedding();
   fBrowser->SetTabTitle("GLViewer", 1);
   fBrowser->ShowCloseTab(kTRUE);

   // Finalize it
   fBrowser->InitPlugins(opt);
   if (map_window)
      fBrowser->MapWindow();

   // --------------------------------

   fViewers = new TEveViewerList("Viewers");
   fViewers->IncDenyDestroy();
   AddToListTree(fViewers, kTRUE);

   fViewer  = new TEveViewer("GLViewer");
   fViewer->SetGLViewer(glv);
   fViewer->IncDenyDestroy();
   AddElement(fViewer, fViewers);

   fViewers->Connect();

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

   EditElement(fViewer);

   gSystem->ProcessEvents();
}

//______________________________________________________________________________
TEveManager::~TEveManager()
{
   // Destructor.

   fGlobalScene->DecDenyDestroy();
   fEventScene->DecDenyDestroy();
   fScenes->DestroyScenes();
   fScenes->DecDenyDestroy();
   fScenes->Destroy();
   fScenes = 0;

   fViewer->DecDenyDestroy();
   fViewers->DestroyElements();
   fViewers->DecDenyDestroy();
   fViewers->Destroy();
   fViewers = 0;

   fOrphanage->DecDenyDestroy();
   fHighlight->DecDenyDestroy();
   fSelection->DecDenyDestroy();

   delete fMacroFolder;
   delete fGeometryAliases;
   delete fGeometries;
   delete fVizDB;
   delete fExcHandler;
}

//______________________________________________________________________________
void TEveManager::ClearOrphanage()
{
   // Clear the orphanage.

   Bool_t old_state = fUseOrphanage;
   fUseOrphanage = kFALSE;
   fOrphanage->DestroyElements();
   fUseOrphanage = old_state;
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

   if (embed)
   {
      fBrowser->ShowCloseTab(kFALSE);
      fBrowser->StartEmbedding(1);
   }
   v->SpawnGLViewer(gClient->GetRoot(), embed ? fEditor : 0);
   v->IncDenyDestroy();
   if (embed)
   {
      fBrowser->StopEmbedding(), fBrowser->SetTabTitle(name, 1);
      fBrowser->ShowCloseTab(kTRUE);
   }
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

   static const TEveException eh("TEveManager::DoRedraw3D ");

   // printf("TEveManager::DoRedraw3D redraw triggered\n");

   // Process element visibility changes, mark relevant scenes as changed.
   {
      TEveElement::List_t scenes;
      for (TEveElement::Set_i i = fStampedElements.begin(); i != fStampedElements.end(); ++i)
      {
         if ((*i)->GetChangeBits() & TEveElement::kCBVisibility)
         {
            (*i)->CollectSceneParents(scenes);
         }
      }
      ScenesChanged(scenes);
   }

   // Process changes in scenes.
   fScenes ->ProcessSceneChanges(fDropLogicals, fStampedElements);
   fViewers->RepaintChangedViewers(fResetCameras, fDropLogicals);

   // Process changed elements again, update GUI (just editor so far,
   // but more can come).
   for (TEveElement::Set_i i = fStampedElements.begin(); i != fStampedElements.end(); ++i)
   {
      if (fEditor->GetModel() == (*i)->GetEditorObject(eh))
         EditElement((*i));

      (*i)->ClearStamps();
   }
   fStampedElements.clear();
   GetListTree()->ClearViewPort(); // Fix this when several list-trees can be added.

   fResetCameras = kFALSE;
   fDropLogicals = kFALSE;

   fTimerActive = kFALSE;
}

//______________________________________________________________________________
void TEveManager::FullRedraw3D(Bool_t resetCameras, Bool_t dropLogicals)
{
   // Perform 3D redraw of all scenes and viewers.

   fScenes ->RepaintAllScenes (dropLogicals);
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
      EditElement(element);

   if (update_scenes) {
      TEveElement::List_t scenes;
      element->CollectSceneParents(scenes);
      ScenesChanged(scenes);
   }

   if (redraw)
      Redraw3D();
}

//______________________________________________________________________________
void TEveManager::ScenesChanged(TEveElement::List_t& scenes)
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
void TEveManager::AddElement(TEveElement* element, TEveElement* parent)
{
   // Add an element. If parent is not specified it is added into
   // current event (which is created if does not exist).

   if (parent == 0) {
      if (fCurrentEvent == 0)
         AddEvent(new TEveEventManager("Event", "Auto-created event directory"));
      parent = fCurrentEvent;
   }

   parent->AddElement(element);
}

//______________________________________________________________________________
void TEveManager::AddGlobalElement(TEveElement* element, TEveElement* parent)
{
   // Add a global element, i.e. one that does not change on each
   // event, like geometry or projection manager.
   // If parent is not specified it is added to a global scene.

   if (parent == 0)
      parent = fGlobalScene;

   parent->AddElement(element);
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
      EditElement(0);

   if (fScenes)
      fScenes->DestroyElementRenderers(element);

   TEveElement::Set_i sei = fStampedElements.find(element);
   if (sei != fStampedElements.end())
      fStampedElements.erase(sei);

   if (element->fImpliedSelected > 0)
      fSelection->RemoveImpliedSelected(element);
   if (element->fImpliedHighlighted > 0)
      fHighlight->RemoveImpliedSelected(element);
}

/******************************************************************************/

//______________________________________________________________________________
void TEveManager::ElementSelect(TEveElement* element)
{
   // Select an element.
   // Now it only calls EditElement() - should also update selection state.

   if (element != 0)
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


//==============================================================================
// VizDB interface
//==============================================================================

//______________________________________________________________________________
Bool_t TEveManager::InsertVizDBEntry(const TString& tag, TEveElement* model,
                                     Bool_t replace, Bool_t update)
{
   // Insert a new visualization-parameter database entry. Returns
   // true if the element is inserted successfully.
   // If entry with the same key already exists the behaviour depends on the
   // 'replace' flag:
   //   true  - The old model is deleted and new one is inserted (default).
   //           Clients of the old model are transferred to the new one and
   //           if 'update' flag is true (default), the new model's parameters
   //           are assigned to all clients.
   //   false - The old model is kept, false is returned.
   //
   // If insert is successful, the ownership of the model-element is
   // transferred to the manager.

   TPair* pair = (TPair*) fVizDB->FindObject(tag);
   if (pair)
   {
      if (replace)
      {
         TEveElement* old_model = dynamic_cast<TEveElement*>(pair->Value());
         for (TEveElement::List_i i = old_model->BeginChildren(); i != old_model->EndChildren(); ++i)
         {
            (*i)->SetVizModel(model);
            if (update)
               (*i)->CopyVizParams(model);
         }
         old_model->DecDenyDestroy();
         old_model->Destroy();
         model->IncDenyDestroy();
         model->SetRnrChildren(kFALSE);
         pair->SetValue(dynamic_cast<TObject*>(model));
         return kTRUE;
      }
      else
      {
         return kFALSE;
      }
   }
   else
   {
      model->IncDenyDestroy();
      model->SetRnrChildren(kFALSE);
      fVizDB->Add(new TObjString(tag), dynamic_cast<TObject*>(model));
      return kTRUE;
   }
}

//______________________________________________________________________________
Bool_t TEveManager::InsertVizDBEntry(const TString& tag, TEveElement* model)
{
   // Insert a new visualization-parameter database entry with the default
   // parameters for replace and update, as specified by members
   // fVizDBReplace(default=kTRUE) and fVizDBUpdate(default=kTRUE).
   // See docs of the above function.

   return InsertVizDBEntry(tag, model, fVizDBReplace, fVizDBUpdate);
}

//______________________________________________________________________________
TEveElement* TEveManager::FindVizDBEntry(const TString& tag)
{
   // Find a visualization-parameter database entry corresponding to tag.
   // If the entry is not found 0 is returned.

   return dynamic_cast<TEveElement*>(fVizDB->GetValue(tag));
}

//______________________________________________________________________________
void TEveManager::LoadVizDB(const TString& filename, Bool_t replace, Bool_t update)
{
   // Load visualization-parameter database from file filename. The
   // replace, update arguments replace the values of fVizDBReplace
   // and fVizDBUpdate members for the duration of the macro
   // execution.

   Bool_t ex_replace = fVizDBReplace;
   Bool_t ex_update  = fVizDBUpdate;
   fVizDBReplace = replace;
   fVizDBUpdate  = update;

   LoadVizDB(filename);

   fVizDBReplace = ex_replace;
   fVizDBUpdate  = ex_update;
}

//______________________________________________________________________________
void TEveManager::LoadVizDB(const TString& filename)
{
   // Load visualization-parameter database from file filename.
   // State of data-members fVizDBReplace and fVizDBUpdate determine
   // how the registered entries are handled.

   TEveUtil::Macro(filename);
}

//______________________________________________________________________________
void TEveManager::SaveVizDB(const TString& filename)
{
   // Save visualization-parameter database to file filename.

   TPMERegexp re("(.+)\\.\\w+");
   if (re.Match(filename) != 2) {
      Error("SaveVizDB", "filename does not match required format '(.+)\\.\\w+'.");
      return;
   }

   ofstream out(filename, ios::out | ios::trunc);
   out << "void " << re[1] << "()\n";
   out << "{\n";
   out << "   TEveManager::Create();\n";

   ClearROOTClassSaved();

   Int_t       var_id = 0;
   TString     var_name;
   TIter       next(fVizDB);
   TObjString *key;
   while ((key = (TObjString*)next()))
   {
      TEveElement* mdl = dynamic_cast<TEveElement*>(fVizDB->GetValue(key));
      if (mdl)
      {
         var_name.Form("x%03d", var_id++);
         mdl->SaveVizParams(out, key->String(), var_name);
      }
      else
      {
         Warning("SaveVizDB", "Saving failed for key '%s'.", key->String().Data());
      }
   }

   out << "}\n";
   out.close();
}

//==============================================================================
// GeoManager, geometry-alias registration
//==============================================================================

//______________________________________________________________________________
TGeoManager* TEveManager::GetGeometry(const TString& filename)
{
   // Get geometry with given filename.
   // This is cached internally so the second time this function is
   // called with the same argument the same geo-manager is returned.
   // gGeoManager is set to the return value.

   static const TEveException eh("TEveManager::GetGeometry ");

   TString exp_filename = filename;
   gSystem->ExpandPathName(exp_filename);
   printf("%s loading: '%s' -> '%s'.\n", eh.Data(),
          filename.Data(), exp_filename.Data());

   gGeoManager = (TGeoManager*) fGeometries->GetValue(filename);
   if (!gGeoManager) {
      Bool_t locked = TGeoManager::IsLocked();
      if (locked) {
         Warning(eh, "TGeoManager is locked ... unlocking it.");
         TGeoManager::UnlockGeometry();
      }
      if (TGeoManager::Import(filename) == 0) {
         throw(eh + "TGeoManager::Import() failed for '" + exp_filename + "'.");
      }
      if (locked) {
         TGeoManager::LockGeometry();
      }

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

      fGeometries->Add(new TObjString(filename), gGeoManager);
   }
   return gGeoManager;
}

//______________________________________________________________________________
TGeoManager* TEveManager::GetGeometryByAlias(const TString& alias)
{
   // Get geometry with given alias.
   // The alias must be registered via RegisterGeometryAlias().

   static const TEveException eh("TEveManager::GetGeometry ");

   TObjString* full_name = (TObjString*) fGeometryAliases->GetValue(alias);
   if (!full_name)
      throw(eh + "geometry alias '" + alias + "' not registered.");
   return GetGeometry(full_name->String());
}

//______________________________________________________________________________
TGeoManager* TEveManager::GetDefaultGeometry()
{
   // Get the default geometry.
   // It should be registered via RegisterGeometryName("Default", <URL>).

   return GetGeometryByAlias("Default");
}

//______________________________________________________________________________
void TEveManager::RegisterGeometryAlias(const TString& alias, const TString& filename)
{
   // Register 'name' as an alias for geometry file 'filename'.
   // The old aliases are silently overwritten.
   // After that the geometry can be retrieved also by calling:
   //   gEve->GetGeometryByName(name);

   fGeometryAliases->Add(new TObjString(alias), new TObjString(filename));
}

//==============================================================================

//______________________________________________________________________________
void TEveManager::SetStatusLine(const char* text)
{
   // Set the text in the right side of browser's status bar.

   fBrowser->SetStatusText(text, 1);
}

//______________________________________________________________________________
void TEveManager::ClearROOTClassSaved()
{
   // Work-around uber ugly hack used in SavePrimitive and co.

   TIter   nextcl(gROOT->GetListOfClasses());
   TClass *cls;
   while((cls = (TClass *)nextcl()))
   {
      cls->ResetBit(TClass::kClassSaved);
   }
}

//______________________________________________________________________________
void TEveManager::CloseEveWindow()
{
   // Close button haas been clicked on EVE main window (browser).
   // Cleanup and terminate application.

   TGMainFrame *mf = (TGMainFrame*) gTQSender;
   TEveBrowser *eb = dynamic_cast<TEveBrowser*>(mf);
   if (eb == fBrowser)
   {
      mf->DontCallClose();
      Terminate();
      gApplication->Terminate();
   }
}


/******************************************************************************/
// Static initialization.
/******************************************************************************/

//______________________________________________________________________________
TEveManager* TEveManager::Create(Bool_t map_window, Option_t* opt)
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
      gEve = new TEveManager(w, h, map_window, opt);
   }
   return gEve;
}

//______________________________________________________________________________
void TEveManager::Terminate()
{
   // Properly terminate global TEveManager.

   if (!gEve) return;

   TGLViewer                *v  = gEve->fViewer->GetGLViewer();
   TEveGListTreeEditorFrame *lf = gEve->fLTEFrame;
   TEveBrowser              *b  = gEve->GetBrowser();

   delete gEve;

   delete v;
   delete lf;
   delete b;

   gEve = 0;
}

//==============================================================================
//==============================================================================
// TEveManager::TExceptionHandler
//==============================================================================

//______________________________________________________________________________
//
// Exception handler for Eve exceptions.

ClassImp(TEveManager::TExceptionHandler);

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
