// @(#)root/eve:$Id$
// Authors: Matevz Tadel & Alja Mrak-Tadel: 2006, 2007

/*************************************************************************
 * Copyright (C) 1995-2007, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#include "TEveViewer.h"
#include "TEveScene.h"
#include "TEveSceneInfo.h"

#include "TEveManager.h"
#include "TEveSelection.h"

#include "TGLSAViewer.h"
#include "TGLEmbeddedViewer.h"
#include "TGLScenePad.h"

#include "TGLPhysicalShape.h" // For handling OnMouseIdle signal
#include "TGLLogicalShape.h"  // For handling OnMouseIdle signal
#include "TGLEventHandler.h"

//==============================================================================
//==============================================================================
// TEveViewer
//==============================================================================

//______________________________________________________________________________
//
// Eve representation of TGLViewer.
//
// The gl-viewer is owned by this class and is deleted in destructor.
//
// The frame is not deleted, it is expected that the gl-viewer implementation
// will delete that. TGLSAViewer and TGEmbeddedViewer both do so.
// This could be an optional argument to SetGLViewer. A frame could be
// passed as well.

ClassImp(TEveViewer);

//______________________________________________________________________________
TEveViewer::TEveViewer(const Text_t* n, const Text_t* t) :
   TEveWindowFrame(0, n, t),
   fGLViewer      (0),
   fGLViewerFrame (0)
{
   // Constructor.
   // The base-class TEveWindowFrame is constructed without a frame so
   // a default composite-frame is instantiated and stored in fGUIFrame.
   // Cleanup is set to no-cleanup as viewers need to be zapped with some
   // more care.

   SetChildClass(TEveSceneInfo::Class());
   fGUIFrame->SetCleanup(kNoCleanup); // the gl-viewer's frame deleted elsewhere.
}

//______________________________________________________________________________
TEveViewer::~TEveViewer()
{
   // Destructor.

   fGLViewerFrame->UnmapWindow();
   GetGUICompositeFrame()->RemoveFrame(fGLViewerFrame);
   fGLViewerFrame->ReparentWindow(gClient->GetDefaultRoot());
   TTimer::SingleShot(150, "TGLViewer", fGLViewer, "Delete()");
}

/******************************************************************************/

//______________________________________________________________________________
const TGPicture* TEveViewer::GetListTreeIcon(Bool_t)
{
   // Return TEveViewer icon.

   return TEveElement::fgListTreeIcons[1];
}

//______________________________________________________________________________
void TEveViewer::SetGLViewer(TGLViewer* viewer, TGFrame* frame)
{
   // Set TGLViewer that is represented by this object.
   // The old gl-viewer is deleted.

   delete fGLViewer;
   fGLViewer      = viewer;
   fGLViewerFrame = frame;

   fGLViewer->SetSmartRefresh(kTRUE);
   fGLViewer->SetResetCameraOnDoubleClick(kFALSE);
}

//______________________________________________________________________________
void TEveViewer::SpawnGLViewer(TGedEditor* ged)
{
   // Spawn new GLViewer and adopt it.

   static const TEveException kEH("TEveViewer::SpawnGLViewer ");

   TGCompositeFrame* cf = GetGUICompositeFrame();

   TGLSAViewer* v = new TGLSAViewer(cf, 0, ged);
   v->ToggleEditObject();
   SetGLViewer(v, v->GetFrame());

   cf->AddFrame(fGLViewerFrame, new TGLayoutHints(kLHintsNormal | kLHintsExpandX | kLHintsExpandY));
}

//______________________________________________________________________________
void TEveViewer::SpawnGLEmbeddedViewer(Int_t border)
{
   // Spawn new GLViewer and adopt it.

   static const TEveException kEH("TEveViewer::SpawnGLEmbeddedViewer ");

   TGCompositeFrame* cf = GetGUICompositeFrame();

   TGLEmbeddedViewer* v = new TGLEmbeddedViewer(cf, 0, border);
   SetGLViewer(v, v->GetFrame());

   cf->AddFrame(fGLViewerFrame, new TGLayoutHints(kLHintsNormal | kLHintsExpandX | kLHintsExpandY));

   fGLViewerFrame->MapWindow();
}

//______________________________________________________________________________
void TEveViewer::Redraw(Bool_t resetCameras)
{
   // Redraw viewer immediately.

   if (resetCameras) fGLViewer->PostSceneBuildSetup(kTRUE);
   fGLViewer->RequestDraw(TGLRnrCtx::kLODHigh);
}

