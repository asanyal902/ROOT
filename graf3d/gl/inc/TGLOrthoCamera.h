// @(#)root/gl:$Id$
// Author:  Richard Maunder  25/05/2005

/*************************************************************************
 * Copyright (C) 1995-2004, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#ifndef ROOT_TGLOrthoCamera
#define ROOT_TGLOrthoCamera

#ifndef ROOT_TGLCamera
#include "TGLCamera.h"
#endif

#ifndef ROOT_TArcBall
#include "TArcBall.h"
#endif

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// TGLOrthoCamera                                                       //
//                                                                      //
// Orthographic projection camera. Currently limited to three types     //
// defined at construction time - kXOY, kXOZ, kZOY - where this refers  //
// to the viewport plane axis - e.g. kXOY has X axis horizontal, Y      //
// vertical - i.e. looking down Z axis with Y vertical.                 //
//
// The plane types restriction could easily be removed to supported     //
// arbitary ortho projections along any axis/orientation with free      //
// rotations about them.                                                //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

class TGLPaintDevice;

class TGLOrthoCamera : public TGLCamera {
public:
   enum EType { kZOY,  kXOZ,  kXOY,    // Pair of world axes aligned to h/v screen.
                kZnOY, kXnOZ, kXnOY }; // 'n' means preceeding axis is negated.
private:
   // Fields
   EType          fType;         //! camera type
   Bool_t         fEnableRotate; //! enable rotation
   Bool_t         fDollyToZoom;  //! zoom when dolly is requested

   // Limits - set in Setup()
   Double_t       fZoomMin;      //! minimum zoom factor
   Double_t       fZoomDefault;  //! default zoom factor
   Double_t       fZoomMax;      //! maximum zoom factor
   TGLBoundingBox fVolume;       //! scene volume

   // Current interaction
   Double_t       fDefXSize, fDefYSize; //! x, y size of scene from camera view
   Double_t       fZoom;                //! current zoom

   //Stuff for TGLPlotPainter. MT: This *must* go to a special subclass.
   Double_t       fShift;
   Double_t       fOrthoBox[4];
   TGLVertex3     fCenter;
   TGLVector3     fTruck;
   TArcBall       fArcBall;
   TPoint         fMousePos;
   Bool_t         fVpChanged;

   // Methods
   void Init();

   static   UInt_t   fgZoomDeltaSens;
public:
   TGLOrthoCamera();
   TGLOrthoCamera(EType type, const TGLVector3 & hAxis, const TGLVector3 & vAxis);
   virtual ~TGLOrthoCamera();

   virtual Bool_t IsOrthographic() const { return kTRUE; }

   virtual void   Setup(const TGLBoundingBox & box, Bool_t reset=kTRUE);
   virtual void   Reset();

   virtual Bool_t Dolly(Int_t delta, Bool_t mod1, Bool_t mod2);
   virtual Bool_t Zoom (Int_t delta, Bool_t mod1, Bool_t mod2);
   virtual Bool_t Truck(Int_t xDelta, Int_t yDelta, Bool_t mod1, Bool_t mod2);
   virtual Bool_t Rotate(Int_t xDelta, Int_t yDelta, Bool_t mod1, Bool_t mod2);
   virtual void   Apply(const TGLBoundingBox & sceneBox, const TGLRect * pickRect = 0) const;

   virtual void   Markup (TGLCameraMarkupStyle* ms) const;

   // External scripting control
   //   void Configure(Double_t left, Double_t right, Double_t top, Double_t bottom);
   virtual void Configure(Double_t zoom, Double_t dolly, Double_t center[3],
                          Double_t hRotate, Double_t vRotate);

   void     SetEnableRotate(Bool_t x) { fEnableRotate = x; }
   Bool_t   GetEnableRotate()   const { return fEnableRotate; }

   Double_t GetZoomMin() const { return fZoomMin; }
   Double_t GetZoomMax() const { return fZoomMax; }
   void     SetZoomMin(Double_t z);
   void     SetZoomMax(Double_t z);
   void     SetZoomMinMax(Double_t min, Double_t max) { SetZoomMin(min); SetZoomMax(max); }

   void     SetDollyToZoom(Bool_t x) { fDollyToZoom = x; }
   Bool_t   GetDollyToZoom()   const { return fDollyToZoom; }

   // Stuff for TGLPlotPainter.
   void   SetViewport(TGLPaintDevice *dev);
   void   SetViewVolume(const TGLVertex3 *box);
   void   StartRotation(Int_t px, Int_t py);
   void   RotateCamera(Int_t px, Int_t py);
   void   StartPan(Int_t px, Int_t py);
   void   Pan(Int_t px, Int_t py);
   void   ZoomIn();
   void   ZoomOut();
   void   SetCamera()const;
   void   Apply(Double_t phi, Double_t theta)const;
   Bool_t ViewportChanged()const{return fVpChanged;}
   Int_t  GetX()const;
   Int_t  GetY()const;
   Int_t  GetWidth()const;
   Int_t  GetHeight()const;

   ClassDef(TGLOrthoCamera,0) // Camera for orthographic view.
};

#endif // ROOT_TGLOrthoCamera
