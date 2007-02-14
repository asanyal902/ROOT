#include <algorithm>
#include <iostream>
#include <cstdlib>
#include <cctype>

#include "KeySymbols.h"
#include "TVirtualGL.h"
#include "Buttons.h"
#include "TROOT.h"
#include "TString.h"
#include "TStyle.h"
#include "TColor.h"
#include "TMath.h"
#include "TAxis.h"
#include "TH1.h"

#include "TGLSurfacePainter.h"
#include "TGLOrthoCamera.h"
#include "TGLIncludes.h"

ClassImp(TGLSurfacePainter)

//______________________________________________________________________________
void TGLSurfacePainter::Projection_t::Swap(Projection_t &rhs)
{
   // Constructor.

   fRGBA[0] = rhs.fRGBA[0], fRGBA[1] = rhs.fRGBA[1], fRGBA[2] = rhs.fRGBA[2], fRGBA[3] = rhs.fRGBA[3];
   fVertices.swap(rhs.fVertices);
}

//______________________________________________________________________________
TGLSurfacePainter::TGLSurfacePainter(TH1 *hist, TGLOrthoCamera *camera, TGLPlotCoordinates *coord, Int_t ctx)
                                     : TGLPlotPainter(hist, camera, coord, ctx, kTRUE, kTRUE, kTRUE),
                                       fType(kSurf),
                                       fSectionPass(kFALSE),
                                       fUpdateTexMap(kTRUE)
{
   // Constructor.
}

//______________________________________________________________________________
char *TGLSurfacePainter::GetPlotInfo(Int_t px, Int_t py)
{
   //Coords for point on surface under cursor.
   if (fSelectedPart) {
      if (fHighColor)
         return fSelectedPart < fSelectionBase ? (char *)"TF2" : (char *)"Switch to true-color mode to obtain correct info";
      return fSelectedPart < fSelectionBase ? (char *)"TF2" : WindowPointTo3DPoint(px, py);
   }
   return "";
}

//______________________________________________________________________________
Bool_t TGLSurfacePainter::InitGeometry()
{
   //Set mesh, normals.
   switch (fCoord->GetCoordType()) {
   case kGLCartesian:
      return InitGeometryCartesian();
   case kGLPolar:
      return InitGeometryPolar();
   case kGLCylindrical:
      return InitGeometryCylindrical();
   case kGLSpherical:
      return InitGeometrySpherical();
   default:
      return kFALSE;
   }
}

//______________________________________________________________________________
void TGLSurfacePainter::StartPan(Int_t px, Int_t py)
{
   //User clicks right mouse button (in a pad).
   fMousePosition.fX = px;
   fMousePosition.fY = fCamera->GetHeight() - py;
   fCamera->StartPan(px, py);
   fBoxCut.StartMovement(px, fCamera->GetHeight() - py);
}

//______________________________________________________________________________
void TGLSurfacePainter::Pan(Int_t px, Int_t py)
{
   //User's moving mouse cursor, with middle mouse button pressed (for pad).
   //Calculate 3d shift related to 2d mouse movement.
   if (!MakeGLContextCurrent())
      return;

   if (fSelectedPart >= fSelectionBase)//Pan camera.
      fCamera->Pan(px, py);
   else if (fSelectedPart > 0) {
      //Convert py into bottom-top orientation.
      py = fCamera->GetHeight() - py;

      if (!fHighColor) {
         if (fBoxCut.IsActive() && (fSelectedPart >= kXAxis && fSelectedPart <= kZAxis))
            fBoxCut.MoveBox(px, py, fSelectedPart);
         else
            MoveSection(px, py);
      }
      else
         MoveSection(px, py);
   }

   fMousePosition.fX = px, fMousePosition.fY = py;
   fUpdateSelection = kTRUE;
}

//______________________________________________________________________________
void TGLSurfacePainter::AddOption(const TString &option)
{
   //Additional options for surfaces.
   using namespace std;
   const Ssiz_t surfPos = option.Index("surf");//"surf" _already_ _exists_ in a string.
   if (surfPos + 4 < option.Length() && isdigit(option[surfPos + 4])) {
      switch (option[surfPos + 4] - '0') {
      case 1:
         fType = kSurf1;
         break;
      case 2:
         fType = kSurf2;
         break;
      case 3:
         fType = kSurf3;
         fCoord->SetCoordType(kGLCartesian);
         break;
      case 4:
         fType = kSurf4;
         break;
      case 5:
         if (fCoord->GetCoordType() != kGLSpherical && fCoord->GetCoordType() != kGLCylindrical)
            fType = kSurf3;
         else
            fType = kSurf5;
         break;
      default:
         fType = kSurf;
      }
   } else
      fType = kSurf;
}

//______________________________________________________________________________
void TGLSurfacePainter::ProcessEvent(Int_t event, Int_t /*px*/, Int_t py)
{
   //Remove all profiles/sections.
   const TGLVertex3 *frame = fBackBox.Get3DBox();
   if (py == kKey_P || py == kKey_p) {

      if (HasSections()) {
         fSectionPass = kTRUE;
         DrawSectionXOZ();
         DrawSectionYOZ();
         DrawSectionXOY();
         fXOZSectionPos = frame[0].Y();
         fYOZSectionPos = frame[0].X();
         fXOYSectionPos = frame[0].Z();
         fSectionPass = kFALSE;
      }
   } else if (event == kButton1Double && (HasSections() || HasProjections() || fBoxCut.IsActive())) {
      fXOZSectionPos = frame[0].Y();
      fYOZSectionPos = frame[0].X();
      fXOYSectionPos = frame[0].Z();
      fXOZProj.clear();
      fYOZProj.clear();
      fXOYProj.clear();
      if (fBoxCut.IsActive())
         fBoxCut.TurnOnOff();
      gGLManager->PaintSingleObject(this);
   } else if (event == kKeyPress && (py == kKey_c || py == kKey_C)) {
      if (fHighColor)
         Info("ProcessEvent", "Switch to true color to use box cut");
      else {
         fBoxCut.TurnOnOff();
         fUpdateSelection = kTRUE;
      }
   }
}

//______________________________________________________________________________
void TGLSurfacePainter::InitGL()const
{
   //Initialize some OpenGL state variables.
   glEnable(GL_LIGHTING);
   glEnable(GL_LIGHT0);
   glEnable(GL_DEPTH_TEST);
   glDisable(GL_CULL_FACE);
   glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
}

