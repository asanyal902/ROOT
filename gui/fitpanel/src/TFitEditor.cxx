// @(#)root/fitpanel:$Id$
// Author: Ilka Antcheva, Lorenzo Moneta 10/08/2006

/*************************************************************************
 * Copyright (C) 1995-2006, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/


//////////////////////////////////////////////////////////////////////////
//                                                                      //
// TFitEditor                                                           //
//                                                                      //
// Allows to perform, explore and compare various fits.                 //
//                                                                      //
// To display the new Fit panel interface right click on a histogram    //
// or a graph to pop up the context menu and then select the menu       //
// entry 'Fit Panel'.                                                   //
//                                                                      //
// "General" Tab                                                        //
//                                                                      //
// The first set of GUI elements is related to the function choice      //
// and settings. The status bar on the bottom provides information      //
// about the current minimization settings using the following          //
// abbreviations:                                                       //
// LIB - shows the current choice between Minuit/Minuit2/Fumili         //
// MIGRAD or FUMILI points to the current minimization method in use.   //
// Itr: - shows the maximum number of iterations nnnn set for the fit.  //
// Prn: - can be DEF/VER/QT and shows the current print option in use.  //
//                                                                      //
// "Predefined" combo box - contains a list of predefined functions     //
// in ROOT. The default one is Gaussian.                                //
//                                                                      //
// "Operation" radio button group defines selected operational mode     //
// between functions: NOP - no operation (default); ADD - addition      //
// CONV - convolution (will be implemented in the future).              //
//                                                                      //
// Users can enter the function expression in a text entry field.       //
// The entered string is checked after Enter key was pressed. An        //
// error message shows up if the string is not accepted. The current    //
// prototype is limited and users have no freedom to enter file/user    //
// function names in this field.                                        //
//                                                                      //
// "Set Parameters" button opens a dialog for parameters settings.      //
//                                                                      //
// "Fit Settings" provides user interface elements related to the       //
// fitter. Currently there are two method choices: Chi-square and       //
// Binned Likelihood.                                                   //
//                                                                      //
// "Linear Fit" check button sets the use of Linear fitter is it is     //
// selected. Otherwise the option 'F' is applied if polN is selected.   //
// "Robust" number entry sets the robust value when fitting graphs.     //
// "No Chi-square" check button sets ON/OFF option 'C' - do not         //
// calculate Chi-square (for Linear fitter).                            //
//                                                                      //
// Fit options:                                                         //
// "Integral" check button switch ON/OFF option 'I' - use integral      //
// of function instead of value in bin center.                          //
// "Best Errors" sets ON/OFF option 'E' - better errors estimation      //
// using Minos technique.                                               //
// "All weights = 1" sets ON/OFF option 'W' - all weights set to 1,     //
// excluding empty bins and ignoring error bars.                        //
// "Empty bins, weights=1" sets ON/OFF option 'WW' -  all weights       //
// equal to 1, including  empty bins, error bars ignored.               //
// "Use range" sets ON/OFF option 'R' - fit only data within the        //
// specified function range with the slider.                            //
// "Improve fit results" sets ON/OFF option 'M' - after minimum is      //
// found, search for a new one.                                         //
// "Add to list" sets On/Off option '+'- add function to the list       //
// without deleting the previous.                                       //
//                                                                      //
// Draw options:                                                        //
// "SAME" sets On/Off function drawing on the same pad.                 //
// "No drawing" sets On/Off option '0'- do not draw function graphics.  //
// "Do not store/draw" sets On/Off option 'N'- do not store the         //
// function, do not draw it.                                            //
//                                                                      //
// Sliders settings are used if option 'R' - use range is active.       //
// Users can change min/max values by pressing the left mouse button    //
// near to the left/right slider edges. It is possible o change both    //
// values simultaneously by pressing the left mouse button near to its  //
// center and moving it to a new desire position.                       //
//                                                                      //
// "Minimization" Tab                                                   //
//                                                                      //
// "Library" group allows you to use Minuit, Minuit2 or Fumili          //
// minimization packages for your fit.                                  //
//  "Minuit" - the popular Minuit minimization package.                 //
//  "Minuit2" - a new object-oriented implementation of Minuit in C++.  //
//  "Fumili" - the popular Fumili minimization package.                 //
//                                                                      //
// "Method" group has currently restricted functionality.               //
//  "MIGRAD" method is available for Minuit and Minuit2                 //
//  "FUMILI" method is available for Fumili and Minuit2                 //
//  "SIMPLEX" method is disabled (will come with the new fitter design) //
//                                                                      //
// "Minimization Settings' group allows users to set values for:        //
//  "Error definition" - between 0.0 and 100.0  (default is 1.0).       //
//  "Maximum tolerance" - the fit relative precision in use.            //
//  "Maximum number of iterations" - default is 5000.                   //
//                                                                      //
// Print options:                                                       //
//  "Default" - between Verbose and Quiet.                              //
//  "Verbose" - prints results after each iteration.                    //
//  "Quiet" - no fit information is printed.                            //
//                                                                      //
// Fit button - performs a fit.                                         //
// Reset - resets all GUI elements and related fit settings to the      //
// default ones.                                                        //
// Close - closes this window.                                          //
//                                                                      //
// Begin_Html                                                           //
/*
<img src="gif/TFitEditor.gif">
*/
//End_Html
//////////////////////////////////////////////////////////////////////////

#include "TFitEditor.h"
#include "TROOT.h"
#include "TClass.h"
#include "TCanvas.h"
#include "TGTab.h"
#include "TGLabel.h"
#include "TG3DLine.h"
#include "TGComboBox.h"
#include "TGTextEntry.h"
#include "TGFont.h"
#include "TGGC.h"
#include "TGButtonGroup.h"
#include "TGNumberEntry.h"
#include "TGDoubleSlider.h"
#include "TGStatusBar.h"
#include "TFitParametersDialog.h"
#include "TGMsgBox.h"
#include "TAxis.h"
#include "TGraph.h"
#include "TGraph2D.h"
#include "TH1.h"
#include "TH2.h"
#include "HFitInterface.h"
#include "TF1.h"
#include "TF2.h"
#include "TF3.h"
#include "TTimer.h"
#include "THStack.h"
#include "TMath.h"
#include "Fit/DataRange.h"
#include "TMultiGraph.h"

enum EFitPanel {
   kFP_FLIST, kFP_GAUS,  kFP_GAUSN, kFP_EXPO,  kFP_LAND,  kFP_LANDN,
   kFP_POL0,  kFP_POL1,  kFP_POL2,  kFP_POL3,  kFP_POL4,  kFP_POL5,
   kFP_POL6,  kFP_POL7,  kFP_POL8,  kFP_POL9,  kFP_USER,
   kFP_NONE,  kFP_ADD,   kFP_CONV,  kFP_FILE,  kFP_PARS,  kFP_RBUST, kFP_EMPW1,
   kFP_INTEG, kFP_IMERR, kFP_USERG, kFP_ADDLS, kFP_ALLW1, kFP_IFITR, kFP_NOCHI,
   kFP_MLIST, kFP_MCHIS, kFP_MBINL, kFP_MUBIN, kFP_MUSER, kFP_MLINF, kFP_MUSR,
   kFP_DSAME, kFP_DNONE, kFP_DADVB, kFP_DNOST, kFP_PDEF,  kFP_PVER,  kFP_PQET,
   kFP_XMIN,  kFP_XMAX,  kFP_YMIN,  kFP_YMAX,  kFP_ZMIN,  kFP_ZMAX,
   
   kFP_LMIN,  kFP_LMIN2, kFP_LFUM,  kFP_MIGRAD,kFP_SIMPLX,kFP_FUMILI, kFP_COMBINATION,
   kFP_SCAN,  kFP_MERR,  kFP_MTOL,  kFP_MITR,
   
   kFP_FIT,   kFP_RESET, kFP_CLOSE
};

enum EParStruct {
   PAR_VAL = 0,
   PAR_MIN = 1,
   PAR_MAX = 2
};

void GetParameters(TFitEditor::FuncParams_t & pars, TF1* func)
{
   int npar = func->GetNpar(); 
   if (npar != (int) pars.size() ) pars.resize(npar); 
   for ( Int_t i = 0; i < npar; ++i )
   {
      Double_t par_min, par_max;
      pars[i][PAR_VAL] = func->GetParameter(i);
      func->GetParLimits(i, par_min, par_max);
      pars[i][PAR_MIN] = par_min;
      pars[i][PAR_MAX] = par_max;
   }
}

void SetParameters(TFitEditor::FuncParams_t & pars, TF1* func)
{
   int npar = func->GetNpar(); 
   if (npar > (int) pars.size() ) pars.resize(npar); 
   for ( Int_t i = 0; i < npar; ++i )
   {
      func->SetParameter(i, pars[i][PAR_VAL]);
      func->SetParLimits(i, pars[i][PAR_MIN], pars[i][PAR_MAX]);
   }
}

template<class FitObject>
void InitParameters(TF1* func, FitObject * fitobj)
{
   
   int special = func->GetNumber(); 
   if (special == 100 || special == 400) { 
      ROOT::Fit::InitGaus(fitobj, func);
      // case gaussian or Landau
   }
}

ClassImp(TFitEditor)

TFitEditor *TFitEditor::fgFitDialog = 0;

//______________________________________________________________________________
TFitEditor * TFitEditor::GetInstance(TVirtualPad* pad, TObject *obj)
{
   // Static method - opens the fit panel.

   if (!pad)
   {
      if (!gPad)
         gROOT->MakeDefCanvas();
      pad = gPad;
   }

   if (!fgFitDialog) {
      fgFitDialog = new TFitEditor(pad, obj);
   } else {
      fgFitDialog->Show(pad, obj);
   }
   return fgFitDialog;
}

//______________________________________________________________________________
TFitEditor::TFitEditor(TVirtualPad* pad, TObject *obj) :
   TGMainFrame(gClient->GetRoot(), 20, 20),
   fCanvas      (0),
   fParentPad   (0),
   fFitObject   (0),
   fDim         (0),
   fXaxis       (0),
   fYaxis       (0),
   fZaxis       (0),
   fXmin        (0),
   fXmax        (0),
   fYmin        (0),
   fYmax        (0),
   fZmin        (0),
   fZmax        (0),
   fFuncPars    (0)

{
   // Constructor of fit editor.

   SetCleanup(kDeepCleanup);
   
   TString name = obj->GetName();
   name.Append("::");
   name.Append(obj->ClassName());
   fObjLabelParent = new TGHorizontalFrame(this, 80, 20);
   TGLabel *label = new TGLabel(fObjLabelParent,"Current selection: ");
   fObjLabelParent->AddFrame(label, new TGLayoutHints(kLHintsLeft, 1, 1, 0, 0));
   fObjLabel = new TGLabel(fObjLabelParent, Form("%s", name.Data()));
   fObjLabelParent->AddFrame(fObjLabel, new TGLayoutHints(kLHintsLeft, 1, 1, 0, 0));
   AddFrame(fObjLabelParent, new TGLayoutHints(kLHintsTop, 1, 1, 10, 10));
   // set red color for the name
   Pixel_t color;
   gClient->GetColorByName("#ff0000", color);
   fObjLabel->SetTextColor(color, kFALSE);
   fObjLabel->SetTextJustify(kTextLeft);

   fTab = new TGTab(this, 10, 10);
   AddFrame(fTab, new TGLayoutHints(kLHintsExpandY | kLHintsExpandX));
   fTab->SetCleanup(kDeepCleanup);
   fTab->Associate(this);
   
   TGHorizontalFrame *cf1 = new TGHorizontalFrame(this, 250, 20, kFixedWidth);
   cf1->SetCleanup(kDeepCleanup);
   fFitButton = new TGTextButton(cf1, "&Fit", kFP_FIT);
   fFitButton->Associate(this);
   cf1->AddFrame(fFitButton, new TGLayoutHints(kLHintsTop |
                                               kLHintsExpandX, 2, 2, 2, 2));

   fResetButton = new TGTextButton(cf1, "&Reset", kFP_RESET);
   fResetButton->Associate(this);
   cf1->AddFrame(fResetButton, new TGLayoutHints(kLHintsTop |
                                                 kLHintsExpandX, 3, 2, 2, 2));

   fCloseButton = new TGTextButton(cf1, "&Close", kFP_CLOSE);
   fCloseButton->Associate(this);
   cf1->AddFrame(fCloseButton, new TGLayoutHints(kLHintsTop |
                                                 kLHintsExpandX, 3, 2, 2, 2));
   AddFrame(cf1, new TGLayoutHints(kLHintsNormal |
                                   kLHintsRight, 0, 5, 5, 5));

   // Create status bar
   int parts[] = { 20, 20, 20, 20, 20 };
   fStatusBar = new TGStatusBar(this, 10, 10);
   fStatusBar->SetParts(parts, 5);
   AddFrame(fStatusBar, new TGLayoutHints(kLHintsBottom | 
                                          kLHintsLeft   | 
                                          kLHintsExpandX));

   CreateGeneralTab();
   CreateMinimizationTab();

   gROOT->GetListOfCleanups()->Add(this);

   MapSubwindows();
   fGeneral->HideFrame(fSliderZParent);

   // do not allow resizing
   TGDimension size = GetDefaultSize();
   SetWindowName("Fit Panel");
   SetIconName("Fit Panel");
   SetClassHints("Fit Panel", "Fit Panel");

   SetMWMHints(kMWMDecorAll | kMWMDecorResizeH  | kMWMDecorMaximize |
                              kMWMDecorMinimize | kMWMDecorMenu,
               kMWMFuncAll  | kMWMFuncResize    | kMWMFuncMaximize |
                              kMWMFuncMinimize,
               kMWMInputModeless);

   if (pad && obj) {
      fParentPad = (TPad *)pad;
      fFitObject = (TObject *)obj;
      SetCanvas(pad->GetCanvas());
      pad->GetCanvas()->Selected(pad, obj, kButton1Down);
   } else {
      Error("FitPanel", "need to have an object drawn first");
      return;
   }
   UInt_t dw = fClient->GetDisplayWidth();
   UInt_t cw = pad->GetCanvas()->GetWindowWidth();
   UInt_t cx = (UInt_t)pad->GetCanvas()->GetWindowTopX();
   UInt_t cy = (UInt_t)pad->GetCanvas()->GetWindowTopY();

   if (cw + size.fWidth < dw) {
      Int_t gedx = 0, gedy = 0;
      gedx = cx+cw+4;
      gedy = cy-20;
      MoveResize(gedx, gedy,size.fWidth, size.fHeight);
      SetWMPosition(gedx, gedy);
   } 

   Resize(size);
   MapWindow();
   gVirtualX->RaiseWindow(GetId());

   ChangeOptions(GetOptions() | kFixedSize);
   SetWMSize(size.fWidth, size.fHeight);
   SetWMSizeHints(size.fWidth, size.fHeight, size.fWidth, size.fHeight, 0, 0);
}

