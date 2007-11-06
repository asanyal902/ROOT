#ifdef __CINT__

#pragma link off all globals;
#pragma link off all classes;
#pragma link off all functions;
#pragma link off all namespaces;

#pragma link C++ nestedclass;

#pragma link C++ namespace TMVA;

#pragma link C++ class TMVA::Configurable+; 
#pragma link C++ class TMVA::Event+; 
#pragma link C++ class TMVA::kNN::Event+; 
#pragma link C++ class TMVA::Factory+; 

// the classifiers
#pragma link C++ class TMVA::MethodBase+; 
#pragma link C++ class TMVA::MethodANNBase+; 
#pragma link C++ class TMVA::MethodTMlpANN+; 
#pragma link C++ class TMVA::MethodRuleFit+; 
#pragma link C++ class TMVA::MethodCuts+; 
#pragma link C++ class TMVA::MethodFisher+; 
#pragma link C++ class TMVA::MethodKNN+; 
#pragma link C++ class TMVA::MethodCFMlpANN+; 
#pragma link C++ class TMVA::MethodCFMlpANN_Utils+; 
#pragma link C++ class TMVA::MethodLikelihood+; 
#pragma link C++ class TMVA::MethodVariable+; 
#pragma link C++ class TMVA::MethodHMatrix+; 
#pragma link C++ class TMVA::MethodPDERS+; 
#pragma link C++ class TMVA::MethodBDT+; 
#pragma link C++ class TMVA::MethodSVM+;
#pragma link C++ class TMVA::MethodBayesClassifier+;
#pragma link C++ class TMVA::MethodFDA+;
#pragma link C++ class TMVA::MethodMLP+;
#pragma link C++ class TMVA::MethodCommittee+;
#pragma link C++ class TMVA::MethodSeedDistance+;

// other classes
#pragma link C++ class TMVA::TSpline2+; 
#pragma link C++ class TMVA::TSpline1+; 
#pragma link C++ class TMVA::PDF+; 
#pragma link C++ class TMVA::BinaryTree+; 
#pragma link C++ class TMVA::BinarySearchTreeNode+; 
#pragma link C++ class TMVA::BinarySearchTree+; 
#pragma link C++ class TMVA::Timer+;
#pragma link C++ class TMVA::RootFinder+; 
#pragma link C++ class TMVA::CrossEntropy+; 
#pragma link C++ class TMVA::DecisionTree+; 
#pragma link C++ class TMVA::DecisionTreeNode+; 
#pragma link C++ class TMVA::MisClassificationError+; 
#pragma link C++ class TMVA::Node+; 
#pragma link C++ class TMVA::SdivSqrtSplusB+; 
#pragma link C++ class TMVA::SeparationBase+; 
#pragma link C++ class TMVA::Tools+; 
#pragma link C++ class TMVA::Reader+; 
#pragma link C++ class TMVA::GeneticAlgorithm+; 
#pragma link C++ class TMVA::GeneticGenes+; 
#pragma link C++ class TMVA::GeneticPopulation+; 
#pragma link C++ class TMVA::GeneticRange+; 
#pragma link C++ class TMVA::GiniIndex+; 
#pragma link C++ class TMVA::SimulatedAnnealing+;
#pragma link C++ class TMVA::TNeuron+;
#pragma link C++ class TMVA::TSynapse+;
#pragma link C++ class TMVA::TActivationChooser+;
#pragma link C++ class TMVA::TActivation+;
#pragma link C++ class TMVA::TActivationSigmoid+;
#pragma link C++ class TMVA::TActivationIdentity+;
#pragma link C++ class TMVA::TActivationTanh+;
#pragma link C++ class TMVA::TActivationRadial+;
#pragma link C++ class TMVA::TNeuronInputChooser+;
#pragma link C++ class TMVA::TNeuronInput+;
#pragma link C++ class TMVA::TNeuronInputSum+;
#pragma link C++ class TMVA::TNeuronInputSqSum+;
#pragma link C++ class TMVA::TNeuronInputAbs+;
#pragma link C++ class TMVA::Types+;
#pragma link C++ class TMVA::Ranking+;
#pragma link C++ class TMVA::RuleFit+;
#pragma link C++ class TMVA::RuleFitAPI+;
#pragma link C++ class TMVA::IMethod+;
#pragma link C++ class TMVA::MsgLogger+;
#pragma link C++ class TMVA::VariableTransformBase+;
 #pragma link C++ class TMVA::VariableIdentityTransform+;
 #pragma link C++ class TMVA::VariableDecorrTransform+;
 #pragma link C++ class TMVA::VariablePCATransform+;
#pragma link C++ class TMVA::Config+;
#pragma link C++ class TMVA::Config::VariablePlotting+;
#pragma link C++ class TMVA::Config::IONames+;
#pragma link C++ class TMVA::KDEKernel+;
#pragma link C++ class TMVA::Interval+;
#pragma link C++ class TMVA::FitterBase+;
#pragma link C++ class TMVA::MCFitter+;
#pragma link C++ class TMVA::GeneticFitter+;
#pragma link C++ class TMVA::SimulatedAnnealingFitter+;
#pragma link C++ class TMVA::MinuitFitter+;
#pragma link C++ class TMVA::MinuitWrapper+;
#pragma link C++ class TMVA::IFitterTarget+;
#pragma link C++ class TMVA::IMetric+;
#pragma link C++ class TMVA::MetricEuler+;
#pragma link C++ class TMVA::MetricManhattan+;
#pragma link C++ class TMVA::SeedDistance+;

#endif
