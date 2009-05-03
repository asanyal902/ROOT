#include <iostream>
#include <cstring>

#include "TROOT.h"
#include "TClass.h"
#include "TVirtualGL.h"
#include "KeySymbols.h"
#include "TCanvas.h"
#include "TMath.h"
#include "TPad.h"
#include "TF3.h"

#include "TGLSurfacePainter.h"
#include "TGLHistPainter.h"
#include "TGLLegoPainter.h"
#include "TGLBoxPainter.h"
#include "TGLTF3Painter.h"
#include "TGLParametric.h"

ClassImp(TGLHistPainter)

//______________________________________________________________________________
/* Begin_Html
<center><h2>The histogram painter class using OpenGL</h2></center>

Histograms are, by default, drawn via the <tt>THistPainter</tt> class.
<tt>TGLHistPainter</tt> allows to paint them using the OpenGL 3D graphics
library. The plotting options provided by <tt>TGLHistPainter</tt> start with
<tt>GL</tt> keyword.

<h3>General information: plot types and supported options</h3>

The following types of plots are provided:
<ul>
<p><li><b>Lego - (<tt>TGLLegoPainter</tt>)</b>
  <br> The supported options are:
  <ul>
  <li> <tt>"GLLEGO"  :</tt> Draw a lego plot.
  <li> <tt>"GLLEGO2" :</tt> Bins with color levels.
  <li> <tt>"GLLEGO3" :</tt> Cylindrical bars.
  </ul>
  Lego painter in cartesian supports logarithmic scales for X, Y, Z.
  In polar only Z axis can be logarithmic, in cylindrical only Y (if you see
  what it means).


<p><li><b>Surfaces (<tt>TF2</tt> and <tt>TH2</tt> with <tt>"GLSURF"</tt> options) - (<tt>TGLSurfacePainter</tt>)</b>
  <br> The supported options are:
  <ul>
  <li> <tt>"GLSURF"  :</tt> Draw a surface.
  <li> <tt>"GLSURF1" :</tt> Surface with color levels
  <li> <tt>"GLSURF2" :</tt> The same as <tt>"GLSURF1"</tt> but without polygon outlines.
  <li> <tt>"GLSURF3" :</tt> Color level projection on top of plot (works only in cartesian coordinate system).
  <li> <tt>"GLSURF4" :</tt> Same as <tt>"GLSURF"</tt> but without polygon outlines.
  </ul>

  The surface painting in cartesian coordinates supports logarithmic scales along X, Y, Z axis.
  In polar coordinates only the Z axis can be logarithmic, in cylindrical coordinates only the Y axis.

<p><li><b>Additional options to <tt>SURF</tt> and <tt>LEGO</tt> - Coordinate systems:</b>
  <br> The supported options are:
  <ul>
  <li> <tt>" "   :</tt> Default, cartesian coordinates system.
  <li> <tt>"POL" :</tt> Polar coordinates system.
  <li> <tt>"CYL" :</tt> Cylindrical coordinates system.
  <li> <tt>"SPH" :</tt> Spherical coordinates system.
  </ul>

<p><li><b><tt>TH3</tt> as boxes (spheres) - (<tt>TGLBoxPainter</tt>)</b>
  <br> The supported options are:
  <ul>
  <li> <tt>"GLBOX" :</tt> TH3 as a set of boxes, size of box is proportional to bin content.
  <li> <tt>"GLBOX1":</tt> the same as "glbox", but spheres are drawn instead of boxes.
  </ul>

<p><li><b><tt>TH3</tt> as iso-surface(s) - (<tt>TGLIsoPainter</tt>)</b>
  <br> The supported option is:
  <ul>
  <li> <tt>"GLISO" :</tt> TH3 is drawn using iso-surfaces.
  </ul>


<p><li><b><tt>TF3</tt> (implicit function) - (<tt>TGLTF3Painter</tt>)</b>
  <br> The supported option is:
  <ul>
  <li> <tt>"GLTF3" :</tt> Draw a <tt>TF3</tt>.
  </ul>

<p><li><b>Parametric surfaces - (<tt>TGLParametricPlot</tt>)</b>
  <br><tt>$ROOTSYS/tutorials/gl/glparametric.C</tt> shows how to create parametric equations and
  visualize the surface.
</ul>

<h3>Interaction with the plots</h3>

<ul>
<p><li><b>General information.</b>
  <br>
  All the interactions are implemented via standard methods <tt>DistancetoPrimitive</tt> and
  <tt>ExecuteEvent</tt>. That's why all the interactions with the OpenGL plots are possible i
  only when the mouse cursor is in the plot's area (the plot's area is the part of a the pad
  occupied by gl-produced picture). If the mouse cursor is not above gl-picture,
  the standard pad interaction is performed.

<p><li><b>Selectable parts.</b>
  <br>
  Different parts of the plot can be selected:
  <ul>
  <li> <em>xoz, yoz, xoy back planes</em>:
     <br>When such a plane selected, it's highlighted in green if the dynamic slicing
     by this plane is supported, and it's highlighted in red, if the dynamic slicing
     is not supported.
  <li><em>The plot itself</em>:
     <br>On surfaces, the selected surface is outlined in red. (TF3 and ISO are not
     outlined). On lego plots, the selected bin is highlihted. The bin number and content are displayed in pad's status
     bar. In box plots, the box or sphere is highlighted and the bin info is displayed in pad's status bar.
  </ul>

<p><li><b>Rotation and zooming.</b>
  <br>
  <ul>
  <li> <em>Rotation</em>:
  <br>
  When the plot is selected, it can be rotated by pressing and holding the left mouse button and move the cursor.
  <li> <em>Zoom/Unzoom</em>:
  <br>
  Mouse wheel or <tt>'j'</tt>, <tt>'J'</tt>, <tt>'k'</tt>, <tt>'K'</tt> keys.
  </ul>

<p><li><b>Panning.</b>
 <br>
  The selected plot can be moved in a pad's area by
  pressing and holding the left mouse button and the shift key.
</ul>

<h3>Box cut</h3>
  Surface, iso, box, TF3 and parametric painters support box cut by pressing the <tt>'c'</tt> or
  <tt>'C'</tt> key when the mouse cursor is in a plot's area. That will display a transparent box,
  cutting away part of the surface (or boxes) in order to show internal part of plot.
  This box can be moved inside the plot's area (the full size of the box is equal to the plot's
  surrounding box) by selecting one of the box cut axes and pressing the left mouse button to move it.

<h3>Plot specific interactions (dynamic slicing etc.)</h3>
  Currently, all gl-plots support some form of slicing.
  When back plane is selected (and if it's highlighted in green)
  you can press and hold left mouse button and shift key
  and move this back plane inside plot's area, creating the slice.
  During this "slicing" plot becomes semi-transparent. To remove all slices (and projected curves for surfaces)
  - double click with left mouse button in a plot's area.
  <ul>
  <p><li><b>Surface with option <tt>"GLSURF"</tt></b>
  <br>
  The surface profile is displayed on the slicing plane.
  The profile projection is drawn on the back plane
  by pressing <tt>'p'</tt> or <tt>'P'</tt> key.

  <p><li><b>TF3</b>
  <br>
  The contour plot is drawn on the slicing plane.
  For <tt>TF3</tt> the color scheme can be changed by pressing <tt>'s'</tt> or <tt>'S'</tt>.

  <p><li><b>Box</b>
  <br>
  The contour plot corresponding to slice plane position is drawn in real time.

  <p><li><b>Iso</b>
  <br>
  Slicing is similar to <tt>"GLBOX"</tt> option.

  <p><li><b>Parametric plot</b>
  <br>
  No slicing. Additional keys: <tt>'s'</tt> or <tt>'S'</tt> to change color scheme - about 20 color schemes supported
  (<tt>'s'</tt> for "scheme"); <tt>'l'</tt> or <tt>'L'</tt> to increase number of polygons (<tt>'l'</tt> for "level" of details),
  <tt>'w'</tt> or <tt>'W'</tt> to show outlines (<tt>'w'</tt> for "wireframe").
  </ul>
End_Html */