//______________________________________________________________________________
TFitEditor::~TFitEditor()
{
   // Fit editor destructor.

   DisconnectSlots();
   fCloseButton->Disconnect("Clicked()");
   TQObject::Disconnect("TCanvas", "Selected(TVirtualPad *, TObject *, Int_t)");
   gROOT->GetListOfCleanups()->Remove(this);

   Cleanup();
   delete fLayoutNone;
   delete fLayoutAdd;
   delete fLayoutConv;
   fgFitDialog = 0;
}

//______________________________________________________________________________
void TFitEditor::CreateGeneralTab()
{
   // Create 'General' tab.

   fTabContainer = fTab->AddTab("General");
   fGeneral = new TGCompositeFrame(fTabContainer, 10, 10, kVerticalFrame);
   fTabContainer->AddFrame(fGeneral, new TGLayoutHints(kLHintsTop |
                                                       kLHintsExpandX,
                                                       5, 5, 2, 2));

   TGGroupFrame *gf1 = new TGGroupFrame(fGeneral, "Function", kFitWidth);
   TGCompositeFrame *tf1 = new TGCompositeFrame(gf1, 350, 26,
                                                kHorizontalFrame);
   TGVerticalFrame *tf11 = new TGVerticalFrame(tf1);
   TGLabel *label1 = new TGLabel(tf11,"Predefined:");
   tf11->AddFrame(label1, new TGLayoutHints(kLHintsNormal, 0, 0, 2, 0));
   fFuncList = BuildFunctionList(tf11, kFP_FLIST);
   fFuncList->Resize(80, 20);
   fFuncList->Select(1, kFALSE);

   TGListBox *lb = fFuncList->GetListBox();
   lb->Resize(lb->GetWidth(), 200);
   tf11->AddFrame(fFuncList, new TGLayoutHints(kLHintsNormal, 0, 0, 5, 0));
   fFuncList->Associate(this);
   tf1->AddFrame(tf11);

   TGHButtonGroup *bgr = new TGHButtonGroup(tf1,"Operation");
   bgr->SetRadioButtonExclusive();
   fNone = new TGRadioButton(bgr, "Nop", kFP_NONE);
   fNone->SetToolTipText("No operation defined");
   fNone->SetState(kButtonDown, kFALSE);
   fAdd = new TGRadioButton(bgr, "Add", kFP_ADD);
   fAdd->SetToolTipText("Addition");
   fConv = new TGRadioButton(bgr, "Conv", kFP_CONV);
   fConv->SetToolTipText("Convolution (not implemented yet)");
   fConv->SetState(kButtonDisabled);
   fLayoutNone = new TGLayoutHints(kLHintsLeft,0,5,3,-10);
   fLayoutAdd  = new TGLayoutHints(kLHintsLeft,10,5,3,-10);
   fLayoutConv = new TGLayoutHints(kLHintsLeft,10,5,3,-10);
   bgr->SetLayoutHints(fLayoutNone,fNone);
   bgr->SetLayoutHints(fLayoutAdd,fAdd);
   bgr->SetLayoutHints(fLayoutConv,fConv);
   bgr->Show();
   bgr->ChangeOptions(kFitWidth | kHorizontalFrame);
   tf1->AddFrame(bgr, new TGLayoutHints(kLHintsNormal, 15, 0, 3, 0));

   gf1->AddFrame(tf1, new TGLayoutHints(kLHintsNormal | kLHintsExpandX));

   TGCompositeFrame *tf2 = new TGCompositeFrame(gf1, 350, 26,
                                                kHorizontalFrame);
   fEnteredFunc = new TGTextEntry(tf2, new TGTextBuffer(50), kFP_FILE);
   fEnteredFunc->SetMaxLength(250);
   fEnteredFunc->SetAlignment(kTextLeft);
   TGTextLBEntry *te = (TGTextLBEntry *)fFuncList->GetSelectedEntry();
   fEnteredFunc->SetText(te->GetTitle());
   fEnteredFunc->SetToolTipText("Enter file_name/function_name or a function expression");
   fEnteredFunc->Resize(250,fEnteredFunc->GetDefaultHeight());
   tf2->AddFrame(fEnteredFunc, new TGLayoutHints(kLHintsLeft    |
                                                 kLHintsCenterY |
                                                 kLHintsExpandX, 2, 2, 2, 2));
   gf1->AddFrame(tf2, new TGLayoutHints(kLHintsNormal |
                                        kLHintsExpandX, 0, 0, 2, 0));

   TGHorizontalFrame *s1 = new TGHorizontalFrame(gf1);
   TGLabel *label21 = new TGLabel(s1, "Selected: ");
   s1->AddFrame(label21, new TGLayoutHints(kLHintsNormal |
                                           kLHintsCenterY, 2, 2, 2, 0));
   TGHorizontal3DLine *hlines = new TGHorizontal3DLine(s1);
   s1->AddFrame(hlines, new TGLayoutHints(kLHintsCenterY | kLHintsExpandX));
   gf1->AddFrame(s1, new TGLayoutHints(kLHintsExpandX));

   TGCompositeFrame *tf4 = new TGCompositeFrame(gf1, 350, 26,
                                                kHorizontalFrame);
   TGTextLBEntry *txt = (TGTextLBEntry *)fFuncList->GetSelectedEntry();
   fSelLabel = new TGLabel(tf4, Form("%s", txt->GetTitle()));
   tf4->AddFrame(fSelLabel, new TGLayoutHints(kLHintsNormal |
                                              kLHintsCenterY, 0, 6, 2, 0));
   Pixel_t color;
   gClient->GetColorByName("#336666", color);
   fSelLabel->SetTextColor(color, kFALSE);
   TGCompositeFrame *tf5 = new TGCompositeFrame(tf4, 120, 20,
                                                kHorizontalFrame | kFixedWidth);
   fSetParam = new TGTextButton(tf5, "Set Parameters...", kFP_PARS);
   tf5->AddFrame(fSetParam, new TGLayoutHints(kLHintsRight   |
                                              kLHintsCenterY |
                                              kLHintsExpandX));
   fSetParam->SetToolTipText("Open a dialog for parameter(s) settings");
   tf4->AddFrame(tf5, new TGLayoutHints(kLHintsRight |
                                        kLHintsTop, 5, 0, 2, 2));
   gf1->AddFrame(tf4, new TGLayoutHints(kLHintsNormal |
                                             kLHintsExpandX, 5, 0, 0, 0));

   fGeneral->AddFrame(gf1, new TGLayoutHints(kLHintsExpandX, 5, 5, 0, 0));


   // 'options' group frame
   TGGroupFrame *gf = new TGGroupFrame(fGeneral, "Fit Settings", kFitWidth);

   // 'method' sub-group
   TGHorizontalFrame *h1 = new TGHorizontalFrame(gf);
   TGLabel *label4 = new TGLabel(h1, "Method");
   h1->AddFrame(label4, new TGLayoutHints(kLHintsNormal |
                                          kLHintsCenterY, 2, 2, 0, 0));
   TGHorizontal3DLine *hline1 = new TGHorizontal3DLine(h1);
   h1->AddFrame(hline1, new TGLayoutHints(kLHintsCenterY | kLHintsExpandX));
   gf->AddFrame(h1, new TGLayoutHints(kLHintsExpandX));

   TGHorizontalFrame *h2 = new TGHorizontalFrame(gf);
   TGVerticalFrame *v1 = new TGVerticalFrame(h2);
   fMethodList = BuildMethodList(v1, kFP_MLIST);
   fMethodList->Select(1, kFALSE);
   fMethodList->Resize(130, 20);
   lb = fMethodList->GetListBox();
   Int_t lbe = lb->GetNumberOfEntries();
   lb->Resize(lb->GetWidth(), lbe*16);
   v1->AddFrame(fMethodList, new TGLayoutHints(kLHintsLeft, 0, 0, 2, 5));

   fLinearFit = new TGCheckButton(v1, "Linear fit", kFP_MLINF);
   fLinearFit->Associate(this);
   fLinearFit->SetToolTipText("Perform Linear fitter if selected");
   v1->AddFrame(fLinearFit, new TGLayoutHints(kLHintsNormal, 0, 0, 2, 2));

   TGHorizontalFrame *v1h = new TGHorizontalFrame(v1);
   TGLabel *label41 = new TGLabel(v1h, "Robust:");
   v1h->AddFrame(label41, new TGLayoutHints(kLHintsNormal |
                                            kLHintsCenterY, 25, 5, 5, 2));
   fRobustValue = new TGNumberEntry(v1h, 1., 5, kFP_RBUST,
                                    TGNumberFormat::kNESRealTwo,
                                    TGNumberFormat::kNEAPositive,
                                    TGNumberFormat::kNELLimitMinMax,0.,1.);
   v1h->AddFrame(fRobustValue, new TGLayoutHints(kLHintsLeft));
   v1->AddFrame(v1h, new TGLayoutHints(kLHintsNormal));
   fRobustValue->SetState(kFALSE);
   fRobustValue->GetNumberEntry()->SetToolTipText("Available only for graphs");

   h2->AddFrame(v1, new TGLayoutHints(kLHintsExpandX | kLHintsExpandY));

   TGVerticalFrame *v2 = new TGVerticalFrame(h2);
   TGCompositeFrame *v21 = new TGCompositeFrame(v2, 120, 20,
                                                kHorizontalFrame | kFixedWidth);
   fUserButton = new TGTextButton(v21, "User-Defined...", kFP_MUSR);
   v21->AddFrame(fUserButton, new TGLayoutHints(kLHintsRight   |
                                                kLHintsCenterY |
                                                kLHintsExpandX));
   fUserButton->SetToolTipText("Open a dialog for entering a user-defined method");
   fUserButton->SetState(kButtonDisabled);
   v2->AddFrame(v21, new TGLayoutHints(kLHintsRight | kLHintsTop));

   fNoChi2 = new TGCheckButton(v2, "No Chi-square", kFP_NOCHI);
   fNoChi2->Associate(this);
   fNoChi2->SetToolTipText("'C'- do not calculate Chi-square (for Linear fitter)");
   v2->AddFrame(fNoChi2, new TGLayoutHints(kLHintsNormal, 0, 0, 34, 2));

   h2->AddFrame(v2, new TGLayoutHints(kLHintsExpandX | kLHintsExpandY, 20, 0, 0, 0));
   gf->AddFrame(h2, new TGLayoutHints(kLHintsExpandX, 20, 0, 0, 0));

   // 'fit option' sub-group
   TGHorizontalFrame *h3 = new TGHorizontalFrame(gf);
   TGLabel *label5 = new TGLabel(h3, "Fit Options");
   h3->AddFrame(label5, new TGLayoutHints(kLHintsNormal |
                                          kLHintsCenterY, 2, 2, 0, 0));
   TGHorizontal3DLine *hline2 = new TGHorizontal3DLine(h3);
   h3->AddFrame(hline2, new TGLayoutHints(kLHintsCenterY | kLHintsExpandX));
   gf->AddFrame(h3, new TGLayoutHints(kLHintsExpandX));

   TGHorizontalFrame *h = new TGHorizontalFrame(gf);
   TGVerticalFrame *v3 = new TGVerticalFrame(h);
   fIntegral = new TGCheckButton(v3, "Integral", kFP_INTEG);
   fIntegral->Associate(this);
   fIntegral->SetToolTipText("'I'- use integral of function instead of value in bin center");
   v3->AddFrame(fIntegral, new TGLayoutHints(kLHintsNormal, 0, 0, 2, 2));

   fBestErrors = new TGCheckButton(v3, "Best errors", kFP_IMERR);
   fBestErrors->Associate(this);
   fBestErrors->SetToolTipText("'E'- better errors estimation using Minos technique");
   v3->AddFrame(fBestErrors, new TGLayoutHints(kLHintsNormal, 0, 0, 2, 2));

   fAllWeights1 = new TGCheckButton(v3, "All weights = 1", kFP_ALLW1);
   fAllWeights1->Associate(this);
   fAllWeights1->SetToolTipText("'W'- all weights=1 for non empty bins; error bars ignored");
   v3->AddFrame(fAllWeights1, new TGLayoutHints(kLHintsNormal, 0, 0, 2, 2));

   fEmptyBinsWghts1 = new TGCheckButton(v3, "Empty bins, weights=1", kFP_EMPW1);
   fEmptyBinsWghts1->Associate(this);
   fEmptyBinsWghts1->SetToolTipText("'WW'- all weights=1 including empty bins; error bars ignored");
   v3->AddFrame(fEmptyBinsWghts1, new TGLayoutHints(kLHintsNormal, 0, 0, 2, 2));

   h->AddFrame(v3, new TGLayoutHints(kLHintsExpandX | kLHintsExpandY));

   TGVerticalFrame *v4 = new TGVerticalFrame(h);
   fUseRange = new TGCheckButton(v4, "Use range", kFP_USERG);
   fUseRange->Associate(this);
   fUseRange->SetToolTipText("'R'- fit only data within the specified function range");
   v4->AddFrame(fUseRange, new TGLayoutHints(kLHintsNormal, 0, 0, 2, 2));

   fImproveResults = new TGCheckButton(v4, "Improve fit results", kFP_IFITR);
   fImproveResults->Associate(this);
   fImproveResults->SetToolTipText("'M'- after minimum is found, search for a new one");
   v4->AddFrame(fImproveResults, new TGLayoutHints(kLHintsNormal, 0, 0, 2, 2));

   fAdd2FuncList = new TGCheckButton(v4, "Add to list", kFP_ADDLS);
   fAdd2FuncList->Associate(this);
   fAdd2FuncList->SetToolTipText("'+'- add function to the list without deleting the previous");
   v4->AddFrame(fAdd2FuncList, new TGLayoutHints(kLHintsNormal, 0, 0, 2, 2));

   fUseGradient = new TGCheckButton(v4, "Use Gradient", kFP_ADDLS);
   fUseGradient->Associate(this);
   fUseGradient->SetToolTipText("'G'- Use the gradient as an aid for the fitting");
   v4->AddFrame(fUseGradient, new TGLayoutHints(kLHintsNormal, 0, 0, 2, 2));

   h->AddFrame(v4, new TGLayoutHints(kLHintsExpandX | kLHintsExpandY, 20, 0, 0, 0));
   gf->AddFrame(h, new TGLayoutHints(kLHintsExpandX, 20, 0, 0, 0));

   // 'draw option' sub-group
   TGHorizontalFrame *h5 = new TGHorizontalFrame(gf);
   TGLabel *label6 = new TGLabel(h5, "Draw Options");
   h5->AddFrame(label6, new TGLayoutHints(kLHintsNormal |
                                          kLHintsCenterY, 2, 2, 2, 2));
   TGHorizontal3DLine *hline3 = new TGHorizontal3DLine(h5);
   h5->AddFrame(hline3, new TGLayoutHints(kLHintsCenterY | kLHintsExpandX));
   gf->AddFrame(h5, new TGLayoutHints(kLHintsExpandX));

   TGHorizontalFrame *h6 = new TGHorizontalFrame(gf);
   TGVerticalFrame *v5 = new TGVerticalFrame(h6);

   fDrawSame = new TGCheckButton(v5, "SAME", kFP_DSAME);
   fDrawSame->Associate(this);
   fDrawSame->SetToolTipText("Superimpose on previous picture in the same pad");
   v5->AddFrame(fDrawSame, new TGLayoutHints(kLHintsNormal, 0, 0, 2, 2));

   fNoDrawing = new TGCheckButton(v5, "No drawing", kFP_DNONE);
   fNoDrawing->Associate(this);
   fNoDrawing->SetToolTipText("'0'- do not draw function graphics");
   v5->AddFrame(fNoDrawing, new TGLayoutHints(kLHintsNormal, 0, 0, 2, 2));

   fNoStoreDrawing = new TGCheckButton(v5, "Do not store/draw", kFP_DNOST);
   fNoStoreDrawing->Associate(this);
   fNoStoreDrawing->SetToolTipText("'N'- do not store the function, do not draw it");
   v5->AddFrame(fNoStoreDrawing, new TGLayoutHints(kLHintsNormal, 0, 0, 2, 2));

   h6->AddFrame(v5, new TGLayoutHints(kLHintsExpandX | kLHintsExpandY));

   TGVerticalFrame *v6 = new TGVerticalFrame(h6);
   TGCompositeFrame *v61 = new TGCompositeFrame(v6, 120, 20,
                                                kHorizontalFrame | kFixedWidth);
   fDrawAdvanced = new TGTextButton(v61, "Advanced...", kFP_DADVB);
   v61->AddFrame(fDrawAdvanced, new TGLayoutHints(kLHintsRight   |
                                                  kLHintsCenterY |
                                                  kLHintsExpandX));
   fDrawAdvanced->SetToolTipText("Open a dialog for advanced draw options");
   fDrawAdvanced->SetState(kButtonDisabled);

   v6->AddFrame(v61, new TGLayoutHints(kLHintsRight | kLHintsTop,
                                       0, 0, (4+fDrawSame->GetHeight())*2, 0));

   h6->AddFrame(v6, new TGLayoutHints(kLHintsExpandX | kLHintsExpandY));
   gf->AddFrame(h6, new TGLayoutHints(kLHintsExpandX, 20, 0, 2, 0));

   fGeneral->AddFrame(gf, new TGLayoutHints(kLHintsExpandX |
                                            kLHintsExpandY, 5, 5, 0, 0));
   // sliderX
   fSliderXParent = new TGHorizontalFrame(fGeneral);
   TGLabel *label8 = new TGLabel(fSliderXParent, "X:");
   fSliderXParent->AddFrame(label8, new TGLayoutHints(kLHintsLeft |
                                                      kLHintsCenterY, 0, 5, 0, 0));
   fSliderX = new TGDoubleHSlider(fSliderXParent, 1, kDoubleScaleBoth);
   fSliderX->SetScale(5);
   fSliderXParent->AddFrame(fSliderX, new TGLayoutHints(kLHintsExpandX | 
                                                        kLHintsCenterY));
   fGeneral->AddFrame(fSliderXParent, new TGLayoutHints(kLHintsExpandX, 5, 5, 0, 0));

   // sliderY - no implemented functionality yet
   fSliderYParent = new TGHorizontalFrame(fGeneral);
   TGLabel *label9 = new TGLabel(fSliderYParent, "Y:");
   fSliderYParent->AddFrame(label9, new TGLayoutHints(kLHintsLeft |
                                                      kLHintsCenterY, 0, 5, 0, 0));
   fSliderY = new TGDoubleHSlider(fSliderYParent, 1, kDoubleScaleBoth);
   fSliderY->SetScale(5);
   fSliderYParent->AddFrame(fSliderY, new TGLayoutHints(kLHintsExpandX | 
                                                        kLHintsCenterY));
   fGeneral->AddFrame(fSliderYParent, new TGLayoutHints(kLHintsExpandX, 5, 5, 0, 0));

   // sliderZ
   fSliderZParent = new TGHorizontalFrame(fGeneral);
   TGLabel *label10 = new TGLabel(fSliderZParent, "Z:");
   fSliderZParent->AddFrame(label10, new TGLayoutHints(kLHintsLeft |
                                                       kLHintsCenterY, 0, 5, 0, 0));
   fSliderZ = new TGDoubleHSlider(fSliderZParent, 1, kDoubleScaleBoth);
   fSliderZ->SetScale(5);
   fSliderZParent->AddFrame(fSliderZ, new TGLayoutHints(kLHintsExpandX | 
                                                        kLHintsCenterY));
   fGeneral->AddFrame(fSliderZParent, new TGLayoutHints(kLHintsExpandX, 5, 5, 0, 0));
}


