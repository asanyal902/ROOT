// @(#)root/reflex:$Id$
// Author: Axel Naumann, 2008

// Copyright CERN, CH-1211 Geneva 23, 2004-2008, All rights reserved.
//
// Permission to use, copy, modify, and distribute this software for any
// purpose is hereby granted without fee, provided that this copyright and
// permissions notice appear in all copies and derivatives.
//
// This software is provided "as is" without express or implied warranty.

#ifndef Reflex_TemplateArgument
#define Reflex_TemplateArgument

#include "Reflex/ValueObject.h"

namespace Reflex {
   ////////////////////////////////////////////////////////////////////////////////
   // TemplateArgument represents an argument of a template instance, or a
   // default value of a template. It can be either an int, an enum constant, or a type.
   ////////////////////////////////////////////////////////////////////////////////

   class TemplateArgument {
   public:
      enum EKindOf {
         kType, // template parameter of type "typename / class / template"
         kValue // non-type template parameter
      };

      TemplateArgument(Type type): fKindOf(kType), fValue(type, 0) {}
      TemplateArgument(const ValueObject& value): fKindOf(kValue), fValue(value) {}

      EKindOf KindOf() const { return fKindOf; }
      const ValueObject& AsValue() const { return fValue; }
      Type               AsType() const { return fType; }

   private:
      EKindOf     fKindOf; // type of template parameter
      ValueObject fValue;  // value for kValue; also used to store the type for kType.
   };
} // namespace Reflex;
