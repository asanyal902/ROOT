// @(#)root/gl:$Id$
// Author:  Timur Pocheptsov  14/06/2006
                                                                                
/*************************************************************************
 * Copyright (C) 1995-2004, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#ifndef ROOT_TGLLegoPainter
#define ROOT_TGLLegoPainter

#include <utility>
#include <vector>

#ifndef ROOT_TGLPlotPainter
#include "TGLPlotPainter.h"
#endif

#ifndef ROOT_TGLQuadric
#include "TGLQuadric.h"
#endif

#ifndef ROOT_TArcBall
#include "TArcBall.h"
#endif

#ifndef ROOT_TGLUtil
#include "TGLUtil.h"
#endif

#ifndef ROOT_TPoint
#include "TPoint.h"
#endif

class TGLAxisPainter;

/*
   TGLLegoPainter. The concrete implementation of abstract TGLPlotPainter.
*/

class TGLLegoPainter : public TGLPlotPainter, public TGLPlotFrame {
private:
   enum ELegoType {
      kColorSimple,
      kColorLevel,
      kCylindricBars
   };

   enum ESelectionType {
      kSelectionSimple,//Histogramm can be selected as a whole object.
      kSelectionFull  //All parts are selectable.
   };

   typedef std::pair<Int_t, Int_t>       Selection_t;
   typedef std::pair<Double_t, Double_t> CosSin_t;

   TH1                  *fHist;
   Int_t                 fGLContext;
   EGLCoordType          fCoordType;

   BinRange_t            fBinsX;
   BinRange_t            fBinsY;
   Range_t               fRangeX;
   Range_t               fRangeY;
   Range_t               fRangeZ;
   Double_t              fMinZ;
   //Bars, cylinders or textured bars.
   ELegoType             fLegoType;

   TGLSelectionBuffer    fSelection;
   Bool_t                fSelectionPass;
   Bool_t                fUpdateSelection;
   Selection_t           fSelectedBin;
   ESelectionType        fSelectionMode;
   Int_t                 fSelectedPlane;   

   TColor               *fPadColor;
   TColor               *fFrameColor;

   Double_t              fXOZProfilePos;
   Double_t              fYOZProfilePos;
   Bool_t                fIsMoving;

   std::vector<Range_t>  fXEdges;
   std::vector<Range_t>  fYEdges;
   std::vector<CosSin_t> fCosSinTableX;
   std::vector<CosSin_t> fCosSinTableY;

   TString               fBinInfo;
   TGLAxisPainter       *fAxisPainter;

   std::vector<Double_t> fZLevels;

   //Temporary stuff here, must be in TGLTexture1D
   UInt_t                fTextureName;
   std::vector<UChar_t>  fTexture;
   TGLQuadric            fQuadric;

   Bool_t                fDrawErrors;

   void         Enable1DTexture();
   void         Disable1DTexture();

   TGLLegoPainter(const TGLLegoPainter &);
   TGLLegoPainter &operator = (const TGLLegoPainter &);

public:
   TGLLegoPainter(TH1 *hist, TGLAxisPainter *axisPainter, Int_t ctx = -1, EGLCoordType type = kGLCartesian, 
                  Bool_t logX = kFALSE, Bool_t logY = kFALSE, Bool_t logZ = kFALSE);

   //TGLPlotPainter's final-verriders
   void         Paint();
   void         SetGLContext(Int_t ctx);
   char        *GetObjectInfo(Int_t px, Int_t py);
   Bool_t       InitGeometry();
   void         StartRotation(Int_t px, Int_t py);
   void         StopRotation();
   void         Rotate(Int_t px, Int_t py);
   void         StartPan(Int_t px, Int_t py);
   void         Pan(Int_t px, Int_t py);
   void         StopPan();
   TObject     *Select(Int_t px, Int_t py);
   void         ZoomIn();
   void         ZoomOut();
   void         SetLogX(Bool_t logX);
   void         SetLogY(Bool_t logY);
   void         SetLogZ(Bool_t logZ);
   void         SetCoordType(EGLCoordType type);
   void         AddOption(const TString &stringOption);
   void         SetPadColor(TColor *padColor);
   void         SetFrameColor(TColor *frameColor);
   void         ProcessEvent(Int_t event, Int_t px, Int_t py);

private:
   //Auxilary functions.
   Bool_t       InitGeometryCartesian();
   Bool_t       InitGeometryPolar();
   Bool_t       InitGeometryCylindrical();
   Bool_t       InitGeometrySpherical();

   void         InitGL();
   Selection_t  ColorToObject(const UChar_t *color);
   void         EncodeToColor(Int_t i, Int_t j)const;
   void         DrawPlot();
   void         DrawLegoCartesian();
   void         DrawLegoPolar();
   void         DrawLegoCylindrical();
   void         DrawLegoSpherical();

   void         SetLegoColor();
   void         ClearBuffers();
   Bool_t       MakeGLContextCurrent()const;

   void         SetSelectionMode();

   void         DrawFrame();
   void         DrawBackPlane(Int_t plane)const;
   void         DrawGrid(Int_t plane)const;
   void         MoveDynamicProfile(Int_t px, Int_t py);
   void         DrawProfiles();
   void         DrawProfileX();
   void         DrawProfileY();
   Bool_t       ClampZ(Double_t &zVal)const;

   static const Float_t    fRedEmission[];
   static const Float_t    fOrangeEmission[];
   static const Float_t    fGreenEmission[];
   static const Float_t    fNullEmission[];

   ClassDef(TGLLegoPainter, 0)//Lego painter
};

#endif