//______________________________________________________________________________
void TFitEditor::CreateMinimizationTab()
{
   // Create 'Minimization' tab.
   
   fTabContainer = fTab->AddTab("Minimization");
   fMinimization = new TGCompositeFrame(fTabContainer, 10, 10, kVerticalFrame);
   fTabContainer->AddFrame(fMinimization, new TGLayoutHints(kLHintsTop |
                                                            kLHintsExpandX,
                                                            5, 5, 2, 2));
   MakeTitle(fMinimization, "Library");

   TGHorizontalFrame *hl = new TGHorizontalFrame(fMinimization);
   fLibMinuit = new TGRadioButton(hl, "Minuit", kFP_LMIN);
   fLibMinuit->Associate(this);
   fLibMinuit->SetToolTipText("Use minimization from libMinuit (default)");
   hl->AddFrame(fLibMinuit, new TGLayoutHints(kLHintsNormal, 40, 0, 0, 1));
   fStatusBar->SetText("LIB Minuit",0);

   fLibMinuit2 = new TGRadioButton(hl, "Minuit2", kFP_LMIN2);
   fLibMinuit2->Associate(this);
   fLibMinuit2->SetToolTipText("New C++ version of Minuit");
   hl->AddFrame(fLibMinuit2, new TGLayoutHints(kLHintsNormal, 35, 0, 0, 1));

   fLibFumili = new TGRadioButton(hl, "Fumili", kFP_LFUM);
   fLibFumili->Associate(this);
   fLibFumili->SetToolTipText("Use minimization from libFumili");
   hl->AddFrame(fLibFumili, new TGLayoutHints(kLHintsNormal, 30, 0, 0, 1));
   fMinimization->AddFrame(hl, new TGLayoutHints(kLHintsExpandX, 20, 0, 5, 1));

   MakeTitle(fMinimization, "Method");

   TGHorizontalFrame *hm = new TGHorizontalFrame(fMinimization);
   fMigrad = new TGRadioButton(hm, "MIGRAD", kFP_MIGRAD);
   fMigrad->Associate(this);
   fMigrad->SetToolTipText("Use MIGRAD as minimization method");
   hm->AddFrame(fMigrad, new TGLayoutHints(kLHintsNormal, 40, 0, 0, 1));
   fStatusBar->SetText("MIGRAD",1);

   fSimplex = new TGRadioButton(hm, "SIMPLEX", kFP_SIMPLX);
   fSimplex->Associate(this);
   fSimplex->SetToolTipText("Use SIMPLEX as minimization method");
   hm->AddFrame(fSimplex, new TGLayoutHints(kLHintsNormal, 20, 0, 0, 1));

   fFumili = new TGRadioButton(hm, "FUMILI", kFP_FUMILI);
   fFumili->Associate(this);
   fFumili->SetToolTipText("Use FUMILI as minimization method");
   hm->AddFrame(fFumili, new TGLayoutHints(kLHintsNormal, 18, 0, 0, 1));
   fMinimization->AddFrame(hm, new TGLayoutHints(kLHintsExpandX, 20, 0, 5, 1));

   TGHorizontalFrame *hm2 = new TGHorizontalFrame(fMinimization);
   fScan = new TGRadioButton(hm2, "SCAN", kFP_SCAN);
   fScan->Associate(this);
   fScan->SetToolTipText("Use SCAN as minimization method");
   hm2->AddFrame(fScan, new TGLayoutHints(kLHintsNormal, 40, 0, 0, 1));

   fCombination = new TGRadioButton(hm2, "Combination", kFP_COMBINATION);
   fCombination->Associate(this);
   fCombination->SetToolTipText("Use Combination as minimization method");
   hm2->AddFrame(fCombination, new TGLayoutHints(kLHintsNormal, 34, 0, 0, 1));
   fMinimization->AddFrame(hm2, new TGLayoutHints(kLHintsExpandX, 20, 0, 5, 1));

   // Set the status to the default minimization options!
   if ( ROOT::Math::MinimizerOptions::DefaultMinimizerType() == "Fumili" ) {
      fLibFumili->SetState(kButtonDown);
      fMigrad->SetState(kButtonDisabled);
      fScan->SetState(kButtonDisabled);
      fCombination->SetState(kButtonDisabled);
      fSimplex->SetState(kButtonDisabled);
      fFumili->SetState(kButtonDown);
   } else if ( ROOT::Math::MinimizerOptions::DefaultMinimizerType() == "Minuit" ) {
      fLibMinuit->SetState(kButtonDown);
      fFumili->SetState(kButtonDisabled);
   } else {
      fLibMinuit2->SetState(kButtonDown);
   }

   if ( ROOT::Math::MinimizerOptions::DefaultMinimizerAlgo() == "Fumili" )
      fFumili->SetState(kButtonDown);
   else if ( ROOT::Math::MinimizerOptions::DefaultMinimizerAlgo() == "Migrad" )
      fMigrad->SetState(kButtonDown);
   else if ( ROOT::Math::MinimizerOptions::DefaultMinimizerAlgo() == "Simplex" )
      fSimplex->SetState(kButtonDown);
   else if ( ROOT::Math::MinimizerOptions::DefaultMinimizerAlgo() == "Minimize" )
      fCombination->SetState(kButtonDown);
   else if ( ROOT::Math::MinimizerOptions::DefaultMinimizerAlgo() == "Scan" )
      fScan->SetState(kButtonDown);

   MakeTitle(fMinimization, "Settings");
   TGLabel *hslabel1 = new TGLabel(fMinimization,"Use ENTER key to validate a new value or click");
   fMinimization->AddFrame(hslabel1, new TGLayoutHints(kLHintsNormal, 61, 0, 5, 1));
   TGLabel *hslabel2 = new TGLabel(fMinimization,"on Reset button to set the defaults.");
   fMinimization->AddFrame(hslabel2, new TGLayoutHints(kLHintsNormal, 61, 0, 1, 10));

   TGHorizontalFrame *hs = new TGHorizontalFrame(fMinimization);
   
   TGVerticalFrame *hsv1 = new TGVerticalFrame(hs, 180, 10, kFixedWidth);
   TGLabel *errlabel = new TGLabel(hsv1,"Error definition (default = 1): ");
   hsv1->AddFrame(errlabel, new TGLayoutHints(kLHintsLeft | kLHintsCenterY, 
                                              1, 1, 5, 7));
   TGLabel *tollabel = new TGLabel(hsv1,"Max tolerance (precision): ");
   hsv1->AddFrame(tollabel, new TGLayoutHints(kLHintsLeft | kLHintsCenterY, 
                                              1, 1, 5, 7));
   TGLabel *itrlabel = new TGLabel(hsv1,"Max number of iterations: ");
   hsv1->AddFrame(itrlabel, new TGLayoutHints(kLHintsLeft | kLHintsCenterY, 
                                              1, 1, 5, 5));
   hs->AddFrame(hsv1, new TGLayoutHints(kLHintsNormal, 60, 0, 0, 0));
   
   TGVerticalFrame *hsv2 = new TGVerticalFrame(hs, 90,10, kFixedWidth);
   fErrorScale = new TGNumberEntryField(hsv2, kFP_MERR, ROOT::Math::MinimizerOptions::DefaultErrorDef(),
                                        TGNumberFormat::kNESRealTwo,
                                        TGNumberFormat::kNEAPositive,
                                        TGNumberFormat::kNELLimitMinMax,0.,100.);
   hsv2->AddFrame(fErrorScale, new TGLayoutHints(kLHintsLeft | kLHintsExpandX, 
                                                 1, 1, 0, 3));
   fTolerance = new TGNumberEntryField(hsv2, kFP_MTOL, ROOT::Math::MinimizerOptions::DefaultTolerance(), 
                                       TGNumberFormat::kNESReal,
                                       TGNumberFormat::kNEAPositive,
                                       TGNumberFormat::kNELLimitMinMax, 0., 1.);
   fTolerance->SetNumber(ROOT::Math::MinimizerOptions::DefaultTolerance());
   hsv2->AddFrame(fTolerance, new TGLayoutHints(kLHintsLeft | kLHintsExpandX, 
                                                1, 1, 3, 3));
   fIterations = new TGNumberEntryField(hsv2, kFP_MITR, 5000, 
                                   TGNumberFormat::kNESInteger,
                                   TGNumberFormat::kNEAPositive,
                                   TGNumberFormat::kNELNoLimits);
   fIterations->SetNumber(ROOT::Math::MinimizerOptions::DefaultMaxIterations());
   hsv2->AddFrame(fIterations, new TGLayoutHints(kLHintsLeft | kLHintsExpandX, 
                                                 1, 1, 3, 3));
   hs->AddFrame(hsv2, new TGLayoutHints(kLHintsNormal, 0, 0, 0, 0));
   fMinimization->AddFrame(hs, new TGLayoutHints(kLHintsExpandX, 0, 0, 1, 1));
   fStatusBar->SetText(Form("Itr: %d",ROOT::Math::MinimizerOptions::DefaultMaxIterations()),2);

   MakeTitle(fMinimization, "Print Options");

   TGHorizontalFrame *h8 = new TGHorizontalFrame(fMinimization);
   fOptDefault = new TGRadioButton(h8, "Default", kFP_PDEF);
   fOptDefault->Associate(this);
   fOptDefault->SetToolTipText("Default is between Verbose and Quiet");
   h8->AddFrame(fOptDefault, new TGLayoutHints(kLHintsNormal, 40, 0, 0, 1));
   fOptDefault->SetState(kButtonDown);
   fStatusBar->SetText("Prn: DEF",3);

   fOptVerbose = new TGRadioButton(h8, "Verbose", kFP_PVER);
   fOptVerbose->Associate(this);
   fOptVerbose->SetToolTipText("'V'- print results after each iteration");
   h8->AddFrame(fOptVerbose, new TGLayoutHints(kLHintsNormal, 30, 0, 0, 1));

   fOptQuiet = new TGRadioButton(h8, "Quiet", kFP_PQET);
   fOptQuiet->Associate(this);
   fOptQuiet->SetToolTipText("'Q'- no print");
   h8->AddFrame(fOptQuiet, new TGLayoutHints(kLHintsNormal, 25, 0, 0, 1));

   fMinimization->AddFrame(h8, new TGLayoutHints(kLHintsExpandX, 20, 0, 5, 1));

}

