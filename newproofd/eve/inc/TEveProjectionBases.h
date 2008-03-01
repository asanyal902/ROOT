// @(#)root/eve:$Id$
// Authors: Matevz Tadel & Alja Mrak-Tadel: 2006, 2007

/*************************************************************************
 * Copyright (C) 1995-2007, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#ifndef ROOT_TEveProjectionBases
#define ROOT_TEveProjectionBases

#include "TEveUtil.h"

#include <list>

class TBuffer3D;

class TEveProjected;
class TEveProjectionManager;

////////////////////////////////////////////////////////////////
//                                                            //
// TEveProjectable                                             //
//                                                            //
// Abstract base class for non-linear projectable objects.    //
//                                                            //
////////////////////////////////////////////////////////////////

class TEveProjectable
{
private:
   TEveProjectable(const TEveProjectable&);            // Not implemented
   TEveProjectable& operator=(const TEveProjectable&); // Not implemented

protected:
   std::list<TEveProjected*> fProjectedList; // references to projected instances.

public:
   TEveProjectable();
   virtual ~TEveProjectable();

   virtual TClass* ProjectedClass() const = 0;

   virtual void AddProjected(TEveProjected* p)    { fProjectedList.push_back(p); }
   virtual void RemoveProjected(TEveProjected* p) { fProjectedList.remove(p); }

   ClassDef(TEveProjectable, 0); // Abstract base class for classes that can be transformed with non-linear projections.
};


////////////////////////////////////////////////////////////////
//                                                            //
// TEveProjected                                               //
//                                                            //
// Abstract base class for non-linear projected objects.      //
//                                                            //
////////////////////////////////////////////////////////////////

class TEveProjected
{
private:
   TEveProjected(const TEveProjected&);            // Not implemented
   TEveProjected& operator=(const TEveProjected&); // Not implemented

protected:
   TEveProjectionManager *fProjector;     // manager
   TEveProjectable       *fProjectable;   // link to original object
   Float_t                fDepth;         // z coordinate

public:
   TEveProjected();
   virtual ~TEveProjected();

   virtual void SetProjection(TEveProjectionManager* proj, TEveProjectable* model);
   virtual void UnRefProjectable(TEveProjectable* assumed_parent);

   virtual void SetDepth(Float_t d) { fDepth = d; }

   virtual void UpdateProjection() = 0;

   ClassDef(TEveProjected, 0); // Abstract base class for classes that hold results of a non-linear projection transformation.
};

#endif
