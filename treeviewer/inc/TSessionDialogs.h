// @(#)root/treeviewer:$Name:  $:$Id: TSessionDialogs.h
// Author: Marek Biskup, Jakub Madejczyk, Bertrand Bellenot 10/08/2005

/*************************************************************************
 * Copyright (C) 1995-2000, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#ifndef ROOT_TSessionDialogs
#define ROOT_TSessionDialogs

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// TSessionDialogs                                                      //
//                                                                      //
// This file defines several dialogs that are used by TSessionViewer    //
// The following dialogs are available: TNewChainDlg and TNewQueryDlg   //
//                                                                      //
//////////////////////////////////////////////////////////////////////////


#ifndef ROOT_TSessionViewer
#include <TSessionViewer.h>
#endif

class TList;
class TDSet;
class TGTextEntry;
class TGTextButton;
class TGTextBuffer;
class TGLabel;
class TGListView;
class TGPicture;
class TGListBox;
class TGFileContainer;

//////////////////////////////////////////////////////////////////////////
// New Chain Dialog
//////////////////////////////////////////////////////////////////////////

class TNewChainDlg : public TGTransientFrame {

private:
   TGFileContainer      *fContents;       // macro files container
   TGListView           *fListView;       // memory objects list view
   TGLVContainer        *fLVContainer;    // and its container
   TGTextBuffer         *fNameBuf;        // buffer for dataset name
   TGTextEntry          *fName;           // dataset name text entry
   TGTextButton         *fApplyButton;    // apply button
   TGTextButton         *fCloseButton;    // close button
   TSeqCollection       *fDSets;          // collection of datasets
   TDSet                *fDSet;           // actual dataset

public:
   TNewChainDlg(const TGWindow *p=0, const TGWindow *main=0);
   virtual ~TNewChainDlg();

   void         UpdateList();
   virtual void OnDoubleClick(TGLVEntry*,Int_t);
   virtual void DisplayDirectory(const TString &fname);
   void         OnElementDblClicked(TGLVEntry* entry, Int_t btn);
   void         OnElementSelected(TDSet *dset); //*SIGNAL*

   virtual Bool_t ProcessMessage(Long_t msg, Long_t parm1, Long_t parm2);
   virtual void CloseWindow();

   ClassDef(TNewChainDlg, 0)
};

//////////////////////////////////////////////////////////////////////////
// New Query Dialog
//////////////////////////////////////////////////////////////////////////

class TNewQueryDlg : public TGTransientFrame {

private:
   Bool_t            fEditMode;        // kTRUE if used to edit existing query
   TGCompositeFrame  *fFrmNewQuery;    // top (main) frame
   TGCompositeFrame  *fFrmMore;        // options frame
   TGTextButton      *fBtnMore;        // "more >>" / "less <<" button
   TGTextButton      *fBtnClose;       // close button
   TGTextButton      *fBtnSave;        // save button

   TGTextEntry       *fTxtQueryName;   // query name text entry
   TGTextEntry       *fTxtChain;       // chain name text entry
   TGTextEntry       *fTxtSelector;    // selector name text entry
   TGTextEntry       *fTxtOptions;     // options text entry
   TGNumberEntry     *fNumEntries;     // number of entries selector
   TGNumberEntry     *fNumFirstEntry;  // first entry selector
   TGTextEntry       *fTxtParFile;     // parameter file name text entry
   TGTextEntry       *fTxtEventList;   // event list text entry
   TSessionViewer    *fViewer;         // pointer on main viewer
   TQueryDescription *fQuery;          // query description class
   TDSet             *fDSet;           // actual dataset

public:
   TNewQueryDlg(TSessionViewer *gui, Int_t Width, Int_t Height,
                   TQueryDescription *query = 0, Bool_t editmode = kFALSE);
   virtual ~TNewQueryDlg();
   void     Build(TSessionViewer *gui);
   void     OnNewQueryMore();
   void     OnBrowseChain();
   void     OnBrowseSelector();
   void     OnBrowseParFile();
   void     OnBrowseEventList();
   void     OnBtnSaveClicked();
   void     OnBtnCloseClicked();
   void     OnElementSelected(TDSet *dset);
   void     CloseWindow();
   void     Popup();
   void     UpdateFields(TQueryDescription *desc);
   virtual Bool_t ProcessMessage(Long_t msg, Long_t parm1, Long_t parm2);

   ClassDef(TNewQueryDlg,0)
};

#endif