//______________________________________________________________________________
void TFitEditor::ConnectSlots()
{
   // Connect GUI signals to fit panel slots.

   // list of predefined functions
   fFuncList->Connect("Selected(Int_t)", "TFitEditor", this, "DoFunction(Int_t)");
   // entered formula or function name
   fEnteredFunc->Connect("ReturnPressed()", "TFitEditor", this, "DoEnteredFunction()");
   // set parameters dialog
   fSetParam->Connect("Clicked()", "TFitEditor", this, "DoSetParameters()");
   // allowed function operations
   fAdd->Connect("Toggled(Bool_t)","TFitEditor", this, "DoAddition(Bool_t)");

   // fit options
   fAllWeights1->Connect("Toggled(Bool_t)","TFitEditor",this,"DoAllWeights1()");
   fEmptyBinsWghts1->Connect("Toggled(Bool_t)","TFitEditor",this,"DoEmptyBinsAllWeights1()");

   // linear fit
   fLinearFit->Connect("Toggled(Bool_t)","TFitEditor",this,"DoLinearFit()");
   fNoChi2->Connect("Toggled(Bool_t)","TFitEditor",this,"DoNoChi2()");

   // draw options
   fNoStoreDrawing->Connect("Toggled(Bool_t)","TFitEditor",this,"DoNoStoreDrawing()");

   // fit, reset, close buttons
   fFitButton->Connect("Clicked()", "TFitEditor", this, "DoFit()");
   fResetButton->Connect("Clicked()", "TFitEditor", this, "DoReset()");
   fCloseButton->Connect("Clicked()", "TFitEditor", this, "DoClose()");

   // user method button
   fUserButton->Connect("Clicked()", "TFitEditor", this, "DoUserDialog()");
   // advanced draw options
   fDrawAdvanced->Connect("Clicked()", "TFitEditor", this, "DoAdvancedOptions()");

   if (fDim > 0)
      fSliderX->Connect("PositionChanged()","TFitEditor",this, "DoSliderXMoved()");
   if (fDim > 1)
      fSliderY->Connect("PositionChanged()","TFitEditor",this, "DoSliderYMoved()");
   if (fDim > 2)
      fSliderZ->Connect("PositionChanged()","TFitEditor",this, "DoSliderZMoved()");

   fParentPad->Connect("RangeAxisChanged()", "TFitEditor", this, "UpdateGUI()");
   
   // 'Minimization' tab
   // library
   fLibMinuit->Connect("Toggled(Bool_t)","TFitEditor",this,"DoLibrary(Bool_t)");
   fLibMinuit2->Connect("Toggled(Bool_t)","TFitEditor",this,"DoLibrary(Bool_t)");
   fLibFumili->Connect("Toggled(Bool_t)","TFitEditor",this,"DoLibrary(Bool_t)");

   // minimization method
   fMigrad->Connect("Toggled(Bool_t)","TFitEditor",this,"DoMinMethod(Bool_t)");
   // Simplex functionality will come with the new fitter design
   fSimplex->Connect("Toggled(Bool_t)","TFitEditor",this,"DoMinMethod(Bool_t)");
   fFumili->Connect("Toggled(Bool_t)","TFitEditor",this,"DoMinMethod(Bool_t)");
   fCombination->Connect("Toggled(Bool_t)","TFitEditor",this,"DoMinMethod(Bool_t)");
   fScan->Connect("Toggled(Bool_t)","TFitEditor",this,"DoMinMethod(Bool_t)");

   // fitter settings
   fIterations->Connect("ReturnPressed()", "TFitEditor", this, "DoMaxIterations()");
   
   // print options
   fOptDefault->Connect("Toggled(Bool_t)","TFitEditor",this,"DoPrintOpt(Bool_t)");
   fOptVerbose->Connect("Toggled(Bool_t)","TFitEditor",this,"DoPrintOpt(Bool_t)");
   fOptQuiet->Connect("Toggled(Bool_t)","TFitEditor",this,"DoPrintOpt(Bool_t)");

}

//______________________________________________________________________________
void TFitEditor::DisconnectSlots()
{
   // Disconnect GUI signals from fit panel slots.

   Disconnect("CloseWindow()");

   fFuncList->Disconnect("Selected(Int_t)");
   fEnteredFunc->Disconnect("ReturnPressed()");
   fSetParam->Disconnect("Clicked()");
   fAdd->Disconnect("Toggled(Bool_t)");

   // fit options
   fAllWeights1->Disconnect("Toggled(Bool_t)");
   fEmptyBinsWghts1->Disconnect("Toggled(Bool_t)");

   // linear fit
   fLinearFit->Disconnect("Toggled(Bool_t)");
   fNoChi2->Disconnect("Toggled(Bool_t)");

   // draw options
   fNoStoreDrawing->Disconnect("Toggled(Bool_t)");

   // fit, reset, close buttons
   fFitButton->Disconnect("Clicked()");
   fResetButton->Disconnect("Clicked()");
   
   // other methods
   fUserButton->Disconnect("Clicked()");
   fDrawAdvanced->Disconnect("Clicked()");

   if (fDim > 0)
      fSliderX->Disconnect("PositionChanged()");
   if (fDim > 1)
      fSliderY->Disconnect("PositionChanged()");
   if (fDim > 2) 
      fSliderZ->Disconnect("PositionChanged()");
   
   // slots related to 'Minimization' tab
   fLibMinuit->Disconnect("Toggled(Bool_t)");
   fLibMinuit2->Disconnect("Toggled(Bool_t)");
   fLibFumili->Disconnect("Toggled(Bool_t)");

   // minimization method
   fMigrad->Disconnect("Toggled(Bool_t)");
   // Simplex functionality will come with the new fitter design
   fSimplex->Disconnect("Toggled(Bool_t)");
   fFumili->Disconnect("Toggled(Bool_t)");
   fCombination->Disconnect("Toggled(Bool_t)");
   fScan->Disconnect("Toggled(Bool_t)");
   
   // fitter settings
   fIterations->Disconnect("ReturnPressed()");

   // print options
   fOptDefault->Disconnect("Toggled(Bool_t)");
   fOptVerbose->Disconnect("Toggled(Bool_t)");
   fOptQuiet->Disconnect("Toggled(Bool_t)");

}

//______________________________________________________________________________
void TFitEditor::SetCanvas(TCanvas *newcan)
{
   // Connect to another canvas.

   if (!newcan || (fCanvas == newcan)) return;

   fCanvas = newcan;
   fParentPad = fCanvas->GetSelectedPad();
   if (!fParentPad) fParentPad = fCanvas;
   newcan->Connect("Selected(TVirtualPad*,TObject*,Int_t)", "TFitEditor",
                   this, "SetFitObject(TVirtualPad *, TObject *, Int_t)");
   ConnectToCanvas();
}

//______________________________________________________________________________
void TFitEditor::ConnectToCanvas()
{
   // Connect fit panel to the 'Selected' signal of canvas 'c'.

   TQObject::Connect("TCanvas", "Selected(TVirtualPad *, TObject *, Int_t)", 
                     "TFitEditor",this, 
                     "SetFitObject(TVirtualPad *, TObject *, Int_t)");
   TQObject::Connect("TCanvas", "Closed()", "TFitEditor", this, "DoNoSelection()");
}

//______________________________________________________________________________
void TFitEditor::Hide()
{
   // Hide the fit panel and set it to non-active state. 

   if (fgFitDialog) {
      fgFitDialog->UnmapWindow();
   }
   if (fParentPad) {
      fParentPad->Disconnect("RangeAxisChanged()");
      DoReset();
   }
   fCanvas = 0;
   fParentPad = 0;
   fFitObject = 0;
   gROOT->GetListOfCleanups()->Remove(this);
}

//______________________________________________________________________________
void TFitEditor::Show(TVirtualPad* pad, TObject *obj)
{
   // Show the fit panel (possible only via context menu).

   if (!gROOT->GetListOfCleanups()->FindObject(this))
      gROOT->GetListOfCleanups()->Add(this);   

   if (!fgFitDialog->IsMapped()) {
      fgFitDialog->MapWindow();
      gVirtualX->RaiseWindow(GetId());
   }
   SetCanvas(pad->GetCanvas());
   fCanvas->Selected(pad, obj, kButton1Down);
}

//______________________________________________________________________________
void TFitEditor::CloseWindow()
{
   // Close fit panel window.

   Hide();
}

//______________________________________________________________________________
// TFitEditor *&TFitEditor::GetFP()
// {
//    // Static: return main fit panel
//    return fgFitDialog;
// }

//______________________________________________________________________________
void TFitEditor::Terminate()
{
   //  Called to delete the fit panel. 

   TQObject::Disconnect("TCanvas", "Closed()");
   delete fgFitDialog;
   fgFitDialog = 0;
}