//______________________________________________________________________________
void TGLSurfacePainter::SetNormals()
{
   //One normal per vertex;
   //this normal is average of
   //neighbouring triangles normals.
   const Int_t nX = fCoord->GetNXBins();
   const Int_t nY = fCoord->GetNYBins();

   fFaceNormals.resize((nX + 1) * (nY + 1));
   fFaceNormals.assign(fFaceNormals.size(), std::pair<TGLVector3, TGLVector3>());
   fFaceNormals.SetRowLen(nY + 1);


   //first, calculate normal for each triangle face
   for (Int_t i = 0; i < nX - 1; ++i) {
      for (Int_t j = 0; j < nY - 1; ++j) {
         //first "bottom-left" triangle
         TMath::Normal2Plane(fMesh[i][j + 1].CArr(), fMesh[i][j].CArr(), fMesh[i + 1][j].CArr(),
                             fFaceNormals[i + 1][j + 1].first.Arr());
         //second "top-right" triangle
         TMath::Normal2Plane(fMesh[i + 1][j].CArr(), fMesh[i + 1][j + 1].CArr(), fMesh[i][j + 1].CArr(),
                             fFaceNormals[i + 1][j + 1].second.Arr());
      }
   }

   fAverageNormals.resize(nX * nY);
   fAverageNormals.SetRowLen(nY);

   fAverageNormals.assign(fAverageNormals.size(), TGLVector3());
   //second, calculate average normal for each vertex
   for (Int_t i = 0; i < nX; ++i) {
      for (Int_t j = 0; j < nY; ++j) {
         TGLVector3 &norm = fAverageNormals[i][j];

         norm += fFaceNormals[i][j].second;
         norm += fFaceNormals[i][j + 1].first;
         norm += fFaceNormals[i][j + 1].second;
         norm += fFaceNormals[i + 1][j].first;
         norm += fFaceNormals[i + 1][j].second;
         norm += fFaceNormals[i + 1][j + 1].first;

         if (!norm.X() && !norm.Y() && !norm.Z())
            continue;

         norm.Normalise();
      }
   }
}

//______________________________________________________________________________
void TGLSurfacePainter::SetSurfaceColor()const
{
   //Set color for surface.
   Float_t diffColor[] = {0.8f, 0.8f, 0.8f, 0.35f};

   if (fHist->GetFillColor() != kWhite && fType != kSurf1 && fType != kSurf2 && fType != kSurf5)
      if (const TColor *c = gROOT->GetColor(fHist->GetFillColor()))
         c->GetRGB(diffColor[0], diffColor[1], diffColor[2]);

   glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, diffColor);
   const Float_t specColor[] = {1.f, 1.f, 1.f, 1.f};
   glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, specColor);
   glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 70.f);
}

//______________________________________________________________________________
void TGLSurfacePainter::DrawPlot()const
{
   //Draw surf/surf1/surf2/surf4
   if (fCoord->GetCoordType() == kGLCartesian) {
      fBackBox.DrawBox(fSelectedPart, fSelectionPass, fZLevels, fHighColor);
      DrawSections();
      if (!fSelectionPass)
         DrawProjections();
   }

   if (!fSelectionPass) {
      SetSurfaceColor();
      glEnable(GL_POLYGON_OFFSET_FILL);
      glPolygonOffset(1.f, 1.f);

      if (HasSections() || HasProjections())
      {
         //Surface is semi-transparent during dynamic profiling
         glEnable(GL_BLEND);
         glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
      }
      
   }

   if (Textured() && !fSelectionPass) {
      if (!PreparePalette())
         fType = kSurf1;
      if (fType != kSurf3 && !fPalette.EnableTexture(GL_MODULATE))
         fType = kSurf1;
   }

   const Int_t nX = fCoord->GetNXBins();
   const Int_t nY = fCoord->GetNYBins();
   const Int_t frontPoint = fBackBox.GetFrontPoint();
   Int_t i = 0, firstJ = 0;
   const Int_t addI = frontPoint == 2 || frontPoint == 1 ? i = 0, 1 : (i = nX - 2, -1);
   const Int_t addJ = frontPoint == 2 || frontPoint == 3 ? firstJ = 0, 1 : (firstJ = nY - 2, -1);

   if (fHighColor && fSelectionPass)
      Rgl::ObjectIDToColor(fSelectionBase, kTRUE);

   for (; addI > 0 ? i < nX - 1 : i >= 0; i += addI) {
      for (Int_t j = firstJ; addJ > 0 ? j < nY - 1 : j >= 0; j += addJ) {
         Int_t triNumber = 2 * i * (nY - 1) + j * 2 + fSelectionBase;

         Double_t xMin = TMath::Min(TMath::Min(fMesh[i][j + 1].X(), fMesh[i][j].X()), fMesh[i + 1][j].X());
         Double_t xMax = TMath::Max(TMath::Max(fMesh[i][j + 1].X(), fMesh[i][j].X()), fMesh[i + 1][j].X());
         Double_t yMin = TMath::Min(TMath::Min(fMesh[i][j + 1].Y(), fMesh[i][j].Y()), fMesh[i + 1][j].Y());
         Double_t yMax = TMath::Max(TMath::Max(fMesh[i][j + 1].Y(), fMesh[i][j].Y()), fMesh[i + 1][j].Y());
         Double_t zMin = TMath::Min(TMath::Min(fMesh[i][j + 1].Z(), fMesh[i][j].Z()), fMesh[i + 1][j].Z());
         Double_t zMax = TMath::Max(TMath::Max(fMesh[i][j + 1].Z(), fMesh[i][j].Z()), fMesh[i + 1][j].Z());

         if (fBoxCut.IsActive() && fBoxCut.IsInCut(xMin, xMax, yMin, yMax, zMin, zMax))
            continue;

         if (fSelectionPass && !fHighColor)
            Rgl::ObjectIDToColor(triNumber, kFALSE);

         if ((fType == kSurf1 || fType == kSurf2 || fType == kSurf5) && !fSelectionPass)
            Rgl::DrawFaceTextured(fMesh[i][j + 1], fMesh[i][j], fMesh[i + 1][j],
                                 fTexMap[i][j + 1], fTexMap[i][j], fTexMap[i + 1][j],
                                 fAverageNormals[i][j + 1], fAverageNormals[i][j],
                                 fAverageNormals[i + 1][j]);
         else
            Rgl::DrawSmoothFace(fMesh[i][j + 1], fMesh[i][j], fMesh[i + 1][j],
                              fAverageNormals[i][j + 1], fAverageNormals[i][j],
                              fAverageNormals[i + 1][j]);

         ++triNumber;

         if (fSelectionPass && !fHighColor)
            Rgl::ObjectIDToColor(triNumber, kFALSE);

         if ((fType == kSurf1 || fType == kSurf2 || fType == kSurf5) && !fSelectionPass)
            Rgl::DrawFaceTextured(fMesh[i + 1][j], fMesh[i + 1][j + 1], fMesh[i][j + 1],
                                 fTexMap[i + 1][j], fTexMap[i + 1][j + 1], fTexMap[i][j + 1],
                                 fAverageNormals[i + 1][j], fAverageNormals[i + 1][j + 1],
                                 fAverageNormals[i][j + 1]);
         else
            Rgl::DrawSmoothFace(fMesh[i + 1][j], fMesh[i + 1][j + 1], fMesh[i][j + 1],
                              fAverageNormals[i + 1][j], fAverageNormals[i + 1][j + 1],
                              fAverageNormals[i][j + 1]);
      }
   }

   if (!fSelectionPass)
      glDisable(GL_POLYGON_OFFSET_FILL);

   if (fBoxCut.IsActive())
      fBoxCut.DrawBox(fSelectionPass, fSelectedPart);

   //Draw outlines here
   if (!fSelectionPass) {
      const TGLEnableGuard  blendGuard(GL_BLEND);
      
      if (fType == kSurf || fType == kSurf1 || fType == kSurf3) {
         const TGLDisableGuard lightGuard(GL_LIGHTING);
         const TGLEnableGuard  smoothGuard(GL_LINE_SMOOTH);

         glDepthMask(GL_FALSE);

         glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
         glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

         glColor4d(0., 0., 0., 0.5);

         for (Int_t i = 0; i < nX - 1; ++i) {
            for (Int_t j = 0; j < nY - 1; ++j) {
               Rgl::DrawQuadOutline(fMesh[i][j + 1], fMesh[i][j], fMesh[i + 1][j], fMesh[i + 1][j + 1]);
            }
         }

         glDepthMask(GL_TRUE);
      }
   }

   if (fType == kSurf3 && !fSelectionPass && fPalette.EnableTexture(GL_MODULATE)) {
      glEnable(GL_BLEND);
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
      DrawContoursProjection();
   }

   if (Textured() && !fSelectionPass)
      fPalette.DisableTexture();

   if (!fSelectionPass && fSelectedPart > 6) {
      //Draw red outline for surface.
      const TGLDisableGuard lightGuard(GL_LIGHTING);
      const TGLEnableGuard blendGuard(GL_BLEND);
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
      const TGLEnableGuard smoothGuard(GL_LINE_SMOOTH);
      glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
      glLineWidth(3.f);

      glColor4d(1.f, 0.f, 0.4f, 0.6f);
      glBegin(GL_LINE_STRIP);
      for (Int_t i = 0; i < nX; ++i)
         glVertex3dv(fMesh[i][0].CArr());
      for (Int_t j = 0; j < nY; ++j)
         glVertex3dv(fMesh[nX - 1][j].CArr());
      for (Int_t i = nX - 1; i >= 0; --i)
         glVertex3dv(fMesh[i][nY - 1].CArr());
      for (Int_t j = nY - 1; j >= 0; --j)
         glVertex3dv(fMesh[0][j].CArr());
      glEnd();
      glLineWidth(1.f);
   }
}

