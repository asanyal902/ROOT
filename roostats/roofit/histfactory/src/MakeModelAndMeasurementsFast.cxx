// @(#)root/roostats:$Id:  cranmer $
// Author: Kyle Cranmer, Akira Shibata
/*************************************************************************
 * Copyright (C) 1995-2008, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

//_________________________________________________
/*
  BEGIN_HTML
  <p>
  This is a package that creates a RooFit probability density function from ROOT histograms 
  of expected distributions and histograms that represent the +/- 1 sigma variations 
  from systematic effects. The resulting probability density function can then be used
  with any of the statistical tools provided within RooStats, such as the profile 
  likelihood ratio, Feldman-Cousins, etc.  In this version, the model is directly
  fed to a likelihodo ratio test, but it needs to be further factorized.</p>

  <p>
  The user needs to provide histograms (in picobarns per bin) and configure the job
  with XML.  The configuration XML is defined in the file config/Config.dtd, but essentially
  it is organized as follows (see config/Combination.xml and config/ee.xml for examples)</p>

  <ul>
  <li> - a top level 'Combination' that is composed of:</li>
  <ul>
  <li>- several 'Channels' (eg. ee, emu, mumu), which are composed of:</li>
  <ul>
  <li>- several 'Samples' (eg. signal, bkg1, bkg2, ...), each of which has:</li>
  <ul>
  <li> - a name</li>
  <li> - if the sample is normalized by theory (eg N = L*sigma) or not (eg. data driven)</li>
  <li> - a nominal expectation histogram</li>
  <li> - a named 'Normalization Factor' (which can be fixed or allowed to float in a fit)</li>
  <li> - several 'Overall Systematics' in normalization with:</li>
  <ul>
  <li> - a name</li>
  <li> - +/- 1 sigma variations (eg. 1.05 and 0.95 for a 5% uncertainty)</li>
  </ul>
  <li>- several 'Histogram Systematics' in shape with:</li>
  <ul>
  <li>- a name (which can be shared with the OverallSyst if correlated)</li>
  <li>- +/- 1 sigma variational histograms</li>
  </ul>
  </ul>
  </ul>
  <li>- several 'Measurements' (corresponding to a full fit of the model) each of which specifies</li>
  <ul>
  <li>- a name for this fit to be used in tables and files</li>
  <ul>
  <li>      - what is the luminosity associated to the measurement in picobarns</li>
  <li>      - which bins of the histogram should be used</li>
  <li>      - what is the relative uncertainty on the luminosity </li>
  <li>      - what is (are) the parameter(s) of interest that will be measured</li>
  <li>      - which parameters should be fixed/floating (eg. nuisance parameters)</li>
  </ul>
  </ul>
  </ul>
  END_HTML
*/
//


// from std
#include <string>
#include <vector>
#include <map>
#include <iostream>
#include <sstream>

// from root
#include "TFile.h"
#include "TH1F.h"
#include "TDOMParser.h"
#include "TXMLAttr.h"
#include "TString.h"

// from roofit
#include "RooStats/ModelConfig.h"

// from this package
#include "Helper.h"
#include "RooStats/HistFactory/ConfigParser.h"
#include "RooStats/HistFactory/EstimateSummary.h"
#include "RooStats/HistFactory/Measurement.h"
#include "RooStats/HistFactory/HistoToWorkspaceFactoryFast.h"
//#include "RooStats/HistFactory/HistoToWorkspaceFactoryNew.h"
#include "RooStats/HistFactory/HistFactoryException.h"

#include "RooStats/HistFactory/MakeModelAndMeasurementsFast.h"

using namespace RooFit;
using namespace RooStats;
using namespace HistFactory;


/*
  void processMeasurement(string outputFileNamePrefix, 
  vector<string> xml_input, 
  TXMLNode* node,
  vector<string> preprocessFunctions);

  void processMeasurementXML(TXMLNode* node, 
  string& outputFileName, string outputFileNamePrefix, 
  Double_t& nominalLumi, Double_t& lumiRelError, Double_t& lumiError,
  Int_t& lowBin, Int_t& highBin,
  string& rowTitle, string& POI, string& mode,
  vector<string>& systToFix,
  map<string,double>& gammaSyst,
  map<string,double>& uniformSyst,
  map<string,double>& logNormSyst,
  map<string,double>& noSyst,
  bool& exportOnly
  );
*/


