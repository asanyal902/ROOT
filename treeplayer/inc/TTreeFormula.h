// @(#)root/treeplayer:$Name:  $:$Id: TTreeFormula.h,v 1.30 2003/07/07 19:34:04 brun Exp $
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
class TArrayI;
class TMethodCall;
class TLeafObject;
class TDataMember;
class TStreamerElement;
class TFormLeafInfoMultiVarDim;
class TFormLeafInfo;
class TBranchElement;
class TAxis;
class TTreeFormulaManager;

class TTreeFormula : public TFormula {

protected:
   enum { kIsCharacter = BIT(12) };
   enum { kDirect, kDataMember, kMethod, 
          kIndexOfEntry, kEntries, kLength, kIteration };
   enum { kAlias = TFormula::kVariable+10000+1,
          kAliasString = kAlias+1
   };

   TTree       *fTree;            //! pointer to Tree
   Short_t     fCodes[kMAXCODES]; //  List of leaf numbers referenced in formula
   Int_t       fNdata[kMAXCODES]; //! This caches the physical number of element in the leaf or datamember.
   Int_t       fNcodes;           //  Number of leaves referenced in formula
   Bool_t      fHasCast;          //  Record whether the formula contain a cast operation or not
   Int_t       fMultiplicity;     //  Indicator of the variability of the formula
   Int_t       fNindex;           //  Size of fIndex
   Int_t      *fLookupType;       //[fNindex] array indicating how each leaf should be looked-up
   TObjArray   fLeaves;           //!  List of leaf used in this formula.
   TObjArray   fDataMembers;      //!  List of leaf data members
   TObjArray   fMethods;          //!  List of leaf method calls
   TObjArray   fAliases;          //!  List of TTreeFormula for each alias used.
   TObjArray   fLeafNames;        //   List of TNamed describing leaves

   Int_t         fNdimensions[kMAXCODES];              //Number of array dimensions in each leaf
   Int_t         fFixedSizes[kMAXCODES][kMAXFORMDIM];  //Physical sizes of lower dimensions for each leaf
   UChar_t       fHasMultipleVarDim[kMAXCODES];        //True if the corresponding variable is an array with more than one variable dimension.

   //the next line should have a mutable in front. See GetNdata()
   Int_t         fCumulSizes[kMAXCODES][kMAXFORMDIM];  //Accumulated sizes of lower dimensions for each leaf after variable dimensions has been calculated
   Int_t         fIndexes[kMAXCODES][kMAXFORMDIM];    //Index of array selected by user for each leaf
   TTreeFormula *fVarIndexes[kMAXCODES][kMAXFORMDIM]; //Pointer to a variable index.

   void        DefineDimensions(Int_t code, Int_t size, TFormLeafInfoMultiVarDim * info, Int_t& virt_dim);

   Int_t       RegisterDimensions(Int_t code, Int_t size, TFormLeafInfoMultiVarDim * multidim = 0);
   Int_t       RegisterDimensions(Int_t code, TBranchElement *branch);
   Int_t       RegisterDimensions(Int_t code, TFormLeafInfo *info);
   Int_t       RegisterDimensions(Int_t code, TLeaf *leaf);
   Int_t       RegisterDimensions(const char *size, Int_t code);

   virtual Double_t   GetValueFromMethod(Int_t i, TLeaf *leaf) const;
   virtual void*      GetValuePointerFromMethod(Int_t i, TLeaf *leaf) const;
   Int_t       GetRealInstance(Int_t instance, Int_t codeindex);
   
   TLeaf*      GetLeafWithDatamember(const char* topchoice, const char* nextchice, UInt_t readentry) const;
   Bool_t      BranchHasMethod(TLeaf* leaf, TBranch* branch, 
                               const char* method,const char* params, 
                               UInt_t readentry) const;

   TList      *fDimensionSetup; //! list of dimension setups, for delayed creation of the dimension information.
   TAxis      *fAxis;           //! pointer to histogram axis if this is a string

   TTreeFormulaManager *fManager; //! The dimension coordinator.
   friend class TTreeFormulaManager;

   void       ResetDimensions();
   Bool_t     LoadCurrentDim();
   
   virtual Bool_t     IsLeafInteger(Int_t code) const;

   virtual Bool_t     IsString(Int_t oper) const;
   virtual Bool_t     IsLeafString(Int_t code) const;
private:
   // Not implemented yet
   TTreeFormula(const TTreeFormula&);
   TTreeFormula& operator=(const TTreeFormula&);

public:
             TTreeFormula();
             TTreeFormula(const char *name,const char *formula, TTree *tree);
   virtual   ~TTreeFormula();
   virtual Int_t       DefinedVariable(TString &variable);
   virtual TClass*     EvalClass() const;
   virtual Double_t    EvalInstance(Int_t i=0, const char *stringStack[]=0);
   virtual const char *EvalStringInstance(Int_t i=0);
   virtual void*       EvalObject(Int_t i=0);
   // EvalInstance should be const.  See comment on GetNdata()
   TFormLeafInfo      *GetLeafInfo(Int_t code) const;
   TMethodCall        *GetMethodCall(Int_t code) const;
   virtual Int_t       GetMultiplicity() const {return fMultiplicity;}
   virtual TLeaf      *GetLeaf(Int_t n) const;
   virtual Int_t       GetNcodes() const {return fNcodes;}
   virtual Int_t       GetNdata();
   //GetNdata should probably be const.  However it need to cache some information about the actual dimension
   //of arrays, so if GetNdata is const, the variables fUsedSizes and fCumulUsedSizes need to be declared
   //mutable.  We will be able to do that only when all the compilers supported for ROOT actually implemented
   //the mutable keyword. 
   //NOTE: Also modify the code in PrintValue which current goes around this limitation :(
   virtual Bool_t      IsInteger() const;
   virtual Bool_t      IsString() const;
   virtual Bool_t      Notify() { UpdateFormulaLeaves(); return kTRUE; }
   virtual char       *PrintValue(Int_t mode=0) const;
   virtual void        SetAxis(TAxis *axis=0);
   virtual void        SetTree(TTree *tree) {fTree = tree;}
   virtual void        UpdateFormulaLeaves();

   ClassDef(TTreeFormula,8)  //The Tree formula
};

#endif
