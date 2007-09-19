// @(#)root/gl:$Id$
// Author:  Richard Maunder / Timur Pocheptsov

/*************************************************************************
 * Copyright (C) 1995-2005, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#ifndef ROOT_TGLSAViewer
#define ROOT_TGLSAViewer

#ifndef ROOT_TGLViewer
#include "TGLViewer.h"
#endif

#ifndef ROOT_TString
#include "TString.h"
#endif

class TGFrame;
class TGCompositeFrame;
class TGPopupMenu;
class TGLSAFrame;
class TGTab;

class TGLGeometryEditor;
class TGLViewerEditor;
class TGLColorEditor;
class TGLRenderArea; // Remove - replace with TGLManager

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// TGLSAViewer                                                          //
//                                                                      //
// The top level standalone viewer object - created via plugin manager. //
// TGLSAViewer
//////////////////////////////////////////////////////////////////////////

// TODO: This really needs to be re-examined along with GUI parts in TGLViewer. 
// It still contiains lots of legacy parts for binding to external GUI (TGLEditors) 
// which could be neater.

class TGLSAViewer : public TGLViewer {
public:
   enum EGLSACommands { kGLHelpAbout, kGLHelpViewer, kGLXOY,
      kGLXOZ, kGLZOY, kGLPerspYOZ, kGLPerspXOZ, kGLPerspXOY, 
      kGLSaveEPS, kGLSavePDF, kGLSavePNG, kGLSaveGIF, 
      kGLSaveJPG, kGLSaveAS, kGLCloseViewer, kGLQuitROOT};

private:
   // GUI components
   TGLSAFrame        *fFrame;
   TGPopupMenu       *fFileMenu;
   TGPopupMenu       *fFileSaveMenu;
   TGPopupMenu       *fCameraMenu;
   TGPopupMenu       *fHelpMenu;
   TGLRenderArea     *fGLArea;
   // Tabs
   TGCompositeFrame  *fLeftVerticalFrame;
   TGTab             *fEditorTab;
   TGLViewerEditor   *fGLEd;
   TGTab             *fObjEdTab;
   TGLColorEditor    *fColorEd;
   TGLGeometryEditor *fGeomEd;
   
   TString            fDirName;
   Int_t              fTypeIdx;
   Bool_t             fOverwrite;

   // Initial window positioning
   static const Int_t fgInitX;
   static const Int_t fgInitY;
   static const Int_t fgInitW;
   static const Int_t fgInitH;

   static const char * fgHelpText;

   void CreateMenus();
   void CreateFrames();

   // non-copyable class
   TGLSAViewer(const TGLSAViewer &);
   TGLSAViewer & operator = (const TGLSAViewer &);

   void SavePicture(const TString &fileName);

protected:
   // Overloadable 
   virtual void PostSceneBuildSetup();
   virtual void SelectionChanged(); // *SIGNAL*
   virtual void ClipChanged();      // *SIGNAL*

public:
   TGLSAViewer(TVirtualPad * pad);
   TGLSAViewer(TGFrame * parent, TVirtualPad * pad);
   ~TGLSAViewer();

   void   Show();
   void   Close();

   // GUI events - editors, frame etc
   void   ProcessGUIEvent(Int_t id);
   Bool_t ProcessFrameMessage(Long_t msg, Long_t parm1, Long_t);

   TGLSAFrame* GetFrame()     const { return fFrame; }
   TGTab*      GetEditorTab() const { return fEditorTab; }

   ClassDef(TGLSAViewer, 0) // Standalone GL viewer
};

#endif
