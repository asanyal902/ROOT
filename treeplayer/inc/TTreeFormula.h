// @(#)root/treeplayer:$Name:  $:$Id: TTreeFormula.h,v 1.6 2000/11/21 20:54:17 brun Exp $
// Author: Rene Brun   19/01/96

/*************************************************************************
 * Copyright (C) 1995-2000, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/
// ---------------------------------- TreeFormula.h

#ifndef ROOT_TTreeFormula
#define ROOT_TTreeFormula



//////////////////////////////////////////////////////////////////////////
//                                                                      //
// TTreeFormula                                                         //
//                                                                      //
// The Tree formula class                                               //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

#ifndef ROOT_TFormula
#include "TFormula.h"
#endif

#ifndef ROOT_TLeaf
#include "TLeaf.h"
#endif

#ifndef ROOT_TObjArray
#include "TObjArray.h"
#endif

const Int_t kMAXCODES = kMAXFOUND; // must be the same as kMAXFOUND in TFormula
const Int_t kMAXFORMDIM = 5; // Maximum number of array dimensions support in TTreeFormula

class TTree;
class TMethodCall;
class TLeafObject;

class TTreeFormula : public TFormula {

protected:
   enum { kIsCharacter = BIT(12) };

   TTree       *fTree;            //! pointer to Tree
   Short_t     fCodes[kMAXCODES]; //  List of leaf numbers referenced in formula
   Int_t       fNcodes;           //  Number of leaves referenced in formula
   Int_t       fMultiplicity;     //  Number of array elements in leaves in case of a TClonesArray
   Int_t       fInstance;         //  Instance number for GetValue
   Int_t       fNindex;           //  Size of fIndex
   Int_t       *fIndex;           //[fNindex]array of instances numbers
   TObjArray   fMethods;          //  List of leaf method calls
   
   Int_t       fNdimensions[kMAXCODES];            //Number of array dimensions in each leaf
   Int_t       fCumulSize[kMAXCODES][kMAXFORMDIM]; //Accumulated size of lower dimensions for each leaf
   Int_t       fCumulUsedSize[kMAXFORMDIM+1];      //Accumulated size of lower dimensions as seen for this formula
   Int_t       fIndexes[kMAXCODES][kMAXFORMDIM];   //Index of array selected by user for each leaf

   void        DefineDimensions(const char *size, Int_t code, Int_t& virt_dim);
public:
             TTreeFormula();
             TTreeFormula(const char *name,const char *formula, TTree *tree);
   virtual   ~TTreeFormula();
   virtual Int_t      DefinedVariable(TString &variable);
   virtual Double_t   EvalInstance(Int_t i=0) const;
   TMethodCall       *GetMethodCall(Int_t code) const;
   virtual Int_t      GetMultiplicity() const {return fMultiplicity;}
   virtual TLeaf     *GetLeaf(Int_t n) const;
   virtual Int_t      GetNcodes() const {return fNcodes;}
   virtual Int_t      GetNdata() const;
   virtual Double_t   GetValueLeafObject(Int_t i, TLeafObject *leaf) const;
   virtual char      *PrintValue(Int_t mode=0) const;
   virtual void       SetTree(TTree *tree) {fTree = tree;}

   ClassDef(TTreeFormula,1)  //The Tree formula
};

#endif
