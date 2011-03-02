//------------------------------------------------------------------------------
// CLING - the C++ LLVM-based InterpreterG :)
// version: $Id$
// author:  Axel Naumann <axel@cern.ch>
//------------------------------------------------------------------------------

#ifndef CLING_INCREMENTAL_AST_PARSER_H
#define CLING_INCREMENTAL_AST_PARSER_H

#include "clang/Basic/SourceLocation.h"
#include "llvm/ADT/OwningPtr.h"
#include "llvm/ADT/StringRef.h"
#include <vector>

namespace clang {
  class CompilerInstance;
  class Decl;
  class Parser;
  class Sema;
  class ASTConsumer;
  class PragmaNamespace;
  class SourceLocation;
  class FileID;
}

namespace cling {
  class MutableMemoryBuffer;
  class ChainedASTConsumer;
  class ASTTransformVisitor;
  class Interpreter;

  class IncrementalASTParser {
  public:
    IncrementalASTParser(clang::CompilerInstance* CI,
                         clang::PragmaNamespace* Pragma,
                         Interpreter* Interp);
    ~IncrementalASTParser();
    void Initialize();
    clang::CompilerInstance* getCI() const { return m_CI.get(); }
    clang::Parser* getParser() const { return m_Parser.get(); }
    clang::CompilerInstance* parse(llvm::StringRef src,
      clang::ASTConsumer* Consumer = 0);
    
    MutableMemoryBuffer* getCurBuffer() {
       return m_MemoryBuffer.back();
    }
   
   ASTTransformVisitor *getTransformer() const { return m_Transformer.get(); }

   void emptyLastFunction();
   clang::Decl* getLastTopLevelDecl() const { return m_LastTopLevelDecl; }
   clang::Decl* getFirstTopLevelDecl() const { return m_FirstTopLevelDecl; }

   void addConsumer(clang::ASTConsumer* consumer);
   void removeConsumer(clang::ASTConsumer* consumer);
  

  private:
    // Should be removed because breaks the encapsulation
    Interpreter* m_Interpreter;  // needed by the m_Transformer
    llvm::OwningPtr<clang::CompilerInstance> m_CI; // compiler instance.
    clang::Sema *m_Sema; // sema used for parsing (owned by CI)
    llvm::OwningPtr<clang::Parser> m_Parser; // parser (incremental)
    llvm::OwningPtr<ASTTransformVisitor> m_Transformer; // delayed id lookup
    std::vector<MutableMemoryBuffer*> m_MemoryBuffer; // One buffer for each command line, owner by the source file manager
    clang::FileID m_MBFileID; // file ID of the memory buffer
    ChainedASTConsumer* m_Consumer; // CI owns it
    clang::Decl* m_LastTopLevelDecl; // last top level decl after most recent call to parse()
    clang::Decl* m_FirstTopLevelDecl; // first top level decl
  };
}
#endif // CLING_INCREMENTAL_AST_PARSER_H
