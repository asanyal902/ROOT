// @(#)root/geom:$Name:  $:$Id: TGeoPgon.cxx,v 1.25 2003/08/21 08:27:34 brun Exp $
// Author: Andrei Gheata   31/01/02
// TGeoPgon::Contains() implemented by Mihaela Gheata

/*************************************************************************
 * Copyright (C) 1995-2000, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

//_____________________________________________________________________________
// TGeoPgon - a polygone. It has at least 10 parameters :
//            - the lower phi limit;
//            - the range in phi;
//            - the number of edges on each z plane;
//            - the number of z planes (at least two) where the inner/outer 
//              radii are changing;
//            - z coordinate, inner and outer radius for each z plane
//
//_____________________________________________________________________________
//Begin_Html
/*
<img src="gif/t_pgon.gif">
*/
//End_Html
//Begin_Html
/*
<img src="gif/t_pgondivZ.gif">
*/
//End_Html

#include "TROOT.h"

#include "TGeoManager.h"
#include "TGeoVolume.h"
#include "TVirtualGeoPainter.h"
#include "TGeoPgon.h"
   
ClassImp(TGeoPgon)

//_____________________________________________________________________________
TGeoPgon::TGeoPgon()
{
// dummy ctor
   SetShapeBit(TGeoShape::kGeoPgon);
   fNedges = 0;
}   

//_____________________________________________________________________________
TGeoPgon::TGeoPgon(Double_t phi, Double_t dphi, Int_t nedges, Int_t nz)
         :TGeoPcon(phi, dphi, nz) 
{
// Default constructor
   SetShapeBit(TGeoShape::kGeoPgon);
   fNedges = nedges;
}

//_____________________________________________________________________________
TGeoPgon::TGeoPgon(const char *name, Double_t phi, Double_t dphi, Int_t nedges, Int_t nz)
         :TGeoPcon(name, phi, dphi, nz) 
{
// Default constructor
   SetShapeBit(TGeoShape::kGeoPgon);
   fNedges = nedges;
}

//_____________________________________________________________________________
TGeoPgon::TGeoPgon(Double_t *param)
         :TGeoPcon(0,0,0) 
{
// Default constructor in GEANT3 style
// param[0] = phi1
// param[1] = dphi
// param[2] = nedges
// param[3] = nz
//
// param[4] = z1
// param[5] = Rmin1
// param[6] = Rmax1
// ...
   SetShapeBit(TGeoShape::kGeoPgon);
   SetDimensions(param);
   ComputeBBox();
}

//_____________________________________________________________________________
TGeoPgon::~TGeoPgon()
{
// destructor
}

//_____________________________________________________________________________
void TGeoPgon::ComputeBBox()
{
// compute bounding box for a polygone
   Double_t zmin = TMath::Min(fZ[0], fZ[fNz-1]);
   Double_t zmax = TMath::Max(fZ[0], fZ[fNz-1]);
   // find largest rmax an smallest rmin
   Double_t rmin, rmax;
   Double_t divphi = fDphi/fNedges;
   // find the radius of the outscribed circle
   rmin = fRmin[TMath::LocMin(fNz, fRmin)];   
   rmax = fRmax[TMath::LocMax(fNz, fRmax)];
   rmax = rmax/TMath::Cos(0.5*divphi*kDegRad);
   Double_t phi1 = fPhi1;
   Double_t phi2 = phi1 + fDphi;
   
   Double_t xc[4];
   Double_t yc[4];
   xc[0] = rmax*TMath::Cos(phi1*kDegRad);
   yc[0] = rmax*TMath::Sin(phi1*kDegRad);
   xc[1] = rmax*TMath::Cos(phi2*kDegRad);
   yc[1] = rmax*TMath::Sin(phi2*kDegRad);
   xc[2] = rmin*TMath::Cos(phi1*kDegRad);
   yc[2] = rmin*TMath::Sin(phi1*kDegRad);
   xc[3] = rmin*TMath::Cos(phi2*kDegRad);
   yc[3] = rmin*TMath::Sin(phi2*kDegRad);

   Double_t xmin = xc[TMath::LocMin(4, &xc[0])];
   Double_t xmax = xc[TMath::LocMax(4, &xc[0])]; 
   Double_t ymin = yc[TMath::LocMin(4, &yc[0])]; 
   Double_t ymax = yc[TMath::LocMax(4, &yc[0])];

   Double_t ddp = -phi1;
   if (ddp<0) ddp+= 360;
   if (ddp<=fDphi) xmax = rmax;
   ddp = 90-phi1;
   if (ddp<0) ddp+= 360;
   if (ddp<=fDphi) ymax = rmax;
   ddp = 180-phi1;
   if (ddp<0) ddp+= 360;
   if (ddp<=fDphi) xmin = -rmax;
   ddp = 270-phi1;
   if (ddp<0) ddp+= 360;
   if (ddp<=fDphi) ymin = -rmax;
   fOrigin[0] = (xmax+xmin)/2;
   fOrigin[1] = (ymax+ymin)/2;
   fOrigin[2] = (zmax+zmin)/2;
   fDX = (xmax-xmin)/2;
   fDY = (ymax-ymin)/2;
   fDZ = (zmax-zmin)/2;
   SetShapeBit(kGeoClosedShape);
}

