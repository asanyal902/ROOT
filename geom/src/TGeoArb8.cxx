// @(#)root/geom:$Name:  $:$Id: TGeoArb8.cxx,v 1.5 2002/07/10 19:24:16 brun Exp $
// Author: Andrei Gheata   31/01/02

/*************************************************************************
 * Copyright (C) 1995-2000, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#include "TROOT.h"

#include "TGeoManager.h"
#include "TGeoVolume.h"
#include "TGeoArb8.h"

    
/*************************************************************************
 * TGeoArb8 - a arbitrary trapezoid with less than 8 vertices standing on
 *   two paralel planes perpendicular to Z axis. Parameters :
 *            - dz - half length in Z;
 *            - xy[8][2] - vector of (x,y) coordinates of vertices
 *               - first four points (xy[i][j], i<4, j<2) are the (x,y)
 *                 coordinates of the vertices sitting on the -dz plane;
 *               - last four points (xy[i][j], i>=4, j<2) are the (x,y)
 *                 coordinates of the vertices sitting on the +dz plane;
 *   The order of defining the vertices of an arb8 is the following :
 *      - point 0 is connected with points 1,3,4
 *      - point 1 is connected with points 0,2,5
 *      - point 2 is connected with points 1,3,6
 *      - point 3 is connected with points 0,2,7
 *      - point 4 is connected with points 0,5,7
 *      - point 5 is connected with points 1,4,6
 *      - point 6 is connected with points 2,5,7
 *      - point 7 is connected with points 3,4,6
 *   Points can be identical in order to create shapes with less than 
 *   8 vertices.
 *
 *************************************************************************/
//Begin_Html
/*
<img src="gif/TGeoTrap.gif">
*/
//End_Html

////////////////////////////////////////////////////////////////////////////
//                                                                        //
// TGeoTrap                                                               //
//                                                                        //
// TRAP is a general trapezoid, i.e. one for which the faces perpendicular//
// to z are trapezia and their centres are not the same x, y. It has 11   //
// parameters: the half length in z, the polar angles from the centre of  //
// the face at low z to that at high z, H1 the half length in y at low z, //
// LB1 the half length in x at low z and y low edge, LB2 the half length  //
// in x at low z and y high edge, TH1 the angle w.r.t. the y axis from the//
// centre of low y edge to the centre of the high y edge, and H2, LB2,    //
// LH2, TH2, the corresponding quantities at high z.                      //
//                                                                        //
////////////////////////////////////////////////////////////////////////////
//Begin_Html
/*
<img src="gif/TGeoTrap.gif">
*/
//End_Html

////////////////////////////////////////////////////////////////////////////
//                                                                        //
// TGeoGtra                                                               //
//                                                                        //
// Gtra is a twisted trapezoid, i.e. one for which the faces perpendicular//
// to z are trapezia and their centres are not the same x, y. It has 12   //
// parameters: the half length in z, the polar angles from the centre of  //
// the face at low z to that at high z, twist, H1 the half length in y at low z, //
// LB1 the half length in x at low z and y low edge, LB2 the half length  //
// in x at low z and y high edge, TH1 the angle w.r.t. the y axis from the//
// centre of low y edge to the centre of the high y edge, and H2, LB2,    //
// LH2, TH2, the corresponding quantities at high z.                      //
//                                                                        //
////////////////////////////////////////////////////////////////////////////
//Begin_Html
/*
<img src="gif/TGeoGtra.gif">
*/
//End_Html

ClassImp(TGeoArb8)

