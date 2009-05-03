#include <stdexcept>
#include <iostream>
#include <fstream>

#include "TAttMarker.h"
#include "TVirtualX.h"
#include "TPad.h"

#include "TGLPadPainter.h"
#include "TGLIncludes.h"
#include "TCanvas.h"
#include "TGLUtil.h"
#include "TError.h"

ClassImp(TGLPadPainter)

//______________________________________________________________________________
TGLPadPainter::TGLPadPainter(TCanvas *cnv)
                  : fCanvas(cnv),
                    fX(0.), fY(0.), fW(0.), fH(0.)
{
   if (!fCanvas) {
      Error("TGLPadPainter::TGLPadPainter", "Null canvas pointer was psecified\n");
      throw std::runtime_error("");
   }
   //So, now I can use fCanvas without checks.
}

/*
"Delegating" part of TGLPadPainter. Line/fill/etc. attributes can be
set inside TPad, but not onle where: 
many of them are set by base sub-objects of 2d primitives
(2d primitives usually inherit TAttLine or TAttFill etc.).  And these sub-objects
call gVirtualX->SetLineWidth ... etc. So, if I save some attributes in my painter,
it will be mess - at any moment I do not know, where to take line attribute - from
gVirtualX or from my own member. So! All attributed, _ALL_ go to/from gVirtualX.
*/

//______________________________________________________________________________
Color_t TGLPadPainter::GetLineColor() const
{
   //Delegate to gVirtualX.
   return gVirtualX->GetLineColor();
}

//______________________________________________________________________________
Style_t TGLPadPainter::GetLineStyle() const
{
   //Delegate to gVirtualX.
   return gVirtualX->GetLineStyle();
}

//______________________________________________________________________________
Width_t TGLPadPainter::GetLineWidth() const
{
   //Delegate to gVirtualX.
   return gVirtualX->GetLineWidth();
}

//______________________________________________________________________________
void TGLPadPainter::SetLineColor(Color_t lcolor)
{
   //Delegate to gVirtualX.
   gVirtualX->SetLineColor(lcolor);
}

//______________________________________________________________________________
void TGLPadPainter::SetLineStyle(Style_t lstyle)
{
   //Delegate to gVirtualX.
   gVirtualX->SetLineStyle(lstyle);
}

//______________________________________________________________________________
void TGLPadPainter::SetLineWidth(Width_t lwidth)
{
   //Delegate to gVirtualX.
   gVirtualX->SetLineWidth(lwidth);
}

//______________________________________________________________________________
Color_t TGLPadPainter::GetFillColor() const
{
   //Delegate to gVirtualX.
   return gVirtualX->GetFillColor();
}

//______________________________________________________________________________
Style_t TGLPadPainter::GetFillStyle() const
{
   //Delegate to gVirtualX.
   return gVirtualX->GetFillStyle();
}

//______________________________________________________________________________
Bool_t TGLPadPainter::IsTransparent() const
{
   //Delegate to gVirtualX.
   //IsTransparent is implemented as inline function in TAttFill.
   return gVirtualX->IsTransparent();
}

//______________________________________________________________________________
void TGLPadPainter::SetFillColor(Color_t fcolor)
{
   //Delegate to gVirtualX.
   gVirtualX->SetFillColor(fcolor);
}

//______________________________________________________________________________
void TGLPadPainter::SetFillStyle(Style_t fstyle)
{
   //Delegate to gVirtualX.
   gVirtualX->SetFillStyle(fstyle);
}

//______________________________________________________________________________
void TGLPadPainter::SetOpacity(Int_t percent)
{
   //Delegate to gVirtualX.
   gVirtualX->SetOpacity(percent);
}

//______________________________________________________________________________
Short_t TGLPadPainter::GetTextAlign() const
{
   //Delegate to gVirtualX.
   return gVirtualX->GetTextAlign();
}

//______________________________________________________________________________
Float_t TGLPadPainter::GetTextAngle() const
{
   //Delegate to gVirtualX.
   return gVirtualX->GetTextAngle();
}

//______________________________________________________________________________
Color_t TGLPadPainter::GetTextColor() const
{
   //Delegate to gVirtualX.
   return gVirtualX->GetTextColor();
}

//______________________________________________________________________________
Font_t TGLPadPainter::GetTextFont() const
{
   //Delegate to gVirtualX.
   return gVirtualX->GetTextFont();
}

