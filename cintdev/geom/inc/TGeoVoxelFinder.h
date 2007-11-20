// @(#)root/geom:$Id$
// Author: Andrei Gheata   04/02/02

/*************************************************************************
 * Copyright (C) 1995-2000, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#ifndef ROOT_TGeoVoxelFinder
#define ROOT_TGeoVoxelFinder

#ifndef ROOT_TObject
#include "TObject.h"
#endif

class TGeoVolume;

/*************************************************************************
 * TGeoVoxelFinder - finder class handling voxels 
 *  
 *************************************************************************/

class TGeoVoxelFinder : public TObject
{
public:
enum EVoxelsType {
   kGeoInvalidVoxels = BIT(15),
   kGeoRebuildVoxels = BIT(16)
};
protected:
   TGeoVolume      *fVolume;          // volume to which applies

   Int_t             fNcandidates;    // ! number of candidates
   Int_t             fCurrentVoxel;   // ! index of current voxel in sorted list
   Int_t             fIbx;            // number of different boundaries on X axis
   Int_t             fIby;            // number of different boundaries on Y axis
   Int_t             fIbz;            // number of different boundaries on Z axis
   Int_t             fNboxes;         // length of boxes array
   Int_t             fNox;            // length of array of X offsets
   Int_t             fNoy;            // length of array of Y offsets
   Int_t             fNoz;            // length of array of Z offsets
   Int_t             fNex;            // length of array of X extra offsets
   Int_t             fNey;            // length of array of Y extra offsets
   Int_t             fNez;            // length of array of Z extra offsets
   Int_t             fNx;             // length of array of X voxels
   Int_t             fNy;             // length of array of Y voxels
   Int_t             fNz;             // length of array of Z voxels
   Int_t             fPriority[3];    // priority for each axis
   Int_t             fSlices[3];      // ! slice indices for current voxel
   Int_t             fInc[3];         // ! slice index increment
   Double_t          fInvdir[3];      // ! 1/current director cosines
   Double_t          fLimits[3];      // limits on X,Y,Z
   Double_t         *fBoxes;          //[fNboxes] list of bounding boxes
   Double_t         *fXb;             //[fIbx] ordered array of X box boundaries
   Double_t         *fYb;             //[fIby] ordered array of Y box boundaries
   Double_t         *fZb;             //[fIbz] ordered array of Z box boundaries
   Int_t            *fOBx;            //[fNox] offsets of daughter indices for slices X
   Int_t            *fOBy;            //[fNoy] offsets of daughter indices for slices Y
   Int_t            *fOBz;            //[fNoz] offsets of daughter indices for slices Z
   Int_t            *fOEx;            //[fNox] offsets of extra indices for slices X
   Int_t            *fOEy;            //[fNoy] offsets of extra indices for slices Y
   Int_t            *fOEz;            //[fNoz] offsets of extra indices for slices Z
   Int_t            *fIndX;           //[fNx] indices of daughters inside boundaries X
   Int_t            *fIndY;           //[fNy] indices of daughters inside boundaries Y
   Int_t            *fIndZ;           //[fNz] indices of daughters inside boundaries Z
   Int_t            *fExtraX;         //[fNex] indices of extra daughters in X slices
   Int_t            *fExtraY;         //[fNey] indices of extra daughters in Y slices
   Int_t            *fExtraZ;         //[fNez] indices of extra daughters in Z slices
   Int_t            *fCheckList;      //! list of candidates
   UChar_t          *fBits1;          //! bits used for list intersection

   TGeoVoxelFinder(const TGeoVoxelFinder&);
   TGeoVoxelFinder& operator=(const TGeoVoxelFinder&);
   