//______________________________________________________________________________
Bool_t TGLSurfacePainter::InitGeometryCartesian()
{
   //Find bin ranges for X and Y axes,
   //axes ranges for X, Y and Z.
   //Function returns false, if logarithmic scale for
   //some axis was requested, but we cannot
   //find correct range.

   if (!fCoord->SetRanges(fHist, kFALSE, kFALSE)) //the second arg must be drawErrors, the third is always kFALSE.
      return kFALSE;

   fBackBox.SetPlotBox(fCoord->GetXRangeScaled(), fCoord->GetYRangeScaled(), fCoord->GetZRangeScaled());
   fCamera->SetViewVolume(fBackBox.Get3DBox());
   //Set surface's mesh
   //Calculates table of X and Y for lego (Z is obtained during drawing) or
   //calculate mesh of triangles with vertices in the centres of bins
   const Int_t nX = fCoord->GetNXBins();
   const Int_t nY = fCoord->GetNYBins();

   fMesh.resize(nX * nY);
   fMesh.SetRowLen(nY);

   for (Int_t i = 0, ir = fCoord->GetFirstXBin(); i < nX; ++i, ++ir) {
      for (Int_t j = 0, jr = fCoord->GetFirstYBin(); j < nY; ++j, ++jr) {
         fCoord->GetXLog() ? fMesh[i][j].X() = TMath::Log10(fXAxis->GetBinCenter(ir)) * fCoord->GetXScale()
                           : fMesh[i][j].X() = fXAxis->GetBinCenter(ir) * fCoord->GetXScale();
         fCoord->GetYLog() ? fMesh[i][j].Y() = TMath::Log10(fYAxis->GetBinCenter(jr)) * fCoord->GetYScale()
                           : fMesh[i][j].Y() = fYAxis->GetBinCenter(jr) * fCoord->GetYScale();

         Double_t z = fHist->GetCellContent(ir, jr);
         ClampZ(z);
         fMesh[i][j].Z() = z;
      }
   }

   if (Textured()) {
      fMinMaxVal.first  = fHist->GetBinContent(fCoord->GetFirstXBin(), fCoord->GetFirstYBin());
      fMinMaxVal.second = fMinMaxVal.first;

      for (Int_t i = fCoord->GetFirstXBin(), e = fCoord->GetLastXBin(); i <= e; ++i) {
         for (Int_t j = fCoord->GetFirstYBin(), e1 = fCoord->GetLastYBin(); j <= e1; ++j) {
            Double_t val = fHist->GetBinContent(i, j);
            fMinMaxVal.first  = TMath::Min(fMinMaxVal.first, val);
            fMinMaxVal.second = TMath::Max(fMinMaxVal.second, val);
         }
      }

      ClampZ(fMinMaxVal.first);
      ClampZ(fMinMaxVal.second);

      fUpdateTexMap = kTRUE;
   }

   SetNormals();

   if (fCoord->Modified()) {
      fUpdateSelection = kTRUE;
      const TGLVertex3 &vertex = fBackBox.Get3DBox()[0];
      fXOZSectionPos = vertex.Y();
      fYOZSectionPos = vertex.X();
      fXOYSectionPos = vertex.Z();
      fCoord->ResetModified();
      Rgl::SetZLevels(fZAxis, fCoord->GetZRange().first, fCoord->GetZRange().second, fCoord->GetZScale(), fZLevels);
   }

   return kTRUE;
}