//______________________________________________________________________________
void TFitEditor::UpdateGUI()
{
   //  Set the fit panel GUI according to the selected object. 

   if (!fFitObject) return;

   fPx1old = fParentPad->XtoAbsPixel(fParentPad->GetUxmin());
   fPy1old = fParentPad->YtoAbsPixel(fParentPad->GetUymin());
   fPx2old = fParentPad->XtoAbsPixel(fParentPad->GetUxmax());
   fPy2old = fParentPad->YtoAbsPixel(fParentPad->GetUymax());

   // sliders
   if (fDim > 0) {
      fSliderX->Disconnect("PositionChanged()");

      TH1* hist = 0;
      switch (fType) {
         case kObjectHisto:
            hist = (TH1*)fFitObject;
            break;

         case kObjectGraph:
            hist = ((TGraph*)fFitObject)->GetHistogram();
            break;

         case kObjectMultiGraph:
            hist = ((TMultiGraph*)fFitObject)->GetHistogram();
            break;

         case kObjectGraph2D:
            hist = ((TGraph2D*)fFitObject)->GetHistogram("empty");
            break;

         case kObjectHStack: 
            hist = (TH1 *)((THStack *)fFitObject)->GetHists()->First();

         case kObjectTree:
            //not implemented
            break;
      }
      
      Int_t ixmin = 0; 
      Int_t ixmax = 0; 
      Int_t iymin = 0; 
      Int_t iymax = 0; 
      Int_t izmin = 0; 
      Int_t izmax = 0; 
      if (hist) {
         fXaxis = hist->GetXaxis();
         fYaxis = hist->GetYaxis();
         fZaxis = hist->GetZaxis();
         fXrange = fXaxis->GetNbins();
         ixmin = fXaxis->GetFirst();
         ixmax = fXaxis->GetLast();
         iymin = fYaxis->GetFirst();
         iymax = fYaxis->GetLast();
         izmin = fZaxis->GetFirst();
         izmax = fZaxis->GetLast();
      }

      fSliderX->SetRange(1,fXrange);
      if (ixmin == 0 && ixmax == 0)
         fSliderX->SetPosition(1,fXrange);
      else 
         fSliderX->SetPosition(ixmin,ixmax);
      fSliderX->SetScale(5);
      fSliderX->Connect("PositionChanged()","TFitEditor",this, "DoSliderXMoved()");
   }

//  no implemented functionality for y & z sliders yet 
   if (fDim > 1) {
      fSliderY->Disconnect("PositionChanged()");

      if (!fSliderYParent->IsMapped())
         fSliderYParent->MapWindow();
      if (fSliderZParent->IsMapped())
         fSliderZParent->UnmapWindow();

      switch (fType) {
         case kObjectHisto: 
         case kObjectGraph2D:
         case kObjectHStack: 
            fYrange = fYaxis->GetNbins();
            fYmin = fYaxis->GetFirst();
            fYmax = fYaxis->GetLast();
            break;
         
         case kObjectGraph: 
         case kObjectMultiGraph: 
         case kObjectTree:  
            //not implemented
            break;
      }
      fSliderY->SetRange(1,fYrange);
      fSliderY->SetPosition(fYmin,fYmax);
      fSliderY->SetScale(5);
      fSliderY->Connect("PositionChanged()","TFitEditor",this, "DoSliderYMoved()");
   }

   
   if (fDim > 2) {
      fSliderZ->Disconnect("PositionChanged()");

      if (!fSliderZParent->IsMapped())
         fSliderZParent->MapWindow();

      switch (fType) {
         case kObjectHStack:
         case kObjectHisto:
            fZrange = fZaxis->GetNbins();
            fZmin = fZaxis->GetFirst();
            fZmax = fZaxis->GetLast();
            break;

         case kObjectGraph:
         case kObjectGraph2D:
         case kObjectMultiGraph:
         case kObjectTree:
            //not implemented
            break;
      }
      fSliderZ->SetRange(1,fZrange);
      fSliderZ->SetPosition(fZmin,fZmax);
      fSliderZ->SetScale(5);
      fSliderZ->Connect("PositionChanged()","TFitEditor",this, "DoSliderZMoved()");
   }
/*
   switch (fDim) {
      case 1:
         fGeneral->HideFrame(fSliderYParent);
         fGeneral->HideFrame(fSliderZParent);
         break;
      case 2:
         fGeneral->HideFrame(fSliderZParent);
         break;
   }
   Layout();*/
}

//______________________________________________________________________________
void TFitEditor::SetFitObject(TVirtualPad *pad, TObject *obj, Int_t event)
{
   // Slot called when the user clicks on an object inside a canvas. 
   // Updates pointers to the parent pad and the selected object
   // for fitting (if suitable).

   if (event != kButton1Down) return;

   if (!pad || !obj) {
      DoNoSelection();
      return;
   }
   
   // is obj suitable for fitting?
   if (!SetObjectType(obj)) return;

   fParentPad = pad;
   fFitObject = obj;
   ShowObjectName(obj);
   UpdateGUI();

   ConnectSlots();
   
   TF1* fitFunc = HasFitFunction(obj);

   if (fitFunc) {
      //fFuncPars = FuncParams_t( fitFunc->GetNpar() );
      GetParameters(fFuncPars, fitFunc);

      // get function range
      fitFunc->GetRange(fFuncXmin, fFuncYmin, fFuncZmin, fFuncXmax,  fFuncYmax,  fFuncZmax);


      TString tmpStr = fitFunc->GetExpFormula();
      TGLBEntry *en = 0;
      if ( tmpStr.Length() == 0 )
      {
         fEnteredFunc->SetText(fitFunc->GetName());
         en= fFuncList->FindEntry(fitFunc->GetName());
         SetEditable(kFALSE);
      }
      else
      {
         fEnteredFunc->SetText(fitFunc->GetExpFormula().Data());
         en= fFuncList->FindEntry(fitFunc->GetExpFormula().Data());
         SetEditable(kTRUE);
      }
      if (en) fFuncList->Select(en->EntryId());
   } else {
      TGTextLBEntry *te = (TGTextLBEntry *)fFuncList->GetSelectedEntry();
      if (te && fNone->GetState() == kButtonDown)
         fEnteredFunc->SetText(te->GetTitle());
      else if (fAdd->GetState() == kButtonDown) {
         TString tmpStr = fEnteredFunc->GetText();
         tmpStr += '+';
         tmpStr +=te->GetTitle();
         fEnteredFunc->SetText(tmpStr);
      } else if ( !te )
         fEnteredFunc->SetText("");
      fEnteredFunc->SelectAll();
   }

   // Update the information about the selected object.
   if (fSetParam->GetState() == kButtonDisabled)
      fSetParam->SetEnabled(kTRUE);
   if (fFitButton->GetState() == kButtonDisabled)
      fFitButton->SetEnabled(kTRUE);
   if (fResetButton->GetState() == kButtonDisabled)
      fResetButton->SetEnabled(kTRUE);
   DoLinearFit();
}

//______________________________________________________________________________
void TFitEditor::DoNoSelection()
{
   // Slot called when users close a TCanvas. 

   if (gROOT->GetListOfCanvases()->IsEmpty()) {
      Terminate();
      return;
   }
   
   DisconnectSlots();
   fParentPad = 0;
   fFitObject = 0;
   fObjLabel->SetText("No object selected");
   fObjLabelParent->Resize(GetDefaultSize());
   Layout();

   fSetParam->SetEnabled(kFALSE);
   fFitButton->SetEnabled(kFALSE);
   fResetButton->SetEnabled(kFALSE);
}

//______________________________________________________________________________
void TFitEditor::RecursiveRemove(TObject* obj)
{
   // When obj is deleted, clear fFitObject if fFitObject = obj.

   if (obj == fFitObject) {
      fFitObject = 0;
      DisconnectSlots();
      fObjLabel->SetText("No object selected");
      fObjLabelParent->Resize(GetDefaultSize());
      Layout();

      fFitButton->SetEnabled(kFALSE);
      fResetButton->SetEnabled(kFALSE);
      fSetParam->SetEnabled(kFALSE);
      
      TQObject::Connect("TCanvas", "Selected(TVirtualPad *, TObject *, Int_t)", 
                        "TFitEditor",this, 
                        "SetFitObject(TVirtualPad *, TObject *, Int_t)");
      TQObject::Connect("TCanvas", "Closed()", "TFitEditor", this, 
                        "DoNoSelection()");
      return;
   }
   if (obj == fParentPad) {
      fFitObject = 0;
      fParentPad = 0;
      DisconnectSlots();
      fObjLabel->SetText("No object selected");
      fObjLabelParent->Resize(GetDefaultSize());
      Layout();

      fFitButton->SetEnabled(kFALSE);
      fResetButton->SetEnabled(kFALSE);
      fSetParam->SetEnabled(kFALSE);
   }
}

//______________________________________________________________________________
TGComboBox* TFitEditor::BuildFunctionList(TGFrame* parent, Int_t id)
{
   // Create function list combo box.

   TGComboBox *c = new TGComboBox(parent, id);

   c->AddEntry("gaus" ,  kFP_GAUS);
   c->AddEntry("gausn",  kFP_GAUSN);
   c->AddEntry("expo",   kFP_EXPO);
   c->AddEntry("landau", kFP_LAND);
   c->AddEntry("landaun",kFP_LANDN);
   c->AddEntry("pol0",   kFP_POL0);
   c->AddEntry("pol1",   kFP_POL1);
   c->AddEntry("pol2",   kFP_POL2);
   c->AddEntry("pol3",   kFP_POL3);
   c->AddEntry("pol4",   kFP_POL4);
   c->AddEntry("pol5",   kFP_POL5);
   c->AddEntry("pol6",   kFP_POL6);
   c->AddEntry("pol7",   kFP_POL7);
   c->AddEntry("pol8",   kFP_POL8);
   c->AddEntry("pol9",   kFP_POL9);
   c->AddEntry("user",   kFP_USER);

   return c;
}

//______________________________________________________________________________
TGComboBox* TFitEditor::BuildMethodList(TGFrame* parent, Int_t id)
{
   // Create method list in a combo box.

   TGComboBox *c = new TGComboBox(parent, id);
   c->AddEntry("Chi-square", kFP_MCHIS);
   c->AddEntry("Binned Likelihood", kFP_MBINL);
   //c->AddEntry("Unbinned Likelihood", kFP_MUBIN); //for later use
   //c->AddEntry("User", kFP_MUSER);                //for later use
   c->Select(kFP_MCHIS);
   return c;
}

//______________________________________________________________________________
void TFitEditor::DoAdvancedOptions()
{
   // Slot connected to advanced option button (opens a dialog).

   new TGMsgBox(fClient->GetRoot(), GetMainFrame(),
                "Info", "Advanced option dialog is not implemented yet",
                kMBIconAsterisk,kMBOk, 0);
}

//______________________________________________________________________________
void TFitEditor::DoEmptyBinsAllWeights1()
{
   // Slot connected to 'include emtry bins and forse all weights to 1' setting.

   if (fEmptyBinsWghts1->GetState() == kButtonDown)
      if (fAllWeights1->GetState() == kButtonDown)
         fAllWeights1->SetState(kButtonUp, kTRUE);
}

//______________________________________________________________________________
void TFitEditor::DoAllWeights1()
{
   // Slot connected to 'set all weights to 1' setting.

   if (fAllWeights1->GetState() == kButtonDown)
      if (fEmptyBinsWghts1->GetState() == kButtonDown)
         fEmptyBinsWghts1->SetState(kButtonUp, kTRUE);
}

//______________________________________________________________________________
void TFitEditor::DoClose()
{
   // Close the fit panel.

   Hide();
}

