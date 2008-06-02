// @(#)root/gl:$Id$
// Author:  Richard Maunder  25/05/2005

/*************************************************************************
 * Copyright (C) 1995-2000, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#include "TGLViewer.h"
#include "TGLIncludes.h"
#include "TGLStopwatch.h"
#include "TGLRnrCtx.h"
#include "TGLSelectBuffer.h"
#include "TGLLightSet.h"
#include "TGLClip.h"
#include "TGLManipSet.h"

#include "TGLScenePad.h"
#include "TGLLogicalShape.h"
#include "TGLPhysicalShape.h"
#include "TGLObject.h"
#include "TGLStopwatch.h"
#include "TBuffer3D.h"
#include "TBuffer3DTypes.h"

#include "TGLOutput.h"

#include "TVirtualPad.h" // Remove when pad removed - use signal
#include "TAtt3D.h"      // Remove when PadPaint delegated to PadScene.
#include "TVirtualX.h"

#include "TMath.h"
#include "TColor.h"
#include "TError.h"
#include "TClass.h"
#include "TROOT.h"

// For event type translation ExecuteEvent
#include "Buttons.h"
#include "GuiTypes.h"

#include "TVirtualGL.h"

#include "TGLWidget.h"
#include "TGLViewerEditor.h"

#include "KeySymbols.h"
#include "TContextMenu.h"
#include "TImage.h"


#ifndef GL_BGRA
#define GL_BGRA GL_BGRA_EXT
#endif

//______________________________________________________________________
// TGLViewer
//
// Base GL viewer object - used by both standalone and embedded (in pad)
// GL. Contains core viewer objects :
//
// GL scene - collection of main drawn objects - see TGLStdScene
// Cameras (fXXXXCamera) - ortho and perspective cameras - see TGLCamera
// Clipping (fClipXXXX) - collection of clip objects - see TGLClip
// Manipulators (fXXXXManip) - collection of manipulators - see TGLManip
//
// It maintains the current active draw styles, clipping object,
// manipulator, camera etc.
//
// TGLViewer is 'GUI free' in that it does not derive from any ROOT GUI
// TGFrame etc - see TGLSAViewer for this. However it contains GUI
// GUI style methods HandleButton() etc to which GUI events can be
// directed from standalone frame or embedding pad to perform
// interaction.
//
// For embedded (pad) GL this viewer is created directly by plugin
// manager. For standalone the derived TGLSAViewer is.
//

ClassImp(TGLViewer)

//______________________________________________________________________________
TGLViewer::TGLViewer(TVirtualPad * pad, Int_t x, Int_t y,
                     Int_t width, Int_t height) :
   fPad(pad),
   fContextMenu(0),
   fPerspectiveCameraXOZ(TGLVector3(-1.0, 0.0, 0.0), TGLVector3(0.0, 1.0, 0.0)), // XOZ floor
   fPerspectiveCameraYOZ(TGLVector3( 0.0,-1.0, 0.0), TGLVector3(1.0, 0.0, 0.0)), // YOZ floor
   fPerspectiveCameraXOY(TGLVector3(-1.0, 0.0, 0.0), TGLVector3(0.0, 0.0, 1.0)), // XOY floor
   fOrthoXOYCamera (TGLOrthoCamera::kXOY,  TGLVector3( 0.0, 0.0, 1.0), TGLVector3(0.0, 1.0, 0.0)), // Looking down  Z axis,  X horz, Y vert
   fOrthoXOZCamera (TGLOrthoCamera::kXOZ,  TGLVector3( 0.0,-1.0, 0.0), TGLVector3(0.0, 0.0, 1.0)), // Looking along Y axis,  X horz, Z vert
   fOrthoZOYCamera (TGLOrthoCamera::kZOY,  TGLVector3(-1.0, 0.0, 0.0), TGLVector3(0.0, 1.0, 0.0)), // Looking along X axis,  Z horz, Y vert
   fOrthoXnOYCamera(TGLOrthoCamera::kXnOY, TGLVector3( 0.0, 0.0,-1.0), TGLVector3(0.0, 1.0, 0.0)), // Looking along Z axis, -X horz, Y vert
   fOrthoXnOZCamera(TGLOrthoCamera::kXnOZ, TGLVector3( 0.0, 1.0, 0.0), TGLVector3(0.0, 0.0, 1.0)), // Looking down  Y axis, -X horz, Z vert
   fOrthoZnOYCamera(TGLOrthoCamera::kZnOY, TGLVector3( 1.0, 0.0, 0.0), TGLVector3(0.0, 1.0, 0.0)), // Looking down  X axis, -Z horz, Y vert
   fCurrentCamera(&fPerspectiveCameraXOZ),

   fLightSet          (0),
   fClipSet           (0),
   fSelectedPShapeRef (0),
   fCurrentOvlElm     (0),

   fEventHandler(0),
   fPushAction(kPushStd), fDragAction(kDragNone),
   fRedrawTimer(0),
   fMaxSceneDrawTimeHQ(5000),
   fMaxSceneDrawTimeLQ(100),
   fClearColor(1),
   fAxesType(TGLUtil::kAxesNone),
   fAxesDepthTest(kTRUE),
   fReferenceOn(kFALSE),
   fReferencePos(0.0, 0.0, 0.0),
   fDrawCameraCenter(kFALSE),
   fCameraMarkup(0),
   fInitGL(kFALSE),
   fSmartRefresh(kFALSE),
   fDebugMode(kFALSE),
   fIsPrinting(kFALSE),
   fPictureFileName("viewer.jpg"),
   fGLWidget(0),
   fGLDevice(-1),
   fGLCtxId(0),
   fIgnoreSizesOnUpdate(kFALSE),
   fResetCamerasOnUpdate(kTRUE),
   fResetCamerasOnNextUpdate(kFALSE),
   fResetCameraOnDoubleClick(kTRUE)
{
   // Construct the viewer object, with following arguments:
   //    'pad' - external pad viewer is bound to
   //    'x', 'y' - initial top left position
   //    'width', 'height' - initial width/height

   InitSecondaryObjects();

   SetViewport(x, y, width, height);
}

//______________________________________________________________________________
TGLViewer::TGLViewer(TVirtualPad * pad) :
   fPad(pad),
   fContextMenu(0),
   fPerspectiveCameraXOZ(TGLVector3(-1.0, 0.0, 0.0), TGLVector3(0.0, 1.0, 0.0)), // XOZ floor
   fPerspectiveCameraYOZ(TGLVector3( 0.0,-1.0, 0.0), TGLVector3(1.0, 0.0, 0.0)), // YOZ floor
   fPerspectiveCameraXOY(TGLVector3(-1.0, 0.0, 0.0), TGLVector3(0.0, 0.0, 1.0)), // XOY floor
   fOrthoXOYCamera (TGLOrthoCamera::kXOY,  TGLVector3( 0.0, 0.0, 1.0), TGLVector3(0.0, 1.0, 0.0)), // Looking down  Z axis,  X horz, Y vert
   fOrthoXOZCamera (TGLOrthoCamera::kXOZ,  TGLVector3( 0.0,-1.0, 0.0), TGLVector3(0.0, 0.0, 1.0)), // Looking along Y axis,  X horz, Z vert
   fOrthoZOYCamera (TGLOrthoCamera::kZOY,  TGLVector3(-1.0, 0.0, 0.0), TGLVector3(0.0, 1.0, 0.0)), // Looking along X axis,  Z horz, Y vert
   fOrthoXnOYCamera(TGLOrthoCamera::kXnOY, TGLVector3( 0.0, 0.0,-1.0), TGLVector3(0.0, 1.0, 0.0)), // Looking along Z axis, -X horz, Y vert
   fOrthoXnOZCamera(TGLOrthoCamera::kXnOZ, TGLVector3( 0.0, 1.0, 0.0), TGLVector3(0.0, 0.0, 1.0)), // Looking down  Y axis, -X horz, Z vert
   fOrthoZnOYCamera(TGLOrthoCamera::kZnOY, TGLVector3( 1.0, 0.0, 0.0), TGLVector3(0.0, 1.0, 0.0)), // Looking down  X axis, -Z horz, Y vert
   fCurrentCamera(&fPerspectiveCameraXOZ),

   fLightSet          (0),
   fClipSet           (0),
   fSelectedPShapeRef (0),
   fCurrentOvlElm     (0),

   fEventHandler(0),
   fPushAction(kPushStd), fDragAction(kDragNone),
   fRedrawTimer(0),
   fMaxSceneDrawTimeHQ(5000),
   fMaxSceneDrawTimeLQ(100),
   fClearColor(1),
   fAxesType(TGLUtil::kAxesNone),
   fAxesDepthTest(kTRUE),
   fReferenceOn(kFALSE),
   fReferencePos(0.0, 0.0, 0.0),
   fDrawCameraCenter(kFALSE),
   fCameraMarkup(0),
   fInitGL(kFALSE),
   fSmartRefresh(kFALSE),
   fDebugMode(kFALSE),
   fIsPrinting(kFALSE),
   fPictureFileName("viewer.jpg"),
   fGLWidget(0),
   fGLDevice(fPad->GetGLDevice()),
   fGLCtxId(0),
   fIgnoreSizesOnUpdate(kFALSE),
   fResetCamerasOnUpdate(kTRUE),
   fResetCamerasOnNextUpdate(kFALSE),
   fResetCameraOnDoubleClick(kTRUE)
{
   //gl-embedded viewer's ctor
   // Construct the viewer object, with following arguments:
   //    'pad' - external pad viewer is bound to
   //    'x', 'y' - initial top left position
   //    'width', 'height' - initial width/height

   InitSecondaryObjects();

   if (fGLDevice != -1) {
      // For the moment instantiate a fake context identity.
      fGLCtxId = new TGLContextIdentity;
      fGLCtxId->AddRef(0);
      Int_t viewport[4] = {0};
      gGLManager->ExtractViewport(fGLDevice, viewport);
      SetViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
   }
}

//______________________________________________________________________________
void TGLViewer::InitSecondaryObjects()
{
   // Common initialization.

   fLightSet = new TGLLightSet;
   fClipSet  = new TGLClipSet;  fOverlay.push_back(fClipSet);

   fSelectedPShapeRef = new TGLManipSet; fOverlay.push_back(fSelectedPShapeRef);
   fSelectedPShapeRef->SetDrawBBox(kTRUE);

   fCameraMarkup = new TGLCameraMarkupStyle;

   fRedrawTimer = new TGLRedrawTimer(*this);
}

//______________________________________________________________________________
TGLViewer::~TGLViewer()
{
   // Destroy viewer object.

   delete fLightSet;
   delete fClipSet;
   delete fSelectedPShapeRef;
   delete fCameraMarkup;

   delete fContextMenu;
   delete fRedrawTimer;

   if (fEventHandler) {
      fGLWidget->SetEventHandler(0);
      delete fEventHandler;
   }

   if (fPad)
      fPad->ReleaseViewer3D();
   if (fGLDevice != -1)
      fGLCtxId->Release(0);
}


//______________________________________________________________________________
void TGLViewer::PadPaint(TVirtualPad* pad)
{
   // Entry point for updating viewer contents via VirtualViewer3D
   // interface.
   // We search and forward the request to appropriate TGLScenePad.
   // If it is not found we create a new TGLScenePad so this can
   // potentially also be used for registration of new pads.

   TGLScenePad* scenepad = 0;
   for (SceneInfoList_i si = fScenes.begin(); si != fScenes.end(); ++si)
   {
      scenepad = dynamic_cast<TGLScenePad*>((*si)->GetScene());
      if (scenepad && scenepad->GetPad() == pad)
         break;
      scenepad = 0;
   }
   if (scenepad == 0)
   {
      scenepad = new TGLScenePad(pad);
      AddScene(scenepad);
   }

   scenepad->PadPaintFromViewer(this);

   PostSceneBuildSetup(fResetCamerasOnNextUpdate || fResetCamerasOnUpdate);
   fResetCamerasOnNextUpdate = kFALSE;

   RequestDraw();
}


/**************************************************************************/
/**************************************************************************/

