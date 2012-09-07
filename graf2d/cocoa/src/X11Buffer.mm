// @(#)root/graf2d:$Id$
// Author: Timur Pocheptsov   29/02/2012

/*************************************************************************
 * Copyright (C) 1995-2012, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

//#define NDEBUG

#include <algorithm>
#include <stdexcept>
#include <cstring>
#include <cassert>
#include <memory>

#include "ROOTOpenGLView.h"
#include "CocoaPrivate.h"
#include "QuartzWindow.h"
#include "QuartzPixmap.h"
#include "QuartzUtils.h"
#include "X11Drawable.h"
#include "X11Buffer.h"
#include "TGWindow.h"
#include "TGClient.h"
#include "TGCocoa.h"

namespace ROOT {
namespace MacOSX {
namespace X11 {

//______________________________________________________________________________
Command::Command(Drawable_t wid, const GCValues_t &gc)
            : fID(wid),
              fGC(gc)
{
}

//______________________________________________________________________________
Command::Command(Drawable_t wid)
            : fID(wid),
              fGC()
{
}

//______________________________________________________________________________
Command::~Command()
{
}

//______________________________________________________________________________
bool Command::HasOperand(Drawable_t wid)const
{
   return wid == fID;      
}

//______________________________________________________________________________
bool Command::IsGraphicsCommand()const
{
   return false;
}

//______________________________________________________________________________
DrawLine::DrawLine(Drawable_t wid, const GCValues_t &gc, const Point_t &p1, const Point_t &p2)
            : Command(wid, gc),
              fP1(p1),
              fP2(p2)
{
}

//______________________________________________________________________________
void DrawLine::Execute()const
{
   TGCocoa * const vx = dynamic_cast<TGCocoa *>(gVirtualX);
   assert(vx != 0 && "Execute, gVirtualX is either null or not of TGCocoa type");
   vx->DrawLineAux(fID, fGC, fP1.fX, fP1.fY, fP2.fX, fP2.fY);
}

//______________________________________________________________________________
DrawSegments::DrawSegments(Drawable_t wid, const GCValues_t &gc, const Segment_t *segments, Int_t nSegments)
                 : Command(wid, gc) 
{
   assert(segments != 0 && "DrawSegments, segments parameter is null");
   assert(nSegments > 0 && "DrawSegments, nSegments <= 0");
   
   fSegments.assign(segments, segments + nSegments);
}

//______________________________________________________________________________
void DrawSegments::Execute()const
{
   TGCocoa * const vx = dynamic_cast<TGCocoa *>(gVirtualX);
   assert(vx != 0 && "Execute, gVirtualX is either null or not of TGCocoa type");
   vx->DrawSegmentsAux(fID, fGC, &fSegments[0], (Int_t)fSegments.size());
}

//______________________________________________________________________________
ClearArea::ClearArea(Window_t wid, const Rectangle_t &area)
             : Command(wid),
               fArea(area)
{
}

//______________________________________________________________________________
void ClearArea::Execute()const
{
   TGCocoa * const vx = dynamic_cast<TGCocoa *>(gVirtualX);
   assert(vx != 0 && "Execute, gVirtualX is either null or not of TGCocoa type");
   vx->ClearAreaAux(fID, fArea.fX, fArea.fY, fArea.fWidth, fArea.fHeight);   
}

//______________________________________________________________________________
CopyArea::CopyArea(Drawable_t src, Drawable_t dst, const GCValues_t &gc, const Rectangle_t &area, const Point_t &dstPoint)
               : Command(dst, gc),
                 fSrc(src),
                 fArea(area),
                 fDstPoint(dstPoint)
{
}

//______________________________________________________________________________
bool CopyArea::HasOperand(Drawable_t drawable)const
{
   return fID == drawable || fSrc == drawable || fGC.fClipMask == drawable;
}

//______________________________________________________________________________
void CopyArea::Execute()const
{
   TGCocoa * const vx = dynamic_cast<TGCocoa *>(gVirtualX);
   assert(vx != 0 && "Execute, gVirtualX is either null or not of TGCocoa type");
   vx->CopyAreaAux(fSrc, fID, fGC, fArea.fX, fArea.fY, fArea.fWidth, fArea.fHeight, fDstPoint.fX, fDstPoint.fY);
}

//______________________________________________________________________________
DrawString::DrawString(Drawable_t wid, const GCValues_t &gc, const Point_t &point, const std::string &text)
               : Command(wid, gc),
                 fPoint(point),
                 fText(text)
{
}

//______________________________________________________________________________
void DrawString::Execute()const
{
   TGCocoa * const vx = dynamic_cast<TGCocoa *>(gVirtualX);
   assert(vx != 0 && "Execute, gVirtualX is either null or not of TGCocoa type");
   vx->DrawStringAux(fID, fGC, fPoint.fX, fPoint.fY, fText.c_str(), fText.length());
}

//______________________________________________________________________________
FillRectangle::FillRectangle(Drawable_t wid, const GCValues_t &gc, const Rectangle_t &rectangle)
                  : Command(wid, gc),
                    fRectangle(rectangle)
{
}

//______________________________________________________________________________
void FillRectangle::Execute()const
{
   TGCocoa * const vx = dynamic_cast<TGCocoa *>(gVirtualX);
   assert(vx != 0 && "Execute, gVirtualX is either null or not of TGCocoa type");
   vx->FillRectangleAux(fID, fGC, fRectangle.fX, fRectangle.fY, fRectangle.fWidth, fRectangle.fHeight);
}

//______________________________________________________________________________
FillPolygon::FillPolygon(Drawable_t wid, const GCValues_t &gc, const Point_t *points, Int_t nPoints)
                : Command(wid, gc)
{
   assert(points != 0 && "FillPolygon, points parameter is null");
   assert(nPoints > 0 && "FillPolygon, nPoints <= 0");
   
   fPolygon.assign(points, points + nPoints);
}

//______________________________________________________________________________   
void FillPolygon::Execute()const
{
   TGCocoa * const vx = dynamic_cast<TGCocoa *>(gVirtualX);
   assert(vx != 0 && "Execute, gVirtualX is either null or not of TGCocoa type");
   vx->FillPolygonAux(fID, fGC, &fPolygon[0], (Int_t)fPolygon.size());
}

//______________________________________________________________________________
DrawRectangle::DrawRectangle(Drawable_t wid, const GCValues_t &gc, const Rectangle_t &rectangle)
                 : Command(wid, gc),
                   fRectangle(rectangle)
{
}

//______________________________________________________________________________
void DrawRectangle::Execute()const
{
   TGCocoa * const vx = dynamic_cast<TGCocoa *>(gVirtualX);
   assert(vx != 0 && "Execute, gVirtualX is either null or not of TGCocoa type");
   vx->DrawRectangleAux(fID, fGC, fRectangle.fX, fRectangle.fY, fRectangle.fWidth, fRectangle.fHeight);
}

//______________________________________________________________________________
UpdateWindow::UpdateWindow(QuartzView *view)
                : Command(view.fID),
                  fView(view)
{
   assert(view != nil && "UpdateWindow, view parameter is nil");//view.fID will be also 0.
}

//______________________________________________________________________________
void UpdateWindow::Execute()const
{
   assert(fView.fContext != 0 && "Execute, view.fContext is null");

   if (QuartzPixmap *pixmap = fView.fBackBuffer) {
      Rectangle_t copyArea = {0, 0, pixmap.fWidth, pixmap.fHeight};
      [fView copy : pixmap area : copyArea withMask : nil clipOrigin : Point_t() toPoint : Point_t()];
   }
}

//______________________________________________________________________________
DeletePixmap::DeletePixmap(Pixmap_t pixmap)
                : Command(pixmap, GCValues_t())
{
}

//______________________________________________________________________________
void DeletePixmap::Execute()const
{
   TGCocoa * const vx = dynamic_cast<TGCocoa *>(gVirtualX);
   assert(vx != 0 && "Execute, gVirtualX is either null or not of TGCocoa type");
   vx->DeletePixmapAux(fID);
}

//______________________________________________________________________________
CommandBuffer::CommandBuffer()
{
}

//______________________________________________________________________________
CommandBuffer::~CommandBuffer()
{
   ClearCommands();
}

//______________________________________________________________________________
void CommandBuffer::AddDrawLine(Drawable_t wid, const GCValues_t &gc, Int_t x1, Int_t y1, Int_t x2, Int_t y2)
{
   try {
      Point_t p1 = {}; 
      //I'd use .fX = x1 from standard C, but ... this is already C++0x + Obj-C :)
      //So, not to make it worse :)
      p1.fX = x1;
      p1.fY = y1;
      Point_t p2 = {};
      p2.fX = x2;
      p2.fY = y2;
      std::auto_ptr<DrawLine> cmd(new DrawLine(wid, gc, p1, p2));//if this throws, I do not care.
      fCommands.push_back(cmd.get());//this can throw.
      cmd.release();
   } catch (const std::exception &) {
      throw;
   }
}

//______________________________________________________________________________
void CommandBuffer::AddDrawSegments(Drawable_t wid, const GCValues_t &gc, const Segment_t *segments, Int_t nSegments)
{
   assert(segments != 0 && "AddDrawSegments, segments parameter is null");
   assert(nSegments > 0 && "AddDrawSegments, nSegments <= 0");

   try {
      std::auto_ptr<DrawSegments> cmd(new DrawSegments(wid, gc, segments, nSegments));
      fCommands.push_back(cmd.get());
      cmd.release();
   } catch (const std::exception &) {
      throw;
   }
}

//______________________________________________________________________________
void CommandBuffer::AddClearArea(Window_t wid, Int_t x, Int_t y, UInt_t w, UInt_t h)
{
   try {
      Rectangle_t r = {};
      r.fX = x;
      r.fY = y;
      r.fWidth = (UShort_t)w;
      r.fHeight = (UShort_t)h;
      std::auto_ptr<ClearArea> cmd(new ClearArea(wid, r));//Can throw, nothing leaks.
      fCommands.push_back(cmd.get());//this can throw.
      cmd.release();
   } catch (const std::exception &) {
      throw;
   }
}

//______________________________________________________________________________
void CommandBuffer::AddCopyArea(Drawable_t src, Drawable_t dst, const GCValues_t &gc, 
                                Int_t srcX, Int_t srcY, UInt_t width, UInt_t height, Int_t dstX, Int_t dstY)
{
   try {
      Rectangle_t area = {};
      area.fX = srcX;
      area.fY = srcY;
      area.fWidth = (UShort_t)width;
      area.fHeight = (UShort_t)height;
      Point_t dstPoint = {};
      dstPoint.fX = dstX;
      dstPoint.fY = dstY;
      std::auto_ptr<CopyArea> cmd(new CopyArea(src, dst, gc, area, dstPoint));//Can throw, nothing leaks.
      fCommands.push_back(cmd.get());//this can throw.
      cmd.release();
   } catch (const std::exception &) {
      throw;
   }
}

//______________________________________________________________________________
void CommandBuffer::AddDrawString(Drawable_t wid, const GCValues_t &gc, Int_t x, Int_t y, const char *text, Int_t len)
{
   try {
      if (len < 0)//Negative length can come from caller.
         len = std::strlen(text);
      const std::string substr(text, len);//Can throw.
      Point_t p = {};
      p.fX = x;
      p.fY = y;
      std::auto_ptr<DrawString> cmd(new DrawString(wid, gc, p, substr));//Can throw.
      fCommands.push_back(cmd.get());//can throw.
      cmd.release();
   } catch (const std::exception &) {
      throw;
   }
}

//______________________________________________________________________________
void CommandBuffer::AddFillRectangle(Drawable_t wid, const GCValues_t &gc, Int_t x, Int_t y, UInt_t w, UInt_t h)
{
   try {
      Rectangle_t r = {};
      r.fX = x;
      r.fY = y;
      r.fWidth = (UShort_t)w;
      r.fHeight = (UShort_t)h;
      std::auto_ptr<FillRectangle> cmd(new FillRectangle(wid, gc, r));
      fCommands.push_back(cmd.get());
      cmd.release();
   } catch (const std::exception &) {
      throw;
   }
}

//______________________________________________________________________________
void CommandBuffer::AddDrawRectangle(Drawable_t wid, const GCValues_t &gc, Int_t x, Int_t y, UInt_t w, UInt_t h)
{
   try {
      Rectangle_t r = {};
      r.fX = x;
      r.fY = y;
      r.fWidth = (UShort_t)w;
      r.fHeight = (UShort_t)h;
      std::auto_ptr<DrawRectangle> cmd(new DrawRectangle(wid, gc, r));
      fCommands.push_back(cmd.get());
      cmd.release();
   } catch (const std::exception &) {
      throw;
   }
}

//______________________________________________________________________________
void CommandBuffer::AddFillPolygon(Drawable_t wid, const GCValues_t &gc, const Point_t *polygon, Int_t nPoints)
{
   assert(polygon != 0 && "AddFillPolygon, polygon parameter is null");
   assert(nPoints > 0 && "AddFillPolygon, nPoints <= 0");
   
   try {
      std::auto_ptr<FillPolygon> cmd(new FillPolygon(wid, gc, polygon, nPoints));
      fCommands.push_back(cmd.get());
      cmd.release();
   } catch (const std::exception &) {
      throw;
   }
}

//______________________________________________________________________________
void CommandBuffer::AddUpdateWindow(QuartzView *view)
{
   assert(view != nil && "AddUpdateWindow, view parameter is nil");
   
   try {
      std::auto_ptr<UpdateWindow> cmd(new UpdateWindow(view));
      fCommands.push_back(cmd.get());
      cmd.release();
   } catch (const std::exception &) {
      throw;
   }
}

//______________________________________________________________________________
void CommandBuffer::AddDeletePixmap(Pixmap_t pixmapID)
{
   try {
      std::auto_ptr<DeletePixmap> cmd(new DeletePixmap(pixmapID));
      fCommands.push_back(cmd.get());
      cmd.release();
   } catch (const std::exception &) {
      throw;
   }
}

//______________________________________________________________________________
void CommandBuffer::Flush(Details::CocoaPrivate *impl)
{
   assert(impl != 0 && "Flush, impl parameter is null");

   //Basic es-guarantee: state is unknown, but valid, no
   //resource leaks, no locked focus.

   //All magic is here.
   CGContextRef prevContext = 0;
   CGContextRef currContext = 0;
   QuartzView *prevView = nil;

   for (size_type i = 0, e = fCommands.size(); i < e; ++i) {
      const Command *cmd = fCommands[i];
      if (!cmd)//Command was deleted by RemoveOperation/RemoveGraphicsOperation.
         continue;
      
      NSObject<X11Drawable> *drawable = impl->GetDrawable(cmd->fID);
      if (drawable.fIsPixmap) {
         cmd->Execute();//Can throw, ok.
         continue;
      }
      
      QuartzView *view = (QuartzView *)impl->GetWindow(cmd->fID).fContentView;
      
      if (prevView != view)
         ClipOverlaps(view);//Can throw, ok.
      
      prevView = view;
      
      try {
         if ([view lockFocusIfCanDraw]) {
            NSGraphicsContext *nsContext = [NSGraphicsContext currentContext];
            assert(nsContext != nil && "Flush, currentContext is nil");
            currContext = (CGContextRef)[nsContext graphicsPort];
            assert(currContext != 0 && "Flush, graphicsPort is null");//remove this assert?
            
            view.fContext = currContext;
            if (prevContext && prevContext != currContext)
               CGContextFlush(prevContext);
            prevContext = currContext;


            const Quartz::CGStateGuard ctxGuard(currContext);
            
            //Either use shape combine mask, or clip mask.
            //TODO: find a way to combine both?
            ROOT::MacOSX::Util::CFScopeGuard<CGImageRef> clipImageGuard;
            QuartzWindow * const topLevelParent = view.fQuartzWindow;
            if (topLevelParent.fShapeCombineMask) {
               //Attach clip mask to the context.
               const NSRect clipRect  = [view convertRect : view.frame toView : nil];
               clipImageGuard.Reset(CGImageCreateWithImageInRect(topLevelParent.fShapeCombineMask.fImage, clipRect));
               //TODO: this geometry looks suspicious, check!
               //CGContextClipToMask(currContext, CGRectMake(0, 0, clipRect.size.width, clipRect.size.height), clipImageGuard.Get());
            } else if (fClippedRegion.size()) {//(view.fClipMaskIsValid)
             //  CGContextClipToMask(currContext, CGRectMake(0, 0, view.fClipMask.fWidth, view.fClipMask.fHeight), view.fClipMask.fImage);
               CGContextClipToRects(currContext, &fClippedRegion[0], fClippedRegion.size());
            }

            cmd->Execute();//This can throw, we should restore as much as we can here.
            
            if (view.fBackBuffer) {
               //Very "special" window.
               Rectangle_t copyArea = {0, 0, view.fBackBuffer.fWidth, view.fBackBuffer.fHeight};
               [view copy : view.fBackBuffer area : copyArea withMask : nil clipOrigin : Point_t() toPoint : Point_t()];
            }
            
            [view unlockFocus];
            
            view.fContext = 0;
         }      
      } catch (const std::exception &) {
         //Focus was locked, roll-back:
         [view unlockFocus];
         //View's context was modified, roll-back:
         view.fContext = 0;
         //Re-throw, something really bad happened (std::bad_alloc).
         throw;
      }
   }

   if (currContext)
      CGContextFlush(currContext);

   ClearCommands();
}

//______________________________________________________________________________
void CommandBuffer::RemoveOperationsForDrawable(Drawable_t drawable)
{
   for (size_type i = 0; i < fCommands.size(); ++i) {
      if (fCommands[i] && fCommands[i]->HasOperand(drawable)) {
         delete fCommands[i];
         fCommands[i] = 0;
      }
   }
}

//______________________________________________________________________________
void CommandBuffer::RemoveGraphicsOperationsForWindow(Window_t wid)
{
   for (size_type i = 0; i < fCommands.size(); ++i) {
      if (fCommands[i] && fCommands[i]->HasOperand(wid) && fCommands[i]->IsGraphicsCommand()) {
         delete fCommands[i];
         fCommands[i] = 0;
      }
   }
}

//______________________________________________________________________________
void CommandBuffer::ClearCommands()
{
   for (size_type i = 0, e = fCommands.size(); i < e; ++i)
      delete fCommands[i];

   fCommands.clear();
}

//______________________________________________________________________________
void CommandBuffer::ClipOverlaps(QuartzView *view)
{
   //Basic es-guarantee, also, if initClipMask fails,
   //view's clip mask will stay invalid (even if it was valid before).

 /*  typedef std::vector<QuartzView *>::reverse_iterator reverse_iterator;

   assert(view != nil && "ClipOverlaps, view parameter is nil");

   fViewBranch.clear();
   
   for (QuartzView *v = view; v; v = v.fParentView)
      fViewBranch.push_back(v);//Can throw, ok.
   
   if (fViewBranch.size())
      fViewBranch.pop_back();//we do not need content view, it does not have any sibling.

   NSRect frame1 = {};
   NSRect frame2 = view.frame;
   
   //[view clearClipMask];
   view.fClipMaskIsValid = NO;
   
   for (reverse_iterator it = fViewBranch.rbegin(), eIt = fViewBranch.rend(); it != eIt; ++it) {
      QuartzView *ancestorView = *it;//Actually, it's either one of ancestors, or a view itself.
      bool doCheck = false;
      for (QuartzView *sibling in [ancestorView.fParentView subviews]) {
         if (ancestorView == sibling) {
            doCheck = true;//all views after this must be checked.
            continue;
         } else if (!doCheck || sibling.fMapState != kIsViewable) {
            continue;
         }
         
         //Real check is here.
         frame1 = sibling.frame;
         frame2.origin = [view.fParentView convertPoint : view.frame.origin toView : ancestorView.fParentView];
         
         //Check if two rects intersect.
         if (RectsOverlap(frame2, frame1)) {
            if (!view.fClipMaskIsValid) {
               if (![view initClipMask])//initClipMask will issue an error message, also, this can throw.
                  return;//Forget about clipping at all.
               view.fClipMaskIsValid = YES;
            }
            //Update view's clip mask - mask out hidden pixels.
            [view addOverlap : FindOverlapRect(frame2, frame1)];
         }
      }
   }
   
   //Now clip all children.
   frame2 = view.frame;//Should it be visible rect instead?
   
   for (QuartzView *child in [view subviews]) {
      if (child.fMapState != kIsViewable)
         continue;
      
      frame1 = child.frame;
      if (view.fParentView)
         frame1.origin = [view.fParentView convertPoint : frame1.origin fromView : view];
      
      if (RectsOverlap(frame2, frame1)) {
         if (!view.fClipMaskIsValid) {
            if (![view initClipMask])
               return;
            view.fClipMaskIsValid = YES;
         }
         
         [view addOverlap : FindOverlapRect(frame2, frame1)];
      }
   }*/

   //NEW: this is a new code (to be tested) to calculate and exclude overlapped areas -
   //to emulate windows with back store (views are not).

   typedef std::vector<QuartzView *>::reverse_iterator reverse_iterator;
   typedef std::vector<CGRect>::iterator rect_iterator;

   assert(view != nil && "ClipOverlaps, view parameter is nil");

   fRectsToClip.clear();
   fClippedRegion.clear();

   fViewBranch.clear();
   for (QuartzView *v = view; v; v = v.fParentView)
      fViewBranch.push_back(v);//Can throw, ok.

   if (fViewBranch.size())
      fViewBranch.pop_back();//we do not need content view, it does not have any sibling.

   WidgetRect clipRect;
   NSRect frame1 = {};

   //At the beginning we operate in a view parent's coordinate system.
   const NSRect frame2 = view.frame;

   for (reverse_iterator it = fViewBranch.rbegin(), eIt = fViewBranch.rend(); it != eIt; ++it) {
      QuartzView *ancestorView = *it;//Actually, it's either one of ancestors, or a view itself.
      bool doCheck = false;
      for (QuartzView *sibling in [ancestorView.fParentView subviews]) {
         if (ancestorView == sibling) {
            doCheck = true;//all views after this must be checked.
            continue;
         } else if (!doCheck || sibling.fMapState != kIsViewable) {
            continue;
         }
         
         frame1 = sibling.frame;//
         frame1.origin = [sibling.fParentView convertPoint : frame1.origin toView : view.fParentView];

         //Check if two rects intersect.
         if (RectsOverlap(frame2, frame1)) {
            //Update view's clip mask - mask out hidden pixels.
            clipRect.x1 = frame1.origin.x;
            clipRect.x2 = clipRect.x1 + frame1.size.width;
            clipRect.y1 = frame1.origin.y;
            clipRect.y2 = clipRect.y1 + frame1.size.height;
            fRectsToClip.push_back(clipRect);
         }
      }
   }
   
   //Now clip all children.
   
   for (QuartzView *child in [view subviews]) {
      if (child.fMapState != kIsViewable)
         continue;
      
      frame1 = child.frame;
      if (view.fParentView)
         frame1.origin = [view convertPoint : frame1.origin toView : view.fParentView];
      
      if (RectsOverlap(frame2, frame1)) {
         clipRect.x1 = frame1.origin.x;
         clipRect.x2 = clipRect.x1 + frame1.size.width;
         clipRect.y1 = frame1.origin.y;
         clipRect.y2 = clipRect.y1 + frame1.size.height;
         fRectsToClip.push_back(clipRect);
      }
   }
   
   if (fRectsToClip.size()) {
      WidgetRect rect(frame2.origin.x, frame2.origin.y, frame2.origin.x + frame2.size.width, frame2.origin.y + frame2.size.height);

      BuildClipRegion(rect);
      
      if (view.fParentView) {
         //Now convert all rects into view's coordinate system.
         for (rect_iterator recIt = fClippedRegion.begin(), eIt = fClippedRegion.end(); recIt != eIt; ++recIt) {
            if (!recIt->size.width && !recIt->size.height)
               continue;
            recIt->origin = [view.fParentView convertPoint : recIt->origin toView : view];
         }
      }
   }
}