//______________________________________________________________________________
void TFitEditor::DoFit()
{
   // Perform a fit with current parameters' settings.

   if (!fFitObject) return;
   if (!fParentPad) return;

   if ( fNone->GetState() != kButtonDisabled && CheckFunctionString(fEnteredFunc->GetText()) )
   {
      new TGMsgBox(fClient->GetRoot(), GetMainFrame(),
                   "Error...", "DoFit\nVerify the entered function string!",
                   kMBIconStop,kMBOk, 0);
      return;
   }
   
   fFitButton->SetState(kButtonEngaged);
   if (gPad) gPad->GetVirtCanvas()->SetCursor(kWatch);
   gVirtualX->SetCursor(GetId(), gVirtualX->CreateCursor(kWatch));

   fParentPad->Disconnect("RangeAxisChanged()");
   TVirtualPad *save = 0;
   save = gPad;
   gPad = fParentPad;
   fParentPad->cd();

   fParentPad->GetCanvas()->SetCursor(kWatch);

   // Option Retrieval!
   ROOT::Math::MinimizerOptions mopts;
   Foption_t fitOpts;
   TString strDrawOpts;


   TF1 *fitFunc = 0;
   if ( fNone->GetState() == kButtonDisabled )
   {
      TGTextLBEntry *te = (TGTextLBEntry *)fFuncList->GetSelectedEntry();
      TF1* tmpF1 = (TF1*) gROOT->GetListOfFunctions()->FindObject(te->GetTitle());
      if ( tmpF1 == 0 )
      {
               new TGMsgBox(fClient->GetRoot(), GetMainFrame(),
                            "Error...", "DoFit\nVerify the entered function string!",
                            kMBIconStop,kMBOk, 0);
               return;
      }
      fitFunc = (TF1*)tmpF1->IsA()->New();
      tmpF1->Copy(*fitFunc);
   }

   ROOT::Fit::DataRange drange; 

   Int_t ixmin = (Int_t)(fSliderX->GetMinPosition()); 
   Int_t ixmax = (Int_t)(fSliderX->GetMaxPosition()); 
   if (ixmin > 1 || ixmax < fXaxis->GetNbins() ) { 
      Double_t xmin = fXaxis->GetBinLowEdge(ixmin);
      Double_t xmax = fXaxis->GetBinUpEdge(ixmax);
      drange.AddRange(0,xmin, xmax);
   }

   if ( fDim > 1 ) {
      assert(fYaxis); 
      Int_t iymin = (Int_t)(fSliderY->GetMinPosition()); 
      Int_t iymax = (Int_t)(fSliderY->GetMaxPosition()); 
      if (iymin > 1 || iymax < fYaxis->GetNbins() ) { 
         Double_t ymin = fYaxis->GetBinLowEdge(iymin);
         Double_t ymax = fYaxis->GetBinUpEdge(iymax);
         drange.AddRange(1,ymin, ymax);
      }
   }
   if ( fDim > 2 ) {
      assert(fZaxis); 
      Int_t izmin = (Int_t)(fSliderZ->GetMinPosition()); 
      Int_t izmax = (Int_t)(fSliderZ->GetMaxPosition()); 
      if (izmin > 1 || izmax < fZaxis->GetNbins() ) { 
         Double_t zmin = fZaxis->GetBinLowEdge(izmin);
         Double_t zmax = fZaxis->GetBinUpEdge(izmax);
         drange.AddRange(2,zmin, zmax);
      }
   }

   if ( fitFunc == 0 )
   {
      // create function with saved range values 
      if ( fDim == 1 )
         fitFunc = new TF1("lastFitFunc",fEnteredFunc->GetText(),fFuncXmin,fFuncXmax);
      else if ( fDim == 2 )
         fitFunc = new TF2("lastFitFunc",fEnteredFunc->GetText(),fFuncXmin,fFuncXmax, fFuncYmin, fFuncYmax);
      else if ( fDim == 3 )
         fitFunc =  new TF3("lastFitFunc",fEnteredFunc->GetText(),fFuncXmin,fFuncXmax, fFuncYmin, fFuncYmax, fFuncZmin, fFuncZmax);
   }
   
   //if ( fFuncPars ) 
   // set parameters from panel in function
   SetParameters(fFuncPars, fitFunc);
   RetrieveOptions(fitOpts, strDrawOpts, mopts, fitFunc->GetNpar());


   switch (fType) {
      case kObjectHisto: {

         TH1 *hist = (TH1*)fFitObject;
         ROOT::Fit::FitObject(hist, fitFunc, fitOpts, mopts, strDrawOpts, drange);

         break;
      }
      case kObjectGraph: {
         TGraph *gr = (TGraph*)fFitObject;
         // not sure if this is needed 
//         TH1F *hist = gr->GetHistogram();
//          if (hist) { //!!! for many graphs in a pad, use the xmin/xmax of pad!!!
//             Int_t npoints = gr->GetN();
//             Double_t *gx = gr->GetX();
//             Double_t gxmin, gxmax;
//             const Int_t imin =  TMath::LocMin(npoints,gx);
//             const Int_t imax =  TMath::LocMax(npoints,gx);
//             gxmin = gx[imin];
//             gxmax = gx[imax];
//             if (xmin < gxmin) xmin = gxmin;
//             if (xmax > gxmax) xmax = gxmax;
//          }

         FitObject(gr, fitFunc, fitOpts, mopts, strDrawOpts, drange);
         break;
      }
      case kObjectMultiGraph: {
         TMultiGraph *mg = (TMultiGraph*)fFitObject;
         FitObject(mg, fitFunc, fitOpts, mopts, strDrawOpts, drange);

         break;
      }
      case kObjectGraph2D: {

         TGraph2D *mg = (TGraph2D*)fFitObject;
         FitObject(mg, fitFunc, fitOpts, mopts, strDrawOpts, drange);

         break;
      }
      case kObjectHStack: {
         // N/A
         break;
      }
      case kObjectTree:  {
         // N/A
         break;
      }
   }

   // update parameters value shown in dialog 
   //if (!fFuncPars) fFuncPars = new Double_t[fitFunc->GetNpar()][3];
   GetParameters(fFuncPars,fitFunc);

   delete fitFunc;


   fParentPad->Modified();
   fParentPad->Update();
   fParentPad->GetCanvas()->SetCursor(kPointer);
   fParentPad->Connect("RangeAxisChanged()", "TFitEditor", this, "UpdateGUI()");
   
   if (save) gPad = save;
   if (fSetParam->GetState() == kButtonDisabled && 
       fLinearFit->GetState() == kButtonUp)
      fSetParam->SetState(kButtonUp);

   if (gPad) gPad->GetVirtCanvas()->SetCursor(kPointer);
   gVirtualX->SetCursor(GetId(), gVirtualX->CreateCursor(kPointer));
   fFitButton->SetState(kButtonUp);
}

//______________________________________________________________________________
Int_t TFitEditor::CheckFunctionString(const char *fname)
{
   // Check entered function string.
   Int_t rvalue = 0;
   if ( fDim == 1 ) {
      TF1 form("tmpCheck", fname);
      rvalue = form.Compile();
   } else if ( fDim == 2 ) {
      TF2 form("tmpCheck", fname);
      rvalue = form.Compile();
   }

   return rvalue;
}

//______________________________________________________________________________
void TFitEditor::DoAddition(Bool_t on)
{
   // Slot connected to addition of predefined functions.

   static Bool_t first = kFALSE;
   TString s = fEnteredFunc->GetText();
   if (on) {
      if (!first) {
         s += "(0)";
         fSelLabel->SetText(fEnteredFunc->GetText());
         fEnteredFunc->SetText(s.Data());
         first = kTRUE;
         ((TGCompositeFrame *)fSelLabel->GetParent())->Layout();
      }
   } else {
      first = kFALSE;
   }
}

//______________________________________________________________________________
void TFitEditor::DoFunction(Int_t selected)
{
   // Slot connected to predefined fit function settings.

   TGTextLBEntry *te = (TGTextLBEntry *)fFuncList->GetSelectedEntry();
   if (fNone->GetState() == kButtonDown || fNone->GetState() == kButtonDisabled) {
      TF1* tmpTF1 = 0;
      switch (fType) {
      case kObjectHisto: {
         tmpTF1 = (TF1*) gROOT->GetListOfFunctions()->FindObject(te->GetTitle());
         break;
      }
      case kObjectGraph: {
         tmpTF1 = (TF1*) gROOT->GetListOfFunctions()->FindObject(te->GetTitle());
         break;
      }
      default: { break; }
      }

      if ( tmpTF1 && strcmp(tmpTF1->GetExpFormula(), "") ) 
      {
         SetEditable(kTRUE);
         fEnteredFunc->SetText(tmpTF1->GetExpFormula());
      }
      else
      {
         if ( selected <= kFP_USER )
            SetEditable(kTRUE);
         else       
            SetEditable(kFALSE);
         fEnteredFunc->SetText(te->GetTitle());
      }
   } else if (fAdd->GetState() == kButtonDown) {
      Int_t np = 0;
      TString s = "";
      if (!strcmp(fEnteredFunc->GetText(), "")) {
         fEnteredFunc->SetText(te->GetTitle());
      } else {
         s = fEnteredFunc->GetTitle();
         TFormula tmp("tmp", fEnteredFunc->GetText());
         np = tmp.GetNpar();
      }
      if (np)
         s += Form("+%s(%d)", te->GetTitle(), np);
      else
         s += Form("%s(%d)", te->GetTitle(), np);
      fEnteredFunc->SetText(s.Data());
   }
   TString tmpStr = fEnteredFunc->GetText();

   // create TF1 with the passed string. Delete previous one if existing
   if (tmpStr.Contains("pol") || tmpStr.Contains("++")) {
      fLinearFit->SetState(kButtonDown, kTRUE);
   } else {
      fLinearFit->SetState(kButtonUp, kTRUE);
   }

   fEnteredFunc->SelectAll();
   fSelLabel->SetText(fEnteredFunc->GetText());
   ((TGCompositeFrame *)fSelLabel->GetParent())->Layout();

   // reset function parameters 
   fFuncPars.clear();
}

//______________________________________________________________________________
void TFitEditor::DoEnteredFunction()
{
   // Slot connected to entered function in text entry.

   if (!strcmp(fEnteredFunc->GetText(), "")) return;
   
   Int_t ok = CheckFunctionString(fEnteredFunc->GetText());

   if (ok != 0) {
      new TGMsgBox(fClient->GetRoot(), GetMainFrame(),
                   "Error...", "DoEnteredFunction\nVerify the entered function string!",
                   kMBIconStop,kMBOk, 0);
      return;
   }

   fSelLabel->SetText(fEnteredFunc->GetText());
   ((TGCompositeFrame *)fSelLabel->GetParent())->Layout();
}

//______________________________________________________________________________
void TFitEditor::DoLinearFit()
{
   // Slot connected to linear fit settings.

   if (fLinearFit->GetState() == kButtonDown) {
      //fSetParam->SetState(kButtonDisabled);
      fBestErrors->SetState(kButtonDisabled);
      fImproveResults->SetState(kButtonDisabled);
      fRobustValue->SetState(kTRUE);
   } else {
      //fSetParam->SetState(kButtonUp);
      fBestErrors->SetState(kButtonUp);
      fImproveResults->SetState(kButtonUp);
      fRobustValue->SetState(kFALSE);
   }
}

//______________________________________________________________________________
void TFitEditor::DoNoChi2()
{
   // Slot connected to 'no chi2' option settings.

   if (fLinearFit->GetState() == kButtonUp)
      fLinearFit->SetState(kButtonDown, kTRUE);
}

//______________________________________________________________________________
void TFitEditor::DoNoStoreDrawing()
{
   // Slot connected to 'no storing, no drawing' settings.
   if (fNoDrawing->GetState() == kButtonUp)
      fNoDrawing->SetState(kButtonDown);
}

//______________________________________________________________________________
void TFitEditor::DoPrintOpt(Bool_t on)
{
   // Slot connected to print option settings.

   TGButton *btn = (TGButton *) gTQSender;
   Int_t id = btn->WidgetId();
   switch (id) {
      case kFP_PDEF:
         if (on) {
            fOptDefault->SetState(kButtonDown);
            fOptVerbose->SetState(kButtonUp);
            fOptQuiet->SetState(kButtonUp);
         }
         fStatusBar->SetText("Prn: DEF",3);
         break;
      case kFP_PVER:
         if (on) {
            fOptVerbose->SetState(kButtonDown);
            fOptDefault->SetState(kButtonUp);
            fOptQuiet->SetState(kButtonUp);
         }
         fStatusBar->SetText("Prn: VER",3);
         break;
      case kFP_PQET:
         if (on) {
            fOptQuiet->SetState(kButtonDown);
            fOptDefault->SetState(kButtonUp);
            fOptVerbose->SetState(kButtonUp);
         }
         fStatusBar->SetText("Prn: QT",3);
      default:
         break;
   }
}

//______________________________________________________________________________
void TFitEditor::DoReset()
{
   // Reset all fit parameters.

   fParentPad->Modified();
   fParentPad->Update();
   fEnteredFunc->SetText("gaus");

   if (fXmin > 1 || fXmax < fXrange) {
      fSliderX->SetRange(fXmin,fXmax);
      fSliderX->SetPosition(fXmin, fXmax);
   } else {
      fSliderX->SetRange(1,fXrange);
      fSliderX->SetPosition(fXmin,fXmax);
   }

   if (fYmin > 1 || fYmax < fYrange) {
      fSliderY->SetRange(fYmin,fYmax);
      fSliderY->SetPosition(fYmin, fYmax);
   } else {
      fSliderY->SetRange(1,fYrange);
      fSliderY->SetPosition(fYmin,fYmax);
   }

   fPx1old = fParentPad->XtoAbsPixel(fParentPad->GetUxmin());
   fPy1old = fParentPad->YtoAbsPixel(fParentPad->GetUymin());
   fPx2old = fParentPad->XtoAbsPixel(fParentPad->GetUxmax());
   fPy2old = fParentPad->YtoAbsPixel(fParentPad->GetUymax());

   if (fLinearFit->GetState() == kButtonDown)
      fLinearFit->SetState(kButtonUp, kTRUE);
   if (fBestErrors->GetState() == kButtonDown)
      fBestErrors->SetState(kButtonUp, kFALSE);
   if (fUseRange->GetState() == kButtonDown)
      fUseRange->SetState(kButtonUp, kFALSE);
   if (fAllWeights1->GetState() == kButtonDown)
      fAllWeights1->SetState(kButtonUp, kFALSE);
   if (fEmptyBinsWghts1->GetState() == kButtonDown)
      fEmptyBinsWghts1->SetState(kButtonUp, kFALSE);
   if (fImproveResults->GetState() == kButtonDown)
      fImproveResults->SetState(kButtonUp, kFALSE);
   if (fAdd2FuncList->GetState() == kButtonDown)
      fAdd2FuncList->SetState(kButtonUp, kFALSE);
   if (fUseGradient->GetState() == kButtonDown)
      fUseGradient->SetState(kButtonUp, kFALSE);
   if (fNoChi2->GetState() == kButtonDown)
      fNoChi2->SetState(kButtonUp, kFALSE);
   if (fDrawSame->GetState() == kButtonDown)
      fDrawSame->SetState(kButtonUp, kFALSE);
   if (fNoDrawing->GetState() == kButtonDown)
      fNoDrawing->SetState(kButtonUp, kFALSE);
   if (fNoStoreDrawing->GetState() == kButtonDown)
      fNoStoreDrawing->SetState(kButtonUp, kFALSE);
   fNone->SetState(kButtonDown, kTRUE);
   fFuncList->Select(1, kTRUE);

   // minimization tab
   if (fLibMinuit->GetState() != kButtonDown)
      fLibMinuit->SetState(kButtonDown, kTRUE);
   if (fMigrad->GetState() != kButtonDown)
      fMigrad->SetState(kButtonDown, kTRUE);
   if (fOptDefault->GetState() != kButtonDown)
      fOptDefault->SetState(kButtonDown, kTRUE);
   if (fErrorScale->GetNumber() != ROOT::Math::MinimizerOptions::DefaultErrorDef()) {
      fErrorScale->SetNumber(ROOT::Math::MinimizerOptions::DefaultErrorDef());
      fErrorScale->ReturnPressed();
   }   
   if (fTolerance->GetNumber() != ROOT::Math::MinimizerOptions::DefaultTolerance()) {
      fTolerance->SetNumber(ROOT::Math::MinimizerOptions::DefaultTolerance());
      fTolerance->ReturnPressed();
   }
   if (fIterations->GetNumber() != ROOT::Math::MinimizerOptions::DefaultMaxIterations()) {
      fIterations->SetIntNumber(ROOT::Math::MinimizerOptions::DefaultMaxIterations());
      fIterations->ReturnPressed();
   }
}