//______________________________________________________________________________
Bool_t TGLSurfacePainter::InitGeometryPolar()
{
   //Find bin ranges for X and Y axes,
   //axes ranges for X, Y and Z.
   //Function returns false, if logarithmic scale for
   //some axis was requested, but we cannot
   //find correct range.

   if (!fCoord->SetRanges(fHist, kFALSE, kFALSE))
      return kFALSE;

   fBackBox.SetPlotBox(fCoord->GetXRangeScaled(), fCoord->GetYRangeScaled(), fCoord->GetZRangeScaled());
   fCamera->SetViewVolume(fBackBox.Get3DBox());

   if (fCoord->Modified()) {
      fUpdateSelection = kTRUE;
      const TGLVertex3 &vertex = fBackBox.Get3DBox()[0];
      fXOZSectionPos = vertex.Y();
      fYOZSectionPos = vertex.X();
      fXOYSectionPos = vertex.Z();
      fCoord->ResetModified();
   }

   const Int_t nY = fCoord->GetNYBins();
   const Int_t nX = fCoord->GetNXBins();

   fMesh.resize(nX * nY);
   fMesh.SetRowLen(nY);

   const Double_t fullAngle = fXAxis->GetXmax() - fXAxis->GetXmin();
   const Double_t phiLow    = fXAxis->GetXmin();
   const Double_t rRange    = fYAxis->GetXmax() - fYAxis->GetXmin();

   for (Int_t i = 0, ir = fCoord->GetFirstXBin(); i < nX; ++i, ++ir) {
      for (Int_t j = 0, jr = fCoord->GetFirstYBin(); j < nY; ++j, ++jr) {
         const Double_t angle  = (fXAxis->GetBinCenter(ir) - phiLow) / fullAngle * TMath::TwoPi();
         const Double_t radius = ((fYAxis->GetBinCenter(jr)) - fYAxis->GetXmin()) /
                                 rRange * fCoord->GetYScale();
         fMesh[i][j].X() = radius * TMath::Cos(angle);
         fMesh[i][j].Y() = radius * TMath::Sin(angle);
         Double_t z = fHist->GetBinContent(ir, jr);
         ClampZ(z);
         fMesh[i][j].Z() = z;
      }
   }

   SetNormals();

   if (Textured()) {
      fMinMaxVal.first  = fHist->GetBinContent(fCoord->GetFirstXBin(), fCoord->GetFirstYBin());
      fMinMaxVal.second = fMinMaxVal.first;

      for (Int_t i = fCoord->GetFirstXBin(), e = fCoord->GetLastXBin(); i <= e; ++i) {
         for (Int_t j = fCoord->GetFirstYBin(), e1 = fCoord->GetLastYBin(); j <= e1; ++j) {
            Double_t val = fHist->GetBinContent(i, j);
            fMinMaxVal.first  = TMath::Min(fMinMaxVal.first, val);
            fMinMaxVal.second = TMath::Max(fMinMaxVal.second, val);
         }
      }

      fUpdateTexMap = kTRUE;
   }


   return kTRUE;
}

//______________________________________________________________________________
Bool_t TGLSurfacePainter::InitGeometryCylindrical()
{
   //Find bin ranges for X and Y axes,
   //axes ranges for X, Y and Z.
   //Function returns false, if logarithmic scale for
   //some axis was requested, but we cannot
   //find correct range.

   if (!fCoord->SetRanges(fHist, kFALSE, kFALSE))
      return kFALSE;

   fBackBox.SetPlotBox(fCoord->GetXRangeScaled(), fCoord->GetYRangeScaled(), fCoord->GetZRangeScaled());
   fCamera->SetViewVolume(fBackBox.Get3DBox());

   if (fCoord->Modified()) {
      fUpdateSelection = kTRUE;
      const TGLVertex3 &vertex = fBackBox.Get3DBox()[0];
      fXOZSectionPos = vertex.Y();
      fYOZSectionPos = vertex.X();
      fXOYSectionPos = vertex.Z();
      fCoord->ResetModified();
   }

   const Int_t nY = fCoord->GetNYBins();
   const Int_t nX = fCoord->GetNXBins();
   fMesh.resize(nX * nY);
   fMesh.SetRowLen(nY);

   Double_t legoR = gStyle->GetLegoInnerR();
   if (legoR > 1. || legoR < 0.)
      legoR = 0.5;
   const Double_t rRange = fCoord->GetZLength();
   const Double_t sc = (1 - legoR) * fCoord->GetXScale();
   legoR *= fCoord->GetXScale();

   const Double_t fullAngle = fXAxis->GetXmax() - fXAxis->GetXmin();
   const Double_t phiLow    = fXAxis->GetXmin();
   Double_t angle = 0.;

   for (Int_t i = 0, ir = fCoord->GetFirstXBin(); i < nX; ++i, ++ir) {
      for (Int_t j = 0, jr = fCoord->GetFirstYBin(); j < nY; ++j, ++jr) {
         angle = (fXAxis->GetBinLowEdge(ir) - phiLow) / fullAngle * TMath::TwoPi();
         Double_t r = fType != kSurf5 ? legoR + (fHist->GetCellContent(ir, jr) - fCoord->GetZRange().first) / rRange * sc : legoR;
         fMesh[i][j].X() = r * TMath::Cos(angle);
         fMesh[i][j].Y() = fCoord->GetYLog() ?
                              TMath::Log10(fYAxis->GetBinCenter(jr)) * fCoord->GetYScale()
                                          :
                              fYAxis->GetBinCenter(jr) * fCoord->GetYScale();
         fMesh[i][j].Z() = r * TMath::Sin(angle);
      }
   }

   if (Textured()) {
      fMinMaxVal.first  = fHist->GetBinContent(fCoord->GetFirstXBin(), fCoord->GetFirstYBin());
      fMinMaxVal.second = fMinMaxVal.first;

      for (Int_t i = fCoord->GetFirstXBin(), e = fCoord->GetLastXBin(); i <= e; ++i) {
         for (Int_t j = fCoord->GetFirstYBin(), e1 = fCoord->GetLastYBin(); j <= e1; ++j) {
            Double_t val = fHist->GetBinContent(i, j);
            fMinMaxVal.first  = TMath::Min(fMinMaxVal.first, val);
            fMinMaxVal.second = TMath::Max(fMinMaxVal.second, val);
         }
      }

      fUpdateTexMap = kTRUE;
   }


   SetNormals();

   return kTRUE;
}