//______________________________________________________________________________
Float_t TGLPadPainter::GetTextSize() const
{
   //Delegate to gVirtualX.
   return gVirtualX->GetTextSize();
}

//______________________________________________________________________________
Float_t TGLPadPainter::GetTextMagnitude() const
{
   //Delegate to gVirtualX.
   return gVirtualX->GetTextMagnitude();
}

//______________________________________________________________________________
void TGLPadPainter::SetTextAlign(Short_t align)
{
   //Delegate to gVirtualX.
   gVirtualX->SetTextAlign(align);
}

//______________________________________________________________________________
void TGLPadPainter::SetTextAngle(Float_t tangle)
{
   //Delegate to gVirtualX.
   gVirtualX->SetTextAngle(tangle);
}

//______________________________________________________________________________
void TGLPadPainter::SetTextColor(Color_t tcolor)
{
   //Delegate to gVirtualX.
   gVirtualX->SetTextColor(tcolor);
}

//______________________________________________________________________________
void TGLPadPainter::SetTextFont(Font_t tfont)
{
   //Delegate to gVirtualX.
   gVirtualX->SetTextFont(tfont);
}

//______________________________________________________________________________
void TGLPadPainter::SetTextSize(Float_t tsize)
{
   //Delegate to gVirtualX.
   gVirtualX->SetTextSize(tsize);
}

//______________________________________________________________________________
void TGLPadPainter::SetTextSizePixels(Int_t npixels)
{
   //Delegate to gVirtualX.
   gVirtualX->SetTextSizePixels(npixels);
}

/*
"Pixmap" part of TGLPadPainter.
In principle, it's bad, that painter creates something like
"painting device", it should only paint on painting device.
But this all comes from gVirtualX and TPad/TCanvas design.
It's better to have painting devices here, than re-write
everything in a correct way.
*/

//______________________________________________________________________________
Int_t TGLPadPainter::CreateDrawable(UInt_t/*w*/, UInt_t/*h*/)
{
   return 0;
}

//______________________________________________________________________________
void TGLPadPainter::ClearDrawable()
{

}

//______________________________________________________________________________
void TGLPadPainter::CopyDrawable(Int_t /*id*/, Int_t /*px*/, Int_t /*py*/)
{
}

//______________________________________________________________________________
void TGLPadPainter::DestroyDrawable()
{

}

//______________________________________________________________________________
void TGLPadPainter::SelectDrawable(Int_t /*device*/)
{
   if (TPad *pad = dynamic_cast<TPad *>(gPad)) {
      Int_t px = 0, py = 0;
      
      pad->XYtoAbsPixel(pad->GetX1(), pad->GetY1(), px, py);
      
      py = fCanvas->GetWh() - py;
      //
      glViewport(px, py, GLsizei(fCanvas->GetWw() * pad->GetAbsWNDC()), GLsizei(fCanvas->GetWh() * pad->GetAbsHNDC()));
      
      glMatrixMode(GL_PROJECTION);
      glLoadIdentity();
      glOrtho(pad->GetX1(), pad->GetX2(), pad->GetY1(), pad->GetY2(), -10., 10.);
      
      glMatrixMode(GL_MODELVIEW);
      glLoadIdentity();
      glTranslated(0., 0., -1.);
   } else {
      Error("TGLPadPainter::SelectDrawable", "function was called not from TPad or TCanvas code\n");
      throw std::runtime_error("");
   }
}

//______________________________________________________________________________
void TGLPadPainter::InitPainter()
{
   //Init gl pad painter:
   //2D painter does not use depth test
   //and should not modify depth-buffer content.
   glDisable(GL_DEPTH_TEST);
   glDisable(GL_CULL_FACE);
   glDisable(GL_LIGHTING);
   
   //Clear the buffer
   fW = fCanvas->GetWw();
   fH = fCanvas->GetWh();
   
   glViewport(0, 0, GLsizei(fW), GLsizei(fH));
   
   glDepthMask(GL_TRUE);
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
   glDepthMask(GL_FALSE);
   
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   
   glOrtho(fCanvas->GetX1(), fCanvas->GetX2(), fCanvas->GetY1(), fCanvas->GetY2(), -10., 10.);

   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();
   glTranslated(0., 0., -1.);
   /*glTranslated(0, -fW / 4, 0.);
   glRotated(15., 0., 0., 1.);*/
}