//______________________________________________________________________________
TGLHistPainter::TGLHistPainter(TH1 *hist)
                   : fDefaultPainter(TVirtualHistPainter::HistPainter(hist)),
                     fEq(0),
                     fHist(hist),
                     fF3(0),
                     fStack(0),
                     fPlotType(kGLDefaultPlot)//THistPainter
{
   //ROOT does not use exceptions, so, if default painter's creation failed,
   //fDefaultPainter is 0. In each function, which use it, I have to check the pointer first.
}

//______________________________________________________________________________
TGLHistPainter::TGLHistPainter(TGLParametricEquation *equation)
                   : fEq(equation),
                     fHist(0),
                     fF3(0),
                     fStack(0),
                     fPlotType(kGLParametricPlot)//THistPainter
{
   //This ctor creates gl-parametric plot's painter.
   fGLPainter.reset(new TGLParametricPlot(equation, &fCamera));
}

//______________________________________________________________________________
Int_t TGLHistPainter::DistancetoPrimitive(Int_t px, Int_t py)
{
   //Selects plot or axis.
   //9999 is the magic number, ROOT's classes use in DistancetoPrimitive.
   
   //[tp: return statement added.
   //tp]
   
   if (fPlotType == kGLDefaultPlot)
      return fDefaultPainter.get() ? fDefaultPainter->DistancetoPrimitive(px, py) : 9999;
   else {
      //return 9999;
      //Adjust px and py - canvas can have several pads inside, so we need to convert
      //the from canvas' system into pad's.
      py -= Int_t((1 - gPad->GetHNDC() - gPad->GetYlowNDC()) * gPad->GetWh());
      px -= Int_t(gPad->GetXlowNDC() * gPad->GetWw());
      //One hist can be appended to several pads,
      //the current pad should have valid OpenGL context.
      const Int_t glContext = gPad->GetGLDevice();

      if (glContext != -1) {
         //Add "viewport" extraction here.
         PadToViewport(kTRUE);

         if (!gGLManager->PlotSelected(fGLPainter.get(), px, py))
            gPad->SetSelected(gPad);
      } else {
         Error("DistancetoPrimitive",
               "Attempt to use TGLHistPainter, while the current pad (gPad) does not support gl");
         gPad->SetSelected(gPad);
      }

      //gPad->GetCanvas()->Update();
      
      return 0;
   }
}

