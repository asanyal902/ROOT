// @(#)root/gui:$Name:  $:$Id: TGToolBar.h,v 1.6 2003/05/28 11:55:31 rdm Exp $
// Author: Fons Rademakers   25/02/98

/*************************************************************************
 * Copyright (C) 1995-2000, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#ifndef ROOT_TGToolBar
#define ROOT_TGToolBar


//////////////////////////////////////////////////////////////////////////
//                                                                      //
// TGToolBar                                                            //
//                                                                      //
// A toolbar is a composite frame that contains TGPictureButtons.       //
// Often used in combination with a TGHorizontal3DLine.                 //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

#ifndef ROOT_TGFrame
#include "TGFrame.h"
#endif

class TGButton;
class TList;


struct ToolBarData_t {
   const char *fPixmap;
   const char *fTipText;
   Bool_t      fStayDown;
   Int_t       fId;
   TGButton   *fButton;
};



class TGToolBar : public TGCompositeFrame {

private:
   TList   *fPictures;    // list of pictures that should be freed
   TList   *fTrash;       // list of buttons and layout hints to be deleted

public:
   TGToolBar(const TGWindow *p, UInt_t w, UInt_t h,
             UInt_t options = kHorizontalFrame,
             Pixel_t back = GetDefaultFrameBackground());
   virtual ~TGToolBar();

   void AddButton(const TGWindow *w, ToolBarData_t *button, Int_t spacing = 0);
   void Cleanup();

   ClassDef(TGToolBar,0)  //A bar containing picture buttons
};

#endif