//______________________________________________________________________________
Bool_t TGLSurfacePainter::InitGeometrySpherical()
{
   //Find bin ranges for X and Y axes,
   //axes ranges for X, Y and Z.
   //Function returns false, if logarithmic scale for
   //some axis was requested, but we cannot
   //find correct range.

   if (!fCoord->SetRanges(fHist, kFALSE, kFALSE))
      return kFALSE;

   fBackBox.SetPlotBox(fCoord->GetXRangeScaled(), fCoord->GetYRangeScaled(), fCoord->GetZRangeScaled());
   fCamera->SetViewVolume(fBackBox.Get3DBox());

   if (fCoord->Modified()) {
      fUpdateSelection = kTRUE;
      const TGLVertex3 &vertex = fBackBox.Get3DBox()[0];
      fXOZSectionPos = vertex.Y();
      fYOZSectionPos = vertex.X();
      fXOYSectionPos = vertex.Z();
      fCoord->ResetModified();
   }

   const Int_t nY = fCoord->GetNYBins();
   const Int_t nX = fCoord->GetNXBins();
   fMesh.resize(nX * nY);
   fMesh.SetRowLen(nY);

   Double_t legoR = gStyle->GetLegoInnerR();
   if (legoR > 1. || legoR < 0.)
      legoR = 0.5;
   const Double_t rRange = fCoord->GetZLength();
   const Double_t sc = (1 - legoR) * fCoord->GetXScale();
   legoR *= fCoord->GetXScale();

   //0 <= theta <= 2 * pi
   const Double_t fullTheta   = fXAxis->GetXmax() - fXAxis->GetXmin();
   const Double_t thetaLow    = fXAxis->GetXmin();
   //0 <= phi <= 2 * pi
   const Double_t fullPhi = fYAxis->GetXmax() - fYAxis->GetXmin();
   const Double_t phiLow  = fYAxis->GetXmin();

   for (Int_t i = 0, ir = fCoord->GetFirstXBin(); i < nX; ++i, ++ir) {

      const Double_t theta = (fXAxis->GetBinCenter(ir) - thetaLow) / fullTheta * TMath::TwoPi();

      for (Int_t j = 0, jr = fCoord->GetFirstYBin(); j < nY; ++j, ++jr) {

         const Double_t phi = (fYAxis->GetBinCenter(jr) - phiLow) / fullPhi * TMath::Pi();
         const Double_t r   = fType != kSurf5 ? legoR + (fHist->GetCellContent(ir, jr) - fCoord->GetZRange().first) / rRange * sc
                                             : legoR;

         fMesh[i][j].X() = r * TMath::Sin(phi) * TMath::Cos(theta);
         fMesh[i][j].Y() = r * TMath::Sin(phi) * TMath::Sin(theta);
         fMesh[i][j].Z() = r * TMath::Cos(phi);
      }
   }

   if (Textured()) {
      fMinMaxVal.first  = fHist->GetBinContent(fCoord->GetFirstXBin(), fCoord->GetFirstYBin());
      fMinMaxVal.second = fMinMaxVal.first;

      for (Int_t i = fCoord->GetFirstXBin(), e = fCoord->GetLastXBin(); i <= e; ++i) {
         for (Int_t j = fCoord->GetFirstYBin(), e1 = fCoord->GetLastYBin(); j <= e1; ++j) {
            Double_t val = fHist->GetBinContent(i, j);
            fMinMaxVal.first  = TMath::Min(fMinMaxVal.first, val);
            fMinMaxVal.second = TMath::Max(fMinMaxVal.second, val);
         }
      }

      fUpdateTexMap = kTRUE;
   }


   SetNormals();

   return kTRUE;
}