//_____________________________________________________________________________   
void TGeoPgon::ComputeNormal(Double_t *point, Double_t *dir, Double_t *norm)
{
// Compute normal to closest surface from POINT. 
   memset(norm,0,3*sizeof(Double_t));
   Double_t phi1=0, phi2=0, c1=0, s1=0, c2=0, s2=0;
   Double_t dz, rmin1, rmin2;
   Bool_t is_seg  = (fDphi<360)?kTRUE:kFALSE;
   if (is_seg) {
      phi1 = fPhi1;
      if (phi1<0) phi1+=360;
      phi2 = phi1 + fDphi;
      phi1 *= kDegRad;
      phi2 *= kDegRad;
      c1 = TMath::Cos(phi1);
      s1 = TMath::Sin(phi1);
      c2 = TMath::Cos(phi2);
      s2 = TMath::Sin(phi2);
      if (TGeoShape::IsCloseToPhi(1E-5, point, c1,s1,c2,s2)) {
         TGeoShape::NormalPhi(point,dir,norm,c1,s1,c2,s2);
         return;
      }
   } // Phi done   

   Int_t ipl = TMath::BinarySearch(fNz, fZ, point[2]);
   if (ipl==(fNz-1) || ipl<0) {
      // point outside Z range
      norm[2] = TMath::Sign(1., norm[2]);
      return;
   }
   Int_t iplclose = ipl;
   if ((fZ[ipl+1]-point[2])<(point[2]-fZ[ipl])) iplclose++;
   dz = TMath::Abs(fZ[iplclose]-point[2]);

   Double_t divphi = fDphi/fNedges;
   Double_t phi = TMath::ATan2(point[1], point[0])*kRadDeg;
   if (phi<0) phi+=360.;
   Double_t ddp = phi-fPhi1;
   if (ddp<0) ddp+=360.;
   Int_t ipsec = Int_t(ddp/divphi);
   Double_t ph0 = (fPhi1+divphi*(ipsec+0.5))*kDegRad;
   // compute projected distance
   Double_t r, rsum, rpgon, ta, calf;
   r = TMath::Abs(point[0]*TMath::Cos(ph0)+point[1]*TMath::Sin(ph0));
   if (dz<1E-5) {
      if (iplclose==0 || iplclose==(fNz-1)) {
         norm[2] = TMath::Sign(1., norm[2]);
         return;
      }
      if (iplclose==ipl && fZ[ipl]==fZ[ipl-1]) {
         if (r<TMath::Max(fRmin[ipl],fRmin[ipl-1]) || r>TMath::Min(fRmax[ipl],fRmax[ipl-1])) {
            norm[2] = TMath::Sign(1., norm[2]);
            return;
         }
      } else {
         if (fZ[iplclose]==fZ[iplclose+1]) {
            if (r<TMath::Max(fRmin[iplclose],fRmin[iplclose+1]) || r>TMath::Min(fRmax[iplclose],fRmax[iplclose+1])) {
               norm[2] = TMath::Sign(1., norm[2]);
               return;
            }
         }
      }
   } //-> Z done

   dz = fZ[ipl+1]-fZ[ipl];
   rmin1 = fRmin[ipl];
   rmin2 = fRmin[ipl+1];
   rsum = rmin1+rmin2;
   Double_t safe = kBig;
   if (rsum>1E-10) {
      ta = (rmin2-rmin1)/dz;
      calf = 1./TMath::Sqrt(1+ta*ta);
      rpgon = rmin1 + (point[2]-fZ[ipl])*ta;
      safe = TMath::Abs((r-rpgon)*calf);
      norm[0] = calf*TMath::Cos(ph0);
      norm[1] = calf*TMath::Sin(ph0);
      norm[2] = calf*ta;
   }
   ta = (fRmax[ipl+1]-fRmax[ipl])/dz;
   calf = 1./TMath::Sqrt(1+ta*ta);
   rpgon = fRmax[ipl] + (point[2]-fZ[ipl])*ta;
   if (safe>TMath::Abs((rpgon-r)*calf)) {
      norm[0] = calf*TMath::Cos(ph0);
      norm[1] = calf*TMath::Sin(ph0);
      norm[2] = calf*ta;
   }   
   if (norm[0]*dir[0]+norm[1]*dir[1]+norm[2]*dir[2]<0) {
      norm[0] = -norm[0];
      norm[1] = -norm[1];
      norm[2] = -norm[2];
   }   
}

//_____________________________________________________________________________
Bool_t TGeoPgon::Contains(Double_t *point) const
{
// test if point is inside this shape
   // check total z range
   if (point[2]<fZ[0]) return kFALSE;
   if (point[2]>fZ[fNz-1]) return kFALSE;
   Double_t divphi = fDphi/fNedges;
   // now check phi
   Double_t phi = TMath::ATan2(point[1], point[0])*kRadDeg;
   if (phi < 0) phi += 360.0;
   Double_t ddp = phi-fPhi1;
   if (ddp<0) ddp+=360.;
   if (ddp>fDphi) return kFALSE;
   // now find phi division
   Int_t ipsec = TMath::Min(Int_t(ddp/divphi), fNedges-1);
   Double_t ph0 = (fPhi1+divphi*(ipsec+0.5))*kDegRad;
   // now check projected distance
   Double_t r = point[0]*TMath::Cos(ph0) + point[1]*TMath::Sin(ph0);
   // find in which Z section the point is in
   Int_t iz = TMath::BinarySearch(fNz, fZ, point[2]);
   if (iz==fNz-1) {
      if (r<fRmin[iz]) return kFALSE;
      if (r>fRmax[iz]) return kFALSE;
      return kTRUE;
   }   
   // now compute rmin and rmax and test the value of r
   Double_t dzrat = (point[2]-fZ[iz])/(fZ[iz+1]-fZ[iz]);
   Double_t rmin = fRmin[iz]+dzrat*(fRmin[iz+1]-fRmin[iz]);
   // is the point inside the 'hole' at the center of the volume ?
   if (r < rmin) return kFALSE;
   Double_t rmax = fRmax[iz]+dzrat*(fRmax[iz+1]-fRmax[iz]);
   if (r > rmax) return kFALSE;
   
   return kTRUE;
}

