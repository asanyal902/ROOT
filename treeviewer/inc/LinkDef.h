/* @(#)root/treeviewer:$Id: LinkDef.h,v 1.10 2006/07/04 10:16:52 rdm Exp $ */

/*************************************************************************
 * Copyright (C) 1995-2000, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#ifdef __CINT__

#pragma link off all globals;
#pragma link off all classes;
#pragma link off all functions;

#pragma link C++ class TTreeViewer+;
#pragma link C++ class TTVLVContainer+;
#pragma link C++ class TTVLVEntry+;
#pragma link C++ class TGSelectBox+;
#pragma link C++ class TGItemContext+;
#pragma link C++ class TTVRecord+;
#pragma link C++ class TTVSession+;

#endif