//______________________________________________________________________________
void TGLViewer::UpdateScene()
{
   // Force update of pad-scenes. Eventually this could be generalized
   // to all scene-types via a virtual function in TGLSceneBase.

   // Cancel any pending redraw timer.
   fRedrawTimer->Stop();

   for (SceneInfoList_i si = fScenes.begin(); si != fScenes.end(); ++si)
   {
      TGLScenePad* scenepad = dynamic_cast<TGLScenePad*>((*si)->GetScene());
      if (scenepad)
         scenepad->PadPaintFromViewer(this);
   }

   PostSceneBuildSetup(fResetCamerasOnNextUpdate || fResetCamerasOnUpdate);
   fResetCamerasOnNextUpdate = kFALSE;

   RequestDraw();
}

//______________________________________________________________________________
void TGLViewer::ResetCurrentCamera()
{
   // Resets position/rotation of current camera to default values.

   MergeSceneBBoxes(fOverallBoundingBox);
   CurrentCamera().Setup(fOverallBoundingBox, kTRUE);
}

//______________________________________________________________________________
void TGLViewer::SetupCameras(Bool_t reset)
{
   // Setup cameras for current bounding box.

   if (IsLocked()) {
      Error("TGLViewer::SetupCameras", "expected kUnlocked, found %s", LockName(CurrentLock()));
      return;
   }

   // Setup cameras if scene box is not empty
   const TGLBoundingBox & box = fOverallBoundingBox;
   if (!box.IsEmpty()) {
      fPerspectiveCameraYOZ.Setup(box, reset);
      fPerspectiveCameraXOZ.Setup(box, reset);
      fPerspectiveCameraXOY.Setup(box, reset);
      fOrthoXOYCamera.Setup(box, reset);
      fOrthoXOZCamera.Setup(box, reset);
      fOrthoZOYCamera.Setup(box, reset);
   }
}