//-----------------------------------------------------------------------------
TGeoArb8::TGeoArb8()
{
   // dummy ctor
   fDz = 0;
   fTwist = 0;
   for (Int_t i=0; i<8; i++) {
      fXY[i][0] = 0.0;
      fXY[i][1] = 0.0;
   }   
}
//-----------------------------------------------------------------------------
TGeoArb8::TGeoArb8(Double_t dz, Double_t *vertices)
         :TGeoBBox(0,0,0)
{
// constructor. If the array of vertices is not null, this should be
// in the format : (x0, y0, x1, y1, ... , x7, y7) 
   fDz = dz;
   fTwist = 0;
   if (vertices) {
      for (Int_t i=0; i<8; i++) {
         fXY[i][0] = vertices[2*i];
         fXY[i][1] = vertices[2*i+1];
      }
      ComputeTwist();
      ComputeBBox();
   } else {
      for (Int_t i=0; i<8; i++) {
         fXY[i][0] = 0.0;
         fXY[i][1] = 0.0;
      }   
   }
}
//-----------------------------------------------------------------------------
TGeoArb8::~TGeoArb8()
{
// destructor
   if (fTwist) delete fTwist;
}
//-----------------------------------------------------------------------------
void TGeoArb8::SetVertex(Int_t vnum, Double_t x, Double_t y)
{
//  set values for a given vertex
   if (vnum<0 || vnum >7) {
      Error("SetVertex", "Invalid vertex number");
      return;
   }
   fXY[vnum][0] = x;
   fXY[vnum][1] = y;
   if (vnum == 7) {
      ComputeTwist();
      ComputeBBox();
   }
}
//-----------------------------------------------------------------------------
void TGeoArb8::ComputeBBox()
{
// compute bounding box for a Arb8
   Double_t xmin, xmax, ymin, ymax;
   xmin = xmax = fXY[0][0];
   ymin = ymax = fXY[0][1];
   
   for (Int_t i=1; i<8; i++) {
      if (xmin>fXY[i][0]) xmin=fXY[i][0];
      if (xmax<fXY[i][0]) xmax=fXY[i][0];
      if (ymin>fXY[i][1]) ymin=fXY[i][1];
      if (ymax<fXY[i][1]) ymax=fXY[i][1];
   }
   fDX = 0.5*(xmax-xmin);
   fDY = 0.5*(ymax-ymin);
   fDZ = fDz;
   fOrigin[0] = 0.5*(xmax+xmin);
   fOrigin[1] = 0.5*(ymax+ymin);
   fOrigin[2] = 0;
}   
//-----------------------------------------------------------------------------
void TGeoArb8::ComputeTwist()
{
// compute tangents of twist angles (angles between projections on XY plane
// of corresponding -dz +dz edges). Called after last point [7] was set.
   Double_t twist[4];
   Bool_t twisted = kFALSE;
   Double_t al1, al2, dx1, dy1, dx2, dy2;
   for (Int_t i=0; i<4; i++) {
      dx1 = fXY[(i+1)%4][0]-fXY[i][0];
      dy1 = fXY[(i+1)%4][1]-fXY[i][1];
      dx2 = fXY[4+(i+1)%4][0]-fXY[4+i][0];
      dy2 = fXY[4+(i+1)%4][1]-fXY[4+i][1];
      if (((dx1==0)&&(dy1==0)) || ((dx2==0)&&(dy2==0))) {
         twist[i] = 0;
         continue;
      }
      al1 = TMath::ATan2(dy1,dx1);
      if (al1<0) al1+=2*TMath::Pi();
      al2 = TMath::ATan2(dy2,dx2);
      if (al2<0) al2+=2*TMath::Pi();
      twist[i] = TMath::Tan(al2-al1);
      if (twist[i]<1E-3) {
         twist[i] = 0;
         continue;
      }
      twisted = kTRUE;
   }
   if (!twisted) return;
   if (fTwist) delete fTwist;
   fTwist = new Double_t[4];
   memcpy(fTwist, &twist[0], 4*sizeof(Double_t));
}
//-----------------------------------------------------------------------------
Bool_t TGeoArb8::Contains(Double_t *point) const
{
// test if point is inside this sphere
   // first check Z range
   if (TMath::Abs(point[2]) > fDz) return kFALSE;
   // compute intersection between Z plane containing point and the arb8
   Double_t poly[8];
//   memset(&poly[0], 0, 8*sizeof(Double_t));
   SetPlaneVertices(point[2], &poly[0]);
   // find the intersections of Y=Ypoint with this poly.
   Double_t segx[4];
   Double_t x1, x2, y1, y2;
   Int_t npts = 0;
   Int_t i;
   for (i=0; i<4; i++) {
      y1 = poly[2*(i%4)+1];
      y2 = poly[2*((i+1)%4)+1];
//      printf("check Yp=%f against y1=%f y2=%f\n", point[1], y1, y2);
      if ((point[1]-y1)*(y2-point[1])<0) continue;
      x1 = poly[2*(i%4)];
      x2 = poly[2*((i+1)%4)];
//      printf("    x1=%f  x2=%f\n", x1,x2);
      // check if point is on the line connecting points 1-2
      if (y1 == y2) {
         if ((point[0]<x1) || (point[0]>x2)) return kFALSE;
         return kTRUE;
      }
      Double_t cf = (point[1]-y1)/(y2-y1);
      segx[npts++] = x1+cf*(x2-x1);
//      printf("   x1+cf*(x2-x1) : %f+%f*(%f-%f)=%f\n",x1, cf, x2, x1, x1+cf*(x2-x1));
      // sort intersection points by X
      if (npts>1) {
         if (segx[npts-2] > segx[npts-1]) {
            x1 = segx[npts-2];
            segx[npts-2] = segx[npts-1];
            segx[npts-1] = x1;
         }
      }
   }
//   printf("Intersections with Y = %f (Xpoint=%f):\n",point[1], point[0]);
//   for (i=0;i<npts;i++) printf("%i : X=%f\n", i,segx[i]);
   if (npts == 0) return kFALSE;
   if (npts == 2) {
      if ((point[0]<segx[npts-2]) || (point[0]>segx[npts-1])) return kFALSE;
      return kTRUE;
   }
   if (npts != 4) return kFALSE;
   // intersection poly is not convex (4 points)
   if ((point[0]<segx[0]) || (point[0]>segx[3])) return kFALSE;
   if ((point[0]>segx[1]) && (point[0]<segx[2])) return kFALSE;
   return kTRUE;
}
//-----------------------------------------------------------------------------
Double_t TGeoArb8::DistToPlane(Double_t *point, Double_t *dir, Int_t ipl, Bool_t in) const 
{
// compute distance to plane ipl :
// ipl=0 : points 0,4,1,5
// ipl=1 : points 1,5,2,6
// ipl=2 : points 2,6,3,7
// ipl=3 : points 3,7,0,4
   Double_t xa,xb,xc,xd;
   Double_t ya,yb,yc,yd;
   xa=fXY[ipl][0];
   ya=fXY[ipl][1];
   xb=fXY[ipl+4][0];
   yb=fXY[ipl+4][1];
   xc=fXY[(ipl+1)%4][0];
   yc=fXY[(ipl+1)%4][1];
   xd=fXY[4+(ipl+1)%4][0];
   yd=fXY[4+(ipl+1)%4][1];
   if (xa==xc) {
      if (ya==yc) {
         if (xb==xd) {
            if (yb==yd) return kBig;
         }   
      }
   }            
   Double_t tx1=0.5*(xb-xa)/fDz;
   Double_t ty1=0.5*(yb-ya)/fDz;
   Double_t tx2=0.5*(xd-xc)/fDz;
   Double_t ty2=0.5*(yd-yc)/fDz;
   Double_t xs1=xa+tx1*(fDz+point[2]);
   Double_t ys1=ya+ty1*(fDz+point[2]);
   Double_t xs2=xc+tx2*(fDz+point[2]);
   Double_t ys2=yc+ty2*(fDz+point[2]);
   Double_t dxs=xs2-xs1;
   Double_t dys=ys2-ys1;
   Double_t dtx=tx2-tx1;
   Double_t dty=ty2-ty1;
   Double_t a=(dtx*dir[1]-dty*dir[0]+(tx1*ty2-tx2*ty1)*dir[2])*dir[2];
   Double_t b=dxs*dir[1]-dys*dir[0]+(dtx*point[1]-dty*point[0]+ty2*xs1-ty1*xs2
              +tx1*ys2-tx2*ys1)*dir[2];
   Double_t c=dxs*point[1]-dys*point[0]+xs1*ys2-xs2*ys1;
   Double_t s=kBig;
   Double_t x1,x2,y1,y2,xp,yp,zi;
   if (a==0) {           
      if (b==0) return kBig;
      s=-c/b;
      if (s>0) {
         if (in) return s;
         zi=point[2]+s*dir[2];
         if (TMath::Abs(zi)<fDz) {
            x1=xs1+tx1*dir[2]*s;
            x2=xs2+tx2*dir[2]*s;
            xp=point[0]+s*dir[0];
            if ((x1==x2) || ((xp-x1)*(x2-xp)>=0)) {
               y1=ys1+ty1*dir[2]*s;
               y2=ys2+ty2*dir[2]*s;
               yp=point[1]+s*dir[1];
               if ((y1==y2) || ((yp-y1)*(y2-yp)>=0)) return s;
            }
         }      
      }
      return kBig;
   }      
   b=0.5*b/a;
   c=c/a;
   Double_t d=b*b-c;
   if (d>=0) {
      s=-b-TMath::Sqrt(d);
      if (s>0) {
         if (in) return s;
         zi=point[2]+s*dir[2];
         if (TMath::Abs(zi)<fDz) {
            x1=xs1+tx1*dir[2]*s;
            x2=xs2+tx2*dir[2]*s;
            xp=point[0]+s*dir[0];
            if ((x1==x2) || ((xp-x1)*(x2-xp)>=0)) {
               y1=ys1+ty1*dir[2]*s;
               y2=ys2+ty2*dir[2]*s;
               yp=point[1]+s*dir[1];
               if ((y1==y2) || ((yp-y1)*(y2-yp)>=0)) return s;
            }
         }      
      }
      s=-b+TMath::Sqrt(d);
      if (s>0) {
         if (in) return s;
         zi=point[2]+s*dir[2];
         if (TMath::Abs(zi)<fDz) {
            x1=xs1+tx1*dir[2]*s;
            x2=xs2+tx2*dir[2]*s;
            xp=point[0]+s*dir[0];
            if ((x1==x2) || ((xp-x1)*(x2-xp)>=0)) {
               y1=ys1+ty1*dir[2]*s;
               y2=ys2+ty2*dir[2]*s;
               yp=point[1]+s*dir[1];
               if ((y1==y2) || ((yp-y1)*(y2-yp)>=0)) return s;
            }
         }      
      }
   }
   return kBig;
}      
      