//______________________________________________________________________________
void TGLHistPainter::DrawPanel()
{
   //Default implementation is OK
   //This function is called from a context menu
   //after right click on a plot's area. Opens window
   //("panel") with several controls.
   if (fDefaultPainter.get())
      fDefaultPainter->DrawPanel();
}

//______________________________________________________________________________
void TGLHistPainter::ExecuteEvent(Int_t event, Int_t px, Int_t py)
{
   //Execute event.
   //Events are: mouse events in a plot's area,
   //key presses (while mouse cursor is in plot's area).
   //"Event execution" means one of the following actions:
   //1. Rotation.
   //2. Panning.
   //3. Zoom changing.
   //4. Moving dynamic profile.
   //5. Plot specific events - for example, 's' or 'S' key press for TF3.
   if (fPlotType == kGLDefaultPlot) {
      if(fDefaultPainter.get()) {
         fDefaultPainter->ExecuteEvent(event, px, py);
      }
   } else {
      //One hist can be appended to several pads,
      //the current pad should have valid OpenGL context.
      const Int_t glContext = gPad->GetGLDevice();

      if (glContext == -1) {
         Error("ExecuteEvent",
               "Attempt to use TGLHistPainter, while the current pad (gPad) does not support gl");
         return;
      } else {
         //Add viewport extraction here.
         /*fGLDevice.SetGLDevice(glContext);
         fGLPainter->SetGLDevice(&fGLDevice);*/
         PadToViewport();
      }

      if (event != kKeyPress) {
         //Adjust px and py - canvas can have several pads inside, so we need to convert
         //the from canvas' system into pad's. If it was a key press event,
         //px and py ARE NOT coordinates.
         py -= Int_t((1 - gPad->GetHNDC() - gPad->GetYlowNDC()) * gPad->GetWh());
         px -= Int_t(gPad->GetXlowNDC() * gPad->GetWw());
      }

      switch (event) {
      case kButton1Double:
         //Left double click removes dynamic sections, user created (if plot type supports sections).
         fGLPainter->ProcessEvent(event, px, py);
         break;
      case kButton1Down:
         //Left mouse down in a plot area starts rotation.
         if (!fGLPainter->CutAxisSelected())
            fCamera.StartRotation(px, py);
         else
            fGLPainter->StartPan(px, py);
         //During rotation, usual TCanvas/TPad machinery (CopyPixmap/Flush/UpdateWindow/etc.)
         //is skipped - I use "bit blasting" functions to copy picture directly onto window.
         //gGLManager->MarkForDirectCopy(glContext, kTRUE);
         break;
      case kButton1Motion:
         //Rotation invalidates "selection buffer"
         // - (color-to-object map, previously read from gl-buffer).
         fGLPainter->InvalidateSelection();
         if (fGLPainter->CutAxisSelected())
            gGLManager->PanObject(fGLPainter.get(), px, py);
         else
            fCamera.RotateCamera(px, py);
         //Draw modified scene onto canvas' window.
         //gGLManager->PaintSingleObject(fGLPainter.get());
         gPad->GetCanvas()->Flush();
         break;
      case kButton1Up:
      case kButton2Up:
         gGLManager->MarkForDirectCopy(glContext, kFALSE);
         break;
      case kMouseMotion:
         gPad->SetCursor(kRotate);
         break;
      case 7://kButton1Down + shift modifier
         //The current version of ROOT does not
         //have enumerators for button events + key modifiers,
         //so I use hardcoded literals. :(
         //With left mouse button down and shift pressed
         //we can move plot as the whole or move
         //plot's parts - dynamic sections.
         fGLPainter->StartPan(px, py);
         gGLManager->MarkForDirectCopy(glContext, kTRUE);
         break;
      case 8://kButton1Motion + shift modifier
         gGLManager->PanObject(fGLPainter.get(), px, py);
         //gGLManager->PaintSingleObject(fGLPainter.get());
         gPad->GetCanvas()->Flush();
         break;
      case kKeyPress:
      case 5:
      case 6:
         //5, 6 are mouse wheel events (see comment about literals above).
         //'p'/'P' - specific events processed by TGLSurfacePainter,
         //'s'/'S' - specific events processed by TGLTF3Painter,
         //'c'/'C' - turn on/off box cut.
         gGLManager->MarkForDirectCopy(glContext, kTRUE);
         if (event == 6 || py == kKey_J || py == kKey_j) {
            fCamera.ZoomIn();
            fGLPainter->InvalidateSelection();
            //gGLManager->PaintSingleObject(fGLPainter.get());
            gPad->GetCanvas()->Flush();
         } else if (event == 5 || py == kKey_K || py == kKey_k) {
            fCamera.ZoomOut();
            fGLPainter->InvalidateSelection();
            //gGLManager->PaintSingleObject(fGLPainter.get());
            gPad->GetCanvas()->Flush();
         } else if (py == kKey_p || py == kKey_P || py == kKey_S || py == kKey_s
                    || py == kKey_c || py == kKey_C || py == kKey_x || py == kKey_X
                    || py == kKey_y || py == kKey_Y || py == kKey_z || py == kKey_Z
                    || py == kKey_w || py == kKey_W || py == kKey_l || py == kKey_L)
         {
            fGLPainter->ProcessEvent(event, px, py);
            //gGLManager->PaintSingleObject(fGLPainter.get());
            gPad->GetCanvas()->Flush();
         }
         gGLManager->MarkForDirectCopy(glContext, kFALSE);
         break;
      }
   }
}