//_____________________________________________________________________________
void TGeoPgon::DefineSection(Int_t snum, Double_t z, Double_t rmin, Double_t rmax)
{
// defines z position of a section plane, rmin and rmax at this z.
   if ((snum<0) || (snum>=fNz)) return;
   fZ[snum]    = z;
   fRmin[snum] = rmin;
   fRmax[snum] = rmax;
   if (snum==(fNz-1)) ComputeBBox();
}

//_____________________________________________________________________________
Double_t TGeoPgon::DistToOutSect(Double_t *point, Double_t *dir, Int_t &iz, Int_t &isect) const
{
// compute distance to outside from a  pgon phi trapezoid
   Double_t saf;
   Double_t dmin = kBig;
   Double_t snext[6];
   Int_t i;
   for (i=0; i<6; i++) snext[i]=kBig;
   Double_t zmin = fZ[iz];
   Double_t zmax = fZ[iz+1];
   if (zmax==zmin) {
      iz += (dir[2]>0)?1:-1;
      Double_t pt[3];
      for (i=0; i<3; i++) pt[i] = point[i]+1E-8*dir[i];
      dmin = 0.;
      if (Contains(&pt[0])) dmin = DistToOutSect(&pt[0], dir, iz, isect);
      return (dmin+1E-8);
   }      
   Double_t divphi = fDphi/fNedges;
   Double_t phi1 = (fPhi1 + divphi*(isect-1))*kDegRad;
   Double_t phi2 = phi1 + divphi*kDegRad;
   Double_t phim = 0.5*(phi1+phi2);
   Double_t cphim = TMath::Cos(phim);
   Double_t sphim = TMath::Sin(phim);
   Double_t minsafe = 0;
   Double_t no[3];
   // check outer slanted face
   Double_t ct, st;
   Double_t fz = (fRmax[iz+1]-fRmax[iz])/(zmax-zmin);
   st = 1./TMath::Sqrt(1.+fz*fz);
   ct = -fz*st;
   if (st<0) st=-st;
   no[0] = st*cphim;
   no[1] = st*sphim;
   no[2] = ct;
   saf = (fRmax[iz]*cphim-point[0])*no[0]+
         (fRmax[iz]*sphim-point[1])*no[1]+
         (fZ[iz]-point[2])*no[2];
   minsafe = saf;
   Double_t calf = dir[0]*no[0]+dir[1]*no[1]+dir[2]*no[2];
   if (calf>0) snext[0] = saf/calf;

   // check inner slanted face
   fz = (fRmin[iz+1]-fRmin[iz])/(zmax-zmin);
   st = -1./TMath::Sqrt(1.+fz*fz);
   ct = -fz*st;
   if (st<0) st=-st;
   no[0] = -st*cphim;
   no[1] = -st*sphim;
   no[2] = ct;
   saf = (fRmin[iz]*cphim-point[0])*no[0]+
         (fRmin[iz]*sphim-point[1])*no[1]+
         (fZ[iz]-point[2])*no[2];
   if (saf<minsafe) minsafe = saf;
   calf = dir[0]*no[0]+dir[1]*no[1]+dir[2]*no[2];
   if (calf>0) snext[1] = saf/calf;
               
   // check upper and lower Z planes
   saf = point[2]-fZ[iz];
   if (saf<minsafe) minsafe = saf;
   if (dir[2]<0) snext[2]=-saf/dir[2];
   saf = fZ[iz+1]-point[2];
   if (saf<minsafe) minsafe = saf;
   if (dir[2]>0) snext[3]=saf/dir[2];

   // check phi1 and phi2 walls
   Double_t r = TMath::Sqrt(point[0]*point[0]+point[1]*point[1]);
   Double_t phi = TMath::ATan2(point[1], point[0]);
   if (phi<phi1) phi+=2*TMath::Pi();
   no[0] = TMath::Sin(phi1);
   no[1] = -TMath::Cos(phi1);
   no[2] = 0;
   saf = TMath::Abs(r*TMath::Sin(phi-phi1));
   if (saf<minsafe) minsafe = saf;
   calf = dir[0]*no[0]+dir[1]*no[1]+dir[2]*no[2];
   if (calf>1E-6) snext[4] = saf/calf;

   no[0] = -TMath::Sin(phi2);
   no[1] = TMath::Cos(phi2);
   no[2] = 0;
   saf = TMath::Abs(r*TMath::Sin(phi2-phi));
   if (saf<minsafe) minsafe = saf;
   calf = dir[0]*no[0]+dir[1]*no[1]+dir[2]*no[2];
   if (calf>1E-6) snext[5] = saf/calf;
   Int_t icheck = TMath::LocMin(6, &snext[0]);
   dmin = snext[icheck];
   if (icheck<2) return dmin;
   Double_t pt[3];
   if (icheck<4) {
      // z plane crossed
      iz += 2*icheck-5;
      if ((iz<0) || (iz>(fNz-2))) return dmin;
      for (i=0; i<3; i++) pt[i]=point[i]+(dmin+1E-8)*dir[i];
      if (Contains(&pt[0]))
         dmin += DistToOutSect(&pt[0], dir, iz, isect)+1E-8;
      return dmin;
   }
   isect += 2*icheck-9;
   if (fDphi==360) {
      if (isect<1) isect=fNedges;
      if (isect>fNedges) isect=1;
   } else {      
      if ((isect<1) || (isect>fNedges)) return dmin;
   }      
   for (i=0; i<3; i++) pt[i]=point[i]+(dmin+1E-8)*dir[i];
   dmin += DistToOutSect(&pt[0], dir, iz, isect)+1E-8;
   return dmin;
}

