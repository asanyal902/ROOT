// @(#)root/eve:$Id$
// Authors: Matevz Tadel & Alja Mrak-Tadel: 2006, 2007

/*************************************************************************
 * Copyright (C) 1995-2007, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#ifndef ROOT_TEveScene
#define ROOT_TEveScene

#include "TEveElement.h"
#include "TEvePad.h"

class TGLScenePad;


/******************************************************************************/
// TEveScene
/******************************************************************************/

class TEveScene : public TEveElementList
{
private:
   TEveScene(const TEveScene&);            // Not implemented
   TEveScene& operator=(const TEveScene&); // Not implemented

protected:
   TEvePad     *fPad;
   TGLScenePad *fGLScene;

   Bool_t       fChanged;
   Bool_t       fSmartRefresh;

public:
   TEveScene(const Text_t* n="TEveScene", const Text_t* t="");
   virtual ~TEveScene();

   virtual void CollectSceneParents(List_t& scenes);

   void   Changed()         { fChanged = kTRUE; }
   Bool_t IsChanged() const { return fChanged;  }
   void   Repaint();

   TGLScenePad* GetGLScene() const { return fGLScene; }
   void SetGLScene(TGLScenePad* s) { fGLScene = s; }

   virtual void SetName(const Text_t* n);
   virtual void Paint(Option_t* option = "");

   virtual const TGPicture* GetListTreeIcon();

   ClassDef(TEveScene, 0); // Reve representation of TGLScene.
};


/******************************************************************************/
// TEveSceneList
/******************************************************************************/

class TEveSceneList : public TEveElementList
{
private:
   TEveSceneList(const TEveSceneList&);            // Not implemented
   TEveSceneList& operator=(const TEveSceneList&); // Not implemented

protected:

public:
   TEveSceneList(const Text_t* n="TEveSceneList", const Text_t* t="");
   virtual ~TEveSceneList() {}

   void RepaintChangedScenes();
   void RepaintAllScenes();

   ClassDef(TEveSceneList, 0); // List of Scenes providing common operations on TEveScene collections.
};

#endif
