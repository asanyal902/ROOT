// @(#)root/tmva $Id$    
// Author: Andrzej Zemla

/**********************************************************************************
 * Project: TMVA - a Root-integrated toolkit for multivariate data analysis       *
 * Package: TMVA                                                                  *
 * Class  : SVWorkingSet                                                          *
 * Web    : http://tmva.sourceforge.net                                           *
 *                                                                                *
 * Description:                                                                   *
 *      Working class for Support Vector Machine                                  *
 *                                                                                *
 * Authors (alphabetical):                                                        *
 *      Marcin Wolter  <Marcin.Wolter@cern.ch> - IFJ PAN, Krakow, Poland          *
 *      Andrzej Zemla  <azemla@cern.ch>        - IFJ PAN, Krakow, Poland          *
 *      (IFJ PAN: Henryk Niewodniczanski Inst. Nucl. Physics, Krakow, Poland)     *   
 *                                                                                *
 * Copyright (c) 2005:                                                            *
 *      CERN, Switzerland                                                         * 
 *      MPI-K Heidelberg, Germany                                                 * 
 *      PAN, Krakow, Poland                                                       *
 *                                                                                *
 * Redistribution and use in source and binary forms, with or without             *
 * modification, are permitted according to the terms listed in LICENSE           *
 * (http://tmva.sourceforge.net/LICENSE)                                          *
 **********************************************************************************/

#ifndef ROOT_TMVA_SVWorkingSet
#define ROOT_TMVA_SVWorkingSet

#include <vector>

#ifndef ROOT_Rtypes
#include "Rtypes.h"
#endif

namespace TMVA {

   class SVEvent;
   class SVKernelMatrix;
   class SVKernelFunction;
   class MsgLogger;
        
   class SVWorkingSet {

   public:

      SVWorkingSet();
      SVWorkingSet( std::vector<TMVA::SVEvent*>*, SVKernelFunction*, Float_t , Bool_t);
      ~SVWorkingSet();
                
      Bool_t  ExamineExample( SVEvent*);
      Bool_t  TakeStep      ( SVEvent*, SVEvent*);
      Bool_t  Terminated();
      void    Train();
      void    PrintStat();
      std::vector<TMVA::SVEvent*>* GetSupportVectors();
      Float_t GetBpar() {return 0.5*(fB_low + fB_up);}
      
      //for regression
      Bool_t ExamineExampleReg(SVEvent*);
      Bool_t TakeStepReg(SVEvent*, SVEvent*);
      Bool_t IsDiffSignificant(Float_t, Float_t, Float_t);
      void   TrainReg();
         
                
   private:

      Bool_t fdoRegression;                      //TODO temporary, find nicer solution 
      std::vector<TMVA::SVEvent*> *fInputData;  // documentation
      std::vector<TMVA::SVEvent*> *fSupVec;     // documentation
      SVKernelFunction            *fKFunction;  // documentation
      SVKernelMatrix              *fKMatrix;    // documentation

      SVEvent                     *fTEventUp;   // documentation
      SVEvent                     *fTEventLow;  // documentation
         	
      Float_t                     fB_low;       // documentation
      Float_t                     fB_up;        // documentation
      Float_t                     fTolerance;   // documentation
      
      mutable MsgLogger*          fLogger;      //! message logger
      
      void SetIndex( TMVA::SVEvent* );
   };
}

#endif //ROOT_TMVA_SVWorkingSet