/*
Now, the most interesting part of TGLPadPainter: painting.
*/

//______________________________________________________________________________
void TGLPadPainter::DrawLine(Double_t x1, Double_t y1, Double_t x2, Double_t y2)
{
   //xs is ok (with fX addition). ys must be converted from windows coordinates into gl.
   const Rgl::Pad::LineAttribSet lineAttribs(kTRUE, gVirtualX->GetLineStyle(), fLimits.GetMaxLineWidth(), kTRUE);

   glBegin(GL_LINES);
   glVertex2d(x1, y1);
   glVertex2d(x2, y2);
   glEnd();
}

//______________________________________________________________________________
void TGLPadPainter::DrawBox(Double_t x1, Double_t y1, Double_t x2, Double_t y2, EBoxMode mode)
{
   if (mode == kHollow) {
      const Rgl::Pad::LineAttribSet lineAttribs(kTRUE, 0, fLimits.GetMaxLineWidth(), kTRUE);
      //
      glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
      glRectd(x1, y1, x2, y2);
      glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
      glLineWidth(1.f);
   } else {
      const Rgl::Pad::FillAttribSet fillAttribs(fSSet, kFALSE);//Set filling parameters.
      glRectd(x1, y1, x2, y2);
   }
//   std::cout<<"Drawing the box "<<gPad->GetX1()<<' '<<gPad->GetY1()<<' '<<gPad->GetX2()<<' '<<gPad->GetY2()<<std::endl;
}

//______________________________________________________________________________
void TGLPadPainter::DrawFillArea(UInt_t n, const Double_t *x, const Double_t *y)
{
   if (!gVirtualX->GetFillStyle())
      return DrawPolyLine(n, x, y);

   fVs.resize(n * 3);
   
   for (UInt_t i = 0; i < n; ++i) {
      fVs[i * 3]     = x[i];
      fVs[i * 3 + 1] = y[i];
      fVs[i * 3 + 2] = 0.;
   }

   const Rgl::Pad::FillAttribSet fillAttribs(fSSet, kFALSE);

   GLUtesselator *t = (GLUtesselator *)fTess.GetTess();
   gluBeginPolygon(t);
   gluNextContour(t, (GLenum)GLU_UNKNOWN);

   for (UInt_t i = 0; i < n; ++i)
      gluTessVertex(t, &fVs[i * 3], &fVs[i * 3]);

      
   gluEndPolygon(t);
   
   std::ofstream out("ellipse.txt");
   out<<gPad->GetX1()<<' '<<gPad->GetY1()<<' '<<gPad->GetX2()<<' '<<gPad->GetY2()<<std::endl;
   for(UInt_t i = 0; i < n; ++i)
      out<<x[i]<<' '<<y[i]<<std::endl;
}

//______________________________________________________________________________
void TGLPadPainter::DrawPolyLine(UInt_t n, const Double_t *x, const Double_t *y)
{
   //xs is ok (with fX addition). ys must be converted from windows coordinates into gl.
   const Rgl::Pad::LineAttribSet lineAttribs(kTRUE, gVirtualX->GetLineStyle(), fLimits.GetMaxLineWidth(), kTRUE);

   glBegin(GL_LINE_STRIP);

   for (UInt_t i = 0; i < n; ++i)
      glVertex2d(x[i], y[i]);
      
   if (!gVirtualX->GetFillStyle())
      glVertex2d(x[0], y[0]);

   glEnd();
}

