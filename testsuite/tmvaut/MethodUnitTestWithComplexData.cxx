#include "MethodUnitTestWithComplexData.h"
#include "TFile.h"
#include "TMVA/MethodBase.h"

using namespace std;
using namespace UnitTesting;
using namespace TMVA;

MethodUnitTestWithComplexData::MethodUnitTestWithComplexData(const TString& treestring, const TString& preparestring, const Types::EMVA& theMethod, const TString& methodTitle, const TString& theOption,
														double lowLimit, double upLimit,
                                                             const std::string & /* xname */ ,const std::string & /* filename */ , std::ostream* /* sptr */) :
   UnitTest(string("ComplexData_")+(string)methodTitle+(string)treestring, __FILE__),  _methodType(theMethod) , _treeString(treestring), _prepareString(preparestring), _methodTitle(methodTitle), _methodOption(theOption), _upROCLimit(upLimit), _lowROCLimit(lowLimit)
{
}


MethodUnitTestWithComplexData::~MethodUnitTestWithComplexData()
{
}

bool MethodUnitTestWithComplexData::ROCIntegralWithinInterval()
{
   return (_ROCValue <= _upROCLimit) && (_ROCValue >= _lowROCLimit);
}

void MethodUnitTestWithComplexData::run()
{
	// FIXME:: create _this_ file or rather somewhere else?
  TString outfileName( "weights/TMVA.root" );
  TFile* outputFile = TFile::Open( outfileName, "RECREATE" );

// FIXME:: if file can't be created do something more?
  if(!outputFile)
    return;    

// FIXME:: make the factory option mutable?
// absolute silence options:
  string factoryOptions( "!V:Silent:Transformations=I;D;P;G;N;U:AnalysisType=Classification:!Color:!DrawProgressBar" );

  Factory* factory = new Factory( "TMVAUnitTesting", outputFile, factoryOptions );
  
  factory->AddVariable( "var0",  "Variable 0", 'F' );
  factory->AddVariable( "var1",  "Variable 1", 'F' );
  factory->AddVariable( "var2",  "Variable 2", 'F' );
  factory->AddVariable( "var3",  "Variable 3", 'F' );
  factory->AddSpectator( "is1", 'I' );
  factory->AddSpectator( "evtno", 'I' );
  
  TFile* input(0);
// FIXME:: give the filename of the sample somewhere else?
  TString fname = "weights/tmva_complex_data.root"; 
  input = TFile::Open( fname );  
  if (input == NULL) create_data("weights/tmva_complex_data.root");
  input = TFile::Open( fname );  
  if (input == NULL) 
    {
      cerr << "broken/inaccessible input file" << endl;
    }
  
  TTree *sig1     = (TTree*)input->Get("TreeS1");
  TTree *sig2     = (TTree*)input->Get("TreeS2");
  TTree *sigfull  = (TTree*)input->Get("TreeSFull");
  TTree *bgd1     = (TTree*)input->Get("TreeB1");
  TTree *bgd2     = (TTree*)input->Get("TreeB2");
  TTree *bgdfull  = (TTree*)input->Get("TreeBFull");
  
  if (_treeString.Contains("sig1"))    factory->AddSignalTree(sig1);  
  if (_treeString.Contains("sig2"))    factory->AddSignalTree(sig2);  
  if (_treeString.Contains("sigfull")) factory->AddSignalTree(sigfull);
  if (_treeString.Contains("bgd1"))    factory->AddBackgroundTree(bgd1);  
  if (_treeString.Contains("bgd2"))    factory->AddBackgroundTree(bgd2);  
  if (_treeString.Contains("bgdfull")) factory->AddBackgroundTree(bgdfull);

  factory->SetSignalWeightExpression("weight");
  factory->SetBackgroundWeightExpression("weight");
  
  TCut mycuts = "";
  TCut mycutb = "";
  if (_prepareString=="") _prepareString = "nTrain_Signal=200:nTrain_Background=200:SplitMode=Random:NormMode=NumEvents:!V" ;
  factory->PrepareTrainingAndTestTree( mycuts, mycutb, _prepareString);
 
  factory->BookMethod(_methodType, _methodTitle, _methodOption);

  factory->TrainAllMethods();
  factory->TestAllMethods();
  factory->EvaluateAllMethods();

  _theMethod = dynamic_cast<TMVA::MethodBase*> (factory->GetMethod(_methodTitle));

  if (_methodType == TMVA::Types::kCuts) {
     // ToDo make class variable _theEffi
     Double_t err=0.;
     Double_t effi = _theMethod->GetEfficiency("Efficiency:0.1", Types::kTesting,err);
#ifdef COUTDEBUG
     std::cout << "Cuts Signal effi at for Background effi of 0.1 = " << effi<<" low limit="<<_lowROCLimit<<" high limit="<<_upROCLimit<<std::endl;
#endif
     test_(effi <= _upROCLimit && effi>=_lowROCLimit);
  }
  else {
     _ROCValue = _theMethod->GetROCIntegral();
#ifdef COUTDEBUG
     std::cout << "ROC integral = "<<_ROCValue <<" low limit="<<_lowROCLimit<<" high limit="<<_upROCLimit<<std::endl;
#endif     
     test_(ROCIntegralWithinInterval());
  }
  outputFile->Close();
  if (outputFile) delete outputFile;

  delete factory;
}

