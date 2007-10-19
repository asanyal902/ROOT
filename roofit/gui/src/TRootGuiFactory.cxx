// @(#)root/gui:$Id$
// Author: Fons Rademakers   15/01/98

/*************************************************************************
 * Copyright (C) 1995-2000, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// TRootGuiFactory                                                      //
//                                                                      //
// This class is a factory for ROOT GUI components. It overrides        //
// the member functions of the ABS TGuiFactory.                         //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

#include "TRootGuiFactory.h"
#include "TRootApplication.h"
#include "TRootCanvas.h"
#include "TRootBrowser.h"
#include "TRootContextMenu.h"
#include "TRootControlBar.h"
#include "TROOT.h"
#include "TPluginManager.h"

ClassImp(TRootGuiFactory)

//______________________________________________________________________________
TRootGuiFactory::TRootGuiFactory(const char *name, const char *title)
   : TGuiFactory(name, title)
{
   // TRootGuiFactory ctor.
}

//______________________________________________________________________________
TApplicationImp *TRootGuiFactory::CreateApplicationImp(const char *classname,
                      Int_t *argc, char **argv)
{
   // Create a ROOT native GUI version of TApplicationImp

   TRootApplication *app = new TRootApplication(classname, argc, argv);
   if (!app->Client()) {
      delete app;
      app = 0;
   }
   return app;
}

//______________________________________________________________________________
TCanvasImp *TRootGuiFactory::CreateCanvasImp(TCanvas *c, const char *title,
                                             UInt_t width, UInt_t height)
{
   // Create a ROOT native GUI version of TCanvasImp

   return new TRootCanvas(c, title, width, height);
}

//______________________________________________________________________________
TCanvasImp *TRootGuiFactory::CreateCanvasImp(TCanvas *c, const char *title,
                                  Int_t x, Int_t y, UInt_t width, UInt_t height)
{
   // Create a ROOT native GUI version of TCanvasImp

   return new TRootCanvas(c, title, x, y, width, height);
}

//______________________________________________________________________________
TBrowserImp *TRootGuiFactory::CreateBrowserImp(TBrowser *b, const char *title,
                                               UInt_t width, UInt_t height)
{
   // Create a ROOT native GUI version of TBrowserImp

   TPluginHandler *ph = gROOT->GetPluginManager()->FindHandler("TBrowserImp");

   if (ph && ph->LoadPlugin() != -1) {
      TBrowserImp *imp = (TBrowserImp *)ph->ExecPlugin(4, b, title, width, height);
      if (imp) return imp;
   }
   return new TRootBrowser(b, title, width, height);
}

//______________________________________________________________________________
TBrowserImp *TRootGuiFactory::CreateBrowserImp(TBrowser *b, const char *title,
                                  Int_t x, Int_t y, UInt_t width, UInt_t height)
{
   // Create a ROOT native GUI version of TBrowserImp

   TPluginHandler *ph = gROOT->GetPluginManager()->FindHandler("TBrowserImp");

   if (ph && ph->LoadPlugin() != -1) {
      TBrowserImp *imp = (TBrowserImp *)ph->ExecPlugin(6, b, title, x, y, width, height);
      if (imp) return imp;
   }
   return new TRootBrowser(b, title, x, y, width, height);
}

//______________________________________________________________________________
TContextMenuImp *TRootGuiFactory::CreateContextMenuImp(TContextMenu *c,
                                             const char *name, const char *)
{
   // Create a ROOT native GUI version of TContextMenuImp

   return new TRootContextMenu(c, name);
}

//______________________________________________________________________________
TControlBarImp *TRootGuiFactory::CreateControlBarImp(TControlBar *c, const char *title)
{
   // Create a ROOT native GUI version of TControlBarImp

   return new TRootControlBar(c, title);
}

//______________________________________________________________________________
TControlBarImp *TRootGuiFactory::CreateControlBarImp(TControlBar *c, const char *title,
                                                     Int_t x, Int_t y)
{
   // Create a ROOT native GUI version of TControlBarImp

   return new TRootControlBar(c, title, x, y);
}