//-----------------------------------------------------------------------------
Double_t TGeoArb8::DistToIn(Double_t *point, Double_t *dir, Int_t iact, Double_t step, Double_t *safe) const
{
// compute distance from outside point to surface of the arb8
   Double_t snxt=kBig;
   if (!TGeoBBox::Contains(point)) {
      snxt=TGeoBBox::DistToIn(point,dir,3);
      if (snxt>1E20) return snxt;
   }   
   Double_t dist[5];
   // check lateral faces
   for (Int_t i=0; i<4; i++) 
      dist[i]=DistToPlane(point, dir, i, kFALSE);  
   // check Z planes
   dist[4]=kBig;
   if (TMath::Abs(point[2])>fDz) {
      if (dir[2]!=0) {
         Double_t pt[3];
         if (point[2]>0) {
            dist[4] = (fDz-point[2])/dir[2];
            pt[2]=fDz;
         } else {   
            dist[4] = (-fDz-point[2])/dir[2];
            pt[2]=-fDz;
         }   
         if (dist[4]<0) {
            dist[4]=kBig;
         } else {   
            for (Int_t j=0; j<2; j++) pt[j]=point[j]+dist[4]*dir[j];
            if (!Contains(&pt[0])) dist[4]=kBig;
         }   
      }
   }   
   return dist[TMath::LocMin(5, &dist[0])];
}   
//-----------------------------------------------------------------------------
Double_t TGeoArb8::DistToOut(Double_t *point, Double_t *dir, Int_t iact, Double_t step, Double_t *safe) const
{
// compute distance from outside point to surface of the arb8
   Double_t dist[6];
   dist[0]=dist[1]=kBig;
   if (dir[2]<0) {
      dist[0]=(-fDz-point[2])/dir[2];
   } else {
      if (dir[2]>0) dist[1]=(fDz-point[2])/dir[2];
   }      
   for (Int_t i=0; i<4; i++) {
      dist[i+2]=DistToPlane(point, dir, i, kTRUE);   
   }   
      
   return dist[TMath::LocMin(6, &dist[0])];   
}   
//-----------------------------------------------------------------------------
Double_t TGeoArb8::DistToSurf(Double_t *point, Double_t *dir) const
{
// computes the distance to next surface of the sphere along a ray
// starting from given point to the given direction.
   return kBig;
}
//-----------------------------------------------------------------------------
TGeoVolume *TGeoArb8::Divide(TGeoVolume *voldiv, const char *divname, Int_t iaxis, Int_t ndiv, 
                             Double_t start, Double_t step) 
{
   Error("Divide", "Division of an arbitrary trapezoid not implemented");
   return voldiv;
}      
//-----------------------------------------------------------------------------
TGeoVolume *TGeoArb8::Divide(TGeoVolume *voldiv, const char *divname, Int_t iaxis, Double_t step) 
{
// Divide all range of iaxis in range/step cells 
   Error("Divide", "Division in all range not implemented");
   return voldiv;
}      
//-----------------------------------------------------------------------------
void TGeoArb8::InspectShape() const
{
// print shape parameters
   printf("*** TGeoArb8 parameters ***\n");
   for (Int_t ip=0; ip<8; ip++) {
      printf("    point #%i : x=%11.5f y=%11.5f z=%11.5f\n", 
             ip, fXY[ip][0], fXY[ip][1], fDz*((ip<4)?-1:1));
   }
   TGeoBBox::InspectShape();
}
//-----------------------------------------------------------------------------
void TGeoArb8::Paint(Option_t *option)
{
// paint this shape according to option
   TGeoBBox::Paint(option);
}
//-----------------------------------------------------------------------------
void TGeoArb8::NextCrossing(TGeoParamCurve *c, Double_t *point) const
{
// computes next intersection point of curve c with this shape
}
//-----------------------------------------------------------------------------
Double_t TGeoArb8::Safety(Double_t *point, Double_t *spoint, Option_t *option) const
{
// computes the closest distance from given point to this shape, according
// to option. The matching point on the shape is stored in spoint.
   return kBig;
}
//-----------------------------------------------------------------------------
void TGeoArb8::SetPlaneVertices(Double_t zpl, Double_t *vertices) const
{
 // compute intersection points between plane at zpl and non-horizontal edges
   Double_t cf = (fDz-zpl)/(2*fDz);
   for (Int_t i=0; i<4; i++) {
      vertices[2*i]   = fXY[i+4][0]+cf*(fXY[i][0]-fXY[i+4][0]);
      vertices[2*i+1] = fXY[i+4][1]+cf*(fXY[i][1]-fXY[i+4][1]);
   }
}
//-----------------------------------------------------------------------------
void TGeoArb8::SetDimensions(Double_t *param)
{
// set arb8 params in one step :
   fDz      = param[0];
   for (Int_t i=0; i<8; i++) {
      fXY[i][0] = param[2*i];
      fXY[i][1] = param[2*i+1];
   }
   ComputeTwist();
   ComputeBBox();
}   
//-----------------------------------------------------------------------------
void TGeoArb8::SetPoints(Double_t *buff) const
{
// create arb8 mesh points
   for (Int_t i=0; i<8; i++) {
      buff[3*i] = fXY[i][0];
      buff[3*i+1] = fXY[i][1];
      buff[3*i+2] = (i<4)?-fDz:fDz;
//      printf("%i X=%9.3fY=%9.3fZ=%9.3f\n",i,buff[3*i],buff[3*i+1],buff[3*i+2] );
   }
}
//-----------------------------------------------------------------------------
void TGeoArb8::SetPoints(Float_t *buff) const
{
// create arb8 mesh points
   for (Int_t i=0; i<8; i++) {
      buff[3*i] = fXY[i][0];
      buff[3*i+1] = fXY[i][1];
      buff[3*i+2] = (i<4)?-fDz:fDz;
//      printf("%i X=%9.3fY=%9.3fZ=%9.3f\n",i,buff[3*i],buff[3*i+1],buff[3*i+2] );
   }
}
//-----------------------------------------------------------------------------
void TGeoArb8::Sizeof3D() const
{
// fill size of this 3-D object
   TGeoBBox::Sizeof3D();
}