//______________________________________________________________________________
void TGLSurfacePainter::DrawProjections()const
{
   // Draw projections.

   const TGLDisableGuard lightGuard(GL_LIGHTING);
   const TGLEnableGuard  blendGuard(GL_BLEND);
   const TGLEnableGuard  lineSmooth(GL_LINE_SMOOTH);
   glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
   glDepthMask(GL_FALSE);
   glLineWidth(3.f);

   typedef std::list<Projection_t>::const_iterator CLI_t;
   for (CLI_t begin = fXOZProj.begin(), end = fXOZProj.end(); begin != end; ++begin) {
      const Projection_t &proj = *begin;
      glColor4ub(proj.fRGBA[0], proj.fRGBA[1], proj.fRGBA[2], proj.fRGBA[3]);

      for(UInt_t i = 0, e = proj.fVertices.size() / 3; i < e; ++i) {
         glBegin(GL_LINE_STRIP);
         glVertex3dv(proj.fVertices[i * 3].CArr());
         glVertex3dv(proj.fVertices[i * 3 + 1].CArr());
         glVertex3dv(proj.fVertices[i * 3 + 2].CArr());
         glEnd();
      }
      const Double_t y = fBackBox.GetFrontPoint() == 2 || fBackBox.GetFrontPoint() == 3 ? fBackBox.Get3DBox()[0].Y() : fBackBox.Get3DBox()[2].Y();
      for(UInt_t i = 0, e = proj.fVertices.size() / 3; i < e; ++i) {
         glBegin(GL_LINE_STRIP);
         const TGLVertex3 &v1 = proj.fVertices[i * 3];
         glVertex3d(v1.X(), y, v1.Z());
         const TGLVertex3 &v2 = proj.fVertices[i * 3 + 1];
         glVertex3d(v2.X(), y, v2.Z());
         const TGLVertex3 &v3 = proj.fVertices[i * 3 + 2];
         glVertex3d(v3.X(), y, v3.Z());
         glEnd();
      }
   }

   for (CLI_t begin = fYOZProj.begin(), end = fYOZProj.end(); begin != end; ++begin) {
      const Projection_t &proj = *begin;
      glColor4ub(proj.fRGBA[0], proj.fRGBA[1], proj.fRGBA[2], proj.fRGBA[3]);

      for(UInt_t i = 0, e = proj.fVertices.size() / 3; i < e; ++i) {
         glBegin(GL_LINE_STRIP);
         glVertex3dv(proj.fVertices[i * 3].CArr());
         glVertex3dv(proj.fVertices[i * 3 + 1].CArr());
         glVertex3dv(proj.fVertices[i * 3 + 2].CArr());
         glEnd();
      }

      const Double_t x = fBackBox.GetFrontPoint() == 2 || fBackBox.GetFrontPoint() == 1 ? fBackBox.Get3DBox()[0].X() : fBackBox.Get3DBox()[2].X();
      for(UInt_t i = 0, e = proj.fVertices.size() / 3; i < e; ++i) {
         glBegin(GL_LINE_STRIP);
         const TGLVertex3 &v1 = proj.fVertices[i * 3];
         glVertex3d(x, v1.Y(), v1.Z());
         const TGLVertex3 &v2 = proj.fVertices[i * 3 + 1];
         glVertex3d(x, v2.Y(), v2.Z());
         const TGLVertex3 &v3 = proj.fVertices[i * 3 + 2];
         glVertex3d(x, v3.Y(), v3.Z());
         glEnd();
      }
   }

   for (CLI_t begin = fXOYProj.begin(), end = fXOYProj.end(); begin != end; ++begin) {
      const Projection_t &proj = *begin;
      glColor4ub(proj.fRGBA[0], proj.fRGBA[1], proj.fRGBA[2], proj.fRGBA[3]);

      for(UInt_t i = 0, e = proj.fVertices.size() / 2; i < e; ++i) {
         glBegin(GL_LINES);
         glVertex3dv(proj.fVertices[i * 2].CArr());
         glVertex3dv(proj.fVertices[i * 2 + 1].CArr());
         glEnd();
      }


      for(UInt_t i = 0, e = proj.fVertices.size() / 2; i < e; ++i) {
         glBegin(GL_LINES);
         const TGLVertex3 &v1 = proj.fVertices[i * 2];
         glVertex3d(v1.X(), v1.Y(), fBackBox.Get3DBox()[0].Z());
         const TGLVertex3 &v2 = proj.fVertices[i * 2 + 1];
         glVertex3d(v2.X(), v2.Y(), fBackBox.Get3DBox()[0].Z());
         glEnd();
      }

   }

   glDepthMask(GL_TRUE);
   glLineWidth(1.f);
}

//______________________________________________________________________________
void TGLSurfacePainter::DrawSectionXOZ()const
{
   // Draw section X.

   using namespace std;
   //XOZ parallel section.
   Int_t binY = -1;
   for (Int_t j = 0, e = fCoord->GetNYBins() - 1; j < e; ++j) {
      if (fMesh[0][j].Y() <= fXOZSectionPos && fXOZSectionPos <= fMesh[0][j + 1].Y()) {
         binY = j;
         break;
      }
   }

   if (binY >= 0) {
      glColor3d(1., 0., 0.);
      glLineWidth(3.f);
      //Draw 2d curve on the profile's plane.
      const TGLPlane profilePlane(0., 1., 0., -fXOZSectionPos);

      if (!fSectionPass) {
         for (Int_t i = 0, e = fCoord->GetNXBins() - 1; i < e; ++i) {
            glBegin(GL_LINE_STRIP);
            glVertex3dv(Intersection(profilePlane, TGLLine3(fMesh[i + 1][binY], fMesh[i + 1][binY + 1]), kFALSE).second.CArr());
            glVertex3dv(Intersection(profilePlane, TGLLine3(fMesh[i + 1][binY], fMesh[i][binY + 1]), kFALSE).second.CArr());
            glVertex3dv(Intersection(profilePlane, TGLLine3(fMesh[i][binY], fMesh[i][binY + 1]), kFALSE).second.CArr());
            glEnd();
         }
         glLineWidth(1.f);
      } else {
         fProj.fVertices.clear();
         for (Int_t i = 0, e = fCoord->GetNXBins() - 1; i < e; ++i) {
            fProj.fVertices.push_back(Intersection(profilePlane, TGLLine3(fMesh[i + 1][binY], fMesh[i + 1][binY + 1]), kFALSE).second);
            fProj.fVertices.push_back(Intersection(profilePlane, TGLLine3(fMesh[i + 1][binY], fMesh[i][binY + 1]), kFALSE).second);
            fProj.fVertices.push_back(Intersection(profilePlane, TGLLine3(fMesh[i][binY], fMesh[i][binY + 1]), kFALSE).second);
         }
         if (fProj.fVertices.size()) {
            fProj.fRGBA[0] = rand() % 200 + 56;
            fProj.fRGBA[1] = rand() % 100;
            fProj.fRGBA[2] = rand() % 100;
            fProj.fRGBA[3] = 150;
            static Projection_t dummy;
            fXOZProj.push_back(dummy);
            fXOZProj.back().Swap(fProj);
         }
      }
   }
}

//______________________________________________________________________________
void TGLSurfacePainter::DrawSectionYOZ()const
{
   // Draw section Y.

   using namespace std;
   //YOZ parallel section.
   Int_t binX = -1;
   for (Int_t i = 0, e = fCoord->GetNXBins() - 1; i < e; ++i) {
      if (fMesh[i][0].X() <= fYOZSectionPos && fYOZSectionPos <= fMesh[i + 1][0].X()) {
         binX = i;
         break;
      }
   }

   if (binX >= 0) {
      //Draw 2d curve on the profile's plane.
      const TGLPlane profilePlane(1., 0., 0., -fYOZSectionPos);

      if (!fSectionPass) {
         glColor3d(1., 0., 0.);
         glLineWidth(3.f);
         for (Int_t j = 0, e = fCoord->GetNYBins() - 1; j < e; ++j) {
            glBegin(GL_LINE_STRIP);
            glVertex3dv(Intersection(profilePlane, TGLLine3(fMesh[binX][j + 1], fMesh[binX + 1][j + 1]), kFALSE).second.CArr());
            glVertex3dv(Intersection(profilePlane, TGLLine3(fMesh[binX][j + 1], fMesh[binX + 1][j]), kFALSE).second.CArr());
            glVertex3dv(Intersection(profilePlane, TGLLine3(fMesh[binX][j], fMesh[binX + 1][j]), kFALSE).second.CArr());
            glEnd();
         }
         glLineWidth(1.f);
      } else {
         fProj.fVertices.clear();
         for (Int_t j = 0, e = fCoord->GetNYBins() - 1; j < e; ++j) {
            fProj.fVertices.push_back(Intersection(profilePlane, TGLLine3(fMesh[binX][j + 1], fMesh[binX + 1][j + 1]), kFALSE).second);
            fProj.fVertices.push_back(Intersection(profilePlane, TGLLine3(fMesh[binX][j + 1], fMesh[binX + 1][j]), kFALSE).second);
            fProj.fVertices.push_back(Intersection(profilePlane, TGLLine3(fMesh[binX][j], fMesh[binX + 1][j]), kFALSE).second);
         }
         if (fProj.fVertices.size()) {
            fProj.fRGBA[0] = rand() % 200 + 56;
            fProj.fRGBA[1] = rand() % 100;
            fProj.fRGBA[2] = rand() % 100;
            fProj.fRGBA[3] = 150;
            static Projection_t dummy;
            fYOZProj.push_back(dummy);
            fYOZProj.back().Swap(fProj);
         }
      }

   }
}