   virtual void        BuildVoxelLimits();
   Int_t              *GetExtraX(Int_t islice, Bool_t left, Int_t &nextra) const;
   Int_t              *GetExtraY(Int_t islice, Bool_t left, Int_t &nextra) const;
   Int_t              *GetExtraZ(Int_t islice, Bool_t left, Int_t &nextra) const;
   Bool_t              GetIndices(Double_t *point);
   Int_t               GetPriority(Int_t iaxis) const {return fPriority[iaxis];}
   Int_t               GetNcandidates() const         {return fNcandidates;}
   Int_t              *GetValidExtra(Int_t *list, Int_t &ncheck);
   Int_t              *GetValidExtra(Int_t n1, UChar_t *array1, Int_t *list, Int_t &ncheck);
   Int_t              *GetValidExtra(Int_t n1, UChar_t *array1, Int_t n2, UChar_t *array2, Int_t *list, Int_t &ncheck);
   virtual Int_t      *GetVoxelCandidates(Int_t i, Int_t j, Int_t k, Int_t &ncheck);
//   Bool_t              Intersect(Int_t n1, Int_t *array1, Int_t n2, Int_t *array2,
//                             Int_t n3, Int_t *array3, Int_t &nf, Int_t *result); 
   Bool_t              Intersect(Int_t n1, UChar_t *array1, Int_t &nf, Int_t *result); 
   Bool_t              Intersect(Int_t n1, UChar_t *array1, Int_t n2, UChar_t *array2,
                             Int_t &nf, Int_t *result); 
   Bool_t              Intersect(Int_t n1, UChar_t *array1, Int_t n2, UChar_t *array2,
                             Int_t n3, UChar_t *array3, Int_t &nf, Int_t *result); 
//   void                IntersectAndStore(Int_t n1, Int_t *array1, Int_t n2, Int_t *array2,
//                             Int_t n3, Int_t *array3);
   Bool_t              IntersectAndStore(Int_t n1, UChar_t *array1); 
   Bool_t              IntersectAndStore(Int_t n1, UChar_t *array1, Int_t n2, UChar_t *array2); 
   Bool_t              IntersectAndStore(Int_t n1, UChar_t *array1, Int_t n2, UChar_t *array2,
                             Int_t n3, UChar_t *array3); 
   virtual void        SortAll(Option_t *option="");
//   Bool_t              Union(Int_t n1, Int_t *array1, Int_t n2, Int_t *array2,
//                             Int_t n3, Int_t *array3);
   Bool_t              Union(Int_t n1, UChar_t *array1);
   Bool_t              Union(Int_t n1, UChar_t *array1, Int_t n2, UChar_t *array2);
   Bool_t              Union(Int_t n1, UChar_t *array1, Int_t n2, UChar_t *array2,
                             Int_t n3, UChar_t *array3);
public :
   TGeoVoxelFinder();
   TGeoVoxelFinder(TGeoVolume *vol);
   virtual ~TGeoVoxelFinder();
   virtual void        CreateCheckList();
   void                DaughterToMother(Int_t id, Double_t *local, Double_t *master) const;
   virtual Double_t    Efficiency();
   virtual Int_t      *GetCheckList(Double_t *point, Int_t &nelem);
   Int_t              *GetCheckList(Int_t &nelem) const {nelem=fNcandidates; return fCheckList;}
//   virtual Bool_t      GetNextIndices(Double_t *point, Double_t *dir);
   virtual Int_t      *GetNextCandidates(Double_t *point, Int_t &ncheck); 
   virtual void        FindOverlaps(Int_t inode) const;
   Bool_t              IsInvalid() const {return TObject::TestBit(kGeoInvalidVoxels);}
   Bool_t              NeedRebuild() const {return TObject::TestBit(kGeoRebuildVoxels);}
   Double_t           *GetBoxes() const {return fBoxes;}
   Bool_t              IsSafeVoxel(Double_t *point, Int_t inode, Double_t minsafe) const;
   virtual void        Print(Option_t *option="") const;
   void                PrintVoxelLimits(Double_t *point) const;
   void                SetInvalid(Bool_t flag=kTRUE) {TObject::SetBit(kGeoInvalidVoxels, flag);}
   void                SetNeedRebuild(Bool_t flag=kTRUE) {TObject::SetBit(kGeoRebuildVoxels, flag);}
   virtual Int_t      *GetNextVoxel(Double_t *point, Double_t *dir, Int_t &ncheck);
   virtual void        SortCrossedVoxels(Double_t *point, Double_t *dir);
   virtual void        Voxelize(Option_t *option="");

   ClassDef(TGeoVoxelFinder, 2)                // voxel finder class
};

/*************************************************************************
 * TGeoCylVoxels - Cylindrical voxels class 
 *  
 *************************************************************************/

class TGeoCylVoxels : public TGeoVoxelFinder
{
private:
   virtual void        SortAll(Option_t *option="");
   virtual void        BuildVoxelLimits();
public:
   TGeoCylVoxels();
   TGeoCylVoxels(TGeoVolume *vol);
   virtual ~TGeoCylVoxels();
   
   virtual Double_t    Efficiency();
   virtual void        FindOverlaps(Int_t inode) const;
   virtual Int_t      *GetCheckList(Double_t *point, Int_t &nelem);
//   virtual Bool_t      GetNextIndices(Double_t *point, Double_t *dir);
   virtual Int_t      *GetNextVoxel(Double_t *point, Double_t *dir, Int_t &ncheck);
   Int_t               IntersectIntervals(Double_t vox1, Double_t vox2, Double_t phi1, Double_t phi2) const;
   virtual void        Print(Option_t *option="") const;
   virtual void        Voxelize(Option_t *option);