//______________________________________________________________________________
void TGLPadPainter::DrawPolyMarker(UInt_t n, const Double_t *x, const Double_t *y)
{
   SaveProjectionMatrix();
   glLoadIdentity();
   //
   glOrtho(0, gPad->GetAbsWNDC() * fCanvas->GetWw(), 0, gPad->GetAbsHNDC() * fCanvas->GetWh(), -10., 10.);
   //
   glMatrixMode(GL_MODELVIEW);
   //
   Float_t rgba[3] = {};
   Rgl::Pad::ExtractRGB(gVirtualX->GetMarkerColor(), rgba);
   glColor3fv(rgba);

   ConvertMarkerPoints(n, x, y);
   
   const TPoint *xy = &fPoly[0];
   const Style_t markerStyle = gVirtualX->GetMarkerStyle();
   switch (markerStyle) {
   case kDot:
      fMarker.DrawDot(n, xy);
      break;
   case kPlus:
      fMarker.DrawPlus(n, xy);
      break;
   case kStar:
      fMarker.DrawStar(n, xy);
      break;
   case kCircle:
   case kOpenCircle:
      fMarker.DrawCircle(n, xy);
      break;
   case kMultiply:
      fMarker.DrawX(n, xy);
      break;
   case kFullDotSmall://"Full dot small"
      fMarker.DrawFullDotSmall(n, xy);
      break;
   case kFullDotMedium:
      fMarker.DrawFullDotMedium(n, xy);
      break;
   case kFullDotLarge:
   case kFullCircle:
      fMarker.DrawFullDotLarge(n, xy);
      break;
   case kFullSquare:
      fMarker.DrawFullSquare(n, xy);
      break;
   case kFullTriangleUp:
      fMarker.DrawFullTrianlgeUp(n, xy);
      break;
   case kFullTriangleDown:
      fMarker.DrawFullTrianlgeDown(n, xy);
      break;
   case kOpenSquare:
      glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
      fMarker.DrawFullSquare(n, xy);
      glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
      break;
   case kOpenTriangleUp:
      glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
      fMarker.DrawFullTrianlgeUp(n, xy);
      glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
      break;
   case kOpenDiamond:
      fMarker.DrawDiamond(n, xy);
      break;
   case kOpenCross:
      fMarker.DrawCross(n, xy);
      break;
   case kFullStar:
      fMarker.DrawFullStar(n, xy);
      break;
   case kOpenStar:
      fMarker.DrawOpenStar(n, xy);
   }
   
   //Switch into user coords!!!.
   RestoreProjectionMatrix();
   glMatrixMode(GL_MODELVIEW);
}

//______________________________________________________________________________
void TGLPadPainter::DrawText(Double_t x, Double_t y, Double_t angle, Double_t mgn, const char *text, ETextMode /*mode*/)
{
   SaveProjectionMatrix();
   glLoadIdentity();
   //
   glOrtho(0, gPad->GetAbsWNDC() * fCanvas->GetWw(), 0, gPad->GetAbsHNDC() * fCanvas->GetWh(), -10., 10.);
   //
   glMatrixMode(GL_MODELVIEW);

   Float_t rgba[3] = {};
   Rgl::Pad::ExtractRGB(gVirtualX->GetTextColor(), rgba);
   glColor3fv(rgba);

   fFM.RegisterFont(Int_t(gVirtualX->GetTextSize()) - 1, TGLFontManager::GetFontNameFromId(gVirtualX->GetTextFont()), TGLFont::kTexture, fF);
   fF.PreRender();

   const UInt_t padH = UInt_t(gPad->GetAbsHNDC() * fCanvas->GetWh());
   fF.Render(text, gPad->XtoPixel(x), padH - gPad->YtoPixel(y), angle, mgn);

   fF.PostRender();
   RestoreProjectionMatrix();
   glMatrixMode(GL_MODELVIEW);
}

//______________________________________________________________________________
void TGLPadPainter::DrawFillArea(UInt_t n, const Float_t *x, const Float_t *y)
{
   if (!gVirtualX->GetFillStyle())
      return DrawPolyLine(n, x, y);

   fVs.resize(n * 3);
   
   for (UInt_t i = 0; i < n; ++i) {
      fVs[i * 3]     = x[i];
      fVs[i * 3 + 1] = y[i];
   }

   const Rgl::Pad::FillAttribSet fillAttribs(fSSet, kFALSE);

   GLUtesselator *t = (GLUtesselator *)fTess.GetTess();
   gluBeginPolygon(t);
   gluNextContour(t, (GLenum)GLU_UNKNOWN);

   for (UInt_t i = 0; i < n; ++i)
      gluTessVertex(t, &fVs[i * 3], &fVs[i * 3]);

      
   gluEndPolygon(t);
}

//______________________________________________________________________________
void TGLPadPainter::DrawPolyLine(UInt_t n, const Float_t *x, const Float_t *y)
{
   //xs is ok (with fX addition). ys must be converted from windows coordinates into gl.
   const Rgl::Pad::LineAttribSet lineAttribs(kTRUE, gVirtualX->GetLineStyle(), fLimits.GetMaxLineWidth(), kTRUE);

   glBegin(GL_LINE_STRIP);

   for (UInt_t i = 0; i < n; ++i)
      glVertex2f(x[i], y[i]);

   glEnd();
}