//______________________________________________________________________________
TList *TGLHistPainter::GetContourList(Double_t contour)const
{
   //Get contour list.
   //I do not use this function. Contours are implemented in
   //a completely different way by gl-painters.
   return fDefaultPainter.get() ? fDefaultPainter->GetContourList(contour) : 0;
}

//______________________________________________________________________________
char *TGLHistPainter::GetObjectInfo(Int_t px, Int_t py)const
{
   //Overrides TObject::GetObjectInfo.
   //For lego info is: bin numbers (i, j), bin content.
   //For TF2 info is: x,y,z 3d surface-point for 2d screen-point under cursor
   //(this can work incorrectly now, because of wrong code in TF2).
   //For TF3 no info now.
   //For box info is: bin numbers (i, j, k), bin content.
   static char errMsg[] = { "TGLHistPainter::GetObjectInfo: Error in a hist painter\n" };
   if (fPlotType == kGLDefaultPlot)
      return fDefaultPainter.get() ? fDefaultPainter->GetObjectInfo(px, py)
                                   : errMsg;
   else
      return gGLManager->GetPlotInfo(fGLPainter.get(), px, py);
}

//______________________________________________________________________________
TList *TGLHistPainter::GetStack()const
{
   // Get stack.
   return fStack;
}

//______________________________________________________________________________
Bool_t TGLHistPainter::IsInside(Int_t x, Int_t y)
{
   //Returns kTRUE if the cell ix, iy is inside one of the graphical cuts.
   //I do not use this function anywhere, this is a "default implementation".
   if (fPlotType == kGLDefaultPlot)
      return fDefaultPainter.get() ? fDefaultPainter->IsInside(x, y) : kFALSE;

   return kFALSE;
}