//_____________________________________________________________________________
Double_t TGeoPgon::DistToOut(Double_t *point, Double_t *dir, Int_t iact, Double_t step, Double_t *safe) const
{
// compute distance from inside point to surface of the polygone
   // first find out in which Z section the point is in
   if (iact<3 && safe) {
      *safe = Safety(point, kTRUE);
      if (iact==0) return kBig;
      if (iact==1 && step<*safe) return kBig;
   }   
   Int_t ipl = TMath::BinarySearch(fNz, fZ, point[2]);
   if (ipl==fNz-1) {
      ipl--;
      if (dir[2]>=0) return 1E-6;
   }   
   if (ipl<0) {
      // point out
      ipl++;
      if (dir[2]<=0) return 1E-6;
   }

   // now find out in which phi section the point is in
   Double_t divphi = fDphi/fNedges;
   Double_t phi = TMath::ATan2(point[1], point[0])*kRadDeg;
   if (phi < fPhi1) phi += 360.0;
   if ((phi<fPhi1) || ((phi-fPhi1)>fDphi)) return kBig;
   // now find phi division [1..fNedges]
   Double_t pt[3];
   memcpy(&pt[0], point, 3*sizeof(Double_t));
   Int_t ipsec = (Int_t)TMath::Min((phi-fPhi1)/divphi+1., (Double_t)fNedges);
   Double_t dsec = DistToOutSect(pt, dir, ipl, ipsec);
   return dsec;   
}   

//_____________________________________________________________________________
Double_t TGeoPgon::DistToIn(Double_t *point, Double_t *dir, Int_t iact, Double_t step, Double_t *safe) const
{
// compute distance from outside point to surface of the polygone
   // first find in which segment we are
   if (iact<3 && safe) {
      *safe = Safety(point, kFALSE);
      if (iact==0) return kBig;
      if (iact==1 && step<*safe) return kBig;
   }   
   Double_t pt[3];
   Double_t eps = 0;
   memcpy(&pt[0], point, 3*sizeof(Double_t));

   UChar_t bits=0;
   const UChar_t kUp = 0x01;
   const UChar_t kDown = 0x02;
   const UChar_t kOut  = kUp | kDown;
   const UChar_t kInhole = 0x04;
   const UChar_t kOuthole = 0x08;
   const UChar_t kInphi = 0x10;
   Bool_t cross=kTRUE;
   // check if ray may intersect outscribed cylinder
   if ((pt[2]<fZ[0]) && (dir[2]<=0)) {
      if (iact==3) return kBig; 
      cross=kFALSE;
   }
   if (cross) {
      if ((pt[2]>fZ[fNz-1]) && (dir[2]>=0)) {
         if (iact==3) return kBig;
         cross=kFALSE;
      }
   }   
   Double_t r2 = pt[0]*pt[0]+pt[1]*pt[1];
   Double_t radmax=0;
   Double_t divphi=fDphi/fNedges;
   if (cross) {
      radmax = fRmax[TMath::LocMax(fNz, fRmax)];
      radmax = radmax/TMath::Cos(0.5*divphi*kDegRad);
      if (r2>(radmax*radmax)) {
         Double_t rpr=-pt[0]*dir[0]-pt[1]*dir[1];
         Double_t nxy = dir[0]*dir[0]+dir[1]*dir[1];
         if (rpr<TMath::Sqrt((r2-radmax*radmax)*nxy)) {
            if (iact==3) return kBig;
            cross = kFALSE;
         }
      }
   }        

   Double_t r = TMath::Sqrt(r2);
   Double_t saf[8];


   Int_t ipl = TMath::BinarySearch(fNz, fZ, pt[2]);
   Int_t ifirst = ipl;
   if (ifirst<0) {
      ifirst=0;
      bits |= kDown;
   } else {
      if (ifirst==(fNz-1)) {
         ifirst=fNz-2;
         bits |= kUp;
      } 
   } 
   if (!(bits & kOut)) {
      saf[0]=pt[2]-fZ[ifirst];
      saf[1]=fZ[ifirst+1]-pt[2];
   } else {
      if (ipl<0) {
         saf[0]=fZ[ifirst]-pt[2];
         saf[1]=-kBig;
      } else {
         saf[0]=-kBig;
         saf[1]=pt[2]-fZ[ifirst+1];
      }   
   }
   // find out if point is in the hole of current segment or outside
   Double_t phi = TMath::ATan2(pt[1], pt[0])*kRadDeg;
   Double_t phi1, phi2;
   if (phi<fPhi1) phi+=360.;
   Int_t ipsec = Int_t((phi-fPhi1)/divphi+1.);
   if (ipsec>fNedges) {
   // point in gap mellon slice
      ipsec = -1;
      saf[2]=saf[3]=-kBig;
      phi1=saf[6]=fPhi1;
      phi2=saf[7]=fPhi1+fDphi;
   } else {
      bits |= kInphi;
      Double_t ph0=(fPhi1+divphi*(ipsec-0.5))*kDegRad;
      phi1=saf[6]=fPhi1+(ipsec-1)*divphi;
      phi2=saf[7]=phi1+divphi;
      Double_t rproj=pt[0]*TMath::Cos(ph0)+pt[1]*TMath::Sin(ph0);
      Double_t dzrat=(pt[2]-fZ[ifirst])/(fZ[ifirst+1]-fZ[ifirst]);
      // rmin and rmax at Z coordinate of the point
      Double_t rmin=fRmin[ifirst]+(fRmin[ifirst+1]-fRmin[ifirst])*dzrat;
      Double_t rmax=fRmax[ifirst]+(fRmax[ifirst+1]-fRmax[ifirst])*dzrat;
      if ((rmin>0) && (rproj<rmin)) bits |= kInhole;
      if (rproj>rmax) bits |= kOuthole;
      Double_t tin=(fRmin[ifirst+1]-fRmin[ifirst])/(fZ[ifirst+1]-fZ[ifirst]);
      Double_t cin=1./TMath::Sqrt(1.0+tin*tin);
      Double_t tou=(fRmax[ifirst+1]-fRmax[ifirst])/(fZ[ifirst+1]-fZ[ifirst]);
      Double_t cou=1./TMath::Sqrt(1.0+tou*tou);
      saf[2] = (bits & kInhole)?((rmin-rproj)*cin):-kBig;
      saf[3] = (bits & kOuthole)?((rproj-rmax)*cou):-kBig;
   }
   // find closest distance to phi walls
   Double_t dph1=(bits & kInphi)?(phi-phi1):(phi1-phi);
   Double_t dph2=(bits & kInphi)?(phi2-phi):(phi-phi2);
   saf[4]=r*TMath::Sin(dph1*kDegRad);
   saf[5]=r*TMath::Sin(dph2*kDegRad);
   // compute distance to boundary   
   if (!cross) return kBig;
   Double_t snxt=DistToInSect(&pt[0], dir, ifirst, ipsec, bits, &saf[0]);
   return (snxt+eps);
}