// Defined later in this file:



void fastDriver(string input){
  // TO DO:
  // would like to fully factorize the XML parsing.  
  // No clear need to have some here and some in ConfigParser


  // Make the list of measurements and channels
  std::vector< HistFactory::Measurement > measurement_list;
  std::vector< HistFactory::Channel >     channel_list;

  HistFactory::ConfigParser xmlParser;

  measurement_list = xmlParser.GetMeasurementsFromXML( input );

  // Fill them using the XML parser
  // xmlParser.FillMeasurementsAndChannelsFromXML( input, measurement_list, channel_list );

  // At this point, we have all the information we need
  // from the xml files.
  

  // We will make the measurements 1-by-1
  // This part will be migrated to the
  // MakeModelAndMeasurements function,
  // but is here for now.

  
  /* Now setup the measurement */
  // At this point, all we need
  // is the list of measurements
    
  for(unsigned int i = 0; i < measurement_list.size(); ++i) {

    HistFactory::Measurement measurement = measurement_list.at(i);

    measurement.CollectHistograms();

    MakeModelAndMeasurementFast( measurement );

  }

  return;

}


void RooStats::HistFactory::MakeModelAndMeasurementFast( RooStats::HistFactory::Measurement& measurement ) {
  
  /*
  // Add the channels to this measurement
  for( unsigned int chanItr = 0; chanItr < channel_list.size(); ++chanItr ) {
  measurement.channels.push_back( channel_list.at( chanItr ) );
  }
  */

  try {

    std::cout << "Making Model and Measurements (Fast) for measurement: " << measurement.GetName() << std::endl;

    double lumiError = measurement.GetLumi()*measurement.GetLumiRelErr();

    std::cout << "using lumi = " << measurement.GetLumi() << " and lumiError = " << lumiError
	 << " including bins between " << measurement.GetBinLow() << " and " << measurement.GetBinHigh() << std::endl;
    std::cout << "fixing the following parameters:"  << std::endl;

    for(vector<string>::iterator itr=measurement.GetConstantParams().begin(); itr!=measurement.GetConstantParams().end(); ++itr){
      cout << "   " << *itr << endl;
    }
  
    /***
	Construction of Model. Only requirement is that they return vector<vector<EstimateSummary> >
	This is where we use the factory.
    ***/
    /*  
	vector<vector<EstimateSummary> > summaries;
	if(xml_input.empty()){
	cerr << "no input channels found" << endl;
	exit(1);
	}
    */
    vector<string> preprocessFunctions;

    std::string rowTitle = measurement.GetName();
    std::string outputFileName = measurement.GetOutputFilePrefix() + "_" + measurement.GetName() + ".root";
    
    
    vector<RooWorkspace*> channel_workspaces;
    vector<string>        channel_names;

    std::cout << "Creating the output file: " << outputFileName << std::endl;

    TFile* outFile = new TFile(outputFileName.c_str(), "recreate");

    std::cout << "Creating the HistoToWorkspaceFactoryFast factory" << std::endl;

    // USING OLD VERSION...
    HistoToWorkspaceFactoryFast factory(measurement.GetOutputFilePrefix(), rowTitle, measurement.GetConstantParams(), 
					measurement.GetLumi(), lumiError, 
					measurement.GetBinLow(), measurement.GetBinHigh(), outFile);
    

    std::cout << "Setting preprocess functions" << std::endl;

    // Make the factory, and do some preprocessing
    // HistoToWorkspaceFactoryFast factory(measurement, rowTitle, outFile);
    factory.SetFunctionsToPreprocess( measurement.GetPreprocessFunctions() );

  
    // for results tables
    fprintf(factory.pFile, " %s &", rowTitle.c_str() );
  
    /***
	First: Loop to make the individual channels
    ***/


    for( unsigned int chanItr = 0; chanItr < measurement.GetChannels().size(); ++chanItr ) {
    
      HistFactory::Channel& channel = measurement.GetChannels().at( chanItr );

      if( ! channel.CheckHistograms() ) {
	std::cout << "MakeModelAndMeasurementsFast: Channel: " << channel.GetName()
		  << " has uninitialized histogram pointers" << std::endl;
	throw bad_hf;
	exit(-1);
      }

      string ch_name = channel.GetName();
      channel_names.push_back(ch_name);

      std::cout << "Starting to process channel: " << ch_name << std::endl;

      //channel.CollectHistograms();

      vector<EstimateSummary> channel_estimateSummary;

      // Okay, let's fill this crappy thing:

      std::cout << "Processing data: " << std::endl;

      // Add the data
      EstimateSummary data_es;
      data_es.name = "Data";
      data_es.channel = channel.GetName();
      data_es.nominal = (TH1*) channel.GetData().GetHisto()->Clone();
      channel_estimateSummary.push_back( data_es );

      // Add the samples
      for( unsigned int sampleItr = 0; sampleItr < channel.GetSamples().size(); ++sampleItr ) {

	EstimateSummary sample_es;
	RooStats::HistFactory::Sample& sample = channel.GetSamples().at( sampleItr );

	std::cout << "Processing sample: " << sample.GetName() << std::endl;

	// Define the mapping
	sample_es.name = sample.GetName();
	sample_es.channel = sample.GetChannelName();
	sample_es.nominal = (TH1*) sample.GetHisto()->Clone();

	std::cout << "Checking NormalizeByTheory" << std::endl;

	if( sample.GetNormalizeByTheory() ) {
	  sample_es.normName = "" ; // Really bad, confusion convention
	}
	else {
	  TString lumiStr;
	  lumiStr += measurement.GetLumi();
	  lumiStr.ReplaceAll(' ', TString());
	  sample_es.normName = lumiStr ;
	}

	std::cout << "Setting the Histo Systs" << std::endl;

	// Set the Histo Systs:
	for( unsigned int histoItr = 0; histoItr < sample.GetHistoSysList().size(); ++histoItr ) {

	  RooStats::HistFactory::HistoSys& histoSys = sample.GetHistoSysList().at( histoItr );

	  sample_es.systSourceForHist.push_back( histoSys.GetName() );
	  sample_es.lowHists.push_back( (TH1*) histoSys.GetHistoLow()->Clone()  );
	  sample_es.highHists.push_back( (TH1*) histoSys.GetHistoHigh()->Clone() );

	}

	std::cout << "Setting the NormFactors" << std::endl;

	for( unsigned int normItr = 0; normItr < sample.GetNormFactorList().size(); ++normItr ) {

	  RooStats::HistFactory::NormFactor& normFactor = sample.GetNormFactorList().at( normItr );

	  EstimateSummary::NormFactor normFactor_es;
	  normFactor_es.name = normFactor.GetName();
	  normFactor_es.val  = normFactor.GetVal();
	  normFactor_es.high = normFactor.GetHigh();
	  normFactor_es.low  = normFactor.GetLow();
	  normFactor_es.constant = normFactor.GetConst();
	  

	  sample_es.normFactor.push_back( normFactor_es );

	}

	std::cout << "Setting the OverallSysList" << std::endl;

	for( unsigned int sysItr = 0; sysItr < sample.GetOverallSysList().size(); ++sysItr ) {

	  RooStats::HistFactory::OverallSys& overallSys = sample.GetOverallSysList().at( sysItr );

	  std::pair<double, double> DownUpPair( overallSys.GetLow(), overallSys.GetHigh() );
	  sample_es.overallSyst[ overallSys.GetName() ]  = DownUpPair; //

	}

	std::cout << "Checking Stat Errors" << std::endl;

	// Do Stat Error
	sample_es.IncludeStatError  = sample.GetStatError().GetActivate();

	// Set the error and error threshold
	sample_es.RelErrorThreshold = channel.GetStatErrorConfig().GetRelErrorThreshold();
	if( sample.GetStatError().GetErrorHist() ) {
	  sample_es.relStatError      = (TH1*) sample.GetStatError().GetErrorHist()->Clone();
	}
	else {
	  sample_es.relStatError    = NULL;
	}


	// Set the constraint type;
	Constraint::Type type = channel.GetStatErrorConfig().GetConstraintType();

	// Set the default
	sample_es.StatConstraintType = EstimateSummary::Gaussian;

	if( type == Constraint::Gaussian) {
	  std::cout << "Using Gaussian StatErrors" << std::endl;
	  sample_es.StatConstraintType = EstimateSummary::Gaussian;
	}
	if( type == Constraint::Poisson ) {
	  std::cout << "Using Poisson StatErrors" << std::endl;
	  sample_es.StatConstraintType = EstimateSummary::Poisson;
	}


	std::cout << "Getting the shape Factor" << std::endl;

	// Get the shape factor
	if( sample.GetShapeFactorList().size() > 0 ) {
	  sample_es.shapeFactorName = sample.GetShapeFactorList().at(0).GetName();
	}
	if( sample.GetShapeFactorList().size() > 1 ) {
	  std::cout << "Error: Only One Shape Factor currently supported" << std::endl;
	  throw bad_hf;
	}


	std::cout << "Setting the ShapeSysts" << std::endl;

	// Get the shape systs:
	for( unsigned int shapeItr=0; shapeItr < sample.GetShapeSysList().size(); ++shapeItr ) {

	  RooStats::HistFactory::ShapeSys& shapeSys = sample.GetShapeSysList().at( shapeItr );

	  EstimateSummary::ShapeSys shapeSys_es;
	  shapeSys_es.name = shapeSys.GetName();
	  shapeSys_es.hist = shapeSys.GetErrorHist();

	  // Set the constraint type;
	  Constraint::Type systype = shapeSys.GetConstraintType();

	  // Set the default
	  shapeSys_es.constraint = EstimateSummary::Gaussian;

	  if( systype == Constraint::Gaussian) {
	    shapeSys_es.constraint = EstimateSummary::Gaussian;
	  }
	  if( systype == Constraint::Poisson ) {
	    shapeSys_es.constraint = EstimateSummary::Poisson;
	  }

	  sample_es.shapeSysts.push_back( shapeSys_es );

	}

	std::cout << "Adding this sample" << std::endl;

	// Push back
	channel_estimateSummary.push_back( sample_es );

      }

      //std::vector< EstimateSummary > dummy;
      RooWorkspace * ws = factory.MakeSingleChannelModel(channel_estimateSummary, measurement.GetConstantParams());
      channel_workspaces.push_back(ws);

      // set poi in ModelConfig
      ModelConfig * proto_config = (ModelConfig *) ws->obj("ModelConfig");
      cout << "Setting Parameter of Interest as :" << measurement.GetPOI() << endl;
      RooRealVar* poi = (RooRealVar*) ws->var( (measurement.GetPOI()).c_str() );
      RooArgSet * params= new RooArgSet;
      if(poi){
	params->add(*poi);
      }
      proto_config->SetParametersOfInterest(*params);
    

      // Gamma/Uniform Constraints:
      // turn some Gaussian constraints into Gamma/Uniform/LogNorm constraints, rename model newSimPdf
      if( measurement.GetGammaSyst().size()>0 || measurement.GetUniformSyst().size()>0 || measurement.GetLogNormSyst().size()>0) {
	factory.EditSyst( ws, ("model_"+ch_name).c_str(), measurement.GetGammaSyst(), measurement.GetUniformSyst(), measurement.GetLogNormSyst(), measurement.GetNoSyst());
	proto_config->SetPdf( *ws->pdf("newSimPdf") );
      }
    
      // fill out ModelConfig and export
      RooAbsData* expData = ws->data("asimovData");
      if(poi){
	proto_config->GuessObsAndNuisance(*expData);
      }
      //ws->writeToFile((measurement.OutputFilePrefix+"_"+ch_name+"_"+rowTitle+"_model.root").c_str());
      std::string ChannelFileName = measurement.GetOutputFilePrefix() + "_" + ch_name + "_" + rowTitle + "_model.root";
      ws->writeToFile( ChannelFileName.c_str() );
    
      // Now, write the measurement to the file
      // Make a new measurement for only this channel
      RooStats::HistFactory::Measurement meas_chan( measurement );
      meas_chan.GetChannels().clear();
      meas_chan.GetChannels().push_back( channel );
      std::cout << "About to write channel measurement to file" << std::endl;
      TFile* chanFile = TFile::Open( ChannelFileName.c_str(), "UPDATE" );
      meas_chan.writeToFile( chanFile );
      chanFile->Close();

      // do fit unless exportOnly requested
      if(! measurement.GetExportOnly()){
	if(!poi){
	  cout <<"can't do fit for this channel, no parameter of interest"<<endl;
	} else{
	  if(ws->data("obsData")){
	    factory.FitModel(ws, ch_name, "newSimPdf", "obsData", false);
	  } else {
	    factory.FitModel(ws, ch_name, "newSimPdf", "asimovData", false);
	  }
	}
      
      }
      fprintf(factory.pFile, " & " );
    } // End loop over channels
  
    /***
	Second: Make the combined model:
	If you want output histograms in root format, create and pass it to the combine routine.
	"combine" : will do the individual cross-section measurements plus combination	
    ***/
  

    RooWorkspace* ws=factory.MakeCombinedModel(channel_names, channel_workspaces);
    // Gamma/Uniform Constraints:
    // turn some Gaussian constraints into Gamma/Uniform/logNormal/noConstraint constraints, rename model newSimPdf
    if( measurement.GetGammaSyst().size()>0 || measurement.GetUniformSyst().size()>0 || measurement.GetLogNormSyst().size()>0 || measurement.GetNoSyst().size()) 
      factory.EditSyst(ws, "simPdf", measurement.GetGammaSyst(), measurement.GetUniformSyst(), measurement.GetLogNormSyst(), measurement.GetNoSyst());
    //
    // set parameter of interest according to the configuration
    //
    ModelConfig * combined_config = (ModelConfig *) ws->obj("ModelConfig");
    cout << "Setting Parameter of Interest as :" << measurement.GetPOI() << endl;
    RooRealVar* poi = (RooRealVar*) ws->var( (measurement.GetPOI()).c_str() );
    //RooRealVar* poi = (RooRealVar*) ws->var((POI+"_comb").c_str());
    RooArgSet * params= new RooArgSet;
    //    cout << poi << endl;
    if(poi){
      params->add(*poi);
    }
    combined_config->SetParametersOfInterest(*params);
    ws->Print();
    
    // Set new PDF if there are gamma/uniform constraint terms
    // Check if we want to check for "GetNoSyst"
    if( measurement.GetGammaSyst().size()>0 || measurement.GetUniformSyst().size()>0 || measurement.GetLogNormSyst().size()>0 || measurement.GetNoSyst().size()>0) 
      combined_config->SetPdf(*ws->pdf("newSimPdf"));

    RooAbsData* simData = ws->data("asimovData");
    combined_config->GuessObsAndNuisance(*simData);
    //	  ws->writeToFile(("results/model_combined_edited.root").c_str());
    //ws->writeToFile((measurement.OutputFilePrefix+"_combined_"+rowTitle+"_model.root").c_str());
    std::string CombinedFileName = measurement.GetOutputFilePrefix()+"_combined_"+rowTitle+"_model.root";
    ws->writeToFile( CombinedFileName.c_str() );
    std::cout << "About to write combined measurement to file" << std::endl;
    TFile* combFile = TFile::Open( CombinedFileName.c_str(), "UPDATE" );
    measurement.writeToFile( combFile );
    combFile->Close();

    // TO DO:
    // Totally factorize the statistical test in "fit Model" to a different area
    if(! measurement.GetExportOnly()){
      if(!poi){
	cout <<"can't do fit for this channel, no parameter of interest"<<endl;
      } else{
	if(ws->data("obsData")){
	  factory.FitModel(ws, "combined", "simPdf", "obsData", false);
	} else {
	  factory.FitModel(ws, "combined", "simPdf", "asimovData", false);
	}
      }
    }
  
    fprintf(factory.pFile, " \\\\ \n");

    outFile->Close();
    delete outFile;

  }
  catch(exception& e)
    {
      std::cout << e.what() << std::endl;
      exit(-1);
    }

  return;


} // end loop over measurements




