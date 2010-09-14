// @(#)root/proofx:$Id$
// Author: Sangsu Ryu 22/6/2010

/*************************************************************************
 * Copyright (C) 1995-2005, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// TProofBenchFileGenerator                                             //
//                                                                      //
// This class lets you generate files and register them as datasets     //
// to be used for Proof benchmark test.                                 //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

#include "TProofBenchFileGenerator.h"
#include "TProofBenchMode.h"
#include "TProofNode.h"
#include "TFile.h"
#include "TFileCollection.h"
#include "TFileInfo.h"
#include "TProof.h"
#include "TString.h"
#include "TDSet.h"
#include "Riostream.h"
#include "TMap.h"
#include "TEnv.h"
#include "TTree.h"
#include "TH1.h"
#include "TLeaf.h"
#include "TCanvas.h"
#include "TStyle.h"
#include "TPaveText.h"
#include "TProfile.h"
#include "TLegend.h"
#include "TKey.h"
#include "TMap.h"
#include "TPerfStats.h"
#include "TParameter.h"
#include "TDrawFeedback.h"
#include "TSortedList.h"
#include "TError.h"

ClassImp(TProofBenchFileGenerator)

//______________________________________________________________________________
TProofBenchFileGenerator::TProofBenchFileGenerator(TProofBenchMode* mode,
                                         Long64_t nevents,
                                         Int_t maxnworkers,
                                         Int_t start,
                                         Int_t stop,
                                         Int_t step,
                                         TString basedir,
                                         Int_t ntracks,
                                         Bool_t regenerate,
                                         TProof* proof)
:fProof(proof),
fMode(mode),
fNEvents(nevents),
fMaxNWorkers(maxnworkers),
fStart(start),
fStop(stop),
fStep(step),
fBaseDir(basedir),
fNTracks(ntracks),
fRegenerate(regenerate),
fNodes(0),
fDataSetGenerated(0)
{
   FillNodeInfo();
   if (maxnworkers==-1){
      SetMaxNWorkers("1x");
   }
   else{
      SetMaxNWorkers(maxnworkers);
   }
   if (stop==-1){
      fStop=fMaxNWorkers;
   }
   if (fBaseDir.IsNull() && gProof){
      fBaseDir=gProof->GetDataPoolUrl();
   }
}

//______________________________________________________________________________
TProofBenchFileGenerator::~TProofBenchFileGenerator(){
   if (fDataSetGenerated){
      delete fDataSetGenerated;
   }
}

//______________________________________________________________________________
Int_t TProofBenchFileGenerator::GenerateFiles(Int_t nf,
                                              Long64_t nevents,
                                              TString basedir,
                                              Int_t regenerate,
                                              Int_t ntracks)
{

   // Generates files on worker nodes for I/O test or for cleanup run
   // Input parameters do not change corresponding data members
   // Data set member (fDataSetGenerated) gets filled up with generated data set elements
   //
   // Input parameters
   //    nf: This parameter is passed to the mode in use.
   //        When -1, data member fNFiles of mode in use is used.
   //    nevents: Number of events in a file
   //             When clean files are generated, this parameter is ignored and files
   //             large enough to clean up node memory are generated.
   //    basedir: Base directory for the files to be generated on the worker nodes. 
   //             When null, data member fBaseDir is used.
   //    regenerate: Regenerate files when >0,
   //                Reuse files if they exist when =0, 
   //                Use data member fRegenerate when ==-1
   //    ntracks: Number of traks in an event. Use data member fNTracks when ==-1
   // Return: 
   //    0 when ok
   //   <0 otherwise
   Int_t nactive_sav;

   if (fProof){
      nactive_sav=fProof->GetParallel();
      fProof->SetParallel(99999);
   }
   else{
      Error("GenerateFiles", "Proof not set, doing nothing");
      return -1;
   }

   // Check input parameters
   if (nf==-1){
      nf=fMode->GetNFiles();
   }
   if (nevents==-1){
      nevents=fNEvents;
   }
      
   if (basedir.IsNull()){
      basedir=fBaseDir;
   }

   if (regenerate==-1){
      regenerate=fRegenerate; 
   }
 
   if (ntracks==-1){
      ntracks=fNTracks;
   }
 
   if (fDataSetGenerated) delete fDataSetGenerated;
   fDataSetGenerated = new TDSet("TTree","EventTree");

   TList* wl=fProof->GetListOfSlaveInfos();

   TSortedList* wlcopy=new TSortedList;
   wlcopy->SetName("PROOF_SlaveInfos");

   TIter nwl(wl);
   TSlaveInfo *si=0;
   while (si=(TSlaveInfo*)nwl()){
      wlcopy->Add(new TSlaveInfo(*si));
   }
   fProof->AddInput(wlcopy);

   // Build file map to generate on worker nodes
   TMap* filesmap=fMode->FilesToProcess(nf);
   // Add the file map in the input list
   fProof->AddInput(filesmap);

   TProofBenchMode::EFileType filetype=fMode->GetFileType();
   fProof->SetParameter("PROOF_BenchmarkFileType", filetype);
   TUrl datapoolurl(basedir);
   TString datadir=datapoolurl.GetFile();
   fProof->SetParameter("PROOF_BenchmarkBaseDir", datadir.Data());
   fProof->SetParameter("PROOF_BenchmarkNEvents", nevents);
   fProof->SetParameter("PROOF_BenchmarkNTracks", ntracks);
   fProof->SetParameter("PROOF_BenchmarkRegenerate", regenerate);

   // Set the packetizer to be the one on test
   fProof->SetParameter("PROOF_Packetizer", "TPacketizerFile");

   // Run
   Int_t entries=filesmap->GetSize();
   fProof->Process("TSelEventGen", entries);

   // Check outputs
   TList* l = fProof->GetOutputList();

   if (!l){
      Error("GenerateFiles", "list of output not found");
      // Put it back to old configuration
      fProof->SetParallel(nactive_sav);
      return -1;
   }

   // Merge outputs
   TObject* obj;
   TList* lfilesgenerated;
   TDSet* tdset;
   TString outputname, hostname, filename, newfilename;

   TIter nxt(l);
   while((obj=nxt())){
      outputname=obj->GetName();
      if (outputname.Contains("PROOF_FilesGenerated")){
         lfilesgenerated=dynamic_cast<TList*>(obj);
         if (lfilesgenerated){
            tdset=dynamic_cast<TDSet*>(lfilesgenerated->At(0)); // lfilesgenerated is 1-element list
            fDataSetGenerated->Add(tdset);
         }
         else{
            Error("GenerateFiles", "%s not type TList*; moving to the next list", outputname.Data());
            continue;
         }
      }
   }

   // Clear input parameters
   fProof->DeleteParameters("PROOF_BenchmarkFileType");
   fProof->DeleteParameters("PROOF_BenchmarkBaseDir");
   fProof->DeleteParameters("PROOF_BenchmarkNEvents");
   fProof->DeleteParameters("PROOF_BenchmarkRegenerate");
   fProof->DeleteParameters("PROOF_BenchmarkNTracks");

   fProof->DeleteParameters("PROOF_Packetizer");
   fProof->GetInputList()->Remove(filesmap);
   filesmap->SetOwner(kTRUE);
   SafeDelete(filesmap);
   fProof->GetInputList()->Remove(wlcopy);
   wlcopy->SetOwner(kTRUE);
   SafeDelete(wlcopy);

   // Put it back to old configuration
   fProof->SetParallel(nactive_sav);

   // Sort output list
   TList* lds=0;
   lds=fDataSetGenerated->GetListOfElements();
   lds->Sort();

   // Print outputs
   TIter nds(lds);
   TDSetElement* dse=0;
   Info("GenerateFiles", "List of files generarted:");
   while (dse=(TDSetElement*)nds()){
      dse->Print("A");
   }
   return 0;
}

//______________________________________________________________________________
Int_t TProofBenchFileGenerator::MakeDataSets(Int_t nf,
                                             Int_t start,
                                             Int_t stop,
                                             Int_t step,
                                             const TDSet* tdset,
                                             const char* option)
{
   // Make data sets out of tdset and register them for benchmark test
   //
   // Input parameters
   //    nf: This parameter is passed to the mode in use.
   //        When -1, data member fNFiles of mode in use is used.
   //    start: Start number of workers, when -1 (default) data member fStart is used
   //    stop: Ending number of workers, when -1 (default) data member fStop is used
   //    step: Incremental number of workers, when -1 (default) data member fStep is used
   //    tdset: Data set (TDSet*) from which file collection will be built in this function. 
   //           When tdset=0, the data set filled up by TProofBenchFileGenerator::GenerateFiles is used.
   //    option: Option to TProof::RegisterDataSet
   // Return: 0 when ok
   //        <0 otherwise

   if (start==-1){
      start=fStart;
      Info("MakeDataSets", "Start number of workers is %d", start);
   }
   if (stop==-1){
       stop=fStop;
       Info("MakeDataSets", "End number of workers is set to %d", stop);
   }
   if (step==-1){
       step=fStep;
       Info("MakeDataSets", "Step number of workers is set to %d", step);
   }
   if (start>stop){
       Error("MakeDataSets", "Wrong inputs; start(%d)>stop(%d)", start, stop);
       return -1;
   }

   TUrl url(fBaseDir);
   if (tdset){
      fMode->MakeDataSets(nf, start, stop, step, tdset, option, &url, fProof);
      return 0;
   }
   else{
      if (fDataSetGenerated){
         fMode->MakeDataSets(nf, start, stop, step, fDataSetGenerated, option, &url, fProof);
      }
      else{
         Error("MakeDataSet", "Empty data set; Either generate files first or supply one.");
         return -1;
      }
   }

   return 0;
}

//______________________________________________________________________________
Int_t TProofBenchFileGenerator::MakeDataSets(const char* option)
{
   // Make data sets and register them for benchmark test
   //
   // Input parameters
   //    option: Option to TProof::RegisterDataSet
   // Return: 0 when ok, 
   //        <0 otherwise

   if ( fDataSetGenerated ) {
      TUrl url(fBaseDir);
      fMode->MakeDataSets(-1, fStart, fStop, fStep, fDataSetGenerated, option, &url, fProof);
   }
   else{
      Error("MakeDataSet", "Generate files first");
      return -1;
   }

   return 0;
}

//______________________________________________________________________________
Int_t TProofBenchFileGenerator::MakeDataSets(Int_t nf,
                                             Int_t np,
                                             const Int_t *wp,
                                             const TDSet* tdset,
                                             const char* option)
{
   // Create the data sets out of tdset and register them for benchmark test
   //
   // Input parameters:
   //    nf: This parameter is passed to the mode in use.
   //        When -1, data member fNFiles of mode in use is used.
   //    np: Number of test points
   //    wp: Array of number of workers for test points (should contain np points)
   //    tdset: Data set (TDSet*) from which file collection will be built in this function. 
   //           When tdset=0, the data set filled up by TProofBenchFileGenerator::GenerateFiles is used.
   //    option: Option to TProof::RegisterDataSet
   // Return:
   //    0 when ok
   //   <0 otherwise

   TUrl url(fBaseDir);
   if (tdset){
      fMode->MakeDataSets(nf, np, wp, tdset, option, &url, fProof);
   }
   else{
      if ( fDataSetGenerated ) {
         fMode->MakeDataSets(nf, np, wp, fDataSetGenerated, option, &url, fProof);
      }
      else{
         Error("MakeDataSet", "Empty data set; Either generate files first or supply one.");
         return -1;
      }
   }
   return 0;
}

//______________________________________________________________________________
void TProofBenchFileGenerator::Print(Option_t* option)const{

   if (fProof) fProof->Print(option);
   if (fMode) fMode->Print(option);
   Printf("fNEvents=%lld", fNEvents);
   Printf("fMaxNWorkers=%d", fMaxNWorkers);
   Printf("fStart=%d", fStart);
   Printf("fStop=%d", fStop);
   Printf("fStep=%d", fStep);
   Printf("fBaseDir=\"%s\"", fBaseDir.Data()); 
   Printf("fNTracksBench=%d", fNTracks);
   Printf("fRegenerate=%d", fRegenerate);
   if (fNodes) fNodes->Print(option);
   if (fDataSetGenerated) fDataSetGenerated->Print(option);
}

//______________________________________________________________________________
void TProofBenchFileGenerator::SetMode(TProofBenchMode* mode)
{
   fMode=mode;
}

//______________________________________________________________________________
void TProofBenchFileGenerator::SetNEvents(Long64_t nevents)
{
   fNEvents=nevents;
}

//______________________________________________________________________________
void TProofBenchFileGenerator::SetMaxNWorkers(Int_t maxnworkers)
{
   fMaxNWorkers=maxnworkers;
}

//______________________________________________________________________________
void TProofBenchFileGenerator::SetMaxNWorkers(TString sworkers)
{
   sworkers.ToLower();
   sworkers.Remove(TString::kTrailing, ' ');
   if (fProof){
      if (sworkers.Contains("x")){
         TList* lslave=fProof->GetListOfSlaveInfos();
         // Number of slave workers regardless of its status, active or inactive
         Int_t nslaves=lslave->GetSize();  
         sworkers.Remove(TString::kTrailing, 'x');
         Int_t mult=sworkers.Atoi();
         // This number to be parameterized in the future
         fMaxNWorkers=mult*nslaves; 
      }
   }
   else{
      Error("SetMaxNWorkers", "Proof not set, doing nothing");
   }
   return;
}

//______________________________________________________________________________
void TProofBenchFileGenerator::SetStart(Int_t start)
{
   fStart=start;
}

//______________________________________________________________________________
void TProofBenchFileGenerator::SetStop(Int_t stop)
{
   fStop=stop;
}

//______________________________________________________________________________
void TProofBenchFileGenerator::SetStep(Int_t step)
{
   fStep=step;
}

//______________________________________________________________________________
void TProofBenchFileGenerator::SetBaseDir(TString basedir)
{
   fBaseDir=basedir;
}

//______________________________________________________________________________
void TProofBenchFileGenerator::SetNTracks(Int_t ntracks)
{
   fNTracks=ntracks;
}

//______________________________________________________________________________
void TProofBenchFileGenerator::SetRegenerate(Int_t regenerate)
{
   fRegenerate=regenerate;
}

//______________________________________________________________________________
TProofBenchMode* TProofBenchFileGenerator::GetMode()const
{
   return fMode;
}

//______________________________________________________________________________
Long64_t TProofBenchFileGenerator::GetNEvents()const
{
   return fNEvents;
}

//______________________________________________________________________________
Int_t TProofBenchFileGenerator::GetMaxNWorkers()const
{
   return fMaxNWorkers;
}

//______________________________________________________________________________
Int_t TProofBenchFileGenerator::GetStart()const
{
   return fStart;
}

//______________________________________________________________________________
Int_t TProofBenchFileGenerator::GetStop()const
{
   return fStop;
}

//______________________________________________________________________________
Int_t TProofBenchFileGenerator::GetStep()const
{
   return fStep;
}

//______________________________________________________________________________
TString TProofBenchFileGenerator::GetBaseDir()const
{
   return fBaseDir;
}

//______________________________________________________________________________
Int_t TProofBenchFileGenerator::GetNTracks()const
{
   return fNTracks;
}

//______________________________________________________________________________
Int_t TProofBenchFileGenerator::GetRegenerate()const
{
   return fRegenerate;
}

//______________________________________________________________________________
Int_t TProofBenchFileGenerator::FillNodeInfo()
{
   // Re-Generate the list of worker node info (fNodes)
   // Return 0 if OK, -1 if proof not set, -2 if info could not be retrieved
   // (the existing info is always removed)
   // Return:
   //     0 when ok
   //    <0 otherwise

   if (!fProof){
      Error("FillNodeInfo", "proof not set, doing nothing");
      return -1;
   }

   if (fNodes) {
      fNodes->SetOwner(kTRUE);
      SafeDelete(fNodes);
   }

   fNodes = new TList;

   // Get info
   TList *wl = fProof->GetListOfSlaveInfos();
   if (!wl) {
      Error("FillNodeInfo", "could not get information about workers!");
      return -2;
   }

   TIter nxwi(wl);
   TSlaveInfo *si = 0;
   TProofNode *ni = 0;
   while ((si = (TSlaveInfo *) nxwi())) {
      if (!(ni = (TProofNode *) fNodes->FindObject(si->GetName()))) {
         ni = new TProofNode(si->GetName(), si->GetSysInfo().fPhysRam);
         fNodes->Add(ni);
      } else {
         ni->AddWrks(1);
      }
   }
   // Notify
   Info("FillNodeInfo","%d physically different mahcines found", fNodes->GetSize());
   // Done
   return 0;
}
