// @(#)root/base:$Name$:$Id$
// Author: Rene Brun   12/12/94

/*************************************************************************
 * Copyright (C) 1995-2000, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#ifndef ROOT_TStyle
#define ROOT_TStyle


//////////////////////////////////////////////////////////////////////////
//                                                                      //
// TStyle                                                               //
//                                                                      //
// A collection of all graphics attributes.                             //
//                                                                      //
//////////////////////////////////////////////////////////////////////////


#ifndef ROOT_TNamed
#include "TNamed.h"
#endif
#ifndef ROOT_TAttAxis
#include "TAttAxis.h"
#endif
#ifndef ROOT_TAttLine
#include "TAttLine.h"
#endif
#ifndef ROOT_TAttFill
#include "TAttFill.h"
#endif
#ifndef ROOT_TAttText
#include "TAttText.h"
#endif
#ifndef ROOT_TAttMarker
#include "TAttMarker.h"
#endif
#ifndef ROOT_TArrayI
#include "TArrayI.h"
#endif

class TBrowser;

class TStyle : public TNamed, public TAttLine, public TAttFill, public TAttMarker, public TAttText {

private:
        TAttAxis      fXaxis;             //X axis attributes
        TAttAxis      fYaxis;             //Y axis attributes
        TAttAxis      fZaxis;             //Z axis attributes
        Float_t       fBarWidth;          //width of bar for graphs
        Float_t       fBarOffset;         //offset of bar for graphs
        Int_t         fDrawBorder;        //flag to draw border(=1) or not (0)
        Int_t         fOptLogx;           //=1 if log scale in X
        Int_t         fOptLogy;           //=1 if log scale in y
        Int_t         fOptLogz;           //=1 if log scale in z
        Int_t         fOptDate;           //=1 if date option is selected
        Int_t         fOptStat;           //=1 if option Stat is selected
        Int_t         fOptTitle;          //=1 if option Title is selected
        Int_t         fOptFile;           //=1 if option File is selected
        Int_t         fOptFit;            //=1 if option Fit is selected
        Int_t         fErrorMarker;       //marker for error bars
        Int_t         fShowEventStatus;   //Show event status panel
        TAttText      fAttDate;           //canvas date attribute
        Float_t       fDateX;             //X position of the date in the canvas 9in NDC)
        Float_t       fDateY;             //Y position of the date in the canvas 9in NDC)
        Float_t       fErrorMsize;        //marker size for error bars
        Float_t       fErrorX;            //per cent of bin width for errors along X
        Color_t       fFuncColor;         //function color
        Style_t       fFuncStyle;         //function style
        Width_t       fFuncWidth;         //function line width
        Color_t       fFrameFillColor;    //pad frame fill color
        Color_t       fFrameLineColor;    //pad frame line color
        Style_t       fFrameFillStyle;    //pad frame fill style
        Style_t       fFrameLineStyle;    //pad frame line style
        Width_t       fFrameLineWidth;    //pad frame line width
        Width_t       fFrameBorderSize;   //pad frame border size
        Int_t         fFrameBorderMode;   //pad frame border mode
        Color_t       fHistFillColor;     //histogram fill color
        Color_t       fHistLineColor;     //histogram line color
        Style_t       fHistFillStyle;     //histogram fill style
        Style_t       fHistLineStyle;     //histogram line style
        Width_t       fHistLineWidth;     //histogram line width
        Color_t       fCanvasColor;       //canvas color
        Width_t       fCanvasBorderSize;  //canvas border size
        Int_t         fCanvasBorderMode;  //canvas border mode
        Int_t         fCanvasDefH;        //default canvas height
        Int_t         fCanvasDefW;        //default canvas width
        Int_t         fCanvasDefX;        //default canvas top X position
        Int_t         fCanvasDefY;        //default canvas top Y position
        Color_t       fPadColor;          //pad color
        Width_t       fPadBorderSize;     //pad border size
        Int_t         fPadBorderMode;     //pad border mode
        Float_t       fPadBottomMargin;   //pad bottom margin
        Float_t       fPadTopMargin;      //pad top margin
        Float_t       fPadLeftMargin;     //pad left margin
        Float_t       fPadRightMargin;    //pad right margin
        Bool_t        fPadGridX;          //true to get the grid along X
        Bool_t        fPadGridY;          //true to get the grid along Y
        Int_t         fPadTickX;          //=1 to set special pad ticks along X
        Int_t         fPadTickY;          //=1 to set special pad ticks along Y
        Float_t       fPaperSizeX;        //PostScript paper size along X
        Float_t       fPaperSizeY;        //PostScript paper size along Y
        Float_t       fScreenFactor;      //Multiplication factor for canvas size and position
        Color_t       fStatColor;         //stat fill area color
        Color_t       fStatTextColor;     //stat text color
        Width_t       fStatBorderSize;    //border size of Stats PaveLabel
        Style_t       fStatFont;          //font style of Stats PaveLabel
        Style_t       fStatStyle;         //fill area style of Stats PaveLabel
        TString       fStatFormat;        //Printing format for stats
        Float_t       fStatX;             //X position of top right corner of stat box
        Float_t       fStatY;             //Y position of top right corner of stat box
        Float_t       fStatW;             //width of stat box
        Float_t       fStatH;             //height of stat box
        Color_t       fTitleColor;        //title fill area color
        Color_t       fTitleTextColor;    //title text color
        Width_t       fTitleBorderSize;   //border size of Title PavelLabel
        Style_t       fTitleFont;         //font style of Title PaveLabel
        Style_t       fTitleStyle;        //fill area style of title PaveLabel
        Float_t       fTitleX;            //X position of top left corner of title box
        Float_t       fTitleY;            //Y position of top left corner of title box
        Float_t       fTitleW;            //width of title box
        Float_t       fTitleH;            //height of title box
        Float_t       fLegoInnerR;        //Inner radius for cylindrical legos
        TArrayI       fPalette;           //Color palette
        TString       fLineStyle[30];     //String describing line style i (for postScript)
        TString       fHeaderPS;          //User defined additional Postscript header
        TString       fFitFormat;         //Printing format for fit parameters
        Float_t       fLineScalePS;       //Line scale factor when drawing lines on Postscript
        Double_t      fTimeOffset;        //Time offset (UTC) to the beginning of an axis

public:
        enum EPaperSize { kA4, kUSLetter };

        TStyle();
        TStyle(const char *name, const char *title);
        TStyle(const TStyle &style);
        virtual          ~TStyle();
        Int_t            AxisChoice(Option_t *axis);
        virtual void     Browse(TBrowser *b);
        static  void     BuildStyles();
        virtual void     Copy(TObject &style);
        virtual void     cd();

        Int_t            GetNdivisions(Option_t *axis="X");
        TAttText        *GetAttDate() {return &fAttDate;}
        Color_t          GetAxisColor(Option_t *axis="X");
        Color_t          GetLabelColor(Option_t *axis="X");
        Style_t          GetLabelFont(Option_t *axis="X");
        Float_t          GetLabelOffset(Option_t *axis="X");
        Float_t          GetLabelSize(Option_t *axis="X");
        Float_t          GetTitleOffset(Option_t *axis="X"); //return axis title offset
        Float_t          GetTitleSize(Option_t *axis="X");   //return axis title size
        Float_t          GetTickLength(Option_t *axis="X");

        Float_t          GetBarOffset() {return fBarOffset;}
        Float_t          GetBarWidth() {return fBarWidth;}
        Int_t            GetDrawBorder() {return fDrawBorder;}
        Int_t            GetErrorMarker() {return fErrorMarker;}
        Float_t          GetErrorMsize() {return fErrorMsize;}
        Float_t          GetErrorX() {return fErrorX;}
        Color_t          GetCanvasColor() {return fCanvasColor;}
        Width_t          GetCanvasBorderSize() {return fCanvasBorderSize;}
        Int_t            GetCanvasBorderMode() {return fCanvasBorderMode;}
        Int_t            GetCanvasDefH()      {return fCanvasDefH;}
        Int_t            GetCanvasDefW()      {return fCanvasDefW;}
        Int_t            GetCanvasDefX()      {return fCanvasDefX;}
        Int_t            GetCanvasDefY()      {return fCanvasDefY;}
        Int_t            GetColorPalette(Int_t i);
        Float_t          GetDateX()           {return fDateX;}
        Float_t          GetDateY()           {return fDateY;}
        const char      *GetFitFormat()       const {return fFitFormat.Data();}
        Int_t            GetNumberOfColors()  {return fPalette.fN;}
        Color_t          GetPadColor()        {return fPadColor;}
        Width_t          GetPadBorderSize()   {return fPadBorderSize;}
        Int_t            GetPadBorderMode()   {return fPadBorderMode;}
        Float_t          GetPadBottomMargin() {return fPadBottomMargin;}
        Float_t          GetPadTopMargin()    {return fPadTopMargin;}
        Float_t          GetPadLeftMargin()   {return fPadLeftMargin;}
        Float_t          GetPadRightMargin()  {return fPadRightMargin;}
        Bool_t           GetPadGridX()        {return fPadGridX;}
        Bool_t           GetPadGridY()        {return fPadGridY;}
        Int_t            GetPadTickX()        {return fPadTickX;}
        Int_t            GetPadTickY()        {return fPadTickY;}
        Color_t          GetFuncColor()       {return fFuncColor;}
        Style_t          GetFuncStyle()       {return fFuncStyle;}
        Width_t          GetFuncWidth()       {return fFuncWidth;}
        Color_t          GetFrameFillColor() {return fFrameFillColor;}
        Color_t          GetFrameLineColor() {return fFrameLineColor;}
        Style_t          GetFrameFillStyle() {return fFrameFillStyle;}
        Style_t          GetFrameLineStyle() {return fFrameLineStyle;}
        Width_t          GetFrameLineWidth() {return fFrameLineWidth;}
        Width_t          GetFrameBorderSize() {return fFrameBorderSize;}
        Int_t            GetFrameBorderMode() {return fFrameBorderMode;}
        Color_t          GetHistFillColor() {return fHistFillColor;}
        Color_t          GetHistLineColor() {return fHistLineColor;}
        Style_t          GetHistFillStyle() {return fHistFillStyle;}
        Style_t          GetHistLineStyle() {return fHistLineStyle;}
        Width_t          GetHistLineWidth() {return fHistLineWidth;}
        Float_t          GetLegoInnerR() {return fLegoInnerR;}
        Int_t            GetOptDate() {return fOptDate;}
        Int_t            GetOptFile() {return fOptFile;}
        Int_t            GetOptFit() {return fOptFit;}
        Int_t            GetOptStat() {return fOptStat;}
        Int_t            GetOptTitle() {return fOptTitle;}
        Int_t            GetOptLogx() {return fOptLogx;}
        Int_t            GetOptLogy() {return fOptLogy;}
        Int_t            GetOptLogz() {return fOptLogz;}
        void             GetPaperSize(Float_t &xsize, Float_t &ysize);
        Int_t            GetShowEventStatus(){return fShowEventStatus;}
        Float_t          GetScreenFactor() {return fScreenFactor;}
        Color_t          GetStatColor() {return fStatColor;}
        Color_t          GetStatTextColor() {return fStatTextColor;}
        Width_t          GetStatBorderSize() {return fStatBorderSize;}
        Style_t          GetStatFont()  {return fStatFont;}
        Style_t          GetStatStyle()  {return fStatStyle;}
        const char      *GetStatFormat() const {return fStatFormat.Data();}
        Float_t          GetStatX()     {return fStatX;}
        Float_t          GetStatY()     {return fStatY;}
        Float_t          GetStatW()     {return fStatW;}
        Float_t          GetStatH()     {return fStatH;}
        Double_t         GetTimeOffset() {return fTimeOffset;} //return axis time offset
        Color_t          GetTitleColor() {return fTitleColor;}  //return histogram title fill area color
        Color_t          GetTitleTextColor() {return fTitleTextColor;}  //return histogram title text color
        Style_t          GetTitleStyle()  {return fTitleStyle;}
        Style_t          GetTitleFont()  {return fTitleFont;} //return histogram title font
        Width_t          GetTitleBorderSize() {return fTitleBorderSize;} //return border size of histogram title TPaveLabel
        Float_t          GetTitleXOffset() {return GetTitleOffset("X");} //return X axis title offset
        Float_t          GetTitleXSize()   {return GetTitleSize("X");}   //return X axis title size
        Float_t          GetTitleYOffset() {return GetTitleOffset("Y");} //return Y axis title offset
        Float_t          GetTitleYSize()   {return GetTitleSize("Y");}   //return Y axis title size
        Float_t          GetTitleX()     {return fTitleX;}  //return left X position of histogram title TPavelabel
        Float_t          GetTitleY()     {return fTitleY;}  //return left bottom position of histogram title TPavelabel
        Float_t          GetTitleW()     {return fTitleW;}  //return width of histogram title TPaveLabel
        Float_t          GetTitleH()     {return fTitleH;}  //return height of histogram title TPavelabel
        const char      *GetHeaderPS() const {return fHeaderPS.Data();}
        const char      *GetLineStyleString(Int_t i=1) const;
        Float_t          GetLineScalePS() {return fLineScalePS;}
        virtual void     Reset(Option_t *option="");

        void             SetFitFormat(const char *format="5.4g") {fFitFormat = format;}
        void             SetHeaderPS(const char *header);
        void             SetLineScalePS(Float_t scale=3) {fLineScalePS=scale;}
        void             SetLineStyleString(Int_t i, const char *text);
        void             SetNdivisions(Int_t n=510, Option_t *axis="X");
        void             SetAxisColor(Color_t color=1, Option_t *axis="X");
        void             SetLabelColor(Color_t color=1, Option_t *axis="X");
        void             SetLabelFont(Style_t font=62, Option_t *axis="X");
        void             SetLabelOffset(Float_t offset=0.005, Option_t *axis="X");
        void             SetLabelSize(Float_t size=0.04, Option_t *axis="X");
        void             SetLegoInnerR(Float_t rad=0.5) {fLegoInnerR = rad;}
        void             SetScreenFactor(Float_t factor=1) {fScreenFactor = factor;}
        void             SetTickLength(Float_t length=0.03, Option_t *axis="X");
        void             SetTitleOffset(Float_t offset=1, Option_t *axis="X"); //set axis title offset
        void             SetTitleSize(Float_t size=0.02, Option_t *axis="X");  //set axis title size
        void             SetOptDate(Int_t datefl=1);
        void             SetOptFile(Int_t file=1) {fOptFile = file;}
        void             SetOptFit(Int_t fit=1);
        void             SetOptLogx(Int_t logx=1) {fOptLogx = logx;}
        void             SetOptLogy(Int_t logy=1) {fOptLogy = logy;}
        void             SetOptLogz(Int_t logz=1) {fOptLogz = logz;}
        void             SetOptStat(Int_t stat=1);
        void             SetOptTitle(Int_t tit=1) {fOptTitle = tit;}
        void             SetBarOffset(Float_t baroff=0.5) {fBarOffset = baroff;}
        void             SetBarWidth(Float_t barwidth=0.5) {fBarWidth = barwidth;}
        void             SetDateX(Float_t x=0.01) {fDateX = x;}
        void             SetDateY(Float_t y=0.01) {fDateY = y;}
        void             SetErrorMarker(Int_t marker=21) {fErrorMarker = marker;}
        void             SetErrorMsize(Float_t msize=0.05) {fErrorMsize = msize;}
        void             SetErrorX(Float_t errorx=0.5) {fErrorX = errorx;}
        void             SetDrawBorder(Int_t drawborder=1) {fDrawBorder = drawborder;}
        void             SetCanvasColor(Color_t color=19) {fCanvasColor = color;}
        void             SetCanvasBorderSize(Width_t size=1) {fCanvasBorderSize = size;}
        void             SetCanvasBorderMode(Int_t mode=1) {fCanvasBorderMode = mode;}
        void             SetCanvasDefH(Int_t h=500) {fCanvasDefH = h;}
        void             SetCanvasDefW(Int_t w=700) {fCanvasDefW = w;}
        void             SetCanvasDefX(Int_t topx=10) {fCanvasDefX = topx;}
        void             SetCanvasDefY(Int_t topy=10) {fCanvasDefY = topy;}
        void             SetPadColor(Color_t color=19) {fPadColor = color;}
        void             SetPadBorderSize(Width_t size=1) {fPadBorderSize = size;}
        void             SetPadBorderMode(Int_t mode=1) {fPadBorderMode = mode;}
        void             SetPadBottomMargin(Float_t margin=0.1) {fPadBottomMargin=margin;}
        void             SetPadTopMargin(Float_t margin=0.1)    {fPadTopMargin=margin;}
        void             SetPadLeftMargin(Float_t margin=0.1)   {fPadLeftMargin=margin;}
        void             SetPadRightMargin(Float_t margin=0.1)  {fPadRightMargin=margin;}
        void             SetPadGridX(Bool_t gridx) {fPadGridX = gridx;}
        void             SetPadGridY(Bool_t gridy) {fPadGridY = gridy;}
        void             SetPadTickX(Int_t tickx)  {fPadTickX = tickx;}
        void             SetPadTickY(Int_t ticky)  {fPadTickY = ticky;}
        void             SetFuncStyle(Style_t style=1) {fFuncStyle = style;}
        void             SetFuncColor(Color_t color=1) {fFuncColor = color;}
        void             SetFuncWidth(Width_t width=4) {fFuncWidth = width;}
        void             SetFrameFillColor(Color_t color=1) {fFrameFillColor = color;}
        void             SetFrameLineColor(Color_t color=1) {fFrameLineColor = color;}
        void             SetFrameFillStyle(Style_t styl=0)  {fFrameFillStyle = styl;}
        void             SetFrameLineStyle(Style_t styl=0)  {fFrameLineStyle = styl;}
        void             SetFrameLineWidth(Width_t width=1) {fFrameLineWidth = width;}
        void             SetFrameBorderSize(Width_t size=1) {fFrameBorderSize = size;}
        void             SetFrameBorderMode(Int_t mode=1) {fFrameBorderMode = mode;}
        void             SetHistFillColor(Color_t color=1) {fHistFillColor = color;}
        void             SetHistLineColor(Color_t color=1) {fHistLineColor = color;}
        void             SetHistFillStyle(Style_t styl=0)  {fHistFillStyle = styl;}
        void             SetHistLineStyle(Style_t styl=0)  {fHistLineStyle = styl;}
        void             SetHistLineWidth(Width_t width=1) {fHistLineWidth = width;}
        void             SetPaperSize(EPaperSize size);
        void             SetPaperSize(Float_t xsize=20, Float_t ysize=26);
        void             SetStatColor(Int_t color=19) {fStatColor=color;}
        void             SetStatTextColor(Int_t color=1) {fStatTextColor=color;}
        void             SetStatStyle(Style_t style=1001) {fStatStyle=style;}
        void             SetStatBorderSize(Width_t size=2) {fStatBorderSize=size;}
        void             SetStatFont(Style_t font=62) {fStatFont=font;}
        void             SetStatFormat(const char *format="6.4g") {fStatFormat = format;}
        void             SetStatX(Float_t x=0)    {fStatX=x;}
        void             SetStatY(Float_t y=0)    {fStatY=y;}
        void             SetStatW(Float_t w=0.19) {fStatW=w;}
        void             SetStatH(Float_t h=0.1)  {fStatH=h;}
        void             SetTimeOffset(Double_t toffset);
        void             SetTitleColor(Int_t color=19)      {fTitleColor=color;}
        void             SetTitleTextColor(Int_t color=1)   {fTitleTextColor=color;}
        void             SetTitleStyle(Style_t style=1001)  {fTitleStyle=style;}
        void             SetTitleFont(Style_t font=62)      {fTitleFont=font;}
        void             SetTitleBorderSize(Width_t size=2) {fTitleBorderSize=size;}
        void             SetTitleXOffset(Float_t offset=1)  {SetTitleOffset(offset,"X");}
        void             SetTitleXSize(Float_t size=0.02)   {SetTitleSize(size,"X");}
        void             SetTitleYOffset(Float_t offset=1)  {SetTitleOffset(offset,"Y");}
        void             SetTitleYSize(Float_t size=0.02)   {SetTitleSize(size,"Y");}
        void             SetTitleX(Float_t x=0) {fTitleX=x;}
        void             SetTitleY(Float_t y=1) {fTitleY=y;}
        void             SetTitleW(Float_t w=0) {fTitleW=w;}
        void             SetTitleH(Float_t h=0) {fTitleH=h;}
        void             ToggleEventStatus() { fShowEventStatus = fShowEventStatus ? 0 : 1; }
        void             SetPalette(Int_t ncolors=0, Int_t *colors=0);

        ClassDef(TStyle,3)  //A collection of all graphics attributes
};


R__EXTERN TStyle  *gStyle;

#endif