/******************************************************************************/

//______________________________________________________________________________
void TEveViewer::AddScene(TEveScene* scene)
{
   // Add 'scene' to the list of scenes.

   static const TEveException eh("TEveViewer::AddScene ");

   TGLSceneInfo* glsi = fGLViewer->AddScene(scene->GetGLScene());
   if (glsi != 0) {
      TEveSceneInfo* si = new TEveSceneInfo(this, scene, glsi);
      AddElement(si);
   } else {
      throw(eh + "scene already in the viewer.");
   }
}

//______________________________________________________________________________
void TEveViewer::RemoveElementLocal(TEveElement* el)
{
   // Remove element 'el' from the list of children and also remove
   // appropriate GLScene from GLViewer's list of scenes.
   // Virtual from TEveElement.

   fGLViewer->RemoveScene(((TEveSceneInfo*)el)->GetGLScene());
}

//______________________________________________________________________________
void TEveViewer::RemoveElementsLocal()
{
   // Remove all children, forwarded to GLViewer.
   // Virtual from TEveElement.

   fGLViewer->RemoveAllScenes();
}

//______________________________________________________________________________
TObject* TEveViewer::GetEditorObject(const TEveException& eh) const
{
   // Object to be edited when this is selected, returns the TGLViewer.
   // Virtual from TEveElement.

   if (!fGLViewer)
      throw(eh + "fGLViewer not set.");
   return fGLViewer;
}

//______________________________________________________________________________
Bool_t TEveViewer::HandleElementPaste(TEveElement* el)
{
   // Receive a pasted object. TEveViewer only accepts objects of
   // class TEveScene.
   // Virtual from TEveElement.

   static const TEveException eh("TEveViewer::HandleElementPaste ");

   TEveScene* scene = dynamic_cast<TEveScene*>(el);
   if (scene != 0) {
      AddScene(scene);
      return kTRUE;
   } else {
      Warning(eh.Data(), "class TEveViewer only accepts TEveScene paste argument.");
      return kFALSE;
   }
}


/******************************************************************************/
/******************************************************************************/
// TEveViewerList
/******************************************************************************/

//______________________________________________________________________________
//
// List of Viewers providing common operations on TEveViewer collections.

ClassImp(TEveViewerList);

//______________________________________________________________________________
TEveViewerList::TEveViewerList(const Text_t* n, const Text_t* t) :
   TEveElementList(n, t),
   fShowTooltip   (kTRUE)
{
   // Constructor.

   SetChildClass(TEveViewer::Class());
}

//==============================================================================

//______________________________________________________________________________
void TEveViewerList::AddElement(TEveElement* el)
{
   // Call base-class implementation.
   // If compund is open and compound of the new element is not set,
   // the el's compound is set to this.

   TEveElementList::AddElement(el);
   el->IncParentIgnoreCnt();
}

//______________________________________________________________________________
void TEveViewerList::RemoveElementLocal(TEveElement* el)
{
   // Decompoundofy el, call base-class version.

   el->DecParentIgnoreCnt();
   TEveElementList::RemoveElementLocal(el);
}

//______________________________________________________________________________
void TEveViewerList::RemoveElementsLocal()
{
   // Decompoundofy children, call base-class version.

   for (List_i i=fChildren.begin(); i!=fChildren.end(); ++i)
   {
      (*i)->DecParentIgnoreCnt();
   }

   TEveElementList::RemoveElementsLocal();
}

//==============================================================================

//______________________________________________________________________________
void TEveViewerList::Connect()
{
   // Connect to TGLViewer class-signals.

   TQObject::Connect("TGLViewer", "MouseOver(TGLPhysicalShape*,UInt_t)",
                     "TEveViewerList", this, "OnMouseOver(TGLPhysicalShape*,UInt_t)");
   TQObject::Connect("TGLViewer", "Clicked(TObject*,UInt_t,UInt_t)",
                     "TEveViewerList", this, "OnClicked(TObject*,UInt_t,UInt_t)");
}

/******************************************************************************/

