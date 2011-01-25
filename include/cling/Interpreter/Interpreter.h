//--------------------------------------------------------------------*- C++ -*-
// CLING - the C++ LLVM-based InterpreterG :)
// version: $Id$
// author:  Lukasz Janyst <ljanyst@cern.ch>
//------------------------------------------------------------------------------

#ifndef CLING_INTERPRETER_H
#define CLING_INTERPRETER_H

#include "llvm/ADT/OwningPtr.h"
#include "llvm/ExecutionEngine/GenericValue.h"

#include <string>

namespace llvm {
  class raw_ostream;
}

namespace clang {
  class ASTConsumer;
  class CompilerInstance;
  class PragmaNamespace;
  class SourceLocation;
  class Decl;
}

namespace cling {
  
  class CIBuilder;
  class ExecutionContext;
  class IncrementalASTParser;
  class InputValidator;
  
  //---------------------------------------------------------------------------
  //! Class for managing many translation units supporting automatic
  //! forward declarations and linking
  //---------------------------------------------------------------------------
  class Interpreter {
  public:
    
    //---------------------------------------------------------------------
    //! Constructor
    //---------------------------------------------------------------------
    Interpreter(const char* llvmdir = 0);
    
    //---------------------------------------------------------------------
    //! Destructor
    //---------------------------------------------------------------------
    virtual ~Interpreter();

    const char* getVersion() const;
    
    void AddIncludePath(const char *path);
	// Using static to avoid creating a Member call, which needs this pointer
    template<typename T>
    static T Eval(size_t This, const char* expr, void* varaddr[] ) {
       llvm::GenericValue result(((Interpreter*) (void*)This)->EvalCore(expr, varaddr));
       //FIXME: we should return T calculated from result
       //printf("%s", expr);
       return T();
    }
     
    int processLine(const std::string& input_line);
    
    int loadFile(const std::string& filename,
                 const std::string* trailcode = 0,
                 bool allowSharedLib = true);
    
    int executeFile(const std::string& filename);
    
    
    bool setPrintAST(bool print = true) {
      bool prev = m_printAST;
      m_printAST = print;
      return prev;
    }
 
    void dumpAST(bool showAST = true, int last = -1);
    
    clang::CompilerInstance* getCI() const;
    clang::CompilerInstance* createCI() const;
    CIBuilder& getCIBuilder() const { return *m_CIBuilder.get(); }
    
    clang::PragmaNamespace& getPragmaHandler() const { return *m_PragmaHandler; }
    void installLazyFunctionCreator(void* (*fp)(const std::string&));

    llvm::raw_ostream& getValuePrinterStream() const { return *m_ValuePrintStream; }

    void RequestContinuation(const clang::SourceLocation&);
    
  private:
    llvm::OwningPtr<cling::CIBuilder> m_CIBuilder; // our compiler intsance builder
    llvm::OwningPtr<ExecutionContext> m_ExecutionContext; // compiler instance.
    llvm::OwningPtr<IncrementalASTParser> m_IncrASTParser; // incremental AST and its parser
    llvm::OwningPtr<InputValidator> m_InputValidator; // balanced paren etc
    clang::PragmaNamespace* m_PragmaHandler; // pragma cling ..., owned by Preprocessor
    unsigned long long m_UniqueCounter; // number of generated call wrappers
    bool m_printAST; // whether to print the AST to be processed
    llvm::OwningPtr<llvm::raw_ostream> m_ValuePrintStream; // stream to dump values into
    clang::Decl *m_LastDump; // last dump point

  private:
    
    void createWrappedSrc(const std::string& src, std::string& wrapped,
                          std::string& stmtFunc);
    
    std::string createUniqueName();
    
    clang::ASTConsumer* maybeGenerateASTPrinter() const;
    clang::CompilerInstance* compileString(const std::string& srcCode);
    clang::CompilerInstance* compileFile(const std::string& filename,
                                         const std::string* trailcode = 0);
     llvm::GenericValue EvalCore(const char* expr, void* varaddr);
  };
  
} // namespace cling

#endif // CLING_INTERPRETER_H