//______________________________________________________________________________
Bool_t TGLHistPainter::IsInside(Double_t x, Double_t y)
{
   //Returns kTRUE if the cell x, y is inside one of the graphical cuts.
   //I do not use this function anywhere, this is a "default implementation".
   if (fPlotType == kGLDefaultPlot)
      return fDefaultPainter.get() ? fDefaultPainter->IsInside(x, y) : kFALSE;

   return kFALSE;
}

//______________________________________________________________________________
void TGLHistPainter::PaintStat(Int_t dostat, TF1 *fit)
{
   //Paint statistics.
   //This does not work on windows.
   if (fDefaultPainter.get())
      fDefaultPainter->PaintStat(dostat, fit);
}

//______________________________________________________________________________
void TGLHistPainter::ProcessMessage(const char *m, const TObject *o)
{
   // Process message.
   if (!std::strcmp(m, "SetF3"))
      fF3 = (TF3 *)o;

   if (fDefaultPainter.get())
      fDefaultPainter->ProcessMessage(m, o);
}

//______________________________________________________________________________
void TGLHistPainter::SetHistogram(TH1 *h)
{
   // Set histogram.
   fHist = h;

   if (fDefaultPainter.get())
      fDefaultPainter->SetHistogram(h);
}

//______________________________________________________________________________
void TGLHistPainter::SetStack(TList *s)
{
   // Set stack.
   fStack = s;

   if (fDefaultPainter.get())
      fDefaultPainter->SetStack(s);
}

//______________________________________________________________________________
Int_t TGLHistPainter::MakeCuts(char *o)
{
   // Make cuts.
   if (fPlotType == kGLDefaultPlot && fDefaultPainter.get())
      return fDefaultPainter->MakeCuts(o);

   return 0;
}

struct TGLHistPainter::PlotOption_t {
   EGLPlotType  fPlotType;
   EGLCoordType fCoordType;
   Bool_t       fBackBox;
   Bool_t       fFrontBox;
   Bool_t       fLogX;
   Bool_t       fLogY;
   Bool_t       fLogZ;
};

//______________________________________________________________________________
void TGLHistPainter::Paint(Option_t *o)
{
   //Final-overrider for TObject::Paint.
   TString option(o);
   option.ToLower();

   const Ssiz_t glPos = option.Index("gl");
   if (glPos != kNPOS)
      option.Remove(glPos, 2);
   else if (fPlotType != kGLParametricPlot){
      gPad->SetCopyGLDevice(kFALSE);
      if (fDefaultPainter.get())
         fDefaultPainter->Paint(o);//option.Data());
      return;
   }

   if (fPlotType != kGLParametricPlot)
      CreatePainter(ParsePaintOption(option), option);

   if (fPlotType == kGLDefaultPlot) {
      //In case of default plot pad
      //should not copy gl-buffer (it will be simply black)
      
      //[tp: code was commented.
      //gPad->SetCopyGLDevice(kFALSE);
      //tp]

      if (fDefaultPainter.get())
         fDefaultPainter->Paint(option.Data());
   } else {
      Int_t glContext = gPad->GetGLDevice();

      if (glContext != -1) {
         //With gl-plot, pad should copy
         //gl-buffer into the final pad/canvas pixmap/DIB.
         //fGLDevice.SetGLDevice(glContext);
         
         //[tp: code commented.
         //gPad->SetCopyGLDevice(kTRUE);
         //tp]
         //fGLPainter->SetGLDevice(&fGLDevice);
         //Add viewport extraction here.
         PadToViewport();
         
         if (gPad->GetFrameFillColor() != kWhite)
            fGLPainter->SetFrameColor(gROOT->GetColor(gPad->GetFrameFillColor()));
         fGLPainter->SetPadColor(gROOT->GetColor(gPad->GetFillColor()));
         if (fGLPainter->InitGeometry())
            gGLManager->PaintSingleObject(fGLPainter.get());
      }
   }
}

