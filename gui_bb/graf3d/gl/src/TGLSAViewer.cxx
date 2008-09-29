// @(#)root/gl:$Id$
// Author:  Timur Pocheptsov / Richard Maunder

/*************************************************************************
 * Copyright (C) 1995-2004, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#include <memory>

#include "TRootHelpDialog.h"
#include "TPluginManager.h"
#include "TApplication.h"
#include "TGClient.h"
#include "TGCanvas.h"
#include "HelpText.h"
#include "GuiTypes.h"
#include "TG3DLine.h"
#include "TSystem.h"
#include "TGFrame.h"
#include "TGLabel.h"
#include "TGMenu.h"
#include "TGSplitter.h"
#include "TColor.h"

#include "TVirtualPad.h"
#include "TGedEditor.h"
#include "TRootEmbeddedCanvas.h"
#include "TString.h"
#include "TGFileDialog.h"

#include "TGLOutput.h"

#include "TGLLogicalShape.h"
#include "TGLPhysicalShape.h"
#include "TGLPShapeObj.h"
#include "TGLClip.h"
#include "TROOT.h"

#ifdef WIN32
#include "TWin32SplashThread.h"
#endif

#include "TGLPhysicalShape.h"
#include "TGLWidget.h"
#include "TGLSAViewer.h"
#include "TGLSAFrame.h"
#include "TGLOutput.h"
#include "TGLEventHandler.h"

const char * TGLSAViewer::fgHelpText1 = "\
DIRECT SCENE INTERACTIONS\n\n\
   Press:\n\
   \tw          --- wireframe mode\n\
   \tr          --- filled polygons mode\n\
   \tt          --- outline mode\n\
   \tj          --- ZOOM in\n\
   \tk          --- ZOOM out\n\
   \tArrow Keys --- PAN (TRUCK) across scene\n\
   \tHome       --- reset current camera\n\n\
   You can ROTATE (ORBIT) the scene by holding the left mouse button and moving\n\
   the mouse (perspective camera, needs to be enabled for orthograpic camers).\n\
   By default, the scene will be rotated about its center. To select arbitrary center\n\
   bring up the viewer-editor and use 'Camera center' controls in the 'Guides' tab.\n\n\
   You can PAN (TRUCK) the camera using the middle mouse button or arrow keys.\n\n\
   You can ZOOM (DOLLY) the camera by dragging side to side holding the right\n\
   mouse button or using the mouse wheel.\n\n\
   RESET the camera by double clicking any button.\n\n\
   SELECT a shape with Shift+Left mouse button click.\n\n\
   SELECT the viewer with Shift+Left mouse button click on a free space.\n\n\
   MOVE a selected shape using Shift+Mid mouse drag.\n\n\
   Invoke the CONTEXT menu with Shift+Right mouse click.\n\n\
   Secondary selection and direct render object interaction is initiated\n\
   by Alt+Left mouse click (Mod1, actually). Only few classes support this option.\n\n\
CAMERA\n\n\
   The \"Camera\" menu is used to select the different projections from \n\
   the 3D world onto the 2D viewport. There are three perspective cameras:\n\n\
   \tPerspective (Floor XOZ)\n\
   \tPerspective (Floor YOZ)\n\
   \tPerspective (Floor XOY)\n\n\
   In each case the floor plane (defined by two axes) is kept level.\n\n\
   There are also three orthographic cameras:\n\n\
   \tOrthographic (XOY)\n\
   \tOrthographic (XOZ)\n\
   \tOrthographic (ZOY)\n\n\
   In each case the first axis is placed horizontal, the second vertical e.g.\n\
   XOY means X horizontal, Y vertical.\n\n";

const char * TGLSAViewer::fgHelpText2 = "\
SHAPES COLOR AND MATERIAL\n\n\
   The selected shape's color can be modified in the Shapes-Color tabs.\n\
   Shape's color is specified by the percentage of red, green, blue light\n\
   it reflects. A surface can reflect DIFFUSE, AMBIENT and SPECULAR light.\n\
   A surface can also emit light. The EMISSIVE parameter allows to define it.\n\
   The surface SHININESS can also be modified.\n\n\
SHAPES GEOMETRY\n\n\
   The selected shape's location and geometry can be modified in the Shapes-Geom\n\
   tabs by entering desired values in respective number entry controls.\n\n\
SCENE CLIPPING\n\n\
   In the Scene-Clipping tabs select a 'Clip Type': None, Plane, Box\n\n\
   For 'Plane' and 'Box' the lower pane shows the relevant parameters:\n\n\
\tPlane: Equation coefficients of form aX + bY + cZ + d = 0\n\
\tBox: Center X/Y/Z and Length X/Y/Z\n\n\
   For Box checking the 'Show / Edit' checkbox shows the clip box (in light blue)\n\
   in viewer. It also attaches the current manipulator to the box - enabling\n\
   direct editing in viewer.\n\n\
MANIPULATORS\n\n\
   A widget attached to the selected object - allowing direct manipulation\n\
   of the object with respect to its local axes.\n\n\
   There are three modes, toggled with keys while manipulator is active:\n\
   \tMode\t\tWidget Component Style\t\tKey\n\
   \t----\t\t----------------------\t\t---\n\
   \tTranslation\tLocal axes with arrows\t\tv\n\
   \tScale\t\tLocal axes with boxes\t\tx\n\
   \tRotate\t\tLocal axes rings\t\tc\n\n\
   Each widget has three axis components - red (X), green (Y) and blue (Z).\n\
   The component turns yellow, indicating an active state, when the mouse is moved\n\
   over it. Left click and drag on the active component to adjust the objects\n\
   translation, scale or rotation.\n\
   Some objects do not support all manipulations (e.g. clipping planes cannot be \n\
   scaled). If a manipulation is not permitted the component it drawn in grey and \n\
   cannot be selected/dragged.\n";


//==============================================================================
// TGLSAViewer
//==============================================================================

//______________________________________________________________________________
//
// The top level standalone GL-viewer - created via plugin manager.


ClassImp(TGLSAViewer);

const Int_t TGLSAViewer::fgInitX = 0;
const Int_t TGLSAViewer::fgInitY = 0;
const Int_t TGLSAViewer::fgInitW = 780;
const Int_t TGLSAViewer::fgInitH = 670;

// A lot of raw pointers/naked new-expressions - good way to discredit C++ (or C++ programmer
// ROOT has system to cleanup - I'll try to use it

const char *gGLSaveAsTypes[] = {"Encapsulated PostScript", "*.eps",
                                "PDF",                     "*.pdf",
                                "GIF",                     "*.gif",
                                "JPEG",                    "*.jpg",
                                "PNG",                     "*.png",
                                0, 0};

//______________________________________________________________________________
TGLSAViewer::TGLSAViewer(TVirtualPad *pad) :
   TGLViewer(pad, fgInitX, fgInitY, fgInitW, fgInitH),
   fFrame(0),
   fFileMenu(0),
   fFileSaveMenu(0),
   fCameraMenu(0),
   fHelpMenu(0),
   fLeftVerticalFrame(0),
   fGedEditor(0),
   fPShapeWrap(0),
   fDirName("."),
   fTypeIdx(0),
   fOverwrite(kFALSE)
{
   // Construct a standalone viewer, bound to supplied 'pad'.
   fFrame = new TGLSAFrame(*this);

   CreateMenus();
   CreateFrames();

   fFrame->SetWindowName("ROOT's GL viewer");
   fFrame->SetClassHints("GLViewer", "GLViewer");
   fFrame->SetMWMHints(kMWMDecorAll, kMWMFuncAll, kMWMInputModeless);
   fFrame->MapSubwindows();

   fFrame->Resize(fFrame->GetDefaultSize());
   fFrame->MoveResize(fgInitX, fgInitY, fgInitW, fgInitH);
   fFrame->SetWMPosition(fgInitX, fgInitY);

   fPShapeWrap = new TGLPShapeObj(0, this);

   // set recursive cleanup, but exclude fGedEditor
   // destructor of fGedEditor has own way of handling child nodes
   TObject* fe = fLeftVerticalFrame->GetList()->First();
   fLeftVerticalFrame->GetList()->Remove(fe);
   fFrame->SetCleanup(kDeepCleanup);
   fLeftVerticalFrame->GetList()->AddFirst(fe);

   Show();
}

//______________________________________________________________________________
TGLSAViewer::TGLSAViewer(const TGWindow *parent, TVirtualPad *pad, TGedEditor *ged) :
   TGLViewer(pad, fgInitX, fgInitY, fgInitW, fgInitH),
   fFrame(0),
   fFileMenu(0),
   fCameraMenu(0),
   fHelpMenu(0),
   fLeftVerticalFrame(0),
   fGedEditor(ged),
   fPShapeWrap(0),
   fTypeIdx(0)
{
   // Construct an embedded standalone viewer, bound to supplied 'pad'.
   //
   // Modified version of the previous constructor for embedding the
   // viewer into another frame (parent).

   fFrame = new TGLSAFrame(parent, *this);

   CreateMenus();
   CreateFrames();

   fFrame->MapSubwindows();
   fFrame->Resize(fFrame->GetDefaultSize());
   fFrame->Resize(fgInitW, fgInitH);

   fPShapeWrap = new TGLPShapeObj(0, this);

   // set recursive cleanup, but exclude fGedEditor
   // destructor of fGedEditor has own way of handling child nodes
   if (fLeftVerticalFrame)
   {
      TObject* fe = fLeftVerticalFrame->GetList()->First();
      fLeftVerticalFrame->GetList()->Remove(fe);
      fFrame->SetCleanup(kDeepCleanup);
      fLeftVerticalFrame->GetList()->AddFirst(fe);
   }

   Show();
}

//______________________________________________________________________________
TGLSAViewer::~TGLSAViewer()
{
   // Destroy standalone viewer object.

   fGedEditor->DisconnectFromCanvas();

   delete fHelpMenu;
   delete fCameraMenu;
   delete fFileSaveMenu;
   delete fFileMenu;
   delete fFrame;
   fGLWidget = 0;
}

//______________________________________________________________________________
void TGLSAViewer::RefreshPadEditor(TObject* changed)
{
   // Refresh pad editor.

   if (changed == 0 || fGedEditor->GetModel() == changed) {
      fGedEditor->SetModel(fPad, fGedEditor->GetModel(), kButton1Down);
   }
}

//______________________________________________________________________________
void TGLSAViewer::CreateMenus()
{
   //File/Camera/Help menus.

   fFileMenu = new TGPopupMenu(fFrame->GetClient()->GetRoot());
   fFileMenu->AddEntry("&Edit Object", kGLEditObject);
   fFileMenu->AddSeparator();
   fFileMenu->AddEntry("&Close Viewer", kGLCloseViewer);
   fFileMenu->AddSeparator();
   fFileSaveMenu = new TGPopupMenu(fFrame->GetClient()->GetRoot());
   fFileSaveMenu->AddEntry("viewer.&eps", kGLSaveEPS);
   fFileSaveMenu->AddEntry("viewer.&pdf", kGLSavePDF);
   fFileSaveMenu->AddEntry("viewer.&gif", kGLSaveGIF);
   fFileSaveMenu->AddEntry("viewer.g&if+", kGLSaveAnimGIF);
   fFileSaveMenu->AddEntry("viewer.&jpg", kGLSaveJPG);
   fFileSaveMenu->AddEntry("viewer.p&ng", kGLSavePNG);
   fFileMenu->AddPopup("&Save", fFileSaveMenu);
   fFileMenu->AddEntry("Save &As...", kGLSaveAS);
   fFileMenu->AddSeparator();
   fFileMenu->AddEntry("&Quit ROOT", kGLQuitROOT);
   fFileMenu->Associate(fFrame);

   fCameraMenu = new TGPopupMenu(fFrame->GetClient()->GetRoot());
   fCameraMenu->AddEntry("Perspective (Floor XOZ)", kGLPerspXOZ);
   fCameraMenu->AddEntry("Perspective (Floor YOZ)", kGLPerspYOZ);
   fCameraMenu->AddEntry("Perspective (Floor XOY)", kGLPerspXOY);
   fCameraMenu->AddEntry("Orthographic (XOY)", kGLXOY);
   fCameraMenu->AddEntry("Orthographic (XOZ)", kGLXOZ);
   fCameraMenu->AddEntry("Orthographic (ZOY)", kGLZOY);
   fCameraMenu->AddEntry("Orthographic (XnOY)", kGLXnOY);
   fCameraMenu->AddEntry("Orthographic (XnOZ)", kGLXnOZ);
   fCameraMenu->AddEntry("Orthographic (ZnOY)", kGLZnOY);
   fCameraMenu->AddSeparator();
   fCameraMenu->AddEntry("Ortho allow rotate", kGLOrthoRotate);
   fCameraMenu->AddEntry("Ortho allow dolly",  kGLOrthoDolly);
   fCameraMenu->Associate(fFrame);

   fHelpMenu = new TGPopupMenu(fFrame->GetClient()->GetRoot());
   fHelpMenu->AddEntry("Help on GL Viewer...", kGLHelpViewer);
   fHelpMenu->AddSeparator();
   fHelpMenu->AddEntry("&About ROOT...", kGLHelpAbout);
   fHelpMenu->Associate(fFrame);

   // Create menubar
   TGMenuBar *menuBar = new TGMenuBar(fFrame, 1, 1, kHorizontalFrame);
   menuBar->AddPopup("&File", fFileMenu, new TGLayoutHints(kLHintsTop | kLHintsLeft, 0, 4, 0, 0));
   menuBar->AddPopup("&Camera", fCameraMenu, new TGLayoutHints(kLHintsTop | kLHintsLeft, 0, 4, 0, 0));
   menuBar->AddPopup("&Help",    fHelpMenu,    new TGLayoutHints(kLHintsTop | kLHintsRight));
   fFrame->AddFrame(menuBar, new TGLayoutHints(kLHintsTop | kLHintsLeft | kLHintsExpandX, 0, 0, 1, 1));

}

//______________________________________________________________________________
void TGLSAViewer::CreateFrames()
{
   // Internal frames creation.

   TGCompositeFrame* compositeFrame = fFrame;
   if (fGedEditor == 0)
   {
      compositeFrame = new TGCompositeFrame(fFrame, 100, 100, kHorizontalFrame | kRaisedFrame);
      fFrame->AddFrame(compositeFrame, new TGLayoutHints(kLHintsExpandX | kLHintsExpandY));

      fLeftVerticalFrame = new TGVerticalFrame(compositeFrame, 195, 10, kFixedWidth);
      compositeFrame->AddFrame(fLeftVerticalFrame, new TGLayoutHints(kLHintsLeft | kLHintsExpandY , 2, 2, 2, 2));

      const TGWindow* cw =  fFrame->GetClient()->GetRoot();
      fFrame->GetClient()->SetRoot(fLeftVerticalFrame);

      fGedEditor = new TGedEditor();
      fGedEditor->GetTGCanvas()->ChangeOptions(0);
      fLeftVerticalFrame->RemoveFrame(fGedEditor);
      fLeftVerticalFrame->AddFrame(fGedEditor, new TGLayoutHints(kLHintsTop | kLHintsLeft | kLHintsExpandX | kLHintsExpandY, 0, 0, 2, 2));
      fLeftVerticalFrame->GetClient()->SetRoot((TGWindow*)cw);
      fLeftVerticalFrame->MapSubwindows();

      TGVSplitter *splitter = new TGVSplitter(compositeFrame);
      splitter->SetFrame(fLeftVerticalFrame, kTRUE);
      compositeFrame->AddFrame(splitter, new TGLayoutHints(kLHintsLeft | kLHintsExpandY, 0,1,2,2) );
   }

   TGVerticalFrame *rightVerticalFrame = new TGVerticalFrame(compositeFrame, 10, 10, kSunkenFrame);
   compositeFrame->AddFrame(rightVerticalFrame, new TGLayoutHints(kLHintsRight | kLHintsExpandX | kLHintsExpandY,0,2,2,2));

   fGLWidget = TGLWidget::Create(rightVerticalFrame, kTRUE, kTRUE, 0, 10, 10);

   SetEventHandler(new TGLEventHandler("Default", fGLWidget, this));

   rightVerticalFrame->AddFrame(fGLWidget, new TGLayoutHints(kLHintsExpandX | kLHintsExpandY));
}


//______________________________________________________________________________
void TGLSAViewer::Show()
{
   // Show the viewer
   fFrame->MapRaised();
   fGedEditor->SetModel(fPad, this, kButton1Down);
   RequestDraw();
}

//______________________________________________________________________________
void TGLSAViewer::Close()
{
   // Close the viewer - destructed.

   // Commit suicide when contained GUI is closed.
   delete this;
}

//______________________________________________________________________________
Bool_t TGLSAViewer::ProcessFrameMessage(Long_t msg, Long_t parm1, Long_t)
{
   // Process GUI message capture by the main GUI frame (TGLSAFrame).

   switch (GET_MSG(msg)) {
   case kC_COMMAND:
      switch (GET_SUBMSG(msg)) {
      case kCM_BUTTON:
      case kCM_MENU:
         switch (parm1) {
         case kGLHelpAbout: {
#ifdef R__UNIX
            TString rootx;
#ifdef ROOTBINDIR
            rootx = ROOTBINDIR;
#else
            rootx = gSystem->Getenv("ROOTSYS");
            if (!rootx.IsNull()) rootx += "/bin";
#endif
            rootx += "/root -a &";
            gSystem->Exec(rootx);
#else
#ifdef WIN32
            new TWin32SplashThread(kTRUE);
#else
            char str[32];
            sprintf(str, "About ROOT %s...", gROOT->GetVersion());
            hd = new TRootHelpDialog(this, str, 600, 400);
            hd->SetText(gHelpAbout);
            hd->Popup();
#endif
#endif
            break;
         }
         case kGLHelpViewer: {
            TRootHelpDialog * hd = new TRootHelpDialog(fFrame, "Help on GL Viewer...", 600, 400);
            hd->AddText(fgHelpText1);
            hd->AddText(fgHelpText2);
            hd->Popup();
            break;
         }
         case kGLPerspYOZ:
            SetCurrentCamera(TGLViewer::kCameraPerspYOZ);
            break;
         case kGLPerspXOZ:
            SetCurrentCamera(TGLViewer::kCameraPerspXOZ);
            break;
         case kGLPerspXOY:
            SetCurrentCamera(TGLViewer::kCameraPerspXOY);
            break;
         case kGLXOY:
            SetCurrentCamera(TGLViewer::kCameraOrthoXOY);
            break;
         case kGLXOZ:
            SetCurrentCamera(TGLViewer::kCameraOrthoXOZ);
            break;
         case kGLZOY:
            SetCurrentCamera(TGLViewer::kCameraOrthoZOY);
            break;
         case kGLXnOY:
            SetCurrentCamera(TGLViewer::kCameraOrthoXnOY);
            break;
         case kGLXnOZ:
            SetCurrentCamera(TGLViewer::kCameraOrthoXnOZ);
            break;
         case kGLZnOY:
            SetCurrentCamera(TGLViewer::kCameraOrthoZnOY);
            break;
         case kGLOrthoRotate:
            ToggleOrthoRotate();
            break;
         case kGLOrthoDolly:
            ToggleOrthoDolly();
            break;
         case kGLSaveEPS:
            SavePicture("viewer.eps");
            break;
         case kGLSavePDF:
            SavePicture("viewer.pdf");
            break;
         case kGLSaveGIF:
            SavePicture("viewer.gif");
            break;
         case kGLSaveAnimGIF:
            SavePicture("viewer.gif+");
            break;
         case kGLSaveJPG:
            SavePicture("viewer.jpg");
            break;
         case kGLSavePNG:
            SavePicture("viewer.png");
            break;
         case kGLSaveAS:
            {
               TGFileInfo fi;
               fi.fFileTypes   = gGLSaveAsTypes;
               fi.fIniDir      = StrDup(fDirName);
               fi.fFileTypeIdx = fTypeIdx;
               fi.fOverwrite   = fOverwrite;
               new TGFileDialog(gClient->GetDefaultRoot(), fFrame, kFDSave, &fi);
               if (!fi.fFilename) return kTRUE;
               TString ft(fi.fFileTypes[fi.fFileTypeIdx+1]);
               fDirName   = fi.fIniDir;
               fTypeIdx   = fi.fFileTypeIdx;
               fOverwrite = fi.fOverwrite;

               TString file = fi.fFilename;
               if (ft.Index(".") != kNPOS)
                  file += ft(ft.Index("."), ft.Length());
               SavePicture(file);
            }
            break;
         case kGLEditObject:
            ToggleEditObject();
            break;
         case kGLCloseViewer:
            // Exit needs to be delayed to avoid bad drawable X ids - GUI
            // will all be changed in future anyway

            TTimer::SingleShot(50, "TGLSAFrame", fFrame, "SendCloseMessage()");
            break;
         case kGLQuitROOT:
            if (!gApplication->ReturnFromRun())
               delete this;
            gApplication->Terminate(0);
            default:
            break;
            }
            default:
            break;
      }
   default:
      break;
   }

   return kTRUE;
}

//______________________________________________________________________________
void TGLSAViewer::SelectionChanged()
{
   // Update GUI components for embedded viewer selection change.

   // !!! MT this whole selection-signal-stuff is verrrry strange.
   // At least, shouldn't we emit a signal here?
   //
   // I misuse this function after overlay-mouse-drag as well.
   // Need something like refresh-viewer-gui which also does ged update.

   TGLPhysicalShape *selected = const_cast<TGLPhysicalShape*>(GetSelected());

   if (selected) {
      fPShapeWrap->fPShape = selected;
      if (fFileMenu->IsEntryChecked(kGLEditObject))
         fGedEditor->SetModel(fPad, selected->GetLogical()->GetExternal(), kButton1Down);
      else
         fGedEditor->SetModel(fPad, fPShapeWrap, kButton1Down);
   } else {
      fPShapeWrap->fPShape = 0;
      fGedEditor->SetModel(fPad, this, kButton1Down);
   }
}

//______________________________________________________________________________
void TGLSAViewer::OverlayDragFinished()
{
   // An overlay operation can result in change to an object.
   // Refresh geditor.

   fGedEditor->SetModel(fPad, fGedEditor->GetModel(), kButton1Down);
}

//______________________________________________________________________________
void TGLSAViewer::ToggleEditObject()
{
   // Toggle state of the 'Edit Object' menu entry.

   if (fFileMenu->IsEntryChecked(kGLEditObject))
      fFileMenu->UnCheckEntry(kGLEditObject);
   else
      fFileMenu->CheckEntry(kGLEditObject);
   SelectionChanged();
}

//______________________________________________________________________________
void TGLSAViewer::ToggleOrthoRotate()
{
   // Toggle state of the 'Ortho allow rotate' menu entry.

   if (fCameraMenu->IsEntryChecked(kGLOrthoRotate))
      fCameraMenu->UnCheckEntry(kGLOrthoRotate);
   else
      fCameraMenu->CheckEntry(kGLOrthoRotate);
   Bool_t state = fCameraMenu->IsEntryChecked(kGLOrthoRotate);
   fOrthoXOYCamera.SetEnableRotate(state);
   fOrthoXOZCamera.SetEnableRotate(state);
   fOrthoZOYCamera.SetEnableRotate(state);
}

//______________________________________________________________________________
void TGLSAViewer::ToggleOrthoDolly()
{
   // Toggle state of the 'Ortho allow dolly' menu entry.

   if (fCameraMenu->IsEntryChecked(kGLOrthoDolly))
      fCameraMenu->UnCheckEntry(kGLOrthoDolly);
   else
      fCameraMenu->CheckEntry(kGLOrthoDolly);
   Bool_t state = ! fCameraMenu->IsEntryChecked(kGLOrthoDolly);
   fOrthoXOYCamera.SetDollyToZoom(state);
   fOrthoXOZCamera.SetDollyToZoom(state);
   fOrthoZOYCamera.SetDollyToZoom(state);
}
