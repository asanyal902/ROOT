//--------------------------------------------------------------------*- C++ -*-
// CLING - the C++ LLVM-based InterpreterG :)
// version: $Id: Display.h 47851 2012-12-05 13:20:58Z tpochep $
// author:  Timur Pocheptsov <Timur.Pocheptsov@cern.ch>
//------------------------------------------------------------------------------

#ifndef CLING_DISPLAY_H
#define CLING_DISPLAY_H

#include <string>

namespace llvm {
  class raw_ostream;
}

namespace cling {
class Interpreter;

void DisplayClasses(llvm::raw_ostream &stream,
                    const Interpreter *interpreter, bool verbose);
void DisplayClass(llvm::raw_ostream &stream, 
                  const Interpreter *interpreter, const char *className, 
                  bool verbose);

void DisplayGlobals(llvm::raw_ostream &stream, const Interpreter *interpreter);
void DisplayGlobal(llvm::raw_ostream &stream, const Interpreter *interpreter, 
                   const std::string &name);

void DisplayTypedefs(llvm::raw_ostream &stream, const Interpreter *interpreter);
void DisplayTypedef(llvm::raw_ostream &stream, const Interpreter *interpreter,
                    const std::string &name);
   
}

#endif