//______________________________________________________________________________
void TFitEditor::DoSetParameters()
{
   // Open set parameters dialog.

   TF1 * fitFunc = 0;
   if ( fNone->GetState() == kButtonDisabled )
   {
      TGTextLBEntry *te = (TGTextLBEntry *)fFuncList->GetSelectedEntry();
      //std::cout << te << "  " << te->GetTitle() << std::endl;
      fitFunc = (TF1*) gROOT->GetListOfFunctions()->FindObject(te->GetTitle());
   }
   else if ( fDim == 1 )
      fitFunc = new TF1("tmpPars",fEnteredFunc->GetText() );
   else if ( fDim == 2 )
      fitFunc = new TF2("tmpPars",fEnteredFunc->GetText() );
   else if ( fDim == 3 )
      fitFunc = new TF3("tmpPars",fEnteredFunc->GetText() );

   if (!fitFunc) { Error("DoSetParameters","NUll function"); return; }

   // case of special functions (gaus, expo, etc...)
   if (fFuncPars.size() == 0) { 
      switch (fType) {
      case kObjectHisto:
         InitParameters( fitFunc, (TH1*)fFitObject) ;
         break;
      case kObjectGraph:
         InitParameters( fitFunc, ((TGraph*)fFitObject));
         break;
      case kObjectMultiGraph:
         InitParameters( fitFunc, ((TMultiGraph*)fFitObject));
         break;
      case kObjectGraph2D:
         InitParameters( fitFunc, ((TGraph2D*)fFitObject));
         break;
      case kObjectHStack: 
         // N/A
         break;
      case kObjectTree:  
         // N/A
         break;
      }

      GetParameters(fFuncPars, fitFunc);
   }                            
   else {
      SetParameters(fFuncPars, fitFunc);
   }

   fParentPad->Disconnect("RangeAxisChanged()");

//    Double_t xmin, xma> x;
//    fitFunc->GetRange(xmin, xmax);
   Int_t ret = 0;
   new TFitParametersDialog(gClient->GetDefaultRoot(), GetMainFrame(), 
                            fitFunc, fParentPad, 0, 0, &ret);

   //if ( fFuncPars ) delete fFuncPars;
   //fFuncPars = new Double_t[fitFunc->GetNpar()][3];
   GetParameters(fFuncPars, fitFunc);

   fParentPad->Connect("RangeAxisChanged()", "TFitEditor", this, "UpdateGUI()");

   if ( fNone->GetState() != kButtonDisabled ) delete fitFunc;
}

//______________________________________________________________________________
void TFitEditor::DoSliderXMoved()
{
   // Slot connected to range settings on x-axis.

   Int_t px1,py1,px2,py2;

   TVirtualPad *save = 0;
   save = gPad;
   gPad = fParentPad;
   gPad->cd();

   Double_t xleft = 0;
   Double_t xright = 0;
   xleft  = fXaxis->GetBinLowEdge((Int_t)((fSliderX->GetMinPosition())+0.5));
   xright = fXaxis->GetBinUpEdge((Int_t)((fSliderX->GetMaxPosition())+0.5));

   Float_t ymin, ymax;
   if ( fDim > 1 )
   {
      ymin = fYaxis->GetBinLowEdge((Int_t)((fSliderY->GetMinPosition())+0.5));//gPad->GetUymin();
      ymax = fYaxis->GetBinUpEdge((Int_t)((fSliderY->GetMaxPosition())+0.5));//gPad->GetUymax();
   }
   else 
   {
      ymin = gPad->GetUymin();
      ymax = gPad->GetUymax();
   }

   px1 = gPad->XtoAbsPixel(xleft);
   py1 = gPad->YtoAbsPixel(ymin);
   px2 = gPad->XtoAbsPixel(xright);
   py2 = gPad->YtoAbsPixel(ymax);

   gPad->GetCanvas()->FeedbackMode(kTRUE);
   gPad->SetLineWidth(1);
   gPad->SetLineColor(2);
   
   gVirtualX->DrawBox(fPx1old, fPy1old, fPx2old, fPy2old, TVirtualX::kHollow);
   gVirtualX->DrawBox(px1, py1, px2, py2, TVirtualX::kHollow);

   fPx1old = px1;
   fPy1old = py1;
   fPx2old = px2 ;
   fPy2old = py2;

   if(save) gPad = save;
}

//______________________________________________________________________________
void TFitEditor::DoSliderYMoved()
{
   // Slot connected to range settings on y-axis.

   Int_t px1,py1,px2,py2;

   TVirtualPad *save = 0;
   save = gPad;
   gPad = fParentPad;
   gPad->cd();

   Float_t ybottom = 0;
   Float_t ytop = 0;
   ybottom = fYaxis->GetBinLowEdge((Int_t)((fSliderY->GetMinPosition())+0.5));
   ytop = fYaxis->GetBinUpEdge((Int_t)((fSliderY->GetMaxPosition())+0.5));

   Float_t xmin = fXaxis->GetBinLowEdge((Int_t)((fSliderX->GetMinPosition())+0.5));//fParentPad->GetUxmin();
   Float_t xmax = fXaxis->GetBinUpEdge((Int_t)((fSliderX->GetMaxPosition())+0.5));//fParentPad->GetUxmax();

   px1 = fParentPad->XtoAbsPixel(xmin);
   py1 = fParentPad->YtoAbsPixel(ybottom);
   px2 = fParentPad->XtoAbsPixel(xmax);
   py2 = fParentPad->YtoAbsPixel(ytop);

   gPad->GetCanvas()->FeedbackMode(kTRUE);
   gPad->SetLineWidth(1);
   gPad->SetLineColor(2);

   gVirtualX->DrawBox(fPx1old, fPy1old, fPx2old, fPy2old, TVirtualX::kHollow);
   gVirtualX->DrawBox(px1, py1, px2, py2, TVirtualX::kHollow);

   fPx1old = px1;
   fPy1old = py1;
   fPx2old = px2 ;
   fPy2old = py2;

   if(save) gPad = save;
}

//______________________________________________________________________________
void TFitEditor::DoSliderZMoved()
{
   // Slot connected to range settings on z-axis.

}

//______________________________________________________________________________
void TFitEditor::DoUserDialog()
{
   // Open a dialog for getting a user defined method.

   new TGMsgBox(fClient->GetRoot(), GetMainFrame(),
                "Info", "Dialog of user method is not implemented yet",
                kMBIconAsterisk,kMBOk, 0);
}

//______________________________________________________________________________
void TFitEditor::SetFunction(const char *function)
{
   // Set the function to be used in performed fit.

   fEnteredFunc->SetText(function);
}

//______________________________________________________________________________
Bool_t TFitEditor::SetObjectType(TObject* obj)
{
   // Check whether the object suitable for fitting and set 
   // its type, dimension and method combo box accordingly.
   
   Bool_t set = kFALSE;

   if (obj->InheritsFrom("TGraph")) {
      fType = kObjectGraph;
      set = kTRUE;
      fDim = 1;
      if (fMethodList->FindEntry("Binned Likelihood"))
         fMethodList->RemoveEntry(kFP_MBINL);
      if (!fMethodList->FindEntry("Chi-square"))
         fMethodList->AddEntry("Chi-square", kFP_MCHIS);
      fMethodList->Select(kFP_MCHIS, kFALSE);
      fRobustValue->SetState(kTRUE);
      fRobustValue->GetNumberEntry()->SetToolTipText("Set robust value");
   } else if (obj->InheritsFrom("TGraph2D")) {
      fType = kObjectGraph2D;
      set = kTRUE;
      fDim = 2;
      if (fMethodList->FindEntry("Unbinned Likelihood"))
         fMethodList->RemoveEntry(kFP_MUBIN);
      if (!fMethodList->FindEntry("Chi-square"))
         fMethodList->AddEntry("Chi-square", kFP_MCHIS);
      fMethodList->Select(kFP_MCHIS, kFALSE);
   } else if (obj->InheritsFrom("THStack")) {
      fType = kObjectHStack;
      set = kTRUE;
      TH1 *hist = (TH1 *)((THStack *)obj)->GetHists()->First();
      fDim = hist->GetDimension();
      if (fMethodList->FindEntry("Unbinned Likelihood"))
         fMethodList->RemoveEntry(kFP_MUBIN);
      if (!fMethodList->FindEntry("Chi-square"))
         fMethodList->AddEntry("Chi-square", kFP_MCHIS);
      fMethodList->Select(kFP_MCHIS, kFALSE);
   } else if (obj->InheritsFrom("TTree")) {
      fType = kObjectTree;
      set = kTRUE;
      fDim = -1; //not implemented
      fMethodList->SetEnabled(kFALSE);
   } else if (obj->InheritsFrom("TH1")){
      fType = kObjectHisto;
      set = kTRUE;
      fDim = ((TH1*)obj)->GetDimension();
      if (!fMethodList->FindEntry("Binned Likelihood"))
         fMethodList->AddEntry("Binned Likelihood", kFP_MBINL);
      if (!fMethodList->FindEntry("Chi-square"))
         fMethodList->AddEntry("Chi-square", kFP_MCHIS);
      fMethodList->Select(kFP_MCHIS, kFALSE);
   } else if (obj->InheritsFrom("TMultiGraph")) {
      fType = kObjectMultiGraph;
      set = kTRUE;
      fDim = 1;
      if (fMethodList->FindEntry("Binned Likelihood"))
         fMethodList->RemoveEntry(kFP_MBINL);
      if (!fMethodList->FindEntry("Chi-square"))
         fMethodList->AddEntry("Chi-square", kFP_MCHIS);
      fMethodList->Select(kFP_MCHIS, kFALSE);
      fRobustValue->SetState(kTRUE);
      fRobustValue->GetNumberEntry()->SetToolTipText("Set robust value");
   }

   if ( fDim < 2 )
      fGeneral->HideFrame(fSliderYParent);
   else
      fGeneral->ShowFrame(fSliderYParent);

   return set;
}

//______________________________________________________________________________
void TFitEditor::ShowObjectName(TObject* obj)
{
   // Show object name on the top.

   TString name;
   
   if (obj) {
      name = obj->GetName();
      name.Append("::");
      name.Append(obj->ClassName());
   } else {
      name = "No object selected";
   }
   fObjLabel->SetText(name.Data());
   fObjLabelParent->Resize(GetDefaultSize());
   Layout();
}

//______________________________________________________________________________
Option_t *TFitEditor::GetDrawOption() const
{
   // Get draw options of the selected object.

   if (!fParentPad) return "";

   TListIter next(fParentPad->GetListOfPrimitives());
   TObject *obj;
   while ((obj = next())) {
      if (obj == fFitObject) return next.GetOption();
   }
   return "";
}

//______________________________________________________________________________
void TFitEditor::DoLibrary(Bool_t on)
{
   // Set selected minimization library in use.

   TGButton *bt = (TGButton *)gTQSender;
   Int_t id = bt->WidgetId(); 

   switch (id) {

      case kFP_LMIN:
         {
            if (on) {
               fLibMinuit->SetState(kButtonDown);
               fLibMinuit2->SetState(kButtonUp);
               fLibFumili->SetState(kButtonUp);
               if (fFumili->GetState() != kButtonDisabled) {
                  fFumili->SetDisabledAndSelected(kFALSE);
               }
               fMigrad->SetState(kButtonDown);
               fStatusBar->SetText("MIGRAD", 1);
               fSimplex->SetState(kButtonUp);
               fCombination->SetState(kButtonUp);
               fScan->SetState(kButtonUp);
               fStatusBar->SetText("LIB Minuit", 0);
            }
            
         }
         break;
      
      case kFP_LMIN2:
         {
            if (on) {
               fLibMinuit->SetState(kButtonUp);
               fLibMinuit2->SetState(kButtonDown);
               fLibFumili->SetState(kButtonUp);
               if (fSimplex->GetState() == kButtonDisabled)
                  fSimplex->SetState(kButtonUp);
               if (fMigrad->GetState() == kButtonDisabled)
                  fMigrad->SetState(kButtonUp);
               if (fFumili->GetState() == kButtonDisabled)
                  fFumili->SetState(kButtonUp);
               if (fCombination->GetState() == kButtonDisabled)
                  fCombination->SetState(kButtonUp);
               if (fScan->GetState() == kButtonDisabled)
                  fScan->SetState(kButtonUp);
               fStatusBar->SetText("LIB Minuit2", 0);
            }
         }
         break;
      
      case kFP_LFUM:
         {
            if (on) {
               fLibMinuit->SetState(kButtonUp);
               fLibMinuit2->SetState(kButtonUp);
               fLibFumili->SetState(kButtonDown);

               if (fFumili->GetState() != kButtonDown) {
                  fFumili->SetState(kButtonDown);
                  fStatusBar->SetText("FUMILI", 1);
               }
               fMigrad->SetDisabledAndSelected(kFALSE);
               fSimplex->SetState(kButtonDisabled);
               fCombination->SetState(kButtonDisabled);
               fScan->SetState(kButtonDisabled);
               fStatusBar->SetText("LIB Fumili", 0);
            }
         }
      default:
         break;
   }
}