   ClassDef(TGeoCylVoxels, 2)                // cylindrical voxel class
};

/*************************************************************************
 * TGeoFullVoxels - Full voxeliztion applies to volumes with limited
 *   number of daughters and stores node information in each cell. 
 *  
 *************************************************************************/

class TGeoFullVoxels : public TGeoVoxelFinder
{
private:
   Int_t               fNvoxels;
   Int_t               fNvx;          // number of slices on X
   Int_t               fNvy;          // number of slices on Y
   Int_t               fNvz;          // number of slices on Z
   UChar_t            *fVox;          //[fNvoxels] voxels storage array

   TGeoFullVoxels(const TGeoFullVoxels&); // Not implemented
   TGeoFullVoxels& operator=(const TGeoFullVoxels&); // Not implemented

public:
   TGeoFullVoxels();
   TGeoFullVoxels(TGeoVolume *vol);
   virtual ~TGeoFullVoxels();
   
//   virtual void        BuildVoxelLimits();
//   virtual Double_t    Efficiency();
//   virtual void        FindOverlaps(Int_t inode) const;
   virtual Int_t      *GetCheckList(Double_t *point, Int_t &nelem);
//   virtual Bool_t      GetNextIndices(Double_t *point, Double_t *dir);
   UChar_t            *GetVoxel(Int_t i, Int_t j, Int_t k) const {return &fVox[(i*fNvy+j)*fNvz+k];}
   virtual Int_t      *GetVoxelCandidates(Int_t i, Int_t j, Int_t k, Int_t &ncheck);
//   virtual Int_t      *GetNextVoxel(Double_t *point, Double_t *dir, Int_t &ncheck);
   virtual void        Print(Option_t *option="") const;
   virtual void        Voxelize(Option_t *option);

   ClassDef(TGeoFullVoxels, 1)                // full voxels class
};

/*************************************************************************
 * TGeoUniformVoxels - voxelization based on equidistant slicing
 *  
 *************************************************************************/

class TGeoUniformVoxels : public TGeoVoxelFinder
{
private:
   Int_t               fNaxis;        // number of axis voxelized
   Int_t               fIaxis[5];     // voxelization axis
   Int_t               fNvoxels[5];   // number of voxels on each axis   
   Double_t            fVoxWidth[5];  // voxel width
   Double_t            fVoxEff[5];    // voxels efficiency
   Double_t           *fMotherLimits[5]; //! mother limits
   Int_t              *fInd[5];          //! arrays of voxels

   TGeoUniformVoxels(const TGeoUniformVoxels&); // Not implemented
   TGeoUniformVoxels& operator=(const TGeoUniformVoxels&); // Not implemented

   Double_t            BuildUniformVoxels(Int_t iaxis, Int_t nforced=0, Bool_t create=kFALSE);
   Bool_t              InRange(Int_t idaughter, Int_t iaxis, Double_t min, Double_t max) const;
   Double_t            ReduceVoxels(Int_t iaxis, Int_t nvoxmax, Double_t vqmin);
public:
   TGeoUniformVoxels();
   TGeoUniformVoxels(TGeoVolume *vol);
   virtual ~TGeoUniformVoxels();
   
//   virtual void        BuildVoxelLimits();
   virtual void        CreateCheckList();
//   virtual Double_t    Efficiency();
//   virtual void        FindOverlaps(Int_t inode) const;
   virtual Int_t      *GetCheckList(Double_t *point, Int_t &nelem);
   void                IntersectAndGetExtra(UChar_t *array1=0, UChar_t *array2=0, UChar_t *array3=0);
   Bool_t              IsCrossedVoxel(Double_t *min, Double_t *max, Int_t id) const;
//   virtual Bool_t      GetNextIndices(Double_t *point, Double_t *dir);
//   virtual Int_t      *GetVoxelCandidates(Int_t i, Int_t j, Int_t k, Int_t &ncheck);
   virtual Int_t      *GetNextVoxel(Double_t *point, Double_t *dir, Int_t &ncheck);
   Int_t               GetVoxelWithExtra(Int_t iaxis, Int_t ivoxel, Int_t inc) const;
   virtual void        Print(Option_t *option="") const;
   virtual void        SortCrossedVoxels(Double_t *point, Double_t *dir);
   virtual void        Voxelize(Option_t *option="");

   ClassDef(TGeoUniformVoxels, 1)                // uniform slicing voxels class
};

#endif