//_____________________________________________________________________________
Double_t TGeoPgon::DistToInSect(Double_t *point, Double_t *dir, Int_t &iz, Int_t & /*ipsec*/, 
                                UChar_t &bits, Double_t *saf) const 
{
   // propagate to next Z plane
//   printf("--dist to sector %i\n", iz);
   const UChar_t kUp = 0x01;
   const UChar_t kDown = 0x02;
//   const UChar_t kOut  = kUp | kDown;
   const UChar_t kInhole = 0x04;
   const UChar_t kOuthole = 0x08;
   const UChar_t kInphi = 0x10;
   Double_t nwall[3];
   Double_t s=kBig;
   Double_t snxt=kBig;
//   gGeoManager->SetNormalChecked(TMath::Abs(dir[2]));
   if (bits & kUp) {
      if (dir[2]>=0) return kBig;
      snxt=-saf[1]/dir[2];
   } else {   
      if (bits & kDown) {
         if (dir[2]<=0) return kBig;
         snxt=saf[0]/dir[2];
      } else {
         if (dir[2]>0) {
            snxt=saf[1]/dir[2];
         } else {
            if (dir[2]<0) {
               snxt=-saf[0]/dir[2];
            }
         }         
      }
   }      
//   printf("---dist to Z : %f\n", snxt);
   // propagate to closest wall
   Double_t calf,tz, st, ct, sp, cp;
//   Double_t divphi=fDphi/fNedges;
   Double_t phi1 = saf[6];
   Double_t phi2 = saf[7];
   Double_t ph0;
   if (bits & kInphi) {
      ph0=0.5*(phi1+phi2)*kDegRad;
      sp = -TMath::Sin(ph0);
      cp = -TMath::Cos(ph0);
      if (bits & kInhole) {
         tz = (fRmin[iz+1]-fRmin[iz])/(fZ[iz+1]-fZ[iz]);
         st = 1./TMath::Sqrt(1.0+tz*tz);
         ct = st*tz;
//         printf("norm to inner : st=%f ct=%f sp=%f cp=%f\n", st,ct,sp,cp);
         nwall[0]=st*cp;
         nwall[1]=st*sp;
         nwall[2]=ct;
         calf = nwall[0]*dir[0]+nwall[1]*dir[1]+nwall[2]*dir[2];
         if (calf<0) {
            s=-saf[2]/calf;
//            printf("dist to inner : %f\n", s);
            if (s<snxt) {
//               gGeoManager->SetNormalChecked(-calf);
               snxt=s;
            }
         }
      }
      if (bits & kOuthole) {
          sp = -sp;
         cp = -cp;
         tz = (fRmax[iz]-fRmax[iz+1])/(fZ[iz+1]-fZ[iz]);
         st = 1./TMath::Sqrt(1.0+tz*tz);
         ct = st*tz;
//         printf("norm to outer : st=%f ct=%f sp=%f cp=%f\n", st,ct,sp,cp);
         nwall[0]=st*cp;
         nwall[1]=st*sp;
         nwall[2]=ct;
         calf = nwall[0]*dir[0]+nwall[1]*dir[1]+nwall[2]*dir[2];
         if (calf<0) {
            s=-saf[3]/calf;
//            printf("dist to outer : %f\n", s);
            if (s<snxt) {
//               gGeoManager->SetNormalChecked(TMath::Abs(calf));
               snxt=s;
            }
         }
      }
   }   
   // propagate to phi planes
   if (saf[4]>=0) {
      nwall[0]=-TMath::Sin(phi1*kDegRad);
      nwall[1]=TMath::Cos(phi1*kDegRad);
      nwall[2]=0;
      if (!(bits & kInphi)) {
         nwall[0] = -nwall[0];
         nwall[1] = -nwall[1];
      }   
//      printf("norm to phi1 : nx=%f ny=%f\n", nwall[0], nwall[1]);
      calf= nwall[0]*dir[0]+nwall[1]*dir[1]+nwall[2]*dir[2];
      if (calf<0) {
         s=-saf[4]/calf;
//         printf("dist to phi1 : %f\n", s);
         if (s<snxt) {
//            gGeoManager->SetNormalChecked(-calf);
            snxt=s;
         }
      }
   }      

   if (saf[5]>=0) {
      nwall[0]=TMath::Sin(phi2*kDegRad);
      nwall[1]=-TMath::Cos(phi2*kDegRad);
      nwall[2]=0;
      if (!(bits & kInphi)) {
         nwall[0] = -nwall[0];
         nwall[1] = -nwall[1];
      }   
//      printf("norm to phi2 : nx=%f ny=%f\n", nwall[0], nwall[1]);
      calf= nwall[0]*dir[0]+nwall[1]*dir[1]+nwall[2]*dir[2];
      if (calf<0) {
         s=-saf[5]/calf;
//         printf("dist to phi2 : %f\n", s);
         if (s<snxt) {
//            gGeoManager->SetNormalChecked(-calf);
            snxt=s;
         }
      }
   }      
   for (Int_t i=0; i<3; i++) point[i]+=dir[i]*(snxt+1E-9);
   if (Contains(point)) return snxt;
   snxt += DistToIn(point, dir, 3);
   return snxt;        
}

