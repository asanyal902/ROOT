// @(#)root/proof:$Name:  $:$Id: TProofSuperMaster.h,v 1.6 2007/03/16 17:06:19 rdm Exp $
// Author: Fons Rademakers   13/02/97

/*************************************************************************
 * Copyright (C) 1995-2000, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#ifndef ROOT_TProofSuperMaster
#define ROOT_TProofSuperMaster


//////////////////////////////////////////////////////////////////////////
//                                                                      //
// TProofSuperMaster                                                    //
//                                                                      //
// This class controls a Parallel ROOT Facility, PROOF, cluster.        //
// It fires the slave servers, it keeps track of how many slaves are    //
// running, it keeps track of the slaves running status, it broadcasts  //
// messages to all slaves, it collects results, etc.                    //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

#ifndef ROOT_TProof
#include "TProof.h"
#endif
#ifndef ROOT_TString
#include "TString.h"
#endif

class TVirtualProofPlayer;
class TDSet;

class TProofSuperMaster : public TProof {

friend class TProofPlayerSuperMaster;

protected:
   Bool_t    StartSlaves(Bool_t, Bool_t);
   Long64_t  Process(TDSet *set, const char *selector,
                     Option_t *option = "", Long64_t nentries = -1,
                     Long64_t firstentry = 0);
   void      ValidateDSet(TDSet *dset);
   virtual   TVirtualProofPlayer *MakePlayer(const char *player = 0, TSocket *s = 0);

public:
   TProofSuperMaster(const char *masterurl, const char *conffile = kPROOF_ConfFile,
                     const char *confdir = kPROOF_ConfDir, Int_t loglevel = 0);
   virtual ~TProofSuperMaster() { }

   ClassDef(TProofSuperMaster,0) //PROOF control class for making submasters
};

#endif
