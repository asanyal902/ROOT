// @(#)root/vmc:$Name:  $:$Id: TVirtualMCApplication.h,v 1.6 2006/08/24 16:31:21 rdm Exp $
// Author: Ivana Hrivnacova, 23/03/2002

/*************************************************************************
 * Copyright (C) 2006, Rene Brun and Fons Rademakers.                    *
 * Copyright (C) 2002, ALICE Experiment at CERN.                         *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#ifndef ROOT_TVirtualMCApplication
#define ROOT_TVirtualMCApplication
//
// Class TVirtualMCApplication
// ---------------------------
// Interface to a user Monte Carlo application.
//

#ifndef ROOT_TNamed
#include "TNamed.h"
#endif
#ifndef ROOT_TMath
#include "TMath.h"
#endif

class TVirtualMCApplication : public TNamed {

public:
   // Standard constructor
   TVirtualMCApplication(const char *name, const char *title);

   // Default constructor
   TVirtualMCApplication();

   // Destructor
   virtual ~TVirtualMCApplication();

   // Static access method
   static TVirtualMCApplication* Instance();

   //
   // methods
   //

   // Construct user geometry
   virtual void ConstructGeometry() = 0;

   // Define parameters for optical processes (optional)
   virtual void ConstructOpGeometry() {}

   // Initialize geometry
   // (Usually used to define sensitive volumes IDs)
   virtual void InitGeometry() = 0;

   // Add user defined particles (optional)
   virtual void AddParticles() {}

   // Generate primary particles
   virtual void GeneratePrimaries() = 0;

   // Define actions at the beginning of the event
   virtual void BeginEvent() = 0;

   // Define actions at the beginning of the primary track
   virtual void BeginPrimary() = 0;

   // Define actions at the beginning of each track
   virtual void PreTrack() = 0;

   // Define action at each step
   virtual void Stepping() = 0;

   // Define actions at the end of each track
   virtual void PostTrack() = 0;

   // Define actions at the end of the primary track
   virtual void FinishPrimary() = 0;

   // Define actions at the end of the event
   virtual void FinishEvent() = 0;

   // Define maximum radius for tracking (optional)
   virtual Double_t TrackingRmax() const { return DBL_MAX; }

   // Define maximum z for tracking (optional)
   virtual Double_t TrackingZmax() const { return DBL_MAX; }

   // Calculate user field \a b at point \a x
   virtual void     Field(const Double_t* x, Double_t* b) const = 0;

private:
   // static data members
   static TVirtualMCApplication* fgInstance; // singleton instance

   ClassDef(TVirtualMCApplication,1)  //Interface to MonteCarlo application
};

// inline methods
inline TVirtualMCApplication* TVirtualMCApplication::Instance()
{ return fgInstance; }

#endif //ROOT_TVirtualMCApplication