//______________________________________________________________________________
void TEveViewerList::RepaintChangedViewers(Bool_t resetCameras, Bool_t dropLogicals)
{
   // Repaint viewers that are tagged as changed.

   for (List_i i=fChildren.begin(); i!=fChildren.end(); ++i)
   {
      TGLViewer* glv = ((TEveViewer*)*i)->GetGLViewer();
      if (glv->IsChanged())
      {
         // printf(" TEveViewer '%s' changed ... reqesting draw.\n", (*i)->GetObject()->GetName());

         if (resetCameras) glv->PostSceneBuildSetup(kTRUE);
         if (dropLogicals) glv->SetSmartRefresh(kFALSE);

         glv->RequestDraw(TGLRnrCtx::kLODHigh);

         if (dropLogicals) glv->SetSmartRefresh(kTRUE);
      }
   }
}

//______________________________________________________________________________
void TEveViewerList::RepaintAllViewers(Bool_t resetCameras, Bool_t dropLogicals)
{
   // Repaint all viewers.

   for (List_i i=fChildren.begin(); i!=fChildren.end(); ++i)
   {
      TGLViewer* glv = ((TEveViewer*)*i)->GetGLViewer();

      // printf(" TEveViewer '%s' sending redraw reqest.\n", (*i)->GetObject()->GetName());

      if (resetCameras) glv->PostSceneBuildSetup(kTRUE);
      if (dropLogicals) glv->SetSmartRefresh(kFALSE);

      glv->RequestDraw(TGLRnrCtx::kLODHigh);

      if (dropLogicals) glv->SetSmartRefresh(kTRUE);
   }
}

/******************************************************************************/

//______________________________________________________________________________
void TEveViewerList::SceneDestructing(TEveScene* scene)
{
   // Callback done from a TEveScene destructor allowing proper
   // removal of the scene from affected viewers.

   for (List_i i=fChildren.begin(); i!=fChildren.end(); ++i)
   {
      TEveViewer* viewer = (TEveViewer*) *i;
      List_i j = viewer->BeginChildren();
      while (j != viewer->EndChildren())
      {
         TEveSceneInfo* sinfo = (TEveSceneInfo*) *j;
         ++j;
         if (sinfo->GetScene() == scene)
            viewer->RemoveElement(sinfo);
      }
   }
}


/******************************************************************************/
// Processing of events from TGLViewers.
/******************************************************************************/


//______________________________________________________________________________
void TEveViewerList::OnMouseOver(TGLPhysicalShape *pshape, UInt_t state)
{
   // Slot for global TGLViewer::MouseOver() signal.
   //
   // The attempt is made to determine the TEveElement being
   // represented by the physical shape and global higlight is updated
   // accordingly.
   //
   // If TEveElement::IsPickable() returns false, the element is not
   // highlighted.

   if (state & kKeyShiftMask || state & kKeyMod1Mask)
      return;

   TObject     *obj = 0;
   TEveElement *el  = 0;

   if (pshape)
   {
      TGLLogicalShape* lshape = const_cast<TGLLogicalShape*>(pshape->GetLogical());
      obj = lshape->GetExternal();
      el  = dynamic_cast<TEveElement*>(obj);
   }

   if (el && !el->IsPickable())
      el = 0;
   gEve->GetHighlight()->UserPickedElement(el, kFALSE);

   if (fShowTooltip)
   {
      TGLViewer       *glw = dynamic_cast<TGLViewer*>((TQObject*) gTQSender);
      TGLEventHandler *glh = (TGLEventHandler*) glw->GetEventHandler();
      if (gEve->GetHighlight()->NumChildren() == 1)
      {
         TString title(gEve->GetHighlight()->FirstChild()->GetElementTitle());
         if ( ! title.IsNull())
            glh->TriggerTooltip(title);
      }
      else
      {
         glh->RemoveTooltip();
      }
   }
}

//______________________________________________________________________________
void TEveViewerList::OnClicked(TObject *obj, UInt_t button, UInt_t state)
{
   // Slot for global TGLViewer::Clicked().
   //
   // The obj is dyn-casted to the TEveElement and global selection is
   // updated accordingly.
   //
   // If TEveElement::IsPickable() returns false, the element is not
   // selected.

   if (button != kButton1 || state & kKeyShiftMask || state & kKeyMod1Mask)
      return;

   TEveElement* el = dynamic_cast<TEveElement*>(obj);
   if (el && !el->IsPickable())
      el = 0;
   gEve->GetSelection()->UserPickedElement(el, state & kKeyControlMask);
}