//______________________________________________________________________________
TGLHistPainter::PlotOption_t
TGLHistPainter::ParsePaintOption(const TString &option)const
{
   //In principle, we can have several conflicting options: "lego surf pol sph",
   //but only one will be selected, which one - depends on parsing order in this function.
   PlotOption_t parsedOption = {kGLDefaultPlot, kGLCartesian, kFALSE, kFALSE, gPad->GetLogx(),
                                gPad->GetLogy(), gPad->GetLogz()};
   //Check coordinate system type.
   if (option.Index("pol") != kNPOS)
      parsedOption.fCoordType = kGLPolar;
   if (option.Index("cyl") != kNPOS)
      parsedOption.fCoordType = kGLCylindrical;
   if (option.Index("sph") != kNPOS)
      parsedOption.fCoordType = kGLSpherical;
   //Define plot type
   if (option.Index("lego") != kNPOS)
      fStack ? parsedOption.fPlotType = kGLStackPlot : parsedOption.fPlotType = kGLLegoPlot;
   if (option.Index("surf") != kNPOS)
      parsedOption.fPlotType = kGLSurfacePlot;
   if (option.Index("tf3") != kNPOS)
      parsedOption.fPlotType = kGLTF3Plot;
   if (option.Index("box") != kNPOS)
      parsedOption.fPlotType = kGLBoxPlot;
   if (option.Index("iso") != kNPOS)
      parsedOption.fPlotType = kGLIsoPlot;


   return parsedOption;
}

//______________________________________________________________________________
void TGLHistPainter::CreatePainter(const PlotOption_t &option, const TString &addOption)
{
   // Create painter.
   if (option.fPlotType != fPlotType) {
      fCoord.ResetModified();
      fGLPainter.reset(0);
   }

   if (option.fPlotType == kGLLegoPlot) {
      if (!fGLPainter.get())
         fGLPainter.reset(new TGLLegoPainter(fHist, &fCamera, &fCoord));
   } else if (option.fPlotType == kGLSurfacePlot) {
      if (!fGLPainter.get())
         fGLPainter.reset(new TGLSurfacePainter(fHist, &fCamera, &fCoord));
   } else if (option.fPlotType == kGLBoxPlot) {
      if (!fGLPainter.get())
         fGLPainter.reset(new TGLBoxPainter(fHist, &fCamera, &fCoord));
   } else if (option.fPlotType == kGLTF3Plot) {
      if (!fGLPainter.get())
         fGLPainter.reset(new TGLTF3Painter(fF3, fHist, &fCamera, &fCoord));
   } else if (option.fPlotType == kGLIsoPlot) {
      if (!fGLPainter.get())
         fGLPainter.reset(new TGLIsoPainter(fHist, &fCamera, &fCoord));
   }

   if (fGLPainter.get()) {
      fPlotType = option.fPlotType;
      fCoord.SetXLog(gPad->GetLogx());
      fCoord.SetYLog(gPad->GetLogy());
      fCoord.SetZLog(gPad->GetLogz());
      fCoord.SetCoordType(option.fCoordType);
      fGLPainter->AddOption(addOption);
   } else
      fPlotType = kGLDefaultPlot;
}

//______________________________________________________________________________
void TGLHistPainter::SetShowProjection(const char *, Int_t)
{
   // Set show projection.
}

//______________________________________________________________________________
void TGLHistPainter::PadToViewport(Bool_t selectionPass)
{
   if (!fGLPainter.get())
      return;
      
   if (const TPad *pad = dynamic_cast<TPad *>(gPad)) {
      TGLRect vp;
      //const Int_t borderSize = pad->GetBorderSize() > 0 ? pad->GetBorderSize() : 2;   
      vp.Width()  = pad->GetAbsWNDC() * pad->GetWw();//TMath::Abs(pad->XtoPixel(pad->GetX2()) - pad->XtoPixel(pad->GetX1()));// - 2 * borderSize;
      vp.Height() = pad->GetAbsHNDC() * pad->GetWh();//TMath::Abs(pad->YtoPixel(pad->GetY2()) - pad->YtoPixel(pad->GetY1()));// - 2 * borderSize;
      if (!selectionPass) {
         pad->XYtoAbsPixel(pad->GetX1(), pad->GetY1(), vp.X(), vp.Y());
         //vp.X() += borderSize;
         vp.Y() = pad->GetCanvas()->GetWh() - vp.Y();// + borderSize;
      }
      fCamera.SetViewport(vp);
   }
}