//______________________________________________________________________________
void TGLViewer::PostSceneBuildSetup(Bool_t resetCameras)
{
   // Perform post scene-build setup.

   MergeSceneBBoxes(fOverallBoundingBox);
   SetupCameras(resetCameras);

   // Set default reference to center
   fReferencePos.Set(fOverallBoundingBox.Center());
   RefreshPadEditor(this);
}


/**************************************************************************/
/**************************************************************************/

//______________________________________________________________________________
void TGLViewer::InitGL()
{
   // Initialise GL state if not already done
   if (fInitGL) {
      Error("TGLViewer::InitGL", "GL already initialised");
   }

   // GL initialisation
   glEnable(GL_LIGHTING);
   glEnable(GL_DEPTH_TEST);
   glEnable(GL_CULL_FACE);
   glCullFace(GL_BACK);
   glClearColor(0.f, 0.f, 0.f, 1.f);
   glClearDepth(1.0);
   glColorMaterial(GL_FRONT_AND_BACK, GL_DIFFUSE);
   glEnable(GL_COLOR_MATERIAL);
   glMaterialf(GL_BACK, GL_SHININESS, 0.0);
   glPolygonMode(GL_FRONT, GL_FILL);
   glDisable(GL_BLEND);

   glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_TRUE);
   Float_t lmodelAmb[] = {0.5f, 0.5f, 1.f, 1.f};
   glLightModelfv(GL_LIGHT_MODEL_AMBIENT, lmodelAmb);
   glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_FALSE);

   glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
   glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

   TGLUtil::CheckError("TGLViewer::InitGL");
   fInitGL = kTRUE;
}

//______________________________________________________________________________
void TGLViewer::RequestDraw(Short_t LODInput)
{
   // Post request for redraw of viewer at level of detail 'LOD'
   // Request is directed via cross thread gVirtualGL object.

   fRedrawTimer->Stop();
   // Ignore request if GL window or context not yet availible - we
   // will get redraw later
   if (!fGLWidget && fGLDevice == -1) {
      fRedrawTimer->RequestDraw(100, LODInput);
      return;
   }

   // Take scene draw lock - to be revisited
   if ( ! TakeLock(kDrawLock)) {
      // If taking drawlock fails the previous draw is still in progress
      // set timer to do this one later
      if (gDebug>3) {
         Info("TGLViewer::RequestDraw", "viewer locked - requesting another draw.");
      }
      fRedrawTimer->RequestDraw(100, LODInput);
      return;
   }
   fLOD = LODInput;

   if (!gVirtualX->IsCmdThread())
      gROOT->ProcessLineFast(Form("((TGLViewer *)0x%x)->DoDraw()", this));
   else
      DoDraw();
}

//______________________________________________________________________________
void TGLViewer::PreRender()
{
   // Initialize objects that influence rendering.
   // Called before every render.

   fCamera = fCurrentCamera;
   fClip   = fClipSet->GetCurrentClip();
   if (fGLDevice != -1)
   {
      fRnrCtx->SetGLCtxIdentity(fGLCtxId);
      fGLCtxId->DeleteGLResources();
   }

   TGLViewerBase::PreRender();

   // Setup lighting
   fLightSet->StdSetupLights(fOverallBoundingBox, *fCamera, fDebugMode);
   fClipSet->SetupClips(fOverallBoundingBox);
}