ClassImp(TGeoTrap)

//-----------------------------------------------------------------------------
TGeoTrap::TGeoTrap()
{
   // dummy ctor
}
//-----------------------------------------------------------------------------
TGeoTrap::TGeoTrap(Double_t dz, Double_t theta, Double_t phi)
{
   fDz = dz;
   fTheta = theta;
   fPhi = phi;
   fH1 = fH2 = fBl1 = fBl2 = fTl1 = fTl2 = fAlpha1 = fAlpha2 = 0;
}
//-----------------------------------------------------------------------------
TGeoTrap::TGeoTrap(Double_t dz, Double_t theta, Double_t phi, Double_t h1,
              Double_t bl1, Double_t tl1, Double_t alpha1, Double_t h2, Double_t bl2, 
              Double_t tl2, Double_t alpha2)
{
// constructor. Have to work on it !!!!
   fDz = dz;
   fTheta = theta;
   fPhi = phi;
   fH1 = h1;
   fH2 = h2;
   fBl1 = bl1;
   fBl2 = bl2;
   fTl1 = tl1;
   fTl2 = tl2;
   fAlpha1 = alpha1;
   fAlpha2 = alpha2;
   for (Int_t i=0; i<8; i++) {
      fXY[i][0] = 0.0;
      fXY[i][1] = 0.0;
   }   
   Double_t tx = TMath::Tan(theta*kDegRad)*TMath::Cos(phi*kDegRad);
   Double_t ty = TMath::Tan(theta*kDegRad)*TMath::Sin(phi*kDegRad);
   Double_t ta1 = TMath::Tan(alpha1*kDegRad);
   Double_t ta2 = TMath::Tan(alpha2*kDegRad);
   fXY[0][0] = -dz*tx-h1*ta1-bl1;    fXY[0][1] = -dz*ty-h1;
   fXY[1][0] = -dz*tx+h1*ta1-tl1;    fXY[1][1] = -dz*ty+h1;
   fXY[2][0] = -dz*tx+h1*ta1+tl1;    fXY[2][1] = -dz*ty+h1;
   fXY[3][0] = -dz*tx-h1*ta1+bl1;    fXY[3][1] = -dz*ty-h1;
   fXY[4][0] = dz*tx-h2*ta2-bl2;    fXY[4][1] = dz*ty-h2;
   fXY[5][0] = dz*tx+h2*ta2-tl2;    fXY[5][1] = dz*ty+h2;
   fXY[6][0] = dz*tx+h2*ta2+tl2;    fXY[6][1] = dz*ty+h2;
   fXY[7][0] = dz*tx-h2*ta2+bl2;    fXY[7][1] = dz*ty-h2;
   ComputeTwist();
   if ((dz<0) || (h1<0) || (bl1<0) || (tl1<0) || 
       (h2<0) || (bl2<0) || (tl2<0)) {
      SetBit(kGeoRunTimeShape);
//      printf("trap : dz=%f, h1=%f, bl1=%f, tl1= %f, h2=%f, bl2=%f, tl2=%f\n",
//             dz,h1,bl1,tl1,h2,bl2,tl2);
   } 
   else TGeoArb8::ComputeBBox();
}
//-----------------------------------------------------------------------------
TGeoTrap::~TGeoTrap()
{
// destructor
}
//-----------------------------------------------------------------------------
TGeoVolume *TGeoTrap::Divide(TGeoVolume *voldiv, const char *divname, Int_t iaxis, Int_t ndiv, 
                             Double_t start, Double_t step) 
{
//--- Divide this trapezoid shape belonging to volume "voldiv" into ndiv volumes
// called divname, from start position with the given step. Only Z divisions
// are supported. For Z divisions just return the pointer to the volume to be 
// divided. In case a wrong division axis is supplied, returns pointer to 
// volume that was divided.
   TGeoShape *shape;           //--- shape to be created
   TGeoVolume *vol;            //--- division volume to be created
   TGeoPatternFinder *finder;  //--- finder to be attached 
   TString opt = "";           //--- option to be attached
   if (iaxis!=3) {
      Error("Divide", "cannot divide trapezoids on other axis than Z");
      return voldiv;
   }
   Double_t points_lo[8];
   Double_t points_hi[8];
   finder = new TGeoPatternTrapZ(voldiv, ndiv, start, start+ndiv*step);
   voldiv->SetFinder(finder);
   finder->SetDivIndex(voldiv->GetNdaughters());
   opt = "Z";
   Double_t txz = ((TGeoPatternTrapZ*)finder)->GetTxz();
   Double_t tyz = ((TGeoPatternTrapZ*)finder)->GetTyz();
   Double_t zmin, zmax, ox,oy,oz;
   for (Int_t idiv=0; idiv<ndiv; idiv++) {
      zmin = start+idiv*step;
      zmax = start+(idiv+1)*step;
      oz = start+idiv*step+step/2;
      ox = oz*txz;
      oy = oz*tyz;
      SetPlaneVertices(zmin, &points_lo[0]);
      SetPlaneVertices(zmax, &points_hi[0]);
      shape = new TGeoTrap(step/2, fTheta, fPhi);
      for (Int_t vert1=0; vert1<4; vert1++)
         ((TGeoArb8*)shape)->SetVertex(vert1, points_lo[2*vert1]-ox, points_lo[2*vert1+1]-oy);
      for (Int_t vert2=0; vert2<4; vert2++)
         ((TGeoArb8*)shape)->SetVertex(vert2+4, points_hi[2*vert2]-ox, points_hi[2*vert2+1]-oy);
      vol = new TGeoVolume(divname, shape, voldiv->GetMaterial());
      voldiv->AddNodeOffset(vol, idiv, oz, opt.Data());
      ((TGeoNodeOffset*)voldiv->GetNodes()->At(voldiv->GetNdaughters()-1))->SetFinder(finder);
   }
   return voldiv;
}   
//-----------------------------------------------------------------------------
TGeoVolume *TGeoTrap::Divide(TGeoVolume *voldiv, const char *divname, Int_t iaxis, Double_t step) 
{
// Divide all range of iaxis in range/step cells 
   Error("Divide", "Division in all range not implemented");
   return voldiv;
}      
//-----------------------------------------------------------------------------
TGeoShape *TGeoTrap::GetMakeRuntimeShape(TGeoShape *mother) const
{
// in case shape has some negative parameters, these has to be computed
// in order to fit the mother
   if (!TestBit(kGeoRunTimeShape)) return 0;
   if (mother->IsRunTimeShape()) {
      Error("GetMakeRuntimeShape", "invalid mother");
      return 0;
   }
   Double_t dz, h1, bl1, tl1, h2, bl2, tl2;
   if (fDz<0) dz=((TGeoTrap*)mother)->GetDz();
   else dz=fDz;
   if (fH1<0) 
      h1 = ((TGeoTrap*)mother)->GetH1();
    else 
      h1 = fH1;
   if (fH2<0) 
      h2 = ((TGeoTrap*)mother)->GetH2();
    else 
      h2 = fH2;
   if (fBl1<0) 
      bl1 = ((TGeoTrap*)mother)->GetBl1();
    else 
      bl1 = fBl1;
   if (fBl2<0) 
      bl2 = ((TGeoTrap*)mother)->GetBl2();
    else 
      bl2 = fBl2;
   if (fTl1<0) 
      tl1 = ((TGeoTrap*)mother)->GetTl1();
    else 
      tl1 = fTl1;
   if (fTl2<0) 
      tl2 = ((TGeoTrap*)mother)->GetTl2();
    else 
      tl2 = fTl2;
   return (new TGeoTrap(dz, fTheta, fPhi, h1, bl1, tl1, fAlpha1, h2, bl2, tl2, fAlpha2));
}