bool MethodUnitTestWithComplexData::create_data(const char* filename, int nmax)
{
   TFile* dataFile = TFile::Open(filename,"RECREATE");
   int nvar = 4;
   int nsig = 0, nbgd=0;
   Float_t weight=1;
   Float_t xvar[100];
   int is1,evtno=0;
   // create signal and background trees
   TTree* treeS1 = new TTree( "TreeS1", "TreeS1", 1 );
   TTree* treeB1 = new TTree( "TreeB1", "TreeB1", 1 );
   TTree* treeS2 = new TTree( "TreeS2", "TreeS2", 1 );
   TTree* treeB2 = new TTree( "TreeB2", "TreeB2", 1 );
   TTree* treeSFull = new TTree( "TreeSFull", "TreeSFull", 1 );
   TTree* treeBFull = new TTree( "TreeBFull", "TreeBFull", 1 );
   for (Int_t ivar=0; ivar<nvar; ivar++) {
      treeS1->Branch( TString(Form( "var%i", ivar )).Data(), &xvar[ivar], TString(Form( "var%i/F", ivar)).Data() );
      treeB1->Branch( TString(Form( "var%i", ivar )).Data(), &xvar[ivar], TString(Form( "var%i/F", ivar)).Data() );
      treeS2->Branch( TString(Form( "var%i", ivar )).Data(), &xvar[ivar], TString(Form( "var%i/F", ivar)).Data() );
      treeB2->Branch( TString(Form( "var%i", ivar )).Data(), &xvar[ivar], TString(Form( "var%i/F", ivar)).Data() );
      treeBFull->Branch( TString(Form( "var%i", ivar )).Data(), &xvar[ivar], TString(Form( "var%i/F", ivar)).Data() );
      treeSFull->Branch( TString(Form( "var%i", ivar )).Data(), &xvar[ivar], TString(Form( "var%i/F", ivar)).Data() );
   }
   treeS1->Branch("weight", &weight, "weight/F");
   treeB1->Branch("weight", &weight, "weight/F");
   treeS2->Branch("weight", &weight, "weight/F");
   treeB2->Branch("weight", &weight, "weight/F");
   treeSFull->Branch("weight", &weight, "weight/F");
   treeBFull->Branch("weight", &weight, "weight/F");

   treeS1->Branch("is1", &is1, "is1/I");
   treeB1->Branch("is1", &is1, "is1/I");
   treeS2->Branch("is1", &is1, "is1/I");
   treeB2->Branch("is1", &is1, "is1/I");
   treeSFull->Branch("is1", &is1, "is1/I");
   treeBFull->Branch("is1", &is1, "is1/I");

   treeS1->Branch("evtno", &evtno, "evtno/I");
   treeB1->Branch("evtno", &evtno, "evtno/I");
   treeS2->Branch("evtno", &evtno, "evtno/I");
   treeB2->Branch("evtno", &evtno, "evtno/I");
   treeSFull->Branch("evtno", &evtno, "evtno/I");
   treeBFull->Branch("evtno", &evtno, "evtno/I");

   TRandom R( 100 );
   do {
      for (Int_t ivar=0; ivar<nvar-1; ivar++) { xvar[ivar]=2.*R.Rndm()-1.;}
      Float_t xout = xvar[0]+xvar[1]+xvar[2]*xvar[1]-xvar[0]*xvar[1]*xvar[1];
      xvar[3] = xout + 4. *R.Rndm()-2.;
      bool is = (TMath::Abs(xout)<0.3);
      if (is) is1=1; else is1=0;
      bool isSignal;
      if (is) isSignal = (R.Rndm()<0.5);
      else isSignal =  (xout>0);
      //if (nsig<10) cout << "xout = " << xout<< (isSignal? " signal": " bbg")  << endl;
      if (isSignal) {
         treeSFull->Fill();
         if (is) treeS1->Fill();
         else treeS2->Fill();
         nsig++;
         evtno++;
      }
      else {
         treeBFull->Fill();
         if (is) treeB1->Fill();
         else treeB2->Fill();
         nbgd++;
         evtno++;
      }
   } while ( nsig < nmax || nbgd < nmax);

   dataFile->Write();
   dataFile->Close();
   return true;
}