/*
// THIS IS THE OBSOLETE VERSION::
// main is int MakeModelAndMeasurements
void fastDriver(string input){
  // TO DO:
  // would like to fully factorize the XML parsing.  
  // No clear need to have some here and some in ConfigParser

  / *** read in the input xml *** /
  TDOMParser xmlparser;
  Int_t parseError = xmlparser.ParseFile( input.c_str() );
  if( parseError ) { 
    std::cerr << "Loading of xml document \"" << input
          << "\" failed" << std::endl;
  } 

  cout << "reading input : " << input << endl;
  TXMLDocument* xmldoc = xmlparser.GetXMLDocument();
  TXMLNode* rootNode = xmldoc->GetRootNode();

  // require combination 
  if( rootNode->GetNodeName() != TString( "Combination" ) ){ return; }

  string outputFileNamePrefix;
  vector<string> preprocessFunctions;
  vector<string> xml_input;
  
  TListIter attribIt = rootNode->GetAttributes();
  TXMLAttr* curAttr = 0;
  while( ( curAttr = dynamic_cast< TXMLAttr* >( attribIt() ) ) != 0 ) {
    if( curAttr->GetName() == TString( "OutputFilePrefix" ) ) {
      outputFileNamePrefix=string(curAttr->GetValue());
      cout << "output file prefix : " << outputFileNamePrefix << endl;
    }
  } 
  TXMLNode* node = rootNode->GetChildren();
  while( node != 0 ) {
    if( node->GetNodeName() == TString( "Input" ) ) { xml_input.push_back(node->GetText()); }
    if( node->GetNodeName() == TString( "Function" ) ) { 
      preprocessFunctions.push_back(ParseFunctionConfig(node ) ); 
    }
    node = node->GetNextNode();
  }
  
  / * process each xml node (= channel) * /
  node = rootNode->GetChildren();
  while( node != 0 ) {
    if( node->GetNodeName() != TString( "Measurement" ) ) {  node = node->GetNextNode(); continue; }
    processMeasurement(outputFileNamePrefix,xml_input,node,preprocessFunctions); // MB : I moved this to a separate function
    node = node->GetNextNode(); // next measurement
  } 
}
*/