//______________________________________________________________________________
void TGLPadPainter::DrawPolyMarker(UInt_t /*n*/, const Float_t */*x*/, const Float_t */*y*/)
{/*
   Float_t rgba[3] = {};
   Rgl::Pad::ExtractRGB(gVirtualX->GetMarkerColor(), rgba);
   glColor3fv(rgba);
   
   fMarker.SetShifts(fX, fH - fY);
   
   const Style_t markerStyle = gVirtualX->GetMarkerStyle();
   switch (markerStyle) {
   case kDot:
      return fMarker.DrawDot(n, xy);
   case kPlus:
      return fMarker.DrawPlus(n, xy);
   case kStar:
      return fMarker.DrawStar(n, xy);
   case kCircle:
   case kOpenCircle:
      return fMarker.DrawCircle(n, xy);
   case kMultiply:
      return fMarker.DrawX(n, xy);
   case kFullDotSmall://"Full dot small"
      return fMarker.DrawFullDotSmall(n, xy);
   case kFullDotMedium:
      return fMarker.DrawFullDotMedium(n, xy);
   case kFullDotLarge:
   case kFullCircle:
      return fMarker.DrawFullDotLarge(n, xy);
   case kFullSquare:
      return fMarker.DrawFullSquare(n, xy);
   case kFullTriangleUp:
      return fMarker.DrawFullTrianlgeUp(n, xy);
   case kFullTriangleDown:
      return fMarker.DrawFullTrianlgeDown(n, xy);
   case kOpenSquare:
      glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
      fMarker.DrawFullSquare(n, xy);
      glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
      break;
   case kOpenTriangleUp:
      glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
      fMarker.DrawFullTrianlgeUp(n, xy);
      glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
      break;
   case kOpenDiamond:
      return fMarker.DrawDiamond(n, xy);
   case kOpenCross:
      return fMarker.DrawCross(n, xy);
   case kFullStar:
      return fMarker.DrawFullStar(n, xy);
   case kOpenStar:
      fMarker.DrawOpenStar(n, xy);
   }*/
}

//______________________________________________________________________________
Double_t TGLPadPainter::GetX(Double_t x)const
{
   return x + fX;
}

//______________________________________________________________________________
Double_t TGLPadPainter::GetY(Double_t y)const
{
   return fH - (y + fY);
}

//______________________________________________________________________________
void TGLPadPainter::SaveProjectionMatrix()const
{
   glMatrixMode(GL_PROJECTION);
   glPushMatrix();
}

//______________________________________________________________________________
void TGLPadPainter::RestoreProjectionMatrix()const
{
   glMatrixMode(GL_PROJECTION);
   glPopMatrix();
}
   
//______________________________________________________________________________
void TGLPadPainter::SaveModelviewMatrix()const
{
   glMatrixMode(GL_MODELVIEW);
   glPushMatrix();
}

//______________________________________________________________________________
void TGLPadPainter::RestoreModelviewMatrix()const
{
   glMatrixMode(GL_MODELVIEW);
   glPopMatrix();
}

//______________________________________________________________________________
void TGLPadPainter::SaveViewport()
{
   glGetIntegerv(GL_VIEWPORT, fVp);
}

//______________________________________________________________________________
void TGLPadPainter::RestoreViewport()
{
   glViewport(fVp[0], fVp[1], fVp[2], fVp[3]);
}

//______________________________________________________________________________
void TGLPadPainter::ConvertMarkerPoints(UInt_t n, const Double_t *x, const Double_t *y)
{
   const UInt_t padH = UInt_t(gPad->GetAbsHNDC() * fCanvas->GetWh());
   
   fPoly.resize(n);
   for (UInt_t i = 0; i < n; ++i) {
      fPoly[i].fX = gPad->XtoPixel(x[i]);
      fPoly[i].fY = padH - gPad->YtoPixel(y[i]);
   }   
}

//______________________________________________________________________________
void TGLPadPainter::InvalidateCS()
{
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   
   glOrtho(gPad->GetX1(), gPad->GetX2(), gPad->GetY1(), gPad->GetY2(), -10., 10.);
   
   glMatrixMode(GL_MODELVIEW);
}