//______________________________________________________________________________
void TFitEditor::DoMinMethod(Bool_t on)
{
   // Set selected minimization method in use.

   TGButton *bt = (TGButton *)gTQSender;
   Int_t id = bt->WidgetId(); 

   switch (id) {

   case kFP_MIGRAD:
   {
      if (on) {
         fSimplex->SetState(kButtonUp);
         fCombination->SetState(kButtonUp);
         fScan->SetState(kButtonUp);
         if (fLibMinuit->GetState() == kButtonDown)
            fFumili->SetState(kButtonDisabled);
         else
            fFumili->SetState(kButtonUp);
         fMigrad->SetState(kButtonDown);
         fStatusBar->SetText("MIGRAD",1);
      }
   }
   break;
   
   case kFP_SIMPLX:
   {
      if (on) {
         fMigrad->SetState(kButtonUp);
         fCombination->SetState(kButtonUp);
         fScan->SetState(kButtonUp);
         if (fLibMinuit->GetState() == kButtonDown)
            fFumili->SetState(kButtonDisabled);
         else
            fFumili->SetState(kButtonUp);
         fSimplex->SetState(kButtonDown);
         fStatusBar->SetText("SIMPLEX",1);
      }
   }
   break;
   
   case kFP_COMBINATION:
   {
      if (on) {
         fMigrad->SetState(kButtonUp);
         fSimplex->SetState(kButtonUp);
         fScan->SetState(kButtonUp);
         if (fLibMinuit->GetState() == kButtonDown)
            fFumili->SetState(kButtonDisabled);
         else
            fFumili->SetState(kButtonUp);    
         fCombination->SetState(kButtonDown);
         fStatusBar->SetText("Combination",1);
      }
   }
   break;

   case kFP_SCAN:
   {
      if (on) {
         fMigrad->SetState(kButtonUp);
         fSimplex->SetState(kButtonUp);
         fCombination->SetState(kButtonUp);
         if (fLibMinuit->GetState() == kButtonDown)
            fFumili->SetState(kButtonDisabled);
         else
            fFumili->SetState(kButtonUp);    
         fScan->SetState(kButtonDown);
         fStatusBar->SetText("SCAN",1);
      }
   }
   break;

   case kFP_FUMILI:
   {
      if (on) {
         fMigrad->SetState(kButtonUp);
         fSimplex->SetState(kButtonUp);
         fCombination->SetState(kButtonUp);
         fScan->SetState(kButtonUp);
         fFumili->SetState(kButtonDown);
         fStatusBar->SetText("FUMILI",1);
      }
   }
   break;
   
   }
}

//______________________________________________________________________________
void TFitEditor::DoMaxIterations()
{
   // Set the maximum number of iterations.

   Long_t itr = fIterations->GetIntNumber();
   fStatusBar->SetText(Form("Itr: %ld",itr),2);
}

//______________________________________________________________________________
void TFitEditor::MakeTitle(TGCompositeFrame *parent, const char *title)
{
   // Create section title in the GUI.

   TGCompositeFrame *ht = new TGCompositeFrame(parent, 350, 10, 
                                               kFixedWidth | kHorizontalFrame);
   ht->AddFrame(new TGLabel(ht, title),
                new TGLayoutHints(kLHintsLeft, 1, 1, 0, 0));
   ht->AddFrame(new TGHorizontal3DLine(ht),
                new TGLayoutHints(kLHintsExpandX | kLHintsCenterY, 5, 5, 2, 2));
   parent->AddFrame(ht, new TGLayoutHints(kLHintsTop, 5, 0, 5, 0));
}

//______________________________________________________________________________
TF1* TFitEditor::HasFitFunction(TObject *obj)
{
   // Look in the list of function for TF1. If a TF1 is
   // found in the list of functions, returns kTRUE; if not returns kFALSE.
   
   TF1 *func = 0;
   TList *lf = 0;

   switch (fType) {
      case kObjectHisto: {
         func =((TH1 *)obj)->GetFunction("fitFunc");
         lf = ((TH1 *)obj)->GetListOfFunctions();
         break;
      }
      case kObjectGraph: {
         func =((TGraph *)obj)->GetFunction("fitFunc");
         lf = ((TGraph *)obj)->GetListOfFunctions();
         break;
      }
      case kObjectMultiGraph: {
         func =((TMultiGraph *)obj)->GetFunction("fitFunc");
         lf = ((TMultiGraph *)obj)->GetListOfFunctions();
         break;
      }
      case kObjectGraph2D: {
         func =(TF1 *)((TGraph2D *)obj)->GetListOfFunctions()->FindObject("fitFunc");
         lf = ((TGraph2D *)obj)->GetListOfFunctions();
         break;
      }
      case kObjectHStack: {
         // N/A
         break;
      }
      case kObjectTree:  {
         // N/A
         break;
      }
   }
   if (func) {
      if (lf) GetFunctionsFromList(lf);
      CheckRange(func);
      
      TGLBEntry *le = fFuncList->FindEntry(Form(func->GetName()));
      if (le) {
         fFuncList->Select(le->EntryId(), kFALSE);
         fSelLabel->SetText(le->GetTitle());
      }
      return func;
   } 
   if (lf) {
      GetFunctionsFromList(lf);
      TObject *obj2;
      TIter next(lf, kIterBackward);
      while ((obj2 = next())) {
         if (obj2->InheritsFrom(TF1::Class())) {
            func = (TF1 *)obj2;
            CheckRange(func);
            //LM:  does not work - why ??????
            //std::cout << "from histo function list  func " << func << "  " << func->GetName() << std::endl; 
            TGLBEntry *le = fFuncList->FindEntry(func->GetName());
            if (le) {
               fFuncList->Select(le->EntryId(), kTRUE);
//               std::cout << func->GetName() << "  " << le->EntryId() << " is selected " << std::endl;
            }
            return func;
         }
      }
   }
   return 0;
}

//______________________________________________________________________________
void TFitEditor::GetFunctionsFromList(TList *list)
{
   // Scan the list of functions of the selected object and include them 
   // by name in the combobox holding the predefined functions.
   
   TObject *obj;
   Int_t newid = kFP_USER*30;

   static TObject *Object = 0; 
   if ( Object != fFitObject )
   {
      fFuncList->RemoveAll();
      if ( fDim == 1 ) // Put back the predefined functions for 1D
         // objects.
      {
         fFuncList->AddEntry("gaus" ,  kFP_GAUS);
         fFuncList->AddEntry("gausn",  kFP_GAUSN);
         fFuncList->AddEntry("expo",   kFP_EXPO);
         fFuncList->AddEntry("landau", kFP_LAND);
         fFuncList->AddEntry("landaun",kFP_LANDN);
         fFuncList->AddEntry("pol0",   kFP_POL0);
         fFuncList->AddEntry("pol1",   kFP_POL1);
         fFuncList->AddEntry("pol2",   kFP_POL2);
         fFuncList->AddEntry("pol3",   kFP_POL3);
         fFuncList->AddEntry("pol4",   kFP_POL4);
         fFuncList->AddEntry("pol5",   kFP_POL5);
         fFuncList->AddEntry("pol6",   kFP_POL6);
         fFuncList->AddEntry("pol7",   kFP_POL7);
         fFuncList->AddEntry("pol8",   kFP_POL8);
         fFuncList->AddEntry("pol9",   kFP_POL9);
         fFuncList->AddEntry("user",   kFP_USER);

         // Need to be setted this way, otherwise when the functions
         // are removed, the list doesn't show them.x
         TGListBox *lb = fFuncList->GetListBox();
         lb->Resize(lb->GetWidth(), 200);
      }

      TIter next(list, kIterForward);
      while ((obj = next()))
         if (obj->InheritsFrom(TF1::Class())) {
            fFuncList->AddEntry(obj->GetName(), newid++);
         }


      fFuncList->Select((newid != kFP_USER*30)?newid-1:1);
      Object = fFitObject;
   }
}

//______________________________________________________________________________
void TFitEditor::CheckRange(TF1 *f1)
{
   // Check the fit function range (if the object has been fitted).
   Double_t fxmin=0, fxmax=0, xmin=0, xmax=0;

   f1->GetRange(fxmin, fxmax);
   xmin = fXaxis->GetXmin();
   xmax = fXaxis->GetXmax();
   fXrange = fXaxis->GetNbins();

   if (fFitObject->InheritsFrom(TGraph::Class())) {
      TGraph *gr = (TGraph *)fFitObject;
      Int_t npoints = gr->GetN();
      Double_t *gx = gr->GetX();
      Double_t gxmin, gxmax;
      const Int_t imin =  TMath::LocMin(npoints,gx);
      const Int_t imax =  TMath::LocMax(npoints,gx);
      gxmin = gx[imin];
      gxmax = gx[imax];
      if (xmin < gxmin) xmin = gxmin;
      if (xmax > gxmax) xmax = gxmax;
      if (xmin > xmax) {
         Double_t tmp = gxmin;
         xmin = xmax;
         xmax = tmp;
      }
   }
   if ((fxmin > xmin) || (fxmax < xmax)) {
      fXmin = fXaxis->FindBin(fxmin);
      fXmax = fXaxis->FindBin(fxmax);
      fUseRange->SetState(kButtonDown);
   } else {
      fXmin = fXaxis->FindBin(xmin);
      fXmax = fXaxis->FindBin(xmax);
      fUseRange->SetState(kButtonUp);
   }

   if (fDim > 0) {
      fSliderX->Disconnect("PositionChanged()");
      fSliderX->SetRange(1,fXrange);
      if (!fXmin && !fXmax)
         fSliderX->SetPosition(1,fXrange);
      else 
         fSliderX->SetPosition(fXmin,fXmax);
      fSliderX->SetScale(5);
      fSliderX->Connect("PositionChanged()","TFitEditor",this, "DoSliderXMoved()");
   }
}

//______________________________________________________________________________
void TFitEditor::RetrieveOptions(Foption_t& fitOpts, TString& drawOpts, ROOT::Math::MinimizerOptions& minOpts, Int_t npar)
{
   drawOpts = "";

   fitOpts.Range    = (fUseRange->GetState() == kButtonDown);
   fitOpts.Integral = (fIntegral->GetState() == kButtonDown);
   fitOpts.More     = (fImproveResults->GetState() == kButtonDown);
   fitOpts.Errors   = (fBestErrors->GetState() == kButtonDown);
   fitOpts.Like = (fMethodList->GetSelected() != kFP_MCHIS);

   if (fEmptyBinsWghts1->GetState() == kButtonDown)
      fitOpts.W1 = 2;
   else if (fAllWeights1->GetState() == kButtonDown)
      fitOpts.W1 = 1;

   TString tmpStr = fEnteredFunc->GetText();
   if ( !(fLinearFit->GetState() == kButtonDown) &&
        (tmpStr.Contains("pol") || tmpStr.Contains("++")) )
      fitOpts.Minuit = 1;

   if ( (int) fFuncPars.size() == npar )
      for ( Int_t i = 0; i < npar; ++i )
         if ( fFuncPars[i][PAR_MIN] != fFuncPars[i][PAR_MAX] )
         {
            fitOpts.Bound = 1;
            break;
         }
   
   fitOpts.Nochisq  = (fNoChi2->GetState() == kButtonDown);
   fitOpts.Nostore  = (fNoStoreDrawing->GetState() == kButtonDown);
   fitOpts.Nograph  = (fNoDrawing->GetState() == kButtonDown);
   fitOpts.Plus     = (fAdd2FuncList->GetState() == kButtonDown);
   fitOpts.Gradient = (fUseGradient->GetState() == kButtonDown);
   fitOpts.Quiet    = ( fOptQuiet->GetState() == kButtonDown );
   fitOpts.Verbose  = ( fOptVerbose->GetState() == kButtonDown );

   if ( !(fType != kObjectGraph) && (fLinearFit->GetState() == kButtonDown) )
   {
      fitOpts.Robust = 1;
      fitOpts.hRobust = fRobustValue->GetNumber();
   }

   if (fDrawSame->GetState() == kButtonDown)
      drawOpts += "SAME";

   drawOpts = GetDrawOption();

   if ( fLibMinuit->GetState() == kButtonDown )
      minOpts.SetMinimizerType ( "Minuit");
   else if ( fLibMinuit2->GetState() == kButtonDown)
      minOpts.SetMinimizerType ( "Minuit2" );
   else if ( fLibFumili->GetState() == kButtonDown )
      minOpts.SetMinimizerType ("Fumili" );

   if ( fMigrad->GetState() == kButtonDown )
      minOpts.SetMinimizerAlgorithm( "Migrad" );
   else if ( fFumili->GetState() == kButtonDown )
      if ( fLibMinuit2->GetState() == kButtonDown )
         minOpts.SetMinimizerAlgorithm( "Fumili2" );
      else 
         minOpts.SetMinimizerAlgorithm( "Fumili" );
   else if ( fSimplex->GetState() == kButtonDown )
      minOpts.SetMinimizerAlgorithm( "Simplex" );
   else if ( fScan->GetState() == kButtonDown )
      minOpts.SetMinimizerAlgorithm( "Scan" );
   else if ( fCombination->GetState() == kButtonDown )
      minOpts.SetMinimizerAlgorithm( "Minimize" );

   minOpts.SetErrorDef ( fErrorScale->GetNumber() );
   minOpts.SetTolerance( fTolerance->GetNumber() );
   minOpts.SetMaxIterations(fIterations->GetIntNumber());
   minOpts.SetMaxFunctionCalls(fIterations->GetIntNumber());
}

void TFitEditor::SetEditable(Bool_t state)
{
   if ( state )
   {
      fEnteredFunc->SetState(kTRUE);
         fAdd->SetState(kButtonUp, kFALSE);
         fNone->SetState(kButtonDown, kFALSE);
   } else {
      fEnteredFunc->SetState(kFALSE);
      fAdd->SetState(kButtonDisabled, kFALSE);
      fNone->SetState(kButtonDisabled, kFALSE);
   }
}