//_____________________________________________________________________________
Int_t TGeoPgon::DistancetoPrimitive(Int_t px, Int_t py)
{
// compute closest distance from point px,py to each corner
   Int_t n = fNedges+1;
   const Int_t numPoints = 2*n*fNz;
   return ShapeDistancetoPrimitive(numPoints, px, py);
}

//_____________________________________________________________________________
TGeoVolume *TGeoPgon::Divide(TGeoVolume *voldiv, const char *divname, Int_t iaxis, Int_t ndiv, 
                             Double_t start, Double_t step) 
{
//--- Divide this polygone shape belonging to volume "voldiv" into ndiv volumes
// called divname, from start position with the given step. Returns pointer
// to created division cell volume in case of Z divisions. Phi divisions are
// allowed only if nedges%ndiv=0 and create polygone "segments" with nedges/ndiv edges.
// Z divisions can be performed if the divided range is in between two consecutive Z planes.
// In case a wrong division axis is supplied, returns pointer to volume that was divided.

//   printf("Dividing %s : nz=%d nedges=%d phi1=%g dphi=%g (ndiv=%d iaxis=%d start=%g step=%g)\n",
//          voldiv->GetName(), fNz, fNedges, fPhi1, fDphi, ndiv, iaxis, start, step);
   TGeoShape *shape;           //--- shape to be created
   TGeoVolume *vol;            //--- division volume to be created
   TGeoVolumeMulti *vmulti;    //--- generic divided volume
   TGeoPatternFinder *finder;  //--- finder to be attached 
   TString opt = "";           //--- option to be attached
   Int_t nedges = fNedges;
   Double_t zmin = start;
   Double_t zmax = start+ndiv*step;            
   Int_t isect = -1;
   Int_t is, id, ipl;
   switch (iaxis) {
      case 1:  //---                R division
         Error("Divide", "makes no sense dividing a pgon on radius");
         return 0;
      case 2:  //---                Phi division
         if (fNedges%ndiv) {
            Error("Divide", "ndiv should divide number of pgon edges");
            return 0;
         }
         nedges = fNedges/ndiv;
         finder = new TGeoPatternCylPhi(voldiv, ndiv, start, start+ndiv*step);
         vmulti = gGeoManager->MakeVolumeMulti(divname, voldiv->GetMedium());
         voldiv->SetFinder(finder);
         finder->SetDivIndex(voldiv->GetNdaughters());            
         shape = new TGeoPgon(-step/2, step, nedges, fNz);
         vol = new TGeoVolume(divname, shape, voldiv->GetMedium());
         vmulti->AddVolume(vol);
         for (is=0; is<fNz; is++)
            ((TGeoPgon*)shape)->DefineSection(is, fZ[is], fRmin[is], fRmax[is]); 
         opt = "Phi";
         for (id=0; id<ndiv; id++) {
            voldiv->AddNodeOffset(vol, id, start+id*step+step/2, opt.Data());
            ((TGeoNodeOffset*)voldiv->GetNodes()->At(voldiv->GetNdaughters()-1))->SetFinder(finder);
         }
         return vmulti;
      case 3: // ---                Z division
         // find start plane
         for (ipl=0; ipl<fNz-1; ipl++) {
            if (start<fZ[ipl]) continue;
            else {
               if ((start+ndiv*step)>fZ[ipl+1]) continue;
            }
            isect = ipl;
            zmin = fZ[isect];
            zmax = fZ[isect+1];
            break;
         }
         if (isect<0) {
            Error("Divide", "cannot divide pcon on Z if divided region is not between 2 consecutive planes");
            return 0;
         }
         finder = new TGeoPatternZ(voldiv, ndiv, start, start+ndiv*step);
         vmulti = gGeoManager->MakeVolumeMulti(divname, voldiv->GetMedium());
         voldiv->SetFinder(finder);
         finder->SetDivIndex(voldiv->GetNdaughters());
         opt = "Z";
         for (id=0; id<ndiv; id++) {
            Double_t z1 = start+id*step;
            Double_t z2 = start+(id+1)*step;
            Double_t rmin1 = (fRmin[isect]*(zmax-z1)-fRmin[isect+1]*(zmin-z1))/(zmax-zmin);
            Double_t rmax1 = (fRmax[isect]*(zmax-z1)-fRmax[isect+1]*(zmin-z1))/(zmax-zmin);
            Double_t rmin2 = (fRmin[isect]*(zmax-z2)-fRmin[isect+1]*(zmin-z2))/(zmax-zmin);
            Double_t rmax2 = (fRmax[isect]*(zmax-z2)-fRmax[isect+1]*(zmin-z2))/(zmax-zmin);
            shape = new TGeoPgon(fPhi1, fDphi, nedges, 2); 
            ((TGeoPgon*)shape)->DefineSection(0, -step/2, rmin1, rmax1); 
            ((TGeoPgon*)shape)->DefineSection(1,  step/2, rmin2, rmax2); 
            vol = new TGeoVolume(divname, shape, voldiv->GetMedium());
            vmulti->AddVolume(vol);
            voldiv->AddNodeOffset(vol, id, start+id*step+step/2, opt.Data());
            ((TGeoNodeOffset*)voldiv->GetNodes()->At(voldiv->GetNdaughters()-1))->SetFinder(finder);
         }
         return vmulti;
      default:
         Error("Divide", "Wrong axis type for division");
         return 0;            
   }
}