//______________________________________________________________________________
void TGLViewer::DoDraw()
{
   // Draw out the the current viewer/scene

   // Locking mainly for Win32 multi thread safety - but no harm in all using it
   // During normal draws a draw lock is taken in other thread (Win32) in RequestDraw()
   // to ensure thread safety. For PrintObjects repeated Draw() calls are made.
   // If no draw lock taken get one now.

   fRedrawTimer->Stop();

   if (CurrentLock() != kDrawLock) {
      if ( ! TakeLock(kDrawLock)) {
         Error("TGLViewer::DoDraw", "viewer is %s", LockName(CurrentLock()));
         return;
      }
   }

   if (fGLDevice == -1 && (fViewport.Width() <= 0 || fViewport.Height() <= 0)) {
      ReleaseLock(kDrawLock);
      if (gDebug > 2) {
	 Info("TGLViewer::DoDraw()", "zero surface area, draw skipped.");
      }
      return;
   }

   if (fGLDevice != -1) {
      Int_t viewport[4] = {};
      gGLManager->ExtractViewport(fGLDevice, viewport);
      SetViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
   }

   TGLStopwatch timer;
   if (gDebug>2) {
      timer.Start();
   }

   // Setup scene draw time
   fRnrCtx->SetRenderTimeOut(fLOD == TGLRnrCtx::kLODHigh ?
                             fMaxSceneDrawTimeHQ :
                             fMaxSceneDrawTimeLQ);
   fRnrCtx->StartStopwatch();

   // GL pre draw setup
   if (!fIsPrinting) PreDraw();

   PreRender();

   Render();

   DrawGuides();
   glClear(GL_DEPTH_BUFFER_BIT);
   RenderOverlay();
   DrawCameraMarkup();
   DrawDebugInfo();

   PostRender();
   PostDraw();

   fRnrCtx->StopStopwatch();

   ReleaseLock(kDrawLock);

   if (gDebug>2) {
      Info("TGLViewer::DoDraw()", "Took %f msec", timer.End());
   }


   // Check if further redraws are needed and schedule them.

   if (CurrentCamera().UpdateInterest(kFALSE)) {
      // Reset major view-dependant cache.
      ResetSceneInfos();
      fRedrawTimer->RequestDraw(0, fLOD);
   }

   if (fLOD != TGLRnrCtx::kLODHigh &&
       (fDragAction < kDragCameraRotate || fDragAction > kDragCameraDolly))
   {
      // Request final draw pass.
      fRedrawTimer->RequestDraw(100, TGLRnrCtx::kLODHigh);
   }
}

//______________________________________________________________________________
Bool_t TGLViewer::SavePicture(const TString &fileName)
{
   // Save current image in various formats (gif, gif+, jpg, png, eps, pdf).
   // 'gif+' will append image to an existng file (animated gif).
   // 'eps' and 'pdf' do not fully support transparency and texturing.
   // The viewer window most be fully contained within the desktop but
   // can be covered by other windows.
   // Returns false if something obvious goes wrong, true otherwise.

   if (fileName.EndsWith(".gif") || fileName.Contains("gif+") ||
            fileName.EndsWith(".jpg") || fileName.EndsWith(".png"))
   {
      if ( ! TakeLock(kDrawLock)) {
         Error("TGLViewer::SavePicture", "viewer locked - try later.");
         return kFALSE;
      }

      std::auto_ptr<TImage>gif(TImage::Create());

      fRnrCtx->SetGrabImage(kTRUE);

      fLOD = TGLRnrCtx::kLODHigh;

      if (!gVirtualX->IsCmdThread())
         gROOT->ProcessLineFast(Form("((TGLViewer *)0x%lx)->DoDraw()", this));
      else
         DoDraw();

      gif->FromGLBuffer(fRnrCtx->GetGrabbedImage(), fViewport.Width(), fViewport.Height());

      fRnrCtx->SetGrabImage(kFALSE);
      delete [] fRnrCtx->GetGrabbedImage();
      fRnrCtx->SetGrabbedImage(0);

      gif->WriteImage(fileName.Data());
   }
   else if (fileName.EndsWith(".eps"))
   {
      TGLOutput::Capture(*this, TGLOutput::kEPS_BSP, fileName.Data());
   }
   else if (fileName.EndsWith(".pdf"))
   {
      TGLOutput::Capture(*this, TGLOutput::kPDF_BSP, fileName.Data());
   }
   else
   {
      Warning("TGLViewer::SavePicture",
              "file %s cannot be saved with this extension.", fileName.Data());
      return kFALSE;
   }
   return kTRUE;
}

//______________________________________________________________________________
Bool_t TGLViewer::SavePicture()
{
   // Save current image using the defualt file name which can be set
   // via SetPictureFileName() and defaults to "viewer.jpg".
   // Really useful for the files ending with 'gif+'.

   return SavePicture(fPictureFileName);
}

//______________________________________________________________________________
void TGLViewer::DrawGuides()
{
   // Draw reference marker and coordinate axes.

   Bool_t disabled = kFALSE;
   if (fReferenceOn)
   {
      glDisable(GL_DEPTH_TEST);
      TGLUtil::DrawReferenceMarker(*fCamera, fReferencePos);
      disabled = kTRUE;
   }
   if (fDrawCameraCenter)
   {
      glDisable(GL_DEPTH_TEST);
      Float_t radius = fCamera->ViewportDeltaToWorld(TGLVertex3(fCamera->GetCenterVec()), 3, 3).Mag();
      const Float_t rgba[4] = { 0, 1, 1, 1.0 };
      TGLUtil::DrawSphere(fCamera->GetCenterVec(), radius, rgba);
      disabled = kTRUE;
   }
   if(fAxesDepthTest && disabled)
   {
      glEnable(GL_DEPTH_TEST);
      disabled = kFALSE;
   }
   else if (fAxesDepthTest == kFALSE && disabled == kFALSE)
   {
      glDisable(GL_DEPTH_TEST);
      disabled = kTRUE;
   }
   TGLUtil::DrawSimpleAxes(*fCamera, fOverallBoundingBox, fAxesType);
   if(disabled)
      glEnable(GL_DEPTH_TEST);
}

//______________________________________________________________________________
void TGLViewer::DrawCameraMarkup()
{
   // Draw camera markup overlay.

   if (fCameraMarkup && fCameraMarkup->Show())
   {
      glMatrixMode(GL_PROJECTION);
      glPushMatrix();
      glLoadIdentity();
      const TGLRect& vp = fRnrCtx->RefCamera().RefViewport();
      gluOrtho2D(0., vp.Width(), 0., vp.Height());
      glMatrixMode(GL_MODELVIEW);
      glPushMatrix();
      glLoadIdentity();
      glDisable(GL_LIGHTING);
      glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
      glDisable(GL_DEPTH_TEST);
      fRnrCtx->RefCamera().Markup(fCameraMarkup);
      glEnable(GL_DEPTH_TEST);
      glEnable(GL_LIGHTING);
      glMatrixMode(GL_PROJECTION);
      glPopMatrix();
      glMatrixMode(GL_MODELVIEW);
      glPopMatrix();
   }
}