//______________________________________________________________________________
void TGLSurfacePainter::DrawSectionXOY()const
{
   // Draw section Z.

   using namespace std;
   //XOY parallel section.
   const Int_t nX = fCoord->GetNXBins();
   const Int_t nY = fCoord->GetNYBins();
   const TGLPlane profilePlane(0., 0., 1., -fXOYSectionPos);
   TGLVertex3 intersection[2];

   glColor3d(1., 0., 0.);
   glLineWidth(3.f);

   if (fSectionPass)
      fProj.fVertices.clear();

   for (Int_t i = 0; i < nX - 1; ++i) {
      for (Int_t j = 0; j < nY - 1; ++j) {
         const TGLVertex3 &v1 = fMesh[i + 1][j], &v2 = fMesh[i][j], &v3 = fMesh[i][j + 1], &v4 = fMesh[i + 1][j + 1];
         Double_t zMin = TMath::Min(TMath::Min(v1.Z(), v2.Z()), v3.Z());
         Double_t zMax = TMath::Max(TMath::Max(v1.Z(), v2.Z()), v3.Z());

         if (zMin < fXOYSectionPos && zMax > fXOYSectionPos) {
            Int_t np = 0;
            if (v1.Z() > fXOYSectionPos && v2.Z() < fXOYSectionPos || v2.Z() > fXOYSectionPos && v1.Z() < fXOYSectionPos)
            {
               TGLLine3 line(v1, v2);
               intersection[np++] = Intersection(profilePlane, line, kFALSE).second;
            }
            if (v2.Z() > fXOYSectionPos && v3.Z() < fXOYSectionPos || v3.Z() > fXOYSectionPos && v2.Z() < fXOYSectionPos)
            {
               TGLLine3 line(v2, v3);
               intersection[np++] = Intersection(profilePlane, line, kFALSE).second;
            }
            if (np < 2 && v1.Z() > fXOYSectionPos && v3.Z() < fXOYSectionPos || v3.Z() > fXOYSectionPos && v1.Z() < fXOYSectionPos)
            {
               TGLLine3 line(v1, v3);
               intersection[np++] = Intersection(profilePlane, line, kFALSE).second;
            }
            if (np > 1) {
               if (!fSectionPass) {
                  glBegin(GL_LINES);
                  glVertex3dv(intersection[0].CArr());
                  glVertex3dv(intersection[1].CArr());
                  glEnd();
               } else {
                  fProj.fVertices.push_back(intersection[0]);
                  fProj.fVertices.push_back(intersection[1]);
               }
            }
         }
         zMin = TMath::Min(v4.Z(), zMin);
         zMax = TMath::Max(v4.Z(), zMax);
         if (zMin < fXOYSectionPos && zMax > fXOYSectionPos) {
            Int_t np = 0;
            if (v3.Z() > fXOYSectionPos && v4.Z() < fXOYSectionPos || v4.Z() > fXOYSectionPos && v3.Z() < fXOYSectionPos)
            {
               TGLLine3 line(v3, v4);
               intersection[np++] = Intersection(profilePlane, line, kFALSE).second;
            }
            if (v4.Z() > fXOYSectionPos && v1.Z() < fXOYSectionPos || v1.Z() > fXOYSectionPos && v4.Z() < fXOYSectionPos)
            {
               TGLLine3 line(v4, v1);
               intersection[np++] = Intersection(profilePlane, line, kFALSE).second;
            }
            if (np < 2 && v3.Z() > fXOYSectionPos && v1.Z() < fXOYSectionPos || v1.Z() > fXOYSectionPos && v3.Z() < fXOYSectionPos)
            {
               TGLLine3 line(v3, v1);
               intersection[np++] = Intersection(profilePlane, line, kFALSE).second;
            }
            if (np > 1) {
               if (!fSectionPass) {
                  glBegin(GL_LINES);
                  glVertex3dv(intersection[0].CArr());
                  glVertex3dv(intersection[1].CArr());
                  glEnd();
               } else {
                  fProj.fVertices.push_back(intersection[0]);
                  fProj.fVertices.push_back(intersection[1]);
               }
            }
         }
      }
   }

   if (fSectionPass && fProj.fVertices.size()) {
      fProj.fRGBA[0] = rand() % 150;
      fProj.fRGBA[1] = rand() % 150;
      fProj.fRGBA[2] = rand() % 206 + 50;
      fProj.fRGBA[3] = 150;
      static Projection_t dummy;
      fXOYProj.push_back(dummy);
      fXOYProj.back().Swap(fProj);
   }

   glLineWidth(1.f);
}

//______________________________________________________________________________
void TGLSurfacePainter::ClampZ(Double_t &zVal)const
{
   //Clamp z value.
   const TGLVertex3 *frame = fBackBox.Get3DBox();

   if (fCoord->GetZLog())
      if (zVal <= 0.)
         zVal = frame[0].Z();
      else
         zVal = TMath::Log10(zVal) * fCoord->GetZScale();
   else
      zVal *= fCoord->GetZScale();

   if (zVal > frame[4].Z())
      zVal = frame[4].Z();
   else if (zVal < frame[0].Z())
      zVal = frame[0].Z();
}