//_____________________________________________________________________________
void TGeoPgon::GetBoundingCylinder(Double_t *param) const
{
//--- Fill vector param[4] with the bounding cylinder parameters. The order
// is the following : Rmin, Rmax, Phi1, Phi2
   param[0] = fRmin[0];           // Rmin
   param[1] = fRmax[0];           // Rmax
   for (Int_t i=1; i<fNz; i++) {
      if (fRmin[i] < param[0]) param[0] = fRmin[i];
      if (fRmax[i] > param[1]) param[1] = fRmax[i];
   }
   Double_t divphi = fDphi/fNedges;
   param[1] /= TMath::Cos(0.5*divphi*kDegRad);
   param[0] *= param[0];
   param[1] *= param[1];
   if (fDphi==360.) {
      param[2] = 0.;
      param[3] = 360.;
      return;
   }   
   param[2] = (fPhi1<0)?(fPhi1+360.):fPhi1;     // Phi1
   param[3] = param[2]+fDphi;                   // Phi2
}   

//_____________________________________________________________________________
void TGeoPgon::InspectShape() const
{
   printf("*** TGeoPgon parameters ***\n");
   printf("    Nedges = %i\n", fNedges);
   TGeoPcon::InspectShape();
}

//_____________________________________________________________________________
void TGeoPgon::Paint(Option_t *option)
{
// paint this shape according to option
   TVirtualGeoPainter *painter = gGeoManager->GetGeomPainter();
   if (!painter) return;
   TGeoVolume *vol = gGeoManager->GetCurrentVolume();
   if (vol->GetShape() != (TGeoShape*)this) return;
   painter->PaintPcon(this, option);
}

//_____________________________________________________________________________
void TGeoPgon::PaintNext(TGeoHMatrix *glmat, Option_t *option)
{
// paint this shape according to option
   TVirtualGeoPainter *painter = gGeoManager->GetGeomPainter();
   if (!painter) return;
   painter->PaintPcon(this, option, glmat);
}

//_____________________________________________________________________________
Double_t TGeoPgon::Safety(Double_t *point, Bool_t in) const
{
// computes the closest distance from given point to this shape, according
// to option. The matching point on the shape is stored in spoint.
   Double_t saf[5];
   Double_t safe, dz;
   Int_t i;
   Int_t ipl = TMath::BinarySearch(fNz, fZ, point[2]);
   if (ipl>1) {
      if(fZ[ipl]==fZ[ipl-1] && point[2]==fZ[ipl]) ipl--;
   }   
   for (i=0; i<5; i++) saf[i]=kBig;
   Double_t ssp[2];
   ssp[0] = ssp[1] = TGeoShape::kBig;
   if (in) {
      //---> first locate Z segment and compute Z safety
      if (ipl==(fNz-1)) return 0;
      if (ipl<0) return 0;
      dz = fZ[ipl+1]-fZ[ipl];
      if (dz<1E-6) {
         if (fRmin[ipl]>0) return 0;
      }
      if (ipl==0) {
         saf[0] = point[2]-fZ[0];
         if (saf[0]<1E-4) return saf[0];
      }
      if (ipl==fNz-2) {
         saf[1] = fZ[fNz-1]-point[2];   
         if (saf[1]<1E-4) return saf[1];
      }
      if (ipl>1) {
         if (fZ[ipl]==fZ[ipl-1]) {
            if (fRmin[ipl]<fRmin[ipl-1] || fRmax[ipl]>fRmax[ipl-1]) {
               saf[0] = point[2]-fZ[ipl];
               if (saf[0]<1E-4) return saf[0];
            }
         }
      }
      if (ipl<fNz-3) {
         if (fZ[ipl+1]==fZ[ipl+2]) {
            if (fRmin[ipl+1]<fRmin[ipl+2] || fRmax[ipl+1]>fRmax[ipl+2]) {
               saf[1] = fZ[ipl+1]-point[2];
               if (saf[1]<1E-4) return saf[1];         
            }
         }
      }
   } else {
      if (ipl>=0 && ipl<fNz-1) {
         dz = fZ[ipl+1]-fZ[ipl];
         if (dz==0) {
            ipl++;
            dz = fZ[ipl+1]-fZ[ipl];
         }
         if (ipl>1) {
            if (fZ[ipl]==fZ[ipl-1]) {
               if (fRmin[ipl]>fRmin[ipl-1] || fRmax[ipl]<fRmax[ipl-1]) {
                  ssp[0] = point[2]-fZ[ipl];
                  if (ssp[0]<1E-4) return ssp[0];
//                  saf[0] = -saf[0];
               }
            }
         }
         if (ipl<fNz-3) {
            if (fZ[ipl+1]==fZ[ipl+2]) {
               if (fRmin[ipl+1]>fRmin[ipl+2] || fRmax[ipl+1]<fRmax[ipl+2]) {
                  ssp[1] = fZ[ipl+1]-point[2];
                  if (ssp[1]<1E-4) return ssp[1];
//                  saf[1] = -saf[1];         
               }
            }
         }
      } else {
         if (ipl<0) {
            ipl=0;
            saf[0] = -fZ[0]+point[2];
         } else {
            ipl=fNz-2;
            saf[1] = -point[2]+fZ[fNz-1];
         }
         dz = fZ[ipl+1]-fZ[ipl];
      }
   }         
   //---> compute phi safety
   if (fDphi<360) {
      Double_t phi1 = fPhi1*kDegRad;
      Double_t phi2 = (fPhi1+fDphi)*kDegRad;
      Double_t c1 = TMath::Cos(phi1);
      Double_t s1 = TMath::Sin(phi1);
      Double_t c2 = TMath::Cos(phi2);
      Double_t s2 = TMath::Sin(phi2);
      saf[2] =  TGeoShape::SafetyPhi(point,in,c1,s1,c2,s2);
   }

   //---> locate phi and compute R safety
   Double_t divphi = fDphi/fNedges;
   Double_t phi = TMath::ATan2(point[1], point[0])*kRadDeg;
   if (phi<0) phi+=360.;
   Double_t ddp = phi-fPhi1;
   if (ddp<0) ddp+=360.;
   Int_t ipsec = Int_t(ddp/divphi);
   Double_t ph0 = (fPhi1+divphi*(ipsec+0.5))*kDegRad;
   // compute projected distance
   Double_t r, rsum, rpgon, ta, calf;
   r = point[0]*TMath::Cos(ph0)+point[1]*TMath::Sin(ph0);
   rsum = fRmin[ipl]+fRmin[ipl+1];
   if (rsum>1E-10) {
      ta = (fRmin[ipl+1]-fRmin[ipl])/dz;
      calf = 1./TMath::Sqrt(1+ta*ta);
      rpgon = fRmin[ipl] + (point[2]-fZ[ipl])*ta;
      saf[3] = (r-rpgon)*calf;
   }
   ta = (fRmax[ipl+1]-fRmax[ipl])/dz;
   calf = 1./TMath::Sqrt(1+ta*ta);
   rpgon = fRmax[ipl] + (point[2]-fZ[ipl])*ta;
   saf[4] = (rpgon-r)*calf;
   if (in) return saf[TMath::LocMin(5,saf)];
   for (i=0; i<5; i++) saf[i]=-saf[i];
   safe = saf[TMath::LocMax(5,saf)];
   safe = TMath::Min(safe, TMath::Min(ssp[0],ssp[1]));
   return safe;
}