//______________________________________________________________________________
void TGLViewer::DrawDebugInfo()
{
   // If in debug mode draw camera aids and overall bounding box.

   if (fDebugMode)
   {
      glDisable(GL_LIGHTING);
      CurrentCamera().DrawDebugAids();

      // Green scene bounding box
      glColor3d(0.0, 1.0, 0.0);
      fOverallBoundingBox.Draw();

      // Scene bounding box center sphere (green) and
      glDisable(GL_DEPTH_TEST);
      Double_t size = fOverallBoundingBox.Extents().Mag() / 200.0;
      static Float_t white[4] = {1.0, 1.0, 1.0, 1.0};
      TGLUtil::DrawSphere(TGLVertex3(0.0, 0.0, 0.0), size, white);
      static Float_t green[4] = {0.0, 1.0, 0.0, 1.0};
      const TGLVertex3 & center = fOverallBoundingBox.Center();
      TGLUtil::DrawSphere(center, size, green);
      glEnable(GL_DEPTH_TEST);

      glEnable(GL_LIGHTING);
   }
}

//______________________________________________________________________________
void TGLViewer::PreDraw()
{
   // Perform GL work which must be done before each draw of scene
   MakeCurrent();
   // Initialise GL if not done
   if (!fInitGL) {
      InitGL();
   }

   // For embedded gl clear color must be pad's background color.
   Color_t ci = (fGLDevice != -1) ? gPad->GetFillColor() : fClearColor;
   TColor *color = gROOT->GetColor(ci);
   Float_t sc[3] = {1.f, 1.f, 1.f};
   if (color)
      color->GetRGB(sc[0], sc[1], sc[2]);
   glClearColor(sc[0], sc[1], sc[2], 1.);

   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

   TGLUtil::CheckError("TGLViewer::PreDraw");
}

//______________________________________________________________________________
void TGLViewer::PostDraw()
{
   // Perform GL work which must be done after each draw of scene
   glFlush();
   if (fRnrCtx->GetGrabImage())
   {
      UChar_t* xx = new UChar_t[4 * fViewport.Width() * fViewport.Height()];
      glReadBuffer(GL_BACK);
      glPixelStorei(GL_PACK_ALIGNMENT,1);
      glReadPixels(0, 0, fViewport.Width(), fViewport.Height(),
                   GL_BGRA, GL_UNSIGNED_BYTE, xx);

      fRnrCtx->SetGrabbedImage(xx);
   }
   SwapBuffers();

   TGLUtil::CheckError("TGLViewer::PostDraw");
}

//______________________________________________________________________________
void TGLViewer::MakeCurrent() const
{
   // Make GL context current
   if (fGLDevice == -1)
      fGLWidget->MakeCurrent();
   else
      gGLManager->MakeCurrent(fGLDevice);
}

//______________________________________________________________________________
void TGLViewer::SwapBuffers() const
{
   // Swap GL buffers
   if ( ! IsDrawOrSelectLock()) {
      Error("TGLViewer::SwapBuffers", "viewer is %s", LockName(CurrentLock()));
   }
   if (fGLDevice == -1)
      fGLWidget->SwapBuffers();
   else {
      gGLManager->ReadGLBuffer(fGLDevice);
      gGLManager->Flush(fGLDevice);
      gGLManager->MarkForDirectCopy(fGLDevice, kFALSE);
   }
}

//______________________________________________________________________________
Bool_t TGLViewer::RequestSelect(Int_t x, Int_t y, Bool_t trySecSel)
{
   // Post request for select draw of viewer, picking objects round the WINDOW
   // point (x,y).
   // Request is directed via cross thread gVirtualGL object

   // Take select lock on scene immediately we enter here - it is released
   // in the other (drawing) thread - see TGLViewer::Select()
   // Removed when gVirtualGL removed

   if ( ! TakeLock(kSelectLock)) {
      return kFALSE;
   }

   if (!gVirtualX->IsCmdThread())
      return Bool_t(gROOT->ProcessLineFast(Form("((TGLViewer *)0x%x)->DoSelect(%d, %d, %s)", this, x, y, trySecSel ? "kTRUE" : "kFALSE")));
   else
      return DoSelect(x, y, trySecSel);
}

