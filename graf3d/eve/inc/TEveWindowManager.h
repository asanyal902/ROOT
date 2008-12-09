// @(#)root/eve:$Id$
// Author: Matevz Tadel 2007

/*************************************************************************
 * Copyright (C) 1995-2007, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#ifndef ROOT_TEveWindowManager
#define ROOT_TEveWindowManager

#include "TEveElement.h"
#include "TQObject.h"

class TEveWindow;
class TEveWindowSlot;

class TEveWindowManager : public TEveElementList,
                          public TQObject
{
private:
   TEveWindowManager(const TEveWindowManager&);            // Not implemented
   TEveWindowManager& operator=(const TEveWindowManager&); // Not implemented

protected:
   TEveWindow   *fCurrentWindow;

public:
   TEveWindowManager(const Text_t* n="TEveWindowManager", const Text_t* t="");
   virtual ~TEveWindowManager();

   void WindowDeleted (TEveWindow* w);
   void WindowSelected(TEveWindow* w);

   TEveWindow* GetCurrentWindow() const { return fCurrentWindow; }
   Bool_t      IsCurrentWindow(const TEveWindow* w) const { return w == fCurrentWindow; }

   TEveWindowSlot* GetCurrentWindowAsSlot() const;

   void CurrentWindowChanged(TEveWindow* window); // *SIGNAL*

   ClassDef(TEveWindowManager, 0); // Manager for EVE windows.
};

#endif