ClassImp(TGeoGtra)

//-----------------------------------------------------------------------------
TGeoGtra::TGeoGtra()
{
   // dummy ctor
   fTwistAngle = 0;
}
//-----------------------------------------------------------------------------
TGeoGtra::TGeoGtra(Double_t dz, Double_t theta, Double_t phi, Double_t twist, Double_t h1,
              Double_t bl1, Double_t tl1, Double_t alpha1, Double_t h2, Double_t bl2, 
              Double_t tl2, Double_t alpha2)
{
// constructor. 
   fTheta = theta;
   fPhi = phi;
   fH1 = h1;
   fH2 = h2;
   fBl1 = bl1;
   fBl2 = bl2;
   fTl1 = tl1;
   fTl2 = tl2;
   fAlpha1 = alpha1;
   fAlpha2 = alpha2;
   Double_t x, y, dx, dy, dx1, dx2, th, ph, al1, al2;
   al1 = alpha1*kDegRad;
   al2 = alpha2*kDegRad;
   th = theta*kDegRad;
   ph = phi*kDegRad;
   dx = 2*dz*TMath::Sin(th)*TMath::Cos(ph);
   dy = 2*dz*TMath::Sin(th)*TMath::Sin(ph);
   fDz = dz;
   dx1 = 2*h1*TMath::Tan(al1);
   dx2 = 2*h2*TMath::Tan(al2);

   fTwistAngle = twist;

   Int_t i;
   for (i=0; i<8; i++) {
      fXY[i][0] = 0.0;
      fXY[i][1] = 0.0;
   }   

   fXY[0][0] = -bl1;                fXY[0][1] = -h1;
   fXY[1][0] = -tl1+dx1;            fXY[1][1] = h1;
   fXY[2][0] = tl1+dx1;             fXY[2][1] = h1;
   fXY[3][0] = bl1;                 fXY[3][1] = -h1;
   fXY[4][0] = -bl2+dx;             fXY[4][1] = -h2+dy;
   fXY[5][0] = -tl2+dx+dx2;         fXY[5][1] = h2+dy;
   fXY[6][0] = tl2+dx+dx2;          fXY[6][1] = h2+dy;
   fXY[7][0] = bl2+dx;              fXY[7][1] = -h2+dy;
   for (i=4; i<8; i++) {
      x = fXY[i][0];
      y = fXY[i][1];
      fXY[i][0] = x*TMath::Cos(twist*kDegRad) + y*TMath::Sin(twist*kDegRad);
      fXY[i][1] = -x*TMath::Sin(twist*kDegRad) + y*TMath::Cos(twist*kDegRad);      
   }
   ComputeTwist();
   if ((dz<0) || (h1<0) || (bl1<0) || (tl1<0) || 
       (h2<0) || (bl2<0) || (tl2<0)) SetBit(kGeoRunTimeShape);
   else TGeoArb8::ComputeBBox();
}
//-----------------------------------------------------------------------------
TGeoGtra::~TGeoGtra()
{
// destructor
}
//-----------------------------------------------------------------------------
TGeoShape *TGeoGtra::GetMakeRuntimeShape(TGeoShape *mother) const
{
// in case shape has some negative parameters, these has to be computed
// in order to fit the mother
   if (!TestBit(kGeoRunTimeShape)) return 0;
   if (mother->IsRunTimeShape()) {
      Error("GetMakeRuntimeShape", "invalid mother");
      return 0;
   }
   Double_t dz, h1, bl1, tl1, h2, bl2, tl2;
   if (fDz<0) dz=((TGeoTrap*)mother)->GetDz();
   else dz=fDz;
   if (fH1<0) 
      h1 = ((TGeoTrap*)mother)->GetH1();
    else 
      h1 = fH1;
   if (fH2<0) 
      h2 = ((TGeoTrap*)mother)->GetH2();
    else 
      h2 = fH2;
   if (fBl1<0) 
      bl1 = ((TGeoTrap*)mother)->GetBl1();
    else 
      bl1 = fBl1;
   if (fBl2<0) 
      bl2 = ((TGeoTrap*)mother)->GetBl2();
    else 
      bl2 = fBl2;
   if (fTl1<0) 
      tl1 = ((TGeoTrap*)mother)->GetTl1();
    else 
      tl1 = fTl1;
   if (fTl2<0) 
      tl2 = ((TGeoTrap*)mother)->GetTl2();
    else 
      tl2 = fTl2;
   return (new TGeoGtra(dz, fTheta, fPhi, fTwistAngle ,h1, bl1, tl1, fAlpha1, h2, bl2, tl2, fAlpha2));
}