//______________________________________________________________________________
Bool_t TGLViewer::DoSelect(Int_t x, Int_t y, Bool_t trySecSel)
{
   // Perform GL selection, picking objects overlapping WINDOW
   // area described by 'rect'. Return kTRUE if selection should be
   // changed, kFALSE otherwise.
   // Select lock should already been taken in other thread in
   // TGLViewer::ReqSelect().

   if (CurrentLock() != kSelectLock) {
      Error("TGLViewer::DoSelect", "expected kSelectLock, found %s", LockName(CurrentLock()));
      return kFALSE;
   }

   MakeCurrent();

   fRnrCtx->BeginSelection(x, y, 3);
   glRenderMode(GL_SELECT);

   PreRender();
   Render();
   PostRender();

   Int_t nHits = glRenderMode(GL_RENDER);
   fRnrCtx->EndSelection(nHits);

   // Process selection.
   if (gDebug > 0) Info("TGLViewer::DoSelect", "Primary select nHits=%d.", nHits);

   if (nHits > 0)
   {
      Int_t idx = 0;
      if (FindClosestRecord(fSelRec, idx))
      {
         if (fSelRec.GetTransparent())
         {
            TGLSelectRecord opaque;
            if (FindClosestOpaqueRecord(opaque, ++idx))
               fSelRec = opaque;
         }
         if (gDebug > 1) fSelRec.Print();
      }
   } else {
      fSelRec.Reset();
   }

   if ( ! trySecSel)
   {
      ReleaseLock(kSelectLock);
      return ! TGLSelectRecord::AreSameSelectionWise(fSelRec, fCurrentSelRec);
   }

   //  Secondary selection.
   {
      if ( nHits < 1 || ! fSelRec.GetSceneInfo() || ! fSelRec.GetPhysShape() ||
           ! fSelRec.GetPhysShape()->GetLogical()->SupportsSecondarySelect())
      {
         if (gDebug > 0)
            Info("TGLViewer::DoSelect", "Skipping secondary selection "
                 "(nPrimHits=%d, sinfo=0x%lx, pshape=0x%lx).\n",
                 nHits, fSelRec.GetSceneInfo(), fSelRec.GetPhysShape());
         ReleaseLock(kSelectLock);
         fSecSelRec.Reset();
         return kFALSE;
      }

      TGLSceneInfo*    sinfo = fSelRec.GetSceneInfo();
      TGLSceneBase*    scene = sinfo->GetScene();
      TGLPhysicalShape* pshp = fSelRec.GetPhysShape();

      SceneInfoList_t foo;
      foo.push_back(sinfo);
      fScenes.swap(foo);
      fRnrCtx->BeginSelection(x, y, 3);
      fRnrCtx->SetSecSelection(kTRUE);
      glRenderMode(GL_SELECT);

      PreRender();
      fRnrCtx->SetSceneInfo(sinfo);
      scene->PreRender(*fRnrCtx);
      fRnrCtx->SetDrawPass(TGLRnrCtx::kPassFill);
      fRnrCtx->SetShapeLOD(TGLRnrCtx::kLODHigh);
      glPushName(pshp->ID());
      // !!! Hack: does not use clipping and proper draw-pass settings.
      pshp->Draw(*fRnrCtx);
      glPopName();
      scene->PostRender(*fRnrCtx);
      fRnrCtx->SetSceneInfo(0);
      PostRender();

      Int_t nSecHits = glRenderMode(GL_RENDER);
      fRnrCtx->EndSelection(nSecHits);
      fScenes.swap(foo);

      if (gDebug > 0) Info("TGLViewer::DoSelect", "Secondary select nSecHits=%d.", nSecHits);

      ReleaseLock(kSelectLock);

      if (nSecHits > 0)
      {
         fSecSelRec = fSelRec;
         fSecSelRec.SetRawOnly(fRnrCtx->GetSelectBuffer()->RawRecord(0));
         if (gDebug > 1) fSecSelRec.Print();
         return kTRUE;
      } else {
         fSecSelRec.Reset();
         return kFALSE;
      }
   }
}

//______________________________________________________________________________
void TGLViewer::ApplySelection()
{
   // Process result from last selection (in fSelRec) and
   // extract a new current selection from it.
   // Here we only use physical shape.

   fCurrentSelRec = fSelRec;

   TGLPhysicalShape * selPhys = fSelRec.GetPhysShape();
   fSelectedPShapeRef->SetPShape(selPhys);

   // Inform external client selection has been modified.
   SelectionChanged();

   RequestDraw(TGLRnrCtx::kLODHigh);
}

//______________________________________________________________________________
Bool_t TGLViewer::RequestOverlaySelect(Int_t x, Int_t y)
{
   // Post request for select draw of viewer, picking objects round the WINDOW
   // point (x,y).
   // Request is directed via cross thread gVirtualGL object

   // Take select lock on viewer immediately - it is released
   // in the other (drawing) thread - see TGLViewer::Select().
   // Removed when gVirtualGL removed

   if ( ! TakeLock(kSelectLock)) {
      return kFALSE;
   }

   if (!gVirtualX->IsCmdThread())
      return Bool_t(gROOT->ProcessLineFast(Form("((TGLViewer *)0x%x)->DoSelect(%d, %d)", this, x, y)));
   else
      return DoOverlaySelect(x, y);
}

//______________________________________________________________________________
Bool_t TGLViewer::DoOverlaySelect(Int_t x, Int_t y)
{
   // Perform GL selection, picking overlay objects only.
   // Return TRUE if the selected overlay-element has changed.

   if (CurrentLock() != kSelectLock) {
      Error("TGLViewer::DoOverlaySelect", "expected kSelectLock, found %s", LockName(CurrentLock()));
      return kFALSE;
   }

   MakeCurrent();

   fRnrCtx->BeginSelection(x, y, 3);
   glRenderMode(GL_SELECT);

   PreRenderOverlaySelection();
   RenderOverlay();
   PostRenderOverlaySelection();

   Int_t nHits = glRenderMode(GL_RENDER);
   fRnrCtx->EndSelection(nHits);

   // Process overlay selection.
   TGLOverlayElement * selElm = 0;
   if (nHits > 0)
   {
      Int_t idx = 0;
      while (idx < nHits && FindClosestOverlayRecord(fOvlSelRec, idx))
      {
         TGLOverlayElement* el = fOvlSelRec.GetOvlElement();
         if (el == fCurrentOvlElm)
         {
            if (el->MouseStillInside(fOvlSelRec))
            {
               selElm = el;
               break;
            }
         }
         else if (el->MouseEnter(fOvlSelRec))
         {
            selElm = el;
            break;
         }
      }
   }
   else
   {
      fOvlSelRec.Reset();
   }

   ReleaseLock(kSelectLock);

   if (fCurrentOvlElm != selElm)
   {
      if (fCurrentOvlElm) fCurrentOvlElm->MouseLeave();
      fCurrentOvlElm = selElm;
      return kTRUE;
   }
   else
   {
      return kFALSE;
   }
}

/**************************************************************************/
// Viewport
/**************************************************************************/

//______________________________________________________________________________
void TGLViewer::SetViewport(Int_t x, Int_t y, Int_t width, Int_t height)
{
   // Set viewer viewport (window area) with bottom/left at (x,y), with
   // dimensions 'width'/'height'

   if (IsLocked() && fGLDevice == -1) {
      Error("TGLViewer::SetViewport", "expected kUnlocked, found %s", LockName(CurrentLock()));
      return;
   }
   // Only process if changed
   if (fViewport.X() == x && fViewport.Y() == y &&
       fViewport.Width() == width && fViewport.Height() == height) {
      return;
   }

   fViewport.Set(x, y, width, height);
   fCurrentCamera->SetViewport(fViewport);

   if (gDebug > 2) {
      Info("TGLViewer::SetViewport", "updated - corner %d,%d dimensions %d,%d", x, y, width, height);
   }
}