//______________________________________________________________________________
char *TGLSurfacePainter::WindowPointTo3DPoint(Int_t px, Int_t py)const
{
   //Find 3d coords using mouse cursor coords.
   if (!MakeGLContextCurrent())
      return "Apocalipshit!";

   py = fCamera->GetHeight() - py;

   const Int_t nY = fCoord->GetNYBins() - 1;
   Int_t selected = fSelectedPart - (fSelectionBase - 1);
   Int_t k = selected / 2;
   Int_t i = k / nY;
   Int_t j = k % nY;

   const Bool_t odd = selected & 1;
   const TGLVertex3 &v1 = odd ? fMesh[i][j + 1] : fMesh[i + 1][j];
   const TGLVertex3 &v2 = odd ? fMesh[i + 1][j + 1] : fMesh[i][j];
   const TGLVertex3 &v3 = odd ? fMesh[i + 1][j] : fMesh[i][j + 1];

   TGLVertex3 winV1, winV2, winV3;

   Double_t mvMatrix[16] = {0.};
   glGetDoublev(GL_MODELVIEW_MATRIX, mvMatrix);
   Double_t prMatrix[16] = {0.};
   glGetDoublev(GL_PROJECTION_MATRIX, prMatrix);
   Int_t viewport[4] = {0};
   glGetIntegerv(GL_VIEWPORT, viewport);

   gluProject(v1.X(), v1.Y(), v1.Z(), mvMatrix, prMatrix, viewport, &winV1.X(), &winV1.Y(), &winV1.Z());
   gluProject(v2.X(), v2.Y(), v2.Z(), mvMatrix, prMatrix, viewport, &winV2.X(), &winV2.Y(), &winV2.Z());
   gluProject(v3.X(), v3.Y(), v3.Z(), mvMatrix, prMatrix, viewport, &winV3.X(), &winV3.Y(), &winV3.Z());

   Double_t planeABCD[4] = {0.};
   TMath::Normal2Plane(winV1.CArr(), winV2.CArr(), winV3.CArr(), planeABCD);
   planeABCD[3] = - winV1.X() * planeABCD[0] - winV1.Y() * planeABCD[1] - winV1.Z() * planeABCD[2];
   Double_t pz = (-planeABCD[3] - planeABCD[0] * px - planeABCD[1] * py) / planeABCD[2];
   Double_t rez[3] = {0.};

   gluUnProject(px, py, pz, mvMatrix, prMatrix, viewport, rez, rez + 1, rez + 2);

   fObjectInfo.Form("(x == %f, y == %f, z == %f)",
                    rez[0] / fCoord->GetXScale(),
                    rez[1] / fCoord->GetYScale(),
                    rez[2] / fCoord->GetZScale());

   return (char *)fObjectInfo.Data();
}
#include <iostream>
//______________________________________________________________________________
Bool_t TGLSurfacePainter::PreparePalette()const
{
   //Generate palette.
   if (!fUpdateTexMap)
      return kTRUE;

   UInt_t paletteSize = fHist->GetContour();
   if (!paletteSize && !(paletteSize = gStyle->GetNumberContours()))
      paletteSize = 20;

   Bool_t rez = fPalette.GeneratePalette(paletteSize, fMinMaxVal);

   if (rez && fHist->TestBit(TH1::kUserContour)) {

      fColorLevels.resize(paletteSize);

      for (UInt_t i = 0; i < paletteSize; ++i) {
         fColorLevels[i] = fHist->GetContourLevelPad(i);
         ClampZ(fColorLevels[i]);
      }

      fPalette.SetContours(&fColorLevels);
   }

   if (rez && fUpdateTexMap)
      GenTexMap();

   fUpdateTexMap = kFALSE;

   return rez;
}

//______________________________________________________________________________
void TGLSurfacePainter::GenTexMap()const
{
   //Find texture coordinates.
   const Int_t nX = fCoord->GetNXBins();
   const Int_t nY = fCoord->GetNYBins();

   fTexMap.resize(nX * nY);
   fTexMap.SetRowLen(nY);

   for (Int_t i = 0, ir = fCoord->GetFirstXBin(); i < nX; ++i, ++ir) {
      for (Int_t j = 0, jr = fCoord->GetFirstYBin(); j < nY; ++j, ++jr) {
         Double_t z = fHist->GetCellContent(ir, jr);
         if (fCoord->GetCoordType() == kGLCartesian)
            ClampZ(z);
         fTexMap[i][j] = fPalette.GetTexCoord(z);
      }
   }
}

//______________________________________________________________________________
void TGLSurfacePainter::DrawContoursProjection()const
{
   //Draw flat textured surface.
   static const Float_t whiteDiffuse[] = {0.8f, 0.8f, 0.8f, 0.65f};
   glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, whiteDiffuse);
   for (Int_t i = 0, ei = fCoord->GetNXBins() - 1; i < ei; ++i) {
      for (Int_t j = 0, ej = fCoord->GetNYBins() - 1; j < ej; ++j) {
         Rgl::DrawFaceTextured(fMesh[i][j + 1], fMesh[i][j], fMesh[i + 1][j],
                               fTexMap[i][j + 1], fTexMap[i][j], fTexMap[i + 1][j],
                               fBackBox.Get3DBox()[4].Z(), TGLVector3(0., 0., 1.));
         Rgl::DrawFaceTextured(fMesh[i + 1][j], fMesh[i + 1][j + 1], fMesh[i][j + 1],
                               fTexMap[i + 1][j], fTexMap[i + 1][j + 1], fTexMap[i][j + 1],
                               fBackBox.Get3DBox()[4].Z(), TGLVector3(0., 0., 1.));
      }
   }
}

//______________________________________________________________________________
Bool_t TGLSurfacePainter::Textured()const
{
   //Checks, if surf requires texture.
   switch (fType) {
   case kSurf1:
   case kSurf2:
   case kSurf3:
   case kSurf5:
      return kTRUE;
   default:;
   }

   return kFALSE;
}

//______________________________________________________________________________
Bool_t TGLSurfacePainter::HasSections()const
{
   //Any section exists.
   return fXOZSectionPos > fBackBox.Get3DBox()[0].Y() || fYOZSectionPos > fBackBox.Get3DBox()[0].X() ||
          fXOYSectionPos > fBackBox.Get3DBox()[0].Z();
}

//______________________________________________________________________________
Bool_t TGLSurfacePainter::HasProjections()const
{
   //Any projection exists.
   return fXOZProj.size() || fYOZProj.size() || fXOYProj.size();
}