//_____________________________________________________________________________
void TGeoPgon::SetDimensions(Double_t *param)
{
   fPhi1    = param[0];
   fDphi    = param[1];
   fNedges  = (Int_t)param[2];
   fNz      = (Int_t)param[3];
   if (!fRmin) fRmin = new Double_t [fNz];
   if (!fRmax) fRmax = new Double_t [fNz];
   if (!fZ)    fZ    = new Double_t [fNz];
   for (Int_t i=0; i<fNz; i++) 
      DefineSection(i, param[4+3*i], param[5+3*i], param[6+3*i]);
}   

//_____________________________________________________________________________
void TGeoPgon::SetPoints(Double_t *buff) const
{
// create polygone mesh points
    Double_t phi, dphi;
    Int_t n = fNedges + 1;
    dphi = fDphi/(n-1);
    Double_t factor = 1./TMath::Cos(kDegRad*dphi/2);
    Int_t i, j;
    Int_t indx = 0;

    if (buff) {
        for (i = 0; i < fNz; i++)
        {
            for (j = 0; j < n; j++)
            {
                phi = (fPhi1+j*dphi)*kDegRad;
                buff[indx++] = factor * fRmin[i] * TMath::Cos(phi);
                buff[indx++] = factor * fRmin[i] * TMath::Sin(phi);
                buff[indx++] = fZ[i];
            }
            for (j = 0; j < n; j++)
            {
                phi = (fPhi1+j*dphi)*kDegRad;
                buff[indx++] = factor * fRmax[i] * TMath::Cos(phi);
                buff[indx++] = factor * fRmax[i] * TMath::Sin(phi);
                buff[indx++] = fZ[i];
            }
        }
    }
}

//_____________________________________________________________________________
void TGeoPgon::SetPoints(Float_t *buff) const
{
// create polygone mesh points
    Double_t phi, dphi;
    Int_t n = fNedges + 1;
    dphi = fDphi/(n-1);
    Double_t factor = 1./TMath::Cos(kDegRad*dphi/2);
    Int_t i, j;
    Int_t indx = 0;

    if (buff) {
        for (i = 0; i < fNz; i++)
        {
            for (j = 0; j < n; j++)
            {
                phi = (fPhi1+j*dphi)*kDegRad;
                buff[indx++] = factor * fRmin[i] * TMath::Cos(phi);
                buff[indx++] = factor * fRmin[i] * TMath::Sin(phi);
                buff[indx++] = fZ[i];
            }
            for (j = 0; j < n; j++)
            {
                phi = (fPhi1+j*dphi)*kDegRad;
                buff[indx++] = factor * fRmax[i] * TMath::Cos(phi);
                buff[indx++] = factor * fRmax[i] * TMath::Sin(phi);
                buff[indx++] = fZ[i];
            }
        }
    }
}

//_____________________________________________________________________________
void TGeoPgon::Sizeof3D() const
{
// fill size of this 3-D object
    TVirtualGeoPainter *painter = gGeoManager->GetGeomPainter();
    if (!painter) return;
    Int_t n;

    n = fNedges+1;

    Int_t numPoints = fNz*2*n;
    Int_t numSegs   = 4*(fNz*n-1+(fDphi == 360));
    Int_t numPolys  = 2*(fNz*n-1+(fDphi == 360));
    painter->AddSize3D(numPoints, numSegs, numPolys);
}