namespace {

//Authors of C++ standard library sure are very smart people.
//The only thing I do not understand, why I do not have NORMAL
//binary_search and have this sh..it which returns bool
//and why I have to use ugly lower_bounds/upper_bounds, which
//does not make my code better.
//The next three functions are used by clipping machinery.

typedef std::vector<int>::iterator int_iterator;

//_____________________________________________________________________________________________________
int_iterator BinarySearchLeft(int_iterator first, int_iterator last, int value)
{
   if (first == last)
      return last;

   const int_iterator it = std::lower_bound(first, last, value);
   assert(it != last && (it == first || *it == value) && "internal logic error");

   //If value < *first, return last (not found).
   return it == first && *it != value ? last : it;
}

//_____________________________________________________________________________________________________
int_iterator BinarySearchRight(int_iterator first, int_iterator last, int value)
{
   if (first == last)
      return last;

   const int_iterator it = std::lower_bound(first, last, value);
   assert((it == last || *it == value) && "internal logic error");

   return it;
}

}//unnamed namespace.

//_____________________________________________________________________________________________________
void CommandBuffer::BuildClipRegion(const WidgetRect &rect)
{
   //Input requirements:
   // 1) all rects are valid (non-empty and x1 < x2, y1 < y2);
   // 2) all rects intersect with widget's rect.
   //I do not check these conditions here, this is done when filling rectsToClip.
   
   //I did not find any reasonable algorithm (have to search better?),
   //code in gdk and pixman has to many dependencies and is lib-specific +
   //they require input to be quite special:
   // a) no overlaps (in my case I have overlaps)
   // b) sorted in a special way.
   //To convert my input into such a format
   //means to implement everything myself (for example, to work out overlaps).

   //Also, my case is more simple: gdk and pixman substract region (== set of rectangles)
   //from another region, I have to substract region from _one_ rectangle.

   //This is quite stupid implementation - I'm calculation rectangles, which form
   //the 'rect' - 'rectsToClip'.
   //Still, I think it's more optimal than my first version, which was _masking_ _pixels_,
   //and at the end was terribly expensive despite of my initial hopes (big masks, millions pixels
   //to set :) ).
   //TODO: benchmark this "cauchemar" and find something not so lame :)
   typedef std::vector<WidgetRect>::const_iterator rect_const_iterator;
   typedef std::vector<bool>::size_type size_type;

   assert(fRectsToClip.size() != 0 && "BuildClipRegion, nothing to clip");

   fClippedRegion.clear();
   fXBounds.clear();
   fYBounds.clear();

   //[First, we "cut" the original rect into stripes.
   for (rect_const_iterator recIt = fRectsToClip.begin(), endIt = fRectsToClip.end(); recIt != endIt; ++recIt) {
      if (recIt->x1 > rect.x1)//upper limit is tested already (input validation).
         fXBounds.push_back(recIt->x1);

      if (recIt->x2 < rect.x2)//lower limit is tested already (input validation).
         fXBounds.push_back(recIt->x2);

      if (recIt->y1 > rect.y1)
         fYBounds.push_back(recIt->y1);

      if (recIt->y2 < rect.y2)
         fYBounds.push_back(recIt->y2);
   }

   if (!fXBounds.size() && !fYBounds.size()) {//Completely hidden.
      fClippedRegion.push_back(CGRectMake(0, 0, 0, 0));
      return;
   }

   std::sort(fXBounds.begin(), fXBounds.end());
   std::sort(fYBounds.begin(), fYBounds.end());

   //We do not need duplicates.
   const int_iterator xBoundsEnd = std::unique(fXBounds.begin(), fXBounds.end());
   const int_iterator yBoundsEnd = std::unique(fYBounds.begin(), fYBounds.end());
   //Rectangle is now "cut into pieces"].

   //["Mark the grid" - find intersections.
   const size_type nXBands = size_type(xBoundsEnd - fXBounds.begin()) + 1;
   const size_type nYBands = size_type(yBoundsEnd - fYBounds.begin()) + 1;

   fGrid.assign(nXBands * nYBands, false);

   //Uff, the first quite horrible part :))
   //Do not even want to think about big-O :)
   //And yes, C++'s lower_bound/binary_search sucks endlessly.
   //I need a normal binary_search, which returns an iterator (with *iterator == value or iterator == last), not a bool :(
   for (rect_const_iterator recIt = fRectsToClip.begin(), endIt = fRectsToClip.end(); recIt != endIt; ++recIt) {

      const int_iterator left = BinarySearchLeft(fXBounds.begin(), xBoundsEnd, recIt->x1);
      const size_type firstXBand = left == xBoundsEnd ? 0 : left - fXBounds.begin() + 1;
      
      const int_iterator right = BinarySearchRight(fXBounds.begin(), xBoundsEnd, recIt->x2);
      const size_type lastXBand = right - fXBounds.begin() + 1;
      
      const int_iterator bottom = BinarySearchLeft(fYBounds.begin(), yBoundsEnd, recIt->y1);
      const size_type firstYBand = bottom == yBoundsEnd ? 0 : bottom - fYBounds.begin() + 1;

      const int_iterator top = BinarySearchRight(fYBounds.begin(), yBoundsEnd, recIt->y2);
      const size_type lastYBand = top - fYBounds.begin() + 1;

      for (size_type i = firstYBand; i < lastYBand; ++i) {
         const size_type baseIndex = i * nXBands;
         for (size_type j = firstXBand; j < lastXBand; ++j)
            fGrid[baseIndex + j] = true;
      }
   }
   //Grid is ready].
   
   //[Create rectangles - check the grid.
   //Event more nightmarish part :))) And I do not merge rectangles.
   //Help me God!
   CGRect newRect = {};

   for (size_type i = 0; i < nYBands; ++i) {
      const size_type baseIndex = i * nXBands;
      for (size_type j = 0; j < nXBands; ++j) {
         if (!fGrid[baseIndex + j]) {
            newRect.origin.x = j ? fXBounds[j - 1] : rect.x1;
            newRect.origin.y = i ? fYBounds[i - 1] : rect.y1;
            
            newRect.size.width = (j == nXBands - 1 ? rect.x2 : fXBounds[j]) - newRect.origin.x;
            newRect.size.height = (i == nYBands - 1 ? rect.y2 : fYBounds[i]) - newRect.origin.y;

            fClippedRegion.push_back(newRect);
         }
      }
   }
   //The end].
}

}//X11
}//MacOSX
}//ROOT
