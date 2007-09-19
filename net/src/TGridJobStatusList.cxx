// @(#)root/net:$Id: TGridJobStatusList.cxx,v 1.1 2007/03/19 16:14:15 rdm Exp $
// Author: Andreas-Joachim Peters  10/12/2006

/*************************************************************************
 * Copyright (C) 1995-2006, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// TGridJobStatusList                                                   //
//                                                                      //
// Abstract base class defining a list of GRID job status               //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

#include "TGridJobStatusList.h"

TGridJobStatusList *gGridJobStatusList = 0;