/*
void processMeasurement(string outputFileNamePrefix, vector<string> xml_input, TXMLNode* node,
			vector<string> preprocessFunctions)
{  
  string outputFileName;
  Double_t nominalLumi=0, lumiRelError=0, lumiError=0;
  Int_t lowBin=0, highBin=0;
  string rowTitle, POI, mode;
  vector<string> systToFix;
  map<string,double> gammaSyst, uniformSyst, logNormSyst, noSyst;
  bool exportOnly = false;
  
  cout << "Now processing measurement " << endl;

  / * first interpret the specific measurement * /   // MB : I moved this to a separate function
  processMeasurementXML(node, 
			outputFileName, outputFileNamePrefix,
			nominalLumi, lumiRelError, lumiError,
			lowBin, highBin,
			rowTitle, POI, mode,
			systToFix,
			gammaSyst,
			uniformSyst,
			logNormSyst,
			noSyst,
			exportOnly
			);
  
  / * Now setup the measurement * /
  cout << "using lumi = " << nominalLumi << " and lumiError = " << lumiError
       << " including bins between " << lowBin << " and " << highBin << endl;
  cout << "fixing the following parameters:"  << endl;
  for(vector<string>::iterator itr=systToFix.begin(); itr!=systToFix.end(); ++itr){
    cout << "   " << *itr << endl;
  }
  
  / ***
      Construction of Model. Only requirement is that they return vector<vector<EstimateSummary> >
      This is where we use the factory.
  *** /
  
  vector<vector<EstimateSummary> > summaries;
  if(xml_input.empty()){
    cerr << "no input channels found" << endl;
    exit(1);
  }
  
  
  vector<RooWorkspace*> chs;
  vector<string> ch_names;
  TFile* outFile = new TFile(outputFileName.c_str(), "recreate");
  HistoToWorkspaceFactoryFast factory(outputFileNamePrefix, rowTitle, systToFix, nominalLumi, lumiError, lowBin, highBin , outFile);
  factory.SetFunctionsToPreprocess(preprocessFunctions);
  
  // for results tables
  fprintf(factory.pFile, " %s &", rowTitle.c_str() );
  
  / ***
      First: Loop to make the individual channels
  *** /

  for(vector<string>::iterator itr=xml_input.begin(); itr!=xml_input.end(); ++itr) {

    vector<EstimateSummary> oneChannel;
    // read xml
    ReadXmlConfig(*itr, oneChannel, nominalLumi);
    // not really needed anymore
    summaries.push_back(oneChannel);
    // use factory to create the workspace
    string ch_name=oneChannel[0].channel;
    ch_names.push_back(ch_name);
    RooWorkspace * ws = factory.MakeSingleChannelModel(oneChannel, systToFix);
    chs.push_back(ws);
    // set poi in ModelConfig
    ModelConfig * proto_config = (ModelConfig *) ws->obj("ModelConfig");
    cout << "Setting Parameter of Interest as :" << POI << endl;
    RooRealVar* poi = (RooRealVar*) ws->var(POI.c_str());
    RooArgSet * params= new RooArgSet;
    if(poi){
      params->add(*poi);
    }
    proto_config->SetParametersOfInterest(*params);
    
    
    // Gamma/Uniform Constraints:
    // turn some Gaussian constraints into Gamma/Uniform/LogNorm/NoConstraint constraints, rename model newSimPdf
    if(gammaSyst.size()>0 || uniformSyst.size()>0 || logNormSyst.size()>0 || noSyst.size()>0) {
      factory.EditSyst(ws,("model_"+oneChannel[0].channel).c_str(),gammaSyst,uniformSyst,logNormSyst,noSyst);
      proto_config->SetPdf(*ws->pdf("newSimPdf"));
    }
    
    // fill out ModelConfig and export
    RooAbsData* expData = ws->data("asimovData");
    if(poi){
      proto_config->GuessObsAndNuisance(*expData);
    }
    ws->writeToFile((outputFileNamePrefix+"_"+ch_name+"_"+rowTitle+"_model.root").c_str());
    
    // do fit unless exportOnly requested
    if(!exportOnly){
      if(!poi){
	cout <<"can't do fit for this channel, no parameter of interest"<<endl;
      } else{
	if(ws->data("obsData")){
	  factory.FitModel(ws, ch_name, "newSimPdf", "obsData", false);
	} else {
	  factory.FitModel(ws, ch_name, "newSimPdf", "asimovData", false);
	}
      }
      
    }
    fprintf(factory.pFile, " & " );
  } // end channel-creation loop
  
  / ***
      Second: Make the combined model:
      If you want output histograms in root format, create and pass it to the combine routine.
      "combine" : will do the individual cross-section measurements plus combination	
  *** /
  
  if ( true || mode.find("comb")!=string::npos) { // KC: always do comb. need to clean up code

    RooWorkspace* ws=factory.MakeCombinedModel(ch_names,chs);
    // Gamma/Uniform Constraints:
    // turn some Gaussian constraints into Gamma/Uniform/logNormal/noConstraint constraints, rename model newSimPdf
    if(gammaSyst.size()>0 || uniformSyst.size()>0 || logNormSyst.size()>0 || noSyst.size()) 
      factory.EditSyst(ws,"simPdf",gammaSyst,uniformSyst,logNormSyst,noSyst);
    //
    // set parameter of interest according to the configuration
    //
    ModelConfig * combined_config = (ModelConfig *) ws->obj("ModelConfig");
    cout << "Setting Parameter of Interest as :" << POI << endl;
    RooRealVar* poi = (RooRealVar*) ws->var((POI).c_str());
    //RooRealVar* poi = (RooRealVar*) ws->var((POI+"_comb").c_str());
    RooArgSet * params= new RooArgSet;
    //    cout << poi << endl;
    if(poi){
      params->add(*poi);
    }
    combined_config->SetParametersOfInterest(*params);
    ws->Print();
    
    // Set new PDF if there are gamma/uniform constraint terms
    if(gammaSyst.size()>0 || uniformSyst.size()>0 || logNormSyst.size()>0) 
      combined_config->SetPdf(*ws->pdf("newSimPdf"));
    
    RooAbsData* simData = ws->data("asimovData");
    combined_config->GuessObsAndNuisance(*simData);
    //	  ws->writeToFile(("results/model_combined_edited.root").c_str());
    ws->writeToFile((outputFileNamePrefix+"_combined_"+rowTitle+"_model.root").c_str());
    
    // TO DO:
    // Totally factorize the statistical test in "fit Model" to a different area
    if(!exportOnly){
      if(!poi){
	cout <<"can't do fit for this channel, no parameter of interest"<<endl;
      } else{
	if(ws->data("obsData")){
	  factory.FitModel(ws, "combined", "simPdf", "obsData", false);
	} else {
	  factory.FitModel(ws, "combined", "simPdf", "asimovData", false);
	}
      }
    }
  } // end combination of channels
  
  fprintf(factory.pFile, " \\\\ \n");
  
  outFile->Close();
  delete outFile;
}
*/

 /*
void processMeasurementXML(TXMLNode* node, 
			   string& outputFileName, string outputFileNamePrefix, 
			   Double_t& nominalLumi, Double_t& lumiRelError, Double_t& lumiError,
			   Int_t& lowBin, Int_t& highBin,
			   string& rowTitle, string& POI, string& mode,
			   vector<string>& systToFix,
			   map<string,double>& gammaSyst,
			   map<string,double>& uniformSyst,
			   map<string,double>& logNormSyst,
			   map<string,double>& noSyst,
			   bool& exportOnly
			  )
{
  //        TListIter attribIt = node->GetAttributes();
  //        TXMLAttr* curAttr = 0;
  TListIter attribIt = node->GetAttributes();
  TXMLAttr* curAttr = 0;

  while( ( curAttr = dynamic_cast< TXMLAttr* >( attribIt() ) ) != 0 ) {
    if( curAttr->GetName() == TString( "Lumi" ) ) {
      nominalLumi=atof(curAttr->GetValue());
    }
    if( curAttr->GetName() == TString( "LumiRelErr" ) ) {
      lumiRelError=atof(curAttr->GetValue());
    }
    if( curAttr->GetName() == TString( "BinLow" ) ) {
      cout <<"\n WARNING: In -standard_form ignore BinLow and BinHigh, just use all bins"<<endl;
      lowBin=atoi(curAttr->GetValue());
    }
    if( curAttr->GetName() == TString( "BinHigh" ) ) {
      cout <<"\n WARNING: In -standard_form ignore BinLow and BinHigh, just use all bins"<<endl;
      highBin=atoi(curAttr->GetValue());
    }
    if( curAttr->GetName() == TString( "Name" ) ) {
      rowTitle=curAttr->GetValue();
      outputFileName=outputFileNamePrefix+"_"+rowTitle+".root";
    }
    if( curAttr->GetName() == TString( "Mode" ) ) {
      cout <<"\n INFO: Mode attribute is deprecated, will ignore\n"<<endl;
      mode=curAttr->GetValue();
    }
    if( curAttr->GetName() == TString( "ExportOnly" ) ) {
      if(curAttr->GetValue() == TString( "True" ) )
	exportOnly = true;
      else
	exportOnly = false;
    }
  }
  lumiError=nominalLumi*lumiRelError;
  
  TXMLNode* mnode = node->GetChildren();
  while( mnode != 0 ) {
    if( mnode->GetNodeName() == TString( "POI" ) ) {
      POI=mnode->GetText();
    }
    if( mnode->GetNodeName() == TString( "ParamSetting" ) ) {
      //            TListIter attribIt = mnode->GetAttributes();
      //TXMLAttr* curAttr = 0;
      attribIt = mnode->GetAttributes();
      curAttr = 0;
      while( ( curAttr = dynamic_cast< TXMLAttr* >( attribIt() ) ) != 0 ) {
	if( curAttr->GetName() == TString( "Const" ) ) {
	  if(curAttr->GetValue()==TString("True")){
	    AddSubStrings(systToFix, mnode->GetText());
	  }
	}
      }
    }
    if( mnode->GetNodeName() == TString( "ConstraintTerm" ) ) {
      vector<string> syst; string type = ""; double rel = 0;
      AddSubStrings(syst,mnode->GetText());
      //            TListIter attribIt = mnode->GetAttributes();
      //            TXMLAttr* curAttr = 0;
      attribIt = mnode->GetAttributes();
      curAttr = 0;
      while( ( curAttr = dynamic_cast< TXMLAttr* >( attribIt() ) ) != 0 ) {
	if( curAttr->GetName() == TString( "Type" ) ) {
	  type = curAttr->GetValue();
	}
	if( curAttr->GetName() == TString( "RelativeUncertainty" ) ) {
	  rel = atof(curAttr->GetValue());
	}
      }
      if (type=="Gamma" && rel!=0) {
	for (vector<string>::const_iterator it=syst.begin(); it!=syst.end(); it++) gammaSyst[(*it).c_str()] = rel;
      }
      if (type=="Uniform" && rel!=0) {
	for (vector<string>::const_iterator it=syst.begin(); it!=syst.end(); it++) uniformSyst[(*it).c_str()] = rel;
      }
      if (type=="LogNormal" && rel!=0) {
	for (vector<string>::const_iterator it=syst.begin(); it!=syst.end(); it++) logNormSyst[(*it).c_str()] = rel;
      }
      if (type=="NoConstraint") {
	for (vector<string>::const_iterator it=syst.begin(); it!=syst.end(); it++) noSyst[(*it).c_str()] = 1.0; // MB : dummy value
      }
    }
    mnode = mnode->GetNextNode();
  } 

  //cout << "processMeasurementXML() : " << gammaSyst.size() << " " << uniformSyst.size() << " " << logNormSyst.size() << " " << noSyst.size() << endl;
  
  // end of xml processing
}

*/