/**************************************************************************/
// Camera methods
/**************************************************************************/

//______________________________________________________________________________
TGLCamera& TGLViewer::RefCamera(ECameraType cameraType)
{
   // Return camera reference by type.

   // TODO: Move these into a vector!
   switch(cameraType) {
      case kCameraPerspXOZ:
         return fPerspectiveCameraXOZ;
      case kCameraPerspYOZ:
         return fPerspectiveCameraYOZ;
      case kCameraPerspXOY:
         return fPerspectiveCameraXOY;
      case kCameraOrthoXOY:
         return fOrthoXOYCamera;
      case kCameraOrthoXOZ:
         return fOrthoXOZCamera;
      case kCameraOrthoZOY:
         return fOrthoZOYCamera;
      case kCameraOrthoXnOY:
         return fOrthoXnOYCamera;
      case kCameraOrthoXnOZ:
         return fOrthoXnOZCamera;
      case kCameraOrthoZnOY:
         return fOrthoZnOYCamera;
      default:
         Error("TGLViewer::SetCurrentCamera", "invalid camera type");
         return *fCurrentCamera;
   }
}

//______________________________________________________________________________
void TGLViewer::SetCurrentCamera(ECameraType cameraType)
{
   // Set current active camera - 'cameraType' one of:
   // kCameraPerspX, kCameraPerspY, kCameraPerspZ
   // kCameraOrthoXOY, kCameraOrthoXOZ, kCameraOrthoZOY

   if (IsLocked()) {
      Error("TGLViewer::SetCurrentCamera", "expected kUnlocked, found %s", LockName(CurrentLock()));
      return;
   }

   // TODO: Move these into a vector!
   switch(cameraType) {
      case kCameraPerspXOZ: {
         fCurrentCamera = &fPerspectiveCameraXOZ;
         break;
      }
      case kCameraPerspYOZ: {
         fCurrentCamera = &fPerspectiveCameraYOZ;
         break;
      }
      case kCameraPerspXOY: {
         fCurrentCamera = &fPerspectiveCameraXOY;
         break;
      }
      case kCameraOrthoXOY: {
         fCurrentCamera = &fOrthoXOYCamera;
         break;
      }
      case kCameraOrthoXOZ: {
         fCurrentCamera = &fOrthoXOZCamera;
         break;
      }
      case kCameraOrthoZOY: {
         fCurrentCamera = &fOrthoZOYCamera;
         break;
      }
      case kCameraOrthoXnOY: {
         fCurrentCamera = &fOrthoXnOYCamera;
         break;
      }
      case kCameraOrthoXnOZ: {
         fCurrentCamera = &fOrthoXnOZCamera;
         break;
      }
      case kCameraOrthoZnOY: {
         fCurrentCamera = &fOrthoZnOYCamera;
         break;
      }
      default: {
         Error("TGLViewer::SetCurrentCamera", "invalid camera type");
         break;
      }
   }

   // Ensure any viewport has been propigated to the current camera
   fCurrentCamera->SetViewport(fViewport);
   RefreshPadEditor(this);

   // And viewer is redrawn
   RequestDraw(TGLRnrCtx::kLODHigh);
}

//______________________________________________________________________________
void TGLViewer::SetOrthoCamera(ECameraType camera,
                               Double_t zoom, Double_t dolly,
                               Double_t center[3],
                               Double_t hRotate, Double_t vRotate)
{
   // Set an orthographic camera to supplied configuration - note this
   // does not need to be the current camera - though you will not see
   // the effect if it is not.
   //
   // 'camera' defines the ortho camera - one of kCameraOrthoXOY / XOZ / ZOY
   // 'left' / 'right' / 'top' / 'bottom' define the WORLD coordinates which
   // corresepond with the left/right/top/bottom positions on the GL viewer viewport
   // E.g. for kCameraOrthoXOY camera left/right are X world coords,
   // top/bottom are Y world coords
   // As this is an orthographic camera the other axis (in eye direction) is
   // no relevant. The near/far clip planes are set automatically based in scene
   // contents

   // TODO: Move these into a vector!
   switch(camera) {
      case kCameraOrthoXOY: {
         fOrthoXOYCamera.Configure(zoom, dolly, center, hRotate, vRotate);
         if (fCurrentCamera == &fOrthoXOYCamera) {
            RequestDraw(TGLRnrCtx::kLODHigh);
         }
         break;
      }
      case kCameraOrthoXOZ: {
         fOrthoXOZCamera.Configure(zoom, dolly, center, hRotate, vRotate);
         if (fCurrentCamera == &fOrthoXOZCamera) {
            RequestDraw(TGLRnrCtx::kLODHigh);
         }
         break;
      }
      case kCameraOrthoZOY: {
         fOrthoZOYCamera.Configure(zoom, dolly, center, hRotate, vRotate);
         if (fCurrentCamera == &fOrthoZOYCamera) {
            RequestDraw(TGLRnrCtx::kLODHigh);
         }
         break;
      }
      default: {
         Error("TGLViewer::SetOrthoCamera", "invalid camera type");
         break;
      }
   }
}

