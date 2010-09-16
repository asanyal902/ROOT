// @(#)root/proofx:$Id$
// Author: Sangsu Ryu 22/06/2010

/*************************************************************************
 * Copyright (C) 1995-2005, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#ifndef ROOT_TProofBenchModeVaryingNFilesWorker
#define ROOT_TProofBenchModeVaryingNFilesWorker

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// TProofBenchModeVaryingNFilesWorker                                   //
//                                                                      //
// A mode for PROOF benchmark test.                                     //
// In this mode, a given number of files are generated for each worker. //
// During the test, the total number of files to be processed           //
// in the cluster is fNFiles * number_of_active_workers_in_the_cluster. //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

#ifndef ROOT_TProofBenchMode
#include "TProofBenchMode.h"
#endif

#ifndef ROOT_TString
#include "TString.h"
#endif

class TProof;

R__EXTERN TProof *gProof;

class TProofBenchModeVaryingNFilesWorker : public TProofBenchMode {

private:

   TProof* fProof;            //proof
   Int_t fNFiles;             //number of files a node
   TList* fNodes;             //list of nodes
   TString fName;             //name of the mode

protected:

   Int_t FillNodeInfo();

public:

   TProofBenchModeVaryingNFilesWorker(Int_t nfiles=1, TProof* proof=gProof);
   virtual ~TProofBenchModeVaryingNFilesWorker();

   TMap* FilesToProcess(Int_t nf);

   Int_t MakeDataSets(Int_t nf,
                      Int_t start,
                      Int_t stop,
                      Int_t step,
                      const TList* listfiles,
                      const char* option,
                      TProof* proof);
   
   Int_t MakeDataSets(Int_t nf,
                      Int_t np,
                      const Int_t *wp,
                      const TList* listfiles,
                      const char *option,
                      TProof* proof);

   TProofBenchMode::EFileType GetFileType(){return TProofBenchMode::kFileBenchmark;}
   void Print(Option_t* option=0)const;

   void SetProof(TProof* proof);
   void SetNFiles(Int_t nfiles);

   TProof* GetProof()const;
   Int_t GetNFiles()const;
   const char* GetName()const;

   ClassDef(TProofBenchModeVaryingNFilesWorker,0)   //A mode for PROOF benchmark test
};

#endif
