// @(#)root/metautils:$Id$
// Author: Axel Naumann, Nov 2011

/*************************************************************************
 * Copyright (C) 1995-2011, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#ifndef ROOT_TMetaUtils
#define ROOT_TMetaUtils

namespace clang {
   class ASTContext;
   class QualType;
}

#include <string>

namespace clang {
   class CompilerInstance;
   class Module;
   class Type;
}

namespace cling {
   class Interpreter;
   class LookupHelper;
}

#include "llvm/ADT/SmallSet.h"

namespace ROOT {
   namespace TMetaUtils {
      
      class TNormalizedCtxt {
         typedef llvm::SmallSet<const clang::Type*, 4> TypesCont_t; 
      private:
         TypesCont_t fTypeToSkip;
      public:
         TNormalizedCtxt(const cling::LookupHelper &lh);

         const TypesCont_t &GetTypeToSkip() const { return fTypeToSkip; }
      };

      // Add default template parameters.
      clang::QualType AddDefaultParameters(clang::QualType instanceType, const cling::Interpreter &interpret, const TNormalizedCtxt &normCtxt);

      // Return the -I needed to find RuntimeUniverse.h
      std::string GetInterpreterExtraIncludePath(bool rootbuild);

      // Return the LLVM / clang resource directory
      std::string GetLLVMResourceDir(bool rootbuild);
      
      // Return the ROOT include directory
      std::string GetROOTIncludeDir(bool rootbuild);

      // Return (in the argument 'output') a mangled version of the C++ symbol/type (pass as 'input')
      // that can be used in C++ as a variable name.
      void GetCppName(std::string &output, const char *input);

      // Return the type name normalized for ROOT,
      // keeping only the ROOT opaque typedef (Double32_t, etc.) and
      // adding default template argument for all types except the STL collections
      // where we remove the default template argument if any.
      void GetNormalizedName(std::string &norm_name, const clang::QualType &type, const cling::Interpreter &interpreter, const TNormalizedCtxt &normCtxt);

   }; // class TMetaUtils

} // namespace ROOT

#endif // ROOT_TMetaUtils