//______________________________________________________________________________
void TGLViewer::SetPerspectiveCamera(ECameraType camera,
                                     Double_t fov, Double_t dolly,
                                     Double_t center[3],
                                     Double_t hRotate, Double_t vRotate)
{
   // Set a perspective camera to supplied configuration - note this
   // does not need to be the current camera - though you will not see
   // the effect if it is not.
   //
   // 'camera' defines the persp camera - one of kCameraPerspXOZ, kCameraPerspYOZ, kCameraPerspXOY
   // 'fov' - field of view (lens angle) in degrees (clamped to 0.1 - 170.0)
   // 'dolly' - distance from 'center'
   // 'center' - world position from which dolly/hRotate/vRotate are measured
   //             camera rotates round this, always facing in (in center of viewport)
   // 'hRotate' - horizontal rotation from initial configuration in degrees
   // 'hRotate' - vertical rotation from initial configuration in degrees

   // TODO: Move these into a vector!
   switch(camera) {
      case kCameraPerspXOZ: {
         fPerspectiveCameraXOZ.Configure(fov, dolly, center, hRotate, vRotate);
         if (fCurrentCamera == &fPerspectiveCameraXOZ) {
            RequestDraw(TGLRnrCtx::kLODHigh);
         }
         break;
      }
      case kCameraPerspYOZ: {
         fPerspectiveCameraYOZ.Configure(fov, dolly, center, hRotate, vRotate);
         if (fCurrentCamera == &fPerspectiveCameraYOZ) {
            RequestDraw(TGLRnrCtx::kLODHigh);
         }
         break;
      }
      case kCameraPerspXOY: {
         fPerspectiveCameraXOY.Configure(fov, dolly, center, hRotate, vRotate);
         if (fCurrentCamera == &fPerspectiveCameraXOY) {
            RequestDraw(TGLRnrCtx::kLODHigh);
         }
         break;
      }
      default: {
         Error("TGLViewer::SetPerspectiveCamera", "invalid camera type");
         break;
      }
   }
}


/**************************************************************************/
// Guide methods
/**************************************************************************/

//______________________________________________________________________________
void TGLViewer::GetGuideState(Int_t & axesType, Bool_t & axesDepthTest, Bool_t & referenceOn, Double_t referencePos[3]) const
{
   // Fetch the state of guides (axes & reference markers) into arguments

   axesType = fAxesType;
   axesDepthTest = fAxesDepthTest;

   referenceOn = fReferenceOn;
   referencePos[0] = fReferencePos.X();
   referencePos[1] = fReferencePos.Y();
   referencePos[2] = fReferencePos.Z();
}

//______________________________________________________________________________
void TGLViewer::SetGuideState(Int_t axesType, Bool_t axesDepthTest, Bool_t referenceOn, const Double_t referencePos[3])
{
   // Set the state of guides (axes & reference markers) from arguments.

   fAxesType    = axesType;
   fAxesDepthTest = axesDepthTest;
   fReferenceOn = referenceOn;
   if (referencePos)
      fReferencePos.Set(referencePos[0], referencePos[1], referencePos[2]);
   if (fGLDevice != -1)
      gGLManager->MarkForDirectCopy(fGLDevice, kTRUE);
   RequestDraw();
}

//______________________________________________________________________________
void TGLViewer::SetDrawCameraCenter(Bool_t x)
{
   // Draw camera look at and rotation point.

   fDrawCameraCenter = x;
   RequestDraw();
}

// Selected physical
//______________________________________________________________________________
const TGLPhysicalShape * TGLViewer::GetSelected() const
{
   // Return selected physical shape.

   return fSelectedPShapeRef->GetPShape();
}

/**************************************************************************/
/**************************************************************************/

//______________________________________________________________________________
void TGLViewer::SelectionChanged()
{
   // Emit signal indicating selection has changed.

   Emit("SelectionChanged()");
}

//______________________________________________________________________________
void TGLViewer::OverlayDragFinished()
{
   // Emit signal indicating that an overlay drag has finished.

   Emit("OverlayDragFinished()");
}

//______________________________________________________________________________
void TGLViewer::MouseOver(TGLPhysicalShape *shape)
{
   // Emit MouseOver signal.

   Emit("MouseOver(TGLPhysicalShape*)", (Long_t)shape);
}

//______________________________________________________________________________
void TGLViewer::MouseOver(TGLPhysicalShape *shape, UInt_t state)
{
   // Emit MouseOver signal.

   Long_t args[2];
   args[0] = (Long_t)shape;
   args[1] = state;
   Emit("MouseOver(TGLPhysicalShape*,UInt_t)", args);
}

//______________________________________________________________________________
void TGLViewer::Clicked(TObject *obj)
{
   // Emit Clicked signal.

   Emit("Clicked(TObject*)", (Long_t)obj);
}

//______________________________________________________________________________
void TGLViewer::Clicked(TObject *obj, UInt_t button, UInt_t state)
{
   // Emit Clicked signal with button id and modifier state.

   Long_t args[3];
   args[0] = (Long_t)obj;
   args[1] = button;
   args[2] = state;
   Emit("Clicked(TObject*,UInt_t,UInt_t)", args);
}

//______________________________________________________________________________
void TGLViewer::MouseIdle(TGLPhysicalShape *shape, UInt_t posx, UInt_t posy)
{
   // Emit MouseIdle signal.

   Long_t args[3];
   static UInt_t oldx = 0, oldy = 0;

   if (oldx != posx || oldy != posy) {
      args[0] = (Long_t)shape;
      args[1] = posx;
      args[2] = posy;
      Emit("MouseIdle(TGLPhysicalShape*,UInt_t,UInt_t)", args);
      oldx = posx;
      oldy = posy;
   }
}

/**************************************************************************/
/**************************************************************************/
//______________________________________________________________________________
Int_t TGLViewer::DistancetoPrimitive(Int_t /*px*/, Int_t /*py*/)
{
   // Calcaulate and return pixel distance to nearest viewer object from
   // window location px, py
   // This is provided for use when embedding GL viewer into pad

   // Can't track the indvidual objects in rollover. Just set the viewer as the
   // selected object, and return 0 (object identified) so we receive ExecuteEvent calls
   gPad->SetSelected(this);
   return 0;
}

//______________________________________________________________________________
void TGLViewer::ExecuteEvent(Int_t event, Int_t px, Int_t py)
{
   // Process event of type 'event' - one of EEventType types,
   // occuring at window location px, py
   // This is provided for use when embedding GL viewer into pad

   if (fEventHandler)
      return fEventHandler->ExecuteEvent(event, px, py);
}

//______________________________________________________________________________
void TGLViewer::PrintObjects()
{
   // Pass viewer for print capture by TGLOutput.

   TGLOutput::Capture(*this);
}

//______________________________________________________________________________
void TGLViewer::SetEventHandler(TGEventHandler *handler)
{
   if (fEventHandler)
      delete fEventHandler;

   fEventHandler = handler;
   if (fGLWidget)
      fGLWidget->SetEventHandler(fEventHandler);
}
